/*
 *	HT Editor
 *	systhread.h
 *
 *	Copyright (C) 2003 Sebastian Biallas (sb@biallas.net)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SYSTHREAD_H__
#define __SYSTHREAD_H__

#include "types.h"

typedef void * sys_mutex;
typedef void * sys_semaphore;
typedef void * sys_thread;

typedef void * (*sys_thread_function)(void *);

/* system-dependent (implementation in $MYSYSTEM/systhread.cc) */
/* all return 0 on success */
int	sys_create_mutex(sys_mutex *m);
int	sys_create_semaphore(sys_semaphore *s);
int	sys_create_thread(sys_thread *t, int flags, sys_thread_function start_routine, void *arg);

void	sys_destroy_mutex(sys_mutex m);
void	sys_destroy_semaphore(sys_semaphore s);
void	sys_destroy_thread(sys_semaphore s);

int	sys_lock_mutex(sys_mutex m);
int	sys_trylock_mutex(sys_mutex m);
void	sys_unlock_mutex(sys_mutex m);

void	sys_signal_semaphore(sys_semaphore s);
void	sys_signal_all_semaphore(sys_semaphore s);
void	sys_wait_semaphore(sys_semaphore s);
void	sys_wait_semaphore_bounded(sys_semaphore s, int ms);

void	sys_lock_semaphore(sys_semaphore s);
void	sys_unlock_semaphore(sys_semaphore s);

void	sys_exit_thread(void *ret);
void *	sys_join_thread(sys_thread t);

#endif
