/*
 *	PearPC
 *	ppc_fpu.h
 *
 *	Copyright (C) 2003, 2004 Sebastian Biallas (sb@biallas.net)
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
 
#ifndef __PPC_FPU_H__
#define __PPC_FPU_H__


#define FPU_SIGN_BIT (0x8000000000000000ULL)

#define FPD_SIGN(v) (((v)&FPU_SIGN_BIT)?1:0)
#define FPD_EXP(v) ((v)>>52)
#define FPD_FRAC(v) ((v)&0x000fffffffffffffULL)

#define FPS_SIGN(v) ((v)&0x80000000)
#define FPS_EXP(v) ((v)>>23)
#define FPS_FRAC(v) ((v)&0x007fffff)

// m must be uint64
#define FPD_PACK_VAR(f, s, e, m) (f) = ((s)?FPU_SIGN_BIT:0ULL)|((((uint64)(e))&0x7ff)<<52)|((m)&((1ULL<<52)-1))
#define FPD_UNPACK_VAR(f, s, e, m) {(s)=FPD_SIGN(f);(e)=FPD_EXP(f)&0x7ff;(m)=FPD_FRAC(f);}

#define FPS_PACK_VAR(f, s, e, m) (f) = ((s)?0x80000000:0)|((e)<<23)|((m)&0x7fffff)
#define FPS_UNPACK_VAR(f, s, e, m) {(s)=FPS_SIGN(f);(e)=FPS_EXP(f)&0xff;(m)=FPS_FRAC(f);}

#define FPD_UNPACK(freg, fvar) FPD_UNPACK(freg, fvar.s, fvar.e, fvar.m)


void ppc_fpu_test();

enum ppc_fpr_type {
	ppc_fpr_norm,
	ppc_fpr_zero,
	ppc_fpr_NaN,
	ppc_fpr_Inf,
};

struct ppc_quadro {
	ppc_fpr_type type;
	int s;
	int e;
	uint64 m0;	// most  significant
	uint64 m1;	// least significant
};

struct ppc_double {
	ppc_fpr_type type;
	int s;
	int e;
	uint64 m;
};

struct ppc_single {
	ppc_fpr_type type;
	int s;
	int e;
	uint m;
};

inline int ppc_count_leading_zeros(uint64 i)
{
	int ret;
	uint32 dd = i >> 32;
	if (dd) {
		ret = 31;
		if (dd > 0xffff) { ret -= 16; dd >>= 16; }
		if (dd > 0xff) { ret -= 8; dd >>= 8; }
		if (dd & 0xf0) { ret -= 4; dd >>= 4; }
		if (dd & 0xc) { ret -= 2; dd >>= 2; }
		if (dd & 0x2) ret--;
	} else {
		dd = (uint32)i;
		ret = 63;
		if (dd > 0xffff) { ret -= 16; dd >>= 16; }
		if (dd > 0xff) { ret -= 8; dd >>= 8; }
		if (dd & 0xf0) { ret -= 4; dd >>= 4; }
		if (dd & 0xc) { ret -= 2; dd >>= 2; }
		if (dd & 0x2) ret--;
	}
	return ret;
}

inline int ppc_fpu_normalize_quadro(ppc_quadro &d)
{
	int ret = d.m0 ? ppc_count_leading_zeros(d.m0) : 64 + ppc_count_leading_zeros(d.m1);
	return ret;
}

inline int ppc_fpu_normalize(ppc_double &d)
{
	return ppc_count_leading_zeros(d.m);
}

inline int ppc_fpu_normalize_single(ppc_single &s)
{
	int ret;
	uint32 dd = s.m;
	ret = 31;
	if (dd > 0xffff) { ret -= 16; dd >>= 16; }
	if (dd > 0xff) { ret -= 8; dd >>= 8; }
	if (dd & 0xf0) { ret -= 4; dd >>= 4; }
	if (dd & 0xc) { ret -= 2; dd >>= 2; }
	if (dd & 0x2) ret--;
	return ret;
}

#include "tools/snprintf.h"
inline void ppc_fpu_unpack_double(ppc_double &res, uint64 d)
{
	FPD_UNPACK_VAR(d, res.s, res.e, res.m);
//	ht_printf("ud: %qx: s:%d e:%d m:%qx\n", d, res.s, res.e, res.m);
	// .124
	if (res.e == 2047) {
		if (res.m == 0) {
			res.type = ppc_fpr_Inf;
		} else {
			res.type = ppc_fpr_NaN;
		}
	} else if (res.e == 0) {
		if (res.m == 0) {
			res.type = ppc_fpr_zero;
		} else {
			// normalize denormalized exponent
			int diff = ppc_fpu_normalize(res) - 8;
			res.m <<= diff+3;
			res.e -= 1023 - 1 + diff;
			res.type = ppc_fpr_norm;
		}
	} else {
		res.e -= 1023; // unbias exponent
		res.type = ppc_fpr_norm;
		// add implied bit
		res.m |= 1ULL<<52;
		res.m <<= 3;
	}
//	ht_printf("ud: %qx: s:%d e:%d m:%qx\n", d, res.s, res.e, res.m);
}


inline void ppc_fpu_unpack_single(ppc_single &res, uint32 d)
{
	FPS_UNPACK_VAR(d, res.s, res.e, res.m);
	// .124
	if (res.e == 255) {
		if (res.m == 0) {
			res.type = ppc_fpr_Inf;
		} else {
			res.type = ppc_fpr_NaN;
		}
	} else if (res.e == 0) {
		if (res.m == 0) {
			res.type = ppc_fpr_zero;
		} else {
			// normalize denormalized exponent
			int diff = ppc_fpu_normalize_single(res) - 8;
			res.m <<= diff+3;
			res.e -= 127 - 1 + diff;
			res.type = ppc_fpr_norm;
		}
	} else {
		res.e -= 127; // unbias exponent
		res.type = ppc_fpr_norm;
		// add implied bit
		res.m |= 1<<23;
		res.m <<= 3;
	}
}

inline uint32 ppc_fpu_round(ppc_double &d) 
{
	// .132
	switch (FPSCR_RN(gCPU.fpscr)) {
	case FPSCR_RN_NEAR:
		if (d.m & 0x7) {
			if ((d.m & 0x7) != 4) {
				d.m += 4;
			} else if (d.m & 8) {
				d.m += 4;
			}
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_ZERO:
		if (d.m & 0x7) {
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_PINF:
		if (!d.s && (d.m & 0x7)) {
			d.m += 8;
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_MINF:
		if (d.s && (d.m & 0x7)) {
			d.m += 8;
			return FPSCR_XX;
		}
		return 0;
	}
	return 0;
}

inline uint32 ppc_fpu_round_single(ppc_single &s) 
{
	switch (FPSCR_RN(gCPU.fpscr)) {
	case FPSCR_RN_NEAR:
		if (s.m & 0x7) {
			if ((s.m & 0x7) != 4) {
				s.m += 4;
			} else if (s.m & 8) {
				s.m += 4;
			}
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_ZERO:
		if (s.m & 0x7) {
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_PINF:
		if (!s.s && (s.m & 0x7)) {
			s.m += 8;
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_MINF:
		if (s.s && (s.m & 0x7)) {
			s.m += 8;
			return FPSCR_XX;
		}
		return 0;
	}
	return 0;
}

inline uint32 ppc_fpu_round_single(ppc_double &s) 
{
	switch (FPSCR_RN(gCPU.fpscr)) {
	case FPSCR_RN_NEAR:
		if (s.m & 0x7) {
			if ((s.m & 0x7) != 4) {
				s.m += 4;
			} else if (s.m & 8) {
				s.m += 4;
			}
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_ZERO:
		if (s.m & 0x7) {
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_PINF:
		if (!s.s && (s.m & 0x7)) {
			s.m += 8;
			return FPSCR_XX;
		}
		return 0;
	case FPSCR_RN_MINF:
		if (s.s && (s.m & 0x7)) {
			s.m += 8;
			return FPSCR_XX;
		}
		return 0;
	}
	return 0;
}

inline uint32 ppc_fpu_pack_double(ppc_double &d, uint64 &res)
{
	// .124
	uint32 ret = 0;
//	ht_printf("pd_type: %d\n", d.type);
	switch (d.type) {
	case ppc_fpr_norm:
//		ht_printf("pd: %qx: s:%d e:%d m:%qx\n", d, d.s, d.e, d.m);
		d.e += 1023; // bias exponent
//		ht_printf("pd: %qx: s:%d e:%d m:%qx\n", d, d.s, d.e, d.m);
		if (d.e > 0) {
			ret |= ppc_fpu_round(d);
			if (d.m & (1ULL<<56)) {
				d.e++;
				d.m >>= 4;
			} else {
				d.m >>= 3;
			}
			if (d.e >= 2047) {
				d.e = 2047;
				d.m = 0;
				ret |= FPSCR_OX;
			}
		} else {
			// number is denormalized
			d.e = -d.e+1;
			if (d.e <= 56) {
				d.m >>= d.e;
				ret |= ppc_fpu_round(d);
				d.m <<= 1;
				if (d.m & (1ULL<<56)) {
					d.e = 1;
					d.m = 0;
				} else {
					d.e = 0;
					d.m >>= 4;
					ret |= FPSCR_UX;
				}
			} else {
				// underflow to zero
				d.e = 0;
				d.m = 0;
				ret |= FPSCR_UX;
			}
		}
		break;
	case ppc_fpr_zero:
		d.e = 0;
		d.m = 0;
		break;
	case ppc_fpr_NaN:
		d.e = 2047;
		d.m = 1;		
		break;
	case ppc_fpr_Inf:
		d.e = 2047;
		d.m = 0;
		break;
	}
//	ht_printf("pd: %qx: s:%d e:%d m:%qx\n", d, d.s, d.e, d.m);
	FPD_PACK_VAR(res, d.s, d.e, d.m);
	return ret;
}

inline uint32 ppc_fpu_pack_single(ppc_double &d, uint32 &res)
{
	// .124
	uint32 ret = 0;
	switch (d.type) {
	case ppc_fpr_norm:
//		ht_printf("ps: %qx: s:%d e:%d m:%qx\n", d, d.s, d.e, d.m);
		d.e += 127; // bias exponent
		d.m >>= 29;
//		ht_printf("ps: %qx: s:%d e:%d m:%qx\n", d, d.s, d.e, d.m);
		if (d.e > 0) {
			ret |= ppc_fpu_round_single(d);
			if (d.m & (1ULL<<27)) {
				d.e++;
				d.m >>= 4;
			} else {
				d.m >>= 3;
			}
			if (d.e >= 255) {
				d.e = 255;
				d.m = 0;
				ret |= FPSCR_OX;
			}
		} else {
			// number is denormalized
			d.e = -d.e+1;
			if (d.e <= 27) {
				d.m >>= d.e;
				ret |= ppc_fpu_round_single(d);
				d.m <<= 1;
				if (d.m & (1ULL<<27)) {
					d.e = 1;
					d.m = 0;
				} else {
					d.e = 0;
					d.m >>= 4;
					ret |= FPSCR_UX;
				}
			} else {
				// underflow to zero
				d.e = 0;
				d.m = 0;
				ret |= FPSCR_UX;
			}
		}
		break;
	case ppc_fpr_zero:
		d.e = 0;
		d.m = 0;
		break;
	case ppc_fpr_NaN:
		d.e = 255;
		d.m = 1;
		break;
	case ppc_fpr_Inf:
		d.e = 255;
		d.m = 0;
		break;
	}
//	ht_printf("ps: %qx: s:%d e:%d m:%qx\n", d, d.s, d.e, d.m);
	FPS_PACK_VAR(res, d.s, d.e, d.m);
	return ret;
}

inline void ppc_fpu_single_to_double(ppc_single &s, ppc_double &d) 
{
	d.s = s.s;
	d.e = s.e;
	d.m = ((uint64)s.m)<<29;
	d.type = s.type;
}

inline uint32 ppc_fpu_pack_double_as_single(ppc_double &d, uint64 &res)
{
	// .757
	ppc_single s;
	s.m = d.m >> 29;
	s.e = d.e;
	s.s = d.s;
	s.type = d.type;
	uint32 ret = 0;
	
	switch (s.type) {
	case ppc_fpr_norm: 
		s.e = d.e+127;
		if (s.e > 0) {
			ret |= ppc_fpu_round_single(s);
			if (s.m & (1<<27)) {
				s.e++;
				s.m >>= 4;
			} else {
				s.m >>= 3;
			}
			if (s.e >= 255) {
				s.type = ppc_fpr_Inf;
				s.e = 255;
				s.m = 0;
				ret |= FPSCR_OX;
			}
			d.e = s.e-127;
		} else {
			// number is denormalized
			s.e = -s.e+1;
			if (s.e <= 27) {
				s.m >>= s.e;
				ret |= ppc_fpu_round_single(s);
				s.m <<= 1;
				if (s.m & (1<<27)) {
					s.e = 1;
					s.m = 0;
				} else {
					s.e = 0;
					s.m >>= 4;
					ret |= FPSCR_UX;
				}
			} else {
				// underflow to zero
				s.type = ppc_fpr_zero;
				s.e = 0;
				s.m = 0;
				ret |= FPSCR_UX;
			}
		}
		break;
	case ppc_fpr_zero:
		s.e = 0;
		s.m = 0;
		break;
	case ppc_fpr_NaN:
		s.e = 2047;
		s.m = 1;		
		break;
	case ppc_fpr_Inf:
		s.e = 2047;
		s.m = 0;
		break;
	}	
	if (s.type == ppc_fpr_norm) {
		d.m = ((uint64)(s.m))<<32;
	} else {
		d.m = s.m;
	}
//	ht_printf("dm: %qx\n", d.m);
	ret |= ppc_fpu_pack_double(d, res);
	return ret;
}

inline uint32 ppc_fpu_double_to_int(ppc_double &d)
{
	switch (d.type) {
	case ppc_fpr_norm: {
		if (d.e < 0) {
			switch (FPSCR_RN(gCPU.fpscr)) {
			case FPSCR_RN_NEAR:
				if (d.e < -1) {
					return 0;
				} else {
					return d.s ? (uint32)-1 : 1;
				}
			case FPSCR_RN_ZERO:
				return 0;
			case FPSCR_RN_PINF:
				if (d.s) {
					return 0;
				} else {
					return 1;
				}
			case FPSCR_RN_MINF:
				if (d.s) {
					return (uint32)-1;
				} else {
					return 0;
				}
			}
		}
		if (d.e >= 31) {
			if (d.s) {
				return 0x80000000;
			} else {
				return 0x7fffffff;
			}
		}		
		int i=0;
		uint64 mask = (1ULL<<(56 - d.e - 1))-1;
		// we have to round
		switch (FPSCR_RN(gCPU.fpscr)) {
		case FPSCR_RN_NEAR:
			if (d.m & mask) {
				if (d.m & (1ULL<<(56 - d.e - 2))) {
					i = 1;
				}
			}
			break;
		case FPSCR_RN_ZERO:
			break;
		case FPSCR_RN_PINF:
			if (!d.s && (d.m & mask)) {
				i = 1;
			}
			break;
		case FPSCR_RN_MINF:
			if (d.s && (d.m & mask)) {
				i = 1;
			}
			break;
		}
		d.m >>= 56 - d.e - 1;
		d.m += i;
		return d.s ? -d.m : d.m;
	}
	case ppc_fpr_zero:
		return 0;
	case ppc_fpr_Inf:
	case ppc_fpr_NaN:
		if (d.s) {
			return 0x80000000;
		} else {
			return 0x7fffffff;
		}
	}
	return 0;
}

double ppc_fpu_get_double(uint64 d);
double ppc_fpu_get_double(ppc_double &d);

void ppc_opc_fabsx();
void ppc_opc_faddx();
void ppc_opc_faddsx();
void ppc_opc_fcmpo();
void ppc_opc_fcmpu();
void ppc_opc_fctiwx();
void ppc_opc_fctiwzx();
void ppc_opc_fdivx();
void ppc_opc_fdivsx();
void ppc_opc_fmaddx();
void ppc_opc_fmaddsx();
void ppc_opc_fmrx();
void ppc_opc_fmsubx();
void ppc_opc_fmsubsx();
void ppc_opc_fmulx();
void ppc_opc_fmulsx();
void ppc_opc_fnabsx();
void ppc_opc_fnegx();
void ppc_opc_fnmaddx();
void ppc_opc_fnmaddsx();
void ppc_opc_fnmsubx();
void ppc_opc_fnmsubsx();
void ppc_opc_fresx();
void ppc_opc_frspx();
void ppc_opc_frsqrtex();
void ppc_opc_fselx();
void ppc_opc_fsqrtx();
void ppc_opc_fsqrtsx();
void ppc_opc_fsubx();
void ppc_opc_fsubsx();

#endif
