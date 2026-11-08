/* Fast extended-precision transcendental functions (double-double), see softfloat_dd.cpp */
#ifndef SOFTFLOAT_DD_H
#define SOFTFLOAT_DD_H

#include "softfloat.h"

floatx80 floatx80_dd_etox(floatx80 a, float_status *status);
floatx80 floatx80_dd_etoxm1(floatx80 a, float_status *status);
floatx80 floatx80_dd_twotox(floatx80 a, float_status *status);
floatx80 floatx80_dd_tentox(floatx80 a, float_status *status);
floatx80 floatx80_dd_sinh(floatx80 a, float_status *status);
floatx80 floatx80_dd_cosh(floatx80 a, float_status *status);
floatx80 floatx80_dd_tanh(floatx80 a, float_status *status);
floatx80 floatx80_dd_logn(floatx80 a, float_status *status);
floatx80 floatx80_dd_lognp1(floatx80 a, float_status *status);
floatx80 floatx80_dd_log2(floatx80 a, float_status *status);
floatx80 floatx80_dd_log10(floatx80 a, float_status *status);
floatx80 floatx80_dd_atanh(floatx80 a, float_status *status);
floatx80 floatx80_dd_sin(floatx80 a, float_status *status);
floatx80 floatx80_dd_cos(floatx80 a, float_status *status);
floatx80 floatx80_dd_tan(floatx80 a, float_status *status);
floatx80 floatx80_dd_sincos(floatx80 a, floatx80 *c, float_status *status);
floatx80 floatx80_dd_atan(floatx80 a, float_status *status);
floatx80 floatx80_dd_asin(floatx80 a, float_status *status);
floatx80 floatx80_dd_acos(floatx80 a, float_status *status);

#endif
