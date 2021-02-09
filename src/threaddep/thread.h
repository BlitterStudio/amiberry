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

/* Do nothing; thread exits if thread function returns.  */
#define UAE_THREAD_EXIT do {} while (0)
