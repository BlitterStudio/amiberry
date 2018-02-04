#ifndef UAE_MMU_COMMON_H
#define UAE_MMU_COMMON_H

#include "uae/types.h"
#include "uae/likely.h"

#ifdef __cplusplus
struct m68k_exception {
	int prb;
	m68k_exception (int exc) : prb (exc) {}
	operator int() { return prb; }
};
#define TRY(var) try
#define CATCH(var) catch(m68k_exception var)
#define THROW(n) throw m68k_exception(n)
#define ENDTRY
#else
/* we are in plain C, just use a stack of long jumps */
#include <setjmp.h>
extern jmp_buf __exbuf;
extern int     __exvalue;
#define TRY(DUMMY)       __exvalue=setjmp(__exbuf);       \
                  if (__exvalue==0) { __pushtry(&__exbuf);
#define CATCH(x)  __poptry(); } else {m68k_exception x=__exvalue; 
#define ENDTRY    __poptry();}
#define THROW(x) if (__is_catched()) {longjmp(__exbuf,x);}
jmp_buf* __poptry(void);
void __pushtry(jmp_buf *j);
int __is_catched(void);

typedef  int m68k_exception;

#endif

#endif /* UAE_MMU_COMMON_H */
