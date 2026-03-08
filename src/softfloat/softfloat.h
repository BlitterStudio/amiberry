#define SOFTFLOAT_68K

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
This C header file is part of the SoftFloat IEC/IEEE Floating-point
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

#ifndef SOFTFLOAT_H
#define SOFTFLOAT_H

#if defined(CONFIG_SOLARIS) && defined(CONFIG_NEEDS_LIBSUNMATH)
#include <sunmath.h>
#endif


/* This 'flag' type must be able to hold at least 0 and 1. It should
 * probably be replaced with 'bool' but the uses would need to be audited
 * to check that they weren't accidentally relying on it being a larger type.
 */
typedef uint8_t flag;

#define LIT64( a ) a##ULL

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point ordering relations
*----------------------------------------------------------------------------*/
enum {
    float_relation_less      = -1,
    float_relation_equal     =  0,
    float_relation_greater   =  1,
    float_relation_unordered =  2
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point types.
*----------------------------------------------------------------------------*/
/* Use structures for soft-float types.  This prevents accidentally mixing
   them with native int/float types.  A sufficiently clever compiler and
   sane ABI should be able to see though these structs.  However
   x86/gcc 3.x seems to struggle a bit, so leave them disabled by default.  */
//#define USE_SOFTFLOAT_STRUCT_TYPES
#ifdef USE_SOFTFLOAT_STRUCT_TYPES
typedef struct {
    uint16_t v;
} float16;
#define float16_val(x) (((float16)(x)).v)
#define make_float16(x) __extension__ ({ float16 f16_val = {x}; f16_val; })
#define const_float16(x) { x }
typedef struct {
    uint32_t v;
} float32;
/* The cast ensures an error if the wrong type is passed.  */
#define float32_val(x) (((float32)(x)).v)
#define make_float32(x) __extension__ ({ float32 f32_val = {x}; f32_val; })
#define const_float32(x) { x }
typedef struct {
    uint64_t v;
} float64;
#define float64_val(x) (((float64)(x)).v)
#define make_float64(x) __extension__ ({ float64 f64_val = {x}; f64_val; })
#define const_float64(x) { x }
#else
typedef uint16_t float16;
typedef uint32_t float32;
typedef uint64_t float64;
#define float16_val(x) (x)
#define float32_val(x) (x)
#define float64_val(x) (x)
#define make_float16(x) (x)
#define make_float32(x) (x)
#define make_float64(x) (x)
#define const_float16(x) (x)
#define const_float32(x) (x)
#define const_float64(x) (x)
#endif
typedef struct {
    uint16_t high;
    uint64_t low;
} floatx80;
typedef struct {
#ifdef HOST_WORDS_BIGENDIAN
    uint64_t high, low;
#else
    uint64_t low, high;
#endif
} float128;

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
enum {
    float_tininess_after_rounding  = 0,
    float_tininess_before_rounding = 1
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point rounding mode.
*----------------------------------------------------------------------------*/
enum {
    float_round_nearest_even = 0,
    float_round_down         = 1,
    float_round_up           = 2,
    float_round_to_zero      = 3,
    float_round_ties_away    = 4,
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point exception flags.
*----------------------------------------------------------------------------*/
enum {
    float_flag_invalid   = 0x01,
	float_flag_denormal  = 0x02,
    float_flag_divbyzero = 0x04,
    float_flag_overflow  = 0x08,
    float_flag_underflow = 0x10,
    float_flag_inexact   = 0x20,
	float_flag_signaling = 0x40,
	float_flag_decimal =   0x80
};

/*----------------------------------------------------------------------------
 | Variables for storing sign, exponent and significand of overflowed or 
 | underflowed extended double-precision floating-point value.
 | Variables for storing sign, exponent and significand of internal extended 
 | double-precision floating-point value for external use.
 *----------------------------------------------------------------------------*/

extern flag floatx80_internal_sign;
extern int32_t floatx80_internal_exp;
extern uint64_t floatx80_internal_sig;
extern int32_t floatx80_internal_exp0;
extern uint64_t floatx80_internal_sig0;
extern uint64_t floatx80_internal_sig1;
extern int8_t floatx80_internal_precision;
extern int8_t floatx80_internal_mode;

typedef struct float_status {
    signed char float_detect_tininess;
    signed char float_rounding_mode;
    uint8_t     float_exception_flags;
    signed char floatx80_rounding_precision;
    /* should denormalised results go to zero and set the inexact flag? */
    flag flush_to_zero;
    /* should denormalised inputs go to zero and set the input_denormal flag? */
    flag flush_inputs_to_zero;
    flag default_nan_mode;
    flag snan_bit_is_one;
	flag floatx80_special_flags;
} float_status;

/*----------------------------------------------------------------------------
 | Function for getting sign, exponent and significand of extended
 | double-precision floating-point intermediate result for external use.
 *----------------------------------------------------------------------------*/
floatx80 getFloatInternalOverflow( void );
floatx80 getFloatInternalUnderflow( void );
floatx80 getFloatInternalRoundedAll( void );
floatx80 getFloatInternalRoundedSome( void );
floatx80 getFloatInternalUnrounded( void );
floatx80 getFloatInternalFloatx80( void );
uint64_t getFloatInternalGRS( void );

static inline void set_float_detect_tininess(int val, float_status *status)
{
    status->float_detect_tininess = val;
}
static inline void set_float_rounding_mode(int val, float_status *status)
{
    status->float_rounding_mode = val;
}
static inline void set_float_exception_flags(int val, float_status *status)
{
    status->float_exception_flags = val;
}
static inline void set_floatx80_rounding_precision(int val,
                                                   float_status *status)
{
    status->floatx80_rounding_precision = val;
}
static inline void set_flush_to_zero(flag val, float_status *status)
{
    status->flush_to_zero = val;
}
static inline void set_flush_inputs_to_zero(flag val, float_status *status)
{
    status->flush_inputs_to_zero = val;
}
static inline void set_default_nan_mode(flag val, float_status *status)
{
    status->default_nan_mode = val;
}
static inline void set_snan_bit_is_one(flag val, float_status *status)
{
    status->snan_bit_is_one = val;
}
static inline int get_float_detect_tininess(float_status *status)
{
    return status->float_detect_tininess;
}
static inline int get_float_rounding_mode(float_status *status)
{
    return status->float_rounding_mode;
}
static inline int get_float_exception_flags(float_status *status)
{
    return status->float_exception_flags;
}
static inline int get_floatx80_rounding_precision(float_status *status)
{
    return status->floatx80_rounding_precision;
}
static inline flag get_flush_to_zero(float_status *status)
{
    return status->flush_to_zero;
}
static inline flag get_flush_inputs_to_zero(float_status *status)
{
    return status->flush_inputs_to_zero;
}
static inline flag get_default_nan_mode(float_status *status)
{
    return status->default_nan_mode;
}

/*----------------------------------------------------------------------------
| Special flags for indicating some unique behavior is required.
*----------------------------------------------------------------------------*/
enum {
	cmp_signed_nan = 0x01, addsub_swap_inf = 0x02, infinity_clear_intbit = 0x04
};

static inline void set_special_flags(uint8_t flags, float_status *status)
{
	status->floatx80_special_flags = flags;
}
static inline int8_t fcmp_signed_nan(float_status *status)
{
	return status->floatx80_special_flags & cmp_signed_nan;
}
static inline int8_t faddsub_swap_inf(float_status *status)
{
	return status->floatx80_special_flags & addsub_swap_inf;
}
static inline int8_t inf_clear_intbit(float_status *status)
{
	return status->floatx80_special_flags & infinity_clear_intbit;
}

/*----------------------------------------------------------------------------
| Routine to raise any or all of the software IEC/IEEE floating-point
| exception flags.
*----------------------------------------------------------------------------*/
void float_raise(uint8_t flags, float_status *status);


/*----------------------------------------------------------------------------
 | The pattern for a default generated single-precision NaN.
 *----------------------------------------------------------------------------*/
#define float32_default_nan 0x7FFFFFFF

/*----------------------------------------------------------------------------
 | The pattern for a default generated double-precision NaN.
 *----------------------------------------------------------------------------*/
#define float64_default_nan LIT64( 0x7FFFFFFFFFFFFFFF )

/*----------------------------------------------------------------------------
 | The pattern for a default generated extended double-precision NaN.  The
 | `high' and `low' values hold the most- and least-significant bits,
 | respectively.
 *----------------------------------------------------------------------------*/
#define floatx80_default_nan_high 0x7FFF
#define floatx80_default_nan_low  LIT64( 0xFFFFFFFFFFFFFFFF )

/*----------------------------------------------------------------------------
 | The pattern for a default generated extended double-precision infinity.
 *----------------------------------------------------------------------------*/
#define floatx80_default_infinity_low  LIT64( 0x0000000000000000 )

/*----------------------------------------------------------------------------
| If `a' is denormal and we are in flush-to-zero mode then set the
| input-denormal exception and return zero. Otherwise just return the value.
*----------------------------------------------------------------------------*/
float64 float64_squash_input_denormal(float64 a, float_status *status);

/*----------------------------------------------------------------------------
| Options to indicate which negations to perform in float*_muladd()
| Using these differs from negating an input or output before calling
| the muladd function in that this means that a NaN doesn't have its
| sign bit inverted before it is propagated.
| We also support halving the result before rounding, as a special
| case to support the ARM fused-sqrt-step instruction FRSQRTS.
*----------------------------------------------------------------------------*/
enum {
    float_muladd_negate_c = 1,
    float_muladd_negate_product = 2,
    float_muladd_negate_result = 4,
    float_muladd_halve_result = 8,
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/

floatx80 int32_to_floatx80(int32_t);
floatx80 int64_to_floatx80(int64_t);

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision conversion routines.
*----------------------------------------------------------------------------*/
floatx80 float32_to_floatx80(float32, float_status *status);
floatx80 float32_to_floatx80_allowunnormal(float32, float_status *status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision conversion routines.
*----------------------------------------------------------------------------*/
floatx80 float64_to_floatx80(float64, float_status *status);

floatx80 float64_to_floatx80_allowunnormal( float64 a, float_status *status );

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision conversion routines.
*----------------------------------------------------------------------------*/
int32_t floatx80_to_int32(floatx80, float_status *status);
#ifdef SOFTFLOAT_68K
int16_t floatx80_to_int16(floatx80, float_status *status);
int8_t floatx80_to_int8(floatx80, float_status *status);
#endif
int32_t floatx80_to_int32_round_to_zero(floatx80, float_status *status);
int64_t floatx80_to_int64(floatx80, float_status *status);
float32 floatx80_to_float32(floatx80, float_status *status);
float64 floatx80_to_float64(floatx80, float_status *status);
#ifdef SOFTFLOAT_68K
floatx80 floatx80_to_floatx80( floatx80, float_status *status);
floatx80 floatdecimal_to_floatx80(floatx80, float_status *status);
floatx80 floatx80_to_floatdecimal(floatx80, int32_t*, float_status *status);
#endif

uint64_t extractFloatx80Frac( floatx80 a );
int32_t extractFloatx80Exp( floatx80 a );
flag extractFloatx80Sign( floatx80 a );

floatx80 floatx80_round_to_int_toward_zero( floatx80 a, float_status *status);
floatx80 floatx80_round_to_float32( floatx80, float_status *status );
floatx80 floatx80_round_to_float64( floatx80, float_status *status );
floatx80 floatx80_round32( floatx80, float_status *status);
floatx80 floatx80_round64( floatx80, float_status *status);

flag floatx80_eq( floatx80, floatx80, float_status *status);
flag floatx80_le( floatx80, floatx80, float_status *status);
flag floatx80_lt( floatx80, floatx80, float_status *status);

#ifdef SOFTFLOAT_68K
// functions are in softfloat.c
floatx80 floatx80_move( floatx80 a, float_status *status );
floatx80 floatx80_abs( floatx80 a, float_status *status );
floatx80 floatx80_neg( floatx80 a, float_status *status );
floatx80 floatx80_getexp( floatx80 a, float_status *status );
floatx80 floatx80_getman( floatx80 a, float_status *status );
floatx80 floatx80_scale(floatx80 a, floatx80 b, float_status *status );
floatx80 floatx80_rem( floatx80 a, floatx80 b, uint64_t *q, flag *s, float_status *status );
floatx80 floatx80_mod( floatx80 a, floatx80 b, uint64_t *q, flag *s, float_status *status );
floatx80 floatx80_sglmul( floatx80 a, floatx80 b, float_status *status );
floatx80 floatx80_sgldiv( floatx80 a, floatx80 b, float_status *status );
floatx80 floatx80_cmp( floatx80 a, floatx80 b, float_status *status );
floatx80 floatx80_tst( floatx80 a, float_status *status );

// functions are in softfloat_fpsp.c
floatx80 floatx80_acos(floatx80 a, float_status *status);
floatx80 floatx80_asin(floatx80 a, float_status *status);
floatx80 floatx80_atan(floatx80 a, float_status *status);
floatx80 floatx80_atanh(floatx80 a, float_status *status);
floatx80 floatx80_cos(floatx80 a, float_status *status);
floatx80 floatx80_cosh(floatx80 a, float_status *status);
floatx80 floatx80_etox(floatx80 a, float_status *status);
floatx80 floatx80_etoxm1(floatx80 a, float_status *status);
floatx80 floatx80_log10(floatx80 a, float_status *status);
floatx80 floatx80_log2(floatx80 a, float_status *status);
floatx80 floatx80_logn(floatx80 a, float_status *status);
floatx80 floatx80_lognp1(floatx80 a, float_status *status);
floatx80 floatx80_sin(floatx80 a, float_status *status);
floatx80 floatx80_sinh(floatx80 a, float_status *status);
floatx80 floatx80_tan(floatx80 a, float_status *status);
floatx80 floatx80_tanh(floatx80 a, float_status *status);
floatx80 floatx80_tentox(floatx80 a, float_status *status);
floatx80 floatx80_twotox(floatx80 a, float_status *status);
floatx80 floatx80_sincos(floatx80 a, floatx80 *c, float_status *status);
#endif

// functions originally internal to softfloat.c
void normalizeFloatx80Subnormal( uint64_t aSig, int32_t *zExpPtr, uint64_t *zSigPtr );
floatx80 packFloatx80( flag zSign, int32_t zExp, uint64_t zSig );
floatx80 roundAndPackFloatx80(int8_t roundingPrecision, flag zSign, int32_t zExp, uint64_t zSig0, uint64_t zSig1, float_status *status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/
floatx80 floatx80_round_to_int(floatx80, float_status *status);
floatx80 floatx80_add(floatx80, floatx80, float_status *status);
floatx80 floatx80_sub(floatx80, floatx80, float_status *status);
floatx80 floatx80_mul(floatx80, floatx80, float_status *status);
floatx80 floatx80_div(floatx80, floatx80, float_status *status);
floatx80 floatx80_sqrt(floatx80, float_status *status);
floatx80 floatx80_normalize(floatx80);
floatx80 floatx80_denormalize(floatx80, flag);

static inline int floatx80_is_zero_or_denormal(floatx80 a)
{
    return (a.high & 0x7fff) == 0;
}

static inline int floatx80_is_any_nan(floatx80 a)
{
    return ((a.high & 0x7fff) == 0x7fff) && (a.low<<1);
}

/*----------------------------------------------------------------------------
| Return whether the given value is an invalid floatx80 encoding.
| Invalid floatx80 encodings arise when the integer bit is not set, but
| the exponent is not zero. The only times the integer bit is permitted to
| be zero is in subnormal numbers and the value zero.
| This includes what the Intel software developer's manual calls pseudo-NaNs,
| pseudo-infinities and un-normal numbers. It does not include
| pseudo-denormals, which must still be correctly handled as inputs even
| if they are never generated as outputs.
*----------------------------------------------------------------------------*/
static inline bool floatx80_invalid_encoding(floatx80 a)
{
    return (a.low & (1ULL << 63)) == 0 && (a.high & 0x7FFF) != 0 && (a.high & 0x7FFF) != 0x7FFF;
}

#define floatx80_zero make_floatx80(0x0000, 0x0000000000000000LL)
#define floatx80_one make_floatx80(0x3fff, 0x8000000000000000LL)
#define floatx80_ln2 make_floatx80(0x3ffe, 0xb17217f7d1cf79acLL)
#define floatx80_pi make_floatx80(0x4000, 0xc90fdaa22168c235LL)
#define floatx80_half make_floatx80(0x3ffe, 0x8000000000000000LL)
#define floatx80_infinity make_floatx80(0x7fff, 0x8000000000000000LL)

#endif /* SOFTFLOAT_H */
