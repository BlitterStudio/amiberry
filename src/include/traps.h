 /*
  * E-UAE - The portable Amiga Emulator
  *
  * Support for traps
  *
  * Copyright Richard Drummond 2005
  *
  * Based on code:
  * Copyright 1995, 1996 Bernd Schmidt
  * Copyright 1996 Ed Hanway
  */

#ifndef TRAPS_H
#define TRAPS_H

/*
 * Data passed to a trap handler
 */
struct TrapContext;


#define TRAPFLAG_NO_REGSAVE  1
#define TRAPFLAG_NO_RETVAL   2
#define TRAPFLAG_EXTRA_STACK 4
#define TRAPFLAG_DORET       8
#define TRAPFLAG_UAERES     16

/*
 * A function which handles a 68k trap
 */
typedef uae_u32 (REGPARAM3 *TrapHandler) (TrapContext *) REGPARAM;


/*
 * Interface with 68k interpreter
 */
extern void REGPARAM3 m68k_handle_trap (unsigned int trap_num) REGPARAM;

unsigned int define_trap (TrapHandler handler_func, int flags, const TCHAR *name);
uaecptr find_trap (const TCHAR *name);

/*
 * Call a 68k Library function from an extended trap
 */
extern uae_u32 CallLib (TrapContext *context, uaecptr library_base, uae_s16 func_offset);
extern uae_u32 CallFunc (TrapContext *context, uaecptr func);

/*
 * Initialization
 */
void init_traps (void);
void init_extended_traps (void);

#define deftrap(f) define_trap((f), 0, _T(""))
#define deftrap2(f, mode, str) define_trap((f), (mode), (str))
#define deftrapres(f, mode, str) define_trap((f), (mode | TRAPFLAG_UAERES), (str))

#endif
