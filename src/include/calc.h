#ifndef UAE_CALC_H
#define UAE_CALC_H

#include "uae/types.h"

extern int calc(const TCHAR *input, double *outval, TCHAR *outstring, int maxlen);
extern bool iscalcformula(const TCHAR *formula);

#endif /* UAE_CALC_H */
