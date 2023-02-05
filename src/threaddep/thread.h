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

int uae_sem_init(uae_sem_t* sem, int dummy, int initial_state);
void uae_sem_destroy(uae_sem_t* event);
#define uae_sem_post(PSEM) SDL_SemPost (*(PSEM))
#define uae_sem_wait(PSEM) SDL_SemWait (*(PSEM))
#define uae_sem_trywait(PSEM) SDL_SemTryWait (*(PSEM))
#define uae_sem_trywait_delay(PSEM, ms) SDL_SemWaitTimeout(*(PSEM), ms)
#define uae_sem_getvalue(PSEM) SDL_SemValue (*(PSEM))

#include "commpipe.h"

typedef SDL_Thread *uae_thread_id;
#define BAD_THREAD 0
typedef SDL_ThreadFunction uae_thread_function;

void uae_set_thread_priority(uae_thread_id* id, int pri);
void uae_end_thread(uae_thread_id* thread);
int uae_start_thread(const char* name, uae_thread_function fn, void* arg, uae_thread_id* thread);
int uae_start_thread_fast(uae_thread_function fn, void* arg, uae_thread_id* thread);
int uae_wait_thread(uae_thread_id* thread);
SDL_threadID uae_thread_get_id();

/* Do nothing; thread exits if thread function returns.  */
#define UAE_THREAD_EXIT do {} while (0)

#ifdef AMIBERRY
static uae_atomic atomic_and(volatile uae_atomic * p, uae_u32 v)
{
    return __atomic_and_fetch(p, v, __ATOMIC_SEQ_CST);
}

static uae_atomic atomic_or(volatile uae_atomic * p, uae_u32 v)
{
    return __atomic_or_fetch(p, v, __ATOMIC_SEQ_CST);
}

static uae_atomic atomic_inc(volatile uae_atomic * p)
{
    return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST);
}

static uae_atomic atomic_dec(volatile uae_atomic * p)
{
    return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST);
}

static uae_u32 atomic_bit_test_and_reset(volatile uae_atomic * p, uae_u32 v)
{
    uae_u32 mask = (1 << v);
    uae_u32 res = __atomic_fetch_and(p, ~mask, __ATOMIC_SEQ_CST);
    return (res & mask);
}

static void atomic_set(volatile uae_atomic* p, uae_u32 v)
{
    __atomic_store_n(p, v, __ATOMIC_SEQ_CST);
}
#endif
