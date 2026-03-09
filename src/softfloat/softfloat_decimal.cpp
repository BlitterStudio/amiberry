/*============================================================================

This C source file is an extension to the SoftFloat IEC/IEEE Floating-point 
Arithmetic Package, Release 2a.

=============================================================================*/

#include <stdint.h>

#include "sysconfig.h"
#include "sysdeps.h"

#define DECIMAL_LOG 0

#if DECIMAL_LOG
#define decimal_log write_log
#else
#define decimal_log(fmt, ...)
#endif

#include "softfloat.h"
#include "softfloat-macros.h"
#include "softfloat/softfloat-specialize.h"

/*----------------------------------------------------------------------------
| Methods for converting decimal floats to binary extended precision floats.
*----------------------------------------------------------------------------*/

static void round128to64(flag aSign, int32_t *aExp, uint64_t *aSig0, uint64_t *aSig1, float_status *status)
{
	flag increment;
	int32_t zExp;
	uint64_t zSig0, zSig1;
	
	zExp = *aExp;
	zSig0 = *aSig0;
	zSig1 = *aSig1;
	
	increment = ( (int64_t) zSig1 < 0 );
	if (status->float_rounding_mode != float_round_nearest_even) {
		if (status->float_rounding_mode == float_round_to_zero) {
			increment = 0;
		} else {
			if (aSign) {
				increment = (status->float_rounding_mode == float_round_down) && zSig1;
			} else {
				increment = (status->float_rounding_mode == float_round_up) && zSig1;
			}
		}
	}
	
	if (increment) {
		++zSig0;
		if (zSig0 == 0) {
			++zExp;
			zSig0 = LIT64(0x8000000000000000);
		} else {
			if ((zSig1 << 1) == 0 && status->float_rounding_mode == float_round_nearest_even)
				zSig0 &= ~1;
		}
	} else {
		if ( zSig0 == 0 ) zExp = 0;
	}
	
	*aExp = zExp;
	*aSig0 = zSig0;
	*aSig1 = 0;
}

static void mul128by128round(int32_t *aExp, uint64_t *aSig0, uint64_t *aSig1, int32_t bExp, uint64_t bSig0, uint64_t bSig1, float_status *status)
{
	int32_t zExp;
	uint64_t zSig0, zSig1, zSig2, zSig3;
	
	zExp = *aExp;
	zSig0 = *aSig0;
	zSig1 = *aSig1;
	
	round128to64(0, &bExp, &bSig0, &bSig1, status);
	
	zExp += bExp - 0x3FFE;
	mul128To256(zSig0, zSig1, bSig0, bSig1, &zSig0, &zSig1, &zSig2, &zSig3);
	zSig1 |= (zSig2 | zSig3) != 0;
	if ( 0 < (int64_t) zSig0 ) {
		shortShift128Left( zSig0, zSig1, 1, &zSig0, &zSig1 );
		--zExp;
	}
	*aExp = zExp;
	*aSig0 = zSig0;
	*aSig1 = zSig1;
	
	round128to64(0, aExp, aSig0, aSig1, status);
}

static void mul128by128(int32_t *aExp, uint64_t *aSig0, uint64_t *aSig1, int32_t bExp, uint64_t bSig0, uint64_t bSig1)
{
	int32_t zExp;
	uint64_t zSig0, zSig1, zSig2, zSig3;
	
	zExp = *aExp;
	zSig0 = *aSig0;
	zSig1 = *aSig1;

	zExp += bExp - 0x3FFE;
	mul128To256(zSig0, zSig1, bSig0, bSig1, &zSig0, &zSig1, &zSig2, &zSig3);
	zSig1 |= (zSig2 | zSig3) != 0;
	if ( 0 < (int64_t) zSig0 ) {
		shortShift128Left( zSig0, zSig1, 1, &zSig0, &zSig1 );
		--zExp;
	}
	*aExp = zExp;
	*aSig0 = zSig0;
	*aSig1 = zSig1;
}

static void div128by128(int32_t *paExp, uint64_t *paSig0, uint64_t *paSig1, int32_t bExp, uint64_t bSig0, uint64_t bSig1)
{
	int32_t zExp, aExp;
	uint64_t zSig0, zSig1, aSig0, aSig1;
	uint64_t rem0, rem1, rem2, rem3, term0, term1, term2, term3;
	
	aExp = *paExp;
	aSig0 = *paSig0;
	aSig1 = *paSig1;
	
	zExp = aExp - bExp + 0x3FFE;
	if ( le128( bSig0, bSig1, aSig0, aSig1 ) ) {
		shift128Right( aSig0, aSig1, 1, &aSig0, &aSig1 );
		++zExp;
	}
	zSig0 = estimateDiv128To64( aSig0, aSig1, bSig0 );
	mul128By64To192( bSig0, bSig1, zSig0, &term0, &term1, &term2 );
	sub192( aSig0, aSig1, 0, term0, term1, term2, &rem0, &rem1, &rem2 );
	while ( (int64_t) rem0 < 0 ) {
		--zSig0;
		add192( rem0, rem1, rem2, 0, bSig0, bSig1, &rem0, &rem1, &rem2 );
	}
	zSig1 = estimateDiv128To64( rem1, rem2, bSig0 );
	if ( ( zSig1 & 0x3FFF ) <= 4 ) {
		mul128By64To192( bSig0, bSig1, zSig1, &term1, &term2, &term3 );
		sub192( rem1, rem2, 0, term1, term2, term3, &rem1, &rem2, &rem3 );
		while ( (int64_t) rem1 < 0 ) {
			--zSig1;
			add192( rem1, rem2, rem3, 0, bSig0, bSig1, &rem1, &rem2, &rem3 );
		}
		zSig1 |= ( ( rem1 | rem2 | rem3 ) != 0 );
	}

	*paExp = zExp;
	*paSig0 = zSig0;
	*paSig1 = zSig1;
}

#if 0

void tentoint128(flag mSign, flag eSign, int32_t *aExp, uint64_t *aSig0, uint64_t *aSig1, int32_t scale, float_status *status)
{
    int32_t mExp;
    uint64_t mSig0, mSig1;

    
    *aExp = 0x3FFF;
    *aSig0 = LIT64(0x8000000000000000);
    *aSig1 = 0;

    mExp = 0x4002;
    mSig0 = LIT64(0xA000000000000000);
    mSig1 = 0;

    
    while (scale) {
        if (scale & 1) {
            mul128by128round(aExp, aSig0, aSig1, mExp, mSig0, mSig1, status);
        }
        mul128by128(&mExp, &mSig0, &mSig1, mExp, mSig0, mSig1);
        scale >>= 1;
    }
}

#else

static void tentoint128(flag mSign, flag eSign, int32_t *aExp, uint64_t *aSig0, uint64_t *aSig1, int32_t scale, float_status *status)
 {
    int8_t save_rounding_mode;
    int32_t mExp;
    uint64_t mSig0, mSig1;
     
    save_rounding_mode = status->float_rounding_mode;
    switch (status->float_rounding_mode) {
        case float_round_nearest_even:
            break;
        case float_round_down:
            if (mSign != eSign) {
                set_float_rounding_mode(float_round_up, status);
            }
            break;
        case float_round_up:
            if (mSign != eSign) {
                set_float_rounding_mode(float_round_down, status);
            }
            break;
        case float_round_to_zero:
            if (eSign == 0) {
                set_float_rounding_mode(float_round_down, status);
            } else {
                set_float_rounding_mode(float_round_up, status);
            }
            break;
        default:
            break;
    }	

	*aExp = 0x3FFF;
	*aSig0 = LIT64(0x8000000000000000);
	*aSig1 = 0;

	mExp = 0x4002;
	mSig0 = LIT64(0xA000000000000000);
	mSig1 = 0;
	
	while (scale) {
		if (scale & 1) {
			mul128by128round(aExp, aSig0, aSig1, mExp, mSig0, mSig1, status);
		}
		mul128by128(&mExp, &mSig0, &mSig1, mExp, mSig0, mSig1);
		scale >>= 1;
	}

	set_float_rounding_mode(save_rounding_mode, status);
}

#endif

static int64_t tentointdec(int32_t scale)
{
	uint64_t decM, decX;
	 
	decX = 1;
	decM = 10;
	 
	while (scale) {
		if (scale & 1) {
			decX *= decM;
		}
		decM *= decM;
		scale >>= 1;
	 }
	
	return decX;
}


static int64_t float128toint64(flag zSign, int32_t zExp, uint64_t zSig0, uint64_t zSig1, float_status *status)
{
	int8_t roundingMode;
	flag roundNearestEven, increment;
	int64_t z;
	
	shift128RightJamming(zSig0, zSig1, 0x403E - zExp, &zSig0, &zSig1);

	roundingMode = status->float_rounding_mode;
	roundNearestEven = (roundingMode == float_round_nearest_even);
	increment = ((int64_t)zSig1 < 0);
	if (!roundNearestEven) {
		if (roundingMode == float_round_to_zero) {
			increment = 0;
		} else {
			if (zSign) {
				increment = (roundingMode == float_round_down ) && zSig1;
			} else {
				increment = (roundingMode == float_round_up ) && zSig1;
			}
		}
	}
	if (increment) {
		++zSig0;
		zSig0 &= ~ (((uint64_t)(zSig1<<1) == 0) & roundNearestEven);
	}
	z = zSig0;
	if (zSig1) float_raise(float_flag_inexact, status);
	return z;
}

static int32_t getDecimalExponent(int32_t aExp, uint64_t aSig)
{
	flag zSign;
	int32_t zExp, shiftCount;
	uint64_t zSig0, zSig1;
	
	if (aSig == 0 || aExp == 0x3FFF) {
		return 0;
	}
	if (aExp < 0) {
		return -4932;
	}

	aSig ^= LIT64(0x8000000000000000);
	aExp -= 0x3FFF;
	zSign = (aExp < 0);
	aExp = zSign ? -aExp : aExp;
	shiftCount = 31 - countLeadingZeros32(aExp);
	zExp = 0x3FFF + shiftCount;
	
	if (shiftCount < 0) {
		shortShift128Left(aSig, 0, -shiftCount, &zSig0, &zSig1);
	} else {
		shift128Right(aSig, 0, shiftCount, &zSig0, &zSig1);
		aSig = (uint64_t)aExp << (63 - shiftCount);
		if (zSign) {
			sub128(aSig, 0, zSig0, zSig1, &zSig0, &zSig1);
		} else {
			add128(aSig, 0, zSig0, zSig1, &zSig0, &zSig1);
		}
	}
	
	shiftCount = countLeadingZeros64(zSig0);
	shortShift128Left(zSig0, zSig1, shiftCount, &zSig0, &zSig1);
	zExp -= shiftCount;
	mul128by128(&zExp, &zSig0, &zSig1, 0x3FFD, LIT64(0x9A209A84FBCFF798), LIT64(0x8F8959AC0B7C9178));
	
	shiftCount = 0x403E - zExp;
	shift128RightJamming(zSig0, zSig1, shiftCount, &zSig0, &zSig1);

	if ((int64_t)zSig1 < 0) {
		++zSig0;
		zSig0 &= ~(((int64_t)(zSig1<<1) == 0) & 1);
	}
	
	zExp = (int32_t)(zSign ? (0 - zSig0) : zSig0);

	return zExp;
}

/*----------------------------------------------------------------------------
| Decimal to binary
*----------------------------------------------------------------------------*/

floatx80 floatdecimal_to_floatx80(floatx80 a, float_status *status)
{
	flag decSign, zSign, decExpSign;
	int32_t decExp, zExp, xExp, shiftCount;
	uint64_t decSig, zSig0, zSig1, xSig0, xSig1;
	
	decSign = extractFloatx80Sign(a);
	decExp = extractFloatx80Exp(a);
	decSig = extractFloatx80Frac(a);
	
	if (decExp == 0x7FFF) return a;
	
	if (decExp == 0 && decSig == 0) return a;
	
	decExpSign = (decExp >> 14) & 1;
	decExp &= 0x3FFF;
	
	shiftCount = countLeadingZeros64( decSig );
	zExp = 0x403E - shiftCount;
	zSig0 = decSig << shiftCount;
	zSig1 = 0;
	zSign = decSign;
	
	tentoint128(decSign, decExpSign, &xExp, &xSig0, &xSig1, decExp, status);

	if (decExpSign) {
		div128by128(&zExp, &zSig0, &zSig1, xExp, xSig0, xSig1);
	} else {
		mul128by128(&zExp, &zSig0, &zSig1, xExp, xSig0, xSig1);
	}
	
	if (zSig1) float_raise(float_flag_decimal, status);
	round128to64(zSign, &zExp, &zSig0, &zSig1, status);
	
	return packFloatx80( zSign, zExp, zSig0 );
	
}

/*----------------------------------------------------------------------------
 | Binary to decimal
 *----------------------------------------------------------------------------*/

floatx80 floatx80_to_floatdecimal(floatx80 a, int32_t *k, float_status *status)
{
	flag aSign, decSign;
	int32_t aExp, decExp, zExp, xExp;
	uint64_t aSig, decSig, decX, zSig0, zSig1, xSig0, xSig1;
	flag ictr, lambda;
	int32_t kfactor, ilog, iscale, len;
	
	aSign = extractFloatx80Sign(a);
	aExp = extractFloatx80Exp(a);
	aSig = extractFloatx80Frac(a);
	
	if (aExp == 0x7FFF) {
		if ((uint64_t) (aSig<<1)) return propagateFloatx80NaNOneArg(a, status);
		return a;
	}
	
	if (aExp == 0) {
		if (aSig == 0) return packFloatx80(aSign, 0, 0);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
	}

	kfactor = *k;

	ilog = getDecimalExponent(aExp, aSig);
	
	ictr = 0;

try_again:
	decimal_log(_T("ILOG = %i\n"), ilog);
	
	if (kfactor > 0) {
		if (kfactor > 17) {
			kfactor = 17;
			float_raise(float_flag_invalid, status);
		}
		len = kfactor;
	} else {
		len = ilog + 1 - kfactor;
		if (len > 17) {
			len = 17;
		}
		if (len < 1) {
			len = 1;
		}
		if (kfactor > ilog) {
			ilog = kfactor;
			decimal_log(_T("ILOG is kfactor = %i\n"), ilog);
		}
	}
	
	decimal_log(_T("LEN = %i\n"),len);
	
	lambda = 0;
	iscale = ilog + 1 - len;

	if (iscale < 0) {
		lambda = 1;
		iscale = -iscale;
	}
	
	decimal_log(_T("ISCALE = %i, LAMBDA = %i\n"),iscale, lambda);
	
	tentoint128(lambda, 0, &xExp, &xSig0, &xSig1, iscale, status);

	decimal_log(_T("AFTER tentoint128: zExp = %04x, zSig0 = %16llx, zSig1 = %16llx\n"), xExp, xSig0, xSig1);

	zExp = aExp;
	zSig0 = aSig;
	zSig1 = 0;
	
	if (lambda) {
		mul128by128(&zExp, &zSig0, &zSig1, xExp, xSig0, xSig1);
	} else {
		div128by128(&zExp, &zSig0, &zSig1, xExp, xSig0, xSig1);
	}

	decimal_log(_T("BEFORE: zExp = %04x, zSig0 = %16llx, zSig1 = %16llx\n"),zExp,zSig0,zSig1);

	decSig = float128toint64(aSign, zExp, zSig0, zSig1, status);

	decimal_log(_T("AFTER: decSig = %llu\n"),decSig);

	if (ictr == 0) {

		decX = tentointdec(len - 1);

		if (decSig < decX) { // z < x
			ilog -= 1;
			ictr = 1;
			goto try_again;
		}
		
		decX *= 10;
		
		if (decSig > decX) { // z > x
			ilog += 1;
			ictr = 1;
			goto try_again;
		}
	}
	
	decSign = aSign;
	decExp = (ilog < 0) ? -ilog : ilog;
	if (decExp > 999) {
		float_raise(float_flag_invalid, status);
	}
	if (ilog < 0) decExp |= 0x4000;
	
	*k = len;
	
	return packFloatx80(decSign, decExp, decSig);
}
