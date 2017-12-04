 /*
  * E-UAE - The portable Amiga Emulator
  *
  * Support for traps
  *
  * Copyright Richard Drummond 2005
  *
  * Inspired by code from UAE:
  * Copyright 1995, 1996 Bernd Schmidt
  * Copyright 1996 Ed Hanway
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "threaddep/thread.h"
#include "autoconf.h"
#include "traps.h"
#include "uae.h"

/*
 * Traps are the mechanism via which 68k code can call emulator code
 * (and for that emulator code in turn to call 68k code). They are
 * thus the basis for much of the cool stuff that E-UAE can do.
 *
 * Emulator traps take advantage of the illegal 68k opwords 0xA000 to
 * 0xAFFF. Normally these would generate an A-line exception. However,
 * when encountered in the RTAREA section of memory, these opwords
 * instead invoke a corresponding emulator trap, allowing a host
 * function to be called.
 *
 * Two types of emulator trap are available - a simple trap and an
 * extended trap. A simple trap may not call 68k code; an extended
 * trap can.
 *
 * Extended traps are rather complex beasts (to implement, not
 * necessarily to use). This is because for the trap handler function
 * to be able to call 68k code, we must somehow allow the emulator's
 * 68k interpreter to resume execution of 68k code in the middle of
 * the trap handler function.
 *
 * In UAE of old this used to be implemented via a stack-swap mechanism.
 * While this worked, it was definitely in the realm of black magic and
 * horribly non-portable, requiring assembly language glue specific to
 * the host ABI and compiler to actually perform the swap.
 *
 * In this implementation, in essence we do something similar - but the
 * new stack is provided by a new thread. No voodoo required, just a
 * working thread layer.
 *
 * The complexity in this approach arises in synchronizing the trap
 * threads with the emulator thread. This implementation errs on the side
 * of paranoia when it comes to thread synchronization. Once all the
 * bugs are knocked out of the bsdsocket emulation, a simpler scheme may
 * suffice.
 */

/*
 * Record of a defined trap (that is, a trap allocated to a host function)
 */
struct Trap
{
  TrapHandler handler;	/* Handler function to be invoked for this trap. */
  int         flags;		/* Trap attributes. */
  const TCHAR *name;		/* For debugging purposes. */
  uaecptr addr;
};

#define MAX_TRAPS 4096

/* Defined traps */
static struct Trap  traps[MAX_TRAPS];
static unsigned int trap_count = 1;

static void trap_HandleExtendedTrap (TrapHandler, int has_retval);

uaecptr find_trap (const TCHAR *name)
{
  int i;

  for (i = 0; i < trap_count; i++) {
  	struct Trap *trap = &traps[i];
  	if ((trap->flags & TRAPFLAG_UAERES) && trap->name && !_tcscmp (trap->name, name))
	    return trap->addr;
  }
  return 0;
}


/*
 * Define an emulator trap
 *
 * handler_func = host function that will be invoked to handle this trap
 * flags        = trap attributes
 * name         = name for debugging purposes
 *
 * returns trap number of defined trap
 */
unsigned int define_trap (TrapHandler handler_func, int flags, const TCHAR *name)
{
  if (trap_count == MAX_TRAPS) {
		write_log (_T("Ran out of emulator traps\n"));
    target_startup_msg(_T("Internal error"), _T("Ran out of emulator traps."));
    uae_restart(1, NULL);
  	return -1;
  } else {
  	int i;
  	unsigned int trap_num;
  	struct Trap *trap;
  	uaecptr addr = here ();

  	for (i = 0; i < trap_count; i++) {
	    if (addr == traps[i].addr)
    		return i;
  	}

  	trap_num = trap_count++;
  	trap = &traps[trap_num];

  	trap->handler = handler_func;
  	trap->flags   = flags;
  	trap->name    = name;
  	trap->addr    = addr;

  	return trap_num;
  }
}


/*
 * This function is called by the 68k interpreter to handle an emulator trap.
 *
 * trap_num = number of trap to invoke
 * regs     = current 68k state
 */
void REGPARAM2 m68k_handle_trap (unsigned int trap_num)
{
  struct Trap *trap = &traps[trap_num];
  uae_u32 retval = 0;

  int has_retval   = (trap->flags & TRAPFLAG_NO_RETVAL) == 0;
  int implicit_rts = (trap->flags & TRAPFLAG_DORET) != 0;

  if (trap_num < trap_count) {
  	if (trap->flags & TRAPFLAG_EXTRA_STACK) {
	    /* Handle an extended trap.
	     * Note: the return value of this trap is passed back to 68k
	     * space via a separate, dedicated simple trap which the trap
	     * handler causes to be invoked when it is done.
	     */
	    trap_HandleExtendedTrap (trap->handler, has_retval);
  	} else {
	    /* Handle simple trap */
	    retval = (trap->handler) (NULL);

	    if (has_retval)
    		m68k_dreg (regs, 0) = retval;

	    if (implicit_rts) {
    		m68k_do_rts ();
    		fill_prefetch ();
	    }
    }
  } else
		write_log (_T("Illegal emulator trap\n"));
}



/*
 * Implementation of extended traps
 */

struct TrapCPUContext
{
	uae_u32 regs[16];
	uae_u32 pc;
	int intmask;
};

struct TrapContext
{
  /* Trap's working copy of 68k state. This is what the trap handler should
  *  access to get arguments from 68k space. */

  /* Trap handler function that gets called on the trap context */
  TrapHandler       trap_handler;
  /* Should the handler return a value to 68k space in D0? */
  int               trap_has_retval;
  /* Return value from trap handler */
  uae_u32           trap_retval;

  /* Copy of 68k state at trap entry. */
	struct TrapCPUContext saved_regs;

  /* Thread which effects the trap context. */
  uae_thread_id     thread;
  /* For IPC between the main emulator. */
  uae_sem_t         switch_to_emu_sem;
  /* context and the trap context. */
  uae_sem_t         switch_to_trap_sem;

  /* When calling a 68k function from a trap handler, this is set to the
  *  address of the function to call. */
  uaecptr           call68k_func_addr;
  /* And this gets set to the return value of the 68k call. */
  uae_u32           call68k_retval;

	uae_u32 calllib_regs[16];
	uae_u8 calllib_reg_inuse[16];
};

static void copytocpucontext(struct TrapCPUContext *cpu)
{
	memcpy (cpu->regs, regs.regs, sizeof (regs.regs));
	cpu->intmask = regs.intmask;
	cpu->pc = m68k_getpc ();
}
static void copyfromcpucontext(struct TrapCPUContext *cpu, uae_u32 pc)
{
	memcpy (regs.regs, cpu->regs, sizeof (regs.regs));
	regs.intmask = cpu->intmask;
	m68k_setpc (pc);
}


/* 68k addresses which invoke the corresponding traps. */
static uaecptr m68k_call_trapaddr;
static uaecptr m68k_return_trapaddr;
static uaecptr exit_trap_trapaddr;

/* For IPC between main thread and trap context */
static uae_sem_t trap_mutex = 0;
static TrapContext *current_context;


/*
 * Thread body for trap context
 */
static void *trap_thread (void *arg)
{
  TrapContext *context = (TrapContext *) arg;

  /* Wait until main thread is ready to switch to the
   * this trap context. */
  uae_sem_wait (&context->switch_to_trap_sem);

  /* Execute trap handler function. */
  context->trap_retval = context->trap_handler (context);

  /* Trap handler is done - we still need to tidy up
   * and make sure the handler's return value is propagated
   * to the calling 68k thread.
   *
   * We do this by causing our exit handler to be executed on the 68k context.
   */

  /* Enter critical section - only one trap at a time, please! */
  uae_sem_wait (&trap_mutex);

	//regs = context->saved_regs;
	/* Set PC to address of the exit handler, so that it will be called
	* when the 68k context resumes. */
	copyfromcpucontext (&context->saved_regs, exit_trap_trapaddr);
  /* Don't allow an interrupt and thus potentially another
   * trap to be invoked while we hold the above mutex.
   * This is probably just being paranoid. */
  regs.intmask = 7;

	//m68k_setpc (exit_trap_trapaddr);
  current_context = context;

  /* Switch back to 68k context */
  uae_sem_post (&context->switch_to_emu_sem);

  /* Good bye, cruel world... */

  /* dummy return value */
  return 0;
}

/*
 * Set up extended trap context and call handler function
 */
static void trap_HandleExtendedTrap (TrapHandler handler_func, int has_retval)
{
  struct TrapContext *context = xcalloc (TrapContext, 1);

  if (context) {
	  uae_sem_init (&context->switch_to_trap_sem, 0, 0);
	  uae_sem_init (&context->switch_to_emu_sem, 0, 0);

	  context->trap_handler    = handler_func;
	  context->trap_has_retval = has_retval;

		//context->saved_regs = regs;
		copytocpucontext (&context->saved_regs);

	  /* Start thread to handle new trap context. */
	  uae_start_thread_fast (trap_thread, (void *)context, &context->thread);

	  /* Switch to trap context to begin execution of
	   * trap handler function.
	   */
	  uae_sem_post (&context->switch_to_trap_sem);

	  /* Wait for trap context to switch back to us.
	   *
	   * It'll do this when the trap handler is done - or when
	   * the handler wants to call 68k code. */
	  uae_sem_wait (&context->switch_to_emu_sem);
  }
}

/*
 * Call m68k function from an extended trap handler
 *
 * This function is to be called from the trap context.
 */

static uae_u32 trap_Call68k (TrapContext *ctx, uaecptr func_addr)
{
  /* Enter critical section - only one trap at a time, please! */
  uae_sem_wait (&trap_mutex);
  current_context = ctx;

  /* Don't allow an interrupt and thus potentially another
   * trap to be invoked while we hold the above mutex.
   * This is probably just being paranoid. */
  regs.intmask = 7;

  /* Set up function call address. */
  ctx->call68k_func_addr = func_addr;

  /* Set PC to address of 68k call trap, so that it will be
   * executed when emulator context resumes. */
  m68k_setpc (m68k_call_trapaddr);
  fill_prefetch ();

  /* Switch to emulator context. */
  uae_sem_post (&ctx->switch_to_emu_sem);

  /* Wait for 68k call return handler to switch back to us. */
  uae_sem_wait (&ctx->switch_to_trap_sem);

  /* End critical section. */
  uae_sem_post (&trap_mutex);

  /* Get return value from 68k function called. */
  return ctx->call68k_retval;
}

/*
 * Handles the emulator's side of a 68k call (from an extended trap)
 */
static uae_u32 REGPARAM2 m68k_call_handler (TrapContext *dummy_ctx)
{
  TrapContext *context = current_context;

  uae_u32 sp;

  sp = m68k_areg (regs, 7);

  /* Push address of trap context on 68k stack. This is
   * so the return trap can find this context. */
  sp -= sizeof (void *);
  put_pointer (sp, context);

  /* Push addr to return handler trap on 68k stack.
   * When the called m68k function does an RTS, the CPU will pull this
   * address off the stack and so call the return handler. */
  sp -= 4;
  put_long (sp, m68k_return_trapaddr);

  m68k_areg (regs, 7) = sp;

  /* Set PC to address of 68k function to call. */
  m68k_setpc (context->call68k_func_addr);
  fill_prefetch ();

  /* End critical section: allow other traps run. */
  uae_sem_post (&trap_mutex);

  /* Restore interrupts. */
  regs.intmask = context->saved_regs.intmask;

  /* Dummy return value. */
  return 0;
}

/*
 * Handles the return from a 68k call at the emulator's side.
 */
static uae_u32 REGPARAM2 m68k_return_handler (TrapContext *dummy_ctx)
{
  TrapContext *context;
  uae_u32 sp;

  /* One trap returning at a time, please! */
  uae_sem_wait (&trap_mutex);

  /* Get trap context from 68k stack. */
  sp = m68k_areg (regs, 7);
  context = (TrapContext *) get_pointer(sp);
  sp += sizeof (void *);
  m68k_areg (regs, 7) = sp;

  /* Get return value from the 68k call. */
  context->call68k_retval = m68k_dreg (regs, 0);

  /* Switch back to trap context. */
  uae_sem_post (&context->switch_to_trap_sem);

  /* Wait for trap context to switch back to us.
   *
   * It'll do this when the trap handler is done - or when
   * the handler wants to call another 68k function. */
  uae_sem_wait (&context->switch_to_emu_sem);

  /* Dummy return value. */
  return 0;
}

/*
 * Handles completion of an extended trap and passes
 * return value from trap function to 68k space.
 */
static uae_u32 REGPARAM2 exit_trap_handler (TrapContext *dummy_ctx)
{
  TrapContext *context = current_context;

  /* Wait for trap context thread to exit. */
  uae_wait_thread (context->thread);

  /* Restore 68k state saved at trap entry. */
	//regs = context->saved_regs;
	copyfromcpucontext (&context->saved_regs, context->saved_regs.pc);
  
  /* If trap is supposed to return a value, then store
   * return value in D0. */
  if (context->trap_has_retval)
  	m68k_dreg (regs, 0) = context->trap_retval;

  uae_sem_destroy (&context->switch_to_trap_sem);
  uae_sem_destroy (&context->switch_to_emu_sem);

  xfree (context);

  /* End critical section */
  uae_sem_post (&trap_mutex);

  /* Dummy return value. */
  return 0;
}



/*
 * Call a 68k library function from extended trap.
 */
uae_u32 CallLib (TrapContext *ctx, uaecptr base, uae_s16 offset)
{
  uae_u32 retval;
	uaecptr olda6 = trap_get_areg(ctx, 6);

	trap_set_areg(ctx, 6, base);
  retval = trap_Call68k (ctx, base + offset);
	trap_set_areg(ctx, 6, olda6);

  return retval;
}

/*
 * Call 68k function from extended trap.
 */
uae_u32 CallFunc(TrapContext *ctx, uaecptr func)
{
	return trap_Call68k(ctx, func);
}


/*
 * Initialize trap mechanism.
 */
void init_traps (void)
{
  trap_count = 0;
}

/*
 * Initialize the extended trap mechanism.
 */
void init_extended_traps (void)
{
  m68k_call_trapaddr = here ();
  calltrap (deftrap2 (m68k_call_handler, TRAPFLAG_NO_RETVAL, _T("m68k_call")));

  m68k_return_trapaddr = here();
  calltrap (deftrap2 (m68k_return_handler, TRAPFLAG_NO_RETVAL, _T("m68k_return")));

  exit_trap_trapaddr = here();
  calltrap (deftrap2 (exit_trap_handler, TRAPFLAG_NO_RETVAL, _T("exit_trap")));

  if(trap_mutex != 0)
    uae_sem_destroy(&trap_mutex);
  trap_mutex = 0;
  uae_sem_init (&trap_mutex, 0, 1);
}

void trap_call_add_dreg(TrapContext *ctx, int reg, uae_u32 v)
{
	ctx->calllib_reg_inuse[reg] = 1;
	ctx->calllib_regs[reg] = v;
}
void trap_call_add_areg(TrapContext *ctx, int reg, uae_u32 v)
{
	ctx->calllib_reg_inuse[reg + 8] = 1;
	ctx->calllib_regs[reg + 8] = v;
}
uae_u32 trap_call_lib(TrapContext *ctx, uaecptr base, uae_s16 offset)
{
	uae_u32 v;
	uae_u32 storedregs[16];
	bool storedregsused[16];
	for (int i = 0; i < 16; i++) {
		storedregsused[i] = false;
		if (ctx->calllib_reg_inuse[i]) {
			if ((i & 7) >= 2) {
				storedregsused[i] = true;
				storedregs[i] = regs.regs[i];
			}
			regs.regs[i] = ctx->calllib_regs[i];
		}
		ctx->calllib_reg_inuse[i] = 0;
	}
	v = CallLib(ctx, base, offset);
	for (int i = 0; i < 16; i++) {
		if (storedregsused[i]) {
			regs.regs[i] = storedregs[i];
		}
	}
	return v;
}
uae_u32 trap_call_func(TrapContext *ctx, uaecptr func)
{
	uae_u32 v;
	uae_u32 storedregs[16];
	bool storedregsused[16];
	for (int i = 0; i < 16; i++) {
		storedregsused[i] = false;
		if (ctx->calllib_reg_inuse[i]) {
			if ((i & 7) >= 2) {
				storedregsused[i] = true;
				storedregs[i] = regs.regs[i];
			}
			regs.regs[i] = ctx->calllib_regs[i];
		}
		ctx->calllib_reg_inuse[i] = 0;
	}
	v = CallFunc(ctx, func);
	for (int i = 0; i < 16; i++) {
		if (storedregsused[i]) {
			regs.regs[i] = storedregs[i];
		}
	}
	return v;
}


bool trap_valid_address(TrapContext *ctx, uaecptr addr, uae_u32 size)
{
	return valid_address(addr, size) != 0;
}

uae_u32 trap_get_dreg(TrapContext *ctx, int reg)
{
	return m68k_dreg(regs, reg);
}
uae_u32 trap_get_areg(TrapContext *ctx, int reg)
{
	return m68k_areg(regs, reg);
}
void trap_set_dreg(TrapContext *ctx, int reg, uae_u32 v)
{
	m68k_dreg(regs, reg) = v;
}
void trap_set_areg(TrapContext *ctx, int reg, uae_u32 v)
{
	m68k_areg(regs, reg) = v;
}
void trap_put_quad(TrapContext *ctx, uaecptr addr, uae_u64 v)
{
	uae_u8 out[8];
	put_long_host(out + 0, v >> 32);
	put_long_host(out + 4, (uae_u32)(v >> 0));
	trap_put_bytes(ctx, out, addr, 8);
}
void trap_put_long(TrapContext *ctx, uaecptr addr, uae_u32 v)
{
	put_long(addr, v);
}
void trap_put_word(TrapContext *ctx, uaecptr addr, uae_u16 v)
{
	put_word(addr, v);
}
void trap_put_byte(TrapContext *ctx, uaecptr addr, uae_u8 v)
{
	put_byte(addr, v);
}

uae_u64 trap_get_quad(TrapContext *ctx, uaecptr addr)
{
	uae_u8 in[8];
	trap_get_bytes(ctx, in, addr, 8);
	return ((uae_u64)get_long_host(in + 0) << 32) | get_long_host(in + 4);
}
uae_u32 trap_get_long(TrapContext *ctx, uaecptr addr)
{
	return get_long(addr);
}
uae_u16 trap_get_word(TrapContext *ctx, uaecptr addr)
{
	return get_word(addr);
}
uae_u8 trap_get_byte(TrapContext *ctx, uaecptr addr)
{
	return get_byte(addr);
}

void trap_put_bytes(TrapContext *ctx, const void *haddrp, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u8 *haddr = (uae_u8*)haddrp;
	if (valid_address(addr, cnt)) {
		memcpy(get_real_address(addr), haddr, cnt);
	} else {
		for (int i = 0; i < cnt; i++) {
			put_byte(addr, *haddr++);
			addr++;
		}
	}
}
void trap_get_bytes(TrapContext *ctx, void *haddrp, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u8 *haddr = (uae_u8*)haddrp;
	if (valid_address(addr, cnt)) {
		memcpy(haddr, get_real_address(addr), cnt);
	} else {
		for (int i = 0; i < cnt; i++) {
			*haddr++ = get_byte(addr);
			addr++;
		}
	}
}
void trap_put_longs(TrapContext *ctx, uae_u32 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u32 *p = (uae_u32*)haddr;
	for (int i = 0; i < cnt; i++) {
		put_long(addr, *p++);
		addr += 4;
	}
}
void trap_get_longs(TrapContext *ctx, uae_u32 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u32 *p = (uae_u32*)haddr;
	for (int i = 0; i < cnt; i++) {
		*p++ = get_long(addr);
		addr += 4;
	}
}
void trap_put_words(TrapContext *ctx, uae_u16 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u16 *p = (uae_u16*)haddr;
	for (int i = 0; i < cnt; i++) {
		put_word(addr, *p++);
		addr += sizeof(uae_u16);
	}
}
void trap_get_words(TrapContext *ctx, uae_u16 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u16 *p = (uae_u16*)haddr;
	for (int i = 0; i < cnt; i++) {
		*p++ = get_word(addr);
		addr += sizeof(uae_u16);
	}
}

int trap_put_string(TrapContext *ctx, const void *haddrp, uaecptr addr, int maxlen)
{
	int len = 0;
	uae_u8 *haddr = (uae_u8*)haddrp;
	for (;;) {
		uae_u8 v = *haddr++;
		put_byte(addr, v);
		addr++;
		if (!v)
			break;
		len++;
	}
	return len;
}
int trap_get_string(TrapContext *ctx, void *haddrp, uaecptr addr, int maxlen)
{
	int len = 0;
	uae_u8 *haddr = (uae_u8*)haddrp;
	for (;;) {
		uae_u8 v = get_byte(addr);
		*haddr++ = v;
		addr++;
		if (!v)
			break;
	}
	len++;
	return len;
}
uae_char *trap_get_alloc_string(TrapContext *ctx, uaecptr addr, int maxlen)
{
	uae_char *buf = xmalloc(uae_char, maxlen);
	trap_get_string(ctx, buf, addr, maxlen);
	return buf;
}

int trap_get_bstr(TrapContext *ctx, uae_u8 *haddr, uaecptr addr, int maxlen)
{
	int len = 0;
	uae_u8 cnt = get_byte(addr);
	while (cnt-- != 0 && maxlen-- > 0) {
		addr++;
		*haddr++ = get_byte(addr);
	}
	*haddr = 0;
	return len;
}

void trap_set_longs(TrapContext *ctx, uaecptr addr, uae_u32 v, int cnt)
{
	if (cnt <= 0)
		return;
	for (int i = 0; i < cnt; i++) {
		put_long(addr, v);
		addr += 4;
	}
}
void trap_set_words(TrapContext *ctx, uaecptr addr, uae_u16 v, int cnt)
{
	if (cnt <= 0)
		return;
	for (int i = 0; i < cnt; i++) {
		put_word(addr, v);
		addr += 2;
	}
}
void trap_set_bytes(TrapContext *ctx, uaecptr addr, uae_u8 v, int cnt)
{
	if (cnt <= 0)
		return;
	for (int i = 0; i < cnt; i++) {
		put_byte(addr, v);
		addr += 1;
	}
}

void trap_multi(TrapContext *ctx, struct trapmd *data, int items)
{
	uae_u32 v = 0;
	for (int i = 0; i < items; i++) {
		struct trapmd *md = &data[i];
		switch (md->cmd)
		{
			case TRAPCMD_PUT_LONG:
			trap_put_long(ctx, md->params[0], md->params[1]);
			break;
			case TRAPCMD_PUT_WORD:
			trap_put_word(ctx, md->params[0], md->params[1]);
			break;
			case TRAPCMD_PUT_BYTE:
			trap_put_byte(ctx, md->params[0], md->params[1]);
			break;
			case TRAPCMD_GET_LONG:
			v = md->params[0] = trap_get_long(ctx, md->params[0]);
			break;
			case TRAPCMD_GET_WORD:
			v = md->params[0] = trap_get_word(ctx, md->params[0]);
			break;
			case TRAPCMD_GET_BYTE:
			v = md->params[0] = trap_get_byte(ctx, md->params[0]);
			break;
			case TRAPCMD_PUT_BYTES:
			trap_put_bytes(ctx, md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_GET_BYTES:
			trap_get_bytes(ctx, md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_PUT_WORDS:
			trap_put_words(ctx, (uae_u16*)md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_GET_WORDS:
			trap_get_words(ctx, (uae_u16*)md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_PUT_LONGS:
			trap_put_longs(ctx, (uae_u32*)md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_GET_LONGS:
			trap_get_longs(ctx, (uae_u32*)md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_PUT_STRING:
			trap_put_string(ctx, md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_GET_STRING:
			trap_get_string(ctx, md->haddr, md->params[0], md->params[1]);
			break;
			case TRAPCMD_SET_LONGS:
			trap_set_longs(ctx, md->params[0], md->params[1], md->params[2]);
			break;
			case TRAPCMD_SET_WORDS:
			trap_set_words(ctx, md->params[0], md->params[1], md->params[2]);
			break;
			case TRAPCMD_SET_BYTES:
			trap_set_bytes(ctx, md->params[0], md->params[1], md->params[2]);
			break;
			case TRAPCMD_NOP:
			break;
		}
		if (md->trapmd_index) {
			data[md->trapmd_index].params[md->parm_num] = v;
		}
	}
}

void trap_memcpyha_safe(TrapContext *ctx, uaecptr dst, const uae_u8 *src, int size)
{
	if (size <= 0)
		return;
	memcpyha_safe(dst, src, size);
}
void trap_memcpyah_safe(TrapContext *ctx, uae_u8 *dst, uaecptr src, int size)
{
	if (size <= 0)
		return;
	memcpyah_safe(dst, src, size);
}
