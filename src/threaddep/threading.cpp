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

SDL_threadID uae_thread_get_id()
{
	return SDL_GetThreadID(nullptr);
}

uae_atomic atomic_and(volatile uae_atomic* p, uae_u32 v)
{
#if defined(CPU_AARCH64)
	__atomic_and_fetch(p, v, __ATOMIC_SEQ_CST);
#else
	return __sync_and_and_fetch(p, v);
#endif
}
uae_atomic atomic_or(volatile uae_atomic* p, uae_u32 v)
{
#if defined(CPU_AARCH64)
	__atomic_or_fetch(p, v, __ATOMIC_SEQ_CST);
#else
	return __sync_or_and_fetch(p, v);
#endif
}
void atomic_set(volatile uae_atomic* p, uae_u32 v)
{
#if defined(CPU_AARCH64)
	__atomic_store_n(p, v, __ATOMIC_SEQ_CST);
#else
	__sync_lock_test_and_set(p, v);
#endif
}
uae_atomic atomic_inc(volatile uae_atomic* p)
{
#if defined(CPU_AARCH64)
	return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST);
#else
	return __sync_add_and_fetch(p, 1);
#endif
}
uae_atomic atomic_dec(volatile uae_atomic* p)
{
#if defined(CPU_AARCH64)
	return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST);
#else
	return __sync_sub_and_fetch(p, 1);
#endif
}

uae_u32 atomic_bit_test_and_reset(volatile uae_atomic* p, uae_u32 v)
{
	uae_u32 mask = (1 << v);
#if defined(CPU_AARCH64)
	uae_u32 res = __atomic_fetch_and(p, ~mask, __ATOMIC_SEQ_CST);
#else
	uae_u32 res = __sync_fetch_and_and(p, ~mask);
#endif
	return (res & mask);
}
