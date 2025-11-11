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

#define NEW_TRAP_DEBUG 0

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "threaddep/thread.h"
#include "autoconf.h"
#include "traps.h"
#include "uae.h"
#include "debug.h"

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
static volatile int hardware_trap_state[MAX_TRAPS];

volatile uae_atomic hwtrap_waiting;

static const int trace_traps = 0;

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
		abort ();
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

	int has_retval = (trap->flags & TRAPFLAG_NO_RETVAL) == 0;
	int implicit_rts = (trap->flags & TRAPFLAG_DORET) != 0;

	if (trap_num < trap_count) {
		if (trap->flags & TRAPFLAG_EXTRA_STACK) {
			/* Handle an extended trap.
			* Note: the return value of this trap is passed back to 68k
			* space via a separate, dedicated simple trap which the trap
			* handler causes to be invoked when it is done.
			*/

			if (trap->name && trap->name[0] != 0 && trace_traps)
				write_log(_T("XTRAP: %s\n"), trap->name);

			trap_HandleExtendedTrap (trap->handler, has_retval);
		} else {
			/* Handle simple trap */

			if (trap->name && trap->name[0] != 0 && trace_traps)
				write_log(_T("TRAP: %s\n"), trap->name);

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
	* access to get arguments from 68k space. */

	/* Trap handler function that gets called on the trap context */
	TrapHandler trap_handler;
	/* Should the handler return a value to 68k space in D0? */
	int trap_has_retval;
	/* Return value from trap handler */
	uae_u32 trap_retval;

	/* Copy of 68k state at trap entry. */
	struct TrapCPUContext saved_regs;

	/* Thread which effects the trap context. */
	uae_thread_id thread;
	/* For IPC between the main emulator. */
	uae_sem_t switch_to_emu_sem;
	/* context and the trap context. */
	uae_sem_t switch_to_trap_sem;

	/* When calling a 68k function from a trap handler, this is set to the
	* address of the function to call.  */
	uaecptr call68k_func_addr;
	/* And this gets set to the return value of the 68k call.  */
	uae_u32 call68k_retval;

	/* new stuff */
	uae_u8 *host_trap_data;
	uae_u8 *host_trap_status;
	uaecptr amiga_trap_data;
	uaecptr amiga_trap_status;
	/* do not ack automatically */
	volatile uae_atomic trap_background;
	volatile bool trap_done;
	uae_u32 calllib_regs[16];
	uae_u8 calllib_reg_inuse[16];
	size_t tindex;
	int tcnt;
	TRAP_CALLBACK callback;
	void *callback_ud;
	int trap_mode;
	int trap_slot;
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
static uae_sem_t trap_mutex;
static TrapContext *current_context;


/*
* Thread body for trap context
*/
static int trap_thread (void *arg)
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
static void trap_HandleExtendedTrap(TrapHandler handler_func, int has_retval)
{
	struct TrapContext *context = xcalloc(TrapContext, 1);

	if (context) {
		uae_sem_init(&context->switch_to_trap_sem, 0, 0);
		uae_sem_init(&context->switch_to_emu_sem, 0, 0);

		context->trap_handler = handler_func;
		context->trap_has_retval = has_retval;

		//context->saved_regs = regs;
		copytocpucontext(&context->saved_regs);

		/* Start thread to handle new trap context. */
		uae_start_thread_fast(trap_thread, (void *)context, &context->thread);

		/* Switch to trap context to begin execution of
		* trap handler function.
		*/
		uae_sem_post(&context->switch_to_trap_sem);

		/* Wait for trap context to switch back to us.
		*
		* It'll do this when the trap handler is done - or when
		* the handler wants to call 68k code. */
		uae_sem_wait(&context->switch_to_emu_sem);

		if (trace_traps) {
			write_log(_T("Exit extended trap PC=%08x\n"), m68k_getpc());
		}
	}
}

/*
* Call m68k function from an extended trap handler
*
* This function is to be called from the trap context.
*/

static uae_u32 call_hardware_trap_back(TrapContext *ctx, uae_u16 cmd, uae_u32 p1, uae_u32 p2, uae_u32 p3, uae_u32 p4);

static uae_u32 trap_Call68k(TrapContext *ctx, uaecptr func_addr)
{
	if (ctx->host_trap_data) {

		return call_hardware_trap_back(ctx, 10, func_addr, 0, 0, 0);

	} else {

		/* Enter critical section - only one trap at a time, please! */
		uae_sem_wait(&trap_mutex);
		current_context = ctx;

		/* Don't allow an interrupt and thus potentially another
		* trap to be invoked while we hold the above mutex.
		* This is probably just being paranoid. */
		regs.intmask = 7;

		/* Set up function call address. */
		ctx->call68k_func_addr = func_addr;

		/* Set PC to address of 68k call trap, so that it will be
		* executed when emulator context resumes. */
		m68k_setpc(m68k_call_trapaddr);
		fill_prefetch();

		if (trace_traps) {
			write_log(_T("Calling m68k PC=%08x %08x\n"), func_addr, m68k_call_trapaddr);
		}

		/* Switch to emulator context. */
		uae_sem_post(&ctx->switch_to_emu_sem);

		/* Wait for 68k call return handler to switch back to us. */
		uae_sem_wait(&ctx->switch_to_trap_sem);

		/* End critical section. */
		uae_sem_post(&trap_mutex);

		/* Get return value from 68k function called. */
		return ctx->call68k_retval;
	}
}

/*
* Handles the emulator's side of a 68k call (from an extended trap)
*/
static uae_u32 REGPARAM2 m68k_call_handler(TrapContext *dummy_ctx)
{
	TrapContext *context = current_context;

	uae_u32 sp;

	sp = m68k_areg(regs, 7);

	/* Push address of trap context on 68k stack. This is
	* so the return trap can find this context. */
	sp -= sizeof(void *);
	put_pointer(sp, context);

	/* Push addr to return handler trap on 68k stack.
	* When the called m68k function does an RTS, the CPU will pull this
	* address off the stack and so call the return handler. */
	sp -= 4;
	put_long(sp, m68k_return_trapaddr);

	m68k_areg(regs, 7) = sp;

	/* Set PC to address of 68k function to call. */
	m68k_setpc(context->call68k_func_addr);
	fill_prefetch();

	/* Restore interrupts. */
	regs.intmask = context->saved_regs.intmask;

	/* End critical section: allow other traps run. */
	uae_sem_post(&trap_mutex);

	/* Dummy return value. */
	return 0;
}

/*
* Handles the return from a 68k call at the emulator's side.
*/
static uae_u32 REGPARAM2 m68k_return_handler(TrapContext *dummy_ctx)
{
	TrapContext *context;
	uae_u32 sp;

	if (trace_traps) {
		write_log(_T("m68k_return_handler\n"));
	}


	/* One trap returning at a time, please! */
	uae_sem_wait(&trap_mutex);

	/* Get trap context from 68k stack. */
	sp = m68k_areg(regs, 7);
	context = (TrapContext *)get_pointer(sp);
	sp += sizeof(void *);
	m68k_areg(regs, 7) = sp;

	/* Get return value from the 68k call. */
	context->call68k_retval = m68k_dreg(regs, 0);

	/* Switch back to trap context. */
	uae_sem_post(&context->switch_to_trap_sem);

	/* Wait for trap context to switch back to us.
	*
	* It'll do this when the trap handler is done - or when
	* the handler wants to call another 68k function. */
	uae_sem_wait(&context->switch_to_emu_sem);

	/* Dummy return value. */
	return 0;
}

/*
* Handles completion of an extended trap and passes
* return value from trap function to 68k space.
*/
static uae_u32 REGPARAM2 exit_trap_handler(TrapContext *dummy_ctx)
{
	TrapContext *context = current_context;

	if (trace_traps) {
		write_log(_T("exit_trap_handler waiting PC=%08x\n"), context->saved_regs.pc);
	}

	/* Wait for trap context thread to exit. */
	uae_wait_thread(&context->thread);

	/* Restore 68k state saved at trap entry. */
	//regs = context->saved_regs;
	copyfromcpucontext(&context->saved_regs, context->saved_regs.pc);

	/* If trap is supposed to return a value, then store
	* return value in D0. */
	if (context->trap_has_retval)
		m68k_dreg(regs, 0) = context->trap_retval;

	uae_sem_destroy(&context->switch_to_trap_sem);
	uae_sem_destroy(&context->switch_to_emu_sem);

	xfree(context);

	/* End critical section */
	uae_sem_post(&trap_mutex);

	/* Dummy return value. */
	return 0;
}



/*
* Call a 68k library function from extended trap.
*/
uae_u32 CallLib(TrapContext *ctx, uaecptr base, uae_s16 offset)
{
	uae_u32 retval;
	uaecptr olda6 = trap_get_areg(ctx, 6);

	trap_set_areg(ctx, 6, base);
	retval = trap_Call68k(ctx, base + offset);
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


/* UAE board traps */

#define TRAP_THREADS (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM)
static smp_comm_pipe trap_thread_pipe[TRAP_THREADS];
static uae_thread_id trap_thread_id[TRAP_THREADS];
static volatile uae_atomic trap_thread_index;
static volatile int hardware_trap_kill[TRAP_THREADS];
static volatile bool trap_in_use[TRAP_THREADS];
static volatile bool trap_in_use2[TRAP_THREADS];
static volatile int trap_cnt;
volatile int trap_mode;

static void hardware_trap_ack(TrapContext *ctx)
{
	uae_u8 *data = ctx->host_trap_data;
	uae_u8 *status = ctx->host_trap_status;
	uaecptr addr = ctx->amiga_trap_status;

	// ack call_hardware_trap
	uaecptr task = get_long_host(data + RTAREA_TRAP_DATA_TASKWAIT);

#if NEW_TRAP_DEBUG
	write_log(_T("TRAP SLOT %d: ACK. TASK = %08x\n"), ctx->trap_slot, task);
#endif

	put_byte_host(status + 2, 0xff);
	if (task && trap_mode == 1) {
		atomic_or(&uae_int_requested, 0x4000);
		set_special_exter(SPCFLAG_UAEINT);
	}
	if (!trap_in_use[ctx->trap_slot])
		write_log(_T("TRAP SLOT %d ACK WIIHOUT ALLOCATION!\n"));
	trap_in_use[ctx->trap_slot] = false;
	xfree(ctx);
}

static int hardware_trap_thread(void *arg)
{
	size_t tid = (size_t)arg;
	for (;;) {
		TrapContext *ctx = (TrapContext*)read_comm_pipe_pvoid_blocking(&trap_thread_pipe[tid]);
		if (!ctx) {
			if (!hardware_trap_state[tid]) {
				break;
			}
			while (comm_pipe_has_data(&trap_thread_pipe[tid])) {
				read_comm_pipe_pvoid_blocking(&trap_thread_pipe[tid]);
			}
			hardware_trap_state[tid] = 0;
			continue;
		}

		if (trap_in_use[ctx->trap_slot]) {
			write_log(_T("TRAP SLOT %d ALREADY IN USE!\n"));
		}
		trap_in_use[ctx->trap_slot] = true;

		uae_u8 *data = ctx->host_trap_data;
		uae_u8 *status = ctx->host_trap_status;
		ctx->tindex = tid;
		ctx->tcnt = ++trap_cnt;

		for (int i = 0; i < 16; i++) {
			uae_u32 v = get_long_host(data + 4 + i * 4);
			ctx->saved_regs.regs[i] = v;
		}
		put_long_host(status + RTAREA_TRAP_STATUS_SECOND, 0);

#if NEW_TRAP_DEBUG
		//if (_tcscmp(trap->name, _T("exter_int_helper")))
			write_log(_T("TRAP SLOT %d: NUM %d\n"), ctx->trap_slot, trap_num);
#endif

		uae_u32 ret;
		struct Trap *trap = NULL;

		if (ctx->trap_done)
			write_log(_T("hardware_trap_thread #1: trap_done set!"));

		if (ctx->callback) {
			ret = ctx->callback(ctx, ctx->callback_ud);
		} else {
			int trap_num = get_word_host(status);
			trap = &traps[trap_num];
			ret = trap->handler(ctx);
		}

		if (ctx->trap_done)
			write_log(_T("hardware_trap_thread #2: trap_done set!"));

		if (!ctx->trap_background) {
			for (int i = 0; i < 15; i++) {
				uae_u32 v = ctx->saved_regs.regs[i];
				put_long_host(data + 4 + i * 4, v);
			}
			if (trap && (trap->flags & TRAPFLAG_NO_RETVAL) == 0) {
				put_long_host(data + 4, ret);
			}
			hardware_trap_ack(ctx);
		} else {
			ctx->trap_done = true;
		}
	}
	hardware_trap_kill[tid] = -1;
	return 0;
}

void trap_background_set_complete(TrapContext *ctx)
{
	if (!trap_is_indirect())
		return;
	if (!ctx || !ctx->trap_background)
		return;
	atomic_dec(&ctx->trap_background);
	if (!ctx->trap_background) {
		if (!ctx->trap_done) {
			write_log(_T("trap_background_set_complete(%d) still waiting!?\n"), ctx->tindex);
			while (!ctx->trap_done);
		}
		hardware_trap_ack(ctx);
	}
}

void trap_dos_active(void)
{
	trap_mode = 0;
}

void call_hardware_trap(uae_u8 *host_base, uaecptr amiga_base, int slot)
{
	TrapContext *ctx = xcalloc(TrapContext, 1);
	ctx->host_trap_data = host_base + RTAREA_TRAP_DATA + slot * RTAREA_TRAP_DATA_SLOT_SIZE;
	ctx->amiga_trap_data = amiga_base + RTAREA_TRAP_DATA + slot * RTAREA_TRAP_DATA_SLOT_SIZE;
	ctx->host_trap_status = host_base + RTAREA_TRAP_STATUS + slot * RTAREA_TRAP_STATUS_SIZE;
	ctx->amiga_trap_status = amiga_base + RTAREA_TRAP_STATUS + slot * RTAREA_TRAP_STATUS_SIZE;
	ctx->trap_mode = trap_mode;
	ctx->trap_slot = slot;
	uae_u32 idx = atomic_inc(&trap_thread_index) & (RTAREA_TRAP_DATA_NUM - 1);
	write_comm_pipe_pvoid(&trap_thread_pipe[idx], ctx, 1);
}

extern uae_sem_t hardware_trap_event[];
extern uae_sem_t hardware_trap_event2[];

static struct TrapContext outtrap[RTAREA_TRAP_DATA_SEND_NUM];
static volatile uae_atomic outtrap_alloc[RTAREA_TRAP_DATA_SEND_NUM];

TrapContext *alloc_host_main_trap_context(void)
{
	if (trap_is_indirect()) {
		int trap_slot = RTAREA_TRAP_DATA_NUM;
		if (atomic_inc(&outtrap_alloc[trap_slot - RTAREA_TRAP_DATA_NUM])) {
			while (outtrap_alloc[trap_slot - RTAREA_TRAP_DATA_NUM] > RTAREA_TRAP_DATA_SEND_NUM)
				sleep_millis(1);
		}
		TrapContext *ctx = &outtrap[trap_slot - RTAREA_TRAP_DATA_NUM];
		ctx->trap_slot = trap_slot;
		return ctx;
	} else {
		return NULL;
	}
}

void free_host_trap_context(TrapContext *ctx)
{
	if (trap_is_indirect()) {
		int trap_slot = RTAREA_TRAP_DATA_NUM;
		atomic_dec(&outtrap_alloc[trap_slot]);
	}
}

static uae_u32 call_hardware_trap_back(TrapContext *ctx, uae_u16 cmd, uae_u32 p1, uae_u32 p2, uae_u32 p3, uae_u32 p4)
{
	int trap_slot = ((ctx->amiga_trap_data & 0xffff) - RTAREA_TRAP_DATA) / RTAREA_TRAP_DATA_SLOT_SIZE;

	if (hardware_trap_kill[trap_slot] != 1) {
		if (hardware_trap_kill[trap_slot] == 0) {
			hardware_trap_kill[trap_slot] = 2;
		}
		return 0;
	}
	
	uae_u8 *data = ctx->host_trap_data + RTAREA_TRAP_DATA_SECOND;
	uae_u8 *status = ctx->host_trap_status + RTAREA_TRAP_STATUS_SECOND;

#if NEW_TRAP_DEBUG
	write_log(_T("TRAP BACK SLOT %d: CMD=%d P=%08x %08x %08x %08x TASK=%08x\n"),
		trap_slot, cmd, p1, p2, p3, p4, get_long_host(ctx->host_trap_data + RTAREA_TRAP_DATA_TASKWAIT));
#endif

	if (trap_slot != ctx->trap_slot)
		write_log(_T("Trap trap slot mismatch %d <> %d!\n"), trap_slot, ctx->trap_slot);

	if (trap_in_use2[trap_slot])
		write_log(_T("Trap slot %d already in use2!\n"), trap_slot);
	trap_in_use2[trap_slot] = true;

	put_long_host(data + 4, p1);
	put_long_host(data + 8, p2);
	put_long_host(data + 12, p3);
	put_long_host(data + 16, p4);
	put_word_host(status, cmd);

	volatile uae_u8 *d = status + 3;

	if (ctx->trap_mode == 2) {
		*d = 0xfe;
		// signal rtarea_bget wait
		uae_sem_post(&hardware_trap_event2[trap_slot]);
	} else if (ctx->trap_mode == 0) {
		*d = 0xfe;
	} else {
		atomic_inc(&hwtrap_waiting);
		atomic_or(&uae_int_requested, 0x2000);
		set_special_exter(SPCFLAG_UAEINT);
		*d = 0xff;
	}

	for (;;) {
		if (hardware_trap_kill[trap_slot] != 0 && hardware_trap_kill[trap_slot] != 1) {
			return 0;
		}
		if (hardware_trap_kill[trap_slot] == 0) {
			hardware_trap_kill[trap_slot] = 2;
			return  0;
		}
		uae_u8 v = *d;
		if (v == 0x01 || v == 0x02 || v == 0x03) {
			break;
		}
		if (uae_sem_trywait_delay(&hardware_trap_event[trap_slot], 100) == -2) {
			hardware_trap_kill[trap_slot] = 3;
			return 0;
		}
	}

	// get result
	uae_u32 v = get_long_host(data + 4);

	if (!trap_in_use2[trap_slot])
		write_log(_T("Trap slot %d in use2 unexpected release!\n"), trap_slot);
	trap_in_use2[trap_slot] = false;

	put_long_host(status, 0);

#if NEW_TRAP_DEBUG
	write_log(_T("TRAP BACK SLOT %d: RET = %08x TASK = %08x\n"), trap_slot, v, get_long_host(ctx->host_trap_data + RTAREA_TRAP_DATA_TASKWAIT));
#endif

	return v;
}

// Executes trap from C-code
void trap_callback(TRAP_CALLBACK cb, void *ud)
{
	if (trap_is_indirect()) {
		int trap_slot = RTAREA_TRAP_DATA_NUM;
		TrapContext *ctx = xcalloc(TrapContext, 1);
		ctx->host_trap_data = rtarea_bank.baseaddr + RTAREA_TRAP_DATA + trap_slot * RTAREA_TRAP_DATA_SLOT_SIZE;
		ctx->amiga_trap_data = rtarea_base + RTAREA_TRAP_DATA + trap_slot * RTAREA_TRAP_DATA_SLOT_SIZE;
		ctx->host_trap_status = rtarea_bank.baseaddr + RTAREA_TRAP_STATUS + trap_slot * RTAREA_TRAP_STATUS_SIZE;
		ctx->amiga_trap_status = rtarea_base + RTAREA_TRAP_STATUS + trap_slot * RTAREA_TRAP_STATUS_SIZE;
		ctx->trap_mode = 1;
		ctx->trap_slot = trap_slot;

		// Set status to allocated, we are skipping hwtrap_entry()
		put_long_host(ctx->host_trap_status + 4, 0x00000000);
		put_long_host(ctx->host_trap_status + 0, 0x00000080);

		ctx->callback = cb;
		ctx->callback_ud = ud;
		write_comm_pipe_pvoid(&trap_thread_pipe[trap_slot], ctx, 1);
	} else {
		cb(NULL, ud);
	}
}

TrapContext *alloc_host_thread_trap_context(void)
{
	if (trap_is_indirect()) {
		int trap_slot = RTAREA_TRAP_DATA_NUM;
		TrapContext *ctx = alloc_host_main_trap_context();
		ctx->host_trap_data = rtarea_bank.baseaddr + RTAREA_TRAP_DATA + trap_slot * RTAREA_TRAP_DATA_SLOT_SIZE;
		ctx->amiga_trap_data = rtarea_base + RTAREA_TRAP_DATA + trap_slot * RTAREA_TRAP_DATA_SLOT_SIZE;
		ctx->host_trap_status = rtarea_bank.baseaddr + RTAREA_TRAP_STATUS + trap_slot * RTAREA_TRAP_STATUS_SIZE;
		ctx->amiga_trap_status = rtarea_base + RTAREA_TRAP_STATUS + trap_slot * RTAREA_TRAP_STATUS_SIZE;
		ctx->trap_mode = 1;

		// Set status to allocated, we are skipping hwtrap_entry()
		put_long_host(ctx->host_trap_status + 4, 0x00000000);
		put_long_host(ctx->host_trap_status, 0x00000080);
		return ctx;
	} else {
		return NULL;
	}
}

/*
* Initialize trap mechanism.
*/
void init_traps(void)
{
	trap_count = 0;
	if (!trap_thread_id[0] && trap_is_indirect()) {
		for (size_t i = 0; i < TRAP_THREADS; i++) {
			init_comm_pipe(&trap_thread_pipe[i], 100, 1);
			hardware_trap_kill[i] = 1;
			uae_start_thread_fast(hardware_trap_thread, (void *)i, &trap_thread_id[i]);
		}
	}
}

void reset_traps(void)
{
	for (int i = 0; i < TRAP_THREADS; i++) {
		if (trap_thread_id[i]) {
			int htk = hardware_trap_kill[i];
			hardware_trap_kill[i] = -1;
			hardware_trap_state[i] = 1;
			write_comm_pipe_pvoid(&trap_thread_pipe[i], NULL, 1);
			while (hardware_trap_state[i] > 0) {
				for (int j = 0; j < RTAREA_TRAP_DATA_SIZE / RTAREA_TRAP_DATA_SLOT_SIZE; j++) {
					uae_sem_post(&hardware_trap_event[j]);
					uae_sem_post(&hardware_trap_event2[j]);
				}
				sleep_millis(1);
			}
			hardware_trap_kill[i] = htk;
		}
	}
	for (int j = 0; j < RTAREA_TRAP_DATA_SIZE / RTAREA_TRAP_DATA_SLOT_SIZE; j++) {
		uae_sem_unpost(&hardware_trap_event[j]);
		uae_sem_unpost(&hardware_trap_event2[j]);
	}
}

void free_traps(void)
{
	for (int i = 0; i < TRAP_THREADS; i++) {
		hardware_trap_state[i] = 0;
		if (trap_thread_id[i]) {
			if (hardware_trap_kill[i] >= 0) {
				hardware_trap_kill[i] = 0;
				write_comm_pipe_pvoid(&trap_thread_pipe[i], NULL, 1);
				while (hardware_trap_kill[i] == 0) {
					sleep_millis(1);
				}
			}
			destroy_comm_pipe(&trap_thread_pipe[i]);
			uae_end_thread(&trap_thread_id[i]);
			trap_thread_id[i] = NULL;
		}
	}
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

#ifdef AMIBERRY
	if(trap_mutex != 0)
		uae_sem_destroy(&trap_mutex);
	trap_mutex = 0;
#endif
	uae_sem_init (&trap_mutex, 0, 1);
}


void trap_reset(void)
{
	trap_mode = 0;
	hwtrap_waiting = 0;
}

bool trap_is_indirect(void)
{
	return currprefs.uaeboard > 2;
}

static bool trap_is_indirect_null(TrapContext *ctx)
{
	return ctx && trap_is_indirect();
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
	if (trap_is_indirect_null(ctx)) {
		uae_u8 *p = ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA;
		for (int i = 0; i < 16; i++) {
			if (ctx->calllib_reg_inuse[i]) {
				put_long_host(p , ctx->calllib_regs[i]);
				ctx->calllib_reg_inuse[i] = 0;
			} else {
				put_long_host(p, ctx->saved_regs.regs[i]);
			}
			p += 4;
		}
		v = call_hardware_trap_back(ctx, TRAPCMD_CALL_LIB, base, offset, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, 0);
	} else {
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
	}
	return v;
}
uae_u32 trap_call_func(TrapContext *ctx, uaecptr func)
{
	uae_u32 v;
	if (trap_is_indirect_null(ctx)) {
		uae_u8 *p = ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA;
		for (int i = 0; i < 16; i++) {
			if (ctx->calllib_reg_inuse[i]) {
				put_long_host(p, ctx->calllib_regs[i]);
				ctx->calllib_reg_inuse[i] = 0;
			} else {
				put_long_host(p, ctx->saved_regs.regs[i]);
			}
			p += 4;
		}
		v = call_hardware_trap_back(ctx, TRAPCMD_CALL_FUNC, func, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, 0, 0);
	} else {
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
	}
	return v;
}


void trap_set_background(TrapContext *ctx)
{
	if (!ctx)
		return;
	if (!trap_is_indirect())
		return;
	atomic_inc(&ctx->trap_background);
}

bool trap_valid_string(TrapContext *ctx, uaecptr addr, uae_u32 maxsize)
{
	if (!ctx || currprefs.uaeboard < 3) {
		for (int i = 0; i < maxsize; i++) {
			if (!valid_address(addr + i, 1))
				return false;
			if (get_byte(addr + i) == 0)
				return true;
		}
		return false;
	}
	// can't really do any checks..
	return true;
}

bool trap_valid_address(TrapContext *ctx, uaecptr addr, uae_u32 size)
{
	if (!ctx || currprefs.uaeboard < 3)
		return valid_address(addr, size) != 0;
	// can't really do any checks..
	return true;
}

uae_u32 trap_get_dreg(TrapContext *ctx, int reg)
{
	if (trap_is_indirect_null(ctx)) {
		if (!ctx) {
			write_log(_T("trap_get_dreg() without TrapContext!\n"));
			return 0;
		}
		return ctx->saved_regs.regs[reg];
	} else {
		return m68k_dreg(regs, reg);
	}
}
uae_u32 trap_get_areg(TrapContext *ctx, int reg)
{
	if (trap_is_indirect_null(ctx)) {
		if (!ctx) {
			write_log(_T("trap_get_areg() without TrapContext!\n"));
			return 0;
		}
		return ctx->saved_regs.regs[reg + 8];
	} else {
		return m68k_areg(regs, reg);
	}
}
void trap_set_dreg(TrapContext *ctx, int reg, uae_u32 v)
{
	if (trap_is_indirect_null(ctx)) {
		if (!ctx) {
			write_log(_T("trap_set_dreg() without TrapContext!\n"));
			return;
		}
		ctx->saved_regs.regs[reg] = v;
	} else {
		m68k_dreg(regs, reg) = v;
	}
}
void trap_set_areg(TrapContext *ctx, int reg, uae_u32 v)
{
	if (trap_is_indirect_null(ctx)) {
		if (!ctx) {
			write_log(_T("trap_set_areg() without TrapContext!\n"));
			return;
		}
		ctx->saved_regs.regs[reg + 8] = v;
	} else {
		m68k_areg(regs, reg) = v;
	}
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
	if (trap_is_indirect_null(ctx)) {
		call_hardware_trap_back(ctx, TRAPCMD_PUT_LONG, addr, v, 0, 0);
	} else {
		put_long(addr, v);
	}
}
void trap_put_longt(TrapContext* ctx, uaecptr addr, size_t v)
{
	if (trap_is_indirect_null(ctx)) {
		call_hardware_trap_back(ctx, TRAPCMD_PUT_LONG, addr, (uae_u32)v, 0, 0);
	}
	else {
		put_long(addr, (uae_u32)v);
	}
}

void trap_put_word(TrapContext *ctx, uaecptr addr, uae_u16 v)
{
	if (trap_is_indirect_null(ctx)) {
		call_hardware_trap_back(ctx, TRAPCMD_PUT_WORD, addr, v, 0, 0);
	} else  {
		put_word(addr, v);
	}
}
void trap_put_byte(TrapContext *ctx, uaecptr addr, uae_u8 v)
{
	if (trap_is_indirect_null(ctx)) {
		call_hardware_trap_back(ctx, TRAPCMD_PUT_BYTE, addr, v, 0, 0);
	} else {
		put_byte(addr, v);
	}
}

uae_u64 trap_get_quad(TrapContext *ctx, uaecptr addr)
{
	uae_u8 in[8];
	trap_get_bytes(ctx, in, addr, 8);
	return ((uae_u64)get_long_host(in + 0) << 32) | get_long_host(in + 4);
}
uae_u32 trap_get_long(TrapContext *ctx, uaecptr addr)
{
	if (trap_is_indirect_null(ctx)) {
		return call_hardware_trap_back(ctx, TRAPCMD_GET_LONG, addr, 0, 0, 0);
	} else {
		return get_long(addr);
	}
}
uae_u16 trap_get_word(TrapContext *ctx, uaecptr addr)
{
	if (trap_is_indirect_null(ctx)) {
		return call_hardware_trap_back(ctx, TRAPCMD_GET_WORD, addr, 0, 0, 0);
	} else {
		return get_word(addr);
	}
}
uae_u8 trap_get_byte(TrapContext *ctx, uaecptr addr)
{
	if (trap_is_indirect_null(ctx)) {
		return call_hardware_trap_back(ctx, TRAPCMD_GET_BYTE, addr, 0, 0, 0);
	} else {
		return get_byte(addr);
	}
}

void trap_put_bytes(TrapContext *ctx, const void *haddrp, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u8 *haddr = (uae_u8*)haddrp;
	if (trap_is_indirect_null(ctx)) {
		while (cnt > 0) {
			int max = cnt > RTAREA_TRAP_DATA_EXTRA_SIZE ? RTAREA_TRAP_DATA_EXTRA_SIZE : cnt;
			memcpy(ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA, haddr, max);
			call_hardware_trap_back(ctx, TRAPCMD_PUT_BYTES, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, addr, max, 0);
			haddr += max;
			addr += max;
			cnt -= max;
		}
	} else {
		if (real_address_allowed() && valid_address(addr, cnt)) {
			memcpy(get_real_address(addr), haddr, cnt);
		} else {
			for (int i = 0; i < cnt; i++) {
				put_byte(addr, *haddr++);
				addr++;
			}
		}
	}
}
void trap_get_bytes(TrapContext *ctx, void *haddrp, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	uae_u8 *haddr = (uae_u8*)haddrp;
	if (trap_is_indirect_null(ctx)) {
		while (cnt > 0) {
			int max = cnt > RTAREA_TRAP_DATA_EXTRA_SIZE ? RTAREA_TRAP_DATA_EXTRA_SIZE : cnt;
			call_hardware_trap_back(ctx, TRAPCMD_PUT_BYTES, addr, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, max, 0);
			memcpy(haddr, ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA, max);
			haddr += max;
			addr += max;
			cnt -= max;
		}
	} else {
		if (real_address_allowed() && valid_address(addr, cnt)) {
			memcpy(haddr, get_real_address(addr), cnt);
		} else {
			for (int i = 0; i < cnt; i++) {
				*haddr++ = get_byte(addr);
				addr++;
			}
		}
	}
}
void trap_put_longs(TrapContext *ctx, uae_u32 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	if (trap_is_indirect_null(ctx)) {
		while (cnt > 0) {
			int max = cnt > RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u32) ? RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u32) : cnt;
			for (int i = 0; i < max; i++) {
				put_long_host(ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA + i * sizeof(uae_u32), *haddr++);
			}
			call_hardware_trap_back(ctx, TRAPCMD_PUT_LONGS, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, addr, max, 0);
			addr += max * sizeof(uae_u32);
			cnt -= max;
		}
	} else {
		uae_u32 *p = (uae_u32*)haddr;
		for (int i = 0; i < cnt; i++) {
			put_long(addr, *p++);
			addr += 4;
		}
	}
}
void trap_get_longs(TrapContext *ctx, uae_u32 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	if (trap_is_indirect_null(ctx)) {
		while (cnt > 0) {
			int max = cnt > RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u32) ? RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u32) : cnt;
			call_hardware_trap_back(ctx, TRAPCMD_GET_LONGS, addr, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, max, 0);
			for (int i = 0; i < max; i++) {
				*haddr++ = get_long_host(ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA + i * sizeof(uae_u32));
			}
			addr += max * sizeof(uae_u32);
			cnt -= max;
		}
	} else {
		uae_u32 *p = (uae_u32*)haddr;
		for (int i = 0; i < cnt; i++) {
			*p++ = get_long(addr);
			addr += 4;
		}
	}
}
void trap_put_words(TrapContext *ctx, uae_u16 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	if (trap_is_indirect_null(ctx)) {
		while (cnt > 0) {
			int max = cnt > RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u16) ? RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u16) : cnt;
			for (int i = 0; i < max; i++) {
				put_word_host(ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA + i * sizeof(uae_u16), *haddr++);
			}
			call_hardware_trap_back(ctx, TRAPCMD_PUT_WORDS, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, addr, max, 0);
			addr += max * sizeof(uae_u16);
			cnt -= max;
		}
	} else {
		uae_u16 *p = (uae_u16*)haddr;
		for (int i = 0; i < cnt; i++) {
			put_word(addr, *p++);
			addr += sizeof(uae_u16);
		}
	}
}
void trap_get_words(TrapContext *ctx, uae_u16 *haddr, uaecptr addr, int cnt)
{
	if (cnt <= 0)
		return;
	if (trap_is_indirect_null(ctx)) {
		while (cnt > 0) {
			int max = cnt > RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u16) ? RTAREA_TRAP_DATA_EXTRA_SIZE / sizeof(uae_u16) : cnt;
			call_hardware_trap_back(ctx, TRAPCMD_GET_WORDS, addr, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, max, 0);
			for (int i = 0; i < max; i++) {
				*haddr++ = get_word_host(ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA + i * sizeof(uae_u16));
			}
			addr += max * sizeof(uae_u16);
			cnt -= max;
		}
	} else {
		uae_u16 *p = (uae_u16*)haddr;
		for (int i = 0; i < cnt; i++) {
			*p++ = get_word(addr);
			addr += sizeof(uae_u16);
		}
	}
}

int trap_put_string(TrapContext *ctx, const void *haddrp, uaecptr addr, int maxlen)
{
	int len = 0;
	uae_u8 *haddr = (uae_u8*)haddrp;
	if (trap_is_indirect_null(ctx)) {
		uae_u8 *p = ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA;
		for (;;) {
			uae_u8 v = *haddr++;
			*p++ = v;
			if (!v)
				break;
			len++;
		}
		call_hardware_trap_back(ctx, TRAPCMD_PUT_STRING, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, addr, maxlen, 0);
	} else {
		for (;;) {
			uae_u8 v = *haddr++;
			put_byte(addr, v);
			addr++;
			if (!v)
				break;
			len++;
		}
	}
	return len;
}
int trap_get_string(TrapContext *ctx, void *haddrp, uaecptr addr, int maxlen)
{
	int len = 0;
	uae_u8 *haddr = (uae_u8*)haddrp;
	if (trap_is_indirect_null(ctx)) {
		uae_u8 *p = ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA;
		call_hardware_trap_back(ctx, TRAPCMD_GET_STRING, addr, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, maxlen, 0);
		for (;;) {
			uae_u8 v = *p++;
			*haddr++ = v;
			if (!v)
				break;
			len++;
		}
	} else {
		for (;;) {
			uae_u8 v = get_byte(addr);
			*haddr++ = v;
			addr++;
			if (!v)
				break;
		}
		len++;
	}
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
	if (trap_is_indirect_null(ctx)) {
		uae_u8 *p = ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA;
		call_hardware_trap_back(ctx, TRAPCMD_GET_BSTR, addr, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, maxlen, 0);
		for (;;) {
			uae_u8 v = *p++;
			*haddr++ = v;
			if (!v)
				break;
			len++;
		}
	} else {
		uae_u8 cnt = get_byte(addr);
		while (cnt-- != 0 && maxlen-- > 0) {
			addr++;
			*haddr++ = get_byte(addr);
		}
		*haddr = 0;
	}
	return len;
}

void trap_set_longs(TrapContext *ctx, uaecptr addr, uae_u32 v, int cnt)
{
	if (cnt <= 0)
		return;
	if (trap_is_indirect_null(ctx)) {
		call_hardware_trap_back(ctx, TRAPCMD_SET_LONGS, addr, v, cnt, 0);
	} else {
		for (int i = 0; i < cnt; i++) {
			put_long(addr, v);
			addr += 4;
		}
	}
}
void trap_set_words(TrapContext *ctx, uaecptr addr, uae_u16 v, int cnt)
{
	if (cnt <= 0)
		return;
	if (trap_is_indirect_null(ctx)) {
		call_hardware_trap_back(ctx, TRAPCMD_SET_WORDS, addr, v, cnt, 0);
	} else {
		for (int i = 0; i < cnt; i++) {
			put_word(addr, v);
			addr += 2;
		}
	}
}
void trap_set_bytes(TrapContext *ctx, uaecptr addr, uae_u8 v, int cnt)
{
	if (cnt <= 0)
		return;
	if (trap_is_indirect_null(ctx)) {
		call_hardware_trap_back(ctx, TRAPCMD_SET_BYTES, addr, v, cnt, 0);
	} else {
		for (int i = 0; i < cnt; i++) {
			put_byte(addr, v);
			addr += 1;
		}
	}
}

void trap_multi(TrapContext *ctx, struct trapmd *data, int items)
{
	if (trap_is_indirect_null(ctx)) {
		uae_u8 *p = ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA;
		for (int i = 0; i < items; i++) {
			struct trapmd *md = &data[i];
			put_word_host(p + 0, md->cmd);
			put_byte_host(p + 2, md->trapmd_index);
			put_byte_host(p + 3, md->parm_num);
			put_long_host(p + 4, md->params[0]);
			put_long_host(p + 8, md->params[1]);
			put_long_host(p + 12, md->params[2]);
			put_long_host(p + 16, md->params[3]);
			p += 5 * 4;
		}
		call_hardware_trap_back(ctx, TRAPCMD_MULTI, ctx->amiga_trap_data + RTAREA_TRAP_DATA_EXTRA, items, 0, 0);
		p = ctx->host_trap_data + RTAREA_TRAP_DATA_EXTRA;
		for (int i = 0; i < items; i++) {
			struct trapmd *md = &data[i];
			md->params[0] = get_long_host(p + 4);
			p += 5 * 4;
		}
	} else {
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
}

void trap_memcpyha_safe(TrapContext *ctx, uaecptr dst, const uae_u8 *src, int size)
{
	if (size <= 0)
		return;
	if (trap_is_indirect()) {
		trap_put_bytes(ctx, src, dst, size);
	} else {
		memcpyha_safe(dst, src, size);
	}
}
void trap_memcpyah_safe(TrapContext *ctx, uae_u8 *dst, uaecptr src, int size)
{
	if (size <= 0)
		return;
	if (trap_is_indirect()) {
		trap_get_bytes(ctx, dst, src, size);
	} else {
		memcpyah_safe(dst, src, size);
	}
}
