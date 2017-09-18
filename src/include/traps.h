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

#ifndef UAE_TRAPS_H
#define UAE_TRAPS_H

#include "uae/types.h"

#define TRAPCMD_MULTI 0
#define TRAPCMD_PUT_LONG 1
#define TRAPCMD_PUT_WORD 2
#define TRAPCMD_PUT_BYTE 3
#define TRAPCMD_GET_LONG 4
#define TRAPCMD_GET_WORD 5
#define TRAPCMD_GET_BYTE 6
#define TRAPCMD_PUT_BYTES 7
#define TRAPCMD_PUT_WORDS 8
#define TRAPCMD_PUT_LONGS 9
#define TRAPCMD_GET_BYTES 10
#define TRAPCMD_GET_WORDS 11
#define TRAPCMD_GET_LONGS 12
#define TRAPCMD_PUT_STRING 13
#define TRAPCMD_GET_STRING 14
#define TRAPCMD_SET_LONGS 15
#define TRAPCMD_SET_WORDS 16
#define TRAPCMD_SET_BYTES 17
#define TRAPCMD_CALL_LIB 18
#define TRAPCMD_CALL_FUNC 19
#define TRAPCMD_NOP 20
#define TRAPCMD_GET_BSTR 21

struct trapmd {
	uae_u16 cmd;
	uae_u32 params[4];
	uae_u8 trapmd_index, parm_num;
	uae_u8 *haddr;
};

/*
 * Data passed to a trap handler
 */
typedef struct TrapContext TrapContext;


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

#define deftrap(f) define_trap((f), 0, _T(#f))
#define deftrap2(f, mode, str) define_trap((f), (mode), (str))
#define deftrapres(f, mode, str) define_trap((f), (mode | TRAPFLAG_UAERES), (str))

/* New trap system */

bool trap_valid_address(TrapContext *ctx, uaecptr addr, uae_u32 size);

void trap_memcpyha_safe(TrapContext *ctx, uaecptr dst, const uae_u8 *src, int size);
void trap_memcpyah_safe(TrapContext *ctx, uae_u8 *dst, uaecptr src, int size);

uae_u32 trap_get_dreg(TrapContext *ctx, int reg);
uae_u32 trap_get_areg(TrapContext *ctx, int reg);
void trap_set_dreg(TrapContext *ctx, int reg, uae_u32 v);
void trap_set_areg(TrapContext *ctx, int reg, uae_u32 v);

void trap_put_quad(TrapContext *ctx, uaecptr addr, uae_u64 v);
void trap_put_long(TrapContext *ctx, uaecptr addr, uae_u32 v);
void trap_put_word(TrapContext *ctx, uaecptr addr, uae_u16 v);
void trap_put_byte(TrapContext *ctx, uaecptr addr, uae_u8 v);

uae_u64 trap_get_quad(TrapContext *ctx, uaecptr addr);
uae_u32 trap_get_long(TrapContext *ctx, uaecptr addr);
uae_u16 trap_get_word(TrapContext *ctx, uaecptr addr);
uae_u8 trap_get_byte(TrapContext *ctx, uaecptr addr);

void trap_put_bytes(TrapContext *ctx, const void *haddrp, uaecptr addr, int cnt);
void trap_get_bytes(TrapContext *ctx, void *haddrp, uaecptr addr, int cnt);
void trap_put_longs(TrapContext *ctx, uae_u32 *haddr, uaecptr addr, int cnt);
void trap_get_longs(TrapContext *ctx, uae_u32 *haddr, uaecptr addr, int cnt);
void trap_put_words(TrapContext *ctx, uae_u16 *haddr, uaecptr addr, int cnt);
void trap_get_words(TrapContext *ctx, uae_u16 *haddr, uaecptr addr, int cnt);

int trap_put_string(TrapContext *ctx, const void *haddrp, uaecptr addr, int maxlen);
int trap_get_string(TrapContext *ctx, void *haddrp, uaecptr addr, int maxlen);
uae_char *trap_get_alloc_string(TrapContext *ctx, uaecptr addr, int maxlen);

void trap_set_longs(TrapContext *ctx, uaecptr addr, uae_u32 v, int cnt);
void trap_set_words(TrapContext *ctx, uaecptr addr, uae_u16 v, int cnt);
void trap_set_bytes(TrapContext *ctx, uaecptr addr, uae_u8 v, int cnt);

void trap_multi(TrapContext *ctx, struct trapmd *data, int items);

void trap_call_add_dreg(TrapContext *ctx, int reg, uae_u32 v);
void trap_call_add_areg(TrapContext *ctx, int reg, uae_u32 v);
uae_u32 trap_call_lib(TrapContext *ctx, uaecptr base, uae_s16 offset);
uae_u32 trap_call_func(TrapContext *ctx, uaecptr func);

#endif /* UAE_TRAPS_H */
