/*
 * compiler/compemu.h - Public interface and definitions
 *
 * Copyright (c) 2001-2004 Milan Jurik of ARAnyM dev team (see AUTHORS)
 *
 * Inspired by Christian Bauer's Basilisk II
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * JIT compiler m68k -> IA-32 and AMD64
 *
 * Original 68040 JIT compiler for UAE, copyright 2000-2002 Bernd Meyer
 * Adaptation for Basilisk II and improvements, copyright 2000-2004 Gwenole Beauchesne
 * Portions related to CPU detection come from linux/arch/i386/kernel/setup.c
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef COMPEMU_H
#define COMPEMU_H

#include "sysconfig.h"
#include "newcpu.h"

#ifdef UAE
#ifdef CPU_64_BIT
typedef uae_u64 uintptr;
typedef uae_s64 intptr;
#else
typedef uae_u32 uintptr;
#endif
/* FIXME: cpummu.cpp also checks for USE_JIT, possibly others */
#define USE_JIT
#endif

#define JITPTR (uae_u32)(uintptr)

/* Now that we do block chaining, and also have linked lists on each tag,
   TAGMASK can be much smaller and still do its job. Saves several megs
   of memory! */
#define TAGMASK 0x0000ffff
#define TAGSIZE (TAGMASK+1)
#define MAXRUN 1024
#define cacheline(x) (((uintptr)x)&TAGMASK)

extern uae_u8* start_pc_p;
extern uae_u32 start_pc;

struct blockinfo_t;

typedef struct {
  uae_u16* location;
  uae_u8  specmem;
} cpu_history;

typedef union {
  cpuop_func* handler;
  struct blockinfo_t* bi;
} cacheline;

#define USE_F_ALIAS 1
#define USE_OFFSET 1
#define COMP_DEBUG 1

#if COMP_DEBUG
#define Dif(x) if (x)
#else
#define Dif(x) if (0)
#endif

#define SCALE 2
#define MAXCYCLES (1000 * CYCLE_UNIT)

#define BYTES_PER_INST 10240  /* paranoid ;-) */
#if defined(CPU_arm)
#define LONGEST_68K_INST 256 /* The number of bytes the longest possible
			       68k instruction takes */
#else
#define LONGEST_68K_INST 16 /* The number of bytes the longest possible
			       68k instruction takes */
#endif
#define MAX_CHECKSUM_LEN 2048 /* The maximum size we calculate checksums
				 for. Anything larger will be flushed
				 unconditionally even with SOFT_FLUSH */
#define MAX_HOLD_BI 3  /* One for the current block, and up to two
			  for jump targets */

#if 1
// gb-- my format from readcpu.cpp is not the same
#define FLAG_X    0x0010
#define FLAG_N    0x0008
#define FLAG_Z    0x0004
#define FLAG_V    0x0002
#define FLAG_C    0x0001
#else
#define FLAG_C    0x0010
#define FLAG_V    0x0008
#define FLAG_Z    0x0004
#define FLAG_N    0x0002
#define FLAG_X    0x0001
#endif
#define FLAG_CZNV (FLAG_C | FLAG_Z | FLAG_N | FLAG_V)
#define FLAG_ALL  (FLAG_C | FLAG_Z | FLAG_N | FLAG_V | FLAG_X)
#define FLAG_ZNV  (FLAG_Z | FLAG_N | FLAG_V)

#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
#define USE_DATA_BUFFER
#elif defined(CPU_AARCH64)
#define N_REGS 18   /* really 32, but 29 to 31 are FP, LR, SP; 18 has special meaning; 27 holds memstart and 28 holds regs-struct */
#else
#define N_REGS 11  /* really 16, but 13 to 15 are SP, LR, PC; 11 holds memstart and 12 is scratch register */
#endif

#define N_FREGS 16  // We use 10 regs: 6 - FP_RESULT, 7 - SCRATCH, 8-15 - Amiga regs FP0-FP7

/* Functions exposed to newcpu, or to what was moved from newcpu.c to
 * compemu_support.c */
extern void compiler_init(void);
extern void compiler_exit(void);
extern void build_comp(void);
extern void set_cache_state(int enabled);
#ifdef JIT
extern void (*flush_icache)(int);
#endif
extern void alloc_cache(void);
extern void compile_block(cpu_history* pc_hist, int blocklen, int totcyles);
extern int check_for_cache_miss(void);

#define scaled_cycles(x) (currprefs.m68k_speed<0?(((x)/SCALE)?(((x)/SCALE<MAXCYCLES?((x)/SCALE):MAXCYCLES)):1):(x))

/* JIT FPU compilation */
extern void comp_fpp_opp (uae_u32 opcode, uae_u16 extra);
extern void comp_fbcc_opp (uae_u32 opcode);
extern void comp_fscc_opp (uae_u32 opcode, uae_u16 extra);

extern uae_u32 needed_flags;
extern uae_u8* comp_pc_p;
extern void* pushall_call_handler;

#define VREGS 22
#define VFREGS 10

#define INMEM 1
#define CLEAN 2
#define DIRTY 3
#define UNDEF 4
#define ISCONST 5

typedef struct {
  uae_u32* mem;
  uae_u32 val;
  uae_u8 status;
  uae_s8 realreg; /* gb-- realreg can hold -1 */
  uae_u8 realind; /* The index in the holds[] array */
  uae_u8 needflush;
} reg_status;

typedef struct {
  uae_u32* mem;
  uae_u8 status;
  uae_s8 realreg; /* gb-- realreg can hold -1 */
  uae_u8 needflush;
} freg_status;

typedef struct {
  uae_u8 use_flags;
  uae_u8 set_flags;
  uae_u8 is_addx;
  uae_u8 cflow;
} op_properties;

extern op_properties prop[65536];

STATIC_INLINE int end_block(uae_u16 opcode)
{
	return (prop[opcode].cflow & fl_end_block);
}

#define SP_REG 15
#define PC_P 16
#define FLAGX 17
#define FLAGTMP 18

#define S1 19
#define S2 20
#define S3 21
#define SCRATCH_REGS 3

#define FP_RESULT 8
#define FS1 9

#define SCRATCH_F64_1  1
#define SCRATCH_F64_2  2
#define SCRATCH_F64_3  3
#define SCRATCH_F64_4  4
#define SCRATCH_F32_1  2
#define SCRATCH_F32_2  4
#define SCRATCH_F32_3  6
#define SCRATCH_F32_4  8

typedef struct {
  uae_u32 touched;
  uae_s8 holds[VREGS];
  uae_u8 nholds;
  uae_u8 locked;
} n_status;

typedef struct {
  uae_s8 holds;
  uae_u8 nholds;
} fn_status;

/* For flag handling */
#define NADA 1
#define TRASH 2
#define VALID 3

/* needflush values */
#define NF_SCRATCH   0
#define NF_TOMEM     1
#define NF_HANDLER   2

typedef struct {
  /* Integer part */
  reg_status state[VREGS];
  n_status   nat[N_REGS];
  uae_u32 flags_on_stack;
  uae_u32 flags_in_flags;
  uae_u32 flags_are_important;
  uae_u32 scratch_in_use[SCRATCH_REGS];
  /* FPU part */
  freg_status fate[VFREGS];
  fn_status   fat[N_FREGS];
} bigstate;

typedef struct {
	/* Integer part */
	uae_s8 virt[VREGS];
	uae_s8 nat[N_REGS];
} smallstate;

#define IM8 uae_s32
#define IM16 uae_s32
#define IM32 uae_s32
#define IMPTR uintptr
#define RR1 uae_u32
#define RR2 uae_u32
#define RR4 uae_u32
#define W1  uae_u32
#define W2  uae_u32
#define W4  uae_u32
#define RW1 uae_u32
#define RW2 uae_u32
#define RW4 uae_u32
#define MEMR uintptr
#define MEMW uintptr
#define MEMRW uintptr

#define FW   uae_u32
#define FR   uae_u32
#define FRW  uae_u32

#define MIDFUNC(nargs,func,args) void func args
#define MENDFUNC(nargs,func,args)
#define COMPCALL(func) func

#define LOWFUNC(flags,mem,nargs,func,args) STATIC_INLINE void func args
#define LENDFUNC(flags,mem,nargs,func,args)

/* What we expose to the outside */
#define DECLARE_MIDFUNC(func) extern void func

#if defined(CPU_arm)
#include "compemu_midfunc_arm.h"
#include "compemu_midfunc_arm2.h"
#endif

#undef DECLARE_MIDFUNC

extern int failure;
#define FAIL(x) do { failure|=x; } while (0)

/* Convenience functions exposed to gencomp */
extern uae_u32 m68k_pc_offset;
/* address and dest are virtual register numbers */
extern void readbyte(int address, int dest);
extern void readword(int address, int dest);
extern void readlong(int address, int dest);
extern void writebyte(int address, int source);
extern void writeword(int address, int source);
extern void writelong(int address, int source);
extern void writeword_clobber(int address, int source);
extern void writelong_clobber(int address, int source);
extern void get_n_addr(int address, int dest);
extern void get_n_addr_jmp(int address, int dest);
extern void calc_disp_ea_020(int base, uae_u32 dp, int target);
#define SYNC_PC_OFFSET 124
extern void sync_m68k_pc(void);
extern uae_u32 get_const(int r);
extern void register_branch(uae_u32 not_taken, uae_u32 taken, uae_u8 cond);
extern void register_possible_exception(void);

#define comp_get_ibyte(o) do_get_mem_byte((uae_u8 *)(comp_pc_p + (o) + 1))
#define comp_get_iword(o) do_get_mem_word((uae_u16 *)(comp_pc_p + (o)))
#define comp_get_ilong(o) do_get_mem_long((uae_u32 *)(comp_pc_p + (o)))

extern int alloc_scratch(void);
extern void release_scratch(int i);

struct blockinfo_t;

typedef struct dep_t {
  uae_u32*            jmp_off;
  struct blockinfo_t* target;
  struct blockinfo_t* source;
  struct dep_t**      prev_p;
  struct dep_t*       next;
} dependency;

typedef struct checksum_info_t {
  uae_u8 *start_p;
  uae_u32 length;
  struct checksum_info_t *next;
} checksum_info;

typedef struct blockinfo_t {
  uae_s32 count;
  cpuop_func* direct_handler_to_use;
  cpuop_func* handler_to_use;
  /* The direct handler does not check for the correct address */

  cpuop_func* handler;
  cpuop_func* direct_handler;

  cpuop_func* direct_pen;
  cpuop_func* direct_pcc;

  uae_u8* nexthandler;
  uae_u8* pc_p;

  uae_u32 c1;
  uae_u32 c2;
  checksum_info *csi;

  struct blockinfo_t* next_same_cl;
  struct blockinfo_t** prev_same_cl_p;
  struct blockinfo_t* next;
  struct blockinfo_t** prev_p;

  uae_u8 optlevel;
  uae_u8 needed_flags;
  uae_u8 status;

  dependency  dep[2];  /* Holds things we depend on */
  dependency* deplist; /* List of things that depend on this */
} blockinfo;

#define BI_INVALID 0
#define BI_ACTIVE 1
#define BI_NEED_RECOMP 2
#define BI_NEED_CHECK 3
#define BI_CHECKING 4
#define BI_COMPILING 5
#define BI_FINALIZING 6

extern const int POPALLSPACE_SIZE;

void execute_normal(void);
void exec_nostats(void);
void do_nothing(void);
void execute_exception(uae_u32 cycles);

typedef fptype fpu_register;

/* Flags for Bernie during development/debugging. Should go away eventually */
#define DISTRUST_CONSISTENT_MEM 0

void jit_abort(const TCHAR *format,...);

#ifdef CPU_64_BIT
static inline uae_u32 check_uae_p32(uintptr address, const char* file, int line)
{
	if (address > (uintptr_t)0xffffffff) {
		jit_abort("JIT: 64-bit pointer (0x%llx) at %s:%d (fatal)",
			(unsigned long long)address, file, line);
	}
	return (uae_u32)address;
}
#define uae_p32(x) (check_uae_p32((uintptr)(x), __FILE__, __LINE__))
#else
#define uae_p32(x) ((uae_u32)(x))
#endif

#endif /* COMPEMU_H */
