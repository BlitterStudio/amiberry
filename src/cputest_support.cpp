
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "fpp.h"
#include "mmu_common.h"
#include "cpummu030.h"

cpuop_func *loop_mode_table[65536];
int cpuipldelay2, cpuipldelay4;

void my_trim(TCHAR *s)
{
	int len;
	while (s[0] != '\0' && _tcscspn(s, _T("\t \r\n")) == 0)
		memmove(s, s + 1, (uaetcslen(s + 1) + 1) * sizeof(TCHAR));
	len = uaetcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		s[--len] = '\0';
}

void write_log(const TCHAR *format, ...)
{

}
void f_out(void *f, const TCHAR *format, ...)
{

}

TCHAR *buf_out(TCHAR *buffer, int *bufsize, const TCHAR *format, ...)
{
	int count;
	va_list parms;
	va_start(parms, format);

	if (buffer == NULL)
		return 0;
	count = _vsntprintf(buffer, (*bufsize) - 1, format, parms);
	va_end(parms);
	*bufsize -= uaetcslen(buffer);
	return buffer + uaetcslen(buffer);
}

void fpux_restore(int *v)
{
}
void fp_init_native(void)
{
	wprintf(_T("fp_init_native called!"));
	exit(0);
}
bool fp_init_native_80(void)
{
	wprintf(_T("fp_init_native_80 called!"));
	exit(0);
	return false;
}
void init_fpucw_x87(void)
{
}
void init_fpucw_x87_80(void)
{
}

int debugmem_get_segment(uaecptr addr, bool *exact, bool *ext, TCHAR *out, TCHAR *name)
{
	return 0;
}
int debugmem_get_symbol(uaecptr addr, TCHAR *out, int maxsize)
{
	return 0;
}
int debugmem_get_sourceline(uaecptr addr, TCHAR *out, int maxsize)
{
	return -1;
}
bool debugger_get_library_symbol(uaecptr base, uaecptr addr, TCHAR *out)
{
	return false;
}

int debug_safe_addr(uaecptr addr, int size)
{
	return 1;
}

void set_cpu_caches(bool flush)
{
}

void (*flush_icache)(int);

void flush_cpu_caches_040(uae_u16 opcode)
{
}

void mmu_tt_modified(void)
{
}

uae_u16 REGPARAM2 mmu_set_tc(uae_u16 tc)
{
	return 0;
}

void mmu_op(uae_u32 opcode, uae_u32 extra)
{
}

uae_u16 mmu030_state[3];
int mmu030_opcode;
int mmu030_idx, mmu030_idx_done;
uae_u32 mmu030_disp_store[2];
uae_u32 mmu030_fmovem_store[2];
uae_u32 mm030_stageb_address;
struct mmu030_access mmu030_ad[MAX_MMU030_ACCESS + 1];

uae_u32 mmu040_move16[4];
