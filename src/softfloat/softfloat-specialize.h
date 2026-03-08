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
This C source fragment is part of the SoftFloat IEC/IEEE Floating-point
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

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is a
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

static inline flag floatx80_is_nan( floatx80 a )
{

    return ( ( a.high & 0x7FFF ) == 0x7FFF ) && (uint64_t) ( a.low<<1 );

}

/*----------------------------------------------------------------------------
| The pattern for a default generated extended double-precision NaN.
*----------------------------------------------------------------------------*/
static inline floatx80 floatx80_default_nan(float_status *status)
{
    floatx80 r;
    r.high = 0x7FFF;
    r.low = LIT64( 0xFFFFFFFFFFFFFFFF );
	return r;
}

/*----------------------------------------------------------------------------
| Raises the exceptions specified by `flags'.  Floating-point traps can be
| defined here if desired.  It is currently not possible for such a trap
| to substitute a result value.  If traps are not implemented, this routine
| should be simply `float_exception_flags |= flags;'.
*----------------------------------------------------------------------------*/

static inline void float_raise(uint8_t flags, float_status *status)
{
    status->float_exception_flags |= flags;
}

/*----------------------------------------------------------------------------
| Internal canonical NaN format.
*----------------------------------------------------------------------------*/
typedef struct {
    flag sign;
    uint64_t high, low;
} commonNaNT;

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

static inline flag float32_is_nan( float32 a )
{

    return ( 0xFF000000 < (uint32_t) ( a<<1 ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

static inline flag float32_is_signaling_nan( float32 a )
{

    return ( ( ( a>>22 ) & 0x1FF ) == 0x1FE ) && ( a & 0x003FFFFF );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

static inline commonNaNT float32ToCommonNaN( float32 a, float_status *status )
{
    commonNaNT z;

    if ( float32_is_signaling_nan( a ) ) float_raise( float_flag_signaling, status );
    z.sign = a>>31;
    z.low = 0;
    z.high = ( (uint64_t) a )<<41;
    return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the single-
| precision floating-point format.
*----------------------------------------------------------------------------*/

static inline float32 commonNaNToFloat32( commonNaNT a )
{

    return ( ( (uint32_t) a.sign )<<31 ) | 0x7FC00000 | ( a.high>>41 );

}

/*----------------------------------------------------------------------------
| Takes two single-precision floating-point values `a' and `b', one of which
| is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

static inline float32 propagateFloat32NaN( float32 a, float32 b, float_status *status )
{
    flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

    aIsNaN = float32_is_nan( a );
    aIsSignalingNaN = float32_is_signaling_nan( a );
    bIsNaN = float32_is_nan( b );
    bIsSignalingNaN = float32_is_signaling_nan( b );
    a |= 0x00400000;
    b |= 0x00400000;
    if ( aIsSignalingNaN | bIsSignalingNaN ) float_raise( float_flag_signaling, status );
    if ( aIsNaN ) {
        return ( aIsSignalingNaN & bIsNaN ) ? b : a;
    }
    else {
        return b;
    }

}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

static inline flag float64_is_nan( float64 a )
{

    return ( LIT64( 0xFFE0000000000000 ) < (uint64_t) ( a<<1 ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

static inline flag float64_is_signaling_nan( float64 a )
{

    return
           ( ( ( a>>51 ) & 0xFFF ) == 0xFFE )
        && ( a & LIT64( 0x0007FFFFFFFFFFFF ) );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

static inline commonNaNT float64ToCommonNaN(float64 a, float_status *status)
{
    commonNaNT z;

    if (float64_is_signaling_nan(a)) {
        float_raise(float_flag_invalid, status);
    }
    z.sign = float64_val(a) >> 63;
    z.low = 0;
    z.high = float64_val(a) << 12;
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the double-
| precision floating-point format.
*----------------------------------------------------------------------------*/

static inline float64 commonNaNToFloat64(commonNaNT a, float_status *status)
{
     return
          ( ( (uint64_t) a.sign )<<63 )
        | LIT64( 0x7FF8000000000000 )
        | ( a.high>>12 );
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is a
| signaling NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

static inline flag floatx80_is_signaling_nan( floatx80 a )
{
    uint64_t aLow;

    aLow = a.low & ~ LIT64( 0x4000000000000000 );
    return
           ( ( a.high & 0x7FFF ) == 0x7FFF )
        && (uint64_t) ( aLow<<1 )
        && ( a.low == aLow );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point NaN `a' to the canonical NaN format.  If `a' is a signaling NaN, the
| invalid exception is raised.
*----------------------------------------------------------------------------*/

static inline commonNaNT floatx80ToCommonNaN( floatx80 a, float_status *status )
{
    commonNaNT z;

    if ( floatx80_is_signaling_nan( a ) ) float_raise( float_flag_signaling, status );
    z.sign = a.high>>15;
    z.low = 0;
    z.high = a.low<<1;
    return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the extended
| double-precision floating-point format.
*----------------------------------------------------------------------------*/

static inline floatx80 commonNaNToFloatx80(commonNaNT a, float_status *status)
{
    floatx80 z;
#ifdef SOFTFLOAT_68K
    z.low = LIT64( 0x4000000000000000 ) | ( a.high>>1 );
#else
    z.low = LIT64( 0xC000000000000000 ) | ( a.high>>1 );
#endif
    z.high = ( ( (int16_t) a.sign )<<15 ) | 0x7FFF;
    return z;
}

/*----------------------------------------------------------------------------
| Takes two extended double-precision floating-point values `a' and `b', one
| of which is a NaN, and returns the appropriate NaN result.  If either `a' or
| `b' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

static inline floatx80 propagateFloatx80NaN( floatx80 a, floatx80 b, float_status *status )
{
    flag aIsNaN, aIsSignalingNaN, bIsSignalingNaN;
#ifndef SOFTFLOAT_68K
    flag bIsNaN;
#endif 

	aIsNaN = floatx80_is_nan( a );
    aIsSignalingNaN = floatx80_is_signaling_nan( a );
    bIsSignalingNaN = floatx80_is_signaling_nan( b );
#ifdef SOFTFLOAT_68K
    a.low |= LIT64( 0x4000000000000000 );
    b.low |= LIT64( 0x4000000000000000 );
    if ( aIsSignalingNaN | bIsSignalingNaN ) float_raise( float_flag_signaling, status );
    return aIsNaN ? a : b;
#else
    bIsNaN = floatx80_is_nan( b );
    a.low |= LIT64( 0xC000000000000000 );
    b.low |= LIT64( 0xC000000000000000 );
    if ( aIsSignalingNaN | bIsSignalingNaN ) float_raise( float_flag_signaling, status );
    if ( aIsNaN ) {
        return ( aIsSignalingNaN & bIsNaN ) ? b : a;
    }
    else {
        return b;
    }
#endif

}

#ifdef SOFTFLOAT_68K
/*----------------------------------------------------------------------------
 | Takes extended double-precision floating-point  NaN  `a' and returns the
 | appropriate NaN result. If `a' is a signaling NaN, the invalid exception
 | is raised.
 *----------------------------------------------------------------------------*/

static inline floatx80 propagateFloatx80NaNOneArg(floatx80 a, float_status *status)
{
    if ( floatx80_is_signaling_nan( a ) )
        float_raise( float_flag_signaling, status );
    a.low |= LIT64( 0x4000000000000000 );
    
    return a;
}
#endif

// 28-12-2016: Added for Previous:

/*----------------------------------------------------------------------------
 | Returns 1 if the extended double-precision floating-point value `a' is
 | zero; otherwise returns 0.
 *----------------------------------------------------------------------------*/

static inline flag floatx80_is_zero( floatx80 a )
{
    
    return ( ( a.high & 0x7FFF ) < 0x7FFF ) && ( a.low == 0 );
    
}

/*----------------------------------------------------------------------------
 | Returns 1 if the extended double-precision floating-point value `a' is
 | infinity; otherwise returns 0.
 *----------------------------------------------------------------------------*/

static inline flag floatx80_is_infinity( floatx80 a )
{
    
    return ( ( a.high & 0x7FFF ) == 0x7FFF ) && ( (uint64_t) ( a.low<<1 ) == 0 );
    
}

/*----------------------------------------------------------------------------
 | Returns 1 if the extended double-precision floating-point value `a' is
 | negative; otherwise returns 0.
 *----------------------------------------------------------------------------*/

static inline flag floatx80_is_negative( floatx80 a )
{
    
    return ( ( a.high & 0x8000 ) == 0x8000 );
    
}

/*----------------------------------------------------------------------------
 | Returns 1 if the extended double-precision floating-point value `a' is
 | unnormal; otherwise returns 0.
 *----------------------------------------------------------------------------*/
static inline flag floatx80_is_unnormal( floatx80 a )
{
	return
		( ( a.high & 0x7FFF ) > 0 )
		&& ( ( a.high & 0x7FFF ) < 0x7FFF)
		&& ( (uint64_t) ( a.low & LIT64( 0x8000000000000000 ) ) == LIT64( 0x0000000000000000 ) );
}

/*----------------------------------------------------------------------------
 | Returns 1 if the extended double-precision floating-point value `a' is
 | denormal; otherwise returns 0.
 *----------------------------------------------------------------------------*/

static inline flag floatx80_is_denormal( floatx80 a )
{
	return
		( ( a.high & 0x7FFF ) == 0 )
		&& ( (uint64_t) ( a.low & LIT64( 0x8000000000000000 ) ) == LIT64( 0x0000000000000000 ) )
		&& (uint64_t) ( a.low<<1 );
}

/*----------------------------------------------------------------------------
 | Returns 1 if the extended double-precision floating-point value `a' is
 | normal; otherwise returns 0.
 *----------------------------------------------------------------------------*/

static inline flag floatx80_is_normal( floatx80 a )
{
	return
		( ( a.high & 0x7FFF ) < 0x7FFF )
		&& ( (uint64_t) ( a.low & LIT64( 0x8000000000000000 ) ) == LIT64( 0x8000000000000000 ) );
}
// End of addition for Previous

