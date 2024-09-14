
#include <cstring>
#include <cstdio>
#include <stdarg.h>

#include "system/systhread.h"

#include "uae/log.h"

int ht_printf(const char *format,...)
{
	UAE_LOG_VA_ARGS_FULL(format);
	return 0;
}
int ht_fprintf(FILE *f, const char *fmt, ...)
{
	return 0;
}
int ht_vfprintf(FILE *file, const char *fmt, va_list args)
{
	return 0;
}
int ht_snprintf(char *str, size_t count, const char *fmt, ...)
{
	return 0;
}
int ht_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
	return 0;
}

void ht_assert_failed(const char *file, int line, const char *assertion)
{
}

#if 0
void prom_quiesce()
{
}
#endif

#include "sysconfig.h"
#include "sysdeps.h"
#include <threaddep/thread.h>

typedef void * sys_mutex;

int sys_lock_mutex(sys_mutex m)
{
	uae_sem_wait(&m);
	return 1;
}

void sys_unlock_mutex(sys_mutex m)
{
	uae_sem_post(&m);
}

int sys_create_mutex(sys_mutex *m)
{
	if (!(*m))
		uae_sem_init(m, 0, 1);
	return 1;
}

void sys_destroy_mutex(sys_mutex m)
{
	uae_sem_destroy(&m);
}
