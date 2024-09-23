

#include "sysconfig.h"
#include "sysdeps.h"

#define MOVEC_DEBUG 0

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "cpummu.h"
#include "cpummu030.h"
#include "cpu_prefetch.h"

int get_cpu_model(void)
{
	return currprefs.cpu_model;
}

void val_move2c2 (int regno, uae_u32 val)
{
	switch (regno) {
	case 0: regs.sfc = val; break;
	case 1: regs.dfc = val; break;
	case 2: regs.cacr = val; break;
	case 3: regs.tcr = val; break;
	case 4: regs.itt0 = val; break;
	case 5: regs.itt1 = val; break;
	case 6: regs.dtt0 = val; break;
	case 7: regs.dtt1 = val; break;
	case 8: regs.buscr = val; break;
	case 0x800: regs.usp = val; break;
	case 0x801: regs.vbr = val; break;
	case 0x802: regs.caar = val; break;
	case 0x803: regs.msp = val; break;
	case 0x804: regs.isp = val; break;
	case 0x805: regs.mmusr = val; break;
	case 0x806: regs.urp = val; break;
	case 0x807: regs.srp = val; break;
	case 0x808: regs.pcr = val; break;
	}
}

uae_u32 val_move2c (int regno)
{
	switch (regno) {
	case 0: return regs.sfc;
	case 1: return regs.dfc;
	case 2: return regs.cacr;
	case 3: return regs.tcr;
	case 4: return regs.itt0;
	case 5: return regs.itt1;
	case 6: return regs.dtt0;
	case 7: return regs.dtt1;
	case 8: return regs.buscr;
	case 0x800: return regs.usp;
	case 0x801: return regs.vbr;
	case 0x802: return regs.caar;
	case 0x803: return regs.msp;
	case 0x804: return regs.isp;
	case 0x805: return regs.mmusr;
	case 0x806: return regs.urp;
	case 0x807: return regs.srp;
	case 0x808: return regs.pcr;
	default: return 0;
	}
}

#ifndef CPUEMU_68000_ONLY

int movec_illg (int regno)
{
	int regno2 = regno & 0x7ff;

	if (currprefs.cpu_model == 68060) {
		if (regno <= 8)
			return 0;
		if (regno == 0x800 || regno == 0x801 ||
			regno == 0x806 || regno == 0x807 || regno == 0x808)
			return 0;
		return 1;
	} else if (currprefs.cpu_model == 68010) {
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
#if MOVEC_DEBUG > 0
	write_log (_T("move2c %04X <- %08X PC=%x\n"), regno, *regp, M68K_GETPC);
#endif
	if (movec_illg(regno)) {
		if (currprefs.cpu_model < 68060 && !regs.s) {
			Exception(8);
			return 0;
		}
		op_illg(0x4E7B);
		return 0;
	} else {
		if (!regs.s) {
			Exception(8);
			return 0;
		}
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
				else if (currprefs.cpu_model == 68060)
					cacr_mask = 0xf8e0e000;
				regs.cacr = *regp & cacr_mask;
				set_cpu_caches (false);
			}
			break;
			/* 68040/060 only */
		case 3:
			regs.tcr = *regp & (currprefs.cpu_model == 68060 ? 0xfffe : 0xc000);
			if (currprefs.mmu_model)
				regs.tcr = mmu_set_tc (regs.tcr);
			break;

			/* no differences between 68040 and 68060 */
		case 4: regs.itt0 = *regp & 0xffffe364; mmu_tt_modified (); break;
		case 5: regs.itt1 = *regp & 0xffffe364; mmu_tt_modified (); break;
		case 6: regs.dtt0 = *regp & 0xffffe364; mmu_tt_modified (); break;
		case 7: regs.dtt1 = *regp & 0xffffe364; mmu_tt_modified (); break;
			/* 68060 only */
		case 8: regs.buscr &= 0x50000000; regs.buscr |= *regp & 0xa0000000; break;

		case 0x800: regs.usp = *regp; break;
		case 0x801: regs.vbr = *regp; break;
		case 0x802: regs.caar = *regp; break;
		case 0x803: regs.msp = *regp; if (regs.m == 1) m68k_areg (regs, 7) = regs.msp; break;
		case 0x804: regs.isp = *regp; if (regs.m == 0) m68k_areg (regs, 7) = regs.isp; break;
			/* 68040 only */
		case 0x805: regs.mmusr = *regp; break;
			 /* 68040 stores all bits, 68060 zeroes low 9 bits */
		case 0x806: regs.urp = *regp & (currprefs.cpu_model == 68060 ? 0xfffffe00 : 0xffffffff); break;
		case 0x807: regs.srp = *regp & (currprefs.cpu_model == 68060 ? 0xfffffe00 : 0xffffffff); break;
			/* 68060 only */
		case 0x808:
			{
				uae_u32 opcr = regs.pcr;
				regs.pcr &= ~(0x40 | 2 | 1);
				regs.pcr |= (*regp) & (0x40 | 2 | 1);
				if (currprefs.fpu_model <= 0)
					regs.pcr |= 2;
				if (((opcr ^ regs.pcr) & 2) == 2) {
					write_log (_T("68060 FPU state: %s\n"), regs.pcr & 2 ? _T("disabled") : _T("enabled"));
					/* flush possible already translated FPU instructions */
					flush_icache (3);
				}
			}
			break;
		default:
			op_illg (0x4E7B);
			return 0;
		}
	}
	return 1;
}

int m68k_movec2 (int regno, uae_u32 *regp)
{
#if MOVEC_DEBUG > 0
	write_log (_T("movec2 %04X PC=%x\n"), regno, M68K_GETPC);
#endif
	if (movec_illg(regno)) {
		if (currprefs.cpu_model < 68060 && !regs.s) {
			Exception(8);
			return 0;
		}
		op_illg(0x4E7A);
		return 0;
	} else {
		if (!regs.s) {
			Exception(8);
			return 0;
		}
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
				else if (currprefs.cpu_model == 68060)
					cacr_mask = 0xf880e000;
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
		case 0x803: *regp = regs.m == 1 ? m68k_areg (regs, 7) : regs.msp; break;
		case 0x804: *regp = regs.m == 0 ? m68k_areg (regs, 7) : regs.isp; break;
		case 0x805: *regp = regs.mmusr; break;
		case 0x806: *regp = regs.urp; break;
		case 0x807: *regp = regs.srp; break;
		case 0x808: *regp = regs.pcr; break;

		default:
			op_illg (0x4E7A);
			return 0;
		}
	}
#if MOVEC_DEBUG > 0
	write_log (_T("-> %08X\n"), *regp);
#endif
	return 1;
}

#endif


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

uae_u32 REGPARAM2 x_get_bitfield (uae_u32 src, uae_u32 bdata[2], uae_s32 offset, int width)
{
	uae_u32 tmp1, tmp2, res, mask;

	offset &= 7;
	mask = 0xffffffffu << (32 - width);
	switch ((offset + width + 7) >> 3) {
	case 1:
		tmp1 = x_cp_get_byte (src);
		res = tmp1 << (24 + offset);
		bdata[0] = tmp1 & ~(mask >> (24 + offset));
		break;
	case 2:
		tmp1 = x_cp_get_word (src);
		res = tmp1 << (16 + offset);
		bdata[0] = tmp1 & ~(mask >> (16 + offset));
		break;
	case 3:
		tmp1 = x_cp_get_word (src);
		tmp2 = x_cp_get_byte (src + 2);
		res = tmp1 << (16 + offset);
		bdata[0] = tmp1 & ~(mask >> (16 + offset));
		res |= tmp2 << (8 + offset);
		bdata[1] = tmp2 & ~(mask >> (8 + offset));
		break;
	case 4:
		tmp1 = x_cp_get_long (src);
		res = tmp1 << offset;
		bdata[0] = tmp1 & ~(mask >> offset);
		break;
	case 5:
		tmp1 = x_cp_get_long (src);
		tmp2 = x_cp_get_byte (src + 4);
		res = tmp1 << offset;
		bdata[0] = tmp1 & ~(mask >> offset);
		res |= tmp2 >> (8 - offset);
		bdata[1] = tmp2 & ~(mask << (8 - offset));
		break;
	default:
		/* Panic? */
		write_log (_T("x_get_bitfield() can't happen %d\n"), (offset + width + 7) >> 3);
		res = 0;
		break;
	}
	return res;
}

void REGPARAM2 x_put_bitfield (uae_u32 dst, uae_u32 bdata[2], uae_u32 val, uae_s32 offset, int width)
{
	offset = (offset & 7) + width;
	switch ((offset + 7) >> 3) {
	case 1:
		x_cp_put_byte (dst, bdata[0] | (val << (8 - offset)));
		break;
	case 2:
		x_cp_put_word (dst, bdata[0] | (val << (16 - offset)));
		break;
	case 3:
		x_cp_put_word (dst, bdata[0] | (val >> (offset - 16)));
		x_cp_put_byte (dst + 2, bdata[1] | (val << (24 - offset)));
		break;
	case 4:
		x_cp_put_long (dst, bdata[0] | (val << (32 - offset)));
		break;
	case 5:
		x_cp_put_long (dst, bdata[0] | (val >> (offset - 32)));
		x_cp_put_byte (dst + 4, bdata[1] | (val << (40 - offset)));
		break;
	default:
		write_log (_T("x_put_bitfield() can't happen %d\n"), (offset + 7) >> 3);
		break;
	}
}

uae_u32 REGPARAM2 get_disp_ea_020 (uae_u32 base, int idx)
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

uae_u32 REGPARAM2 x_get_disp_ea_020 (uae_u32 base, int idx)
{
	uae_u16 dp = x_next_iword ();
	int reg = (dp >> 12) & 15;
	int cycles = 0;
	uae_u32 v;

	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	regd <<= (dp >> 9) & 3;
	if (dp & 0x100) {

		uae_s32 outer = 0;
		if (dp & 0x80)
			base = 0;
		if (dp & 0x40)
			regd = 0;

		if ((dp & 0x30) == 0x20) {
			base += (uae_s32)(uae_s16) x_next_iword ();
			cycles++;
		}
		if ((dp & 0x30) == 0x30) {
			base += x_next_ilong ();
			cycles++;
		}

		if ((dp & 0x3) == 0x2) {
			outer = (uae_s32)(uae_s16) x_next_iword ();
			cycles++;
		}
		if ((dp & 0x3) == 0x3) {
			outer = x_next_ilong ();
			cycles++;
		}

		if ((dp & 0x4) == 0) {
			base += regd;
			cycles++;
		}
		if (dp & 0x3) {
			base = x_get_long (base);
			cycles++;
		}
		if (dp & 0x4) {
			base += regd;
			cycles++;
		}
		v = base + outer;
	} else {
		v = base + (uae_s32)((uae_s8)dp) + regd;
	}
#if 0
	if (cycles && currprefs.cpu_cycle_exact)
		x_do_cycles (cycles * cpucycleunit);
#endif
	return v;
}

#ifndef CPU_TESTER

uae_u32 REGPARAM2 x_get_disp_ea_ce030 (uae_u32 base, int idx)
{
	uae_u16 dp = next_iword_030ce ();
	int reg = (dp >> 12) & 15;
	uae_u32 v;

	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	regd <<= (dp >> 9) & 3;
	if (dp & 0x100) {
		uae_s32 outer = 0;
		if (dp & 0x80)
			base = 0;
		if (dp & 0x40)
			regd = 0;

		if ((dp & 0x30) == 0x20) {
			base += (uae_s32)(uae_s16) next_iword_030ce ();
		}
		if ((dp & 0x30) == 0x30) {
			base += next_ilong_030ce ();
		}

		if ((dp & 0x3) == 0x2) {
			outer = (uae_s32)(uae_s16) next_iword_030ce ();
		}
		if ((dp & 0x3) == 0x3) {
			outer = next_ilong_030ce ();
		}

		if ((dp & 0x4) == 0) {
			base += regd;
		}
		if (dp & 0x3) {
			base = x_get_long (base);
		}
		if (dp & 0x4) {
			base += regd;
		}
		v = base + outer;
	} else {
		v = base + (uae_s32)((uae_s8)dp) + regd;
	}
	return v;
}

uae_u32 REGPARAM2 x_get_disp_ea_ce020 (uae_u32 base, int idx)
{
	uae_u16 dp = next_iword_020ce ();
	int reg = (dp >> 12) & 15;
	uae_u32 v;

	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	regd <<= (dp >> 9) & 3;
	if (dp & 0x100) {
		uae_s32 outer = 0;
		if (dp & 0x80)
			base = 0;
		if (dp & 0x40)
			regd = 0;

		if ((dp & 0x30) == 0x20) {
			base += (uae_s32)(uae_s16) next_iword_020ce ();
		}
		if ((dp & 0x30) == 0x30) {
			base += next_ilong_020ce ();
		}

		if ((dp & 0x3) == 0x2) {
			outer = (uae_s32)(uae_s16) next_iword_020ce ();
		}
		if ((dp & 0x3) == 0x3) {
			outer = next_ilong_020ce ();
		}

		if ((dp & 0x4) == 0) {
			base += regd;
		}
		if (dp & 0x3) {
			base = x_get_long (base);
		}
		if (dp & 0x4) {
			base += regd;
		}
		v = base + outer;
	} else {
		v = base + (uae_s32)((uae_s8)dp) + regd;
	}
	return v;
}

uae_u32 REGPARAM2 x_get_disp_ea_040(uae_u32 base, int idx)
{
	uae_u16 dp = next_iword_cache040();
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
			base += (uae_s32)(uae_s16)next_iword_cache040();
		if ((dp & 0x30) == 0x30)
			base += next_ilong_cache040();

		if ((dp & 0x3) == 0x2)
			outer = (uae_s32)(uae_s16)next_iword_cache040();
		if ((dp & 0x3) == 0x3)
			outer = next_ilong_cache040();

		if ((dp & 0x4) == 0)
			base += regd;
		if (dp & 0x3)
			base = x_get_long(base);
		if (dp & 0x4)
			base += regd;

		return base + outer;
	}
	else {
		return base + (uae_s32)((uae_s8)dp) + regd;
	}
}

#endif


int getMulu68kCycles(uae_u16 src)
{
	int cycles = 0;
	if (currprefs.cpu_model == 68000) {
		cycles = 38 - 4;
		for (int bits = 0; bits < 16 && src; bits++, src >>= 1) {
			if (src & 1)
				cycles += 2;
		}
	} else {
		cycles = 40 - 4;
	}
	return cycles;
}

int getMuls68kCycles(uae_u16 src)
{
	int cycles;
	if (currprefs.cpu_model == 68000) {
		cycles = 38 - 4;
		uae_u32 usrc = ((uae_u32)src) << 1;
		for (int bits = 0; bits < 16 && usrc; bits++, usrc >>= 1) {
			if ((usrc & 3) == 1 || (usrc & 3) == 2) {
				cycles += 2;
			}
		}
	} else {
		cycles = 40 - 4;
		// 2 extra cycles added if source is negative
		if (src & 0x8000)
			cycles += 2;
	}
	return cycles;
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


DIVU 68000:

Overflow (always): 10 cycles.
Worst case: 136 cycles.
Best case: 76 cycles.

DIVU 68010:

Overflow (always): 8 cycles.
Wost case: 108 cycles.
Best case: 78 cycles.

DIVS 68000:

Absolute overflow: 16-18 cycles.
Signed overflow is not detected prematurely.

Worst case: 156 cycles.
Best case without signed overflow: 122 cycles.
Best case with signed overflow: 120 cycles

DIVS 68010:

Absolute overflow: 16 cycles.
Signed overflow is not detected prematurely.

Worst case: 122 cycles.
Best case: 120 cycles.

*/

int getDivu68kCycles (uae_u32 dividend, uae_u16 divisor)
{
	int mcycles;
	uae_u32 hdivisor;
	int i;

	if (divisor == 0)
		return 0;

	if (currprefs.cpu_model == 68010) {

		// Overflow
		if ((dividend >> 16) >= divisor) {
			return 4;
		}

		mcycles = 74;

		hdivisor = divisor << 16;

		for (i = 0; i < 15; i++) {
			uae_u32 temp;
			temp = dividend;

			dividend <<= 1;

			// If carry from shift
			if ((uae_s32)temp < 0) {
				dividend -= hdivisor;
			} else {
				mcycles += 2;
				if (dividend >= hdivisor) {
					dividend -= hdivisor;
				}
			}
		}
		return mcycles;

	} else {

		// Overflow
		if ((dividend >> 16) >= divisor)
			return (mcycles = 5) * 2 - 4;

		mcycles = 38;

		hdivisor = divisor << 16;

		for (i = 0; i < 15; i++) {
			uae_u32 temp;
			temp = dividend;

			dividend <<= 1;

			// If carry from shift
			if ((uae_s32)temp < 0) {
				dividend -= hdivisor;
			} else {
				mcycles += 2;
				if (dividend >= hdivisor) {
					dividend -= hdivisor;
					mcycles--;
				}
			}
		}
		// -4 = remove prefetch cycle
		return mcycles * 2 - 4;
	}
}

int getDivs68kCycles (uae_s32 dividend, uae_s16 divisor, int *extra)
{
	int mcycles;
	uae_u32 aquot;
	int i;

	if (divisor == 0) {
		return 0;
	}

	if (currprefs.cpu_model == 68010) {
		// Check for absolute overflow
		if (((uae_u32)abs(dividend) >> 16) >= (uae_u16)abs(divisor))
			return 12;
		mcycles = 116;
		// add 2 extra cycles if negative dividend
		if (dividend < 0) {
			mcycles += 2;
			*extra = 1;
		}
		return mcycles;
	}

	mcycles = 6;

	if (dividend < 0)
		mcycles++;

	// Check for absolute overflow
	if (((uae_u32)abs (dividend) >> 16) >= (uae_u16)abs (divisor))
		return (mcycles + 2) * 2 - 4;

	// report special case where IPL check is 2 cycles earlier
	*extra = (divisor < 0 && dividend >= 0) ? 1 : 0;

	// Absolute quotient
	aquot = (uae_u32) abs (dividend) / (uae_u16)abs (divisor);

	mcycles += 55;

	if (divisor >= 0) {
		if (dividend >= 0)
			mcycles--;
		else
			mcycles++;
	}

	// Count 15 msbits in absolute of quotient

	for (i = 0; i < 15; i++) {
		if ((uae_s16)aquot >= 0)
			mcycles++;
		aquot <<= 1;
	}

	return mcycles * 2 - 4;
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
		CLEAR_CZNV();
		if (issigned) {
			SET_ZFLG(1);
		} else {
			uae_s16 d = dst >> 16;
			if (d < 0)
				SET_NFLG(1);
			else if (d == 0)
				SET_ZFLG(1);
			SET_VFLG(1);
		}
	} else if (currprefs.cpu_model == 68040) {
		SET_CFLG(0);
	} else if (currprefs.cpu_model == 68060) {
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
 * 68000: V=1, N=1, C=0, Z=0
 * 68010: V=1, N=(dividend >=0 or divisor >= 0), C=0, Z=0
 * 68020: V=1, C=0, Z=0, N=X
 * 68040: V=1, C=0, NZ not modified.
 * 68060: V=1, C=0, NZ not modified.
 *
 * X) N is set if original 32-bit destination value is negative.
 *
 */

void setdivuflags(uae_u32 dividend, uae_u16 divisor)
{
	if (currprefs.cpu_model == 68060) {
		SET_VFLG(1);
		SET_CFLG(0);
	} else if (currprefs.cpu_model == 68040) {
		SET_VFLG(1);
		SET_CFLG(0);
	} else if (currprefs.cpu_model >= 68020) {
		SET_VFLG(1);
		if ((uae_s32)dividend < 0)
			SET_NFLG(1);
	} else if (currprefs.cpu_model == 68010) {
		SET_VFLG(1);
		if ((uae_s32)dividend < 0 && (uae_s16)divisor < 0) {
			SET_NFLG(0);
		} else {
			SET_NFLG(1);
		}
		SET_ZFLG(0);
		SET_CFLG(0);
	} else {
		// 68000
		SET_VFLG(1);
		SET_NFLG(1);
		SET_ZFLG(0);
		SET_CFLG(0);
	}
}

/*
 * DIVS overflow
 *
 * 68000: V=1, C=0, N=1, Z=0
 * 68010: V=1, C=0, N=0, Z=
 * 68020: V=1, C=0, ZN = X
 * 68040: V=1, C=0. NZ not modified.
 * 68060: V=1, C=0, NZ not modified.
 *
 * X) if absolute overflow(Check getDivs68kCycles for details) : Z = 0, N = 0
 * if not absolute overflow : N is set if internal result BYTE is negative, Z is set if it is zero!
 *
 */

void setdivsflags(uae_s32 dividend, uae_s16 divisor)
{
	if (currprefs.cpu_model == 68060) {
		SET_VFLG(1);
		SET_CFLG(0);
	} else if (currprefs.cpu_model == 68040) {
		SET_VFLG(1);
		SET_CFLG(0);
	} else if (currprefs.cpu_model >= 68020) {
		CLEAR_CZNV();
		SET_VFLG(1);
		// absolute overflow?
		if (((uae_u32)abs(dividend) >> 16) >= (uae_u16)abs(divisor))
			return;
		uae_u32 aquot = (uae_u32)abs(dividend) / (uae_u16)abs(divisor);
		if ((uae_s8)aquot == 0)
			SET_ZFLG(1);
		if ((uae_s8)aquot < 0)
			SET_NFLG(1);
	} else if (currprefs.cpu_model == 68010) {
		CLEAR_CZNV();
		SET_VFLG(1);
		SET_NFLG(1);
	} else {
		// 68000
		CLEAR_CZNV();
		SET_VFLG(1);
		SET_NFLG(1);
	}
}

/*
 * CHK undefined flags
 *
 * 68000: CV=0. Z: dst==0. N: dst < 0. !N: dst > src.
 * 68020: Z: dst==0. N: dst < 0. V: src-dst overflow. C: if dst < 0: (dst > src || src >= 0), if dst > src: (src >= 0).
 * 68040: C=0. C=1 if exception and (dst < 0 && src >= 0) || (src >= 0 && dst >= src) || (dst < 0 && src < dst)
 * 68060: N=0. If exception: N=dst < 0
 *
 */
void setchkundefinedflags(uae_s32 src, uae_s32 dst, int size)
{
	if (currprefs.cpu_model < 68020) {
		CLEAR_CZNV();
		if (dst == 0)
			SET_ZFLG(1);
		if (dst < 0)
			SET_NFLG(1);
		else if (dst > src)
			SET_NFLG(0);
	} else if (currprefs.cpu_model == 68020 || currprefs.cpu_model == 68030) {
		CLEAR_CZNV();
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
	} else if (currprefs.cpu_model == 68040) {
		SET_CFLG(0);
		if (dst < 0 || dst > src) {
			if (dst < 0 && src >= 0) {
				SET_CFLG(1);
			} else if (src >= 0 && dst >= src) {
				SET_CFLG(1);
			} else if (dst < 0 && src < dst) {
				SET_CFLG(1);
			}
		}
		SET_NFLG(dst < 0);
	} else if (currprefs.cpu_model == 68060) {
		SET_NFLG(0);
		if (dst < 0 || dst > src) {
			SET_NFLG(dst < 0);
		}
	}
}

/*
 * CHK2/CMP2 undefined flags
 *
 * 68020-68030: See below..
 * 68040: NV not modified.
 * 68060: N: val<0 V=0
 *
 */

// This is the complex one.
// Someone else can attempt to simplify this..
void setchk2undefinedflags(uae_s32 lower, uae_s32 upper, uae_s32 val, int size)
{
	if (currprefs.cpu_model == 68060) {
		SET_VFLG(0);
		SET_NFLG(val < 0);
		return;
	} else if (currprefs.cpu_model == 68040) {
		return;
	}

	SET_NFLG(0);
	SET_VFLG(0);

	if (val == lower || val == upper)
		return;

	if (lower < 0 && upper >= 0) {
		if (val < lower) {
			SET_NFLG(1);
		}
		if (val >= 0 && val < upper) {
			SET_NFLG(1);
		}
		if (val >= 0 && lower - val >= 0) {
			SET_VFLG(1);
			SET_NFLG(0);
			if (val > upper) {
				SET_NFLG(1);
			}
		}
	} else if (lower >= 0 && upper < 0) {
		if (val >= 0) {
			SET_NFLG(1);
		}
		if (val > upper) {
			SET_NFLG(1);
		}
		if (val > lower && upper - val >= 0) {
			SET_VFLG(1);
			SET_NFLG(0);
		}
	} else if (lower >= 0 && upper >= 0 && lower > upper) {
		if (val > upper && val < lower) {
			SET_NFLG(1);
		}
		if (val < 0 && lower - val < 0) {
			SET_VFLG(1);
		}
		if (val < 0 && lower - val >= 0) {
			SET_NFLG(1);
		}
	} else if (lower >= 0 && upper >= 0 && lower <= upper) {
		if (val >= 0 && val < lower) {
			SET_NFLG(1);
		}
		if (val > upper) {
			SET_NFLG(1);
		}
		if (val < 0 && upper - val < 0) {
			SET_VFLG(1);
			SET_NFLG(1);
		}
	} else if (lower < 0 && upper < 0 && lower > upper) {
		if (val >= 0) {
			SET_NFLG(1);
		}
		if (val > upper && val < lower) {
			SET_NFLG(1);
		}
		if (val >= 0 && val - lower < 0) {
			SET_NFLG(0);
			SET_VFLG(1);
		}
	} else if (lower < 0 && upper < 0 && lower <= upper) {
		if (val < lower) {
			SET_NFLG(1);
		}
		if (val < 0 && val > upper) {
			SET_NFLG(1);
		}
		if (val >= 0 && val - lower < 0) {
			SET_NFLG(1);
			SET_VFLG(1);
		}
	}
}

#ifndef CPUEMU_68000_ONLY

static void divsl_overflow(uae_u16 extra, uae_s64 a, uae_s32 divider)
{
	if (currprefs.cpu_model >= 68040) {
		SET_VFLG(1);
		SET_CFLG(0);
	} else {
		uae_s32 a32 = (uae_s32)a;
		bool neg64 = a < 0;
		bool neg32 = a32 < 0;
		SET_VFLG(1);
		if (extra & 0x0400) {
			// this is still missing condition where Z is set
			// without none of input parameters being zero.
			uae_s32 ahigh = a >> 32;
			if (ahigh == 0) {
				SET_ZFLG(1);
				SET_NFLG(0);
			} else if (ahigh < 0 && divider < 0 && ahigh > divider) {
				SET_ZFLG(0);
				SET_NFLG(0);
			} else {
				if (a32 == 0) {
					SET_ZFLG(1);
					SET_NFLG(0);
				} else {
					SET_ZFLG(0);
					SET_NFLG(neg32 ^ neg64);
				}
			}
		} else {
			if (a32 == 0) {
				SET_ZFLG(1);
				SET_NFLG(0);
			} else {
				SET_NFLG(neg32);
				SET_ZFLG(0);
			}
		}
		SET_CFLG(0);
	}
}

static void divul_overflow(uae_u16 extra, uae_s64 a)
{
	if (currprefs.cpu_model >= 68040) {
		SET_VFLG(1);
		SET_CFLG(0);
	} else {
		uae_s32 a32 = (uae_s32)a;
		bool neg32 = a32 < 0;
		SET_VFLG(1);
		SET_NFLG(neg32);
		SET_ZFLG(a32 == 0);
		SET_CFLG(0);
	}
}

static void divsl_divbyzero(uae_u16 extra, uae_s64 a, uaecptr oldpc)
{
	if (currprefs.cpu_model >= 68040) {
		SET_CFLG(0);
	} else {
		SET_NFLG(0);
		SET_ZFLG(1);
		SET_CFLG(0);
	}
	Exception_cpu_oldpc(5, oldpc);
}

static void divul_divbyzero(uae_u16 extra, uae_s64 a, uaecptr oldpc)
{
	if (currprefs.cpu_model >= 68040) {
		SET_CFLG(0);
	} else {
		uae_s32 a32 = (uae_s32)a;
		bool neg32 = a32 < 0;
		SET_NFLG(neg32);
		SET_ZFLG(a32 == 0);
		SET_VFLG(1);
		SET_CFLG(0);
	}
	Exception_cpu_oldpc(5, oldpc);
}

int m68k_divl(uae_u32 opcode, uae_u32 src, uae_u16 extra, uaecptr oldpc)
{
	if ((extra & 0x400) && currprefs.int_no_unimplemented && currprefs.cpu_model == 68060) {
		return -1;
	}

	if (extra & 0x800) {
		/* signed variant */
		uae_s64 a = (uae_s64)(uae_s32)m68k_dreg (regs, (extra >> 12) & 7);
		uae_s64 quot, rem;

		if (extra & 0x400) {
			a &= 0xffffffffu;
			a |= (uae_s64)m68k_dreg (regs, extra & 7) << 32;
		}

		if (src == 0) {
			divsl_divbyzero(extra, a, oldpc);
			return 0;
		}

		if ((uae_u64)a == 0x8000000000000000UL && src == ~0u) {
			divsl_overflow(extra, a, src);
		} else {
			rem = a % (uae_s64)(uae_s32)src;
			quot = a / (uae_s64)(uae_s32)src;
			if ((quot & UVAL64 (0xffffffff80000000)) != 0
				&& (quot & UVAL64 (0xffffffff80000000)) != UVAL64 (0xffffffff80000000))
			{
				divsl_overflow(extra, a, src);
			} else {
				if (((uae_s32)rem < 0) != ((uae_s64)a < 0)) rem = -rem;
				SET_VFLG (0);
				SET_CFLG (0);
				SET_ZFLG (((uae_s32)quot) == 0);
				SET_NFLG (((uae_s32)quot) < 0);
				m68k_dreg (regs, extra & 7) = (uae_u32)rem;
				m68k_dreg (regs, (extra >> 12) & 7) = (uae_u32)quot;
			}
		}
	} else {
		/* unsigned */
		uae_u64 a = (uae_u64)(uae_u32)m68k_dreg (regs, (extra >> 12) & 7);
		uae_u64 quot, rem;

		if (extra & 0x400) {
			a &= 0xffffffffu;
			a |= (uae_u64)m68k_dreg (regs, extra & 7) << 32;
		}

		if (src == 0) {
			divul_divbyzero(extra, a, oldpc);
			return 0;
		}

		rem = a % (uae_u64)src;
		quot = a / (uae_u64)src;
		if (quot > 0xffffffffu) {
			divul_overflow(extra, a);
		} else {
			SET_VFLG (0);
			SET_CFLG (0);
			SET_ZFLG (((uae_s32)quot) == 0);
			SET_NFLG (((uae_s32)quot) < 0);
			m68k_dreg (regs, extra & 7) = (uae_u32)rem;
			m68k_dreg (regs, (extra >> 12) & 7) = (uae_u32)quot;
		}
	}
	return 1;
}


int m68k_mull (uae_u32 opcode, uae_u32 src, uae_u16 extra)
{
	if ((extra & 0x400) && currprefs.int_no_unimplemented && currprefs.cpu_model == 68060) {
		return -1;
	}
	if (extra & 0x800) {
		/* signed */
		uae_s64 a = (uae_s64)(uae_s32)m68k_dreg (regs, (extra >> 12) & 7);

		a *= (uae_s64)(uae_s32)src;
		SET_VFLG (0);
		SET_CFLG (0);
		if (extra & 0x400) {
			// 32 * 32 = 64
			// 68040 is different.
			if (currprefs.cpu_model >= 68040) {
				m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
				m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
			} else {
				// 020/030
				m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
				m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
			}
			SET_ZFLG(a == 0);
			SET_NFLG(a < 0);
		} else {
			// 32 * 32 = 32
			uae_s32 b = (uae_s32)a;
			m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
			if ((a & UVAL64(0xffffffff80000000)) != 0 && (a & UVAL64(0xffffffff80000000)) != UVAL64(0xffffffff80000000)) {
				SET_VFLG(1);
			}
			SET_ZFLG(b == 0);
			SET_NFLG(b < 0);
		}
	} else {
		/* unsigned */
		uae_u64 a = (uae_u64)(uae_u32)m68k_dreg (regs, (extra >> 12) & 7);

		a *= (uae_u64)src;
		SET_VFLG (0);
		SET_CFLG (0);
		if (extra & 0x400) {
			// 32 * 32 = 64
			// 68040 is different.
			if (currprefs.cpu_model >= 68040) {
				m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
				m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
			} else {
				// 020/030
				m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
				m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
			}
			SET_ZFLG(a == 0);
			SET_NFLG(((uae_s64)a) < 0);
		} else {
			// 32 * 32 = 32
			uae_s32 b = (uae_s32)a;
			m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
			if ((a & UVAL64(0xffffffff00000000)) != 0) {
				SET_VFLG(1);
			}
			SET_ZFLG(b == 0);
			SET_NFLG(b < 0);
		}
	}
	return 1;
}

#endif

uae_u32 exception_pc(int nr)
{
	// bus error, address error, illegal instruction, privilege violation, a-line, f-line
	if (nr == 2 || nr == 3 || nr == 4 || nr == 8 || nr == 10 || nr == 11)
		return regs.instruction_pc;
	return m68k_getpc();
}

void Exception_build_stack_frame(uae_u32 oldpc, uae_u32 currpc, uae_u32 ssw, int nr, int format)
{
	int i;

	switch (format) {
	case 0x0: // four word stack frame
	case 0x1: // throwaway four word stack frame
		break;
	case 0x2: // six word stack frame
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), oldpc);
		break;
	case 0x3: // floating point post-instruction stack frame (68040)
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.fp_ea);
		break;
	case 0x4: // floating point unimplemented stack frame (68LC040, 68EC040)
		// or 68060 bus access fault stack frame
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), ssw);
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), oldpc);
		break;
	case 0x7: // access error stack frame (68040)

		for (i = 3; i >= 0; i--) {
			// WB1D/PD0,PD1,PD2,PD3
			m68k_areg(regs, 7) -= 4;
			x_put_long(m68k_areg(regs, 7), mmu040_move16[i]);
		}

		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), 0); // WB1A
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), 0); // WB2D
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.wb2_address); // WB2A
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.wb3_data); // WB3D
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.mmu_fault_addr); // WB3A

		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.mmu_fault_addr); // FA

		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), 0);
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.wb2_status);
		regs.wb2_status = 0;
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.wb3_status);
		regs.wb3_status = 0;

		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), ssw);
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.mmu_effective_addr);
		break;
	case 0x8: // bus/address error (68010)
	{
		uae_u16 in = regs.read_buffer;
		uae_u16 out = regs.write_buffer;
		for (i = 0; i < 15; i++) {
			m68k_areg(regs, 7) -= 2;
			x_put_word(m68k_areg(regs, 7), 0);
		}
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), 0x0000); // version (probably bits 12 to 15 only because other bits change)
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.irc); // instruction input buffer
		m68k_areg(regs, 7) -= 2;
		// unused not written
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), in); // data input buffer
		m68k_areg(regs, 7) -= 2;
		// unused not written
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), out); // data output buffer
		m68k_areg(regs, 7) -= 2;
		// unused not written
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.mmu_fault_addr); // fault addr
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.mmu_fault_addr >> 16); // fault addr
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), ssw); // ssw
		break;
	}
	case 0x9: // coprocessor mid-instruction stack frame (68020, 68030)
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.fp_ea);
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.fp_opword);
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), oldpc);
		break;
	case 0xB: // long bus cycle fault stack frame (68020, 68030)
		// Store state information to internal register space
#if MMU030_DEBUG
		if (mmu030_idx >= MAX_MMU030_ACCESS) {
			write_log(_T("mmu030_idx out of bounds! %d >= %d\n"), mmu030_idx, MAX_MMU030_ACCESS);
		}
#endif
		if (!(ssw & MMU030_SSW_RW)) {
			// written value that caused fault but was not yet written to memory
			// this value is used in cpummu030 when write is retried.
			mmu030_ad[mmu030_idx_done].val = regs.wb3_data;
		}
		for (i = 0; i < mmu030_idx_done + 1; i++) {
			m68k_areg(regs, 7) -= 4;
			x_put_long(m68k_areg(regs, 7), mmu030_ad[i].val);
		}
		while (i < MAX_MMU030_ACCESS) {
			uae_u32 v = 0;
			m68k_areg(regs, 7) -= 4;
			// mmu030_idx is always small enough if instruction is FMOVEM.
			if (mmu030_state[1] & MMU030_STATEFLAG1_FMOVEM) {
#if MMU030_DEBUG
				if (mmu030_idx >= MAX_MMU030_ACCESS - 2) {
					write_log(_T("mmu030_idx (FMOVEM) out of bounds! %d >= %d\n"), mmu030_idx, MAX_MMU030_ACCESS - 2);
				}
#endif
				if (i == MAX_MMU030_ACCESS - 2)
					v = mmu030_fmovem_store[0];
				else if (i == MAX_MMU030_ACCESS - 1)
					v = mmu030_fmovem_store[1];
			}
			x_put_long(m68k_areg(regs, 7), v);
			i++;
		}
		// version & internal information (We store index here)
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), (mmu030_idx & 0xf) | ((mmu030_idx_done & 0xf) << 4) | (regs.wb2_status << 8));
		// 3* internal registers
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), mmu030_state[2] | (regs.wb3_status << 8));
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.wb2_address); // = mmu030_state[1]
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), mmu030_state[0]);
		// data input buffer = fault address
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.mmu_fault_addr);
		// 2xinternal
		{
			uae_u32 ps = (regs.prefetch020_valid[0] ? 1 : 0) | (regs.prefetch020_valid[1] ? 2 : 0) | (regs.prefetch020_valid[2] ? 4 : 0);
			ps |= ((regs.pipeline_r8[0] & 7) << 8);
			ps |= ((regs.pipeline_r8[1] & 7) << 11);
			ps |= ((regs.pipeline_pos & 15) << 16);
			ps |= ((regs.pipeline_stop & 15) << 20);
			if (mmu030_opcode == -1)
				ps |= 1 << 31;
			m68k_areg(regs, 7) -= 4;
			x_put_long(m68k_areg(regs, 7), ps);
		}
		// stage b address
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), mm030_stageb_address);
		// 2xinternal
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), mmu030_disp_store[1]);
		/* fall through */
	case 0xA:
		// short bus cycle fault stack frame (68020, 68030)
		// used when instruction's last write causes bus fault
		m68k_areg(regs, 7) -= 4;
		if (format == 0xb) {
			x_put_long(m68k_areg(regs, 7), mmu030_disp_store[0]); // 28 0x1c
		} else {
			uae_u32 ps = (regs.prefetch020_valid[0] ? 1 : 0) | (regs.prefetch020_valid[1] ? 2 : 0) | (regs.prefetch020_valid[2] ? 4 : 0);
			ps |= ((regs.pipeline_r8[0] & 7) << 8);
			ps |= ((regs.pipeline_r8[1] & 7) << 11);
			ps |= ((regs.pipeline_pos & 15) << 16);
			ps |= ((regs.pipeline_stop & 15) << 20);
			x_put_long(m68k_areg(regs, 7), ps); // 28 0x1c
		}
		m68k_areg(regs, 7) -= 4;
		// Data output buffer = value that was going to be written
		x_put_long(m68k_areg(regs, 7), regs.wb3_data); // 24 0x18
		m68k_areg(regs, 7) -= 4;
		if (format == 0xb) {
			x_put_long(m68k_areg(regs, 7), (mmu030_opcode & 0xffff) | (regs.prefetch020[0] << 16));  // Internal register (opcode storage) 20 0x14
		} else {
			x_put_long(m68k_areg(regs, 7), regs.irc | (regs.prefetch020[0] << 16));  // Internal register (opcode storage)  20 0x14
		}
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), regs.mmu_fault_addr); // data cycle fault address 16 0x10
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.prefetch020[2]);  // Instr. pipe stage B 14 0x0e
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.prefetch020[1]);  // Instr. pipe stage C 12 0x0c
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), ssw); // 10 0x0a
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.wb2_address); // = mmu030_state[1]); 8 0x08
		break;
	default:
		write_log(_T("Unknown exception stack frame format: %X\n"), format);
		return;
	}
	m68k_areg(regs, 7) -= 2;
	x_put_word(m68k_areg(regs, 7), (format << 12) | (nr * 4));
	m68k_areg(regs, 7) -= 4;
	x_put_long(m68k_areg(regs, 7), currpc);
	m68k_areg(regs, 7) -= 2;
	x_put_word(m68k_areg(regs, 7), regs.sr);
}

void Exception_build_stack_frame_common(uae_u32 oldpc, uae_u32 currpc, uae_u32 ssw, int nr, int vector_nr)
{
	if (nr == 5 || nr == 6 || nr == 7 || nr == 9) {
		if (nr == 9)
			oldpc = regs.trace_pc;
		if (currprefs.cpu_model <= 68010)
			Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x0);
		else
			Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x2);
	} else if (nr == 60 || nr == 61) {
		Exception_build_stack_frame(oldpc, regs.instruction_pc, regs.mmu_ssw, vector_nr, 0x0);
	} else if (nr >= 48 && nr <= 55) {
		if (regs.fpu_exp_pre) {
			if (currprefs.cpu_model == 68060 && nr == 55 && regs.fp_unimp_pend == 2) { // packed decimal real
				Exception_build_stack_frame(regs.fp_ea, regs.instruction_pc, 0, vector_nr, 0x2);
			} else {
				Exception_build_stack_frame(oldpc, regs.instruction_pc, 0, vector_nr, 0x0);
			}
		} else { /* post-instruction */
			if (currprefs.cpu_model == 68060 && nr == 55 && regs.fp_unimp_pend == 2) { // packed decimal real
				Exception_build_stack_frame(regs.fp_ea, currpc, 0, vector_nr, 0x2);
			} else {
				Exception_build_stack_frame(oldpc, currpc, 0, vector_nr, 0x3);
			}
		}
	} else if (nr == 11 && regs.fp_unimp_ins) {
		regs.fp_unimp_ins = false;
		if ((currprefs.cpu_model == 68060 && (currprefs.fpu_model == 0 || (regs.pcr & 2))) ||
			(currprefs.cpu_model == 68040 && currprefs.fpu_model == 0)) {
			Exception_build_stack_frame(regs.fp_ea, currpc, regs.instruction_pc, vector_nr, 0x4);
		} else {
			Exception_build_stack_frame(regs.fp_ea, currpc, regs.mmu_ssw, vector_nr, 0x2);
		}
	} else {
		Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x0);
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

void cpu_restore_fixup(void)
{
	if (mmufixup[0].reg >= 0) {
		m68k_areg(regs, mmufixup[0].reg & 15) = mmufixup[0].value;
		mmufixup[0].reg = -1;
	}
	if (mmufixup[1].reg >= 0) {
		m68k_areg(regs, mmufixup[1].reg & 15) = mmufixup[1].value;
		mmufixup[1].reg = -1;
	}
}

// Low word: Clear + Z and N
void ccr_68000_long_move_ae_LZN(uae_s32 src)
{
	CLEAR_CZNV();
	uae_s16 vsrc = (uae_s16)(src & 0xffff);
	SET_ZFLG(vsrc == 0);
	SET_NFLG(vsrc < 0);
}

// Low word: Clear + N only
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

void dreg_68000_long_replace_low(int reg, uae_u16 v)
{
	m68k_dreg(regs, reg) = (m68k_dreg(regs, reg) & 0xffff0000) | v;
}

void areg_68000_long_replace_low(int reg, uae_u16 v)
{
	m68k_areg(regs, reg) = (m68k_areg(regs, reg) & 0xffff0000) | v;
}

// Change F-line to privilege violation if missing co-pro
// 68040 and 68060 always return F-line
bool privileged_copro_instruction(uae_u16 opcode)
{
	if (currprefs.cpu_model >= 68020 && !regs.s) {
		int reg = opcode & 7;
		int mode = (opcode >> 3) & 7;
		int id = (opcode >> 9) & 7;
		// cpSAVE and cpRESTORE: privilege violation if user mode.
		if ((opcode & 0xf1c0) == 0xf100) {
			// cpSAVE
			// check if valid EA
			if (mode == 2 || (mode >= 4 && mode <= 6) || (mode == 7 && (reg == 0 || reg == 1))) {
				if (currprefs.cpu_model < 68040 || (currprefs.cpu_model >= 68040 && id == 1))
					return true;
			}
		} else if ((opcode & 0xf1c0) == 0xf140) {
			// cpRESTORE
			// check if valid EA
			if (mode == 2 || mode == 3 || (mode >= 5 && mode <= 6) || (mode == 7 && reg <= 3)) {
				if (currprefs.cpu_model < 68040 || (currprefs.cpu_model >= 68040 && id == 1))
					return true;
			}
		}
	}
	return false;
}

bool generates_group1_exception(uae_u16 opcode)
{
	struct instr *table = &table68k[opcode];
	// illegal/a-line/f-line?
	if (table->mnemo == i_ILLG)
		return true;
	// privilege violation?
	if (!regs.s) {
		if (table->plev == 1 && currprefs.cpu_model > 68000)
			return true;
		if (table->plev == 2)
			return true;
		if (table->plev == 3 && table->size == sz_word)
			return true;
	}
	return false;
}
