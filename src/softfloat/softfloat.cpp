
#define SOFTFLOAT_68K

#include <stdint.h>
#include <stdlib.h>
#include "softfloat/softfloat.h"


/*
 * QEMU float support
 *
 * The code in this source file is derived from release 2a of the SoftFloat
 * IEC/IEEE Floating-point Arithmetic Package. Those parts of the code (and
 * some later contributions) are provided under that license, as detailed below.
 * It has subsequently been modified by contributors to the QEMU Project,
 * so some portions are provided under:
 *  the SoftFloat-2a license
 *  the BSD license
 *  GPL-v2-or-later
 *
 * Any future contributions to this file after December 1st 2014 will be
 * taken to be licensed under the Softfloat-2a license unless specifically
 * indicated otherwise.
 */

/*
===============================================================================
This C source file is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2a.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://HTTP.CS.Berkeley.EDU/~jhauser/
arithmetic/SoftFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort
has been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT
TIMES RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO
PERSONS AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ANY
AND ALL LOSSES, COSTS, OR OTHER PROBLEMS ARISING FROM ITS USE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) they include prominent notice that the work is derivative, and (2) they
include prominent notice akin to these four paragraphs for those parts of
this code that are retained.

===============================================================================
*/

/* BSD licensing:
 * Copyright (c) 2006, Fabrice Bellard
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Portions of this work are licensed under the terms of the GNU GPL,
 * version 2 or later. See the COPYING file in the top-level directory.
 */

/* We only need stdlib for abort() */

/*----------------------------------------------------------------------------
| Primitive arithmetic functions, including multi-word arithmetic, and
| division and square root approximations.  (Can be specialized to target if
| desired.)
*----------------------------------------------------------------------------*/
#include "softfloat-macros.h"

/*----------------------------------------------------------------------------
 | Variables for storing sign, exponent and significand of internal extended
 | double-precision floating-point value for external use.
 *----------------------------------------------------------------------------*/
flag floatx80_internal_sign = 0;
int32_t floatx80_internal_exp = 0;
uint64_t floatx80_internal_sig = 0;
int32_t floatx80_internal_exp0 = 0;
uint64_t floatx80_internal_sig0 = 0;
uint64_t floatx80_internal_sig1 = 0;
int8_t floatx80_internal_precision = 80;
int8_t floatx80_internal_mode = float_round_nearest_even;

/*----------------------------------------------------------------------------
 | Functions for storing sign, exponent and significand of extended
 | double-precision floating-point intermediate result for external use.
 *----------------------------------------------------------------------------*/
floatx80 roundSaveFloatx80Internal( int8_t roundingPrecision, flag zSign, int32_t zExp, uint64_t zSig0, uint64_t zSig1, float_status *status )
{
    int64_t roundIncrement, roundMask, roundBits;
    flag increment;
    
    if ( roundingPrecision == 80 ) {
        goto precision80;
    } else if ( roundingPrecision == 64 ) {
        roundIncrement = LIT64( 0x0000000000000400 );
        roundMask = LIT64( 0x00000000000007FF );
    } else if ( roundingPrecision == 32 ) {
        roundIncrement = LIT64( 0x0000008000000000 );
        roundMask = LIT64( 0x000000FFFFFFFFFF );
    } else {
        goto precision80;
    }
    
    zSig0 |= ( zSig1 != 0 );
    if ( status->float_rounding_mode != float_round_nearest_even ) {
        if ( status->float_rounding_mode == float_round_to_zero ) {
            roundIncrement = 0;
        } else {
            roundIncrement = roundMask;
            if ( zSign ) {
                if ( status->float_rounding_mode == float_round_up ) roundIncrement = 0;
            } else {
                if ( status->float_rounding_mode == float_round_down ) roundIncrement = 0;
            }
        }
    }
    
    roundBits = zSig0 & roundMask;
    
    zSig0 += roundIncrement;
    if ( zSig0 < roundIncrement ) {
        ++zExp;
        zSig0 = LIT64( 0x8000000000000000 );
    }
    roundIncrement = roundMask + 1;
    if ( status->float_rounding_mode == float_round_nearest_even && ( roundBits<<1 == roundIncrement ) ) {
        roundMask |= roundIncrement;
    }
    zSig0 &= ~ roundMask;
    if ( zSig0 == 0 ) zExp = 0;
    return packFloatx80( zSign, zExp, zSig0 );

precision80:
    increment = ( (int64_t) zSig1 < 0 );
    if ( status->float_rounding_mode != float_round_nearest_even ) {
        if ( status->float_rounding_mode == float_round_to_zero ) {
            increment = 0;
        } else {
            if ( zSign ) {
                increment = ( status->float_rounding_mode == float_round_down ) && zSig1;
            } else {
                increment = ( status->float_rounding_mode == float_round_up ) && zSig1;
            }
        }
    }
    if ( increment ) {
        ++zSig0;
        if ( zSig0 == 0 ) {
            ++zExp;
            zSig0 = LIT64( 0x8000000000000000 );
        } else {
            zSig0 &= ~ ( ( (uint64_t) ( zSig1<<1 ) == 0 ) & ( status->float_rounding_mode == float_round_nearest_even ) );
        }
    } else {
        if ( zSig0 == 0 ) zExp = 0;
    }
    return packFloatx80( zSign, zExp, zSig0 );
}

static void saveFloatx80Internal( int8_t prec, flag zSign, int32_t zExp, uint64_t zSig0, uint64_t zSig1, float_status *status )
{
    floatx80_internal_sign = zSign;
    floatx80_internal_exp = zExp;
    floatx80_internal_sig0 = zSig0;
    floatx80_internal_sig1 = zSig1;
    floatx80_internal_precision = prec;
    floatx80_internal_mode = status->float_rounding_mode;
}

static void saveFloat64Internal( flag zSign, int16_t zExp, uint64_t zSig, float_status *status )
{
    floatx80_internal_sign = zSign;
    floatx80_internal_exp = zExp + 0x3C01;
    floatx80_internal_sig0 = zSig<<1;
    floatx80_internal_sig1 = 0;
    floatx80_internal_precision = 64;
    floatx80_internal_mode = status->float_rounding_mode;
}

static void saveFloat32Internal( flag zSign, int16_t zExp, uint32_t zSig, float_status *status )
{
	floatx80 z = roundSaveFloatx80Internal( 32, zSign, zExp + 0x3F81, ( (uint64_t) zSig )<<33, 0, status );

    floatx80_internal_sign = zSign;
    floatx80_internal_exp = extractFloatx80Exp( z );
    floatx80_internal_sig = extractFloatx80Frac( z );
    floatx80_internal_exp0 = zExp + 0x3F81;
    floatx80_internal_sig0 = ( (uint64_t) zSig )<<33;
    floatx80_internal_sig1 = 0;
}

/*----------------------------------------------------------------------------
 | Functions for returning sign, exponent and significand of extended
 | double-precision floating-point intermediate result for external use.
 *----------------------------------------------------------------------------*/

void getRoundedFloatInternal( int8_t roundingPrecision, flag *pzSign, int32_t *pzExp, uint64_t *pzSig )
{
    int64_t roundIncrement, roundMask, roundBits;
    flag increment;

    flag zSign = floatx80_internal_sign;
    int32_t zExp = floatx80_internal_exp;
    uint64_t zSig0 = floatx80_internal_sig0;
    uint64_t zSig1 = floatx80_internal_sig1;
    
    if ( roundingPrecision == 80 ) {
        goto precision80;
    } else if ( roundingPrecision == 64 ) {
        roundIncrement = LIT64( 0x0000000000000400 );
        roundMask = LIT64( 0x00000000000007FF );
    } else if ( roundingPrecision == 32 ) {
        roundIncrement = LIT64( 0x0000008000000000 );
        roundMask = LIT64( 0x000000FFFFFFFFFF );
    } else {
        goto precision80;
    }
    
    zSig0 |= ( zSig1 != 0 );
    if ( floatx80_internal_mode != float_round_nearest_even ) {
        if ( floatx80_internal_mode == float_round_to_zero ) {
            roundIncrement = 0;
        } else {
            roundIncrement = roundMask;
            if ( zSign ) {
                if ( floatx80_internal_mode == float_round_up ) roundIncrement = 0;
            } else {
                if ( floatx80_internal_mode == float_round_down ) roundIncrement = 0;
            }
        }
    }
    
    roundBits = zSig0 & roundMask;
    
    zSig0 += roundIncrement;
    if ( zSig0 < roundIncrement ) {
        ++zExp;
        zSig0 = LIT64( 0x8000000000000000 );
    }
    roundIncrement = roundMask + 1;
    if ( floatx80_internal_mode == float_round_nearest_even && ( roundBits<<1 == roundIncrement ) ) {
        roundMask |= roundIncrement;
    }
    zSig0 &= ~ roundMask;
    if ( zSig0 == 0 ) zExp = 0;
    
    *pzSign = zSign;
    *pzExp = zExp;
    *pzSig = zSig0;
    return;
    
precision80:
    increment = ( (int64_t) zSig1 < 0 );
    if ( floatx80_internal_mode != float_round_nearest_even ) {
        if ( floatx80_internal_mode == float_round_to_zero ) {
            increment = 0;
        } else {
            if ( zSign ) {
                increment = ( floatx80_internal_mode == float_round_down ) && zSig1;
            } else {
                increment = ( floatx80_internal_mode == float_round_up ) && zSig1;
            }
        }
    }
    if ( increment ) {
        ++zSig0;
        if ( zSig0 == 0 ) {
            ++zExp;
            zSig0 = LIT64( 0x8000000000000000 );
        } else {
            zSig0 &= ~ ( ( (uint64_t) ( zSig1<<1 ) == 0 ) & ( floatx80_internal_mode == float_round_nearest_even ) );
        }
    } else {
        if ( zSig0 == 0 ) zExp = 0;
    }
    
    *pzSign = zSign;
    *pzExp = zExp;
    *pzSig = zSig0;
}

floatx80 getFloatInternalOverflow( void )
{
    flag zSign;
    int32_t zExp;
    uint64_t zSig;
    
    getRoundedFloatInternal( floatx80_internal_precision, &zSign, &zExp, &zSig );
    
    if (zExp > (0x7fff + 0x6000)) { // catastrophic
        zExp = 0;
    } else {
        zExp -= 0x6000;
    }

    return packFloatx80( zSign, zExp, zSig );
    
}

floatx80 getFloatInternalUnderflow( void )
{
    flag zSign;
    int32_t zExp;
    uint64_t zSig;
    
    getRoundedFloatInternal( floatx80_internal_precision, &zSign, &zExp, &zSig );
    
    if (zExp < (0x0000 - 0x6000)) { // catastrophic
        zExp = 0;
    } else {
        zExp += 0x6000;
    }
    
    return packFloatx80( zSign, zExp, zSig );
    
}

floatx80 getFloatInternalRoundedAll( void )
{
    flag zSign;
    int32_t zExp;
    uint64_t zSig, zSig32, zSig64, zSig80;
    
    if (floatx80_internal_precision == 80) {
        getRoundedFloatInternal( 80, &zSign, &zExp, &zSig80 );
        zSig = zSig80;
    } else if (floatx80_internal_precision == 64) {
        getRoundedFloatInternal( 80, &zSign, &zExp, &zSig80 );
        getRoundedFloatInternal( 64, &zSign, &zExp, &zSig64 );
        zSig = zSig64;
        zSig |= zSig80 & LIT64( 0x00000000000007FF );
    } else {
        getRoundedFloatInternal( 80, &zSign, &zExp, &zSig80 );
        getRoundedFloatInternal( 64, &zSign, &zExp, &zSig64 );
        getRoundedFloatInternal( 32, &zSign, &zExp, &zSig32 );
        zSig = zSig32;
        zSig |= zSig64 & LIT64( 0x000000FFFFFFFFFF );
        zSig |= zSig80 & LIT64( 0x00000000000007FF );
    }

    return packFloatx80( zSign, zExp & 0x7FFF, zSig );

}

floatx80 getFloatInternalRoundedSome( void )
{
    flag zSign;
    int32_t zExp;
    uint64_t zSig, zSig32, zSig64, zSig80;
    
    if (floatx80_internal_precision == 80) {
        getRoundedFloatInternal( 80, &zSign, &zExp, &zSig80 );
        zSig = zSig80;
    } else if (floatx80_internal_precision == 64) {
        getRoundedFloatInternal( 64, &zSign, &zExp, &zSig64 );
        zSig80 = floatx80_internal_sig0;
        if (zSig64 != (zSig80 & LIT64( 0xFFFFFFFFFFFFF800 ))) {
            zSig80++;
        }
        zSig = zSig64;
        zSig |= zSig80 & LIT64( 0x00000000000007FF );
    } else {
        getRoundedFloatInternal( 32, &zSign, &zExp, &zSig32 );
        zSig80 = floatx80_internal_sig0;
        if (zSig32 != (zSig80 & LIT64( 0xFFFFFF0000000000 ))) {
           zSig80++;
        }
        zSig = zSig32;
        zSig |= zSig80 & LIT64( 0x000000FFFFFFFFFF );
    }
    
    return packFloatx80( zSign, zExp & 0x7FFF, zSig );
    
}

floatx80 getFloatInternalFloatx80( void )
{
    flag zSign;
    int32_t zExp;
    uint64_t zSig;
    
    getRoundedFloatInternal( 80, &zSign, &zExp, &zSig );
    
    return packFloatx80( zSign, zExp & 0x7FFF, zSig );
    
}

floatx80 getFloatInternalUnrounded( void )
{
    flag zSign = floatx80_internal_sign;
    int32_t zExp = floatx80_internal_exp;
    uint64_t zSig = floatx80_internal_sig0;
    
    return packFloatx80( zSign, zExp & 0x7FFF, zSig );
    
}

uint64_t getFloatInternalGRS( void )
{
#if 1
    if (floatx80_internal_sig1)
        return 5;
    
    if (floatx80_internal_precision == 64 &&
        floatx80_internal_sig0 & LIT64( 0x00000000000007FF )) {
        return 1;
    }
    if (floatx80_internal_precision == 32 &&
        floatx80_internal_sig0 & LIT64( 0x000000FFFFFFFFFF )) {
        return 1;
    }
    
    return 0;
#else
    uint64_t roundbits;
    shift64RightJamming(floatx80_internal_sig1, 61, &roundbits);

    return roundbits;
#endif    
}

/*----------------------------------------------------------------------------
| Functions and definitions to determine:  (1) whether tininess for underflow
| is detected before or after rounding by default, (2) what (if anything)
| happens when exceptions are raised, (3) how signaling NaNs are distinguished
| from quiet NaNs, (4) the default generated quiet NaNs, and (5) how NaNs
| are propagated from function inputs to output.  These details are target-
| specific.
*----------------------------------------------------------------------------*/
#include "softfloat-specialize.h"

/*----------------------------------------------------------------------------
| Takes a 64-bit fixed-point value `absZ' with binary point between bits 6
| and 7, and returns the properly rounded 32-bit integer corresponding to the
| input.  If `zSign' is 1, the input is negated before being converted to an
| integer.  Bit 63 of `absZ' must be zero.  Ordinarily, the fixed-point input
| is simply rounded to an integer, with the inexact exception raised if the
| input cannot be represented exactly as an integer.  However, if the fixed-
| point input is too large, the invalid exception is raised and the largest
| positive or negative integer is returned.
*----------------------------------------------------------------------------*/

static int32_t roundAndPackInt32(flag zSign, uint64_t absZ, float_status *status)
{
    int8_t roundingMode;
    flag roundNearestEven;
    int8_t roundIncrement, roundBits;
    int32_t z;

    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    switch (roundingMode) {
    case float_round_nearest_even:
    case float_round_ties_away:
        roundIncrement = 0x40;
        break;
    case float_round_to_zero:
        roundIncrement = 0;
        break;
    case float_round_up:
        roundIncrement = zSign ? 0 : 0x7f;
        break;
    case float_round_down:
        roundIncrement = zSign ? 0x7f : 0;
        break;
    default:
        abort();
    }
    roundBits = absZ & 0x7F;
    absZ = ( absZ + roundIncrement )>>7;
    absZ &= ~ ( ( ( roundBits ^ 0x40 ) == 0 ) & roundNearestEven );
    z = absZ;
    if ( zSign ) z = - z;
    if ( ( absZ>>32 ) || ( z && ( ( z < 0 ) ^ zSign ) ) ) {
        float_raise(float_flag_invalid, status);
        return zSign ? (int32_t) 0x80000000 : 0x7FFFFFFF;
    }
    if (roundBits) {
        status->float_exception_flags |= float_flag_inexact;
    }
    return z;

}


#ifdef SOFTFLOAT_68K // 30-01-2017: Added for Previous
static int16_t roundAndPackInt16( flag zSign, uint64_t absZ, float_status *status )
{
    int8_t roundingMode;
    flag roundNearestEven;
    int8_t roundIncrement, roundBits;
    int16_t z;
    
    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    roundIncrement = 0x40;
    if ( ! roundNearestEven ) {
        if ( roundingMode == float_round_to_zero ) {
            roundIncrement = 0;
        }
        else {
            roundIncrement = 0x7F;
            if ( zSign ) {
                if ( roundingMode == float_round_up ) roundIncrement = 0;
            }
            else {
                if ( roundingMode == float_round_down ) roundIncrement = 0;
            }
        }
    }
    roundBits = absZ & 0x7F;
    absZ = ( absZ + roundIncrement )>>7;
    absZ &= ~ ( ( ( roundBits ^ 0x40 ) == 0 ) & roundNearestEven );
    z = absZ;
    if ( zSign ) z = - z;
    z = (int16_t) z;
    if ( ( absZ>>16 ) || ( z && ( ( z < 0 ) ^ zSign ) ) ) {
        float_raise( float_flag_invalid, status );
        return zSign ? (int16_t) 0x8000 : 0x7FFF;
    }
    if ( roundBits ) status->float_exception_flags |= float_flag_inexact;
    return z;
    
}

static int8_t roundAndPackInt8( flag zSign, uint64_t absZ, float_status *status )
{
    int8_t roundingMode;
    flag roundNearestEven;
    int8_t roundIncrement, roundBits;
    int8_t z;
    
    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    roundIncrement = 0x40;
    if ( ! roundNearestEven ) {
        if ( roundingMode == float_round_to_zero ) {
            roundIncrement = 0;
        }
        else {
            roundIncrement = 0x7F;
            if ( zSign ) {
                if ( roundingMode == float_round_up ) roundIncrement = 0;
            }
            else {
                if ( roundingMode == float_round_down ) roundIncrement = 0;
            }
        }
    }
    roundBits = absZ & 0x7F;
    absZ = ( absZ + roundIncrement )>>7;
    absZ &= ~ ( ( ( roundBits ^ 0x40 ) == 0 ) & roundNearestEven );
    z = absZ;
    if ( zSign ) z = - z;
    z = (int8_t) z;
    if ( ( absZ>>8 ) || ( z && ( ( z < 0 ) ^ zSign ) ) ) {
        float_raise( float_flag_invalid, status );
        return zSign ? (int8_t) 0x80 : 0x7F;
    }
    if ( roundBits ) status->float_exception_flags |= float_flag_inexact;
    return z;
    
}
#endif // End of addition for Previous

/*----------------------------------------------------------------------------
| Takes the 128-bit fixed-point value formed by concatenating `absZ0' and
| `absZ1', with binary point between bits 63 and 64 (between the input words),
| and returns the properly rounded 64-bit integer corresponding to the input.
| If `zSign' is 1, the input is negated before being converted to an integer.
| Ordinarily, the fixed-point input is simply rounded to an integer, with
| the inexact exception raised if the input cannot be represented exactly as
| an integer.  However, if the fixed-point input is too large, the invalid
| exception is raised and the largest positive or negative integer is
| returned.
*----------------------------------------------------------------------------*/

static int64_t roundAndPackInt64(flag zSign, uint64_t absZ0, uint64_t absZ1,
                               float_status *status)
{
    int8_t roundingMode;
    flag roundNearestEven, increment;
    int64_t z;

    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    switch (roundingMode) {
    case float_round_nearest_even:
    case float_round_ties_away:
        increment = ((int64_t) absZ1 < 0);
        break;
    case float_round_to_zero:
        increment = 0;
        break;
    case float_round_up:
        increment = !zSign && absZ1;
        break;
    case float_round_down:
        increment = zSign && absZ1;
        break;
    default:
        abort();
    }
    if ( increment ) {
        ++absZ0;
        if ( absZ0 == 0 ) goto overflow;
        absZ0 &= ~ ( ( (uint64_t) ( absZ1<<1 ) == 0 ) & roundNearestEven );
    }
    z = absZ0;
    if ( zSign ) z = - z;
    if ( z && ( ( z < 0 ) ^ zSign ) ) {
 overflow:
        float_raise(float_flag_invalid, status);
        return
              zSign ? (int64_t) LIT64( 0x8000000000000000 )
            : LIT64( 0x7FFFFFFFFFFFFFFF );
    }
    if (absZ1) {
        status->float_exception_flags |= float_flag_inexact;
    }
    return z;

}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static inline uint32_t extractFloat32Frac( float32 a )
{

    return float32_val(a) & 0x007FFFFF;

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static inline int extractFloat32Exp(float32 a)
{

    return ( float32_val(a)>>23 ) & 0xFF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static inline flag extractFloat32Sign( float32 a )
{

    return float32_val(a)>>31;

}

/*----------------------------------------------------------------------------
| Normalizes the subnormal single-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

static void
 normalizeFloat32Subnormal(uint32_t aSig, int *zExpPtr, uint32_t *zSigPtr)
{
    int8_t shiftCount;

    shiftCount = countLeadingZeros32( aSig ) - 8;
    *zSigPtr = aSig<<shiftCount;
    *zExpPtr = 1 - shiftCount;

}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| single-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

static inline float32 packFloat32(flag zSign, int zExp, uint32_t zSig)
{

    return make_float32(
          ( ( (uint32_t) zSign )<<31 ) + ( ( (uint32_t) zExp )<<23 ) + zSig);

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper single-precision floating-
| point value corresponding to the abstract input.  Ordinarily, the abstract
| value is simply rounded and packed into the single-precision format, with
| the inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal single-
| precision floating-point number.
|     The input significand `zSig' has its binary point between bits 30
| and 29, which is 7 bits to the left of the usual location.  This shifted
| significand must be normalized or smaller.  If `zSig' is not normalized,
| `zExp' must be 0; in that case, the result returned is a subnormal number,
| and it must not require rounding.  In the usual case that `zSig' is
| normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
| The handling of underflow and overflow follows the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static float32 roundAndPackFloat32(flag zSign, int zExp, uint32_t zSig,
                                   float_status *status)
{
    int8_t roundingMode;
    flag roundNearestEven;
    int8_t roundIncrement, roundBits;
    flag isTiny;

    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    switch (roundingMode) {
    case float_round_nearest_even:
    case float_round_ties_away:
        roundIncrement = 0x40;
        break;
    case float_round_to_zero:
        roundIncrement = 0;
        break;
    case float_round_up:
        roundIncrement = zSign ? 0 : 0x7f;
        break;
    case float_round_down:
        roundIncrement = zSign ? 0x7f : 0;
        break;
    default:
        abort();
        break;
    }
    roundBits = zSig & 0x7F;
    if ( 0xFD <= (uint16_t) zExp ) {
        if (    ( 0xFD < zExp )
             || (    ( zExp == 0xFD )
                  && ( (int32_t) ( zSig + roundIncrement ) < 0 ) )
           ) {
#ifdef SOFTFLOAT_68K
            float_raise( float_flag_overflow, status );
			saveFloat32Internal( zSign, zExp, zSig, status );
            if ( roundBits ) float_raise( float_flag_inexact, status );
#else
            float_raise(float_flag_overflow | float_flag_inexact, status);
#endif
            return packFloat32( zSign, 0xFF, - ( roundIncrement == 0 ));
        }
        if ( zExp < 0 ) {
            if (status->flush_to_zero) {
                //float_raise(float_flag_output_denormal, status);
                return packFloat32(zSign, 0, 0);
            }
            isTiny =
                (status->float_detect_tininess
                 == float_tininess_before_rounding)
                || ( zExp < -1 )
                || ( zSig + roundIncrement < 0x80000000 );
#ifdef SOFTFLOAT_68K
            if ( isTiny ) {
                float_raise( float_flag_underflow, status );
                saveFloat32Internal( zSign, zExp, zSig, status );
            }
#endif
            shift32RightJamming( zSig, - zExp, &zSig );
            zExp = 0;
            roundBits = zSig & 0x7F;
#ifndef SOFTFLOAT_68K
            if (isTiny && roundBits)
                float_raise(float_flag_underflow, status);
#endif
        }
    }
    if (roundBits) {
        status->float_exception_flags |= float_flag_inexact;
    }
    zSig = ( zSig + roundIncrement )>>7;
    zSig &= ~ ( ( ( roundBits ^ 0x40 ) == 0 ) & roundNearestEven );
    if ( zSig == 0 ) zExp = 0;
    return packFloat32( zSign, zExp, zSig );

}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static inline uint64_t extractFloat64Frac( float64 a )
{

    return float64_val(a) & LIT64( 0x000FFFFFFFFFFFFF );

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static inline int extractFloat64Exp(float64 a)
{

    return ( float64_val(a)>>52 ) & 0x7FF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static inline flag extractFloat64Sign( float64 a )
{

    return float64_val(a)>>63;

}

/*----------------------------------------------------------------------------
| If `a' is denormal and we are in flush-to-zero mode then set the
| input-denormal exception and return zero. Otherwise just return the value.
*----------------------------------------------------------------------------*/
float64 float64_squash_input_denormal(float64 a, float_status *status)
{
    if (status->flush_inputs_to_zero) {
        if (extractFloat64Exp(a) == 0 && extractFloat64Frac(a) != 0) {
            //float_raise(float_flag_input_denormal, status);
            return make_float64(float64_val(a) & (1ULL << 63));
        }
    }
    return a;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal double-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

static void
 normalizeFloat64Subnormal(uint64_t aSig, int *zExpPtr, uint64_t *zSigPtr)
{
    int8_t shiftCount;

    shiftCount = countLeadingZeros64( aSig ) - 11;
    *zSigPtr = aSig<<shiftCount;
    *zExpPtr = 1 - shiftCount;

}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| double-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

static inline float64 packFloat64(flag zSign, int zExp, uint64_t zSig)
{

    return make_float64(
        ( ( (uint64_t) zSign )<<63 ) + ( ( (uint64_t) zExp )<<52 ) + zSig);

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper double-precision floating-
| point value corresponding to the abstract input.  Ordinarily, the abstract
| value is simply rounded and packed into the double-precision format, with
| the inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal double-
| precision floating-point number.
|     The input significand `zSig' has its binary point between bits 62
| and 61, which is 10 bits to the left of the usual location.  This shifted
| significand must be normalized or smaller.  If `zSig' is not normalized,
| `zExp' must be 0; in that case, the result returned is a subnormal number,
| and it must not require rounding.  In the usual case that `zSig' is
| normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
| The handling of underflow and overflow follows the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static float64 roundAndPackFloat64(flag zSign, int zExp, uint64_t zSig,
                                   float_status *status)
{
    int8_t roundingMode;
    flag roundNearestEven;
    int roundIncrement, roundBits;
    flag isTiny;

    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    switch (roundingMode) {
    case float_round_nearest_even:
    case float_round_ties_away:
        roundIncrement = 0x200;
        break;
    case float_round_to_zero:
        roundIncrement = 0;
        break;
    case float_round_up:
        roundIncrement = zSign ? 0 : 0x3ff;
        break;
    case float_round_down:
        roundIncrement = zSign ? 0x3ff : 0;
        break;
    default:
        abort();
    }
    roundBits = zSig & 0x3FF;
    if ( 0x7FD <= (uint16_t) zExp ) {
        if (    ( 0x7FD < zExp )
             || (    ( zExp == 0x7FD )
                  && ( (int64_t) ( zSig + roundIncrement ) < 0 ) )
           ) {
#ifdef SOFTFLOAT_68K
			float_raise( float_flag_overflow, status );
			saveFloat64Internal( zSign, zExp, zSig, status );
            if ( roundBits ) float_raise( float_flag_inexact, status );
#else
            float_raise(float_flag_overflow | float_flag_inexact, status);
#endif
            return packFloat64( zSign, 0x7FF, - ( roundIncrement == 0 ));
        }
        if ( zExp < 0 ) {
            if (status->flush_to_zero) {
                //float_raise(float_flag_output_denormal, status);
                return packFloat64(zSign, 0, 0);
            }
            isTiny =
                   (status->float_detect_tininess
                    == float_tininess_before_rounding)
                || ( zExp < -1 )
                || ( zSig + roundIncrement < LIT64( 0x8000000000000000 ) );
#ifdef SOFTFLOAT_68K
            if ( isTiny ) {
                float_raise( float_flag_underflow, status );
                saveFloat64Internal( zSign, zExp, zSig, status );
            }
#endif
            shift64RightJamming( zSig, - zExp, &zSig );
            zExp = 0;
            roundBits = zSig & 0x3FF;
#ifndef SOFTFLOAT_68K
            if (isTiny && roundBits)
                float_raise(float_flag_underflow, status);
#endif
        }
    }
    if (roundBits) {
        status->float_exception_flags |= float_flag_inexact;
    }
    zSig = ( zSig + roundIncrement )>>10;
    zSig &= ~ ( ( ( roundBits ^ 0x200 ) == 0 ) & roundNearestEven );
    if ( zSig == 0 ) zExp = 0;
    return packFloat64( zSign, zExp, zSig );

}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

uint64_t extractFloatx80Frac( floatx80 a )
{

    return a.low;

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

int32_t extractFloatx80Exp( floatx80 a )
{

    return a.high & 0x7FFF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the extended double-precision floating-point value
| `a'.
*----------------------------------------------------------------------------*/

flag extractFloatx80Sign( floatx80 a )
{

    return a.high>>15;

}

/*----------------------------------------------------------------------------
| Normalizes the subnormal extended double-precision floating-point value
| represented by the denormalized significand `aSig'.  The normalized exponent
| and significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

void normalizeFloatx80Subnormal( uint64_t aSig, int32_t *zExpPtr, uint64_t *zSigPtr )
{
    int8_t shiftCount;

    shiftCount = countLeadingZeros64( aSig );
    *zSigPtr = aSig<<shiftCount;
#ifdef SOFTFLOAT_68K
	*zExpPtr = -shiftCount;
#else
	*zExpPtr = 1 - shiftCount;
#endif
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into an
| extended double-precision floating-point value, returning the result.
*----------------------------------------------------------------------------*/

floatx80 packFloatx80( flag zSign, int32_t zExp, uint64_t zSig )
{
    floatx80 z;

    z.low = zSig;
    z.high = ( ( (uint16_t) zSign )<<15 ) + zExp;
    return z;

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and extended significand formed by the concatenation of `zSig0' and `zSig1',
| and returns the proper extended double-precision floating-point value
| corresponding to the abstract input.  Ordinarily, the abstract value is
| rounded and packed into the extended double-precision format, with the
| inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal extended
| double-precision floating-point number.
|     If `roundingPrecision' is 32 or 64, the result is rounded to the same
| number of bits as single or double precision, respectively.  Otherwise, the
| result is rounded to the full precision of the extended double-precision
| format.
|     The input significand must be normalized or smaller.  If the input
| significand is not normalized, `zExp' must be 0; in that case, the result
| returned is a subnormal number, and it must not require rounding.  The
| handling of underflow and overflow follows the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#ifndef SOFTFLOAT_68K
floatx80 roundAndPackFloatx80(int8_t roundingPrecision, flag zSign,
                                     int32_t zExp, uint64_t zSig0, uint64_t zSig1,
                                     float_status *status)
{
    int8_t roundingMode;
    flag roundNearestEven, increment, isTiny;
    int64_t roundIncrement, roundMask, roundBits;

    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    if ( roundingPrecision == 80 ) goto precision80;
    if ( roundingPrecision == 64 ) {
        roundIncrement = LIT64( 0x0000000000000400 );
        roundMask = LIT64( 0x00000000000007FF );
    }
    else if ( roundingPrecision == 32 ) {
        roundIncrement = LIT64( 0x0000008000000000 );
        roundMask = LIT64( 0x000000FFFFFFFFFF );
    }
    else {
        goto precision80;
    }
    zSig0 |= ( zSig1 != 0 );
    switch (roundingMode) {
    case float_round_nearest_even:
    case float_round_ties_away:
        break;
    case float_round_to_zero:
        roundIncrement = 0;
        break;
    case float_round_up:
        roundIncrement = zSign ? 0 : roundMask;
        break;
    case float_round_down:
        roundIncrement = zSign ? roundMask : 0;
        break;
    default:
        abort();
    }
    roundBits = zSig0 & roundMask;
#ifdef SOFTFLOAT_68K
	if ( 0x7FFE <= (uint32_t) zExp ) {
#else
	if ( 0x7FFD <= (uint32_t) ( zExp - 1 ) ) {
#endif
        if (    ( 0x7FFE < zExp )
             || ( ( zExp == 0x7FFE ) && ( zSig0 + roundIncrement < zSig0 ) )
           ) {
            goto overflow;
        }
#ifdef SOFTFLOAT_68K
        if ( zExp < 0 ) {
#else
		if ( zExp <= 0 ) {
#endif
            if (status->flush_to_zero) {
                //float_raise(float_flag_output_denormal, status);
                return packFloatx80(zSign, 0, 0);
            }
            isTiny =
                   (status->float_detect_tininess
                    == float_tininess_before_rounding)
#ifdef SOFTFLOAT_68K
				|| ( zExp < -1 )
#else
				|| ( zExp < 0 )
#endif
				|| ( zSig0 <= zSig0 + roundIncrement );
#ifdef SOFTFLOAT_68K
			if ( isTiny ) {
				float_raise( float_flag_underflow, status );
				saveFloatx80Internal( zSign, zExp, zSig0, zSig1, status );
			}
			shift64RightJamming( zSig0, -zExp, &zSig0 );
#else
			shift64RightJamming( zSig0, 1 - zExp, &zSig0 );
#endif
            zExp = 0;
            roundBits = zSig0 & roundMask;
#ifdef SOFTFLOAT_68K
            if ( isTiny ) float_raise( float_flag_underflow, status );
#else
            if (isTiny && roundBits) {
                float_raise(float_flag_underflow, status);
            }
#endif
if (roundBits) {
                status->float_exception_flags |= float_flag_inexact;
            }
            zSig0 += roundIncrement;
#ifndef SOFTFLOAT_68K
			if ( (int64_t) zSig0 < 0 ) zExp = 1;
#endif
            roundIncrement = roundMask + 1;
            if ( roundNearestEven && ( roundBits<<1 == roundIncrement ) ) {
                roundMask |= roundIncrement;
            }
            zSig0 &= ~ roundMask;
            return packFloatx80( zSign, zExp, zSig0 );
        }
    }
    if (roundBits) {
        status->float_exception_flags |= float_flag_inexact;
    }
    zSig0 += roundIncrement;
    if ( zSig0 < roundIncrement ) {
        ++zExp;
        zSig0 = LIT64( 0x8000000000000000 );
    }
    roundIncrement = roundMask + 1;
    if ( roundNearestEven && ( roundBits<<1 == roundIncrement ) ) {
        roundMask |= roundIncrement;
    }
    zSig0 &= ~ roundMask;
    if ( zSig0 == 0 ) zExp = 0;
    return packFloatx80( zSign, zExp, zSig0 );
 precision80:
    switch (roundingMode) {
    case float_round_nearest_even:
    case float_round_ties_away:
        increment = ((int64_t)zSig1 < 0);
        break;
    case float_round_to_zero:
        increment = 0;
        break;
    case float_round_up:
        increment = !zSign && zSig1;
        break;
    case float_round_down:
        increment = zSign && zSig1;
        break;
    default:
        abort();
    }
#ifdef SOFTFLOAT_68K
	if ( 0x7FFE <= (uint32_t) zExp ) {
#else
	if ( 0x7FFD <= (uint32_t) ( zExp - 1 ) ) {
#endif
        if (    ( 0x7FFE < zExp )
             || (    ( zExp == 0x7FFE )
                  && ( zSig0 == LIT64( 0xFFFFFFFFFFFFFFFF ) )
                  && increment
                )
           ) {
            roundMask = 0;
 overflow:
#ifndef SOFTFLOAT_68K
            float_raise(float_flag_overflow | float_flag_inexact, status);
#else
            float_raise( float_flag_overflow, status );
			saveFloatx80Internal( zSign, zExp, zSig0, zSig1, status );
            if ( ( zSig0 & roundMask ) || zSig1 ) float_raise( float_flag_inexact, status );
#endif
			if (    ( roundingMode == float_round_to_zero )
                 || ( zSign && ( roundingMode == float_round_up ) )
                 || ( ! zSign && ( roundingMode == float_round_down ) )
               ) {
                return packFloatx80( zSign, 0x7FFE, ~ roundMask );
            }
            return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
        }
#ifdef SOFTFLOAT_68K
		if ( zExp < 0 ) {
#else
		if ( zExp <= 0 ) {
#endif
            isTiny =
                   (status->float_detect_tininess
                    == float_tininess_before_rounding)
#ifdef SOFTFLOAT_68K
				|| ( zExp < -1 )
#else
				|| ( zExp < 0 )
#endif
                || ! increment
                || ( zSig0 < LIT64( 0xFFFFFFFFFFFFFFFF ) );
#ifdef SOFTFLOAT_68K
			if ( isTiny ) {
				float_raise( float_flag_underflow, status );
				saveFloatx80Internal( zSign, zExp, zSig0, zSig1, status );
			}
			shift64ExtraRightJamming( zSig0, zSig1, -zExp, &zSig0, &zSig1 );
#else
			shift64ExtraRightJamming( zSig0, zSig1, 1 - zExp, &zSig0, &zSig1 );
#endif
            zExp = 0;
#ifndef SOFTFLOAT_68K
			if ( isTiny && zSig1 ) float_raise( float_flag_underflow, status );
#endif
            if (zSig1) float_raise(float_flag_inexact, status);
            switch (roundingMode) {
            case float_round_nearest_even:
            case float_round_ties_away:
                increment = ((int64_t)zSig1 < 0);
                break;
            case float_round_to_zero:
                increment = 0;
                break;
            case float_round_up:
                increment = !zSign && zSig1;
                break;
            case float_round_down:
                increment = zSign && zSig1;
                break;
            default:
                abort();
            }
            if ( increment ) {
                ++zSig0;
                zSig0 &=
                    ~ ( ( (uint64_t) ( zSig1<<1 ) == 0 ) & roundNearestEven );
#ifndef SOFTFLOAT_68K
				if ( (int64_t) zSig0 < 0 ) zExp = 1;
#endif
            }
            return packFloatx80( zSign, zExp, zSig0 );
        }
    }
    if (zSig1) {
        status->float_exception_flags |= float_flag_inexact;
    }
    if ( increment ) {
        ++zSig0;
        if ( zSig0 == 0 ) {
            ++zExp;
            zSig0 = LIT64( 0x8000000000000000 );
        }
        else {
            zSig0 &= ~ ( ( (uint64_t) ( zSig1<<1 ) == 0 ) & roundNearestEven );
        }
    }
    else {
        if ( zSig0 == 0 ) zExp = 0;
    }
    return packFloatx80( zSign, zExp, zSig0 );

}

#else // SOFTFLOAT_68K

floatx80 roundAndPackFloatx80( int8_t roundingPrecision, flag zSign, int32_t zExp, uint64_t zSig0, uint64_t zSig1, float_status *status )
{
    int8_t roundingMode;
    flag roundNearestEven, increment;
    int64_t roundIncrement, roundMask, roundBits;
    int32_t expOffset;
    
    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    if ( roundingPrecision == 80 ) goto precision80;
    if ( roundingPrecision == 64 ) {
        roundIncrement = LIT64( 0x0000000000000400 );
        roundMask = LIT64( 0x00000000000007FF );
        expOffset = 0x3C00;
    } else if ( roundingPrecision == 32 ) {
        roundIncrement = LIT64( 0x0000008000000000 );
        roundMask = LIT64( 0x000000FFFFFFFFFF );
        expOffset = 0x3F80;
    } else {
        goto precision80;
    }
    zSig0 |= ( zSig1 != 0 );
    if ( ! roundNearestEven ) {
        if ( roundingMode == float_round_to_zero ) {
            roundIncrement = 0;
        } else {
            roundIncrement = roundMask;
            if ( zSign ) {
                if ( roundingMode == float_round_up ) roundIncrement = 0;
            } else {
                if ( roundingMode == float_round_down ) roundIncrement = 0;
            }
        }
    }
    roundBits = zSig0 & roundMask;
    if ( ( ( 0x7FFE - expOffset ) < zExp ) ||
        ( ( zExp == ( 0x7FFE - expOffset ) ) && ( zSig0 + roundIncrement < zSig0 ) ) ) {
        float_raise( float_flag_overflow, status );
        saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status );
        if ( zSig0 & roundMask ) float_raise( float_flag_inexact, status );
        if (    ( roundingMode == float_round_to_zero )
            || ( zSign && ( roundingMode == float_round_up ) )
            || ( ! zSign && ( roundingMode == float_round_down ) )
            ) {
            return packFloatx80( zSign, 0x7FFE - expOffset, ~ roundMask );
        }
        return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
    }
    if ( zExp < ( expOffset + 1 ) ) {
        float_raise( float_flag_underflow, status );
        saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status );
        shift64RightJamming( zSig0, -( zExp - ( expOffset + 1 ) ), &zSig0 );
        zExp = expOffset + 1;
        roundBits = zSig0 & roundMask;
        if ( roundBits ) float_raise( float_flag_inexact, status );
        zSig0 += roundIncrement;
        roundIncrement = roundMask + 1;
        if ( roundNearestEven && ( roundBits<<1 == roundIncrement ) ) {
            roundMask |= roundIncrement;
        }
        zSig0 &= ~ roundMask;
        return packFloatx80( zSign, zExp, zSig0 );
    }
    if ( roundBits ) {
        float_raise( float_flag_inexact, status );
        saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status);
    }
    zSig0 += roundIncrement;
    if ( zSig0 < roundIncrement ) {
        ++zExp;
        zSig0 = LIT64( 0x8000000000000000 );
    }
    roundIncrement = roundMask + 1;
    if ( roundNearestEven && ( roundBits<<1 == roundIncrement ) ) {
        roundMask |= roundIncrement;
    }
    zSig0 &= ~ roundMask;
    if ( zSig0 == 0 ) zExp = 0;
    return packFloatx80( zSign, zExp, zSig0 );
precision80:
    increment = ( (int64_t) zSig1 < 0 );
    if ( ! roundNearestEven ) {
        if ( roundingMode == float_round_to_zero ) {
            increment = 0;
        } else {
            if ( zSign ) {
                increment = ( roundingMode == float_round_down ) && zSig1;
            } else {
                increment = ( roundingMode == float_round_up ) && zSig1;
            }
        }
    }
    if ( 0x7FFE <= (uint32_t) zExp ) {
        if ( ( 0x7FFE < zExp ) ||
            ( ( zExp == 0x7FFE ) && ( zSig0 == LIT64( 0xFFFFFFFFFFFFFFFF ) ) && increment )
            ) {
            roundMask = 0;
            float_raise( float_flag_overflow, status );
            saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status );
            if ( ( zSig0 & roundMask ) || zSig1 ) float_raise( float_flag_inexact, status );
            if (    ( roundingMode == float_round_to_zero )
                || ( zSign && ( roundingMode == float_round_up ) )
                || ( ! zSign && ( roundingMode == float_round_down ) )
                ) {
                return packFloatx80( zSign, 0x7FFE, ~ roundMask );
            }
            return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
        }
        if ( zExp < 0 ) {
            float_raise( float_flag_underflow, status );
            saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status);
            shift64ExtraRightJamming( zSig0, zSig1, -zExp, &zSig0, &zSig1 );
            zExp = 0;
            if ( zSig1 ) float_raise( float_flag_inexact, status );
            if ( roundNearestEven ) {
                increment = ( (int64_t) zSig1 < 0 );
            } else {
                if ( zSign ) {
                    increment = ( roundingMode == float_round_down ) && zSig1;
                } else {
                    increment = ( roundingMode == float_round_up ) && zSig1;
                }
            }
            if ( increment ) {
                ++zSig0;
                zSig0 &=
                ~ ( ( (uint64_t) ( zSig1<<1 ) == 0 ) & roundNearestEven );
            }
            return packFloatx80( zSign, zExp, zSig0 );
        }
    }
    if ( zSig1 ) {
        float_raise( float_flag_inexact, status );
        saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status );
    }
    if ( increment ) {
        ++zSig0;
        if ( zSig0 == 0 ) {
            ++zExp;
            zSig0 = LIT64( 0x8000000000000000 );
        } else {
            zSig0 &= ~ ( ( (uint64_t) ( zSig1<<1 ) == 0 ) & roundNearestEven );
        }
    } else {
        if ( zSig0 == 0 ) zExp = 0;
    }
    return packFloatx80( zSign, zExp, zSig0 );
    
}

#endif

#ifdef SOFTFLOAT_68K // 21-01-2017: Added for Previous
floatx80 roundSigAndPackFloatx80( int8_t roundingPrecision, flag zSign, int32_t zExp, uint64_t zSig0, uint64_t zSig1, float_status *status )
{
    int8_t roundingMode;
    flag roundNearestEven, isTiny;
    int64_t roundIncrement, roundMask, roundBits;
    
    roundingMode = status->float_rounding_mode;
    roundNearestEven = ( roundingMode == float_round_nearest_even );
    if ( roundingPrecision == 32 ) {
        roundIncrement = LIT64( 0x0000008000000000 );
        roundMask = LIT64( 0x000000FFFFFFFFFF );
    } else if ( roundingPrecision == 64 ) {
        roundIncrement = LIT64( 0x0000000000000400 );
        roundMask = LIT64( 0x00000000000007FF );
    } else {
        return roundAndPackFloatx80( 80, zSign, zExp, zSig0, zSig1, status );
    }
    zSig0 |= ( zSig1 != 0 );
    if ( ! roundNearestEven ) {
        if ( roundingMode == float_round_to_zero ) {
            roundIncrement = 0;
        }
        else {
            roundIncrement = roundMask;
            if ( zSign ) {
                if ( roundingMode == float_round_up ) roundIncrement = 0;
            }
            else {
                if ( roundingMode == float_round_down ) roundIncrement = 0;
            }
        }
    }
    roundBits = zSig0 & roundMask;
    
    if ( 0x7FFE <= (uint32_t) zExp ) {
        if (    ( 0x7FFE < zExp )
            || ( ( zExp == 0x7FFE ) && ( zSig0 + roundIncrement < zSig0 ) )
            ) {
            float_raise( float_flag_overflow, status );
			saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status);
			if ( zSig0 & roundMask ) float_raise( float_flag_inexact, status );
            if (    ( roundingMode == float_round_to_zero )
                || ( zSign && ( roundingMode == float_round_up ) )
                || ( ! zSign && ( roundingMode == float_round_down ) )
                ) {
                return packFloatx80( zSign, 0x7FFE, LIT64( 0xFFFFFFFFFFFFFFFF ) );
            }
            return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
        }
        
        if ( zExp < 0 ) {
            isTiny =
            ( status->float_detect_tininess == float_tininess_before_rounding )
            || ( zExp < -1 )
            || ( zSig0 <= zSig0 + roundIncrement );
			if ( isTiny ) {
				float_raise( float_flag_underflow, status );
				saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status );
			}
            shift64RightJamming( zSig0, -zExp, &zSig0 );
            zExp = 0;
            roundBits = zSig0 & roundMask;
            if ( roundBits ) float_raise ( float_flag_inexact, status );
            zSig0 += roundIncrement;
            if ( roundNearestEven && ( roundBits == roundIncrement ) ) {
                roundMask |= roundIncrement<<1;
            }
            zSig0 &= ~roundMask;
            return packFloatx80( zSign, zExp, zSig0 );
        }
    }
    if ( roundBits ) {
        float_raise( float_flag_inexact, status );
        saveFloatx80Internal( roundingPrecision, zSign, zExp, zSig0, zSig1, status );
    }
    zSig0 += roundIncrement;
    if ( zSig0 < roundIncrement ) {
        ++zExp;
        zSig0 = LIT64( 0x8000000000000000 );
    }
    roundIncrement = roundMask + 1;
    if ( roundNearestEven && ( roundBits<<1 == roundIncrement ) ) {
        roundMask |= roundIncrement;
    }
    zSig0 &= ~ roundMask;
    if ( zSig0 == 0 ) zExp = 0;
    return packFloatx80( zSign, zExp, zSig0 );
    
}
#endif // End of Addition for Previous


/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent
| `zExp', and significand formed by the concatenation of `zSig0' and `zSig1',
| and returns the proper extended double-precision floating-point value
| corresponding to the abstract input.  This routine is just like
| `roundAndPackFloatx80' except that the input significand does not have to be
| normalized.
*----------------------------------------------------------------------------*/

static floatx80 normalizeRoundAndPackFloatx80(int8_t roundingPrecision,
                                              flag zSign, int32_t zExp,
                                              uint64_t zSig0, uint64_t zSig1,
                                              float_status *status)
{
    int8_t shiftCount;

    if ( zSig0 == 0 ) {
        zSig0 = zSig1;
        zSig1 = 0;
        zExp -= 64;
    }
    shiftCount = countLeadingZeros64( zSig0 );
    shortShift128Left( zSig0, zSig1, shiftCount, &zSig0, &zSig1 );
    zExp -= shiftCount;
    return roundAndPackFloatx80(roundingPrecision, zSign, zExp,
                                zSig0, zSig1, status);

}

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 int32_to_floatx80(int32_t a)
{
    flag zSign;
    uint32_t absA;
    int8_t shiftCount;
    uint64_t zSig;

    if ( a == 0 ) return packFloatx80( 0, 0, 0 );
    zSign = ( a < 0 );
    absA = zSign ? - a : a;
    shiftCount = countLeadingZeros32( absA ) + 32;
    zSig = absA;
    return packFloatx80( zSign, 0x403E - shiftCount, zSig<<shiftCount );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 float32_to_floatx80(float32 a, float_status *status)
{
    flag aSign;
    int aExp;
    uint32_t aSig;

    aSig = extractFloat32Frac( a );
    aExp = extractFloat32Exp( a );
    aSign = extractFloat32Sign( a );
    if ( aExp == 0xFF ) {
		if ( aSig ) return commonNaNToFloatx80( float32ToCommonNaN( a, status ), status );
		return packFloatx80( aSign, 0x7FFF, floatx80_default_infinity_low );
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
        normalizeFloat32Subnormal( aSig, &aExp, &aSig );
    }
    aSig |= 0x00800000;
    return packFloatx80( aSign, aExp + 0x3F80, ( (uint64_t) aSig )<<40 );

}

#ifdef SOFTFLOAT_68K // 31-12-2016: Added for Previous
floatx80 float32_to_floatx80_allowunnormal(float32 a , float_status *status)
{
    flag aSign;
    int16_t aExp;
    uint32_t aSig;
    
    aSig = extractFloat32Frac(a);
    aExp = extractFloat32Exp(a);
    aSign = extractFloat32Sign(a);
    if (aExp == 0xFF) {
        return packFloatx80( aSign, 0x7FFF, ( (uint64_t) aSig )<<40 );
    }
    if (aExp == 0) {
        if (aSig == 0) return packFloatx80(aSign, 0, 0);
        return packFloatx80(aSign, 0x3F81, ((uint64_t) aSig) << 40);
    }
    aSig |= 0x00800000;
    return packFloatx80(aSign, aExp + 0x3F80, ((uint64_t)aSig) << 40);
    
}
#endif // end of addition for Previous

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 float64_to_floatx80(float64 a, float_status *status)
{
    flag aSign;
    int aExp;
    uint64_t aSig;

    aSig = extractFloat64Frac( a );
    aExp = extractFloat64Exp( a );
    aSign = extractFloat64Sign( a );
    if ( aExp == 0x7FF ) {
        if ( aSig ) return commonNaNToFloatx80( float64ToCommonNaN( a, status ), status );
        return packFloatx80( aSign, 0x7FFF, floatx80_default_infinity_low );
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
        normalizeFloat64Subnormal( aSig, &aExp, &aSig );
    }
    return
        packFloatx80(
            aSign, aExp + 0x3C00, ( aSig | LIT64( 0x0010000000000000 ) )<<11 );

}

#ifdef SOFTFLOAT_68K // 31-12-2016: Added for Previous
floatx80 float64_to_floatx80_allowunnormal( float64 a, float_status *status )
{
    flag aSign;
    int16_t aExp;
    uint64_t aSig;
    
    aSig = extractFloat64Frac( a );
    aExp = extractFloat64Exp( a );
    aSign = extractFloat64Sign( a );
    if ( aExp == 0x7FF ) {
        return packFloatx80( aSign, 0x7FFF, aSig<<11 );
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
        return packFloatx80( aSign, 0x3C01, aSig<<11 );
    }
    return
    packFloatx80(
                 aSign, aExp + 0x3C00, ( aSig | LIT64( 0x0010000000000000 ) )<<11 );
    
}
#endif // end of addition for Previous

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 32-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic---which means in particular that the conversion
| is rounded according to the current rounding mode.  If `a' is a NaN, the
| largest positive integer is returned.  Otherwise, if the conversion
| overflows, the largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

int32_t floatx80_to_int32(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp, shiftCount;
    uint64_t aSig;

    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
#ifdef SOFTFLOAT_68K
    if ( aExp == 0x7FFF ) {
		if ( (uint64_t) ( aSig<<1 ) ) {
            a = propagateFloatx80NaNOneArg( a, status );
            if ( a.low == aSig ) float_raise( float_flag_invalid, status );
            return (int32_t)(a.low>>32);
        }
		float_raise( float_flag_invalid, status );
        return aSign ? (int32_t) 0x80000000 : 0x7FFFFFFF;
    }
#else
 	if ( ( aExp == 0x7FFF ) && (bits64) ( aSig<<1 ) ) aSign = 0;
#endif
    shiftCount = 0x4037 - aExp;
    if ( shiftCount <= 0 ) shiftCount = 1;
    shift64RightJamming( aSig, shiftCount, &aSig );
    return roundAndPackInt32(aSign, aSig, status);

}

#ifdef SOFTFLOAT_68K // 30-01-2017: Addition for Previous
int16_t floatx80_to_int16( floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp, shiftCount;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    if ( aExp == 0x7FFF ) {
        float_raise( float_flag_invalid, status );
        if ( (uint64_t) ( aSig<<1 ) ) {
            a = propagateFloatx80NaNOneArg( a, status );
            if ( a.low == aSig ) float_raise( float_flag_invalid, status );
            return (int16_t)(a.low>>48);
        }
        return aSign ? (int16_t) 0x8000 : 0x7FFF;
    }
    shiftCount = 0x4037 - aExp;
    if ( shiftCount <= 0 ) shiftCount = 1;
    shift64RightJamming( aSig, shiftCount, &aSig );
    return roundAndPackInt16( aSign, aSig, status );
    
}
int8_t floatx80_to_int8( floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp, shiftCount;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    if ( aExp == 0x7FFF ) {
         if ( (uint64_t) ( aSig<<1 ) ) {
            a = propagateFloatx80NaNOneArg( a, status );
            if ( a.low == aSig ) float_raise( float_flag_invalid, status );
            return (int8_t)(a.low>>56);
        }
        float_raise( float_flag_invalid, status );
        return aSign ? (int8_t) 0x80 : 0x7F;
    }
    shiftCount = 0x4037 - aExp;
    if ( shiftCount <= 0 ) shiftCount = 1;
    shift64RightJamming( aSig, shiftCount, &aSig );
    return roundAndPackInt8( aSign, aSig, status );
    
}
#endif // End of addition for Previous


/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 32-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic, except that the conversion is always rounded
| toward zero.  If `a' is a NaN, the largest positive integer is returned.
| Otherwise, if the conversion overflows, the largest integer with the same
| sign as `a' is returned.
*----------------------------------------------------------------------------*/

int32_t floatx80_to_int32_round_to_zero(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp, shiftCount;
    uint64_t aSig, savedASig;
    int32_t z;

    if (floatx80_invalid_encoding(a)) {
        float_raise(float_flag_invalid, status);
        return 1 << 31;
    }
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    if ( 0x401E < aExp ) {
        if ( ( aExp == 0x7FFF ) && (uint64_t) ( aSig<<1 ) ) aSign = 0;
        goto invalid;
    }
    else if ( aExp < 0x3FFF ) {
        if (aExp || aSig) {
            status->float_exception_flags |= float_flag_inexact;
        }
        return 0;
    }
    shiftCount = 0x403E - aExp;
    savedASig = aSig;
    aSig >>= shiftCount;
    z = aSig;
    if ( aSign ) z = - z;
    if ( ( z < 0 ) ^ aSign ) {
 invalid:
        float_raise(float_flag_invalid, status);
        return aSign ? (int32_t) 0x80000000 : 0x7FFFFFFF;
    }
    if ( ( aSig<<shiftCount ) != savedASig ) {
        status->float_exception_flags |= float_flag_inexact;
    }
    return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 64-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic---which means in particular that the conversion
| is rounded according to the current rounding mode.  If `a' is a NaN,
| the largest positive integer is returned.  Otherwise, if the conversion
| overflows, the largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

int64_t floatx80_to_int64(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp, shiftCount;
    uint64_t aSig, aSigExtra;

    if (floatx80_invalid_encoding(a)) {
        float_raise(float_flag_invalid, status);
        return 1ULL << 63;
    }
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    shiftCount = 0x403E - aExp;
    if ( shiftCount <= 0 ) {
        if ( shiftCount ) {
            float_raise(float_flag_invalid, status);
            if (    ! aSign
                 || (    ( aExp == 0x7FFF )
                      && ( aSig != LIT64( 0x8000000000000000 ) ) )
               ) {
                return LIT64( 0x7FFFFFFFFFFFFFFF );
            }
            return (int64_t) LIT64( 0x8000000000000000 );
        }
        aSigExtra = 0;
    }
    else {
        shift64ExtraRightJamming( aSig, 0, shiftCount, &aSig, &aSigExtra );
    }
    return roundAndPackInt64(aSign, aSig, aSigExtra, status);

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the single-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float32 floatx80_to_float32(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;

    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) {
            return commonNaNToFloat32(floatx80ToCommonNaN(a, status));
        }
        return packFloat32( aSign, 0xFF, 0 );
    }
#ifdef SOFTFLOAT_68K
    if ( aExp == 0 ) {
        if ( aSig == 0) return packFloat32( aSign, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    shift64RightJamming( aSig, 33, &aSig );
    aExp -= 0x3F81;
#else
    shift64RightJamming( aSig, 33, &aSig );
    if ( aExp || aSig ) aExp -= 0x3F81;
#endif
    return roundAndPackFloat32(aSign, aExp, aSig, status);

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the double-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float64 floatx80_to_float64(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig, zSig;

    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) {
            return commonNaNToFloat64(floatx80ToCommonNaN(a, status), status);
        }
        return packFloat64( aSign, 0x7FF, 0 );
    }
#ifdef SOFTFLOAT_68K
    if ( aExp == 0 ) {
        if ( aSig == 0) return packFloat64( aSign, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    shift64RightJamming( aSig, 1, &zSig );
    aExp -= 0x3C01;
#else
    shift64RightJamming( aSig, 1, &zSig );
    if ( aExp || aSig ) aExp -= 0x3C01;
#endif
    return roundAndPackFloat64(aSign, aExp, zSig, status);

}

#ifdef SOFTFLOAT_68K // 31-01-2017
/*----------------------------------------------------------------------------
 | Returns the result of converting the extended double-precision floating-
 | point value `a' to the extended double-precision floating-point format.
 | The conversion is performed according to the IEC/IEEE Standard for Binary
 | Floating-Point Arithmetic.
 *----------------------------------------------------------------------------*/
        
floatx80 floatx80_to_floatx80( floatx80 a, float_status *status )
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    
    if ( aExp == 0x7FFF && (uint64_t) ( aSig<<1 ) ) {
        return propagateFloatx80NaNOneArg( a, status );
    }
    if ( aExp == 0 && aSig != 0 ) {
        return normalizeRoundAndPackFloatx80( status->floatx80_rounding_precision, aSign, aExp, aSig, 0, status );
    }
    return a;
    
}
#endif

#ifdef SOFTFLOAT_68K // 30-01-2016: Added for Previous
floatx80 floatx80_round32( floatx80 a, float_status *status )
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaNOneArg( a, status );
        return a;
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return a;
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    
    return roundSigAndPackFloatx80( 32, aSign, aExp, aSig, 0, status );
}

floatx80 floatx80_round64( floatx80 a, float_status *status )
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaNOneArg( a, status );
        return a;
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return a;
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    
    return roundSigAndPackFloatx80( 64, aSign, aExp, aSig, 0, status );
}

floatx80 floatx80_round_to_float32( floatx80 a, float_status *status )
{
	flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
	aSign = extractFloatx80Sign( a );
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaNOneArg( a, status );
        return a;
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return a;
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    
    return roundAndPackFloatx80( 32, aSign, aExp, aSig, 0, status );
}
        
floatx80 floatx80_round_to_float64( floatx80 a, float_status *status )
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
	aSign = extractFloatx80Sign( a );
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );

    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaNOneArg( a, status );
        return a;
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return a;
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }

    return roundAndPackFloatx80( 64, aSign, aExp, aSig, 0, status );
}
      

floatx80 floatx80_normalize( floatx80 a )
{
    flag aSign;
    int16_t aExp;
    uint64_t aSig;
    int8_t shiftCount;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    
    if ( aExp == 0x7FFF || aExp == 0 ) return a;
    if ( aSig == 0 ) return packFloatx80(aSign, 0, 0);
    
    shiftCount = countLeadingZeros64( aSig );
    
    if ( shiftCount > aExp ) shiftCount = aExp;
    
    aExp -= shiftCount;
    aSig <<= shiftCount;
    
    return packFloatx80( aSign, aExp, aSig );
}
#endif // end of addition for Previous

/*----------------------------------------------------------------------------
| Rounds the extended double-precision floating-point value `a' to an integer,
| and returns the result as an extended quadruple-precision floating-point
| value.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 floatx80_round_to_int(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp;
    uint64_t lastBitMask, roundBitsMask;
    floatx80 z;

    if (floatx80_invalid_encoding(a)) {
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    aExp = extractFloatx80Exp( a );
    if ( 0x403E <= aExp ) {
        if ( ( aExp == 0x7FFF ) && (uint64_t) ( extractFloatx80Frac( a )<<1 ) ) {
            return propagateFloatx80NaNOneArg(a, status);
        }
        return a;
    }
    if ( aExp < 0x3FFF ) {
        if (    ( aExp == 0 )
 #ifdef SOFTFLOAT_68K
				&& ( (uint64_t) extractFloatx80Frac( a ) == 0 ) ) {
#else
				&& ( (uint64_t) ( extractFloatx80Frac( a )<<1 ) == 0 ) ) {
#endif
           return a;
        }
        status->float_exception_flags |= float_flag_inexact;
        aSign = extractFloatx80Sign( a );
        switch (status->float_rounding_mode) {
         case float_round_nearest_even:
            if ( ( aExp == 0x3FFE ) && (uint64_t) ( extractFloatx80Frac( a )<<1 )
               ) {
                return
                    packFloatx80( aSign, 0x3FFF, LIT64( 0x8000000000000000 ) );
            }
            break;
        case float_round_ties_away:
            if (aExp == 0x3FFE) {
                return packFloatx80(aSign, 0x3FFF, LIT64(0x8000000000000000));
            }
            break;
         case float_round_down:
            return
                  aSign ?
                      packFloatx80( 1, 0x3FFF, LIT64( 0x8000000000000000 ) )
                : packFloatx80( 0, 0, 0 );
         case float_round_up:
            return
                  aSign ? packFloatx80( 1, 0, 0 )
                : packFloatx80( 0, 0x3FFF, LIT64( 0x8000000000000000 ) );
        }
        return packFloatx80( aSign, 0, 0 );
    }
    lastBitMask = 1;
    lastBitMask <<= 0x403E - aExp;
    roundBitsMask = lastBitMask - 1;
    z = a;
    switch (status->float_rounding_mode) {
    case float_round_nearest_even:
        z.low += lastBitMask>>1;
        if ((z.low & roundBitsMask) == 0) {
            z.low &= ~lastBitMask;
        }
        break;
    case float_round_ties_away:
        z.low += lastBitMask >> 1;
        break;
    case float_round_to_zero:
        break;
    case float_round_up:
        if (!extractFloatx80Sign(z)) {
            z.low += roundBitsMask;
        }
        break;
    case float_round_down:
        if (extractFloatx80Sign(z)) {
            z.low += roundBitsMask;
        }
        break;
    default:
        abort();
    }
    z.low &= ~ roundBitsMask;
    if ( z.low == 0 ) {
        ++z.high;
        z.low = LIT64( 0x8000000000000000 );
    }
    if (z.low != a.low) {
        status->float_exception_flags |= float_flag_inexact;
    }
    return z;

}

#ifdef SOFTFLOAT_68K // 09-01-2017: Added for Previous
floatx80 floatx80_round_to_int_toward_zero( floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp;
    uint64_t lastBitMask, roundBitsMask;
    floatx80 z;
    
    aExp = extractFloatx80Exp( a );
    if ( 0x403E <= aExp ) {
        if ( ( aExp == 0x7FFF ) && (uint64_t) ( extractFloatx80Frac( a )<<1 ) ) {
            return propagateFloatx80NaNOneArg( a, status );
        }
        return a;
    }
    if ( aExp < 0x3FFF ) {
        if (    ( aExp == 0 )
#ifdef SOFTFLOAT_68K
            && ( (uint64_t) extractFloatx80Frac( a ) == 0 ) ) {
#else
            && ( (uint64_t) ( extractFloatx80Frac( a )<<1 ) == 0 ) ) {
#endif
            return a;
        }
        status->float_exception_flags |= float_flag_inexact;
        aSign = extractFloatx80Sign( a );
        return packFloatx80( aSign, 0, 0 );
    }
    lastBitMask = 1;
    lastBitMask <<= 0x403E - aExp;
    roundBitsMask = lastBitMask - 1;
    z = a;
    z.low &= ~ roundBitsMask;
    if ( z.low == 0 ) {
        ++z.high;
        z.low = LIT64( 0x8000000000000000 );
    }
    if ( z.low != a.low ) status->float_exception_flags |= float_flag_inexact;
    return z;
    
}
#endif // End of addition for Previous

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the extended double-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the sum is
| negated before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static floatx80 addFloatx80Sigs(floatx80 a, floatx80 b, flag zSign,
                                float_status *status)
{
    int32_t aExp, bExp, zExp;
    uint64_t aSig, bSig, zSig0, zSig1;
    int32_t expDiff;

    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
#ifdef SOFTFLOAT_68K
	if ( aExp == 0 ) {
		normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
	}
	if ( bExp == 0 ) {
		normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
	}
#endif
    expDiff = aExp - bExp;
    if ( 0 < expDiff ) {
        if ( aExp == 0x7FFF ) {
            if ((uint64_t)(aSig << 1)) {
                return propagateFloatx80NaN(a, b, status);
            }
            return a;
        }
#ifndef SOFTFLOAT_68K
		if ( bExp == 0 ) --expDiff;
#endif
        shift64ExtraRightJamming( bSig, 0, expDiff, &bSig, &zSig1 );
        zExp = aExp;
    }
    else if ( expDiff < 0 ) {
        if ( bExp == 0x7FFF ) {
            if ((uint64_t)(bSig << 1)) {
                return propagateFloatx80NaN(a, b, status);
            }
            return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
        }
#ifndef SOFTFLOAT_68K
		if ( aExp == 0 ) ++expDiff;
#endif
        shift64ExtraRightJamming( aSig, 0, - expDiff, &aSig, &zSig1 );
        zExp = bExp;
    }
    else {
        if ( aExp == 0x7FFF ) {
            if ( (uint64_t) ( ( aSig | bSig )<<1 ) ) {
                return propagateFloatx80NaN(a, b, status);
            }
            return a;
        }
        zSig1 = 0;
        zSig0 = aSig + bSig;
 #ifndef SOFTFLOAT_68K
		if ( aExp == 0 ) {
			normalizeFloatx80Subnormal( zSig0, &zExp, &zSig0 );
			goto roundAndPack;
		}
#endif
        zExp = aExp;
#ifdef SOFTFLOAT_68K
		if ( aSig == 0 && bSig == 0 ) return packFloatx80( zSign, 0, 0 );
        if ( aSig == 0 || bSig == 0 ) goto roundAndPack;
#endif
		goto shiftRight1;
    }
    zSig0 = aSig + bSig;
    if ( (int64_t) zSig0 < 0 ) goto roundAndPack;
 shiftRight1:
    shift64ExtraRightJamming( zSig0, zSig1, 1, &zSig0, &zSig1 );
    zSig0 |= LIT64( 0x8000000000000000 );
    ++zExp;
 roundAndPack:
    return roundAndPackFloatx80(status->floatx80_rounding_precision,
                                zSign, zExp, zSig0, zSig1, status);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the extended
| double-precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static floatx80 subFloatx80Sigs(floatx80 a, floatx80 b, flag zSign,
                                float_status *status)
{
    int32_t aExp, bExp, zExp;
    uint64_t aSig, bSig, zSig0, zSig1;
    int32_t expDiff;

    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
    expDiff = aExp - bExp;
    if ( 0 < expDiff ) goto aExpBigger;
    if ( expDiff < 0 ) goto bExpBigger;
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( ( aSig | bSig )<<1 ) ) {
            return propagateFloatx80NaN(a, b, status);
        }
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
 #ifndef SOFTFLOAT_68K
	if ( aExp == 0 ) {
		aExp = 1;
		bExp = 1;
	}
#endif
    zSig1 = 0;
    if ( bSig < aSig ) goto aBigger;
    if ( aSig < bSig ) goto bBigger;
    return packFloatx80(status->float_rounding_mode == float_round_down, 0, 0);
 bExpBigger:
    if ( bExp == 0x7FFF ) {
        if ((uint64_t)(bSig << 1)) {
            return propagateFloatx80NaN(a, b, status);
        }
        return packFloatx80( zSign ^ 1, 0x7FFF, LIT64( 0x8000000000000000 ) );
    }
#ifndef SOFTFLOAT_68K
	if ( aExp == 0 ) ++expDiff;
#endif
    shift128RightJamming( aSig, 0, - expDiff, &aSig, &zSig1 );
 bBigger:
    sub128( bSig, 0, aSig, zSig1, &zSig0, &zSig1 );
    zExp = bExp;
    zSign ^= 1;
    goto normalizeRoundAndPack;
 aExpBigger:
    if ( aExp == 0x7FFF ) {
        if ((uint64_t)(aSig << 1)) {
            return propagateFloatx80NaN(a, b, status);
        }
        return a;
    }
#ifndef SOFTFLOAT_68K
	if ( bExp == 0 ) --expDiff;
#endif
    shift128RightJamming( bSig, 0, expDiff, &bSig, &zSig1 );
 aBigger:
    sub128( aSig, 0, bSig, zSig1, &zSig0, &zSig1 );
    zExp = aExp;
 normalizeRoundAndPack:
    return normalizeRoundAndPackFloatx80(status->floatx80_rounding_precision,
                                         zSign, zExp, zSig0, zSig1, status);
}

/*----------------------------------------------------------------------------
| Returns the result of adding the extended double-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 floatx80_add(floatx80 a, floatx80 b, float_status *status)
{
    flag aSign, bSign;

    if (floatx80_invalid_encoding(a) || floatx80_invalid_encoding(b)) {
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    aSign = extractFloatx80Sign( a );
    bSign = extractFloatx80Sign( b );
    if ( aSign == bSign ) {
        return addFloatx80Sigs(a, b, aSign, status);
    }
    else {
        return subFloatx80Sigs(a, b, aSign, status);
    }

}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the extended double-precision floating-
| point values `a' and `b'.  The operation is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 floatx80_sub(floatx80 a, floatx80 b, float_status *status)
{
    flag aSign, bSign;

    if (floatx80_invalid_encoding(a) || floatx80_invalid_encoding(b)) {
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    aSign = extractFloatx80Sign( a );
    bSign = extractFloatx80Sign( b );
    if ( aSign == bSign ) {
        return subFloatx80Sigs(a, b, aSign, status);
    }
    else {
        return addFloatx80Sigs(a, b, aSign, status);
    }

}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the extended double-precision floating-
| point values `a' and `b'.  The operation is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 floatx80_mul(floatx80 a, floatx80 b, float_status *status)
{
    flag aSign, bSign, zSign;
    int32_t aExp, bExp, zExp;
    uint64_t aSig, bSig, zSig0, zSig1;

    if (floatx80_invalid_encoding(a) || floatx80_invalid_encoding(b)) {
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
    bSign = extractFloatx80Sign( b );
    zSign = aSign ^ bSign;
    if ( aExp == 0x7FFF ) {
        if (    (uint64_t) ( aSig<<1 )
             || ( ( bExp == 0x7FFF ) && (uint64_t) ( bSig<<1 ) ) ) {
            return propagateFloatx80NaN(a, b, status);
        }
        if ( ( bExp | bSig ) == 0 ) goto invalid;
        return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
    }
    if ( bExp == 0x7FFF ) {
        if ((uint64_t)(bSig << 1)) {
            return propagateFloatx80NaN(a, b, status);
        }
        if ( ( aExp | aSig ) == 0 ) {
 invalid:
            float_raise(float_flag_invalid, status);
            return floatx80_default_nan(status);
        }
        return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( zSign, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    if ( bExp == 0 ) {
        if ( bSig == 0 ) return packFloatx80( zSign, 0, 0 );
        normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
    }
    zExp = aExp + bExp - 0x3FFE;
    mul64To128( aSig, bSig, &zSig0, &zSig1 );
    if ( 0 < (int64_t) zSig0 ) {
        shortShift128Left( zSig0, zSig1, 1, &zSig0, &zSig1 );
        --zExp;
    }
    return roundAndPackFloatx80(status->floatx80_rounding_precision,
                                zSign, zExp, zSig0, zSig1, status);
}

#ifdef SOFTFLOAT_68K // 21-01-2017: Added for Previous
floatx80 floatx80_sglmul( floatx80 a, floatx80 b, float_status *status )
{
	flag aSign, bSign, zSign;
	int32_t aExp, bExp, zExp;
	uint64_t aSig, bSig, zSig0, zSig1;
	floatx80 z;
	
	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	bSig = extractFloatx80Frac( b );
	bExp = extractFloatx80Exp( b );
	bSign = extractFloatx80Sign( b );
	zSign = aSign ^ bSign;
	if ( aExp == 0x7FFF ) {
		if (    (uint64_t) ( aSig<<1 )
			|| ( ( bExp == 0x7FFF ) && (uint64_t) ( bSig<<1 ) ) ) {
			return propagateFloatx80NaN( a, b, status );
		}
		if ( ( bExp | bSig ) == 0 ) goto invalid;
		return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
	}
	if ( bExp == 0x7FFF ) {
		if ( (uint64_t) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b, status );
		if ( ( aExp | aSig ) == 0 ) {
		invalid:
			float_raise( float_flag_invalid, status );
			z.low = floatx80_default_nan_low;
			z.high = floatx80_default_nan_high;
			return z;
		}
		return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
	}
	if ( aExp == 0 ) {
		if ( aSig == 0 ) return packFloatx80( zSign, 0, 0 );
		normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
	}
	if ( bExp == 0 ) {
		if ( bSig == 0 ) return packFloatx80( zSign, 0, 0 );
		normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
	}
	aSig &= LIT64( 0xFFFFFF0000000000 );
	bSig &= LIT64( 0xFFFFFF0000000000 );
	zExp = aExp + bExp - 0x3FFE;
	mul64To128( aSig, bSig, &zSig0, &zSig1 );
	if ( 0 < (int64_t) zSig0 ) {
		shortShift128Left( zSig0, zSig1, 1, &zSig0, &zSig1 );
		--zExp;
	}
	return roundSigAndPackFloatx80( 32, zSign, zExp, zSig0, zSig1, status);
        
}
#endif // End of addition for Previous
 

/*----------------------------------------------------------------------------
| Returns the result of dividing the extended double-precision floating-point
| value `a' by the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 floatx80_div(floatx80 a, floatx80 b, float_status *status)
{
    flag aSign, bSign, zSign;
    int32_t aExp, bExp, zExp;
    uint64_t aSig, bSig, zSig0, zSig1;
    uint64_t rem0, rem1, rem2, term0, term1, term2;

    if (floatx80_invalid_encoding(a) || floatx80_invalid_encoding(b)) {
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
    bSign = extractFloatx80Sign( b );
    zSign = aSign ^ bSign;
    if ( aExp == 0x7FFF ) {
        if ((uint64_t)(aSig << 1)) {
            return propagateFloatx80NaN(a, b, status);
        }
        if ( bExp == 0x7FFF ) {
            if ((uint64_t)(bSig << 1)) {
                return propagateFloatx80NaN(a, b, status);
            }
            goto invalid;
        }
        return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
    }
    if ( bExp == 0x7FFF ) {
        if ((uint64_t)(bSig << 1)) {
            return propagateFloatx80NaN(a, b, status);
        }
        return packFloatx80( zSign, 0, 0 );
    }
    if ( bExp == 0 ) {
        if ( bSig == 0 ) {
            if ( ( aExp | aSig ) == 0 ) {
 invalid:
                float_raise(float_flag_invalid, status);
                return floatx80_default_nan(status);
            }
            float_raise(float_flag_divbyzero, status);
            return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
        }
        normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( zSign, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    zExp = aExp - bExp + 0x3FFE;
    rem1 = 0;
    if ( bSig <= aSig ) {
        shift128Right( aSig, 0, 1, &aSig, &rem1 );
        ++zExp;
    }
    zSig0 = estimateDiv128To64( aSig, rem1, bSig );
    mul64To128( bSig, zSig0, &term0, &term1 );
    sub128( aSig, rem1, term0, term1, &rem0, &rem1 );
    while ( (int64_t) rem0 < 0 ) {
        --zSig0;
        add128( rem0, rem1, 0, bSig, &rem0, &rem1 );
    }
    zSig1 = estimateDiv128To64( rem1, 0, bSig );
    if ( (uint64_t) ( zSig1<<1 ) <= 8 ) {
        mul64To128( bSig, zSig1, &term1, &term2 );
        sub128( rem1, 0, term1, term2, &rem1, &rem2 );
        while ( (int64_t) rem1 < 0 ) {
            --zSig1;
            add128( rem1, rem2, 0, bSig, &rem1, &rem2 );
        }
        zSig1 |= ( ( rem1 | rem2 ) != 0 );
    }
    return roundAndPackFloatx80(status->floatx80_rounding_precision,
                                zSign, zExp, zSig0, zSig1, status);
}

#ifdef SOFTFLOAT_68K // 21-01-2017: Addition for Previous
floatx80 floatx80_sgldiv( floatx80 a, floatx80 b, float_status *status )
{
	flag aSign, bSign, zSign;
	int32_t aExp, bExp, zExp;
	uint64_t aSig, bSig, zSig0, zSig1;
	uint64_t rem0, rem1, rem2, term0, term1, term2;
	floatx80 z;
	
	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	bSig = extractFloatx80Frac( b );
	bExp = extractFloatx80Exp( b );
	bSign = extractFloatx80Sign( b );
	zSign = aSign ^ bSign;
	if ( aExp == 0x7FFF ) {
		if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaN( a, b, status );
		if ( bExp == 0x7FFF ) {
			if ( (uint64_t) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b, status );
			goto invalid;
		}
		return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
	}
	if ( bExp == 0x7FFF ) {
		if ( (uint64_t) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b, status );
		return packFloatx80( zSign, 0, 0 );
	}
	if ( bExp == 0 ) {
		if ( bSig == 0 ) {
			if ( ( aExp | aSig ) == 0 ) {
			invalid:
				float_raise( float_flag_invalid, status );
				z.low = floatx80_default_nan_low;
				z.high = floatx80_default_nan_high;
		        return z;
			}
			float_raise( float_flag_divbyzero, status );
			return packFloatx80( zSign, 0x7FFF, floatx80_default_infinity_low );
		}
		normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
	}
	if ( aExp == 0 ) {
		if ( aSig == 0 ) return packFloatx80( zSign, 0, 0 );
		normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
	}

	zExp = aExp - bExp + 0x3FFE;
	rem1 = 0;
	if ( bSig <= aSig ) {
		shift128Right( aSig, 0, 1, &aSig, &rem1 );
		++zExp;
	}
	zSig0 = estimateDiv128To64( aSig, rem1, bSig );
	mul64To128( bSig, zSig0, &term0, &term1 );
	sub128( aSig, rem1, term0, term1, &rem0, &rem1 );
	while ( (int64_t) rem0 < 0 ) {
		--zSig0;
		add128( rem0, rem1, 0, bSig, &rem0, &rem1 );
	}
	zSig1 = estimateDiv128To64( rem1, 0, bSig );
	if ( (uint64_t) ( zSig1<<1 ) <= 8 ) {
		mul64To128( bSig, zSig1, &term1, &term2 );
		sub128( rem1, 0, term1, term2, &rem1, &rem2 );
		while ( (int64_t) rem1 < 0 ) {
			--zSig1;
			add128( rem1, rem2, 0, bSig, &rem1, &rem2 );
		}
		zSig1 |= ( ( rem1 | rem2 ) != 0 );
	}
	return roundSigAndPackFloatx80( 32, zSign, zExp, zSig0, zSig1, status);
        
}
#endif // End of addition for Previous
   

/*----------------------------------------------------------------------------
| Returns the remainder of the extended double-precision floating-point value
| `a' with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#ifndef SOFTFLOAT_68K
floatx80 floatx80_rem(floatx80 a, floatx80 b, float_status *status)
{
    flag aSign, zSign;
    int32_t aExp, bExp, expDiff;
    uint64_t aSig0, aSig1, bSig;
    uint64_t q, term0, term1, alternateASig0, alternateASig1;

    if (floatx80_invalid_encoding(a) || floatx80_invalid_encoding(b)) {
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    aSig0 = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
    if ( aExp == 0x7FFF ) {
        if (    (uint64_t) ( aSig0<<1 )
             || ( ( bExp == 0x7FFF ) && (uint64_t) ( bSig<<1 ) ) ) {
            return propagateFloatx80NaN(a, b, status);
        }
        goto invalid;
    }
    if ( bExp == 0x7FFF ) {
        if ((uint64_t)(bSig << 1)) {
            return propagateFloatx80NaN(a, b, status);
        }
        return a;
    }
    if ( bExp == 0 ) {
        if ( bSig == 0 ) {
 invalid:
            float_raise(float_flag_invalid, status);
            return floatx80_default_nan(status);
        }
        normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
    }
    if ( aExp == 0 ) {
        if ( (uint64_t) ( aSig0<<1 ) == 0 ) return a;
        normalizeFloatx80Subnormal( aSig0, &aExp, &aSig0 );
    }
    bSig |= LIT64( 0x8000000000000000 );
    zSign = aSign;
    expDiff = aExp - bExp;
    aSig1 = 0;
    if ( expDiff < 0 ) {
        if ( expDiff < -1 ) return a;
        shift128Right( aSig0, 0, 1, &aSig0, &aSig1 );
        expDiff = 0;
    }
    q = ( bSig <= aSig0 );
    if ( q ) aSig0 -= bSig;
    expDiff -= 64;
    while ( 0 < expDiff ) {
        q = estimateDiv128To64( aSig0, aSig1, bSig );
        q = ( 2 < q ) ? q - 2 : 0;
        mul64To128( bSig, q, &term0, &term1 );
        sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        shortShift128Left( aSig0, aSig1, 62, &aSig0, &aSig1 );
        expDiff -= 62;
    }
    expDiff += 64;
    if ( 0 < expDiff ) {
        q = estimateDiv128To64( aSig0, aSig1, bSig );
        q = ( 2 < q ) ? q - 2 : 0;
        q >>= 64 - expDiff;
        mul64To128( bSig, q<<( 64 - expDiff ), &term0, &term1 );
        sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        shortShift128Left( 0, bSig, 64 - expDiff, &term0, &term1 );
        while ( le128( term0, term1, aSig0, aSig1 ) ) {
            ++q;
            sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        }
    }
    else {
        term1 = 0;
        term0 = bSig;
    }
    sub128( term0, term1, aSig0, aSig1, &alternateASig0, &alternateASig1 );
    if (    lt128( alternateASig0, alternateASig1, aSig0, aSig1 )
         || (    eq128( alternateASig0, alternateASig1, aSig0, aSig1 )
              && ( q & 1 ) )
       ) {
        aSig0 = alternateASig0;
        aSig1 = alternateASig1;
        zSign = ! zSign;
    }
    return
        normalizeRoundAndPackFloatx80(
            80, zSign, bExp + expDiff, aSig0, aSig1, status);

}
#else // 09-01-2017: Modified version for Previous
floatx80 floatx80_rem( floatx80 a, floatx80 b, uint64_t *q, flag *s, float_status *status )
{
    flag aSign, bSign, zSign;
    int32_t aExp, bExp, expDiff;
    uint64_t aSig0, aSig1, bSig;
    uint64_t qTemp, term0, term1, alternateASig0, alternateASig1;
    floatx80 z;
    
    aSig0 = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
    bSign = extractFloatx80Sign( b );

	if ( aExp == 0x7FFF ) {
        if (    (uint64_t) ( aSig0<<1 )
            || ( ( bExp == 0x7FFF ) && (uint64_t) ( bSig<<1 ) ) ) {
            return propagateFloatx80NaN( a, b, status );
        }
        goto invalid;
    }
    if ( bExp == 0x7FFF ) {
        if ( (uint64_t) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b, status );
        *s = (aSign != bSign);
        *q = 0;
        return a;
    }
    if ( bExp == 0 ) {
        if ( bSig == 0 ) {
        invalid:
            float_raise( float_flag_invalid, status );
            z.low = floatx80_default_nan_low;
            z.high = floatx80_default_nan_high;
            return z;
        }
        normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
    }
    if ( aExp == 0 ) {
#ifdef SOFTFLOAT_68K
        if ( aSig0 == 0 ) {
            *s = (aSign != bSign);
            *q = 0;
            return a;
        }
#else
        if ( (uint64_t) ( aSig0<<1 ) == 0 ) return a;
#endif
        normalizeFloatx80Subnormal( aSig0, &aExp, &aSig0 );
    }
    bSig |= LIT64( 0x8000000000000000 );
    zSign = aSign;
    expDiff = aExp - bExp;
    *s = (aSign != bSign);
    aSig1 = 0;
    if ( expDiff < 0 ) {
        if ( expDiff < -1 ) return a;
        shift128Right( aSig0, 0, 1, &aSig0, &aSig1 );
        expDiff = 0;
    }
    qTemp = ( bSig <= aSig0 );
    if ( qTemp ) aSig0 -= bSig;
    *q = ( expDiff > 63 ) ? 0 : ( qTemp<<expDiff );
    expDiff -= 64;
    while ( 0 < expDiff ) {
        qTemp = estimateDiv128To64( aSig0, aSig1, bSig );
        qTemp = ( 2 < qTemp ) ? qTemp - 2 : 0;
        mul64To128( bSig, qTemp, &term0, &term1 );
        sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        shortShift128Left( aSig0, aSig1, 62, &aSig0, &aSig1 );
        *q = ( expDiff > 63 ) ? 0 : ( qTemp<<expDiff );
        expDiff -= 62;
    }
    expDiff += 64;
    if ( 0 < expDiff ) {
        qTemp = estimateDiv128To64( aSig0, aSig1, bSig );
        qTemp = ( 2 < qTemp ) ? qTemp - 2 : 0;
        qTemp >>= 64 - expDiff;
        mul64To128( bSig, qTemp<<( 64 - expDiff ), &term0, &term1 );
        sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        shortShift128Left( 0, bSig, 64 - expDiff, &term0, &term1 );
        while ( le128( term0, term1, aSig0, aSig1 ) ) {
            ++qTemp;
            sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        }
        *q += qTemp;
    }
    else {
        term1 = 0;
        term0 = bSig;
    }
    sub128( term0, term1, aSig0, aSig1, &alternateASig0, &alternateASig1 );
    if (    lt128( alternateASig0, alternateASig1, aSig0, aSig1 )
        || (    eq128( alternateASig0, alternateASig1, aSig0, aSig1 )
            && ( qTemp & 1 ) )
        ) {
        aSig0 = alternateASig0;
        aSig1 = alternateASig1;
        zSign = ! zSign;
        ++*q;
    }
    return
    normalizeRoundAndPackFloatx80(
                                  80, zSign, bExp + expDiff, aSig0, aSig1, status );
    
}
#endif // End of modification


#ifdef SOFTFLOAT_68K // 08-01-2017: Added for Previous
/*----------------------------------------------------------------------------
 | Returns the modulo remainder of the extended double-precision floating-point
 | value `a' with respect to the corresponding value `b'.
 *----------------------------------------------------------------------------*/

floatx80 floatx80_mod( floatx80 a, floatx80 b, uint64_t *q, flag *s, float_status *status )
{
    flag aSign, bSign, zSign;
    int32_t aExp, bExp, expDiff;
    uint64_t aSig0, aSig1, bSig;
    uint64_t qTemp, term0, term1;
    floatx80 z;
    
    aSig0 = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
    bSign = extractFloatx80Sign( b );

	if ( aExp == 0x7FFF ) {
        if (    (uint64_t) ( aSig0<<1 )
            || ( ( bExp == 0x7FFF ) && (uint64_t) ( bSig<<1 ) ) ) {
            return propagateFloatx80NaN( a, b, status );
        }
        goto invalid;
    }
    if ( bExp == 0x7FFF ) {
        if ( (uint64_t) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b, status );
        *s = (aSign != bSign);
        *q = 0;
        return a;
    }
    if ( bExp == 0 ) {
        if ( bSig == 0 ) {
        invalid:
            float_raise( float_flag_invalid, status );
            z.low = floatx80_default_nan_low;
            z.high = floatx80_default_nan_high;
            return z;
        }
        normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
    }
    if ( aExp == 0 ) {
#ifdef SOFTFLOAT_68K
        if ( aSig0 == 0 ) {
            *s = (aSign != bSign);
            *q = 0;
            return a;
        }
#else
        if ( (uint64_t) ( aSig0<<1 ) == 0 ) return a;
#endif
        normalizeFloatx80Subnormal( aSig0, &aExp, &aSig0 );
    }
    bSig |= LIT64( 0x8000000000000000 );
    zSign = aSign;
    expDiff = aExp - bExp;
    *s = (aSign != bSign);
    aSig1 = 0;
    if ( expDiff < 0 ) return a;
    qTemp = ( bSig <= aSig0 );
    if ( qTemp ) aSig0 -= bSig;
    *q = ( expDiff > 63 ) ? 0 : ( qTemp<<expDiff );
    expDiff -= 64;
    while ( 0 < expDiff ) {
        qTemp = estimateDiv128To64( aSig0, aSig1, bSig );
        qTemp = ( 2 < qTemp ) ? qTemp - 2 : 0;
        mul64To128( bSig, qTemp, &term0, &term1 );
        sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        shortShift128Left( aSig0, aSig1, 62, &aSig0, &aSig1 );
        *q = ( expDiff > 63 ) ? 0 : ( qTemp<<expDiff );
		expDiff -= 62;
    }
    expDiff += 64;
    if ( 0 < expDiff ) {
        qTemp = estimateDiv128To64( aSig0, aSig1, bSig );
        qTemp = ( 2 < qTemp ) ? qTemp - 2 : 0;
        qTemp >>= 64 - expDiff;
        mul64To128( bSig, qTemp<<( 64 - expDiff ), &term0, &term1 );
        sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        shortShift128Left( 0, bSig, 64 - expDiff, &term0, &term1 );
        while ( le128( term0, term1, aSig0, aSig1 ) ) {
            ++qTemp;
            sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
        }
        *q += qTemp;
    }
    return
        normalizeRoundAndPackFloatx80(
            80, zSign, bExp + expDiff, aSig0, aSig1, status );
    
}
#endif // end of addition for Previous


/*----------------------------------------------------------------------------
| Returns the square root of the extended double-precision floating-point
| value `a'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 floatx80_sqrt(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp, zExp;
    uint64_t aSig0, aSig1, zSig0, zSig1, doubleZSig0;
    uint64_t rem0, rem1, rem2, rem3, term0, term1, term2, term3;

    if (floatx80_invalid_encoding(a)) {
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    aSig0 = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    if ( aExp == 0x7FFF ) {
        if ((uint64_t)(aSig0 << 1)) {
            return propagateFloatx80NaNOneArg(a, status);
        }
        if ( ! aSign ) return a;
        goto invalid;
    }
    if ( aSign ) {
        if ( ( aExp | aSig0 ) == 0 ) return a;
 invalid:
        float_raise(float_flag_invalid, status);
        return floatx80_default_nan(status);
    }
    if ( aExp == 0 ) {
        if ( aSig0 == 0 ) return packFloatx80( 0, 0, 0 );
        normalizeFloatx80Subnormal( aSig0, &aExp, &aSig0 );
    }
    zExp = ( ( aExp - 0x3FFF )>>1 ) + 0x3FFF;
    zSig0 = estimateSqrt32( aExp, aSig0>>32 );
    shift128Right( aSig0, 0, 2 + ( aExp & 1 ), &aSig0, &aSig1 );
    zSig0 = estimateDiv128To64( aSig0, aSig1, zSig0<<32 ) + ( zSig0<<30 );
    doubleZSig0 = zSig0<<1;
    mul64To128( zSig0, zSig0, &term0, &term1 );
    sub128( aSig0, aSig1, term0, term1, &rem0, &rem1 );
    while ( (int64_t) rem0 < 0 ) {
        --zSig0;
        doubleZSig0 -= 2;
        add128( rem0, rem1, zSig0>>63, doubleZSig0 | 1, &rem0, &rem1 );
    }
    zSig1 = estimateDiv128To64( rem1, 0, doubleZSig0 );
    if ( ( zSig1 & LIT64( 0x3FFFFFFFFFFFFFFF ) ) <= 5 ) {
        if ( zSig1 == 0 ) zSig1 = 1;
        mul64To128( doubleZSig0, zSig1, &term1, &term2 );
        sub128( rem1, 0, term1, term2, &rem1, &rem2 );
        mul64To128( zSig1, zSig1, &term2, &term3 );
        sub192( rem1, rem2, 0, 0, term2, term3, &rem1, &rem2, &rem3 );
        while ( (int64_t) rem1 < 0 ) {
            --zSig1;
            shortShift128Left( 0, zSig1, 1, &term2, &term3 );
            term3 |= 1;
            term2 |= doubleZSig0;
            add192( rem1, rem2, rem3, 0, term2, term3, &rem1, &rem2, &rem3 );
        }
        zSig1 |= ( ( rem1 | rem2 | rem3 ) != 0 );
    }
    shortShift128Left( 0, zSig1, 1, &zSig0, &zSig1 );
    zSig0 |= doubleZSig0;
    return roundAndPackFloatx80(status->floatx80_rounding_precision,
                                0, zExp, zSig0, zSig1, status);
}


#ifdef SOFTFLOAT_68K // 07-01-2017: Added for Previous
/*----------------------------------------------------------------------------
 | Returns the mantissa of the extended double-precision floating-point
 | value `a'.
 *----------------------------------------------------------------------------*/

floatx80 floatx80_getman( floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaNOneArg( a, status );
        float_raise( float_flag_invalid, status );
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    
    return roundAndPackFloatx80(status->floatx80_rounding_precision, aSign, 0x3FFF, aSig, 0, status);
}

/*----------------------------------------------------------------------------
 | Returns the exponent of the extended double-precision floating-point
 | value `a' as an extended double-precision value.
 *----------------------------------------------------------------------------*/

floatx80 floatx80_getexp( floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaNOneArg( a, status );
        float_raise( float_flag_invalid, status );
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    
    return int32_to_floatx80(aExp - 0x3FFF);
}

/*----------------------------------------------------------------------------
 | Scales extended double-precision floating-point value in operand `a' by
 | value `b'. The function truncates the value in the second operand 'b' to
 | an integral value and adds that value to the exponent of the operand 'a'.
 | The operation performed according to the IEC/IEEE Standard for Binary
 | Floating-Point Arithmetic.
 *----------------------------------------------------------------------------*/

floatx80 floatx80_scale(floatx80 a, floatx80 b, float_status *status)
{
    flag aSign, bSign;
    int32_t aExp, bExp, shiftCount;
    uint64_t aSig, bSig;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    bSig = extractFloatx80Frac(b);
    bExp = extractFloatx80Exp(b);
    bSign = extractFloatx80Sign(b);
    
    if ( bExp == 0x7FFF ) {
        if ( (uint64_t) ( bSig<<1 ) ||
            ( ( aExp == 0x7FFF ) && (uint64_t) ( aSig<<1 ) ) ) {
            return propagateFloatx80NaN( a, b, status );
        }
        float_raise( float_flag_invalid, status );
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaN( a, b, status );
        return packFloatx80( aSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( aSign, 0, 0);
        if ( bExp < 0x3FFF ) return a;
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    
    if ( bExp < 0x3FFF ) return a;
    
    if ( 0x400F < bExp ) {
        aExp = bSign ? -0x6001 : 0xE000;
        return roundAndPackFloatx80(
                    status->floatx80_rounding_precision, aSign, aExp, aSig, 0, status );
    }
    
    shiftCount = 0x403E - bExp;
    bSig >>= shiftCount;
    aExp = bSign ? ( aExp - bSig ) : ( aExp + bSig );
    
    return roundAndPackFloatx80(
                status->floatx80_rounding_precision, aSign, aExp, aSig, 0, status);
    
}
 
/*-----------------------------------------------------------------------------
 | Calculates the absolute value of the extended double-precision floating-point
 | value `a'.  The operation is performed according to the IEC/IEEE Standard
 | for Binary Floating-Point Arithmetic.
 *----------------------------------------------------------------------------*/
    
floatx80 floatx80_abs(floatx80 a, float_status *status)
{
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    
    if ( aExp == 0x7FFF && (uint64_t) ( aSig<<1 ) ) {
        return propagateFloatx80NaNOneArg( a, status );
    }
    
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( 0, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }

    return roundAndPackFloatx80(
                status->floatx80_rounding_precision, 0, aExp, aSig, 0, status );
    
}
    
/*-----------------------------------------------------------------------------
 | Changes the sign of the extended double-precision floating-point value 'a'.
 | The operation is performed according to the IEC/IEEE Standard for Binary
 | Floating-Point Arithmetic.
 *----------------------------------------------------------------------------*/
    
floatx80 floatx80_neg(floatx80 a, float_status *status)
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if ( aExp == 0x7FFF && (uint64_t) ( aSig<<1 ) ) {
        return propagateFloatx80NaNOneArg( a, status );
    }
    
    aSign = !aSign;
    
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
        normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
    }
    
    return roundAndPackFloatx80(
                status->floatx80_rounding_precision, aSign, aExp, aSig, 0, status );
    
}

/*----------------------------------------------------------------------------
 | Returns the result of comparing the extended double-precision floating-
 | point values `a' and `b'.  The result is abstracted for matching the
 | corresponding condition codes.
 *----------------------------------------------------------------------------*/
    
floatx80 floatx80_cmp( floatx80 a, floatx80 b, float_status *status )
{
    flag aSign, bSign;
    int32_t aExp, bExp;
    uint64_t aSig, bSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    bSig = extractFloatx80Frac( b );
    bExp = extractFloatx80Exp( b );
    bSign = extractFloatx80Sign( b );
    
    if ( ( aExp == 0x7FFF && (uint64_t) ( aSig<<1 ) ) ||
         ( bExp == 0x7FFF && (uint64_t) ( bSig<<1 ) ) ) {
        return propagateFloatx80NaN( packFloatx80( 0, aExp, aSig ),
                                     packFloatx80( 0, bExp, bSig ), status );
	}
    
    if ( bExp < aExp ) return packFloatx80( aSign, 0x3FFF, LIT64( 0x8000000000000000 ) );
    if ( aExp < bExp ) return packFloatx80( bSign ^ 1, 0x3FFF, LIT64( 0x8000000000000000 ) );
    
    if ( aExp == 0x7FFF ) {
        if ( aSign == bSign ) return packFloatx80( aSign, 0, 0 );
        return packFloatx80( aSign, 0x3FFF, LIT64( 0x8000000000000000 ) );
    }
    
    if ( bSig < aSig ) return packFloatx80( aSign, 0x3FFF, LIT64( 0x8000000000000000 ) );
    if ( aSig < bSig ) return packFloatx80( bSign ^ 1, 0x3FFF, LIT64( 0x8000000000000000 ) );
    
    if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
    
    if ( aSign == bSign ) return packFloatx80( 0, 0, 0 );
    
    return packFloatx80( aSign, 0x3FFF, LIT64( 0x8000000000000000 ) );
    
}

floatx80 floatx80_tst( floatx80 a, float_status *status )
{
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    
    if ( aExp == 0x7FFF && (uint64_t) ( aSig<<1 ) )
        return propagateFloatx80NaNOneArg( a, status );
    return a;
}

floatx80 floatx80_move( floatx80 a, float_status *status )
{
    flag aSign;
    int32_t aExp;
    uint64_t aSig;
    
    aSig = extractFloatx80Frac( a );
    aExp = extractFloatx80Exp( a );
    aSign = extractFloatx80Sign( a );
    
    if ( aExp == 0x7FFF ) {
        if ( (uint64_t) ( aSig<<1 ) ) return propagateFloatx80NaNOneArg( a, status );
        return a;
    }
    if ( aExp == 0 ) {
        if ( aSig == 0 ) return a;
        normalizeRoundAndPackFloatx80( status->floatx80_rounding_precision, aSign, aExp, aSig, 0, status );
    }
    return roundAndPackFloatx80( status->floatx80_rounding_precision, aSign, aExp, aSig, 0, status );
}

floatx80 floatx80_denormalize( floatx80 a, flag eSign)
{
	flag aSign;
	int32_t aExp;
	uint64_t aSig;
	int32_t shiftCount;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	
	if ( eSign ) {
		shiftCount = 0x8000 - aExp;
		aExp = 0;
		if (shiftCount > 63) {
			aSig = 0;
		} else {
			aSig >>= shiftCount;
		}
	}
	return packFloatx80(aSign, aExp, aSig);
}

#endif // End of addition for Previous

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| equal to the corresponding value `b', and 0 otherwise.  The comparison is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

flag floatx80_eq( floatx80 a, floatx80 b, float_status *status )
{
	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
				&& (uint64_t) ( extractFloatx80Frac( a )<<1 ) )
			|| (    ( extractFloatx80Exp( b ) == 0x7FFF )
				&& (uint64_t) ( extractFloatx80Frac( b )<<1 ) )
		) {
		if (    floatx80_is_signaling_nan( a )
				|| floatx80_is_signaling_nan( b ) ) {
			float_raise( float_flag_invalid, status );
		}
		return 0;
	}
	return
			( a.low == b.low )
		&& (    ( a.high == b.high )
				|| (    ( a.low == 0 )
					&& ( (uint16_t) ( ( a.high | b.high )<<1 ) == 0 ) )
			);

}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| less than or equal to the corresponding value `b', and 0 otherwise.  The
| comparison is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

flag floatx80_le( floatx80 a, floatx80 b, float_status *status )
{
	flag aSign, bSign;

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
				&& (uint64_t) ( extractFloatx80Frac( a )<<1 ) )
			|| (    ( extractFloatx80Exp( b ) == 0x7FFF )
				&& (uint64_t) ( extractFloatx80Frac( b )<<1 ) )
		) {
		float_raise( float_flag_invalid, status );
		return 0;
	}
	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign != bSign ) {
		return
				aSign
			|| (    ( ( (uint16_t) ( ( a.high | b.high )<<1 ) ) | a.low | b.low )
					== 0 );
	}
	return
			aSign ? le128( b.high, b.low, a.high, a.low )
		: le128( a.high, a.low, b.high, b.low );
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| less than the corresponding value `b', and 0 otherwise.  The comparison
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

flag floatx80_lt( floatx80 a, floatx80 b, float_status *status )
{
	flag aSign, bSign;

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
				&& (uint64_t) ( extractFloatx80Frac( a )<<1 ) )
			|| (    ( extractFloatx80Exp( b ) == 0x7FFF )
				&& (uint64_t) ( extractFloatx80Frac( b )<<1 ) )
		) {
		float_raise( float_flag_invalid, status );
		return 0;
	}
	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign != bSign ) {
		return
				aSign
			&& (    ( ( (uint16_t) ( ( a.high | b.high )<<1 ) ) | a.low | b.low )
					!= 0 );
	}
	return
			aSign ? lt128( b.high, b.low, a.high, a.low )
		: lt128( a.high, a.low, b.high, b.low );

}


/*----------------------------------------------------------------------------
| Returns the result of converting the 64-bit two's complement integer `a'
| to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

floatx80 int64_to_floatx80( int64_t a )
{
	flag zSign;
	uint64_t absA;
	int8_t shiftCount;

	if ( a == 0 ) return packFloatx80( 0, 0, 0 );
	zSign = ( a < 0 );
	absA = zSign ? - a : a;
	shiftCount = countLeadingZeros64( absA );
	return packFloatx80( zSign, 0x403E - shiftCount, absA<<shiftCount );
}
