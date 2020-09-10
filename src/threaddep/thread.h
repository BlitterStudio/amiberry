/*
  * UAE - The Un*x Amiga Emulator
  * 
  * Threading support, using SDL
  * 
  * Copyright 1997, 2001 Bernd Schmidt
  */

#pragma once
#include <SDL.h>
#include <SDL_thread.h>

/* Sempahores. We use POSIX semaphores; if you are porting this to a machine
 * with different ones, make them look like POSIX semaphores. */
typedef SDL_sem *uae_sem_t;

STATIC_INLINE int uae_sem_init(uae_sem_t* sem, int dummy, int init)
{
	*sem = SDL_CreateSemaphore(init);
	return *sem == nullptr;
}

STATIC_INLINE void uae_sem_destroy(uae_sem_t* event)
{
	if (*event)
	{
		SDL_DestroySemaphore(*event);
		*event = nullptr;
	}
}
#define uae_sem_post(PSEM) SDL_SemPost (*PSEM)
#define uae_sem_wait(PSEM) SDL_SemWait (*PSEM)
#define uae_sem_trywait(PSEM) SDL_SemTryWait (*PSEM)
#define uae_sem_getvalue(PSEM) SDL_SemValue (*PSEM)

#include "commpipe.h"

typedef SDL_Thread *uae_thread_id;
#define BAD_THREAD 0

STATIC_INLINE void uae_set_thread_priority(uae_thread_id* id, int pri)
{
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
}

STATIC_INLINE void uae_end_thread(uae_thread_id* tid)
{
	if (tid)
	{
		SDL_WaitThread(*tid, static_cast<int*>(nullptr));
		*tid = NULL;
	}
}

STATIC_INLINE long uae_start_thread(const TCHAR* name, int (*f)(void*), void* arg, uae_thread_id* foo)
{
	auto result = 1;
	auto* id = SDL_CreateThread(f, "StartThread", arg);
	if (id == nullptr)
	{
		write_log("ERROR creating thread\n");
		result = 0;
	}
	if (foo)
	{
		*foo = id;
	}

	return result;
}

STATIC_INLINE long uae_start_thread_fast(int (*f)(void*), void* arg, uae_thread_id* foo)
{
	return uae_start_thread(nullptr, f, arg, foo);
}

STATIC_INLINE void uae_wait_thread(uae_thread_id thread)
{
	SDL_WaitThread(thread, static_cast<int*>(nullptr));
}

/* Do nothing; thread exits if thread function returns.  */
#define UAE_THREAD_EXIT do {} while (0)
