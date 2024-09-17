
#include <stdarg.h>
#include <stdlib.h>
#include <sys/timeb.h>

void write_log (const char *format, ...);

extern "C"
{
extern void __cdecl ppc_translate_init(void);

int __cdecl snprintf (char * s, size_t n, const char * format, ... )
{
	return 0;
}

int __cdecl __mingw_vprintf(const char * format, va_list arg)
{
	return 0;
}

int __cdecl __mingw_vfprintf(void *stream, const char * format, va_list arg)
{
	return 0;
}

struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};

void __cdecl gettimeofday(struct timeval *tv, void *blah)
{
	struct timeb time;

	ftime (&time);

	tv->tv_sec = time.time;
	tv->tv_usec = time.millitm * 1000;
}

void __cdecl inflateInit2_(void)
{
}
void __cdecl inflate(void)
{
}
void __cdecl inflateEnd(void)
{
}
void __cdecl compress2(void)
{
}
void __cdecl compressBound(void)
{
}

void __cdecl opendir(void)
{
}
void __cdecl readdir(void)
{
}
void __cdecl closedir(void)
{
}

void __cdecl popen(void)
{
}
void __cdecl pclose(void)
{
}

void __cdecl pixman_format_supported_source(void) { }
void __cdecl pixman_image_composite(void) { }
void __cdecl pixman_image_create_bits(void) { }
void __cdecl pixman_image_create_solid_fill(void) { }
void __cdecl pixman_image_fill_rectangles(void) { }
void __cdecl pixman_image_get_data(void) { }
void __cdecl pixman_image_get_height(void) { }
void __cdecl pixman_image_get_width(void) { }
void __cdecl pixman_image_get_stride(void) { }
void __cdecl pixman_image_unref(void) { }

int optarg;
int optind;
int getopt;

void __cdecl __emutls_get_address(void)
{
}

int __cdecl sizeof_CPUPPCState(void);
int __cdecl sizeof_PowerPCCPU(void);

void * __cdecl cpu_ppc_init(const char *cpu_model);


}

void crap(void)
{
	const char *cpu_model = "604e";
	void *cpu, *env;
	int cpu_size, env_size;

	cpu_size = sizeof_CPUPPCState();
	env_size = sizeof_PowerPCCPU();

	cpu = cpu_ppc_init(cpu_model);

}
