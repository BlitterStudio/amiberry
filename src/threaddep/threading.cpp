#include "sysconfig.h"
#include "sysdeps.h"
#include "uae.h"
#include "thread.h"

int uae_start_thread_fast(uae_thread_function fn, void* arg, uae_thread_id* thread)
{
	return uae_start_thread(NULL, fn, arg, thread);
}

int uae_start_thread(const char* name, uae_thread_function fn, void* arg, uae_thread_id* tid)
{
	auto result = 1;
	if (name != nullptr) {
		write_log("uae_start_thread \"%s\" function at %p arg %p\n", name,
			fn, arg);
	} else {
		name = "StartThread";
	}
	
	auto* thread = SDL_CreateThread(fn, name, arg);
	if (thread == nullptr)
	{
		write_log("ERROR creating thread, %s\n", SDL_GetError());
		result = 0;
	}
	if (tid) {
		*tid = thread;
	} else if (thread) {
		// If the caller didn't ask for the thread ID, we can't wait for it later.
		// If the caller didn't ask for the thread ID, we can't wait for it later.
		SDL_DetachThread(thread);
	}
	return result;
}

int uae_wait_thread(uae_thread_id* thread)
{
	if (thread && *thread)
	{
		SDL_WaitThread(*thread, nullptr);
		*thread = nullptr;
	}
	return 0;
}

void uae_end_thread(uae_thread_id* thread)
{
	uae_wait_thread(thread);
}

// WARNING: this does NOT reset an already-created semaphore. If *sem is
// non-null it merely SIGNALS it (count++), inflating the count. Calling this
// again on a long-lived binary mutex (e.g. on a reset/install path) pushes its
// count above 1, after which uae_sem_wait/trywait no longer enforces mutual
// exclusion and two threads can enter the critical section at once.
// Re-init sites MUST guard with `if (!sem)` (create-once) or destroy the
// semaphore first (`if (sem) uae_sem_destroy(&sem);`) before calling this.
int uae_sem_init(uae_sem_t* sem, int dummy, int initial_state)
{
	if (*sem)
	{
		SDL_SignalSemaphore(*sem);
	}
	else
	{
		*sem = SDL_CreateSemaphore(initial_state);
	}
	return *sem == nullptr;
}

void uae_sem_destroy(uae_sem_t* event)
{
	if (*event)
	{
		SDL_DestroySemaphore(*event);
		*event = nullptr;
	}
}

void uae_set_thread_priority(uae_thread_id* id, int pri)
{
	SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
}

SDL_ThreadID uae_thread_get_id(SDL_Thread *thread)
{
	return SDL_GetThreadID(thread);
}
