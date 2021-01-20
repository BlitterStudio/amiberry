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
	if (name != NULL) {
		write_log("uae_start_tread \"%s\" function at %p arg %p\n", name,
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
	SDL_WaitThread(*thread, NULL);
	return 0;
}

void uae_end_thread(uae_thread_id* thread)
{
	if (thread)
	{
		SDL_WaitThread(*thread, NULL);
		*thread = NULL;
	}
}

int uae_sem_init(uae_sem_t* sem, int dummy, int init)
{
	*sem = SDL_CreateSemaphore(init);
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
