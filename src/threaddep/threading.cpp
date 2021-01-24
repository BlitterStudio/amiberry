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
	}
	else
		name = "StartThread";
	
	auto* thread_id = SDL_CreateThread(fn, name, arg);
	if (thread_id == nullptr)
	{
		write_log("ERROR creating thread\n");
		result = 0;
	}
	if (tid) {
		*tid = thread_id;
	}
	return result;
}

int uae_wait_thread(uae_thread_id* thread)
{
	if (thread)
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

int uae_sem_init(uae_sem_t* sem, int dummy, int initial_state)
{
	if(*sem)
	{
		SDL_SemPost(*sem);
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
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
}
