

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include "msc_dirent.h"
#else
#include <dirent.h>
#endif

typedef unsigned int uae_u32;
typedef int uae_s32;
typedef unsigned short uae_u16;
typedef short uae_s16;
typedef unsigned char uae_u8;
typedef signed char uae_s8;

#include "cputest_defines.h"

extern void callinflate(uae_u8*, uae_u8*,uae_u8*);

struct fpureg
{
	uae_u16 exp;
	uae_u16 dummy;
	uae_u32 m[2];
};

// must match asm.S
struct registers
{
	uae_u32 regs[16];
	uae_u32 ssp;
	uae_u32 msp;
	uae_u32 pc;
	uae_u32 sr;
	uae_u32 expsr;
	uae_u32 exc, exc010;
	uae_u32 excframe;
	uae_u32 tracecnt;
	uae_u16 tracedata[6];
	uae_u32 cycles, cycles2, cyclest;
	uae_u32 blttemp;

	struct fpureg fpuregs[8];
	uae_u32 fpiar, fpcr, fpsr;
	uae_u32 fsave[216 / 4];

	uae_u32 srcaddr, dstaddr, branchtarget;
	uae_u8 branchtarget_mode;
	uae_u32 endpc;

	uae_u16 fpeaset;
};

static short continue_on_error;
static struct registers test_regs;
static struct registers last_regs;
static struct registers cur_regs;
static uae_u8 *opcode_memory;
static uae_u32 opcode_memory_addr;
static uae_u8 *low_memory;
static uae_u8 *high_memory;
static int low_memory_size;
static int high_memory_size;
static uae_u32 test_low_memory_start, test_low_memory_end;
static uae_u32 test_high_memory_start, test_high_memory_end;
static uae_u8 *test_memory;
static uae_u32 test_memory_addr, test_memory_end;
static uae_u32 test_memory_size;
static uae_u8 *test_data;
#define BLT_TEMP_SIZE 8000
static uae_u32 blt_data;
static uae_u8 *safe_memory_start, *safe_memory_end;
static short safe_memory_mode;
static uae_u32 user_stack_memory, super_stack_memory;
static uae_u32 exception_vectors;
static int test_data_size;
static uae_u32 oldvbr;
static uae_u8 *vbr_zero = 0;
static int hmem_rom, lmem_rom;
static uae_u8 *absallocated;
static int cpu_lvl, fpu_model;
static uae_u8 fpu_max_precision;
static uae_u16 sr_undefined_mask;
static int check_undefined_sr;
static short is_fpu_adjust;
static short fpu_adjust_exp;
struct fpureg fpu_adjust;
static uae_u32 cpustatearraystore[16];
static uae_u32 cpustatearraynew[] = {
	0x00000005, // SFC
	0x00000005, // DFC
	0x00000000, // CACR
	0x00000000, // CAAR
	0x00000000, // MSP
};

static uae_u8 *low_memory_temp;
static uae_u8 *high_memory_temp;
static uae_u8 *low_memory_back;
static uae_u8 *high_memory_back;
static int low_memory_offset;
static int high_memory_offset;

static uae_u32 vbr[256];
static int exceptioncount[3][128];
static int supercnt;
static uae_u32 startpc, endpc;

static char inst_name[16+1];
#define PAGE_OUTBUFFER_SIZE 3000
static int outbuffer_size;
static char *stored_outbuffer;
static char outbuffer[6000];
static char outbuffer2[6000];
static char tmpbuffer[1024];
static char path[256];

static char *outbp;
static short infoadded;
static int errors;
static int totalerrors;
static int testcnt;
static short testcntsub, testcntsubmax;
static int invalidcycles;
static int fpu_approx, fpu_approxcnt;
static short dooutput = 1;
static short quit;
static uae_u8 ccr_mask;
static uae_u32 fpsr_ignore_mask;
static short fpiar_ignore;
static uae_u32 addressing_mask = 0x00ffffff;
static uae_u32 interrupt_mask;
static short initial_interrupt_mask;
static short initial_interrupt;
static short loop_mode_jit, loop_mode_68010, loop_mode_cnt;
static short instructionsize;
static short disasm;
static short basicexcept;
static short excskipccr;
static short skipmemwrite;
static short skipregchange;
static short skipccrchange;
static short askifmissing;
static short nextall;
static short exitmode;
static int exitcnt, exitcnt2, irqcnt;
static short cycles, cycles_range, cycles_adjust;
static short gotcycles;
static short interrupttest;
static short interrupt_delay_cnt;
static short interrupttest_diff_cnt;
static uae_u32 cyclecounter_addr;
static int errorcnt;
static short uaemode;
#ifdef AMIGA
static short interrupt_count;
static uae_u16 main_intena;
#endif
static int prealloc_test_data_size = 1001000;
static int prealloc_gzip_size = 500000;
static int prealloc;
static uae_u8 *debug_item_data;
static short debug_item_count;

#define SIZE_STORED_ADDRESS_OFFSET 6
#define SIZE_STORED_ADDRESS 20
static uae_u8 srcaddr[SIZE_STORED_ADDRESS];
static uae_u8 dstaddr[SIZE_STORED_ADDRESS];
static uae_u8 branchtarget[SIZE_STORED_ADDRESS];
static uae_u8 stackaddr[SIZE_STORED_ADDRESS];
static uae_u32 stackaddr_ptr;
static uae_u8 *opcode_memory_end;

#define MAX_IPL_PC_VALUES 4
static uae_u32 ipl_pc[MAX_IPL_PC_VALUES];
static int ipl_pc_cnt[MAX_IPL_PC_VALUES];

static char opcode[32], group[32], cpustr[10];

void *x_opendir(char *path);
void x_closedir(void *v);
int x_readdir(char *path, void *v, char *s);

#ifndef M68K

#define xmemcpy memcpy

static uae_u8 *allocate_absolute(uae_u32 addr, uae_u32 size)
{
	return calloc(1, size);
}
static void free_absolute(uae_u32 addr, uae_u32 size)
{
}
static uae_u8 *allocate_blttemp(uae_u32 size)
{
	return calloc(1, size);
}
static void free_blttemp(uae_u32 addr, uae_u32 size)
{
}
static void execute_test000(struct registers *regs)
{
}
static void execute_test010(struct registers *regs)
{
}
static void execute_test020(struct registers *regs)
{
}
static void execute_testfpu(struct registers *regs)
{
}
static uae_u32 tosuper(uae_u32 v, uae_u32 vv)
{
	return 0;
}
static void touser(uae_u32 v, uae_u32 vv)
{
}
static uae_u32 exceptiontable000, exceptiontable010, exceptiontable020, exceptiontablefpu;
static uae_u32 testexit(void)
{
	return 0;
}
static uae_u32 setvbr(uae_u32 v)
{
	return 0;
}
static uae_u32 get_cpu_model(void)
{
	return 0;
}
static void setcpu(uae_u32 v, uae_u32 *s, uae_u32 *d)
{
}
static void flushcache(uae_u32 v)
{
}
static void berrcopy(void *src, void *dst, uae_u32 size, uae_u32 hasvbr)
{
}
static uae_u32 fpucomp(void *v)
{
	return 0;
}
static uae_u32 fpucompzero(void *v)
{
	return 0;
}
static void initfpu(uae_u32 v)
{
}
static void *error_vector;
#else

static void xmemcpy(void *d, void *s, int size)
{
	__builtin_memcpy(d, s, size);
}

extern uae_u8 *allocate_absolute(uae_u32, uae_u32);
extern void free_absolute(uae_u32, uae_u32);
extern uae_u32 allocate_blttemp(uae_u32);
extern void free_blttemp(uae_u32, uae_u32);
extern void execute_test000(struct registers*);
extern void execute_test010(struct registers *);
extern void execute_test020(struct registers *);
extern void execute_testfpu(struct registers *);
extern uae_u32 tosuper(uae_u32, uae_u32);
extern void touser(uae_u32, uae_u32);
extern uae_u32 exceptiontable000, exceptiontable010, exceptiontable020, exceptiontablefpu;
extern uae_u32 testexit(void);
extern uae_u32 setvbr(uae_u32);
extern uae_u32 get_cpu_model(void);
extern void setcpu(uae_u32, uae_u32*, uae_u32*);
extern void flushcache(uae_u32);
extern void *error_vector;
extern void berrcopy(void*, void*, uae_u32, uae_u32);
extern uae_u32 fpucomp(void *);
extern uae_u32 fpucompzero(void *);
extern void initfpu(uae_u32);

#endif
static uae_u32 exceptiontableinuse;

struct accesshistory
{
	uae_u8 *addr;
	uae_u32 val;
	uae_u32 oldval;
	int size;
};
static int ahcnt;

#define MAX_ACCESSHIST 64
static struct accesshistory ahist[MAX_ACCESSHIST];

static int is_valid_test_addr_read(uae_u32 a)
{
	a &= addressing_mask;
	if ((uae_u8*)a >= safe_memory_start && (uae_u8*)a < safe_memory_end && (safe_memory_mode & 1))
		return 0;
	return (a >= test_low_memory_start && a < test_low_memory_end && test_low_memory_start != 0xffffffff) ||
		(a >= test_high_memory_start && a < test_high_memory_end && test_high_memory_start != 0xffffffff) ||
		(a >= test_memory_addr && a < test_memory_end);
}

static int is_valid_test_addr_readwrite(uae_u32 a)
{
	a &= addressing_mask;
	if ((uae_u8*)a >= safe_memory_start && (uae_u8*)a < safe_memory_end)
		return 0;
	return (a >= test_low_memory_start && a < test_low_memory_end && test_low_memory_start != 0xffffffff) ||
		(a >= test_high_memory_start && a < test_high_memory_end && test_high_memory_start != 0xffffffff) ||
		(a >= test_memory_addr && a < test_memory_end);
}

uae_u16 lw(uae_u16 *p)
{
	if (!is_valid_test_addr_read((uae_u32)p) || !is_valid_test_addr_read(((uae_u32)p) + 1))
		return 0;
	return *p;
}
uae_u32 llu(uae_u16 *p)
{
	if (!is_valid_test_addr_read((uae_u32)p) || !is_valid_test_addr_read(((uae_u32)p) + 1))
		return 0;
	if (!is_valid_test_addr_read(((uae_u32)p) + 2) || !is_valid_test_addr_read(((uae_u32)p) + 3))
		return p[0] << 16;
	return *(uae_u32*)p;
}
uae_s32 lls(uae_u16 *p)
{
	return (uae_s32)llu(p);
}

static void endinfo(void)
{
	printf("Last test: %u\n", testcnt);
	uae_u8 *p = opcode_memory;
	for (int i = 0; i < 4 * 2; i += 2) {
		if (!is_valid_test_addr_read((uae_u32)(&p[i])))
			break;
		uae_u16 v = (p[i] << 8) | (p[i + 1]);
		printf("%08x %04x\n", (uae_u32)&p[i], v);
		if (v == ILLG_OPCODE && i > 0)
			break;
	}
	printf("\n");
}

static void safe_memcpy(uae_u8 *d, uae_u8 *s, int size)
{
	if (safe_memory_start == (uae_u8*)0xffffffff && safe_memory_end == (uae_u8*)0xffffffff) {
		xmemcpy(d, s, size);
		return;
	}
	if (safe_memory_end <= d || safe_memory_start >= d + size) {
		if (safe_memory_end <= s || safe_memory_start >= s + size) {
			xmemcpy(d, s, size);
			return;
		}
	}
	while (size > 0) {
		int size2 = size > sizeof(tmpbuffer) ? sizeof(tmpbuffer) : size;
		if ((d + size2 > safe_memory_start && d < safe_memory_end) ||
			(s + size2 > safe_memory_start && s < safe_memory_end)) {
			for (int i = 0; i < size2; i++) {
				if ((d >= safe_memory_start && d < safe_memory_end) ||
					(s >= safe_memory_start && s < safe_memory_end)) {
					s++;
					d++;
					continue;
				}
				*d++ = *s++;
			}
		} else {
			xmemcpy(d, s, size2);
			d += size2;
			s += size2;
		}
		size -= size2;
	}
}

static int test_active;
static uae_u32 enable_data;
static uae_u32 error_vectors[12];

// if exception happens outside of test code, jump to
// infinite loop and flash colors.
static void reset_error_vectors(void)
{
	uae_u32 *p;
	if (cpu_lvl == 0) {
		p = (uae_u32*)vbr_zero;
	} else {
		p = vbr;
	}
	for (int i = 2; i < 4; i++) {
		p[i] = error_vectors[i - 2];
	}
}

static void set_error_vectors(void)
{
	uae_u32 *p;
	if (cpu_lvl == 0) {
		p = (uae_u32 *)vbr_zero;
	} else {
		p = vbr;
	}
	for (int i = 2; i < 4; i++) {
		p[i] = (uae_u32)&error_vector;
	}
}

static void start_test(void)
{
	if (test_active)
		return;

#ifdef M68K
	if (lmem_rom > 0) {
		if (memcmp(low_memory, low_memory_temp, low_memory_size)) {
			printf("Low memory ROM mismatch!\n");
			exit(0);
		}
	}
	if (hmem_rom > 0) {
		if (memcmp(high_memory, high_memory_temp, high_memory_size)) {
			printf("High memory ROM mismatch!\n");
			exit(0);
		}
	}
#endif

	test_active = 1;

	enable_data = tosuper(0, 1);

#ifdef AMIGA
	main_intena = *((volatile uae_u16*)0xdff01c);
#endif

	if (test_low_memory_start != 0xffffffff)
		safe_memcpy(low_memory_back + low_memory_offset, low_memory + low_memory_offset, low_memory_size - low_memory_offset);

	// always copy exception vectors if 68000
	if (cpu_lvl == 0 && (low_memory_offset > 0x08 || test_low_memory_start == 0xffffffff))
		safe_memcpy(low_memory_back + 8, low_memory + 8, (192 - 2) * 4);

	if (!hmem_rom && test_high_memory_start != 0xffffffff)
		safe_memcpy(high_memory_back, high_memory + high_memory_offset, high_memory_size - high_memory_offset);

	if (test_low_memory_start != 0xffffffff)
		safe_memcpy(low_memory + low_memory_offset, low_memory_temp + low_memory_offset, low_memory_size - low_memory_offset);

	if (cpu_lvl == 0 && low_memory_offset > 0x08)
		safe_memcpy(low_memory + 8, low_memory_temp + 8, (192 - 2) * 4);

	if (!hmem_rom && test_high_memory_start != 0xffffffff)
		safe_memcpy(high_memory + high_memory_offset, high_memory_temp, high_memory_size - high_memory_offset);

	if (cpu_lvl == 0) {
		uae_u32 *p = (uae_u32 *)vbr_zero;
		for (int i = 2; i < 12; i++) {
			p[i] = (uae_u32)(((uae_u32)&exceptiontable000) + (i - 2) * 2);
			if (exception_vectors && i >= 4) {
				p[i] = exception_vectors;
			}
			if (i < 12 + 2) {
				error_vectors[i - 2] = p[i];
			}
		}
		if (interrupttest || initial_interrupt_mask || initial_interrupt) {
			for (int i = 24; i < 24 + 8; i++) {
				p[i] = (uae_u32)(((uae_u32)&exceptiontable000) + (i - 2) * 2);
				if (exception_vectors) {
					p[i] = exception_vectors;
				}
			}
		}
		for (int i = 32; i < 48; i++) {
			p[i] = (uae_u32)(((uae_u32)&exceptiontable000) + (i - 2) * 2);
			if (exception_vectors) {
				p[i] = exception_vectors;
			}
		}
		exceptiontableinuse = (uae_u32)&exceptiontable000;
	} else {
		oldvbr = setvbr((uae_u32)vbr);
		for (int i = 2; i < 64; i++) {
			if (fpu_model) {
				vbr[i] = (uae_u32)(((uae_u32)&exceptiontablefpu) + (i - 2) * 2);
				exceptiontableinuse = (uae_u32)&exceptiontablefpu;
			} else if (cpu_lvl == 1) {
				vbr[i] = (uae_u32)(((uae_u32)&exceptiontable010) + (i - 2) * 2);
				exceptiontableinuse = (uae_u32)&exceptiontable010;
			} else {
				vbr[i] = (uae_u32)(((uae_u32)&exceptiontable020) + (i - 2) * 2);
				exceptiontableinuse = (uae_u32)&exceptiontable020;
			}
			if (i >= 2 && i < 12) {
				error_vectors[i - 2] = vbr[i];
			}
			if (exception_vectors && i >= 4) {
				vbr[i] = exception_vectors;
			}
		}
	}
	if (cpu_lvl >= 4) {
		// 68040/060 CACR (IE=1)
		cpustatearraynew[2] = 0x00008000;
	} else {
		// 68020/30 CACR (CI=1,IE=1)
		cpustatearraynew[2] = 0x0009;
		if (cpu_lvl == 3) {
			// 68030 CACR CD=1
			cpustatearraynew[2] |= 0x0800;
		}
	}

	setcpu(cpu_lvl, cpustatearraynew, cpustatearraystore);
}

static void end_test(void)
{
	if (!test_active)
		return;
	test_active = 0;

	if (test_low_memory_start != 0xffffffff)
		safe_memcpy(low_memory + low_memory_offset, low_memory_back + low_memory_offset, low_memory_size - low_memory_offset);

	if (cpu_lvl == 0 && (low_memory_offset > 0x08 || test_low_memory_start == 0xffffffff))
		safe_memcpy(low_memory + 8, low_memory_back + 8, (192 - 2) * 4);

	if (!hmem_rom && test_high_memory_start != 0xffffffff)
		safe_memcpy(high_memory + high_memory_offset, high_memory_back, high_memory_size - high_memory_offset);

	if (cpu_lvl > 0) {
		setvbr(oldvbr);
	}
	setcpu(cpu_lvl, cpustatearraystore, NULL);

#ifdef AMIGA
	*((volatile uae_u16*)0xdff09a) = 0x7fff;
	*((volatile uae_u16*)0xdff09c) = 0x7fff;
	*((volatile uae_u16*)0xdff09a) = main_intena | 0x8000;
#endif

	touser(enable_data, 1);
}

static int readdata(uae_u8 *p, int size, FILE *f, uae_u8 *unpack, int *offsetp)
{
	if (!unpack) {
		return fread(p, 1, size, f);
	} else {
		memcpy(p, unpack + (*offsetp), size);
		(*offsetp) += size;
		return size;
	}
}
static void seekdata(int seek, FILE *f, uae_u8 *unpack, int *offsetp)
{
	if (!unpack) {
		fseek(f, seek, SEEK_CUR);
	} else {
		(*offsetp) += seek;
	}
}

static uae_u8 *parse_gzip(uae_u8 *gzbuf, int *sizep)
{
	uae_u8 *end = gzbuf + (*sizep);
	uae_u8 flags = gzbuf[3];
	uae_u16 v;
	if (gzbuf[0] != 0x1f && gzbuf[1] != 0x8b)
		return NULL;
	gzbuf += 10;
	if (flags & 2) /* multipart not supported */
		return NULL;
	if (flags & 32) /* encryption not supported */
		return NULL;
	if (flags & 4) { /* skip extra field */
		v = *gzbuf++;
		v |= (*gzbuf++) << 8;
		gzbuf += v + 2;
	}
	if (flags & 8) { /* get original file name */
		while (*gzbuf++);
	}
	if (flags & 16) { /* skip comment */
		while (*gzbuf++);
	}
	*sizep = (end[-4] << 0) | (end[-3] << 8) | (end[-2] << 16) | (end[-1] << 24);
	return gzbuf;
}

#define INFLATE_STACK_SIZE 3000
static uae_u8 *inflatestack;
static uae_u8 *prealloc_gzip;

#ifdef AMIGA
extern void uae_command(char*);
#endif

static int set_berr(int mask, int ask)
{
#ifdef AMIGA
	if (uaemode) {
		if (!mask) {
			sprintf(tmpbuffer, "dbg \"w 0\"");
		} else {
			sprintf(tmpbuffer, "dbg \"w 0 %08x %08x B%s%s%s\"", (uae_u32)safe_memory_start, safe_memory_end - safe_memory_start, (mask & 1) ? "R" : "", (mask & 2) ? "W" : "", (mask & 4) ? "P" : "");
		}
		uae_command(tmpbuffer);
		return 0;
	}
#endif
	if (!ask) {
		return 0;
	}
	if (mask) {
		printf("Re-enable write bus error mode and press any key (ESC=abort)\n");
	} else {
		printf("Disable write bus error mode and press any key (SPACE=skip,ESC=abort)\n");
	}
	return getchar();
}

static int load_file_offset(FILE *f, int *foffsetp)
{
	int size = -1;
	unsigned char buf[4] = { 0 };
	fseek(f, *foffsetp, SEEK_SET);
	fread(buf, 1, sizeof(buf), f);
	if (buf[0] == 0xaf && buf[1] == 'M' && buf[2] == 'R' && buf[3] == 'G') {
		fread(buf, 1, sizeof(buf), f);
		size = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3] << 0);
		if (size == 0) {
			*foffsetp = -1;
			return 0;
		}
		*foffsetp = *foffsetp + size + 8;
		return size;
	}
	*foffsetp = -1;
	return -1;
}

static uae_u8 *load_file(const char *path, const char *file, uae_u8 *p, int *sizep, int *foffsetp, int exiterror, int candirect)
{
	char fname[256];
	uae_u8 *unpack = NULL;
	int unpackoffset = 0;
	int size = 0;
	int foffset = foffsetp ? *foffsetp : 0;

	if (path) {
		strcpy(fname, path);
		strcat(fname, file);
		if (strchr(file, '.')) {
			fname[strlen(fname) - 1] = 'z';
		} else {
			strcat(fname, ".gz");
		}
	} else {
		strcpy(fname, file);
	}
	FILE *f = fopen(fname, "rb");
	if (f) {
		int gsize = 0;
		if (foffsetp) {
			gsize = load_file_offset(f, foffsetp);
			if (gsize == 0) {
				return NULL;
			}
			if (gsize > 0) {
				*sizep = gsize;
			}
		}
		if (*sizep <= 0 || foffsetp == NULL) {
			fseek(f, 0, SEEK_END);
			gsize = ftell(f);
			fseek(f, 0, SEEK_SET);
		}

		uae_u8 *gzbuf = NULL;
		if (prealloc) {
			if (!prealloc_gzip) {
				prealloc_gzip = malloc(prealloc_gzip_size);
				if (!prealloc_gzip) {
					printf("Couldn't preallocate gzip temp memory, %d bytes.\n", prealloc_gzip_size);
					exit(0);
				}
			}
			if (gsize > prealloc_gzip_size) {
				printf("Preallocated gzip temp memory space is too small %d < %d\n", prealloc_gzip_size, gsize);
				exit(0);
			}
			gzbuf = prealloc_gzip;
		} else {
			gzbuf = malloc(gsize);
		}
		if (!gzbuf) {
			printf("Couldn't allocate %d bytes (packed), file '%s'\n", gsize, fname);
			exit(0);
		}
		if (fread(gzbuf, 1, gsize, f) != gsize) {
			printf("Couldn't read file '%s'\n", fname);
			exit(0);
		}
		fclose(f);
		size = gsize;
		uae_u8 *gzdata = parse_gzip(gzbuf, &size);
		if (!gzdata) {
			printf("Couldn't parse gzip file '%s'\n", fname);
			exit(0);
		}
		f = NULL;
		if (!inflatestack) {
			inflatestack = malloc(INFLATE_STACK_SIZE);
			if (!inflatestack) {
				printf("Couldn't allocate %d bytes (inflate stack)\n", INFLATE_STACK_SIZE);
				exit(0);
			}
			inflatestack += INFLATE_STACK_SIZE;
		}
		if (!p) {
			p = calloc(1, size);
			if (!p) {
				printf("Couldn't allocate %d bytes, file '%s'\n", size, fname);
				exit(0);
			}
			printf("Decompressing '%s' (%d -> %d)\n", fname, gsize, size);
			callinflate(p, gzdata, inflatestack);
			*sizep = size;
			if (!prealloc_gzip) {
				free(gzbuf);
			}
			return p;
		} else if (candirect) {
			printf("Decompressing '%s' (%d -> %d)\n", fname, gsize, size);
			callinflate(p, gzdata, inflatestack);
			*sizep = size;
			if (!prealloc_gzip) {
				free(gzbuf);
			}
			return p;
		} else {
			unpack = calloc(1, size);
			if (!unpack) {
				printf("Couldn't allocate %d bytes (unpack), file '%s'\n", size, fname);
				exit(0);
			}
			printf("Decompressing '%s' (%d -> %d)\n", fname, gsize, size);
			callinflate(unpack, gzdata, inflatestack);
			*sizep = size;
		}
		if (!prealloc_gzip) {
			free(gzbuf);
		}
	}
	if (!unpack) {
		if (path) {
			sprintf(fname, "%s%s", path, file);
		} else {
			strcpy(fname, file);
		}
		f = fopen(fname, "rb");
		if (!f) {
			if (exiterror) {
				printf("Couldn't open '%s'\n", fname);
				exit(0);
			}
			return NULL;
		}
		if (foffsetp) {
			size = load_file_offset(f, foffsetp);
			if (size == 0) {
				return NULL;
			}
			if (size > 0) {
				*sizep = size;
			}
		}
		size = *sizep;
		if (size <= 0) {
			fseek(f, 0, SEEK_END);
			size = ftell(f);
			fseek(f, 0, SEEK_SET);
		}
	}
	if (!p) {
		p = calloc(1, size);
		if (!p) {
			printf("Couldn't allocate %d bytes, file '%s'\n", size, fname);
			exit(0);
		}
	} else {
		if (prealloc && prealloc_test_data_size < size) {
			printf("Preallocated memory space is too small %d < %d\n", prealloc_test_data_size, size);
			exit(0);
		}
	}
	if (safe_memory_end < p || safe_memory_start >= p + size) {
		*sizep = readdata(p, size, f, unpack, &unpackoffset);
	} else {
		if (size > 0 && p < safe_memory_start) {
			int size2 = safe_memory_start - p;
			if (size2 > size)
				size2 = size;
			if (readdata(p, size2, f, unpack, &unpackoffset) != size2)
				goto end;
			p += size2;
			size -= size2;
		}
		if ((safe_memory_mode & 2) && (safe_memory_mode & (1 | 4))) {
			// if writing causes bus error and other bit(s) are also active: skip it
			if (size > 0 && p >= safe_memory_start && p < safe_memory_end) {
				int size2 = safe_memory_end - p;
				if (size2 > size)
					size2 = size;
				seekdata(size2, f, unpack, &unpackoffset);
				p += size2;
				size -= size2;
			}
		} else if (safe_memory_mode == 2) {
			// if only writes generate bus error: load data if different
			if (size > 0 && p >= safe_memory_start && p < safe_memory_end) {
				int size2 = safe_memory_end - p;
				if (size2 > size)
					size2 = size;
				uae_u8 *tmp = malloc(size2);
				if (!tmp) {
					printf("Couldn't allocate safe tmp memory (%d bytes)\n", size2);
					exit(0);
				}
				readdata(tmp, size2, f, unpack, &unpackoffset);
				if (memcmp(tmp, p, size2)) {
					uae_u32 t = tosuper(0, 0);
					berrcopy(tmp, p, size2 / 2, cpu_lvl > 0);
					touser(t, 0);
					if (memcmp(tmp, p, size2)) {
						// data was still different, do it manually.
						int ch = set_berr(0, 1);
						if (ch == 27) {
							exit(0);
						} else if (ch == 32) {
							seekdata(size2, f, unpack, &unpackoffset);
							p += size2;
							size -= size2;
						} else {
							memcpy(p, tmp, size2);
							p += size2;
							size -= size2;
							ch = set_berr(2, 1);
							if (ch == 27) {
								exit(0);
							}
						}
					} else {
						p += size2;
						size -= size2;
					}
				} else {
					// data was identical
					printf("Write-only bus error mode, data already correct. Skipping read.\n");
					p += size2;
					size -= size2;
				}
				free(tmp);
			}
		}
		if (size > 0) {
			if (readdata(p, size, f, unpack, &unpackoffset) != size)
				goto end;
		}
		size = *sizep;
	}
	if (*sizep != size) {
end:
		printf("Couldn't read file '%s' (%d <> %d)\n", fname, *sizep, size);
		exit(0);
	}
	if (f) {
		fclose(f);
	}
	if (unpack) {
		free(unpack);
	}
	return p;
}

static void endinfo_debug(uae_u8 *p)
{
	printf("PTR %08x, %08x, count %d\n", debug_item_data, p, debug_item_count);
}

static void pl(uae_u8 *p, uae_u32 v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >>  8;
	p[3] = v >>  0;
}
static void pw(uae_u8 *p, uae_u32 v)
{
	p[0] = v >> 8;
	p[1] = v >> 0;
}

static uae_u32 gl(uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

static uae_u16 gw(uae_u8 *p)
{
	return (p[0] << 8) | (p[1] << 0);
}

static uae_u8 *restore_fpvalue(uae_u8 *p, struct fpureg *fp)
{
	uae_u8 v = *p++;
	if ((v & CT_SIZE_MASK) != CT_SIZE_FPU) {
		end_test();
		printf("Expected CT_SIZE_FPU, got %02x\n", v);
		endinfo_debug(p);
		endinfo();
		exit(0);
	}
	fp->dummy = 0;
	uae_u8 size = *p++;
	if (size == 0x00) {
		fp->exp = 0;
		fp->m[0] = fp->m[1] = 0;
	} else if (size == 0xff) {
		fp->exp = gw(p);
		p += 2;
		fp->m[0] = gl(p);
		p += 4;
		fp->m[1] = gl(p);
		p += 4;
	} else {
		uae_u8 f[10];
		f[0] = fp->exp >> 8;
		f[1] = fp->exp;
		f[2] = fp->m[0] >> 24;
		f[3] = fp->m[0] >> 16;
		f[4] = fp->m[0] >> 8;
		f[5] = fp->m[0] >> 0;
		f[6] = fp->m[1] >> 24;
		f[7] = fp->m[1] >> 16;
		f[8] = fp->m[1] >> 8;
		f[9] = fp->m[1] >> 0;
		uae_u8 size1 = (size >> 4) & 15;
		for (uae_u8 i = 0; i < size1; i++) {
			f[i] = *p++;
		}
		uae_u8 size2 = (size >> 0) & 15;
		for (uae_u8 i = 0; i < size2; i++) {
			f[9 - i] = *p++;
		}
		fp->exp = (f[0] << 8) | (f[1] << 0);
		fp->m[0] = (f[2] << 24) | (f[3] << 16) | (f[4] << 8) | (f[5] << 0);
		fp->m[1] = (f[6] << 24) | (f[7] << 16) | (f[8] << 8) | (f[9] << 0);
	}
	return p;
}

static uae_u8 *restore_value(uae_u8 *p, uae_u32 *vp, int *sizep)
{
	uae_u32 val = *vp;
	uae_u8 v = *p++;
	switch(v & CT_SIZE_MASK)
	{
		case CT_SIZE_BYTE:
		val &= 0xffffff00;
		val |= *p++;
		*sizep = 0;
		break;
		case CT_SIZE_WORD:
		val &= 0xffff0000;
		val |= (*p++) << 8;
		val |= *p++;
		*sizep = 1;
		break;
		case CT_SIZE_LONG:
		val  = (*p++) << 24;
		val |= (*p++) << 16;
		val |= (*p++) << 8;
		val |= *p++;
		*sizep = 2;
		break;
		case CT_SIZE_FPU:
		end_test();
		printf("Unexpected CT_SIZE_FPU (%02x)\n", v);
		endinfo_debug(p);
		endinfo();
		exit(0);
		break;
	}
	*vp = val;
	return p;
}

static uae_u8 *restore_rel(uae_u8 *p, uae_u32 *vp, int nocheck)
{
	uae_u32 v = *vp;
	switch ((*p++) & CT_SIZE_MASK)
	{
		case CT_RELATIVE_START_BYTE:
		{
			uae_u8 val;
			val = *p++;
			v += (uae_s8)val;
			break;
		}
		case CT_RELATIVE_START_WORD:
		{
			uae_u16 val;
			val = (*p++) << 8;
			val |= *p++;
			v += (uae_s16)val;
			break;
		}
		case CT_ABSOLUTE_WORD:
		{
			uae_u16 val;
			val = (*p++) << 8;
			val |= *p++;
			v = (uae_s32)(uae_s16)val;
			break;
		}
		case CT_ABSOLUTE_LONG:
		{
			uae_u32 val;
			val = (*p++) << 24;
			val |= (*p++) << 16;
			val |= (*p++) << 8;
			val |= *p++;
			v = val;
			if (!nocheck) {
				if (test_low_memory_start != 0xffffffff && (val & addressing_mask) < low_memory_size) {
					; // low memory
				} else if (test_high_memory_start != 0xffffffff && (val & ~addressing_mask) == ~addressing_mask && val >= 0xfff80000) {
					; // high memory
				} else if ((val & addressing_mask) < test_memory_addr || (val & addressing_mask) >= test_memory_addr + test_memory_size) {
					end_test();
					printf("restore_rel CT_ABSOLUTE_LONG outside of test memory! %08x\n", v);
					endinfo_debug(p);
					endinfo();
					exit(0);
				}
			}
			break;
		}
	}
	*vp = v;
	return p;
}

static uae_u8 *restore_rel_ordered(uae_u8 *p, uae_u32 *vp)
{
	if (*p == CT_EMPTY)
		return p + 1;
	return restore_rel(p, vp, 1);
}


static void validate_mode(uae_u8 *p, uae_u8 v)
{
	uae_u8 mode = p[0];
	if ((mode & CT_DATA_MASK) != v) {
		end_test();
		printf("CT_MEMWRITE expected but got %02X\n", mode);
		endinfo_debug(p);
		endinfo();
		exit(0);
	}
}

static uae_u8 *get_memory_addr(uae_u8 *p, uae_u8 **addrp)
{
	uae_u8 v = *p++;
	switch(v & CT_SIZE_MASK)
	{
		case CT_ABSOLUTE_WORD:
		{
			uae_u16 val;
			val = (*p++) << 8;
			val |= *p++;
			uae_u8 *addr;
			uae_s16 offset = (uae_s16)val;
			if (offset < 0) {
				addr = high_memory + 32768 + offset;
			} else {
				addr = low_memory + offset;
			}
			validate_mode(p, CT_MEMWRITE);
			*addrp = addr;
			return p;
		}
		case CT_ABSOLUTE_LONG:
		{
			uae_u32 val;
			val  = (*p++) << 24;
			val |= (*p++) << 16;
			val |= (*p++) << 8;
			val |= *p++;
			if (val < low_memory_size) {
#ifndef M68K
				uae_u8 *addr = low_memory + val;
#else
				uae_u8 *addr = (uae_u8*)val;
#endif
				validate_mode(p, CT_MEMWRITE);
				*addrp = addr;
				return p;
			} else if (val >= test_memory_addr && val < test_memory_addr + test_memory_size) {
#ifndef M68K
				uae_u8 *addr = test_memory + (val - test_memory_addr);
#else
				uae_u8 *addr = (uae_u8*)val;
#endif
				validate_mode(p, CT_MEMWRITE);
				*addrp = addr;
				return p;
			} else {
				end_test();
				printf("get_memory_addr CT_ABSOLUTE_LONG outside of test memory! %08x\n", val);
				endinfo_debug(p);
				endinfo();
				exit(0);
			}
		}
		case CT_RELATIVE_START_WORD:
		{
			uae_u16 val;
			val = (*p++) << 8;
			val |= *p++;
			uae_s16 offset = (uae_s16)val;
			uae_u8 *addr = opcode_memory + offset;
			validate_mode(p, CT_MEMWRITE);
			*addrp = addr;
			return p;
		}
		break;

		default:
			end_test();
			printf("get_memory_addr unknown size %02x\n", v);
			endinfo_debug(p);
			endinfo();
			exit(0);
	}
	return NULL;
}

static void tomem(uae_u8 *p, uae_u32 v, uae_u32 oldv, int size, int storedata)
{
	if (ahcnt >= MAX_ACCESSHIST) {
		end_test();
		printf("History index too large! %d >= %d\n", ahcnt, MAX_ACCESSHIST);
		exit(0);
	}

	if (storedata) {
		struct accesshistory *ah = &ahist[ahcnt++];
		ah->oldval = oldv;
		ah->val = v;
		ah->size = size;
		ah->addr = p;
	}
	switch (size)
	{
		case 0:
			p[0] = (uae_u8)v;
			break;
		case 1:
			p[0] = (uae_u8)(v >> 8);
			p[1] = (uae_u8)(v >> 0);
			break;
		case 2:
			p[0] = (uae_u8)(v >> 24);
			p[1] = (uae_u8)(v >> 16);
			p[2] = (uae_u8)(v >> 8);
			p[3] = (uae_u8)(v >> 0);
			break;
	}
}

static void restoreahist(void)
{
	if (!ahcnt)
		return;
	for (int i = ahcnt - 1; i >= 0; i--) {
		struct accesshistory *ah = &ahist[i];
		tomem(ah->addr, ah->oldval, 0, ah->size, 0);
	}
	ahcnt = 0;
}

static uae_u8 *restore_bytes(uae_u8 *mem, uae_u8 *p)
{
	uae_u8 *addr = mem;
	uae_u16 len, offset;
	if (*p == 0xff) {
		p++;
		offset = *p++;
		len = *p++;
		if (len == 0) {
			len = 256;
		}
	} else {
		uae_u8 v = *p++;
		offset = v >> 5;
		len = v & 31;
		if (len == 0) {
			len = 32;
		}
	}
	addr += offset;
#ifndef _MSC_VER
	xmemcpy(addr, p, len);
#endif
	p += len;
	return p;
}

static uae_u8 *restore_memory(uae_u8 *p, int storedata)
{
	uae_u8 v = *p;
	switch (v & CT_SIZE_MASK)
	{
		case CT_ABSOLUTE_WORD:
		{
			uae_u8 *addr;
			int size;
			p = get_memory_addr(p, &addr);
			uae_u32 mv = 0;
			uae_u32 oldv = 0;
			p = restore_value(p, &oldv, &size);
			p = restore_value(p, &mv, &size);
			tomem(addr, mv, oldv, size, storedata);
			return p;
		}
		case CT_ABSOLUTE_LONG:
		{
			uae_u8 *addr;
			int size;
			p = get_memory_addr(p, &addr);
			uae_u32 mv = 0;
			uae_u32 oldv = 0;
			p = restore_value(p, &oldv, &size);
			p = restore_value(p, &mv, &size);
			tomem(addr, mv, oldv, size, storedata);
			return p;
		}
	}
	if ((v & CT_DATA_MASK) == CT_MEMWRITES) {
		switch (v & CT_SIZE_MASK)
		{
			case CT_PC_BYTES:
			{
				p = restore_bytes(opcode_memory, p + 1);
				break;
			}
			default:
				end_test();
				printf("Unknown restore_memory type!?\n");
				endinfo_debug(p);
				endinfo();
				exit(0);
				break;
			}
	} else {
		switch (v & CT_SIZE_MASK)
		{
			case CT_RELATIVE_START_WORD:
			{
				uae_u8 *addr;
				int size;
				p = get_memory_addr(p, &addr);
				uae_u32 mv = 0, oldv = 0;
				p = restore_value(p, &oldv, &size);
				p = restore_value(p, &mv, &size);
				tomem(addr, mv, oldv, size, storedata);
				return p;
			}
			default:
				end_test();
				printf("Unknown restore_memory type!?\n");
				endinfo_debug(p);
				endinfo();
				exit(0);
				break;
		}
	}
	return p;
}

static uae_u8 *restore_edata(uae_u8 *p)
{
	p++;
	uae_u8 v = *p++;
	switch(v)
	{
		case CT_EDATA_IRQ_CYCLES:
			interrupt_delay_cnt = *p++;
		break;
		default:
		end_test();
		printf("Unexpected CT_EDATA 0x%02x\n", *p);
		endinfo_debug(p);
		endinfo();
		exit(0);
	}
	return p;
}

static uae_u8 *restore_data(uae_u8 *p, struct registers *r)
{
	uae_u8 v = *p;
	if (v & CT_END) {
		end_test();
		printf("Unexpected end bit!? 0x%02x offset %d\n", v, p - test_data);
		endinfo_debug(p);
		endinfo();
		exit(0);
	}
	int mode = v & CT_DATA_MASK;
	if (mode == CT_SRCADDR) {
		int size;
		p = restore_value(p, &r->srcaddr, &size);
	} else if (mode == CT_DSTADDR) {
		int size;
		p = restore_value(p, &r->dstaddr, &size);
	} else if (mode == CT_ENDPC) {
		int size;
		p = restore_value(p, &r->endpc, &size);
	} else if (mode == CT_PC) {
		int size;
		p = restore_value(p, &r->pc, &size);
	} else if (mode == CT_BRANCHTARGET) {
		int size;
		p = restore_value(p, &r->branchtarget, &size);
		r->branchtarget_mode = *p++;
	} else if (mode < CT_AREG + 8) {
		int size;
		if ((v & CT_SIZE_MASK) == CT_SIZE_FPU) {
			p = restore_fpvalue(p, &r->fpuregs[mode]);
		} else {
			p = restore_value(p, &r->regs[mode], &size);
		}
	} else if (mode == CT_SR) {
		int size;
		p = restore_value(p, &r->sr, &size);
	} else if (mode == CT_CYCLES) {
		int size;
		p = restore_value(p, &r->cycles, &size);
		gotcycles = 1;
	} else if (mode == CT_FPIAR) {
		int size;
		p = restore_value(p, &r->fpiar, &size);
	} else if (mode == CT_FPCR) {
		int size;
		p = restore_value(p, &r->fpcr, &size);
	} else if (mode == CT_FPSR) {
		int size;
		p = restore_value(p, &r->fpsr, &size);
	} else if (mode == CT_MEMWRITE) {
		// if memwrite, store old data
		p = restore_memory(p, 1);
	} else if (mode == CT_MEMWRITES) {
		p = restore_memory(p, 0);
	} else if (mode == CT_EDATA) {
		p = restore_edata(p);
	} else {
		end_test();
		printf("Unexpected mode %02x\n", v);
		endinfo();
		exit(0);
	}
	return p;
}

static uae_u16 test_ccrignoremask;

static int addr_diff(uae_u8 *ap, uae_u8 *bp, int size)
{
	for (int i = 0; i < size; i++) {
		if (is_valid_test_addr_read((uae_u32)bp)) {
			if (*ap != *bp)
				return 1;
		}
		ap++;
		bp++;
	}
	return 0;
}

static void addinfo_bytes(char *name, uae_u8 *src, uae_u32 address, int offset, int len)
{
	sprintf(outbp, "%s %08x ", name, address);
	address += offset;
	outbp += strlen(outbp);
	int cnt = 0;
	while (len-- > 0) {
		if (offset == 0)
			*outbp++ = '*';
		else if (cnt > 0)
			*outbp++ = '.';
		if ((uae_u8*)address >= safe_memory_start && (uae_u8*)address < safe_memory_end && (safe_memory_mode & (1 | 4))) {
			outbp[0] = '?';
			outbp[1] = '?';
		} else {
			sprintf(outbp, "%02x", src[cnt]);
		}
		outbp += 2;
		offset++;
		address++;
		cnt++;
	}
	*outbp++ = '\n';
}

extern uae_u16 disasm_instr(uae_u16 *, char *, int);

static short is_valid_word(uae_u8 *p)
{
	if (!is_valid_test_addr_read((uae_u32)p) || !is_valid_test_addr_read((uae_u32)p + 1))
		return 0;
	return 1;
}

#define MAX_DISM_LINES 15

static void out_disasm(uae_u8 *mem)
{
	uae_u16 *code;
#ifndef M68K
	uae_u16 swapped[16];
	for (int i = 0; i < 16; i++) {
		swapped[i] = (mem[i * 2 + 0] << 8) | (mem[i * 2 + 1] << 0);
	}
	code = swapped;
#else
	code = (uae_u16*)mem;
#endif
	uae_u8 *p = mem;
	int offset = 0;
	int lines = 0;
	short last = 0;
	while (lines++ < MAX_DISM_LINES) {
		int v = 0;
		if (!is_valid_word(p)) {
			sprintf(outbp, "%08x -- INACCESSIBLE --\n", (uae_u32)p);
			outbp += strlen(outbp);
			break;
		}
		tmpbuffer[0] = 0;
		if (!(((uae_u32)code) & 1)) {
			v = disasm_instr(code + offset, tmpbuffer, cpu_lvl);
			sprintf(outbp, "%08x ", (uae_u32)p);
			outbp += strlen(outbp);
			for (int i = 0; i < v; i++) {
				uae_u8 *pb = &p[i * 2 + 0];
				uae_u16 v = 0;
				if (is_valid_word(pb)) {
					v = (pb[0] << 8) | (pb[1]);
				}
				sprintf(outbp, "%04x ", v);
				outbp += strlen(outbp);
				if (v == NOP_OPCODE)
					lines--;
			}
			sprintf(outbp, " %s\n", tmpbuffer);
			outbp += strlen(outbp);
			if (!is_valid_word((uae_u8*)(code + offset)))
				break;
			if (v <= 0)
				break;
			if (last)
				break;
			if (code[offset] == ILLG_OPCODE)
				last = 1;
			while (v > 0) {
				offset++;
				p += 2;
				v--;
			}
		} else {
			if (!is_valid_test_addr_read((uae_u32)code))
				break;
			sprintf(outbp, "%08x %02x\n", (uae_u32)code, *((uae_u8*)code));
			code = (uae_u16*)(((uae_u32)code) + 1);
			p++;
			outbp += strlen(outbp);
		}
		if (v < 0)
			break;
	}
	*outbp = 0;
}

static void addinfo(void)
{
	struct registers *r = &cur_regs;
	if (infoadded)
		return;
	infoadded = 1;
	if (!dooutput)
		return;

	if (disasm) {
		out_disasm(opcode_memory);
		if (r->branchtarget != 0xffffffff && !(r->branchtarget & 1)) {
			out_disasm((uae_u8*)r->branchtarget);
		}
	}

	uae_u16 *code = (uae_u16*)opcode_memory;
	if (code[0] == 0x4e73 || code[0] == 0x4e74 || code[0] == 0x4e75 || code[0] == 0x4e77) {
		addinfo_bytes("P", stackaddr, stackaddr_ptr, -SIZE_STORED_ADDRESS_OFFSET, SIZE_STORED_ADDRESS);
		addinfo_bytes(" ", (uae_u8*)stackaddr_ptr - SIZE_STORED_ADDRESS_OFFSET, stackaddr_ptr, -SIZE_STORED_ADDRESS_OFFSET, SIZE_STORED_ADDRESS);
	}
	if (r->srcaddr != 0xffffffff) {
		uae_u8 *a = srcaddr;
		uae_u8 *b = (uae_u8*)r->srcaddr - SIZE_STORED_ADDRESS_OFFSET;
		addinfo_bytes("S", a, r->srcaddr, -SIZE_STORED_ADDRESS_OFFSET, SIZE_STORED_ADDRESS);
		if (addr_diff(a, b, SIZE_STORED_ADDRESS)) {
			addinfo_bytes(" ", b, r->srcaddr, -SIZE_STORED_ADDRESS_OFFSET, SIZE_STORED_ADDRESS);
		}
	}
	if (r->dstaddr != 0xffffffff) {
		uae_u8 *a = dstaddr;
		uae_u8 *b = (uae_u8*)r->dstaddr - SIZE_STORED_ADDRESS_OFFSET;
		addinfo_bytes("D", a, r->dstaddr, -SIZE_STORED_ADDRESS_OFFSET, SIZE_STORED_ADDRESS);
		if (addr_diff(a, b, SIZE_STORED_ADDRESS)) {
			addinfo_bytes(" ", b, r->dstaddr, -SIZE_STORED_ADDRESS_OFFSET, SIZE_STORED_ADDRESS);
		}
	}
	if (r->branchtarget != 0xffffffff && r->srcaddr != r->branchtarget && r->dstaddr != r->branchtarget) {
		uae_u8 *b = (uae_u8*)r->branchtarget - SIZE_STORED_ADDRESS_OFFSET;
		addinfo_bytes("B", b, r->branchtarget, -SIZE_STORED_ADDRESS_OFFSET, SIZE_STORED_ADDRESS);
	}
//	sprintf(outbp, "STARTPC=%08x ENDPC=%08x\n", startpc, endpc);
//	outbp += strlen(outbp);
}

struct srbit
{
	char *name;
	int bit;
	int size;
};
static const struct srbit srbits[] = {
	{ "T1", 15, 1 },
	{ "T0", 14, 1 },
	{ "S", 13, 1 },
	{ "M", 12, 1 },
	{ "IM", 8, 3 },
	{ "X", 4, 1 },
	{ "N", 3, 1 },
	{ "Z", 2, 1 },
	{ "V", 1, 1 },
	{ "C", 0, 1 },
	{ NULL, 0, 0 }
};

// r = registers to output
// r1 = test results
// r2 = expected results
// sreg = original registers

static void out_regs(struct registers *r, struct registers *r1, struct registers *r2, struct registers *sreg, short before, short branched)
{
	if (before) {
		for (int i = 0; i < 16; i++) {
			if (i > 0 && (i % 4) == 0) {
				strcat(outbp, "\n");
			} else if ((i % 8) != 0) {
				strcat(outbp, " ");
			}
			uae_u32 v1 = r1->regs[i];
			uae_u32 v2 = r2->regs[i];
			uae_u32 sv = sreg->regs[i];
			outbp += strlen(outbp);
			sprintf(outbp, "%c%d:%c%08x", i < 8 ? 'D' : 'A', i & 7, v1 == v2 && v1 != sv ? '*' : (v1 != v2 ? '!' : ' '), r->regs[i]);
			outbp += strlen(outbp);
		}
		*outbp++ = '\n';
		sprintf(outbp, "SR:%c%04x      PC: %08x ISP: %08x", r1->sr == r2->sr && r1->sr != sreg->sr ? '*' : (r1->sr != r2->sr ? '!' : ' '), r->sr, r->pc, r->ssp);
	} else {
		// output only lines that have at least one modified register to save screen space
		for (int i = 0; i < 4; i++) {
			int diff = 0;
			for (int j = 0; j < 4; j++) {
				int idx = i * 4 + j;
				if (r1->regs[idx] != sreg->regs[idx]) {
					diff = 1;
				}
			}
			if (diff) {
				for (int j = 0; j < 4; j++) {
					int idx = i * 4 + j;
					if (j > 0)
						*outbp++ = ' ';
					uae_u32 v1 = r1->regs[idx];
					uae_u32 v2 = r2->regs[idx];
					uae_u32 sv = sreg->regs[idx];
					sprintf(outbp, "%c%d:%c%08x", idx < 8 ? 'D' : 'A', idx & 7, v1 == v2 && v1 != sv ? '*' : ((v1 != v2) ? '!' : ' '), r->regs[idx]);
					outbp += strlen(outbp);
				}
				*outbp++ = '\n';
			}
		}
		sprintf(outbp, "SR:%c%04x/%04x PC: %08x ISP: %08x", r1->sr == r2->sr && r1->sr != sreg->sr ? '*' : (r1->sr != r2->sr ? '!' : ' '), r->sr, r->expsr, r->pc, r->ssp);
	}
	outbp += strlen(outbp);
	if (cpu_lvl >= 2 && cpu_lvl <= 4) {
		sprintf(outbp, " MSP: %08x", r->msp);
		outbp += strlen(outbp);
	}
	if (branched) {
		sprintf(outbp, " B: %d", branched);
		outbp += strlen(outbp);
	}
	*outbp++ = '\n';
	*outbp = 0;

	if (before >= 0) {
		uae_u16 s = r->sr; // current value
		uae_u16 s1 = sreg->sr; // original value
		uae_u16 s2 = r1->sr; // test result value
		uae_u16 s3 = r2->sr; // expected result value
		for (int i = 0; srbits[i].name; i++) {
			if (i > 0)
				*outbp++ = ' ';
			uae_u16 mask = 0;
			for (int j = 0; j < srbits[i].size; j++) {
				mask <<= 1;
				mask |= 1 << srbits[i].bit;
			}
			sprintf(outbp, "%s%c%d", srbits[i].name,
				(s2 & mask) != (s3 & mask) ? '!' : ((s1 & mask) != (s2 & mask) ? '*' : '='), (s & mask) >> srbits[i].bit);
			outbp += strlen(outbp);
		}
		*outbp++ = '\n';
		*outbp = 0;
	}

	if (!fpu_model)
		return;

	if (before) {
		for (int i = 0; i < 8; i++) {
			if (i > 0 && (i % 2) == 0) {
				strcat(outbp, "\n");
			} else if ((i % 4) != 0) {
				strcat(outbp, " ");
			}
			outbp += strlen(outbp);
			struct fpureg *f = &r->fpuregs[i];
			void *f1 = &r->fpuregs[i];
			void *f2 = &r1->fpuregs[i];
			sprintf(outbp, "FP%d:%c%04x-%08x%08x %f",
				i,
				memcmp(f1, f2, sizeof(struct fpureg)) ? '*' : ' ',
				f->exp, f->m[0], f->m[1],
				*((long double *)f));
			outbp += strlen(outbp);
		}
		*outbp++ = '\n';
		*outbp = 0;
	} else {
		for (int i = 0; i < 8; i += 2) {
			struct fpureg *f = &r->fpuregs[i];
			void *f1 = &r1->fpuregs[i];
			void *f2 = &r2->fpuregs[i];
			void *f1b = &r1->fpuregs[i + 1];
			void *f2b = &r2->fpuregs[i + 1];

			if (memcmp(f1, f2, sizeof(struct fpureg)) || memcmp(f1b, f2b, sizeof(struct fpureg))) {
				for (int j = 0; j < 2; j++) {
					f1 = &r->fpuregs[i + j];
					f2 = &test_regs.fpuregs[i + j];
					f = &test_regs.fpuregs[i + j];
					if (j > 0)
						*outbp++ = ' ';
					sprintf(outbp, "FP%d:%c%04x-%08x%08x %f",
						i + j,
						memcmp(f1, f2, sizeof(struct fpureg)) ? '*' : ' ',
						f->exp, f->m[0], f->m[1],
						*((long double *)f));
					outbp += strlen(outbp);
				}
				*outbp++ = '\n';
				*outbp = 0;
			}
		}
	}
	sprintf(outbp, "FPSR:%c%08x FPCR:%c%08x FPIAR:%c%08x\n",
		r->fpsr != r2->fpsr ? '*' : ' ', r->fpsr,
		r->fpcr != r2->fpcr ? '*' : ' ', r->fpcr,
		r->fpiar != r2->fpiar ? '*' : ' ', r->fpiar);

	outbp += strlen(outbp);

}

static void hexdump(uae_u8 *p, int len, short lf)
{
	for (int i = 0; i < len; i++) {
		if (i > 0)
			*outbp++ = '.';
		sprintf(outbp, "%02x", p[i]);
		outbp += strlen(outbp);
	}
	if (lf)
		*outbp++ = '\n';
}

static uae_u8 last_exception[256], last_exception_extra;
static int last_exception_len;
static uae_u8 alternate_exception1[256];
static uae_u8 alternate_exception2[256];
static uae_u8 alternate_exception3[256];
static uae_u8 masked_exception[256];
static int mask_exception;
static int exception_stored;

static int compare_exception(uae_u8 *s1, uae_u8 *s2, int len, int domask, uae_u8 *mask)
{
	if (!domask) {
		return memcmp(s1, s2, len);
	} else {
		for (int i = 0; i < len; i++) {
			uae_u8 m = mask[i];
			if (m == 0)
				continue;
			if ((s1[i] & m) != (s2[i] & m))
				return 1;
		}
		return 0;
	}
}

static uae_u8 *get_exceptionframe(struct registers *regs, short excnum, int *sizep)
{
	uae_u8 *frame = (uae_u8*)regs->excframe;
	short size = 0;
	if (cpu_lvl == 0) {
		if (excnum == 2 || excnum == 3) {
			size = 14;
		} else {
			size = 6;
		}
		*sizep = size;
		return frame;
	}
	uae_u16 type = (frame[4] << 8) | frame[5];
	switch (type >> 12)
	{
	case 0:
		size = 8;
		break;
	case 2:
	case 3:
		size = 12;
		break;
	case 4:
		size = 16;
		break;
	case 7:
		size = 60;
		break;
	case 8:
		size = 82;
		break;
	case 9:
		size = 20;
		break;
	case 10:
		size = 32;
		break;
	case 11:
		size = 92;
		break;
	}
	*sizep = size;
	return frame;
}

static uae_u8 *validate_exception(struct registers *regs, uae_u8 *p, short excnum, short *gotexcnum, short *experr, short *extratrace, short *group2with1)
{
	int exclen = 0;
	uae_u8 *exc;
	uae_u8 *op = p;
	uae_u8 *sp = (uae_u8*)regs->excframe;
	uae_u32 v;
	uae_u8 excdatalen = *p++;
	int excrwp = 0;
	int alts = 0;

	mask_exception = 0;
	if (!excdatalen) {
		return p;
	}

	if (excdatalen != 0xff) {
		// check possible extra trace
		last_exception_extra = *p++;
		if (last_exception_extra & 0x40) {
			*group2with1 = *p++;
			exceptioncount[0][*group2with1]++;
			last_exception_extra &= ~0x40;
		}
		if ((last_exception_extra & 0x3f) == 9) {
			exceptioncount[0][last_exception_extra & 0x3f]++;
			uae_u32 ret = (regs->tracedata[1] << 16) | regs->tracedata[2];
			uae_u16 sr = regs->tracedata[0];
			if (regs->tracecnt == 0) {
				uae_u32 pc;
				if (!(last_exception_extra & 0x80)) {
					pc = exceptiontableinuse + (excnum - 2) * 2;
				} else {
					pc = opcode_memory_addr;
					p = restore_rel_ordered(p + 2, &pc);
				}
				sprintf(outbp, "Expected trace exception (PC=%08x) but got none.\n", pc);
				outbp += strlen(outbp);
				*experr |= 2;
			} else if (!(last_exception_extra & 0x80)) {
				// Trace stacked with group 2 exception
				if (!(sr & 0x2000) || (sr | 0x2000 | 0xc000) != (regs->sr | 0x2000 | 0xc000)) {
					sprintf(outbp, "Trace (%d stacked) SR mismatch: %04x != %04x\n", excnum, sr, regs->sr);
					outbp += strlen(outbp);
					*experr |= 2;
				}
				uae_u32 retv = exceptiontableinuse + (excnum - 2) * 2;
				if (ret != retv) {
					sprintf(outbp, "Trace (%d stacked) PC mismatch: %08x != %08x\n", excnum, ret, retv);
					outbp += strlen(outbp);
					*experr |= 2;
				}
				*extratrace = 1;
			} else {
				// Standalone Trace
				uae_u16 vsr = (p[0] << 8) | (p[1]);
				p += 2;
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				if ((vsr & test_ccrignoremask) != (sr & test_ccrignoremask)) {
					sprintf(outbp, "Trace (non-stacked) SR mismatch: %04x != %04x (PC=%08x)\n", sr, vsr, v);
					outbp += strlen(outbp);
					*experr |= 2;
				}
				if (v != ret) {
					sprintf(outbp, "Trace (non-stacked) PC mismatch: %08x != %08x (SR=%04x)\n", ret, v, vsr);
					outbp += strlen(outbp);
					*experr |= 2;
				}
				*extratrace = -1;
			}
		} else if (!last_exception_extra && excnum != 9) {
			if (regs->tracecnt > 0) {
				uae_u32 ret = (regs->tracedata[1] << 16) | regs->tracedata[2];
				uae_u16 sr = regs->tracedata[0];
				sprintf(outbp, "Got unexpected trace exception: SR=%04x PC=%08x.\n", sr, ret);
				outbp += strlen(outbp);
				if (excnum >= 2) {
					sprintf(outbp, "Exception %d also pending.\n", excnum);
					outbp += strlen(outbp);
				}
				*experr |= 2;
			}
		} else if (last_exception_extra) {
			end_test();
			printf("Unsupported exception extra %d\n", last_exception_extra);
			exit(0);
		}
		// trace only
		if (excnum == 9 && *gotexcnum == 4) {
			sp = (uae_u8*)regs->tracedata;
			*gotexcnum = 9;
		}
	}

	last_exception_len = 0;
	if (excnum == 1)
		return p;

	exc = last_exception;
	if (excdatalen != 0xff) {
		if (cpu_lvl == 0) {
			if (excnum == 2 || excnum == 3) {
				// status (with undocumented opcode part)
				uae_u8 status = p[0];
				uae_u8 opcode0 = p[1];
				uae_u8 opcode1 = p[2];
				p += 1 + 2;
				uae_u8 opcode0b = opcode0;
				uae_u8 opcode1b = opcode1;
				if (status & 0x20) {
					opcode0b = p[0];
					opcode1b = p[1];
					p += 2;
				}
				exc[0] = opcode0;
				exc[1] = (opcode1 & ~0x1f) | (status & 0x1f);
				excrwp = ((status & 0x10) == 0) ? 1 : 0;
				if (status & 2)
					excrwp = 2;
				// access address
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				pl(exc + 2, v);
				// opcode
				exc[6] = opcode0;
				exc[7] = opcode1;
				// sr
				exc[8] = regs->sr >> 8;
				exc[9] = regs->sr;
				// program counter
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				pl(exc + 10, v);
				exclen = 14;
				if (basicexcept) {
					// I/N field is not always as documented
					memcpy(alternate_exception1, exc, exclen);
					alternate_exception1[1] ^= 0x08; // I/N
					alts = 1;
					if (status & 0x80) {
						// opcode field is not always current opcode
						memcpy(alternate_exception2, exc, exclen);
						alternate_exception2[0] = opcode0b;
						alternate_exception2[1] = (opcode1b & ~0x1f) | (status & 0x1f);
						alternate_exception2[6] = opcode0b;
						alternate_exception2[7] = opcode1b;
						memcpy(alternate_exception3, alternate_exception2, exclen);
						alternate_exception3[1] ^= 0x08; // I/N
						alts += 2;
					}
				}
			} else {
				// sr
				exc[0] = regs->sr >> 8;
				exc[1] = regs->sr;
				// program counter
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				pl(exc + 2, v);
				exclen = 6;
			}
		} else if (cpu_lvl > 0) {
			// sr
			exc[0] = regs->sr >> 8;
			exc[1] = regs->sr;
			// program counter
			v = opcode_memory_addr;
			p = restore_rel_ordered(p, &v);
			pl(exc + 2, v);
			const uae_u16 t0 = *p++;
			const uae_u16 t1 = *p++;
			// frame type
			uae_u16 frame = (t0 << 8) | t1;
			exc[6] = frame >> 8;
			exc[7] = frame >> 0;

			switch (frame >> 12)
			{
			case 0:
				exclen = 8;
				break;
			case 2:
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				pl(exc + 8, v);
				exclen = 12;
				if (excnum == 11 || excnum == 55) {
					regs->fpeaset = (*p++) & 1;
					if (!regs->fpeaset) {
						mask_exception = 1;
						memset(masked_exception, 0xff, exclen);
						memset(masked_exception + 8, 0, 4);
					}
				}
				break;
			case 3:
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				pl(exc + 8, v);
				exclen = 12;
				regs->fpeaset = (*p++) & 1;
				if (!regs->fpeaset) {
					mask_exception = 1;
					memset(masked_exception, 0xff, exclen);
					memset(masked_exception + 8, 0, 4);
				}
				break;
			case 4:
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				pl(exc + 8, v);
				v = opcode_memory_addr;
				p = restore_rel_ordered(p, &v);
				pl(exc + 12, v);
				exclen = 16;
				regs->fpeaset = (*p++) & 1;
				if (!regs->fpeaset) {
					mask_exception = 1;
					memset(masked_exception, 0xff, exclen);
					memset(masked_exception + 8, 0, 4);
				}
				break;
			case 8: // 68010 bus/address error
				{
					// program counter
					v = opcode_memory_addr;
					p = restore_rel_ordered(p, &v);
					pl(exc + 2, v);
					uae_u16 ssw = (p[0] << 8) | p[1];
					exc[8] = *p++;
					exc[9] = *p++;
					excrwp = ((exc[8] & 1) == 0) ? 1 : 0;
					if (exc[9] & 2)
						excrwp = 2;
					uae_u32 fault_addr = opcode_memory_addr;
					p = restore_rel_ordered(p, &fault_addr);
					pl(exc + 10, fault_addr);
					// data out
					exc[16] = *p++;
					exc[17] = *p++;
					// data in
					exc[20] = *p++;
					exc[21] = *p++;
					// inst
					exc[24] = *p++;
					exc[25] = *p++;
					exc[14] = exc[15] = 0;
					sp[14] = sp[15] = 0;
					exc[18] = exc[19] = 0;
					sp[18] = sp[19] = 0;
					exc[22] = exc[23] = 0;
					sp[22] = sp[23] = 0;
					if (basicexcept) {
						exclen = 14;
						mask_exception = 1;
						memset(masked_exception, 0xff, sizeof(masked_exception));
						masked_exception[8] = ~0x30;
					} else {
						// ignore undocumented data
						exclen = 26;
						// read input buffer may contain either actual data read from memory or previous read data
						// this depends on hardware, cpu does dummy read cycle and some hardware returns memory data, some ignore it.
						memcpy(alternate_exception1, exc, exclen);
						memcpy(alternate_exception2, exc, exclen);
						alternate_exception1[20] = p[0];
						alternate_exception1[21] = p[1];
						memcpy(alternate_exception3, alternate_exception1, exclen);
						// same with instruction input buffer if branch instruction generates address error
						alternate_exception2[24] = p[0];
						alternate_exception2[25] = p[1];
						alternate_exception3[24] = p[0];
						alternate_exception3[25] = p[1];
						if (excnum == 2 || !is_valid_test_addr_readwrite(fault_addr - 4) || !is_valid_test_addr_readwrite(fault_addr + 4)) {
							// bus error read: cpu may still read the data, depends on hardware.
							// ignore input buffer contents
							mask_exception = 1;
							memset(masked_exception, 0xff, sizeof(masked_exception));
							masked_exception[20] = 0;
							masked_exception[21] = 0;
						}
						alts = 3;
					}
					if (instructionsize == 0) {
						// if byte wide bus error, ignore high byte of input and output buffer
						// contents of high byte depends on used alu operation and instruction type and more..
						masked_exception[16] = 0;
						masked_exception[20] = 0;
					}
					p += 2;
				}
				break;
			case 0x0a:
			case 0x0b:
				exclen = 8;
				break;
			default:
				end_test();
				printf("Unknown frame %04x\n", frame);
				exit(0);
				break;
			}
		}
		last_exception_len = exclen;
		if (p != op + excdatalen + 1) {
			end_test();
			printf("Exception %d length mismatch %d != %d\n", excnum, excdatalen, p - op - 1);
			exit(0);
		}
	} else {
		exclen = last_exception_len;
	}

	exceptioncount[excrwp][*gotexcnum]++;

	if (exclen == 0 || *gotexcnum != excnum)
		return p;
	int err = 0;
	if (compare_exception(exc, sp, exclen, mask_exception, masked_exception)) {
		err = 1;
		if (err && alts > 0) {
			if (alts >= 1 && !compare_exception(alternate_exception1, sp, exclen, mask_exception, masked_exception))
				err = 0;
			if (alts >= 2 && !compare_exception(alternate_exception2, sp, exclen, mask_exception, masked_exception))
				err = 0;
			if (alts >= 3 && !compare_exception(alternate_exception3, sp, exclen, mask_exception, masked_exception))
				err = 0;
		}
	}
	if (err) {
		sprintf(outbp, "Exception %d stack frame mismatch:\n", excnum);
		outbp += strlen(outbp);
		strcpy(outbp, "Expected: ");
		outbp += strlen(outbp);
		hexdump(exc, exclen, 1);
		strcpy(outbp, "Got     : ");
		outbp += strlen(outbp);
		hexdump(sp, exclen, 1);
		*experr |= 1;
	}
	exception_stored = exclen;
	return p;
}

static int getexceptioncycles(int exc)
{
	if (cpu_lvl == 0) {
		switch (exc)
		{
		case 2:
			return 58;
		case 3:
			return 54;
		case 4:
		case 5:
		case 6:
		case 8:
		case 9:
		case 10:
		case 11:
			return 34;
		case 7:
			return 30;
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
			return 44;
		default:
			return 34;
		}
	} else {
		switch (exc)
		{
		case 2:
			return 134;
		case 3:
			return 130;
		case 4:
		case 5:
		case 6:
		case 8:
		case 9:
		case 10:
		case 11:
		case 14:
			return 38;
		case 7:
			return 38;
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
			return 48;
		default:
			return 38;
		}
	}
}

#ifdef AMIGA
// 7MHz 68000 PAL A500 only!
static int get_cycles_amiga(void)
{
	uae_u16 vstart = (test_regs.cycles >> 24) & 0xff;
	uae_u16 vend = (test_regs.cycles >> 8) & 0xff;
	uae_u16 hstart = (test_regs.cycles >> 16) & 0xff;
	uae_u16 hend = (test_regs.cycles >> 0) & 0xff;

	// trace exception?
	if (test_regs.cyclest != 0xffffffff) {
		vend = (test_regs.cyclest >> 8) & 0xff;
		hend = (test_regs.cyclest >> 0) & 0xff;
	}

	if (test_regs.cycles2 & 0x00010000) {
		if (vstart > vend) {
			vstart += 0x100;
			vend += 0x138 + 1;
		} else {
			vstart += 0x100;
			vend += 0x100;
		}
	} else {
		if (vstart > vend) {
			vend += 0x100;
		}
	}

	// hpos 0-1: vertical count hasn't been increased yet
	if (hstart <= 1) {
		vstart++;
	}
	if (hend <= 1) {
		vend++;
	}

	if (hstart >= hend) {
		hend += 227;
		vend--;
	}
	int startcycle = vstart * 227 + hstart;
	int endcycle = vend * 227 + hend;
	int gotcycles = (endcycle - startcycle) * 2;
	return gotcycles;
}

static uae_u16 test_intena, test_intreq;

static const uae_u16 itable[] =
{
	0,
	1 << 2,
	1 << 3,
	1 << 4,
	1 << 7,
	1 << 11,
	1 << 13,
	0
};

static void set_interrupt(void)
{
	if (interrupttest == 1) {
		if (interrupt_count < 15) {
			volatile uae_u16 *intena = (uae_u16*)0xdff09a;
			volatile uae_u16 *intreq = (uae_u16*)0xdff09c;
			uae_u16 mask = 1 << interrupt_count;
			test_intena = mask | 0x8000 | 0x4000;
			test_intreq = mask | 0x8000;
			*intena = test_intena;
			*intreq = test_intreq;
		}
	}
	if (interrupttest == 2) {
		volatile uae_u16 *intena = (uae_u16*)0xdff09a;
		volatile uae_u16 *intreq = (uae_u16*)0xdff09c;
		volatile uae_u16 *serper = (uae_u16*)0xdff032;
		*serper = IPL_TRIGGER_SERPER;
		*intena = 0x8000 | 0x4000 | IPL_TRIGGER_INTMASK;
		*intreq = IPL_TRIGGER_INTMASK;
	}
	if (initial_interrupt) {
		uae_u16 mask = itable[initial_interrupt];
		volatile uae_u16 *intena = (uae_u16*)0xdff09a;
		volatile uae_u16 *intreq = (uae_u16*)0xdff09c;
		*intena = 0x8000 | 0x4000 | mask;
		*intreq = 0x8000 | mask;
	}
}

static void clear_interrupt(void)
{
	volatile uae_u16 *intena = (uae_u16*)0xdff09a;
	volatile uae_u16 *intreq = (uae_u16*)0xdff09c;
	*intena = 0x7fff;
	*intreq = 0x7fff;
}

#endif

static int check_cycles(int exc, short extratrace, short extrag2w1, struct registers *lregs)
{
	int gotcycles = 0;

	if ((test_regs.cycles >> 16) == 0xffff) {
		// cycle counter got exception (interrupt) during exception handling
		// -> cycle counter was invalidated. Ignore cycle count.
		invalidcycles++;
		return 1;
	}

	if (cyclecounter_addr != 0xffffffff) {
		gotcycles = (test_regs.cycles & 0xffff) - (test_regs.cycles >> 16);
		if (gotcycles == 0) {
			end_test();
			printf("Cycle counter hardware address 0x%08x returned zero cycles.\n", cyclecounter_addr);
			exit(0);
		}
	} else {
#ifdef AMIGA
		gotcycles = get_cycles_amiga();
		// if write bus error, decrease cycles by 2. Tester hardware side-effect.
		if (exc == 2) {
			if (cpu_lvl == 0 && (last_exception[1] & 0x10) == 0) {
				gotcycles -= 2;
			}
			if (cpu_lvl == 1 && (last_exception[8] & 0x01) == 0) {
				gotcycles -= 2;
			}
		}
#else
		end_test();
		printf("No cycle count support\n");
		exit(0);
#endif
	}

	int expectedcycles = lregs->cycles;
	int exceptioncycles = getexceptioncycles(exc);
	if (cpu_lvl == 0) {
		// move.w CYCLEREG,cycles
		gotcycles -= 20;
		// RTE
		gotcycles -= 20;
		// <test instruction>
		// EXCEPTION
		expectedcycles += exceptioncycles;
		// bsr.b
		gotcycles -= 18;
		// move.w sr,-(sp)
		gotcycles -= 14;
		// move.w CYCLEREG,cycles
		gotcycles -= 8;
	} else {
		// move.w CYCLEREG,cycles
		gotcycles -= 20;
		// move.w #x,dummy
		gotcycles -= 20;
		// RTE
		gotcycles -= 24;
		// <test instruction>
		// ILLEGAL
		expectedcycles += exceptioncycles;
		// bsr.b
		gotcycles -= 18;
		// move.w sr,-(sp)
		gotcycles -= 12;
		// move.w CYCLEREG,cycles
		gotcycles -= 8;
	}
	gotcycles += cycles_adjust;

	if (extratrace > 0) {
		expectedcycles += getexceptioncycles(9);
	}
	// address error during group 2 exception stacking (=odd exception vector)
	if (extrag2w1) {
		// 4 idle, write pc low, write sr, write pc high, read vector high, read vector low,
		// exception vector fetch that generates address error
		expectedcycles += 7 * 4;
		if (extrag2w1 == 7) {
			// TRAPV has no startup idle cycles
			expectedcycles -= 4;
		}
		if (cpu_lvl == 1) {
			// 68010: frame type
			expectedcycles += 4;
		}
		if (extrag2w1 >= 24 && extrag2w1 < 24 + 8) {
			// interrupt
			expectedcycles += 2 + 4 + 4;
		}
	}

	if (0 || abs(gotcycles - expectedcycles) > cycles_range) {
		sprintf(outbp, "Got %d cycles (%d + %d) but expected %d (%d + %d) %08x\n",
			gotcycles, gotcycles - exceptioncycles, exceptioncycles,
			expectedcycles, expectedcycles - exceptioncycles, exceptioncycles, test_regs.cycles);
		outbp += strlen(outbp);
		return 0;
	}
	return 1;
}

// mostly ugly workarounds for logarithmic/trigonometric functions
// not returning identical values (6888x algorithms are unknown)
static short fpucheckextra(struct fpureg *f1, struct fpureg *f2)
{
	uae_u32 vx[9];

	if (!is_fpu_adjust)
		return 0;

	uae_u32 m1[2];
	uae_u32 m2[2];
	m1[0] = f1->m[0];
	m1[1] = f1->m[1];
	m2[0] = f2->m[0];
	m2[1] = f2->m[1];
	uae_u16 exp1 = f1->exp & 0x7fff;
	uae_u16 exp2 = f2->exp & 0x7fff;
	// NaN or Infinite
	if (exp1 == 0x7fff || exp2 == 0x7fff) {
		if (m1[0] == 0 && m1[1] == 0) {
			if (m2[0] != 0 || m2[1] != 0) {
				return 0;
			}
		}
		if (m2[0] == 0 && m2[1] == 0) {
			if (m1[0] != 0 || m1[1] != 0) {
				return 0;
			}
		}
		return 1;
	}

	// One zero: other must be close enough to zero
	if ((!exp1 && !m1[0] && !m1[1]) || (!exp2 && !m2[0] && !m2[1])) {
		vx[0] = f1->exp << 16;
		vx[1] = f1->m[0];
		vx[2] = f1->m[1];
		vx[3] = f2->exp << 16;
		vx[4] = f2->m[0];
		vx[5] = f2->m[1];
		vx[6] = (16383 - 10) << 16;
		vx[7] = 0x80000000;
		vx[8] = 0x00000000;
		if (fpucompzero(vx)) {
			fpu_approx++;
			return 1;
		}
		return 0;
	}

	vx[0] = f1->exp << 16;
	vx[1] = f1->m[0];
	vx[2] = f1->m[1];
	vx[3] = f2->exp << 16;
	vx[4] = f2->m[0];
	vx[5] = f2->m[1];
	vx[6] = fpu_adjust_exp << 16;
	vx[7] = 0x80000000;
	vx[8] = 0x00000000;

	if (fpucomp(vx)) {
		fpu_approx++;
		return 1;
	}
	return 0;
}

static void loop_mode_error(uae_u32 ov, uae_u32 nv)
{
	strcpy(outbp, " expected ");
	outbp += strlen(outbp);
	for (short i = loop_mode_cnt - 1; i >= 0; i--) {
		if ((ov >> i) & 1) {
			*outbp++ = '1';
		} else {
			*outbp++ = '0';
		}
	}
	strcpy(outbp, " got ");
	outbp += strlen(outbp);
	for (short i = loop_mode_cnt - 1; i >= 0; i--) {
		if ((nv >> i) & 1) {
			*outbp++ = '1';
		} else {
			*outbp++ = '0';
		}
	}
	*outbp++ = '\n';
}

static const char cvzn[] = { "CVZN" };
static void loop_mode_error_CVZN(uae_u32 ov, uae_u32 nv)
{
	// Swap V<>Z
	ov = (ov & 0xff0000ff) | ((ov >> 8) & 0x0000ff00) | ((ov << 8) & 0x00ff0000);
	nv = (nv & 0xff0000ff) | ((nv >> 8) & 0x0000ff00) | ((nv << 8) & 0x00ff0000);

	for (short i = 0; i < 4; i++) {
		if ((ov & 0xff) != (nv & 0xff)) {
			sprintf(outbp, "Loop mode %c:", cvzn[i]);
			outbp += strlen(outbp);
			loop_mode_error(ov, nv);
		}
		ov >>= 8;
		nv >>= 8;
	}
}
static void loop_mode_error_X(uae_u32 ov, uae_u32 nv)
{
	sprintf(outbp, "Loop mode X:");
	outbp += strlen(outbp);
	ov >>= 4;
	nv >>= 4;
	loop_mode_error(ov, nv);
}

static uae_u16 lmtable1[LM_BUFFER / 2];
static uae_u16 lmtable2[LM_BUFFER / 2];

// sregs: registers before execution of test code
// tregs: registers used during execution of test code, also modified by test code.
// lregs: registers after modifications from data files. Test ok if tregs == lregs.

static uae_u8 *validate_test(uae_u8 *p, short ignore_errors, short ignore_sr, struct registers *sregs, struct registers *tregs, struct registers *lregs, int opcodeendsizeextra)
{
	uae_u8 regs_changed[16] = { 0 }, regs_changed_any = 0;
	uae_u8 regs_fpuchanged[8] = { 0 }, regs_fpuchanged_any = 0;
	uae_u8 sr_changed = 0, ccr_changed = 0, pc_changed = 0;
	uae_u8 fpiar_changed = 0, fpsr_changed = 0, fpcr_changed = 0;
	short exc = -1;

	if (loop_mode_jit) {
		memset(lmtable1, 0xff, sizeof(lmtable1));
		memset(lmtable2, 0xff, sizeof(lmtable2));
	}

	// Register modified in test? (start register != test register)
	for (short i = 0; i < 16; i++) {
		if (sregs->regs[i] != tregs->regs[i]) {
			regs_changed[i] = 1;
			regs_changed_any = 1;
		}
	}
	if ((sregs->sr & test_ccrignoremask & 0xff00) != (tregs->sr & test_ccrignoremask & 0xff00)) {
		sr_changed = 1;
	}
	if ((sregs->sr & test_ccrignoremask & ccr_mask & 0x00ff) != (tregs->sr & test_ccrignoremask & ccr_mask & 0x00ff)) {
		ccr_changed = 1;
	}
	if (sregs->pc + opcodeendsizeextra != tregs->pc) {
		pc_changed = 1;
	}
	if (fpu_model) {
		for (short i = 0; i < 8; i++) {
			if (memcmp(&sregs->fpuregs[i], &tregs->fpuregs[i], sizeof(struct fpureg))) {
				regs_fpuchanged[i] = 1;
				regs_fpuchanged_any = 1;
			}
		}
		if (sregs->fpsr != tregs->fpsr) {
			fpsr_changed = 1;
		}
		if (sregs->fpcr != tregs->fpcr) {
			fpcr_changed = 1;
		}
		if (sregs->fpiar != tregs->fpiar) {
			fpiar_changed = 1;
		}
	}

	if (*p == CT_END_SKIP)
		return p + 1;

	short experr = 0;
	int errflag = 0;
	int errflag_orig = 0;
	short extratrace = 0;
	short extrag2w1 = 0;
	short exceptionnum = 0;
	uae_u8 *outbp_old = outbp;
	short branched = 0, branched2 = 0;
	exception_stored = 0;

	for (;;) {
		uae_u8 v = *p;
		if (v & CT_END) {
			exc = v & CT_EXCEPTION_MASK;
			short cpuexc = tregs->exc & 65535;
			short cpuexc010 = tregs->exc010 & 65535;
			branched = (v & CT_BRANCHED) != 0;
			branched2 = branched;
			p++;
			if (cpu_lvl > 0 && exc >= 2 && cpuexc010 != cpuexc) {
				if (dooutput) {
					sprintf(outbp, "Exception: vector number does not match vector offset! (%d <> %d) %d\n", exc, cpuexc010, cpuexc);
					experr |= 1;
					outbp += strlen(outbp);
					errflag |= 1 << 16;
				}
				break;
			}
			if (ignore_errors) {
				if (exc) {
					p = validate_exception(&test_regs, p, exc, &cpuexc, &experr, &extratrace, &extrag2w1);
				}
				errflag_orig |= errflag;
				errflag = 0;
				break;
			}
			if (exc == 0 && cpuexc == 4) {
				// successful complete generates exception 4 with matching PC
				if (lregs->pc + opcodeendsizeextra != tregs->pc) {
					branched2 = lregs->pc < opcode_memory_addr || lregs->pc >= opcode_memory_addr + OPCODE_AREA;
					if (dooutput) {
						int excsize;
						uae_u8 *excp;
						sprintf(outbp, "PC (%c): expected %08x but got %08x ", branched ? 'B' : '-', lregs->pc, tregs->pc);
						outbp += strlen(outbp);
						if (tregs->pc == opcode_memory_addr) {
							sprintf(outbp, "Got unexpected exception %d (unsupported instruction?) ", cpuexc);
						} else {
							sprintf(outbp, "Got unexpected exception %d ", cpuexc);
						}
						outbp += strlen(outbp);
						excp = get_exceptionframe(&test_regs, cpuexc, &excsize);
						if (excp && excsize) {
							hexdump(excp, excsize, 1);
						}
					}
					errflag |= 1 << 16;
				}
				break;
			}
			if (exc) {
				p = validate_exception(&test_regs, p, exc, &cpuexc, &experr, &extratrace, &extrag2w1);
				if (experr) {
					errflag |= 1 << 16;
					errflag_orig |= errflag;
				}
				if (basicexcept && (cpuexc == 2 || cpuexc == 3)) {
					errflag &= ~(1 << 0);
					errflag &= ~(1 << 7);
				}
				exceptionnum = cpuexc;
			}
			if (exc != cpuexc && exc >= 2) {
				if (dooutput) {
					if (cpuexc == 4 && lregs->pc + opcodeendsizeextra == tregs->pc) {
						sprintf(outbp, "Exception: expected %d but got no exception.\n", exc);
						outbp += strlen(outbp);
					} else if (cpuexc == 4) {
						sprintf(outbp, "Exception: expected %d but got %d (or no exception)\n", exc, cpuexc);
						outbp += strlen(outbp);
					} else {
						int excsize;
						uae_u8 *excp;
						sprintf(outbp, "Exception: expected %d but got %d\n", exc, cpuexc);
						outbp += strlen(outbp);
						excp = get_exceptionframe(&test_regs, cpuexc, &excsize);
						if (excp && excsize) {
							hexdump(excp, excsize, 1);
						}
					}
					if ((experr & 2) && !(experr & 1) && extratrace) {
						sprintf(outbp, "Valid trace exception found\n");
						outbp += strlen(outbp);
					}
					experr |= 2;
				}
				errflag |= 1 << 16;
			}
			break;
		}
		short mode = v & CT_DATA_MASK;

		if (mode < CT_AREG + 8 && (v & CT_SIZE_MASK) != CT_SIZE_FPU) {
			uae_u32 val = lregs->regs[mode];
			int size;
			p = restore_value(p, &val, &size);
			if (val != tregs->regs[mode]) {
				if (!ignore_errors && !skipregchange) {
					if (dooutput) {
						if (sregs->regs[mode] == tregs->regs[mode]) {
							sprintf(outbp, "%c%d: expected %08x but register was not modified\n", mode < CT_AREG ? 'D' : 'A', mode & 7, val);
						} else {
							sprintf(outbp, "%c%d: expected %08x but got %08x\n", mode < CT_AREG ? 'D' : 'A', mode & 7, val, tregs->regs[mode]);
						}
						outbp += strlen(outbp);
					}
					errflag |= 1 << 0;
				}
			}
			lregs->regs[mode] = val;
			regs_changed[mode] = 0;
		} else if (mode < CT_AREG && (v & CT_SIZE_MASK) == CT_SIZE_FPU) {
			struct fpureg val;
			memcpy(&val, &lregs->fpuregs[mode], sizeof(struct fpureg));
			p = restore_fpvalue(p, &val);
			if (memcmp(&val, &tregs->fpuregs[mode], sizeof(struct fpureg))) {
				if (!ignore_errors && !skipregchange && !fpucheckextra(&val, &tregs->fpuregs[mode])) {
					if (dooutput) {
						if (!memcmp(&sregs->fpuregs, &tregs->fpuregs[mode], sizeof(struct fpureg))) {
							sprintf(outbp, "FP%d: expected %04x-%08x%08x but register was not modified\n", mode,
								val.exp, val.m[0], val.m[1]);
						} else {
							sprintf(outbp, "FP%d: expected %04x-%08x%08x but got %04x-%08x%08x\n", mode,
								val.exp, val.m[0], val.m[1],
								tregs->fpuregs[mode].exp, tregs->fpuregs[mode].m[0], tregs->fpuregs[mode].m[1]);
						}
						outbp += strlen(outbp);
					}
					errflag |= 1 << 1;
				}
			}
			memcpy(&lregs->fpuregs[mode], &val, sizeof(struct fpureg));
			regs_fpuchanged[mode] = 0;
		} else if (mode == CT_SR) {
			uae_u32 val = lregs->sr;
			int size;
			// High 16 bit: ignore mask, low 16 bit: SR/CCR
			p = restore_value(p, &val, &size);
			test_ccrignoremask = ~(val >> 16);
			if ((val & ccr_mask & (sr_undefined_mask & test_ccrignoremask)) != (tregs->sr & ccr_mask & (sr_undefined_mask & test_ccrignoremask)) && !ignore_errors && !ignore_sr) {
				if (dooutput) {
					sprintf(outbp, "SR: expected %04x -> %04x but got %04x", sregs->sr & 0xffff, val & 0xffff, tregs->sr & 0xffff);
					outbp += strlen(outbp);
					if (test_ccrignoremask != 0xffff) {
						sprintf(outbp, " (%04x)", test_ccrignoremask);
						outbp += strlen(outbp);
					}
					*outbp++ = '\n';
					errflag |= 1 << 2;
					errflag |= 1 << 7;
				}
				// SR check
				uae_u16 mask = test_ccrignoremask & 0xff00;
				if ((val & (sr_undefined_mask & mask)) == (tregs->sr & (sr_undefined_mask & mask))) {
					errflag &= ~(1 << 2);
				}
				// CCR check
				mask = test_ccrignoremask & 0x00ff;
				if (skipccrchange || ((val & (sr_undefined_mask & mask)) == (tregs->sr & (sr_undefined_mask & mask)))) {
					errflag &= ~(1 << 7);
				}
			}
			sr_changed = 0;
			ccr_changed = 0;
			if (!(tregs->expsr & 0x2000)) {
				sprintf(outbp, "SR S-bit is not set at start of exception handler!\n");
				outbp += strlen(outbp);
				errflag |= 1 << 16;
			}
			if ((tregs->expsr & 0xff) != (tregs->sr & 0xff)) {
				sprintf(outbp, "Exception stacked CCR (%04x) != CCR (%04x) at start of exception handler!\n", tregs->sr, tregs->expsr);
				outbp += strlen(outbp);
				errflag |= 1 << 16;
			}
			lregs->sr = val;
		} else if (mode == CT_PC) {
			uae_u32 val = lregs->pc;
			p = restore_rel(p, &val, 1);
			pc_changed = 0;
			lregs->pc = val;
		} else if (mode == CT_CYCLES) {
			uae_u32 val = lregs->cycles;
			int size;
			p = restore_value(p, &val, &size);
			lregs->cycles = val;
			gotcycles = 1;
		} else if (mode == CT_FPCR) {
			uae_u32 val = lregs->fpcr;
			int size;
			p = restore_value(p, &val, &size);
			if (val != tregs->fpcr) {
				if (!ignore_errors) {
					if (dooutput) {
						if (sregs->fpcr == tregs->fpcr) {
							sprintf(outbp, "FPCR: expected %08x but register was not modified\n", val);
						} else {
							sprintf(outbp, "FPCR: expected %08x -> %08x but got %08x\n", sregs->fpcr, val, tregs->fpcr);
						}
						outbp += strlen(outbp);
					}
					errflag |= 1 << 3;
				}
			}
			lregs->fpcr = val;
			fpcr_changed = 0;
		} else if (mode == CT_FPSR) {
			uae_u32 val = lregs->fpsr;
			int size;
			p = restore_value(p, &val, &size);
			if (val != tregs->fpsr) {
				if (!ignore_errors && ((val & fpsr_ignore_mask) != (tregs->fpsr & fpsr_ignore_mask))) {
					if (dooutput) {
						if (sregs->fpsr == tregs->fpsr) {
							sprintf(outbp, "FPSR: expected %08x but register was not modified\n", val);
						} else {
							sprintf(outbp, "FPSR: expected %08x -> %08x but got %08x\n", sregs->fpsr, val, tregs->fpsr);
						}
						outbp += strlen(outbp);
					}
					errflag |= 1 << 4;
				}
			}
			lregs->fpsr = val;
			fpsr_changed = 0;
		} else if (mode == CT_FPIAR) {
			uae_u32 val = lregs->fpiar;
			int size;
			p = restore_value(p, &val, &size);
			if (val != tregs->fpiar) {
				if (!ignore_errors && !fpiar_ignore) {
					if (dooutput) {
						if (sregs->fpiar == tregs->fpiar) {
							sprintf(outbp, "FPIAR: expected %08x but register was not modified\n", val);
						} else {
							sprintf(outbp, "FPIAR: expected %08x but got %08x\n", val, tregs->fpiar);
						}
						outbp += strlen(outbp);
					}
					errflag |= 1 << 5;
				}
			}
			lregs->fpiar = val;
			fpiar_changed = 0;
		} else if (mode == CT_MEMWRITES) {
			p = restore_memory(p, 0);
		} else if (mode == CT_MEMWRITE) {
			uae_u8 *addr;
			uae_u32 val = 0, mval = 0, oldval = 0;
			int size;
			short lm = 0;
			p = get_memory_addr(p, &addr);
			p = restore_value(p, &oldval, &size);
			p = restore_value(p, &val, &size);
			switch(size)
			{
				case 0:
				mval = addr[0];
				if (mval != val && !ignore_errors && !skipmemwrite) {
					if (dooutput) {
						sprintf(outbp, "Memory byte write: address %08x, expected %02x but got %02x\n", (uae_u32)addr, val, mval);
						outbp += strlen(outbp);
					}
					errflag |= 1 << 6;
				}
				addr[0] = oldval;
				break;
				case 1:
				mval = (addr[0] << 8) | (addr[1]);
				if (loop_mode_jit && addr >= test_memory && addr < test_memory + LM_BUFFER) {
					lmtable1[(addr - test_memory) / 2] = val;
					lmtable2[(addr - test_memory) / 2] = mval;
					lm = 1;
				}
				if (mval != val && !ignore_errors && !skipmemwrite) {
					if (dooutput) {
						if (!lm || (lm && (val == 0xffff || mval == 0xffff))) {
							sprintf(outbp, "Memory word write: address %08x, expected %04x but got %04x\n", (uae_u32)addr, val, mval);
						}
						outbp += strlen(outbp);
					}
					errflag |= 1 << 6;
				}
				addr[0] = oldval >> 8;
				addr[1] = oldval;
				break;
				case 2:
				mval = gl(addr);
				if (mval != val && !ignore_errors && !skipmemwrite) {
					if (dooutput) {
						sprintf(outbp, "Memory long write: address %08x, expected %08x but got %08x\n", (uae_u32)addr, val, mval);
						outbp += strlen(outbp);
					}
					errflag |= 1 << 6;
				}
				pl(addr, oldval);
				break;
			}
		} else {
			end_test();
			printf("Unknown test data %02x mode %d\n", v, mode);
			exit(0);
		}
	}
	if (!ignore_errors) {
		if (!skipregchange && regs_changed_any) {
			for (short mode = 0; mode < 16; mode++) {
				if (regs_changed[mode] && tregs->regs[mode] != lregs->regs[mode]) {
					if (dooutput) {
						uae_u32 val = lregs->regs[mode];
						sprintf(outbp, "%c%d: expected %08x but got %08x\n", mode < CT_AREG ? 'D' : 'A', mode & 7, val, tregs->regs[mode]);
						outbp += strlen(outbp);
					}
					errflag |= 1 << 0;
				}
			}
		}
		if (!ignore_sr) {
			if (sr_changed && tregs->sr != lregs->sr) {
				if (dooutput) {
					sprintf(outbp, "SR: modified %04x -> %04x but expected %04x\n", sregs->sr & 0xffff, tregs->sr & 0xffff, lregs->sr & 0xffff);
					outbp += strlen(outbp);
				}
				errflag |= 1 << 2;
			} else if (ccr_changed && !skipccrchange && (tregs->sr & ccr_mask & 0xff) != (lregs->sr & ccr_mask & 0xff)) {
				if (dooutput) {
					sprintf(outbp, "SR: modified %04x -> %04x but expected %04x\n", sregs->sr & 0xffff, tregs->sr & 0xffff, lregs->sr & 0xffff);
					outbp += strlen(outbp);
				}
				errflag |= 1 << 2;
			}
		}
		if (!skipregchange && regs_fpuchanged_any) {
			for (short mode = 0; mode < 8; mode++) {
				if (regs_fpuchanged[mode] && memcmp(&tregs->fpuregs[mode], &lregs->fpuregs[mode], sizeof(struct fpureg))) {
					if (!fpucheckextra(&tregs->fpuregs[mode], &lregs->fpuregs[mode])) {
						if (dooutput) {
							struct fpureg *val = &lregs->fpuregs[mode];
							sprintf(outbp, "FP%d: expected %04x-%08x%08x but got %04x-%08x%08x\n", mode,
								val->exp, val->m[0], val->m[1],
								tregs->fpuregs[mode].exp, tregs->fpuregs[mode].m[0], tregs->fpuregs[mode].m[1]);
							outbp += strlen(outbp);
						}
						errflag |= 1 << 1;
					}
				}
			}
		}
		if (!ignore_sr) {
			if (fpsr_changed && (tregs->fpsr & fpsr_ignore_mask) != (lregs->fpsr & fpsr_ignore_mask)) {
				if (dooutput) {
					uae_u32 val = lregs->fpsr;
					sprintf(outbp, "FPSR: expected %08x -> %08x but got %08x\n", sregs->fpsr, val, tregs->fpsr);
					outbp += strlen(outbp);
				}
				errflag |= 1 << 3;
			}
		}
		if (fpcr_changed && tregs->fpcr != lregs->fpcr) {
			if (dooutput) {
				uae_u32 val = lregs->fpcr;
				sprintf(outbp, "FPCR: expected %08x -> %08x but got %08x\n", sregs->fpcr, val, tregs->fpcr);
				outbp += strlen(outbp);
			}
			errflag |= 1 << 4;
		}
		if (fpiar_changed && tregs->fpiar != lregs->fpiar && !fpiar_ignore) {
			if (dooutput) {
				uae_u32 val = lregs->fpiar;
				sprintf(outbp, "FPIAR: expected %08x but got %08x\n", val, tregs->fpiar);
				outbp += strlen(outbp);
			}
			errflag |= 1 << 5;
		}
		if (cycles && cpu_lvl <= 1) {
			if (!gotcycles && errflag) {
				if (dooutput) {
					sprintf(outbp, "No Cycle count data available.\n");
					outbp += strlen(outbp);
				}
			} else {
				if (!check_cycles(exc, extratrace, extrag2w1, lregs)) {
					errflag |= 1 << 8;
				}
			}
		}
		errflag_orig |= errflag;
	}

	if (loop_mode_jit) {
		short idx = 0, cnt = 0, end = loop_mode_cnt;
		for (;;) {
			uae_u16 v1a = lmtable1[idx];
			uae_u16 v2a = lmtable2[idx];
			uae_u16 v1b = lmtable1[idx + 1];
			uae_u16 v2b = lmtable2[idx + 1];
			idx++;
			if (loop_mode_jit == 3 || loop_mode_jit == 4) {
				idx++;
			}
			if (idx >= LM_BUFFER / 2) {
				break;
			}
			if (v1a == 0xffff && v2a == 0xffff) {
				continue;
			}
			cnt++;
			if (v1a != v2a) {
				if (loop_mode_jit == 3 || loop_mode_jit == 4) {
					static const char *constss[] = { "C", "Z", "V", "N", "X" };
					const char *c = "?";
					if (v1b < 5 && v1b == v2b) {
						c = constss[v1b];
					}
					sprintf(outbp, "LM %02d/%02d %s: %d %c %d (%04x %04x - %04x %04x)\n",
						cnt, end, c, (v1a & 0xff) != 0, v1a != v2a ? '!' : '=', (v2a & 0xff) != 0,
						v1a, v1b, v2a, v2b);
					outbp += strlen(outbp);
				} else if (loop_mode_jit == 1 || loop_mode_jit == 2) {
					sprintf(outbp, "LM %02d/%02d CCR: %02x != %02x",
						cnt, end, v1a, v2a);
					outbp += strlen(outbp);
					sprintf(outbp, " X%c%d N%c%d Z%c%d V%c%d C%c%d\n",
						(v1a & 0x10) != (v2a & 0x10) ? '!' : '=', (v2a & 0x10) != 0,
						(v1a & 0x08) != (v2a & 0x08) ? '!' : '=', (v2a & 0x08) != 0,
						(v1a & 0x04) != (v2a & 0x04) ? '!' : '=', (v2a & 0x04) != 0,
						(v1a & 0x02) != (v2a & 0x02) ? '!' : '=', (v2a & 0x02) != 0,
						(v1a & 0x01) != (v2a & 0x01) ? '!' : '=', (v2a & 0x01) != 0);
					outbp += strlen(outbp);
				}
			}
			if (cnt >= end) {
				break;
			}
		}
	}

	// if excskipccr + bus, address, divide by zero or chk + only CCR mismatch detected: clear error
	if (excskipccr && errflag == (1 << 7) && (exceptionnum == 2 || exceptionnum == 3 || exceptionnum == 5 || exceptionnum == 6)) {
		errflag = 0;
	}

	if (exitcnt == 0 && !exitmode && !errflag) {
		errflag = 1;
	}

	if (errflag && dooutput) {
		outbp = outbuffer;
		addinfo();
		strcpy(outbp, outbuffer2);
		strcat(outbp, "Registers before:\n");
		outbp += strlen(outbp);
		out_regs(sregs, tregs, lregs, sregs, 1, branched);
		strcat(outbp, "Registers after:\n");
		outbp += strlen(outbp);
		out_regs(tregs, tregs, lregs, sregs, 0, branched2);
#ifdef AMIGA
		if (interrupttest == 1) {
			sprintf(outbp, "INTREQ: %04x INTENA: %04x\n", test_intreq, test_intena);
			outbp += strlen(outbp);
		}
#endif
		if (exc > 1) {
			if (!(experr & 1)) {
				sprintf(outbp, "OK: exception %d ", exc);
				outbp += strlen(outbp);
				if (exception_stored) {
					hexdump(last_exception, exception_stored, 0);
				}
				if (extrag2w1) {
					sprintf(outbp, " (+EXC %d)", extrag2w1);
					outbp += strlen(outbp);
				}
				*outbp++ = '\n';
			}
			if ((exc == 3 || exc == 2) && cpu_lvl == 0) {
				sprintf(outbp, "RW=%d IN=%d FC=%d\n",
					((test_regs.exc >> (16 + 4)) & 1),
					((test_regs.exc >> (16 + 3)) & 1),
					((test_regs.exc >> (16 + 0)) & 7));
				outbp += strlen(outbp);
			}
		} else if (exc == 0 && (test_regs.exc & 65535) == 4 && lregs->pc == test_regs.pc) {
			sprintf(outbp, "OK: No exception generated\n");
			outbp += strlen(outbp);
		}
		errors++;
	}
	if (!errflag && errflag_orig && dooutput) {
		outbp = outbp_old;
		*outbp = 0;
		infoadded = 0;
	}
	return p;
}

static void out_endinfo(void)
{
	sprintf(outbp, "%u (%u/%u/%u) ", testcnt, testcntsub, testcntsubmax, interrupt_delay_cnt);
	outbp += strlen(outbp);
	sprintf(outbp, "S=%d", supercnt);
	outbp += strlen(outbp);
	for (short i = 0; i < 128; i++) {
		if (exceptioncount[0][i] || exceptioncount[1][i] || exceptioncount[2][i]) {
			if (i == 2 || i == 3) {
				sprintf(outbp, " E%02d=%d/%d/%d", i, exceptioncount[0][i], exceptioncount[1][i], exceptioncount[2][i]);
			} else {
				sprintf(outbp, " E%02d=%d", i, exceptioncount[0][i]);
			}
			outbp += strlen(outbp);
		}
	}
	if (invalidcycles) {
		sprintf(outbp, " INVCYC=%d", invalidcycles);
		outbp += strlen(outbp);
	}
	strcat(outbp, "\n");
	outbp += strlen(outbp);
	if (interrupttest == 2) {
		for (short i = 0; i < MAX_IPL_PC_VALUES; i++) {
			if (ipl_pc[i]) {
				sprintf(outbp, "%08x:%d ", ipl_pc[i], ipl_pc_cnt[i]);
				outbp += strlen(outbp);
			}
		}
		strcat(outbp, "\n");
		outbp += strlen(outbp);
	}
	if (fpu_approxcnt) {
		sprintf(outbp, "FPU approximate matches: %d\n", fpu_approxcnt);
	}
	supercnt = 0;
	memset(exceptioncount, 0, sizeof(exceptioncount));
}

static void store_addr(uae_u32 s, uae_u8 *d)
{
	if (s == 0xffffffff)
		return;
	for (int i = 0; i < SIZE_STORED_ADDRESS; i++) {
		uae_u32 ss = s + (i - SIZE_STORED_ADDRESS_OFFSET);
		if (is_valid_test_addr_read(ss)) {
			*d++ = *((uae_u8*)ss);
		} else {
			*d++ = 0;
		}
	}
}

static uae_u32 xorshiftstate;
static uae_u32 xorshift32(void)
{
	uae_u32 x = xorshiftstate;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xorshiftstate = x;
	return xorshiftstate;
}

static void copyregs(struct registers *d, struct registers *s, short fpumode)
{
	if (fpumode) {
		memcpy(d->regs, s->regs, offsetof(struct registers, fsave));
	} else {
		memcpy(d->regs, s->regs, offsetof(struct registers, fpuregs));
	}
}

static void process_test(uae_u8 *p)
{
	outbp = outbuffer2;
	outbp[0] = 0;
	infoadded = 0;
	errors = 0;

	memset(&cur_regs, 0, sizeof(struct registers));
	cur_regs.sr = interrupt_mask << 8;
	cur_regs.srcaddr = 0xffffffff;
	cur_regs.dstaddr = 0xffffffff;
	cur_regs.branchtarget = 0xffffffff;

	if (safe_memory_mode) {
		set_berr(safe_memory_mode, 0);
	}

	endpc = opcode_memory_addr;
	startpc = opcode_memory_addr;
	start_test();

	test_ccrignoremask = 0xffff;

#ifdef AMIGA
	interrupt_count = 0;
	clear_interrupt();
#endif
	ahcnt = 0;

	short doopcodeswap = 1;

	if (interrupttest >= 2) {
		doopcodeswap = 0;
	}

	for (;;) {

		cur_regs.endpc = endpc;
		cur_regs.pc = startpc;

		debug_item_data = p;
		debug_item_count = 0;
		for (;;) {
			uae_u8 v = *p;
			if (v == CT_END_INIT || v == CT_END_FINISH)
				break;
			p = restore_data(p, &cur_regs);
			debug_item_count++;
		}
		debug_item_count = -1;
		if (*p == CT_END_FINISH)
			break;
		p++;

		int stackcopysize = 0;
		for (short i = 0; i < 32; i += 2) {
			if (!is_valid_test_addr_readwrite(cur_regs.regs[15] + i))
				break;
			stackcopysize += 2;
		}

		store_addr(cur_regs.srcaddr, srcaddr);
		store_addr(cur_regs.dstaddr, dstaddr);
		store_addr(cur_regs.branchtarget, branchtarget);
		startpc = cur_regs.pc;
		endpc = cur_regs.endpc;
		opcode_memory_end = (uae_u8*)endpc;

		int fpumode = fpu_model && (opcode_memory[0] & 0xf0) == 0xf0;
		if (fpumode) {
			uae_u32 v = 0;
			if (fpu_max_precision == 2) {
				v = 2 << 6;
			} else if (fpu_max_precision == 1) {
				v = 1 << 6;
			}
			initfpu(v);
		}
		copyregs(&last_regs, &cur_regs, fpumode);

		uae_u32 originalopcodeend = (NOP_OPCODE << 16) | ILLG_OPCODE;
		short opcodeendsizeextra = 0;
		uae_u32 opcodeend = originalopcodeend;
		int extraccr = 0;
		int validendsize = 0;
		if (is_valid_test_addr_readwrite((uae_u32)opcode_memory_end + 2)) {
			validendsize = 2;
		} else if (is_valid_test_addr_readwrite((uae_u32)opcode_memory_end)) {
			validendsize = 1;
		}

		int old_super = -1;

		for (;;) {
			uae_u16 sr_mask = 0;

			if (extraccr & 1)
				sr_mask |= 0x2000; // S
			if (extraccr & 2)
				sr_mask |= 0x4000; // T0
			if (extraccr & 4)
				sr_mask |= 0x8000; // T1
			if (extraccr & 8)
				sr_mask |= 0x1000; // M

			uae_u8 ccrmode = *p++;
			short maxccr = ccrmode & 0x3f;
			short ccrshift = 0;
			while ((maxccr - 1) & (1 << ccrshift)) {
				ccrshift++;
			}
			ccrshift--;
			testcntsubmax = maxccr;
			testcntsub = 0;
			for (short ccrcnt = 0; ccrcnt < maxccr; ccrcnt++, testcntsub++) {
				short ccr = ccrcnt & (maxccr - 1);
				fpu_approx = 0;

				if (doopcodeswap) {
					opcodeend = (opcodeend >> 16) | (opcodeend << 16);
				}
				opcodeendsizeextra = (opcodeend >> 16) == NOP_OPCODE ? 2 : 0;
				if (validendsize == 2) {
					pl(opcode_memory_end, opcodeend);
				} else if (validendsize == 1) {
					pw(opcode_memory_end, opcodeend >> 16);
				}

				if (cur_regs.branchtarget != 0xffffffff && !(cur_regs.branchtarget & 1)) {
					if (cur_regs.branchtarget_mode == 1) {
						uae_u32 bv = gl((uae_u8*)cur_regs.branchtarget);
						bv = (bv >> 16) | (bv << 16);
						pl((uae_u8*)cur_regs.branchtarget, bv);
					} else if (cur_regs.branchtarget_mode == 2) {
						uae_u16 bv = gw((uae_u8*)cur_regs.branchtarget);
						if (bv == NOP_OPCODE)
							bv = ILLG_OPCODE;
						else
							bv = NOP_OPCODE;
						pw((uae_u8*)cur_regs.branchtarget, bv);
					}
				}

				cur_regs.ssp = super_stack_memory - 0x80;
				cur_regs.msp = super_stack_memory;
				cur_regs.fpiar = 0xffffffff;

				copyregs(&test_regs, &cur_regs, fpumode);

				test_regs.pc = startpc;
				test_regs.cyclest = 0xffffffff;
				test_regs.fpeaset = 0;
				test_regs.blttemp = blt_data;

#ifdef M68K
				if (stackcopysize > 0)
					xmemcpy((void*)test_regs.ssp, (void*)test_regs.regs[15], stackcopysize);
#endif
				if (maxccr >= 32) {
					test_regs.sr = ccr & 0xff;
				} else {
					test_regs.sr = (ccr & 1) ? 31 : 0;
				}
				test_regs.sr |= sr_mask | (initial_interrupt_mask << 8);
				if (fpumode) {
					test_regs.fpcr = 0;
					test_regs.fpsr = 0;
					if (maxccr < 16) {
						// all on/all off
						test_regs.fpsr = ((ccr & 1) ? 15 : 0) << 24;
						test_regs.fpcr = ((ccr & 1) ? 15 : 0) << 4;
					} else {
						if (ccrmode & 0x40) {
							// condition codes
							test_regs.fpsr = (ccr & 15) << 24;
						} else {
							// precision and rounding
							test_regs.fpcr = (ccr & 15) << 4;
						}
					}
				}

#ifdef AMIGA
				if (interrupttest == 1) {
					interrupt_count = *p++;
				}
#endif

				while ((*p) == CT_OVERRIDE_REG) {
					p++;
					uae_u8 v = *p++;
					uae_u8 r = v & CT_DATA_MASK;
					uae_u8 size = v & CT_SIZE_MASK;
					if (r == CT_SR) {
						if (size == CT_SIZE_BYTE) {
							test_regs.sr &= 0xff00;
							test_regs.sr |= *p++;
						} else {
							test_regs.sr = (*p++) << 8;
							test_regs.sr |= *p++;
						}
						cur_regs.sr = test_regs.sr;
					} else if (r == CT_FPSR) {
						cur_regs.fpsr = test_regs.fpsr = gl(p);
						p += 4;
					} else if (r == CT_FPCR) {
						cur_regs.fpcr = test_regs.fpcr = gl(p);
						p += 4;
					} else if (r == CT_FPIAR) {
						cur_regs.fpiar = test_regs.fpiar = gl(p);
						p += 4;
					} else if (r >= CT_DREG && r < CT_DREG + 16) {
						if (size == CT_SIZE_FPU) {
							struct fpureg *f = &test_regs.fpuregs[r - CT_DREG];
							uae_u32 val = gl(p);
							f->exp = val >> 16;
							f->dummy = val;
							f->m[0] = gl(p + 4);
							f->m[1] = gl(p + 8);
							p += 12;
						} else {
							cur_regs.regs[r - CT_DREG] = gl(p);
							p += 4;
						}
					} else {
						end_test();
						printf("Unknown override register %02x!\n", v);
						exit(0);
					}
				}

				test_regs.expsr = test_regs.sr | 0x2000;

				// internally modified registers become part of cur_regs
				cur_regs.sr = test_regs.sr;
				cur_regs.expsr = test_regs.expsr;
				cur_regs.fpsr = test_regs.fpsr;
				cur_regs.fpcr = test_regs.fpcr;
				if (!sr_mask && !ccr) {
					last_regs.sr = test_regs.sr;
					last_regs.expsr = test_regs.expsr;
					last_regs.fpsr = test_regs.fpsr;
					last_regs.fpcr = test_regs.fpcr;
				}

				int super = (test_regs.sr & 0x2000) != 0;

				if (super != old_super) {
					stackaddr_ptr = super ? test_regs.ssp : test_regs.regs[15];
					store_addr(stackaddr_ptr, stackaddr);
					old_super = super;
				}

				if (exitcnt >= 0) {
					exitcnt--;
					if (!exitcnt && exitmode) {
						goto end;
					}
				}

				if ((*p) == CT_END_SKIP) {

					p++;

				} else {

					short ignore_errors = 0;
					short ignore_sr = 0;

					if ((ccr_mask & ccr) || (ccr == 0)) {

						reset_error_vectors();

#if 0
						volatile int *tn = (volatile int*)0x100;
						*tn = testcnt;
#endif
#if 0
						uae_u32 *sx = (uae_u32*)startpc;
						if (*sx == 0xf2364c98) {
							volatile uae_u16 *tn = (volatile uae_u16*)0x70000;
							*tn = 0x1234;
						}
#endif


#ifdef AMIGA
						if (interrupttest || initial_interrupt_mask || initial_interrupt) {
							set_interrupt();
						}
#endif

						if (cpu_lvl == 1) {
							execute_test010(&test_regs);
						} else if (cpu_lvl >= 2) {
							flushcache(cpu_lvl);
							if (fpu_model)
								execute_testfpu(&test_regs);
							else
								execute_test020(&test_regs);
						} else {
							execute_test000(&test_regs);
						}

#ifdef AMIGA
						if (interrupttest || initial_interrupt_mask || initial_interrupt) {
							clear_interrupt();
						}
#endif

						if (ccr_mask == 0 && ccr == 0)
							ignore_sr = 1;

						set_error_vectors();

					} else {

						ignore_errors = 1;
						ignore_sr = 1;

					}

#if 0
					if ((*p) == CT_SKIP_REGS) {
						p++;
						for (int i = 0; i < 16; i++) {
							test_regs.regs[i] = regs.regs[i];
						}
						if (fpu_model) {
							for (int i = 0; i < 8; i++) {
								test_regs.fpuregs[i] = regs.fpuregs[i];
							}
						}
					}
#endif

					p = validate_test(p, ignore_errors, ignore_sr, &cur_regs, &test_regs, &last_regs, opcodeendsizeextra);

					if (interrupttest == 2) {
						short found = 0;
						for (short i = 0; i < MAX_IPL_PC_VALUES; i++) {
							if (ipl_pc[i] == test_regs.pc) {
								ipl_pc_cnt[i]++;
								found = 1;

							}
						}
						if (!found) {
							for (short i = 0; i < MAX_IPL_PC_VALUES; i++) {
								if (ipl_pc[i] == 0) {
									ipl_pc[i] = test_regs.pc;
									ipl_pc_cnt[i] = 1;
									break;
								}
							}
						}
					}

					testcnt++;
					if (super)
						supercnt++;

				}

				if (testexit()) {
					end_test();
					printf("\nAborted (%d)\n", testcnt);
					exit(0);
				}

				if (fpu_approx) {
					fpu_approxcnt++;
				}

				if (quit || errors) {
					if (!quit && errorcnt > 0 && totalerrors < errorcnt) {
						if (totalerrors > 0) {
							strcat(stored_outbuffer, "----------------------------------------\n");
						}
						if (strlen(stored_outbuffer) + strlen(outbuffer) + 40 >= outbuffer_size) {
							goto end;
						}
						strcat(stored_outbuffer, outbuffer);
						outbp = stored_outbuffer + strlen(stored_outbuffer);
						out_endinfo();
						infoadded = 0;
						errors = 0;
						outbuffer[0] = 0;
						outbuffer2[0] = 0;
						outbp = outbuffer2;
					} else {
						goto end;
					}
					totalerrors++;
				}
			}

			if (*p == CT_END) {
				p++;
				break;
			}

			extraccr = *p++;

		}

		if (validendsize == 2) {
			pl(opcode_memory_end, originalopcodeend);
		} else if (validendsize == 1) {
			pw(opcode_memory_end, originalopcodeend >> 16);
		}

		restoreahist();
	}

end:
	end_test();
	set_berr(0, 0);

	if (errorcnt > 0) {
		printf("%s", stored_outbuffer);
	} else {
		if (infoadded) {
			printf("\n");
			printf("%s", outbuffer);
		}
	}
	if (exitcnt == 0 && exitmode) {
		printf("Exit count expired\n");
	}
}

static void freestuff(void)
{
	if (test_memory && test_memory_addr) {
		free_absolute(test_memory_addr, test_memory_size);
	}
	if (blt_data) {
		free_blttemp(blt_data, BLT_TEMP_SIZE);
	}
#ifdef WAITEXIT
	getchar();
#endif
}

static uae_u32 read_u32(uae_u8 *headerfile, int *poffset)
{
	uae_u8 data[4] = { 0 };
	memcpy(data, headerfile + (*poffset), 4);
	(*poffset) += 4;
	return gl(data);
}

static int test_mnemo(const char *opcode)
{
	int size;
	uae_u32 v;
	char tfname[256];
	int filecnt = 0;
	uae_u32 starttimeid;
	int lvl;

	errors = 0;
	quit = 0;


	sprintf(tfname, "%s/0000.dat", opcode);
	size = -1;
	int foffset = 0;
	uae_u8 *headerfile = load_file(path, tfname, NULL, &size, &foffset, 1, 1);
	if (!headerfile) {
		printf("Data file failed to open\n");
		exit(0);
	}
	int headoffset = 0;
	v = read_u32(headerfile, &headoffset);
	if (v != DATA_VERSION) {
		printf("Invalid test data file (header %08x<>%08x)\n", v, DATA_VERSION);
		exit(0);
	}

	starttimeid = read_u32(headerfile, &headoffset);
	uae_u32 hmem_lmem = read_u32(headerfile, &headoffset);
	hmem_rom = (uae_s16)(hmem_lmem >> 16);
	lmem_rom = (uae_s16)(hmem_lmem & 65535);
	test_memory_addr = read_u32(headerfile, &headoffset);
	test_memory_size = read_u32(headerfile, &headoffset);
	test_memory_end = test_memory_addr + test_memory_size;
	opcode_memory_addr = read_u32(headerfile, &headoffset);
	opcode_memory = (uae_u8*)opcode_memory_addr;
	v = read_u32(headerfile, &headoffset);
	lvl = (v >> 16) & 15;
	interrupt_mask = (v >> 20) & 7;
	addressing_mask = (v & 0x80000000) ? 0xffffffff : 0x00ffffff;
	interrupttest = (v >> 26) & 3;
	sr_undefined_mask = v & 0xffff;
	safe_memory_mode = (v >> 23) & 7;
	loop_mode_jit = (v >> 28) & 1;
	loop_mode_68010 = (v >> 29) & 1;
	v = read_u32(headerfile, &headoffset);
	initial_interrupt_mask = v & 7;
	initial_interrupt = (v >> 3) & 7;
	fpu_max_precision = (v >> 6) & 3;
	v = read_u32(headerfile, &headoffset);
	v = read_u32(headerfile, &headoffset);
	fpu_model = read_u32(headerfile, &headoffset);
	test_low_memory_start = read_u32(headerfile, &headoffset);
	test_low_memory_end = read_u32(headerfile, &headoffset);
	test_high_memory_start = read_u32(headerfile, &headoffset);
	test_high_memory_end = read_u32(headerfile, &headoffset);
	safe_memory_start = (uae_u8*)read_u32(headerfile, &headoffset);
	safe_memory_end = (uae_u8*)read_u32(headerfile, &headoffset);
	user_stack_memory = read_u32(headerfile, &headoffset);
	super_stack_memory = read_u32(headerfile, &headoffset);
	exception_vectors = read_u32(headerfile, &headoffset);
	v = read_u32(headerfile, &headoffset);
	if (loop_mode_jit || loop_mode_68010) {
		loop_mode_cnt = v & 0xff;
	}
	if ((v >> 8) & 15) {
		loop_mode_jit = (v >> 8) & 15;
	}
	read_u32(headerfile, &headoffset);
	read_u32(headerfile, &headoffset);
	memcpy(inst_name, headerfile + headoffset, sizeof(inst_name) - 1);
	inst_name[sizeof(inst_name) - 1] = 0;
	free(headerfile);
	headerfile = NULL;

	int lvl2 = cpu_lvl;
	if (lvl2 == 5 && lvl2 != lvl)
		lvl2 = 4;

	if (lvl != lvl2) {
		printf("Mismatched CPU model: %u <> %u\n",
			68000 + 10 * (cpu_lvl < 5 ? cpu_lvl : 6), 68000 + (lvl < 5 ? lvl : 6) * 10);
		return 0;
	}

	if (!check_undefined_sr) {
		sr_undefined_mask = ~sr_undefined_mask;
	} else {
		sr_undefined_mask = 0xffff;
	}

	if (lmem_rom >= 0 && (low_memory_size <= 0 || !low_memory_temp)) {
		printf("lmem.dat required but it was not loaded or was missing.\n");
		return 0;
	}
	if (hmem_rom >= 0 && (high_memory_size <= 0 || !high_memory_temp)) {
		printf("hmem.dat required but it was not loaded or was missing.\n");
		return 0;
	}

	low_memory_offset = 0;
	if (test_low_memory_start != 0xffffffff) {
		low_memory_offset = test_low_memory_start;
	} else {
		low_memory_offset = 0x100;
		test_low_memory_end = 0xffffffff;
	}

	high_memory_offset = 0;
	if (test_high_memory_start != 0xffffffff) {
		high_memory_offset = test_high_memory_start & 0x7fff;
	} else {
		test_high_memory_end = 0xffffffff;
	}

	if (!absallocated) {
		test_memory = allocate_absolute(test_memory_addr, test_memory_size);
		if (!test_memory) {
			printf("Couldn't allocate tmem area %08x-%08x\n", test_memory_addr, test_memory_addr + test_memory_size - 1);
			exit(0);
		}
		absallocated = test_memory;
	}
	if (absallocated != test_memory) {
		printf("tmem area changed!?\n");
		exit(0);
	}

	size = test_memory_size;
	load_file(path, "tmem.dat", test_memory, &size, NULL, 1, 0);
	if (size != test_memory_size) {
		printf("tmem.dat size mismatch\n");
		exit(0);
	}

	printf("CPUlvl=%d, Mask=%08x Code=%08x SP=%08x ISP=%08x\n",
		cpu_lvl, addressing_mask, (uae_u32)opcode_memory,
		user_stack_memory, super_stack_memory);
	printf(" Low: %08x-%08x High: %08x-%08x\n",
		test_low_memory_start, test_low_memory_end - 1,
		test_high_memory_start, test_high_memory_end - 1);
	printf("Test: %08x-%08x Safe: %08x-%08x\n",
		test_memory_addr, test_memory_end - 1,
		(uae_u32)safe_memory_start, (uae_u32)safe_memory_end - 1);
	printf("%s (%s):\n", inst_name, group);

	testcnt = 0;
	fpu_approxcnt = 0;
	memset(exceptioncount, 0, sizeof(exceptioncount));
	supercnt = 0;
	exitcnt = exitcnt2;

	uae_u8 *test_data_prealloc = NULL;

	if (prealloc) {
		test_data_prealloc = malloc(prealloc_test_data_size);
		if (!test_data_prealloc) {
			printf("Couldn't preallocate test data memory, %d bytes.\n", prealloc_test_data_size);
			exit(0);
		}
	}

	blt_data = allocate_blttemp(BLT_TEMP_SIZE);
	if (!blt_data) {
		printf("blt_temp failed to allocated.\n");
		exit(0);
	}

	int otestcnt = -1;
	for (;;) {
		if (otestcnt != testcnt) {
			printf("%s (%s). %u...\n", tfname, group, testcnt);
			otestcnt = testcnt;
		}

		sprintf(tfname, "%s/%04d.dat", opcode, filecnt);

		test_data_size = 0;
		test_data = load_file(path, tfname, test_data_prealloc, &test_data_size, &foffset, 0, 1);

		if (!test_data) {
			if (foffset < 0) {
				filecnt++;
				foffset = 0;
				continue;
			}
			if (askifmissing) {
				printf("Couldn't open '%s%s'. Type new path and press enter.\n", path, tfname);
				path[0] = 0;
				fgets(path, sizeof(path), stdin);
				if (path[0]) {
					path[strlen(path) - 1] = 0;
					if (path[0]) {
						continue;
					}
				}
			}
			quit = 1;
			break;
		}
		if (gl(test_data) != DATA_VERSION) {
			printf("Invalid test data file (header, %08x<>%08x)\n", gl(test_data), DATA_VERSION);
			exit(0);
		}
		if (gl(test_data + 4) != starttimeid) {
			printf("Test data file header mismatch (old test data file?)\n");
			break;
		}
		if (test_data[test_data_size - 2] != CT_END_FINISH) {
			printf("Invalid test data file (footer)\n");
			free(test_data);
			exit(0);
		}
		uae_u32 flags = gl(test_data + 8);
		instructionsize = flags & 3;

		// last file?
		int last = test_data[test_data_size - 1] == CT_END_FINISH;

		test_data_size -= 16;
		if (test_data_size <= 0)
			break;

		test_data += 16;
		process_test(test_data);
		test_data -= 16;

		if (!prealloc) {
			free(test_data);
		}
		test_data = NULL;

		if (errors || quit || last || (!exitcnt && exitmode)) {
			break;
		}

		if (foffset <= 0) {
			filecnt++;
		}
	}

	if (blt_data) {
		free_blttemp(blt_data, BLT_TEMP_SIZE);
		blt_data = 0;
	}

	if (errorcnt == 0) {
		outbp = outbuffer;
		out_endinfo();
		printf("%s", outbuffer);
	}
	if (exitcnt == 0 && !exitmode) {
		printf("Exit count expired\n");
	}
	if (!errors && !quit) {
		printf("All tests complete (total %u).\n", testcnt);
	}

	return errors || quit;
}

static int getparamval(const char *p)
{
	uae_u32 inv = 0;
	if (p[0] == '~') {
		inv = 0xffffffff;
		p++;
	}
	if (strlen(p) > 2 && p[0] == '0' && toupper(p[1]) == 'X') {
		char *endptr;
		return strtol(p + 2, &endptr, 16) ^ inv;
	} else {
		return atol(p) ^ inv;
	}
}

int main(int argc, char *argv[])
{
	int stop_on_error = 1;

	atexit(freestuff);

#ifndef M68K

	char *params[] = { "", "lslw.w", "", NULL };
	argv = params;
	argc = 3;

	strcpy(path, "C:\\projects\\winuae\\src\\cputest\\data\\");

	vbr_zero = calloc(1, 1024);

	cpu_lvl = 0;

#else

#define _stricmp strcasecmp

	strcpy(path, "data/");

	low_memory = (uae_u8*)0;
	high_memory = (uae_u8*)0xffff8000;

	cpu_lvl = get_cpu_model();

	if (cpu_lvl == 5) {
		// Overwrite MOVEC to/from MSP
		// with NOPs if 68060
		extern void *msp_address1;
		extern void *msp_address2;
		extern void *msp_address3;
		extern void *msp_address4;
		*((uae_u32*)&msp_address1) = 0x4e714e71;
		*((uae_u32*)&msp_address2) = 0x4e714e71;
		*((uae_u32*)&msp_address3) = 0x4e714e71;
		*((uae_u32*)&msp_address4) = 0x4e714e71;
	}

#endif

	if (argc < 2) {
		printf("cputest (<group>)/<all/mnemonic> (<start mnemonic>) (other params)\n");
#ifdef M68K
		printf("Designed and programmed by Toni Wilen. ("REVDATE " " REVTIME")");
#endif
		printf("\n");
		printf("mnemonic = test single mnemonic\n");
		printf("all = test all\n");
		printf("all <mnemonic> = test all, starting from <mnemonic>\n");
		printf("all <mnemonic> -next = test all, starting after <mnemonic>\n");
		printf("-continue = don't stop on error (continue to next test, all mode only)\n");
		printf("-errors n = store up to n errors before aborting test.\n");
		printf("-ccrmask = ignore CCR bits that are not set.\n");
		printf("-nodisasm = do not disassemble failed test.\n");
		printf("-basicexc = do only basic checks when exception is 2 or 3.\n");
		printf("-skipexcccr = ignore CCR if DivByZero, CHK, Address/Bus error exception.\n");
		printf("-skipmem = do not validate memory writes.\n");
		printf("-skipreg = do not validate registers.\n");
		printf("-askifmissing = ask for new path if dat file is missing.\n");
		printf("-exit n = exit after n tests.\n");
		printf("-exitok n = exit after n tests, continue normally.\n");
		printf("-fpuadj <exp> 16-bit exponent range value. (0 = 1.0)\n");
		printf("-fpsrmask = ignore FPSR bits that are not set.\n");
		printf("-nofpiar = ignore FPIAR.\n");
		printf("-cycles [range adjust] = check cycle counts.\n");
		printf("-cyclecnt <address>. Use custom hardware cycle counter.\n");
#ifdef AMIGA
		printf("-uae = running in UAE, automatic bus error enable/disable.\n");
#endif
		return 0;
	}

	opcode[0] = 0;
	strcpy(group, "default");

	check_undefined_sr = 1;
	ccr_mask = 0xff;
	fpsr_ignore_mask = 0xffffffff;
	fpiar_ignore = 0;
	disasm = 1;
	exitcnt2 = -1;
	exitmode = 0;
	cyclecounter_addr = 0xffffffff;
	cycles_range = 2;
	fpu_adjust_exp = -1;

	for (int i = 1; i < argc; i++) {
		char *s = argv[i];
		char *next = i + 1 < argc && argv[i + 1][0] != '-' ? argv[i + 1] : NULL;
		if (s[0] != '-' && opcode[0] == 0 && strlen(s) < sizeof(opcode) - 1) {
			strcpy(opcode, s);
			continue;
		}
		if (!_stricmp(s, "-continue")) {
			stop_on_error = 0;
		} else if (!_stricmp(s, "-nostop")) {
			continue_on_error = 1;
		} else if (!_stricmp(s, "-noundefined")) {
			check_undefined_sr = 0;
		} else if (!_stricmp(s, "-ccrmask")) {
			ccr_mask = 0;
			if (next) {
				ccr_mask = ~getparamval(next);
				i++;
			}
		} else if (!_stricmp(s, "-fpsrmask")) {
			fpsr_ignore_mask = (1 << 27) | (1 << 26);
			if (next) {
				fpsr_ignore_mask = ~getparamval(next);
				i++;
			}
		} else if (!_stricmp(s, "-nofpiar")) {
			fpiar_ignore = 1;
		} else if (!_stricmp(s, "-silent")) {
			dooutput = 0;
		} else if (!_stricmp(s, "-68000")) {
			cpu_lvl = 0;
		} else if (!_stricmp(s, "-68010")) {
			cpu_lvl = 1;
		} else if (!_stricmp(s, "-68020")) {
			cpu_lvl = 2;
		} else if (!_stricmp(s, "-68030")) {
			cpu_lvl = 3;
		} else if (!_stricmp(s, "-68040")) {
			cpu_lvl = 4;
		} else if (!_stricmp(s, "-68060")) {
			cpu_lvl = 5;
		} else if (!_stricmp(s, "-nodisasm")) {
			disasm = 0;
		} else if (!_stricmp(s, "-basicexc")) {
			basicexcept = 1;
		} else if (!_stricmp(s, "-skipexcccr")) {
			excskipccr = 1;
		} else if (!_stricmp(s, "-skipmem")) {
			skipmemwrite = 1;
		} else if (!_stricmp(s, "-skipreg")) {
			skipregchange = 1;
		} else if (!_stricmp(s, "-skipccr")) {
			skipccrchange = 1;
		} else if (!_stricmp(s, "-askifmissing")) {
			askifmissing = 1;
		} else if (!_stricmp(s, "-next")) {
			nextall = 1;
		} else if (!_stricmp(s, "-exit")) {
			if (next) {
				exitcnt2 = atoi(next);
				exitmode = 0;
				i++;
			}
		} else if (!_stricmp(s, "-exitok")) {
			if (next) {
				exitcnt2 = atoi(next);
				exitmode = 1;
				i++;
			}
		} else if (!_stricmp(s, "-irqcnt")) {
			if (next) {
				irqcnt = atoi(next);
				i++;
			}
		} else if (!_stricmp(s, "-prealloc")) {
			prealloc = 1;
		} else if (!_stricmp(s, "-fpuadj")) {
			fpu_adjust_exp = 0;
			if (next) {
				fpu_adjust_exp = atol(next);
			}
			is_fpu_adjust = 1;
			fpu_adjust_exp += 16384;
		} else if (!_stricmp(s, "-cycles")) {
			cycles = 1;
			if (i + 1 < argc && argv[i][0] != '-') {
				i++;
				cycles_range = atoi(argv[i]);
				if (i + 1 < argc && argv[i][0] != '-') {
					i++;
					cycles_adjust = atoi(argv[i]);
				}
			}
		} else if (!_stricmp(s, "-cyclecnt")) {
			if (next) {
				cyclecounter_addr = getparamval(next);
				cycles = 1;
			}
		} else if (!_stricmp(s, "-uae")) {
			uaemode = 1;
		} else if (!_stricmp(s, "-errors")) {
			if (i + 1 < argc) {
				i++;
				errorcnt = atoi(argv[i]);
			}
		}

	}

#ifdef M68K
	if(cyclecounter_addr != 0xffffffff) {
		// yes, this is far too ugly
		extern uae_u32 cyclereg_address1;
		extern uae_u32 cyclereg_address2;
		extern uae_u32 cyclereg_address3;
		extern uae_u32 cyclereg_address4;
		extern uae_u32 cyclereg_address5;
		extern uae_u32 cyclereg_address6;
		*((uae_u32*)((uae_u32)(&cyclereg_address1) + 2)) = cyclecounter_addr;
		*((uae_u32*)((uae_u32)(&cyclereg_address2) + 2)) = cyclecounter_addr;
		*((uae_u32*)((uae_u32)(&cyclereg_address3) + 2)) = cyclecounter_addr;
		*((uae_u32*)((uae_u32)(&cyclereg_address4) + 2)) = cyclecounter_addr;
		*((uae_u32*)((uae_u32)(&cyclereg_address5) + 2)) = cyclecounter_addr;
		*((uae_u32*)((uae_u32)(&cyclereg_address6) + 2)) = cyclecounter_addr;
	}
#endif

	if (errorcnt > 0) {
		outbuffer_size = errorcnt * PAGE_OUTBUFFER_SIZE;
		stored_outbuffer = calloc(outbuffer_size, 1);
		if (!stored_outbuffer) {
			printf("Out of memory when allocating temporary output buffer.\n");
			return 0;
		}
	}

	void *groupd = NULL;
	
	char *p = strchr(opcode, '/');
	if (p) {
		strcpy(tmpbuffer, opcode);
		strcpy(group, opcode);
		group[p - opcode] = 0;
		strcpy(opcode, tmpbuffer + (p - opcode) + 1);
	}
	
	if (!strcmp(group, "all")) {
		groupd = x_opendir(path);
	}

	int cpumodel = cpu_lvl == 5 ? 6 : cpu_lvl;
	sprintf(cpustr, "%u_", cpumodel);
	char *pathptr = path + strlen(path);

	for (;;) {

		if (groupd) {
			pathptr[0] = 0;
			char dname[256];
			int groupdr = x_readdir(path, groupd, dname);
			if (!groupdr)
				break;
			if (groupdr > 0)
				continue;
			if (dname[0] == '.')
				continue;
			if (strnicmp(cpustr, dname, strlen(cpustr)))
				continue;
			sprintf(pathptr, "%s/", dname);		
			strcpy(group, dname + strlen(cpustr));
		} else {
			sprintf(pathptr, "%u_%s/",cpumodel, group);
		}

		low_memory_size = -1;
		low_memory_temp = load_file(path, "lmem.dat", NULL, &low_memory_size, NULL, 0, 1);
		high_memory_size = -1;
		high_memory_temp = load_file(path, "hmem.dat", NULL, &high_memory_size, NULL, 0, 1);

#ifndef M68K
		low_memory = calloc(1, 32768);
		if (high_memory_size > 0)
			high_memory = calloc(1, high_memory_size);
#endif

		low_memory_back = calloc(1, 32768);
		if (!low_memory_back) {
			printf("Couldn't allocate low_memory_back.\n");
			return 0;
		}
		if (!low_memory_temp) {
			low_memory_temp = calloc(1, 32768);
			if (!low_memory_temp) {
				printf("Couldn't allocate low_memory_temp.\n");
				return 0;
			}
		}

		if (high_memory_size > 0) {
			high_memory_back = calloc(1, high_memory_size);
			if (!high_memory_back) {
				printf("Couldn't allocate high_memory_back.\n");
				return 0;
			}
		}

		if (!_stricmp(opcode, "all") || (opcode[0] && opcode[strlen(opcode) - 1] == '*')) {
			void *d = x_opendir(path);
			if (!d) {
				printf("Couldn't list directory '%s'\n", path);
				return 0;
			}
#define MAX_FILE_LEN 128
#define MAX_FILES 500
			char *dirs = calloc(MAX_FILES, MAX_FILE_LEN);
			int diroff = 0;
			int dircnt = 0;
			if (!dirs)
				return 0;

			// get directory
			for (;;) {
				int dr = x_readdir(path, d, dirs + diroff);
				if (!dr)
					break;
				if (dr < 0 && dirs[diroff] != '.') {
					dircnt++;
					diroff += MAX_FILE_LEN;
					if (dircnt >= MAX_FILES) {
						printf("too many directories!? (%d)\n", dircnt);
						return 0;
					}
				}
			}
			x_closedir(d);

			// sort directory
			for (int i = 0; i < diroff; i += MAX_FILE_LEN) {
				for (int j = i + MAX_FILE_LEN; j < diroff; j += MAX_FILE_LEN) {
					if (_stricmp(dirs + i, dirs + j) > 0) {
						char tmp[MAX_FILE_LEN];
						strcpy(tmp, dirs + j);
						strcpy(dirs + j, dirs + i);
						strcpy(dirs + i, tmp);
					}
				}
			}

			int first = 0;
			if (argc >= 3 && argv[2][0] != '-') {
				first = -1;
				for (int i = 0; i < diroff; i += MAX_FILE_LEN) {
					if (!_stricmp(dirs + i, argv[2])) {
						first = i;
						break;
					}
				}
				if (first < 0) {
					printf("Couldn't find '%s'\n", argv[2]);
					return 0;
				}
			}
			if (nextall) {
				first += MAX_FILE_LEN;
			}

			int err = 0;
			if (!_stricmp(opcode, "all")) {
				for (int i = first; i < diroff; i += MAX_FILE_LEN) {
					if (test_mnemo(dirs + i)) {
						err = 1;
						if (stop_on_error)
							break;
					}
				}
			} else {
				for (int i = first; i < diroff; i += MAX_FILE_LEN) {
					char *name = dirs + i;
					if (!strnicmp(name, opcode, strlen(opcode) - 1)) {
						if (test_mnemo(name)) {
							if (stop_on_error)
								break;
						}
					}
				}
			}

			free(dirs);

			if (err && stop_on_error)
				break;

		} else {
			if (test_mnemo(opcode)) {
				if (stop_on_error)
					break;
			}
		}

		free(low_memory_temp);
		free(high_memory_temp);
		free(low_memory_back);
		free(high_memory_back);

		if (!groupd)
			break;
	}

	if (groupd)
		x_closedir(groupd);

	return 0;
}
