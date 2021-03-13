#ifndef UAE_MMU_COMMON_H
#define UAE_MMU_COMMON_H

#include "uae/types.h"

struct m68k_exception {
	int prb;
	m68k_exception (int exc) : prb (exc) {}
	operator int() { return prb; }
};
#define TRY(var) try
#define CATCH(var) catch(m68k_exception var)
#define THROW(n) throw m68k_exception(n)
#define ENDTRY

/* special status word (access error stack frame) */
/* 68030 */
#define MMU030_SSW_RW       0x0040
#define MMU030_SSW_SIZE_W       0x0020

#endif /* UAE_MMU_COMMON_H */
