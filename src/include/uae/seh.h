/*
 * Structured exception handling
 * Copyright (C) 2014 Frode Solheim
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#ifndef UAE_SEH_H
#define UAE_SEH_H

#ifdef _MSC_VER

/* Structured exception handling is available */

#else

/* MinGW defines __try / __except */
#undef __try
#undef __except

/* Structured exception handling is not available - do nothing */
#define __try
#define __except(x)

#endif

#endif /* UAE_SEH_H */
