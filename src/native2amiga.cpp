/*
* UAE - The Un*x Amiga Emulator
*
* Call Amiga Exec functions outside the main UAE thread.
*
* Copyright 1999 Patrick Ohly
*
* Uses the EXTER interrupt that is setup in filesys.c
* and needs thread support.
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"
#include "options.h"
#include "memory.h"
#include "autoconf.h"
#include "traps.h"
#include "native2amiga.h"

smp_comm_pipe native2amiga_pending;
static uae_sem_t n2asem = 0;

/*
* to be called when setting up the hardware
*/

void native2amiga_install (void)
{
	init_comm_pipe (&native2amiga_pending, 300, 2);
	uae_sem_init (&n2asem, 0, 1);
}

void native2amiga_reset (void)
{
	smp_comm_pipe *p = &native2amiga_pending;
	p->rdp = 0;
	p->wrp = 0;
	p->reader_waiting = 0;
	p->writer_waiting = 0;
}

/*
* to be called when the Amiga boots, i.e. by filesys_diagentry()
*/
void native2amiga_startup (void)
{
}

int native2amiga_isfree (void)
{
	return comm_pipe_has_data (&native2amiga_pending) == 0;
}

#ifdef SUPPORT_THREADS

void uae_nativesem_wait(void)
{
	uae_sem_wait(&n2asem);
}
void uae_nativesem_post(void)
{
	uae_sem_post(&n2asem);
}

void uae_Cause (uaecptr interrupt)
{
	uae_nativesem_wait();
	write_comm_pipe_int (&native2amiga_pending, 3, 0);
	write_comm_pipe_u32 (&native2amiga_pending, interrupt, 1);
	do_uae_int_requested ();
	uae_nativesem_post();
}

void uae_ReplyMsg (uaecptr msg)
{
	uae_nativesem_wait();
	write_comm_pipe_int (&native2amiga_pending, 2, 0);
	write_comm_pipe_u32 (&native2amiga_pending, msg, 1);
	do_uae_int_requested ();
	uae_nativesem_post();
}

void uae_PutMsg (uaecptr port, uaecptr msg)
{
	uae_nativesem_wait();
	write_comm_pipe_int (&native2amiga_pending, 1, 0);
	write_comm_pipe_u32 (&native2amiga_pending, port, 0);
	write_comm_pipe_u32 (&native2amiga_pending, msg, 1);
	do_uae_int_requested ();
	uae_nativesem_post();
}

void uae_Signal (uaecptr task, uae_u32 mask)
{
	uae_nativesem_wait();
	write_comm_pipe_int (&native2amiga_pending, 0, 0);
	write_comm_pipe_u32 (&native2amiga_pending, task, 0);
	write_comm_pipe_int (&native2amiga_pending, mask, 1);
	do_uae_int_requested ();
	uae_nativesem_post();
}

void uae_Signal_with_Func(uaecptr task, uae_u32 mask, UAE_PROCESSED state)
{
	uae_nativesem_wait();
	write_comm_pipe_int(&native2amiga_pending, 0 | 0x80, 0);
	write_comm_pipe_pvoid(&native2amiga_pending, (void *) state, 0);
	write_comm_pipe_u32(&native2amiga_pending, task, 0);
	write_comm_pipe_int(&native2amiga_pending, mask, 1);
	do_uae_int_requested();
	uae_nativesem_post();
}


void uae_NotificationHack (uaecptr port, uaecptr nr)
{
	uae_nativesem_wait();
	write_comm_pipe_int (&native2amiga_pending, 4, 0);
	write_comm_pipe_int (&native2amiga_pending, port, 0);
	write_comm_pipe_int (&native2amiga_pending, nr, 1);
	do_uae_int_requested ();
	uae_nativesem_post();
}

void uae_ShellExecute(TCHAR *command)
{
	TCHAR *cmd = my_strdup(command);
	uae_nativesem_wait();
	write_comm_pipe_int(&native2amiga_pending, 5, 0);
	write_comm_pipe_pvoid(&native2amiga_pending, cmd, 1);
	do_uae_int_requested();
	uae_nativesem_post();
}

void uae_ShellExecute2(uae_u32 id)
{
	uae_nativesem_wait();
	write_comm_pipe_int(&native2amiga_pending, 6, 0);
	write_comm_pipe_int(&native2amiga_pending, id, 1);
	do_uae_int_requested();
	uae_nativesem_post();
}

#endif

uaecptr uae_AllocMem (TrapContext *ctx, uae_u32 size, uae_u32 flags, uaecptr sysbase)
{
	trap_set_dreg(ctx, 0, size);
	trap_set_dreg(ctx, 1, flags);
	return CallLib(ctx, sysbase, -198); /* AllocMem */
}

void uae_FreeMem (TrapContext *ctx, uaecptr memory, uae_u32 size, uaecptr sysbase)
{
	trap_set_dreg(ctx, 0, size);
	trap_set_areg(ctx, 1, memory);
	CallLib(ctx, sysbase, -0xD2); /* FreeMem */
}
