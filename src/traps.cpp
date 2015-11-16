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
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "td-sdl/thread.h"
#include "autoconf.h"
#include "traps.h"

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
    const char *name;		/* For debugging purposes. */
    uaecptr addr;
};

#define MAX_TRAPS 4096

/* Defined traps */
static struct Trap  traps[MAX_TRAPS];
static unsigned int trap_count;


static const int trace_traps = 0;


static void trap_HandleExtendedTrap (TrapHandler, int has_retval);

uaecptr find_trap (const char *name)
{
    int i;

    for (i = 0; i < trap_count; i++) {
	struct Trap *trap = &traps[i];
	if ((trap->flags & TRAPFLAG_UAERES) && trap->name && !strcmp (trap->name, name))
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
unsigned int define_trap (TrapHandler handler_func, int flags, const char *name)
{
    if (trap_count == MAX_TRAPS) {
	write_log ("Ran out of emulator traps\n");
	abort ();
	return -1;
    } else {
	unsigned int trap_num = trap_count++;
	struct Trap *trap = &traps[trap_num];

	trap->handler = handler_func;
	trap->flags   = flags;
	trap->name    = name;
	trap->addr    = here ();

	return trap_num;
    }
}


/*
 * This function is called by the 68k interpreter to handle an emulator trap.
 *
 * trap_num = number of trap to invoke
 * regs     = current 68k state
 */
void REGPARAM2 m68k_handle_trap (unsigned int trap_num, struct regstruct *regs)
{
    struct Trap *trap = &traps[trap_num];
    uae_u32 retval = 0;

    int has_retval   = (trap->flags & TRAPFLAG_NO_RETVAL) == 0;
    int implicit_rts = (trap->flags & TRAPFLAG_DORET) != 0;

    if (trap->name && trap->name[0] != 0 && trace_traps)
	write_log ("TRAP: %s\n", trap->name);

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
	    retval = (trap->handler) ((TrapContext *)regs);

	    if (has_retval)
		m68k_dreg (regs, 0) = retval;

	    if (implicit_rts) {
		m68k_do_rts (regs);
		fill_prefetch_slow (regs);
	    }
       }

    } else
	write_log ("Illegal emulator trap %d\n", trap_num);
}



/*
 * Implementation of extended traps
 */

typedef struct ExtendedTrapContext
{
    /*
     * Same as simple trap
     */
    struct regstruct  regs;			/* Trap's working copy of 68k state. This is what
						 * the trap handler should access to get arguments
						 * from 68k space. */

    /*
     * Extended trap only
     */
    TrapHandler       trap_handler;		/* Trap handler function that gets called on the trap context */
    int               trap_has_retval;		/* Should the handler return a value to 68k space in D0? */
    uae_u32           trap_retval;		/* Return value from trap handler */


    struct regstruct  saved_regs;                /* Copy of 68k state at trap entry. */


    uae_thread_id     thread;			/* Thread which effects the trap context. */
    uae_sem_t         switch_to_emu_sem;	/* For IPC between the main emulator. */
    uae_sem_t         switch_to_trap_sem;	/* context and the trap context. */


    uaecptr           call68k_func_addr;	/* When calling a 68k function from a trap handler, this
						 * is set to the address of the function to call. */
    uae_u32           call68k_retval;		/* And this gets set to the return value of the 68k call */
} ExtendedTrapContext;


/* 68k addresses which invoke the corresponding traps. */
static uaecptr m68k_call_trapaddr;
static uaecptr m68k_return_trapaddr;
static uaecptr exit_trap_trapaddr;

/* For IPC between main thread and trap context */
static uae_sem_t trap_mutex = 0;
static ExtendedTrapContext *current_context;


/*
 * Thread body for trap context
 */
static void *trap_thread (void *arg)
{
    ExtendedTrapContext *context = (ExtendedTrapContext *) arg;

    uae_set_thread_priority (2);

    /* Wait until main thread is ready to switch to the
     * this trap context. */
    uae_sem_wait (&context->switch_to_trap_sem);

    /* Execute trap handler function. */
    context->trap_retval = context->trap_handler ((TrapContext *)context);

    /* Trap handler is done - we still need to tidy up
     * and make sure the handler's return value is propagated
     * to the calling 68k thread.
     *
     * We do this by causing our exit handler to be executed on the 68k context.
     */

    /* Enter critical section - only one trap at a time, please! */
    uae_sem_wait (&trap_mutex);

    /* Don't allow an interrupt and thus potentially another
     * trap to be invoked while we hold the above mutex.
     * This is probably just being paranoid. */
    context->regs.intmask = 7;

    /* Set PC to address of the exit handler, so that it will be called
     * when the 68k context resumes. */
    m68k_setpc (&context->regs, exit_trap_trapaddr);
    current_context = context;

    /* Switch back to 68k context */
    uae_sem_post (&context->switch_to_emu_sem);

    /* Good bye, cruel world... */

    /* dummy return value */
    write_log("trap_thread: exit (arg=0x%08X)\n", arg);
    return 0;
}

/*
 * Set up extended trap context and call handler function
 */
static void trap_HandleExtendedTrap (TrapHandler handler_func, int has_retval)
{
    struct ExtendedTrapContext *context = (struct ExtendedTrapContext *) calloc (1, sizeof (ExtendedTrapContext));

    if (context) {
	uae_sem_init (&context->switch_to_trap_sem, 0, 0);
	uae_sem_init (&context->switch_to_emu_sem, 0, 0);

	context->trap_handler    = handler_func;
	context->trap_has_retval = has_retval;

	context->regs = regs;       /* Working copy of regs */

	context->saved_regs = regs; /* Copy of regs to be restored when trap is done */

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

	/* Use trap's modified 68k state. This will reset the PC, so that
	 * execution will resume at either the m68k call handler or the
	 * the exit handler. */
	regs = context->regs;
    }
}

/*
 * Call m68k function from an extended trap handler
 *
 * This function is to be called from the trap context.
 */
static uae_u32 trap_Call68k (ExtendedTrapContext *context, uaecptr func_addr)
{
    /* Enter critical section - only one trap at a time, please! */
    uae_sem_wait (&trap_mutex);
    current_context = context;

    /* Don't allow an interrupt and thus potentially another
     * trap to be invoked while we hold the above mutex.
     * This is probably just being paranoid. */
    context->regs.intmask = 7;

    /* Set up function call address. */
    context->call68k_func_addr = func_addr;

    /* Set PC to address of 68k call trap, so that it will be
     * executed when emulator context resumes. */
    m68k_setpc (&context->regs, m68k_call_trapaddr);
    fill_prefetch_slow (&context->regs);

    /* Switch to emulator context. */
    uae_sem_post (&context->switch_to_emu_sem);

    /* Wait for 68k call return handler to switch back to us. */
    uae_sem_wait (&context->switch_to_trap_sem);

    /* End critical section. */
    uae_sem_post (&trap_mutex);

    /* Get return value from 68k function called. */
    return context->call68k_retval;
}

/*
 * Handles the emulator's side of a 68k call (from an extended trap)
 */
static uae_u32 REGPARAM2 m68k_call_handler (struct regstruct *regs)
{
    ExtendedTrapContext *context = current_context;

    uae_u32 sp = m68k_areg (regs, 7);

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
    m68k_setpc (regs, context->call68k_func_addr);
    fill_prefetch_slow (&context->regs);

    /* End critical section: allow other traps run. */
    uae_sem_post (&trap_mutex);

    /* Restore interrupts. */
    regs->intmask = context->saved_regs.intmask;

    /* Dummy return value. */
    return 0;
}

/*
 * Handles the return from a 68k call at the emulator's side.
 */
static uae_u32 REGPARAM2 m68k_return_handler (struct regstruct *regs)
{
    ExtendedTrapContext *context;
    uae_u32 sp;

    /* One trap returning at a time, please! */
    uae_sem_wait (&trap_mutex);

    /* Get trap context from 68k stack. */
    sp = m68k_areg (regs, 7);
    context = (struct ExtendedTrapContext *) get_pointer(sp);
    sp += sizeof (void *);
    m68k_areg (regs, 7) = sp;

    /* Get return value from the 68k call. */
    context->call68k_retval = m68k_dreg (regs, 0);

    /* Update trap's working copy of CPU state. */
    context->regs = *regs;

    /* Switch back to trap context. */
    uae_sem_post (&context->switch_to_trap_sem);

    /* Wait for trap context to switch back to us.
     *
     * It'll do this when the trap handler is done - or when
     * the handler wants to call another 68k function. */
    uae_sem_wait (&context->switch_to_emu_sem);

    /* Use trap's modified 68k state. This will reset the PC, so that
     * execution will resume at either the m68k call handler or the
     * the exit handler. */
    *regs = context->regs;

    /* Dummy return value. */
    return 0;
}

/*
 * Handles completion of an extended trap and passes
 * return value from trap function to 68k space.
 */
static uae_u32 REGPARAM2 exit_trap_handler (struct regstruct *regs)
{
    ExtendedTrapContext *context = current_context;

    /* Wait for trap context thread to exit. */
    uae_wait_thread (context->thread);

    /* Restore 68k state saved at trap entry. */
    *regs = context->saved_regs;

    /* If trap is supposed to return a value, then store
     * return value in D0. */
    if (context->trap_has_retval)
	m68k_dreg (regs, 0) = context->trap_retval;

    uae_sem_destroy (&context->switch_to_trap_sem);
    uae_sem_destroy (&context->switch_to_emu_sem);

    free (context);

    /* End critical section */
    uae_sem_post (&trap_mutex);

    /* Dummy return value. */
    return 0;
}



/*
 * Call a 68k library function from extended trap.
 */
uae_u32 CallLib (TrapContext *context, uaecptr base, uae_s16 offset)
{
    uae_u32 retval;
    uaecptr olda6 = m68k_areg (&context->regs, 6);

    m68k_areg (&context->regs, 6) = base;
    retval = trap_Call68k ((ExtendedTrapContext *)context, base + offset);
    m68k_areg (&context->regs, 6) = olda6;

    return retval;
}

/*
 * Call 68k function from extended trap.
 */
uae_u32 CallFunc (TrapContext *context, uaecptr func)
{
    return trap_Call68k ((ExtendedTrapContext *)context, func);
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
    calltrap (deftrap2 ((TrapHandler)m68k_call_handler, TRAPFLAG_NO_RETVAL, "m68k_call"));

    m68k_return_trapaddr = here();
    calltrap (deftrap2 ((TrapHandler)m68k_return_handler, TRAPFLAG_NO_RETVAL, "m68k_return"));

    exit_trap_trapaddr = here();
    calltrap (deftrap2 ((TrapHandler)exit_trap_handler, TRAPFLAG_NO_RETVAL, "exit_trap"));

    if(trap_mutex != 0)
      uae_sem_destroy(&trap_mutex);
    trap_mutex = 0;
    uae_sem_init (&trap_mutex, 0, 1);
}
