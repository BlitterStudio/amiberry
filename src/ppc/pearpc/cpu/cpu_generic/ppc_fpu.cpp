/*
 *	PearPC
 *	ppc_fpu.cc
 *                        
 *	Copyright (C) 2003, 2004 Sebastian Biallas (sb@biallas.net)
 *	Copyright (C) 2003 Stefan Weyergraf
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include "tracers.h"
#include "ppc_cpu.h"
#include "ppc_dec.h"
#include "ppc_fpu.h"

// .121


#define PPC_FPR_TYPE2(a,b) (((a)<<8)|(b))
inline void ppc_fpu_add(ppc_double &res, ppc_double &a, ppc_double &b)
{
	switch (PPC_FPR_TYPE2(a.type, b.type)) {
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_norm): {
		int diff = a.e - b.e;
		if (diff<0) {
			diff = -diff;
			if (diff <= 56) {
				a.m >>= diff;
			} else if (a.m != 0) {
				a.m = 1;	
			} else {
				a.m = 0;
			}
			res.e = b.e;
		} else {
			if (diff <= 56) {
				b.m >>= diff;
			} else if (b.m != 0) {
				b.m = 1;
			} else {
				b.m = 0;
			}
			res.e = a.e;
		}
		res.type = ppc_fpr_norm;
		if (a.s == b.s) {
			res.s = a.s;
			res.m = a.m + b.m;
			if (res.m & (1ULL<<56)) {
			    res.m >>= 1;
			    res.e++;
			}
		} else {
			res.s = a.s;
			res.m = a.m - b.m;
			if (!res.m) {
				if (FPSCR_RN(gCPU.fpscr) == FPSCR_RN_MINF) {
					res.s |= b.s;
				} else {
					res.s &= b.s;
				}
				res.type = ppc_fpr_zero;
			} else {
				if ((sint64)res.m < 0) {
					res.m = b.m - a.m;
					res.s = b.s;
				}
				diff = ppc_fpu_normalize(res) - 8;
				res.e -= diff;
				res.m <<= diff;
			}	
		}
		break;
	}
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_NaN):
		res.s = a.s;
		res.type = ppc_fpr_NaN;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_zero):
		res.e = a.e;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_zero):
		res.s = a.s;
		res.m = a.m;
		res.type = a.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_norm):
		res.e = b.e;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_NaN):
		res.s = b.s;
		res.m = b.m;
		res.type = b.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_Inf):
		if (a.s != b.s) {
			// +oo + -oo == NaN
			res.s = a.s ^ b.s;
			res.type = ppc_fpr_NaN;
			break;
		}
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_zero):
		res.s = a.s;
		res.type = a.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_Inf):
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_Inf):
		res.s = b.s;
		res.type = b.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_zero):
		// round bla
		res.type = ppc_fpr_zero;
		res.s = a.s && b.s;
		break;
	}
}

inline void ppc_fpu_quadro_mshr(ppc_quadro &q, int exp)
{
	if (exp >= 64) {
		q.m1 = q.m0;
		q.m0 = 0;
		exp -= 64;
	}
	uint64 t = q.m0 & ((1ULL<<exp)-1);
	q.m0 >>= exp;
	q.m1 >>= exp;
	q.m1 |= t<<(64-exp);
}

inline void ppc_fpu_quadro_mshl(ppc_quadro &q, int exp)
{
	if (exp >= 64) {
		q.m0 = q.m1;
		q.m1 = 0;
		exp -= 64;
	}
	uint64 t = (q.m1 >> (64-exp)) & ((1ULL<<exp)-1);
	q.m0 <<= exp;
	q.m1 <<= exp;
	q.m0 |= t;
}

inline void ppc_fpu_add_quadro_m(ppc_quadro &res, const ppc_quadro &a, const ppc_quadro &b)
{
	res.m1 = a.m1+b.m1;
	if (res.m1 < a.m1) {
		res.m0 = a.m0+b.m0+1;
	} else {
		res.m0 = a.m0+b.m0;
	}
}

inline void ppc_fpu_sub_quadro_m(ppc_quadro &res, const ppc_quadro &a, const ppc_quadro &b)
{
	res.m1 = a.m1-b.m1;
	if (a.m1 < b.m1) {
		res.m0 = a.m0-b.m0-1;
	} else {
		res.m0 = a.m0-b.m0;
	}
}

// res has 107 significant bits. a, b have 106 significant bits each.
inline void ppc_fpu_add_quadro(ppc_quadro &res, ppc_quadro &a, ppc_quadro &b)
{
	// treat as 107 bit mantissa
	if (a.type == ppc_fpr_norm) ppc_fpu_quadro_mshl(a, 1);
	if (b.type == ppc_fpr_norm) ppc_fpu_quadro_mshl(b, 1);
	switch (PPC_FPR_TYPE2(a.type, b.type)) {
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_norm): {
		int diff = a.e - b.e;
		if (diff < 0) {
			diff = -diff;
			if (diff <= 107) {
				// FIXME: may set x_prime
				ppc_fpu_quadro_mshr(a, diff);
			} else if (a.m0 || a.m1) {
				a.m0 = 0;
				a.m1 = 1;
			} else {
				a.m0 = 0;
				a.m1 = 0;
			}
			res.e = b.e;
		} else {
			if (diff <= 107) {
				// FIXME: may set x_prime
				ppc_fpu_quadro_mshr(b, diff);
			} else if (b.m0 || b.m1) {
				b.m0 = 0;
				b.m1 = 1;
			} else {
				b.m0 = 0;
				b.m1 = 0;
			}
			res.e = a.e;
		}
		res.type = ppc_fpr_norm;
		if (a.s == b.s) {
			res.s = a.s;
			ppc_fpu_add_quadro_m(res, a, b);
			int X_prime = res.m1 & 1;
			if (res.m0 & (1ULL<<(107-64))) {
				ppc_fpu_quadro_mshr(res, 1);
				res.e++;
			}
			// res = [107]
			res.m1 = (res.m1 & 0xfffffffffffffffeULL) | X_prime;
		} else {
			res.s = a.s;
			int cmp;
			if (a.m0 < b.m0) {
				cmp = -1;
			} else if (a.m0 > b.m0) {
				cmp = +1;
			} else {
				if (a.m1 < b.m1) {
					cmp = -1;
				} else if (a.m1 > b.m1) {
					cmp = +1;
				} else {
					cmp = 0;
				}
			}
			if (!cmp) {
				if (FPSCR_RN(gCPU.fpscr) == FPSCR_RN_MINF) {
					res.s |= b.s;
				} else {
					res.s &= b.s;
				}
				res.type = ppc_fpr_zero;
			} else {
				if (cmp < 0) {
					ppc_fpu_sub_quadro_m(res, b, a);
					res.s = b.s;
				} else {
					ppc_fpu_sub_quadro_m(res, a, b);
				}
				diff = ppc_fpu_normalize_quadro(res) - (128-107);
				int X_prime = res.m1 & 1;
				res.m1 &= 0xfffffffffffffffeULL;
				ppc_fpu_quadro_mshl(res, diff);
				res.e -= diff;
				res.m1 |= X_prime;
			}
			// res = [107]
		}
		break;
	}
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_NaN):
		res.s = a.s;
		res.type = ppc_fpr_NaN;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_zero):
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_zero):
		res.e = a.e;
		res.s = a.s;
		res.m0 = a.m0;
		res.m1 = a.m1;
		res.type = a.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_norm):
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_NaN):
		res.e = b.e;
		res.s = b.s;
		res.m0 = b.m0;
		res.m1 = b.m1;
		res.type = b.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_Inf):
		if (a.s != b.s) {
			// +oo + -oo == NaN
			res.s = a.s ^ b.s;
			res.type = ppc_fpr_NaN;
			break;
		}
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_zero):
		res.s = a.s;
		res.type = a.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_Inf):
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_Inf):
		res.s = b.s;
		res.type = b.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_zero):
		// round bla
		res.type = ppc_fpr_zero;
		res.s = a.s && b.s;
		break;
	}
}

inline void ppc_fpu_add_uint64_carry(uint64 &a, uint64 b, uint64 &carry)
{
	carry = (a+b < a) ? 1 : 0;
	a += b;
}

// 'res' has 56 significant bits on return, a + b have 56 significant bits each
inline void ppc_fpu_mul(ppc_double &res, const ppc_double &a, const ppc_double &b)
{
	res.s = a.s ^ b.s;
	switch (PPC_FPR_TYPE2(a.type, b.type)) {
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_norm): {
		res.type = ppc_fpr_norm;
		res.e = a.e + b.e;
//		printf("new exp: %d\n", res.e);
//		ht_printf("MUL:\na.m: %qb\nb.m: %qb\n", a.m, b.m);
		uint64 fH, fM1, fM2, fL;
		fL = (a.m & 0xffffffff) * (b.m & 0xffffffff);	// [32] * [32] = [63,64]
		fM1 = (a.m >> 32) * (b.m & 0xffffffff);		// [24] * [32] = [55,56]
		fM2 = (a.m & 0xffffffff) * (b.m >> 32);		// [32] * [24] = [55,56]
		fH = (a.m >> 32) * (b.m >> 32);			// [24] * [24] = [47,48]
//		ht_printf("fH: %qx fM1: %qx fM2: %qx fL: %qx\n", fH, fM1, fM2, fL);

		// calulate fH * 2^64 + (fM1 + fM2) * 2^32 + fL
		uint64 rL, rH;
		rL = fL;					// rL = rH = [63,64]
		rH = fH;					// rH = fH = [47,48]
		uint64 split;
		split = fM1 + fM2;
		uint64 carry;
		ppc_fpu_add_uint64_carry(rL, (split & 0xffffffff) << 32, carry); // rL = [63,64]
		rH += carry;					// rH = [0 .. 2^48]
		rH += split >> 32;				// rH = [0:48], where 46, 47 or 48 set

		// res.m = [0   0  .. 0  | rH_48 rH_47 .. rH_0 | rL_63 rL_62 .. rL_55]
		//         [---------------------------------------------------------]
		// bit   = [63  62 .. 58 | 57    56    .. 9    | 8     7        0    ]
		//         [---------------------------------------------------------]
		//         [15 bits zero |      49 bits rH     | 8 most sign.bits rL ]
		res.m = rH << 9;
		res.m |= rL >> (64-9);
		// res.m = [58]

//		ht_printf("fH: %qx fM1: %qx fM2: %qx fL: %qx\n", fH, fM1, fM2, fL);
		if (res.m & (1ULL << 57)) {
			res.m >>= 2;
			res.e += 2;
		} else if (res.m & (1ULL << 56)) {
			res.m >>= 1;
			res.e++;
		}
		// res.m = [56]
		break;
	}
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_NaN):
		res.type = a.type;
		res.e = a.e;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_zero): 
		res.s = a.s;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_zero): 
		res.type = a.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_NaN): 
		res.s = b.s;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_zero): 
		res.type = b.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_zero): 
		res.type = ppc_fpr_NaN;
		break;
	}
}

// 'res' has 'prec' significant bits on return, a + b have 56 significant bits each
// for 111 >= prec >= 64
inline void ppc_fpu_mul_quadro(ppc_quadro &res, ppc_double &a, ppc_double &b, int prec)
{
	res.s = a.s ^ b.s;
	switch (PPC_FPR_TYPE2(a.type, b.type)) {
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_norm): {
		res.type = ppc_fpr_norm;
		res.e = a.e + b.e;
//		printf("new exp: %d\n", res.e);
//		ht_printf("MUL:\na.m: %016qx\nb.m: %016qx\n", a.m, b.m);
		uint64 fH, fM1, fM2, fL;
		fL = (a.m & 0xffffffff) * (b.m & 0xffffffff);	// [32] * [32] = [63,64]
		fM1 = (a.m >> 32) * (b.m & 0xffffffff);		// [24] * [32] = [55,56]
		fM2 = (a.m & 0xffffffff) * (b.m >> 32);		// [32] * [24] = [55,56]
		fH = (a.m >> 32) * (b.m >> 32);			// [24] * [24] = [47,48]
//		ht_printf("fH: %016qx fM1: %016qx fM2: %016qx fL: %016qx\n", fH, fM1, fM2, fL);

		// calulate fH * 2^64 + (fM1 + fM2) * 2^32 + fL
		uint64 rL, rH;
		rL = fL;					// rL = rH = [63,64]
		rH = fH;					// rH = fH = [47,48]
		uint64 split;
		split = fM1 + fM2;
		uint64 carry;
		ppc_fpu_add_uint64_carry(rL, (split & 0xffffffff) << 32, carry); // rL = [63,64]
		rH += carry;					// rH = [0 .. 2^48]
		rH += split >> 32;				// rH = [0:48], where 46, 47 or 48 set

		// res.m0 = [0    0   .. 0   | rH_48 rH_47 .. rH_0 | rL_63 rL_62 .. rL_0]
		//          [-----------------------------------------------------------]
		// log.bit= [127  126 .. 113 | 112            64   | 63    62       0   ]
		//          [-----------------------------------------------------------]
		//          [ 15 bits zero   |      49 bits rH     |      64 bits rL    ]
		res.m0 = rH;
		res.m1 = rL;
		// res.m0|res.m1 = [111,112,113]

//		ht_printf("res = %016qx%016qx\n", res.m0, res.m1);
		if (res.m0 & (1ULL << 48)) {
			ppc_fpu_quadro_mshr(res, 2+(111-prec));
			res.e += 2;
		} else if (res.m0 & (1ULL << 47)) {
			ppc_fpu_quadro_mshr(res, 1+(111-prec));
			res.e += 1;
		} else {
			ppc_fpu_quadro_mshr(res, 111-prec);
		}
		// res.m0|res.m1 = [prec]
		break;
	}
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_NaN):
		res.type = a.type;
		res.e = a.e;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_zero): 
		res.s = a.s;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_norm): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_zero): 
		res.type = a.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_NaN): 
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_NaN): 
		res.s = b.s;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_zero): 
		res.type = b.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_Inf): 
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_zero): 
		res.type = ppc_fpr_NaN;
		break;
	}
}

// calculate one of these:
// + m1 * m2 + s
// + m1 * m2 - s
// - m1 * m2 + s
// - m1 * m2 - s
// using a 106 bit accumulator
//
// .752
//
// FIXME: There is a bug in this code that shows up in Mac OS X Finder fwd/bwd
// button: the top line is not rendered correctly. This works with the jitc_x86
// FPU however...
inline void ppc_fpu_mul_add(ppc_double &res, ppc_double &m1, ppc_double &m2,
	ppc_double &s)
{
	ppc_quadro p;
/*	ht_printf("m1 = %d * %016qx * 2^%d, %s\n", m1.s, m1.m, m1.e,
		ppc_fpu_get_fpr_type(m1.type));
	ht_printf("m2 = %d * %016qx * 2^%d, %s\n", m2.s, m2.m, m2.e,
		ppc_fpu_get_fpr_type(m2.type));*/
	// create product with 106 significant bits
	ppc_fpu_mul_quadro(p, m1, m2, 106);
/*	ht_printf("p = %d * %016qx%016qx * 2^%d, %s\n", p.s, p.m0, p.m1, p.e,
		ppc_fpu_get_fpr_type(p.type));*/
	// convert s into ppc_quadro
/*	ht_printf("s = %d * %016qx * 2^%d %s\n", s.s, s.m, s.e,
		ppc_fpu_get_fpr_type(s.type));*/
	ppc_quadro q;
	q.e = s.e;
	q.s = s.s;
	q.type = s.type;
	q.m0 = 0;
	q.m1 = s.m;
	// .. with 106 significant bits
	ppc_fpu_quadro_mshl(q, 106-56);
/*	ht_printf("q = %d * %016qx%016qx * 2^%d %s\n", q.s, q.m0, q.m1, q.e,
		ppc_fpu_get_fpr_type(q.type));*/
	// now we must add p, q.
	ppc_quadro x;
	ppc_fpu_add_quadro(x, p, q);
	// x = [107]
/*	ht_printf("x = %d * %016qx%016qx * 2^%d %s\n", x.s, x.m0, x.m1, x.e,
		ppc_fpu_get_fpr_type(x.type));*/
	res.type = x.type;
	res.s = x.s;
	res.e = x.e;
	if (x.type == ppc_fpr_norm) {
		res.m = x.m0 << 13;			// 43 bits from m0
		res.m |= (x.m1 >> (64-12)) << 1;	// 12 bits from m1
		res.m |= x.m1 & 1;			// X' bit from m1
	}
/*	ht_printf("res = %d * %016qx * 2^%d %s\n", res.s, res.m, res.e,
		ppc_fpu_get_fpr_type(res.type));*/
}

inline void ppc_fpu_div(ppc_double &res, const ppc_double &a, const ppc_double &b)
{
	res.s = a.s ^ b.s;
	switch (PPC_FPR_TYPE2(a.type, b.type)) {
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_norm): {
		res.type = ppc_fpr_norm;
		res.e = a.e - b.e;
		res.m = 0;
		uint64 am = a.m, bm = b.m;
		uint i = 0;
		while (am && (i<56)) {
			res.m <<= 1;
			if (am >= bm) {
				res.m |= 1;
				am -= bm;
			}
			am <<= 1;
//			printf("am=%llx, bm=%llx, rm=%llx\n", am, bm, res.m);
			i++;
		}
		res.m <<= 57-i;
		if (res.m & (1ULL << 56)) {
			res.m >>= 1;
		} else {
			res.e--;
		}
//		printf("final: am=%llx, bm=%llx, rm=%llx\n", am, bm, res.m);
		break;
	}
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_NaN):
		res.e = a.e;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_norm):
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_Inf):
	case PPC_FPR_TYPE2(ppc_fpr_NaN, ppc_fpr_zero):
		res.s = a.s;
		// fall-thru
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_norm):
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_norm):
		res.type = a.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_NaN):
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_NaN):
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_NaN):
		res.s = b.s;
		res.type = b.type;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_Inf):
		res.type = ppc_fpr_zero;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_norm, ppc_fpr_zero):
		res.type = ppc_fpr_Inf;
		break;
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_Inf):
	case PPC_FPR_TYPE2(ppc_fpr_Inf, ppc_fpr_zero):
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_Inf):
	case PPC_FPR_TYPE2(ppc_fpr_zero, ppc_fpr_zero):
		res.type = ppc_fpr_NaN;
		break;
	}
}

inline void ppc_fpu_sqrt(ppc_double &D, const ppc_double &B)
{
	switch (B.type) {
	case ppc_fpr_norm:
		if (B.s) {
			D.type = ppc_fpr_NaN;
			gCPU.fpscr |= FPSCR_VXSQRT;
			break;
		}
		// D := 1/2(D_old + B/D_old)
		D = B;
		D.e /= 2;
		for (int i=0; i<6; i++) {
			ppc_double D_old = D;
			ppc_double B_div_D_old;
			ppc_fpu_div(B_div_D_old, B, D_old);
			ppc_fpu_add(D, D_old, B_div_D_old);
			D.e--;
			
/*			uint64 e;
			ppc_double E = D;
			ppc_fpu_pack_double(E, e);
			printf("%.20f\n", *(double *)&e);*/
		}
		break;
	case ppc_fpr_zero:
		D.type = ppc_fpr_zero;
		D.s = B.s;
		break;
	case ppc_fpr_Inf:
		if (B.s) {
			D.type = ppc_fpr_NaN;
			gCPU.fpscr |= FPSCR_VXSQRT;
		} else {
			D.type = ppc_fpr_Inf;
			D.s = 0;
		}
		break;
	case ppc_fpr_NaN:
		D.type = ppc_fpr_NaN;
		break;
	}	
}

void ppc_fpu_test()
{
	ppc_double A, B, C;
	double a, b, c;
	A.type = B.type = ppc_fpr_norm;
	A.s = 1;
	A.e = 0;
	A.m = 0;
	A.m = ((1ULL<<56)-1)-((1ULL<<10)-1);
	ht_printf("%qb\n", A.m);
	B.s = 1;
	B.e = 0;
	B.m = 0;
	B.m = ((1ULL<<56)-1)-((1ULL<<50)-1);
	a = ppc_fpu_get_double(A);
	b = ppc_fpu_get_double(B);
	printf("%f + %f = \n", a, b);
	ppc_fpu_add(C, A, B);
	uint64 d;
	uint32 s;
	ppc_fpu_pack_double_as_single(C, d);
	ht_printf("%064qb\n", d);
	ppc_fpu_unpack_double(C, d);
	ppc_fpu_pack_single(C, s);
	ht_printf("single: %032b\n", s);
	ppc_single Cs;
	ppc_fpu_unpack_single(Cs, s);
	ppc_fpu_single_to_double(Cs, C);
//	ht_printf("%d\n", ppc_fpu_double_to_int(C));
	c = ppc_fpu_get_double(C);
	printf("%f\n", c);
}

/*
 *	a and b must not be NaNs
 */
inline uint32 ppc_fpu_compare(ppc_double &a, ppc_double &b)
{
	if (a.type == ppc_fpr_zero) {
		if (b.type == ppc_fpr_zero) return 2;
		return (b.s) ? 4: 8;
	}
	if (b.type == ppc_fpr_zero) return (a.s) ? 8: 4;
	if (a.s != b.s) return (a.s) ? 8: 4;
	if (a.e > b.e) return (a.s) ? 8: 4;
	if (a.e < b.e) return (a.s) ? 4: 8;
	if (a.m > b.m) return (a.s) ? 8: 4;
	if (a.m < b.m) return (a.s) ? 4: 8;
	return 2;
}

double ppc_fpu_get_double(uint64 d)
{	
	ppc_double dd;
	ppc_fpu_unpack_double(dd, d);
	return ppc_fpu_get_double(dd);
}

double ppc_fpu_get_double(ppc_double &d)
{
	if (d.type == ppc_fpr_norm) {
		double r = d.m;
		for (int i=0; i<55; i++) {
			r = r / 2.0;
		}
		if (d.e < 0) {
			for (int i=0; i>d.e; i--) {
				r = r / 2.0;
			}
		} else if (d.e > 0) {
			for (int i=0; i<d.e; i++) {
				r = r * 2.0;
			}
		}
		if (d.s) r = -r;
		return r;
	} else {
		return 0.0;
	}
}

/***********************************************************************************
 *
 */
 
 
/*
 *	fabsx		Floating Absolute Value
 *	.484
 */
void ppc_opc_fabsx()
{
	int frD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, frA, frB);
	PPC_OPC_ASSERT(frA==0);
	gCPU.fpr[frD] = gCPU.fpr[frB] & ~FPU_SIGN_BIT;
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fabs.\n");
	}
}
/*
 *	faddx		Floating Add (Double-Precision)
 *	.485
 */
void ppc_opc_faddx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frC==0);
	ppc_double A, B, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	if (A.s != B.s && A.type == ppc_fpr_Inf && B.type == ppc_fpr_Inf) {
		gCPU.fpscr |= FPSCR_VXISI;
	}
	ppc_fpu_add(D, A, B);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fadd.\n");
	}
}
/*
 *	faddx		Floating Add Single
 *	.486
 */
void ppc_opc_faddsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frC==0);
	ppc_double A, B, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	if (A.s != B.s && A.type == ppc_fpr_Inf && B.type == ppc_fpr_Inf) {
		gCPU.fpscr |= FPSCR_VXISI;
	}
	ppc_fpu_add(D, A, B);
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fadds.\n");
	}
}
/*
 *	fcmpo		Floating Compare Ordered
 *	.488
 */
static uint32 ppc_fpu_cmp_and_mask[8] = {
	0xfffffff0,
	0xffffff0f,
	0xfffff0ff,
	0xffff0fff,
	0xfff0ffff,
	0xff0fffff,
	0xf0ffffff,
	0x0fffffff,
};
void ppc_opc_fcmpo()
{
	int crfD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, crfD, frA, frB);
	crfD >>= 2;
	ppc_double A, B;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	uint32 cmp;
	if (A.type == ppc_fpr_NaN || B.type == ppc_fpr_NaN) {
		gCPU.fpscr |= FPSCR_VXSNAN;
		/*if (bla)*/ gCPU.fpscr |= FPSCR_VXVC;
		cmp = 1;
	} else {
		cmp = ppc_fpu_compare(A, B);
	}
	crfD = 7-crfD;
	gCPU.fpscr &= ~0x1f000;
	gCPU.fpscr |= (cmp << 12);
	gCPU.cr &= ppc_fpu_cmp_and_mask[crfD];
	gCPU.cr |= (cmp << (crfD * 4));
}
/*
 *	fcmpu		Floating Compare Unordered
 *	.489
 */
void ppc_opc_fcmpu()
{
	int crfD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, crfD, frA, frB);
	crfD >>= 2;
	ppc_double A, B;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	uint32 cmp;
	if (A.type == ppc_fpr_NaN || B.type == ppc_fpr_NaN) {
		gCPU.fpscr |= FPSCR_VXSNAN;
		cmp = 1;
	} else {
		cmp = ppc_fpu_compare(A, B);
	}
	crfD = 7-crfD;
	gCPU.fpscr &= ~0x1f000;
	gCPU.fpscr |= (cmp << 12);
	gCPU.cr &= ppc_fpu_cmp_and_mask[crfD];
	gCPU.cr |= (cmp << (crfD * 4));
}
/*
 *	fctiwx		Floating Convert to Integer Word
 *	.492
 */
void ppc_opc_fctiwx()
{
	int frD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, frA, frB);
	PPC_OPC_ASSERT(frA==0);
	ppc_double B;
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	gCPU.fpr[frD] = ppc_fpu_double_to_int(B);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fctiw.\n");
	}
}
/*
 *	fctiwzx		Floating Convert to Integer Word with Round toward Zero
 *	.493
 */
void ppc_opc_fctiwzx()
{
	int frD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, frA, frB);
	PPC_OPC_ASSERT(frA==0);
	uint32 oldfpscr = gCPU.fpscr;
	gCPU.fpscr &= ~3;
	gCPU.fpscr |= 1;
	ppc_double B;
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	gCPU.fpr[frD] = ppc_fpu_double_to_int(B);
	gCPU.fpscr = oldfpscr;
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fctiwz.\n");
	}
}
/*
 *	fdivx		Floating Divide (Double-Precision)
 *	.494
 */
void ppc_opc_fdivx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frC==0);
	ppc_double A, B, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	if (A.type == ppc_fpr_zero && B.type == ppc_fpr_zero) {
		gCPU.fpscr |= FPSCR_VXZDZ;
	}
	if (A.type == ppc_fpr_Inf && B.type == ppc_fpr_Inf) {
		gCPU.fpscr |= FPSCR_VXIDI;
	}
	if (B.type == ppc_fpr_zero && A.type != ppc_fpr_zero) {
		// FIXME::
		gCPU.fpscr |= FPSCR_VXIDI;		
	}
	ppc_fpu_div(D, A, B);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fdiv.\n");
	}
}
/*
 *	fdivsx		Floating Divide Single
 *	.495
 */
void ppc_opc_fdivsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frC==0);
	ppc_double A, B, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	if (A.type == ppc_fpr_zero && B.type == ppc_fpr_zero) {
		gCPU.fpscr |= FPSCR_VXZDZ;
	}
	if (A.type == ppc_fpr_Inf && B.type == ppc_fpr_Inf) {
		gCPU.fpscr |= FPSCR_VXIDI;
	}
	if (B.type == ppc_fpr_zero && A.type != ppc_fpr_zero) {
		// FIXME::
		gCPU.fpscr |= FPSCR_VXIDI;
	}
	ppc_fpu_div(D, A, B);
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fdivs.\n");
	}
}
/*
 *	fmaddx		Floating Multiply-Add (Double-Precision)
 *	.496
 */
void ppc_opc_fmaddx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	ppc_fpu_mul_add(D, A, C, B);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fmadd.\n");
	}
}
/*
 *	fmaddx		Floating Multiply-Add Single
 *	.497
 */
void ppc_opc_fmaddsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	ppc_fpu_mul_add(D, A, C, B);
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fmadds.\n");
	}
}
/*
 *	fmrx		Floating Move Register
 *	.498
 */
void ppc_opc_fmrx()
{
	int frD, rA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, rA, frB);
	PPC_OPC_ASSERT(rA==0);
	gCPU.fpr[frD] = gCPU.fpr[frB];
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fmr.\n");
	}
}
/*
 *	fmsubx		Floating Multiply-Subtract (Double-Precision)
 *	.499
 */
void ppc_opc_fmsubx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	B.s ^= 1;
	ppc_fpu_mul_add(D, A, C, B);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fmsub.\n");
	}
}
/*
 *	fmsubsx		Floating Multiply-Subtract Single
 *	.500
 */
void ppc_opc_fmsubsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	ppc_fpu_mul_add(D, A, C, B);
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fmsubs.\n");
	}
}
/*
 *	fmulx		Floating Multipy (Double-Precision)
 *	.501
 */
void ppc_opc_fmulx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frB==0);
	ppc_double A, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	if ((A.type == ppc_fpr_Inf && C.type == ppc_fpr_zero)
	 || (A.type == ppc_fpr_zero && C.type == ppc_fpr_Inf)) {
		gCPU.fpscr |= FPSCR_VXIMZ;
	}
	ppc_fpu_mul(D, A, C);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
//	*((double*)&gCPU.fpr[frD]) = *((double*)(&gCPU.fpr[frA]))*(*((double*)(&gCPU.fpr[frC])));
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fmul.\n");
	}
}
/*
 *	fmulsx		Floating Multipy Single
 *	.502
 */
void ppc_opc_fmulsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frB==0);
	ppc_double A, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	if ((A.type == ppc_fpr_Inf && C.type == ppc_fpr_zero)
	 || (A.type == ppc_fpr_zero && C.type == ppc_fpr_Inf)) {
		gCPU.fpscr |= FPSCR_VXIMZ;
	}
	ppc_fpu_mul(D, A, C);
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fmuls.\n");
	}
}
/*
 *	fnabsx		Floating Negative Absolute Value
 *	.503
 */
void ppc_opc_fnabsx()
{
	int frD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, frA, frB);
	PPC_OPC_ASSERT(frA==0);
	gCPU.fpr[frD] = gCPU.fpr[frB] | FPU_SIGN_BIT;
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fnabs.\n");
	}
}
/*
 *	fnegx		Floating Negate
 *	.504
 */
void ppc_opc_fnegx()
{
	int frD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, frA, frB);
	PPC_OPC_ASSERT(frA==0);
	gCPU.fpr[frD] = gCPU.fpr[frB] ^ FPU_SIGN_BIT;
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fneg.\n");
	}
}
/*
 *	fnmaddx		Floating Negative Multiply-Add (Double-Precision) 
 *	.505
 */
void ppc_opc_fnmaddx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D/*, E*/;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	ppc_fpu_mul_add(D, A, C, B);
	D.s ^= 1;
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fnmadd.\n");
	}
}
/*
 *	fnmaddsx	Floating Negative Multiply-Add Single
 *	.506
 */
void ppc_opc_fnmaddsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	ppc_fpu_mul_add(D, A, C, B);
	D.s ^= 1;
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fnmadds.\n");
	}
}
/*
 *	fnmsubx		Floating Negative Multiply-Subtract (Double-Precision)
 *	.507
 */
void ppc_opc_fnmsubx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	B.s ^= 1;
	ppc_fpu_mul_add(D, A, C, B);
	D.s ^= 1;
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fnmsub.\n");
	}
}
/*
 *	fnsubsx		Floating Negative Multiply-Subtract Single
 *	.508
 */
void ppc_opc_fnmsubsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A, B, C, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_unpack_double(C, gCPU.fpr[frC]);
	B.s ^= 1;
	ppc_fpu_mul_add(D, A, C, B);
	D.s ^= 1;
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fnmsubs.\n");
	}
}
/*
 *	fresx		Floating Reciprocal Estimate Single
 *	.509
 */
void ppc_opc_fresx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frA==0 && frC==0);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fres.\n");
	}
	PPC_FPU_ERR("fres\n");
}
/*
 *	frspx		Floating Round to Single
 *	.511
 */
void ppc_opc_frspx()
{
	int frD, frA, frB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, frA, frB);
	PPC_OPC_ASSERT(frA==0);
	ppc_double B;
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(B, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("frsp.\n");
	}
}
/*
 *	frsqrtex	Floating Reciprocal Square Root Estimate
 *	.512
 */
void ppc_opc_frsqrtex()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frA==0 && frC==0);
	ppc_double B;
	ppc_double D;
	ppc_double E;
	ppc_double Q;
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_sqrt(Q, B);
	E.type = ppc_fpr_norm; E.s = 0; E.e = 0; E.m = 0x80000000000000ULL;
	ppc_fpu_div(D, E, Q);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);	
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("frsqrte.\n");
	}
}
/*
 *	fselx		Floating Select
 *	.514
 */
void ppc_opc_fselx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	ppc_double A;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	if (A.type == ppc_fpr_NaN || (A.type != ppc_fpr_zero && A.s)) {
		gCPU.fpr[frD] = gCPU.fpr[frB];
	} else {
		gCPU.fpr[frD] = gCPU.fpr[frC];
	}
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fsel.\n");
	}
}
/*
 *	fsqrtx		Floating Square Root (Double-Precision)
 *	.515
 */
void ppc_opc_fsqrtx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frA==0 && frC==0);
	ppc_double B;
	ppc_double D;
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	ppc_fpu_sqrt(D, B);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fsqrt.\n");
	}
}
/*
 *	fsqrtsx		Floating Square Root Single
 *	.515
 */
void ppc_opc_fsqrtsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frA==0 && frC==0);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fsqrts.\n");
	}
	PPC_FPU_ERR("fsqrts\n");
}
/*
 *	fsubx		Floating Subtract (Double-Precision)
 *	.517
 */
void ppc_opc_fsubx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frC==0);
	ppc_double A, B, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	if (B.type != ppc_fpr_NaN) {
		B.s ^= 1;
	}
	if (A.s != B.s && A.type == ppc_fpr_Inf && B.type == ppc_fpr_Inf) {
		gCPU.fpscr |= FPSCR_VXISI;
	}
	ppc_fpu_add(D, A, B);
	gCPU.fpscr |= ppc_fpu_pack_double(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fsub.\n");
	}
}
/*
 *	fsubsx		Floating Subtract Single
 *	.518
 */
void ppc_opc_fsubsx()
{
	int frD, frA, frB, frC;
	PPC_OPC_TEMPL_A(gCPU.current_opc, frD, frA, frB, frC);
	PPC_OPC_ASSERT(frC==0);
	ppc_double A, B, D;
	ppc_fpu_unpack_double(A, gCPU.fpr[frA]);
	ppc_fpu_unpack_double(B, gCPU.fpr[frB]);
	if (B.type != ppc_fpr_NaN) {
		B.s ^= 1;
	}
	if (A.s != B.s && A.type == ppc_fpr_Inf && B.type == ppc_fpr_Inf) {
		gCPU.fpscr |= FPSCR_VXISI;
	}
	ppc_fpu_add(D, A, B);
	gCPU.fpscr |= ppc_fpu_pack_double_as_single(D, gCPU.fpr[frD]);
	if (gCPU.current_opc & PPC_OPC_Rc) {
		// update cr1 flags
		PPC_FPU_ERR("fsubs.\n");
	}
}

