/*
 * compiler/compemu_support.cpp - Core dynamic translation engine
 *
 * Copyright (c) 2001-2009 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Christian Bauer's Basilisk II
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * JIT compiler m68k -> IA-32 and AMD64 / ARM
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


#define writemem_special writemem
#define readmem_special  readmem

#include "sysconfig.h"
#include "sysdeps.h"

#if defined(JIT)

#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "comptbl.h"
#include "compemu.h"
#include <SDL.h>

#define DEBUG 0
#include "debug.h"

#if DEBUG
#define PROFILE_COMPILE_TIME		1
#define PROFILE_UNTRANSLATED_INSNS	1
#define bug printf
#endif

#ifdef JIT_DEBUG
#undef abort
#define abort() do { \
	fprintf(stderr, "Abort in file %s at line %d\n", __FILE__, __LINE__); \
	compiler_dumpstate(); \
  SDL_Quit();  \
	exit(EXIT_FAILURE); \
} while (0)
#endif

#ifdef PROFILE_COMPILE_TIME
#include <time.h>
static uae_u32 compile_count	= 0;
static clock_t compile_time		= 0;
static clock_t emul_start_time	= 0;
static clock_t emul_end_time	= 0;
#endif

#ifdef PROFILE_UNTRANSLATED_INSNS
static int untranslated_top_ten = 30;
static uae_u32 raw_cputbl_count[65536] = { 0, };
static uae_u16 opcode_nums[65536];


static int untranslated_compfn(const void *e1, const void *e2)
{
	return raw_cputbl_count[*(const uae_u16 *)e1] < raw_cputbl_count[*(const uae_u16 *)e2];
}
#endif

#define NATMEM_OFFSETX (uae_u32)natmem_offset

// %%% BRIAN KING WAS HERE %%%
#include <sys/mman.h>
extern void jit_abort(const TCHAR*,...);
static compop_func *compfunctbl[65536];
static compop_func *nfcompfunctbl[65536];
#ifdef NOFLAGS_SUPPORT
static cpuop_func *nfcpufunctbl[65536];
#endif
uae_u8* comp_pc_p;

// gb-- Extra data for Basilisk II/JIT
#ifdef JIT_DEBUG
static int JITDebug	= 0;	// Enable runtime disassemblers through mon?
#else
const int	JITDebug= 0;	  // Don't use JIT debug mode at all
#endif
#if USE_INLINING
static int follow_const_jumps	= 1;		// Flag: translation through constant jumps	
#else
const int	follow_const_jumps = 0;
#endif

static uae_u32 current_cache_size	= 0;		// Cache grows upwards: how much has been consumed already
static int lazy_flush	= 1;	      // Flag: lazy translation cache invalidation
static int avoid_fpu = 1;	        // Flag: compile FPU instructions ?
const int tune_alignment = 1;	    // Tune code alignments for running CPU ?
const int tune_nop_fillers = 1;	  // Tune no-op fillers for architecture
static int setzflg_uses_bsf	= 0;	// setzflg virtual instruction can use native BSF instruction correctly?
static int align_loops = 0;	      // Align the start of loops
static int align_jumps = 0;	      // Align the start of jumps

op_properties prop[65536];

STATIC_INLINE bool is_const_jump(uae_u32 opcode)
{
	return (prop[opcode].cflow == fl_const_jump);
}

STATIC_INLINE bool may_trap(uae_u32 opcode)
{
	return (prop[opcode].cflow & fl_trap);
}

STATIC_INLINE unsigned int cft_map (unsigned int f)
{
  return f;
}

uae_u8* start_pc_p;
uae_u32 start_pc;
uae_u32 current_block_pc_p;
static uintptr current_block_start_target;
uae_u32 needed_flags;
static uintptr next_pc_p;
static uintptr taken_pc_p;
static int branch_cc;
static int redo_current_block;

int segvcount = 0;
static uae_u8* current_compile_p = NULL;
static uae_u8* max_compile_start;
uae_u8* compiled_code = NULL;
const int POPALLSPACE_SIZE = 2048; /* That should be enough space */
static uae_u8 *popallspace = NULL;

void* pushall_call_handler = NULL;
static void* popall_do_nothing = NULL;
static void* popall_exec_nostats = NULL;
static void* popall_execute_normal = NULL;
static void* popall_cache_miss = NULL;
static void* popall_recompile_block = NULL;
static void* popall_check_checksum = NULL;

/* The 68k only ever executes from even addresses. So right now, we
 waste half the entries in this array
 UPDATE: We now use those entries to store the start of the linked
 lists that we maintain for each hash result. */
static cacheline cache_tags[TAGSIZE];
static int letit=0;
static blockinfo* hold_bi[MAX_HOLD_BI];
static blockinfo* active;
static blockinfo* dormant;

#ifdef PANDORA

void cache_free (uae_u8 *cache, int size)
{
  munmap(cache, size);
}

uae_u8 *cache_alloc (int size)
{
  size = size < getpagesize() ? getpagesize() : size;

  void *cache = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (!cache) {
    printf ("Cache_Alloc of %d failed. ERR=%d\n", size, errno);
  }
  else
    memset(cache, 0, size);
  return (uae_u8 *) cache;
}

#endif

#ifdef NOFLAGS_SUPPORT
/* 68040 */
extern const struct cputbl op_smalltbl_0_nf[];
#endif
extern const struct comptbl op_smalltbl_0_comp_nf[];
extern const struct comptbl op_smalltbl_0_comp_ff[];
#ifdef NOFLAGS_SUPPORT
/* 68020 + 68881 */
extern const struct cputbl op_smalltbl_1_nf[];
/* 68020 */
extern const struct cputbl op_smalltbl_2_nf[];
/* 68010 */
extern const struct cputbl op_smalltbl_3_nf[];
/* 68000 */
extern const struct cputbl op_smalltbl_4_nf[];
/* 68000 slow but compatible.  */
extern const struct cputbl op_smalltbl_5_nf[];
#endif

static void flush_icache_hard(uaecptr ptr, int n);
static void flush_icache_lazy(uaecptr ptr, int n);
static void flush_icache_none(uaecptr ptr, int n);
void (*flush_icache)(uaecptr ptr, int n) = flush_icache_none;

static bigstate live;
static smallstate empty_ss;
static smallstate default_ss;
static int optlev;

static int writereg(int r, int size);
static void unlock2(int r);
static void setlock(int r);
static int readreg_specific(int r, int size, int spec);
static int writereg_specific(int r, int size, int spec);
static void prepare_for_call_1(void);
static void prepare_for_call_2(void);
#ifndef ALIGN_NOT_NEEDED
static void align_target(uae_u32 a);
#endif

STATIC_INLINE void flush_cpu_icache(void *from, void *to);
STATIC_INLINE void write_jmp_target(uae_u32 *jmpaddr, cpuop_func* a);
STATIC_INLINE void emit_jmp_target(uae_u32 a);

uae_u32 m68k_pc_offset;

/* Flag handling is complicated.
 
 *
 * x86 instructions create flags, which quite often are exactly what we
 * want. So at times, the "68k" flags are actually in the x86 flags.
 *
 * Then again, sometimes we do x86 instructions that clobber the x86
 * flags, but don't represent a corresponding m68k instruction. In that
 * case, we have to save them.
 *
 * We used to save them to the stack, but now store them back directly
 * into the regflags.cznv of the traditional emulation. Thus some odd
 * names.
 *
 * So flags can be in either of two places (used to be three; boy were
 * things complicated back then!); And either place can contain either
 * valid flags or invalid trash (and on the stack, there was also the
 * option of "nothing at all", now gone). A couple of variables keep
 * track of the respective states.
 *
 * To make things worse, we might or might not be interested in the flags.
 * by default, we are, but a call to dont_care_flags can change that
 * until the next call to live_flags. If we are not, pretty much whatever
 * is in the register and/or the native flags is seen as valid.
*/

STATIC_INLINE blockinfo* get_blockinfo(uae_u32 cl)
{
  return cache_tags[cl+1].bi;
}

STATIC_INLINE blockinfo* get_blockinfo_addr(void* addr)
{
  blockinfo*  bi = get_blockinfo(cacheline(addr));

  while (bi) {
  	if (bi->pc_p == addr)
	    return bi;
  	bi = bi->next_same_cl;
  }
  return NULL;
}


/*******************************************************************
 * All sorts of list related functions for all of the lists        *
 *******************************************************************/

STATIC_INLINE void remove_from_cl_list(blockinfo* bi)
{
  uae_u32 cl = cacheline(bi->pc_p);

  if (bi->prev_same_cl_p)
  	*(bi->prev_same_cl_p) = bi->next_same_cl;
  if (bi->next_same_cl)
  	bi->next_same_cl->prev_same_cl_p = bi->prev_same_cl_p;
  if (cache_tags[cl+1].bi)
  	cache_tags[cl].handler = cache_tags[cl+1].bi->handler_to_use;
  else
  	cache_tags[cl].handler = (cpuop_func *)popall_execute_normal;
}

STATIC_INLINE void remove_from_list(blockinfo* bi)
{
  if (bi->prev_p)
  	*(bi->prev_p) = bi->next;
  if (bi->next)
  	bi->next->prev_p = bi->prev_p;
}

STATIC_INLINE void remove_from_lists(blockinfo* bi)
{
  remove_from_list(bi);
  remove_from_cl_list(bi);
}

STATIC_INLINE void add_to_cl_list(blockinfo* bi)
{
  uae_u32 cl = cacheline(bi->pc_p);

  if (cache_tags[cl+1].bi)
  	cache_tags[cl+1].bi->prev_same_cl_p = &(bi->next_same_cl);
  bi->next_same_cl = cache_tags[cl+1].bi;

  cache_tags[cl+1].bi = bi;
  bi->prev_same_cl_p = &(cache_tags[cl+1].bi);

  cache_tags[cl].handler = bi->handler_to_use;
}

STATIC_INLINE void raise_in_cl_list(blockinfo* bi)
{
  remove_from_cl_list(bi);
  add_to_cl_list(bi);
}

STATIC_INLINE void add_to_active(blockinfo* bi)
{
  if (active)
  	active->prev_p = &(bi->next);
  bi->next = active;

  active = bi;
  bi->prev_p = &active;
}

STATIC_INLINE void add_to_dormant(blockinfo* bi)
{
  if (dormant)
  	dormant->prev_p = &(bi->next);
  bi->next = dormant;

  dormant = bi;
  bi->prev_p = &dormant;
}

STATIC_INLINE void remove_dep(dependency* d)
{
  if (d->prev_p)
  	*(d->prev_p) = d->next;
  if (d->next)
  	d->next->prev_p = d->prev_p;
  d->prev_p = NULL;
  d->next = NULL;
}

/* This block's code is about to be thrown away, so it no longer
   depends on anything else */
STATIC_INLINE void remove_deps(blockinfo* bi)
{
  remove_dep(&(bi->dep[0]));
  remove_dep(&(bi->dep[1]));
}

STATIC_INLINE void adjust_jmpdep(dependency* d, cpuop_func* a)
{
	write_jmp_target(d->jmp_off, a);
}

/********************************************************************
 * Soft flush handling support functions                            *
 ********************************************************************/

STATIC_INLINE void set_dhtu(blockinfo* bi, cpuop_func* dh)
{
  D2(panicbug("bi is %p\n", bi));
  if (dh != bi->direct_handler_to_use) {
  	dependency* x = bi->deplist;
  	D2(panicbug("bi->deplist=%p\n", bi->deplist));
  	while (x) {
	    D2(panicbug("x is %p\n", x));
	    D2(panicbug("x->next is %p\n", x->next));
	    D2(panicbug("x->prev_p is %p\n", x->prev_p));

	    if (x->jmp_off) {
    		adjust_jmpdep(x, dh);
	    }
	    x = x->next;
  	}
  	bi->direct_handler_to_use = (cpuop_func*)dh;
  }
}

STATIC_INLINE void invalidate_block(blockinfo* bi)
{
  int i;

  bi->optlevel = 0;
  bi->count = currprefs.optcount[0]-1;
  bi->handler = NULL;
  bi->handler_to_use = (cpuop_func *)popall_execute_normal;
  bi->direct_handler = NULL;
  set_dhtu(bi, bi->direct_pen);
  bi->needed_flags = 0xff;

	bi->status = BI_INVALID;
  for (i=0; i<2; i++) {
	  bi->dep[i].jmp_off = NULL;
	  bi->dep[i].target = NULL;
  }
  remove_deps(bi);
}

STATIC_INLINE void create_jmpdep(blockinfo* bi, int i, uae_u32* jmpaddr, uae_u32 target)
{
  blockinfo* tbi = get_blockinfo_addr((void*)(uintptr)target);

  Dif(!tbi) {
  	jit_abort (_T("JIT: Could not create jmpdep!\n"));
  }
  bi->dep[i].jmp_off = jmpaddr;
	bi->dep[i].source = bi;
  bi->dep[i].target = tbi;
  bi->dep[i].next = tbi->deplist;
  if (bi->dep[i].next)
  	bi->dep[i].next->prev_p = &(bi->dep[i].next);
  bi->dep[i].prev_p = &(tbi->deplist);
  tbi->deplist = &(bi->dep[i]);
}

STATIC_INLINE void block_need_recompile(blockinfo * bi)
{
  uae_u32 cl = cacheline(bi->pc_p);

	set_dhtu(bi,bi->direct_pen);
  bi->direct_handler = bi->direct_pen;

  bi->handler_to_use = (cpuop_func *)popall_execute_normal;
  bi->handler = (cpuop_func *)popall_execute_normal;
  if (bi == cache_tags[cl + 1].bi)
	  cache_tags[cl].handler = (cpuop_func *)popall_execute_normal;
  bi->status = BI_NEED_RECOMP;
}

STATIC_INLINE void mark_callers_recompile(blockinfo * bi)
{
  dependency *x = bi->deplist;

  while (x)	{
  	dependency *next = x->next;	/* This disappears when we mark for
								 * recompilation and thus remove the
								 * blocks from the lists */
  	if (x->jmp_off) {
  	  blockinfo *cbi = x->source;

  	  Dif(cbi->status == BI_INVALID) {
    		jit_abort(_T("invalid block in dependency list\n")); // FIXME?
  	  }
  	  if (cbi->status == BI_ACTIVE || cbi->status == BI_NEED_CHECK) {
  		  block_need_recompile(cbi);
  		  mark_callers_recompile(cbi);
  	  }
  	  else if (cbi->status == BI_COMPILING) {
    		redo_current_block = 1;
  	  }
  	  else if (cbi->status == BI_NEED_RECOMP) {
    		/* nothing */
  	  }
  	  else {
    		D2(panicbug(_T("Status %d in mark_callers\n"), cbi->status)); // FIXME?
  	  }
  	}
  	x = next;
  }
}

STATIC_INLINE blockinfo* get_blockinfo_addr_new(void* addr, int setstate)
{
  blockinfo* bi = get_blockinfo_addr(addr);
  int i;

  if (!bi) {
  	for (i=0; i<MAX_HOLD_BI && !bi; i++) {
	    if (hold_bi[i]) {
    		(void)cacheline(addr);

		    bi = hold_bi[i];
		    hold_bi[i] = NULL;
		    bi->pc_p = (uae_u8 *)addr;
		    invalidate_block(bi);
		    add_to_active(bi);
		    add_to_cl_list(bi);
	    }
  	}
  }
  if (!bi) {
  	jit_abort (_T("JIT: Looking for blockinfo, can't find free one\n"));
  }
  return bi;
}

static void prepare_block(blockinfo* bi);

/* Managment of blockinfos.

   A blockinfo struct is allocated whenever a new block has to be
   compiled. If the list of free blockinfos is empty, we allocate a new
   pool of blockinfos and link the newly created blockinfos altogether
   into the list of free blockinfos. Otherwise, we simply pop a structure
   of the free list.

   Blockinfo are lazily deallocated, i.e. chained altogether in the
   list of free blockinfos whenvever a translation cache flush (hard or
   soft) request occurs.
*/

template< class T >
class LazyBlockAllocator
{
	enum {
		kPoolSize = 1 + (16384 - sizeof(T) - sizeof(void *)) / sizeof(T)
	};
	struct Pool {
		T chunk[kPoolSize];
		Pool * next;
	};
	Pool * mPools;
	T * mChunks;
public:
	LazyBlockAllocator() : mPools(0), mChunks(0) { }
	~LazyBlockAllocator();
	T * acquire();
	void release(T * const);
};

template< class T >
LazyBlockAllocator<T>::~LazyBlockAllocator()
{
	Pool * currentPool = mPools;
	while (currentPool) {
		Pool * deadPool = currentPool;
		currentPool = currentPool->next;
		free(deadPool);
	}
}

template< class T >
T * LazyBlockAllocator<T>::acquire()
{
	if (!mChunks) {
		// There is no chunk left, allocate a new pool and link the
		// chunks into the free list
		Pool * newPool = (Pool *)malloc(sizeof(Pool));
		if (newPool == NULL) {
	    jit_abort(_T("FATAL: Could not allocate block pool!\n"));
    }
		for (T * chunk = &newPool->chunk[0]; chunk < &newPool->chunk[kPoolSize]; chunk++) {
			chunk->next = mChunks;
			mChunks = chunk;
		}
		newPool->next = mPools;
		mPools = newPool;
	}
	T * chunk = mChunks;
	mChunks = chunk->next;
	return chunk;
}

template< class T >
void LazyBlockAllocator<T>::release(T * const chunk)
{
	chunk->next = mChunks;
	mChunks = chunk;
}

template< class T >
class HardBlockAllocator
{
public:
	T * acquire() {
		T * data = (T *)current_compile_p;
		current_compile_p += sizeof(T);
		return data;
	}

	void release(T * const chunk) {
		// Deallocated on invalidation
	}
};

#if USE_SEPARATE_BIA
static LazyBlockAllocator<blockinfo> BlockInfoAllocator;
static LazyBlockAllocator<checksum_info> ChecksumInfoAllocator;
#else
static HardBlockAllocator<blockinfo> BlockInfoAllocator;
static HardBlockAllocator<checksum_info> ChecksumInfoAllocator;
#endif

STATIC_INLINE checksum_info *alloc_checksum_info(void)
{
	checksum_info *csi = ChecksumInfoAllocator.acquire();
	csi->next = NULL;
	return csi;
}

STATIC_INLINE void free_checksum_info(checksum_info *csi)
{
	csi->next = NULL;
	ChecksumInfoAllocator.release(csi);
}

STATIC_INLINE void free_checksum_info_chain(checksum_info *csi)
{
	while (csi != NULL) {
		checksum_info *csi2 = csi->next;
		free_checksum_info(csi);
		csi = csi2;
	}
}

STATIC_INLINE blockinfo *alloc_blockinfo(void)
{
	blockinfo *bi = BlockInfoAllocator.acquire();
#if USE_CHECKSUM_INFO
	bi->csi = NULL;
#endif
	return bi;
}

STATIC_INLINE void free_blockinfo(blockinfo *bi)
{
#if USE_CHECKSUM_INFO
	free_checksum_info_chain(bi->csi);
	bi->csi = NULL;
#endif
	BlockInfoAllocator.release(bi);
}

STATIC_INLINE void alloc_blockinfos(void)
{
  int i;
  blockinfo* bi;

  for (i=0;i<MAX_HOLD_BI;i++) {
  	if (hold_bi[i])
	    return;
  	bi=hold_bi[i]=alloc_blockinfo();

  	prepare_block(bi);
  }
}

/********************************************************************
 * Preferences handling. This is just a convenient place to put it  *
 ********************************************************************/
bool check_prefs_changed_comp (void)
{
  bool changed = 0;

  if (currprefs.cachesize != changed_prefs.cachesize) {
	  currprefs.cachesize = changed_prefs.cachesize;
	  alloc_cache();
	  changed = 1;
  }
  return changed;
}

/********************************************************************
 * Functions to emit data into memory, and other general support    *
 ********************************************************************/

static uae_u8* target;

STATIC_INLINE void emit_byte(uae_u8 x)
{
  *target++ = x;
}

STATIC_INLINE void emit_long(uae_u32 x)
{
  *((uae_u32*)target) = x;
  target += 4;
}

#define MAX_COMPILE_PTR	max_compile_start

STATIC_INLINE uae_u32 reverse32(uae_u32 v)
{
#if 1
	// gb-- We have specialized byteswapping functions, just use them
	return do_byteswap_32(v);
#else
	return ((v>>24)&0xff) | ((v>>8)&0xff00) | ((v<<8)&0xff0000) | ((v<<24)&0xff000000);
#endif
}

void set_target(uae_u8* t)
{
  target = t;
}

STATIC_INLINE uae_u8* get_target_noopt(void)
{
  return target;
}

STATIC_INLINE uae_u8* get_target(void)
{
  return get_target_noopt();
}


/********************************************************************
 * New version of data buffer: interleave data and code             *
 ********************************************************************/
#if defined(CPU_arm) && !defined(ARMV6T2)

#define DATA_BUFFER_SIZE 768             // Enlarge POPALLSPACE_SIZE if this value is greater than 768
#define DATA_BUFFER_MAXOFFSET 4096 - 32  // max range between emit of data and use of data
static uae_u8* data_writepos = 0;
static uae_u8* data_endpos = 0;
#ifdef DEBUG_DATA_BUFFER
static uae_u32 data_wasted = 0;
static uae_u32 data_buffers_used = 0;
#endif

static uae_s32 data_natmem_pos = 0;

STATIC_INLINE void compemu_raw_branch(IMM d);

STATIC_INLINE void data_check_end(uae_s32 n, uae_s32 codesize)
{
  if(data_writepos + n > data_endpos || get_target_noopt() + codesize - data_writepos > DATA_BUFFER_MAXOFFSET)
  {
    // Start new buffer
#ifdef DEBUG_DATA_BUFFER
    if(data_writepos < data_endpos)
      data_wasted += data_endpos - data_writepos;
    data_buffers_used++;
#endif
    compemu_raw_branch(DATA_BUFFER_SIZE);
    data_writepos = get_target_noopt();
    data_endpos = data_writepos + DATA_BUFFER_SIZE;
    set_target(get_target_noopt() + DATA_BUFFER_SIZE);

    data_natmem_pos = 0;
  }
}

STATIC_INLINE uae_s32 data_word_offs(uae_u16 x)
{
  data_check_end(4, 4);
  *((uae_u16*)data_writepos) = x;
  data_writepos += 2;
  *((uae_u16*)data_writepos) = 0;
  data_writepos += 2;
  return (uae_s32)data_writepos - (uae_s32)get_target_noopt() - 12;
}

STATIC_INLINE uae_s32 data_long(uae_u32 x, uae_s32 codesize)
{
  data_check_end(4, codesize);
  *((uae_u32*)data_writepos) = x;
  data_writepos += 4;
  return (uae_s32)data_writepos - 4;
}

STATIC_INLINE uae_s32 data_long_offs(uae_u32 x)
{
  data_check_end(4, 4);
  *((uae_u32*)data_writepos) = x;
  data_writepos += 4;
  return (uae_s32)data_writepos - (uae_s32)get_target_noopt() - 12;
}

STATIC_INLINE uae_s32 get_data_offset(uae_s32 t) 
{
	return t - (uae_s32)get_target_noopt() - 8;
}

STATIC_INLINE uae_s32 get_data_natmem(void)
{
  if(data_natmem_pos == 0 || (uae_s32)get_target_noopt() - data_natmem_pos >= DATA_BUFFER_MAXOFFSET)
  {
    data_natmem_pos = data_long(NATMEM_OFFSETX, 4);
  }
  return get_data_offset(data_natmem_pos);
}

STATIC_INLINE void reset_data_buffer(void)
{
  data_writepos = 0;
  data_endpos = 0;
}

#endif

/********************************************************************
 * Getting the information about the target CPU                     *
 ********************************************************************/
STATIC_INLINE void clobber_flags(void);

#if defined(CPU_arm) 
#include "codegen_arm.cpp"
#else
#include "compemu_raw_x86.cpp"
#endif


/********************************************************************
 * Flags status handling. EMIT TIME!                                *
 ********************************************************************/

static void make_flags_live_internal(void)
{
  if (live.flags_in_flags == VALID)
  	return;
  Dif (live.flags_on_stack == TRASH) {
  	jit_abort (_T("JIT: Want flags, got something on stack, but it is TRASH\n"));
  }
  if (live.flags_on_stack == VALID) {
	  int tmp;
	  tmp = readreg_specific(FLAGTMP, 4, FLAG_NREG2);
	  raw_reg_to_flags(tmp);
	  unlock2(tmp);

    live.flags_in_flags = VALID;
    return;
  }
  jit_abort (_T("JIT: Huh? live.flags_in_flags=%d, live.flags_on_stack=%d, but need to make live\n"),
    live.flags_in_flags, live.flags_on_stack);
}

static void flags_to_stack(void)
{
  if (live.flags_on_stack == VALID)
  	return;
  if (!live.flags_are_important) {
	  live.flags_on_stack = VALID;
	  return;
  }
  Dif (live.flags_in_flags != VALID)
	  jit_abort(_T("flags_to_stack != VALID"));
  else  {
	  int tmp;
	  tmp = writereg_specific(FLAGTMP, 4, FLAG_NREG1);
	  raw_flags_to_reg(tmp);
	  unlock2(tmp);
  }
  live.flags_on_stack = VALID;
}

STATIC_INLINE void clobber_flags(void)
{
  if (live.flags_in_flags == VALID && live.flags_on_stack != VALID)
  	flags_to_stack();
  live.flags_in_flags = TRASH;
}

/* Prepare for leaving the compiled stuff */
STATIC_INLINE void flush_flags(void)
{
  flags_to_stack();
}

int touchcnt;

/********************************************************************
 * register allocation per block logging                            *
 ********************************************************************/

#define L_UNKNOWN -127
#define L_UNAVAIL -1
#define L_NEEDED -2
#define L_UNNEEDED -3

STATIC_INLINE void do_load_reg(int n, int r)
{
  if (r == FLAGTMP)
	  raw_load_flagreg(n, r);
  else if (r == FLAGX)
	  raw_load_flagx(n, r);
  else
	  compemu_raw_mov_l_rm(n, (uintptr) live.state[r].mem);
}

/********************************************************************
 * register status handling. EMIT TIME!                             *
 ********************************************************************/

STATIC_INLINE void set_status(int r, int status)
{
  live.state[r].status = status;
}

STATIC_INLINE int isinreg(int r)
{
  return live.state[r].status == CLEAN || live.state[r].status == DIRTY;
}

static void tomem(int r)
{
  int rr = live.state[r].realreg;

  if (live.state[r].status == DIRTY) {
  	switch (live.state[r].dirtysize) {
	    case 1: compemu_raw_mov_b_mr((uintptr)live.state[r].mem, rr); break;
	    case 2: compemu_raw_mov_w_mr((uintptr)live.state[r].mem, rr); break;
	    case 4: compemu_raw_mov_l_mr((uintptr)live.state[r].mem, rr); break;
	    default: abort();
	  }
	  set_status(r, CLEAN);
	  live.state[r].dirtysize = 0;
  }
}

STATIC_INLINE int isconst(int r)
{
  return live.state[r].status == ISCONST;
}

int is_const(int r)
{
  return isconst(r);
}

STATIC_INLINE void writeback_const(int r)
{
  if (!isconst(r))
  	return;

  compemu_raw_mov_l_mi((uintptr)live.state[r].mem, live.state[r].val);
  live.state[r].val = 0;
  set_status(r, INMEM);
}

STATIC_INLINE void tomem_c(int r)
{
  if (isconst(r)) {
  	writeback_const(r);
  }
  else
  	tomem(r);
}

static  void evict(int r)
{
  int rr;

  if (!isinreg(r))
  	return;
  tomem(r);
  rr = live.state[r].realreg;

  Dif (live.nat[rr].locked &&
  	live.nat[rr].nholds == 1) {
    	jit_abort (_T("JIT: register %d in nreg %d is locked!\n"), r, live.state[r].realreg);
  }

  live.nat[rr].nholds--;
  if (live.nat[rr].nholds != live.state[r].realind) { /* Was not last */
	  int topreg = live.nat[rr].holds[live.nat[rr].nholds];
	  int thisind = live.state[r].realind;
	  live.nat[rr].holds[thisind] = topreg;
	  live.state[topreg].realind = thisind;
  }
  live.state[r].realreg = -1;
  set_status(r, INMEM);
}

STATIC_INLINE void free_nreg(int r)
{
  int i = live.nat[r].nholds;

  while (i) {
  	int vr;

	  --i;
	  vr = live.nat[r].holds[i];
	  evict(vr);
  }
  Dif (live.nat[r].nholds != 0) {
  	jit_abort (_T("JIT: Failed to free nreg %d, nholds is %d\n"), r, live.nat[r].nholds);
  }
}

/* Use with care! */
STATIC_INLINE void isclean(int r)
{
  if (!isinreg(r))
  	return;
  live.state[r].validsize = 4;
  live.state[r].dirtysize = 0;
  live.state[r].val = 0;
  set_status(r, CLEAN);
}

STATIC_INLINE void disassociate(int r)
{
  isclean(r);
  evict(r);
}

STATIC_INLINE void set_const(int r, uae_u32 val)
{
  disassociate(r);
  live.state[r].val = val;
  set_status(r, ISCONST);
}

static  int alloc_reg_hinted(int r, int size, int willclobber, int hint)
{
  int bestreg;
  uae_s32 when;
  int i;
  uae_s32 badness = 0; /* to shut up gcc */
  bestreg = -1;
  when = 2000000000;

  /* XXX use a regalloc_order table? */
  for (i=0; i<N_REGS; i++) {
  	badness = live.nat[i].touched;
  	if (live.nat[i].nholds == 0)
	    badness = 0;
  	if (i == hint)
	    badness -= 200000000;
  	if (!live.nat[i].locked && badness<when) {
    		bestreg = i;
    		when = badness;
    		if (live.nat[i].nholds == 0 && hint < 0)
  		    break;
    		if (i == hint)
  		    break;
	  }
  }
  Dif (bestreg == -1)
	  jit_abort(_T("alloc_reg_hinted bestreg=-1"));

  if (live.nat[bestreg].nholds > 0) {
  	free_nreg(bestreg);
  }
  if (isinreg(r)) {
  	int rr = live.state[r].realreg;
  	/* This will happen if we read a partially dirty register at a
	   bigger size */
  	Dif (willclobber || live.state[r].validsize >= size)
	    jit_abort(_T("willclobber || live.state[r].validsize>=size"));
  	Dif (live.nat[rr].nholds != 1)
	    jit_abort(_T("live.nat[rr].nholds!=1"));
  	if (size == 4 && live.state[r].validsize == 2) {
	    compemu_raw_mov_l_rm(bestreg, (uintptr)live.state[r].mem);
	    compemu_raw_MERGE_rr(rr, bestreg);
	    live.state[r].validsize = 4;
	    live.nat[rr].touched = touchcnt++;
	    return rr;
	  }
	  if (live.state[r].validsize == 1) {
	    /* Nothing yet */
	  }
	  evict(r);
  }

  if (!willclobber) {
	  if (live.state[r].status != UNDEF) {
	    if (isconst(r)) {
		    compemu_raw_mov_l_ri(bestreg, live.state[r].val);
		    live.state[r].val = 0;
		    live.state[r].dirtysize = 4;
		    set_status(r, DIRTY);
	    }
	    else {
		    do_load_reg(bestreg, r);
		    live.state[r].dirtysize = 0;
		    set_status(r, CLEAN);
	    }
  	}
  	else {
	    live.state[r].val = 0;
	    live.state[r].dirtysize = 0;
	    set_status(r, CLEAN);
	  }
	  live.state[r].validsize = 4;
  }
  else { /* this is the easiest way, but not optimal. FIXME! */
  	/* Now it's trickier, but hopefully still OK */
  	if (!isconst(r) || size == 4) {
	    live.state[r].validsize = size;
	    live.state[r].dirtysize = size;
	    live.state[r].val = 0;
	    set_status(r, DIRTY);
  	}
  	else {
      if (live.state[r].status != UNDEF)
    		compemu_raw_mov_l_ri(bestreg, live.state[r].val);
      live.state[r].val = 0;
      live.state[r].validsize = 4;
      live.state[r].dirtysize = 4;
      set_status(r, DIRTY);
  	}
  }
  live.state[r].realreg = bestreg;
  live.state[r].realind = live.nat[bestreg].nholds;
  live.nat[bestreg].touched = touchcnt++;
  live.nat[bestreg].holds[live.nat[bestreg].nholds] = r;
  live.nat[bestreg].nholds++;

  return bestreg;
}


static void unlock2(int r)
{
  Dif (!live.nat[r].locked)
  	jit_abort(_T("unlock2 %d not locked"), r);
  live.nat[r].locked--;
}

static  void setlock(int r)
{
  live.nat[r].locked++;
}


static void mov_nregs(int d, int s)
{
  int nd = live.nat[d].nholds;
  int i;

  if (s == d)
  	return;

  if (nd > 0)
  	free_nreg(d);

  compemu_raw_mov_l_rr(d, s);

  for (i=0; i<live.nat[s].nholds; i++) {
  	int vs = live.nat[s].holds[i];

	  live.state[vs].realreg = d;
	  live.state[vs].realind = i;
	  live.nat[d].holds[i] = vs;
  }
  live.nat[d].nholds = live.nat[s].nholds;

  live.nat[s].nholds = 0;
}


STATIC_INLINE void make_exclusive(int r, int size, int spec)
{
  reg_status oldstate;
  int rr = live.state[r].realreg;
  int nr;
  int nind;
  int ndirt = 0;
  int i;

  if (!isinreg(r))
  	return;
  if (live.nat[rr].nholds == 1)
  	return;
  for (i=0; i<live.nat[rr].nholds; i++) {
	  int vr = live.nat[rr].holds[i];
	  if (vr != r &&
	    (live.state[vr].status == DIRTY || live.state[vr].val))
    ndirt++;
  }
  if (!ndirt && size < live.state[r].validsize && !live.nat[rr].locked) {
  	/* Everything else is clean, so let's keep this register */
  	for (i=0; i<live.nat[rr].nholds; i++) {
      int vr = live.nat[rr].holds[i];
      if (vr != r) {
	      evict(vr);
	      i--; /* Try that index again! */
	    }
  	}
  	Dif (live.nat[rr].nholds != 1) {
	    jit_abort (_T("JIT: natreg %d holds %d vregs, %d not exclusive\n"),
	      rr, live.nat[rr].nholds, r);
    }
  	return;
  }

  /* We have to split the register */
  oldstate = live.state[r];

  setlock(rr); /* Make sure this doesn't go away */
  /* Forget about r being in the register rr */
  disassociate(r);
  /* Get a new register, that we will clobber completely */
  if (oldstate.status == DIRTY) {
  	/* If dirtysize is <4, we need a register that can handle the
	  eventual smaller memory store! Thanks to Quake68k for exposing
    this detail ;-) */
    nr = alloc_reg_hinted(r, oldstate.dirtysize, 1, spec);
  }
  else {
  	nr = alloc_reg_hinted(r, 4, 1, spec);
  }
  nind = live.state[r].realind;
  live.state[r] = oldstate;   /* Keep all the old state info */
  live.state[r].realreg = nr;
  live.state[r].realind = nind;

  if (size < live.state[r].validsize) {
  	if (live.state[r].val) {
      /* Might as well compensate for the offset now */
      compemu_raw_lea_l_brr(nr,rr,oldstate.val);
      live.state[r].val = 0;
      live.state[r].dirtysize = 4;
      set_status(r, DIRTY);
  	}
  	else
	    compemu_raw_mov_l_rr(nr, rr);  /* Make another copy */
  }
  unlock2(rr);
}

STATIC_INLINE int readreg_general(int r, int size, int spec)
{
  int n;
  int answer = -1;

	if (live.state[r].status == UNDEF) {
		D(panicbug("JIT: WARNING: Unexpected read of undefined register %d\n", r));
	}

  if (isinreg(r) && live.state[r].validsize >= size) {
	  n = live.state[r].realreg;

    answer = n;

	  if (answer < 0)
	    evict(r);
  }
  /* either the value was in memory to start with, or it was evicted and
     is in memory now */
  if (answer < 0) {
  	answer = alloc_reg_hinted(r, spec >= 0 ? 4 : size, 0, spec);
  }

  if (spec >= 0 && spec != answer) {
	  /* Too bad */
	  mov_nregs(spec, answer);
	  answer = spec;
  }
  live.nat[answer].locked++;
  live.nat[answer].touched = touchcnt++;
  return answer;
}



static int readreg(int r, int size)
{
  return readreg_general(r, size, -1);
}

static int readreg_specific(int r, int size, int spec)
{
  return readreg_general(r, size, spec);
}

/* writereg_general(r, size, spec)
 *
 * INPUT
 * - r    : mid-layer register
 * - size : requested size (1/2/4)
 * - spec : -1 if find or make a register free, otherwise specifies
 *          the physical register to use in any case
 *
 * OUTPUT
 * - hard (physical, x86 here) register allocated to virtual register r
 */
STATIC_INLINE int writereg_general(int r, int size, int spec)
{
  int n;
  int answer = -1;

  make_exclusive(r, size, spec);
  if (isinreg(r)) {
	  int nvsize = size > live.state[r].validsize ? size : live.state[r].validsize;
	  int ndsize = size > live.state[r].dirtysize ? size : live.state[r].dirtysize;
	  n = live.state[r].realreg;

  	Dif (live.nat[n].nholds != 1)
	    jit_abort(_T("live.nat[%d].nholds!=1"), n);

    live.state[r].dirtysize = ndsize;
    live.state[r].validsize = nvsize;
    answer = n;

	  if (answer < 0)
	    evict(r);
  }
  /* either the value was in memory to start with, or it was evicted and
     is in memory now */
  if (answer < 0) {
  	answer = alloc_reg_hinted(r, size, 1, spec);
  }
  if (spec >= 0 && spec != answer) {
	  mov_nregs(spec, answer);
	  answer = spec;
  }
  if (live.state[r].status == UNDEF)
  	live.state[r].validsize = 4;
  live.state[r].dirtysize = size > live.state[r].dirtysize ? size : live.state[r].dirtysize;
  live.state[r].validsize = size > live.state[r].validsize ? size : live.state[r].validsize;

  live.nat[answer].locked++;
  live.nat[answer].touched = touchcnt++;
  if (size == 4) {
    live.state[r].val = 0;
  }
  else {
    Dif (live.state[r].val) {
      jit_abort (_T("JIT: Problem with val\n"));
  	}
  }
  set_status(r, DIRTY);
  return answer;
}

static int writereg(int r, int size)
{
  return writereg_general(r, size, -1);
}

static int writereg_specific(int r, int size, int spec)
{
  return writereg_general(r, size, spec);
}

STATIC_INLINE int rmw_general(int r, int wsize, int rsize)
{
  int n;
  int answer = -1;

  if (live.state[r].status == UNDEF) {
		D(panicbug("JIT: WARNING: Unexpected read of undefined register %d\n", r));
	}
  make_exclusive(r, 0, -1);

  Dif (wsize < rsize) {
  	jit_abort (_T("JIT: Cannot handle wsize<rsize in rmw_general()\n"));
  }
  if (isinreg(r) && live.state[r].validsize >= rsize) {
	  n = live.state[r].realreg;
	  Dif (live.nat[n].nholds != 1)
	    jit_abort(_T("live.nat[n].nholds!=1"), n);

    answer = n;
	  if (answer < 0)
	    evict(r);
  }
  /* either the value was in memory to start with, or it was evicted and
     is in memory now */
  if (answer < 0) {
  	answer = alloc_reg_hinted(r, rsize, 0, -1);
  }

  if (wsize > live.state[r].dirtysize)
  	live.state[r].dirtysize = wsize;
  if (wsize > live.state[r].validsize)
  	live.state[r].validsize = wsize;
  set_status(r, DIRTY);

  live.nat[answer].locked++;
  live.nat[answer].touched = touchcnt++;

  Dif (live.state[r].val) {
  	jit_abort (_T("JIT: Problem with val(rmw)\n"));
  }
  return answer;
}

static int rmw(int r, int wsize, int rsize)
{
  return rmw_general(r, wsize, rsize);
}

/********************************************************************
 * FPU register status handling. EMIT TIME!                         *
 ********************************************************************/

#ifdef USE_JIT_FPU
static  void f_tomem(int r)
{
  if (live.fate[r].status == DIRTY) {
#if USE_LONG_DOUBLE
  	raw_fmov_ext_mr((uintptr)live.fate[r].mem, live.fate[r].realreg); 
#else
  	raw_fmov_mr((uintptr)live.fate[r].mem, live.fate[r].realreg); 
#endif
  	live.fate[r].status = CLEAN;
  }
}

static  void f_tomem_drop(int r)
{
  if (live.fate[r].status == DIRTY) {
#if USE_LONG_DOUBLE
  	raw_fmov_ext_mr_drop((uintptr)live.fate[r].mem, live.fate[r].realreg); 
#else
  	raw_fmov_mr_drop((uintptr)live.fate[r].mem, live.fate[r].realreg); 
#endif
  	live.fate[r].status = INMEM;
  }
}


STATIC_INLINE int f_isinreg(int r)
{
  return live.fate[r].status == CLEAN || live.fate[r].status == DIRTY;
}

static void f_evict(int r)
{
  int rr;

  if (!f_isinreg(r))
  	return;
  rr = live.fate[r].realreg;
  if (live.fat[rr].nholds == 1)
  	f_tomem_drop(r);
  else
  	f_tomem(r);

  Dif (live.fat[rr].locked &&
  	live.fat[rr].nholds==1) {
  	jit_abort (_T("JIT: FPU register %d in nreg %d is locked!\n"),r,live.fate[r].realreg);
  }
  live.fat[rr].nholds--;
  if (live.fat[rr].nholds!=live.fate[r].realind) { /* Was not last */
	  int topreg=live.fat[rr].holds[live.fat[rr].nholds];
	  int thisind=live.fate[r].realind;
	  live.fat[rr].holds[thisind]=topreg;
	  live.fate[topreg].realind=thisind;
  }
  live.fate[r].status=INMEM;
  live.fate[r].realreg=-1;
}

STATIC_INLINE void f_free_nreg(int r)
{
  int i=live.fat[r].nholds;

  while (i) {
  	int vr;

	  --i;
	  vr=live.fat[r].holds[i];
	  f_evict(vr);
  }
  Dif (live.fat[r].nholds!=0) {
  	jit_abort (_T("JIT: Failed to free nreg %d, nholds is %d\n"),r,live.fat[r].nholds);
  }
}


/* Use with care! */
STATIC_INLINE void f_isclean(int r)
{
  if (!f_isinreg(r))
  	return;
  live.fate[r].status=CLEAN;
}

STATIC_INLINE void f_disassociate(int r)
{
  f_isclean(r);
  f_evict(r);
}



static  int f_alloc_reg(int r, int willclobber)
{
  int bestreg;
  uae_s32 when;
  int i;
  uae_s32 badness;
  bestreg=-1;
  when=2000000000;
  for (i=N_FREGS;i--;) {
	  badness=live.fat[i].touched;
	  if (live.fat[i].nholds==0)
	    badness=0;

  	if (!live.fat[i].locked && badness<when) {
	    bestreg=i;
	    when=badness;
	    if (live.fat[i].nholds==0)
    		break;
  	}
  }
  Dif (bestreg==-1)
  	abort();

  if (live.fat[bestreg].nholds>0) {
  	f_free_nreg(bestreg);
  }
  if (f_isinreg(r)) {
  	f_evict(r);
  }

  if (!willclobber) {
  	if (live.fate[r].status!=UNDEF) {
#if USE_LONG_DOUBLE
	    raw_fmov_ext_rm(bestreg,(uintptr)live.fate[r].mem);
#else
	    raw_fmov_rm(bestreg,(uintptr)live.fate[r].mem);
#endif
  	}
	  live.fate[r].status=CLEAN;
  }
  else {
  	live.fate[r].status=DIRTY;
  }
  live.fate[r].realreg=bestreg;
  live.fate[r].realind=live.fat[bestreg].nholds;
  live.fat[bestreg].touched=touchcnt++;
  live.fat[bestreg].holds[live.fat[bestreg].nholds]=r;
  live.fat[bestreg].nholds++;

  return bestreg;
}

static  void f_unlock(int r)
{
  Dif (!live.fat[r].locked)
  	jit_abort(_T("unlock %d"), r);
  live.fat[r].locked--;
}

static  void f_setlock(int r)
{
  live.fat[r].locked++;
}

STATIC_INLINE int f_readreg(int r)
{
  int n;
  int answer=-1;

  if (f_isinreg(r)) {
	  n=live.fate[r].realreg;
	  answer=n;
  }
  /* either the value was in memory to start with, or it was evicted and
     is in memory now */
  if (answer<0)
  	answer=f_alloc_reg(r,0);

  live.fat[answer].locked++;
  live.fat[answer].touched=touchcnt++;
  return answer;
}

STATIC_INLINE void f_make_exclusive(int r, int clobber)
{
  freg_status oldstate;
  int rr=live.fate[r].realreg;
  int nr;
  int nind;
  int ndirt=0;
  int i;

  if (!f_isinreg(r))
  	return;
  if (live.fat[rr].nholds==1)
  	return;
  for (i=0;i<live.fat[rr].nholds;i++) {
	  int vr=live.fat[rr].holds[i];
	  if (vr!=r && live.fate[vr].status==DIRTY)
	    ndirt++;
  }
  if (!ndirt && !live.fat[rr].locked) {
  	/* Everything else is clean, so let's keep this register */
  	for (i=0;i<live.fat[rr].nholds;i++) {
	    int vr=live.fat[rr].holds[i];
	    if (vr!=r) {
    		f_evict(vr);
    		i--; /* Try that index again! */
	    }
  	}
  	Dif (live.fat[rr].nholds!=1) {
	    D(panicbug("JIT: realreg %d holds %d (",rr,live.fat[rr].nholds));
	    for (i=0;i<live.fat[rr].nholds;i++) {
    		D(panicbug("JIT: %d(%d,%d)",live.fat[rr].holds[i],
		       live.fate[live.fat[rr].holds[i]].realreg,
		       live.fate[live.fat[rr].holds[i]].realind));
	    }
	    jit_abort(_T("x"));
	  }
	  return;
  }

  /* We have to split the register */
  oldstate=live.fate[r];

  f_setlock(rr); /* Make sure this doesn't go away */
  /* Forget about r being in the register rr */
  f_disassociate(r);
  /* Get a new register, that we will clobber completely */
  nr=f_alloc_reg(r,1);
  nind=live.fate[r].realind;
  if (!clobber)
  	raw_fmov_rr(nr,rr);  /* Make another copy */
  live.fate[r]=oldstate;   /* Keep all the old state info */
  live.fate[r].realreg=nr;
  live.fate[r].realind=nind;
  f_unlock(rr);
}


STATIC_INLINE int f_writereg(int r)
{
  int n;
  int answer=-1;

  f_make_exclusive(r,1);
  if (f_isinreg(r)) {
	  n=live.fate[r].realreg;
	  answer=n;
  }
  if (answer<0) {
  	answer=f_alloc_reg(r,1);
  }
  live.fate[r].status=DIRTY;
  live.fat[answer].locked++;
  live.fat[answer].touched=touchcnt++;
  return answer;
}

static int f_rmw(int r)
{
  int n;

  f_make_exclusive(r,0);
  if (f_isinreg(r)) {
  	n=live.fate[r].realreg;
  }
  else
  	n=f_alloc_reg(r,0);
  live.fate[r].status=DIRTY;
  live.fat[n].locked++;
  live.fat[n].touched=touchcnt++;
  return n;
}

static void fflags_into_flags_internal(uae_u32 tmp)
{
  int r;

  clobber_flags();
  r=f_readreg(FP_RESULT);
	if (FFLAG_NREG_CLOBBER_CONDITION) {
	  int tmp2=tmp;
	  tmp=writereg_specific(tmp,4,FFLAG_NREG);
	  raw_fflags_into_flags(r);
	  unlock2(tmp);
	  forget_about(tmp2);
	}
	else
    raw_fflags_into_flags(r);
  f_unlock(r);
  live_flags();
}

#endif //USE_JIT_FPU


#if defined(CPU_arm)
#include "compemu_midfunc_arm.cpp"
#include "compemu_midfunc_arm2.cpp"
#else
#include "compemu_midfunc_x86.c"
#endif


/********************************************************************
 * Support functions exposed to gencomp. CREATE time                *
 ********************************************************************/

uae_u32 get_const(int r)
{
	Dif (!isconst(r)) {
    jit_abort(_T("Register %d should be constant, but isn't\n"), r);
	}
  return live.state[r].val;
}

void sync_m68k_pc(void)
{
  if (m68k_pc_offset) {
	  add_l_ri(PC_P, m68k_pc_offset);
	  comp_pc_p += m68k_pc_offset;
	  m68k_pc_offset = 0;
  }
}

/********************************************************************
 * Scratch registers management                                     *
 ********************************************************************/

struct scratch_t {
  uae_u32 regs[VREGS];
#ifdef USE_JIT_FPU
  fptype fscratch[VFREGS];
#endif
};

static scratch_t scratch;

/********************************************************************
 * Support functions exposed to newcpu                              *
 ********************************************************************/

void compiler_init(void)
{
	static int initialized = 0;
	if (initialized)
		return;

#ifdef USE_JIT_FPU
	// Use JIT compiler for FPU instructions ?
	avoid_fpu = !bx_options.jit.jitfpu;
#else
	// JIT FPU is always disabled
	avoid_fpu = 1;
#endif
//	D(bug("<JIT compiler> : compile FPU instructions : %s", !avoid_fpu ? "yes" : "no"));
	
	// Initialize target CPU (check for features, e.g. CMOV, rat stalls)
	raw_init_cpu();

	// Translation cache flush mechanism
	lazy_flush = 1; //(bx_options.jit.jitlazyflush == 0) ? 0 : 1;
	flush_icache = lazy_flush ? flush_icache_lazy : flush_icache_hard;
	
	// Compiler features
#if USE_INLINING
	follow_const_jumps = 1; //bx_options.jit.jitinline;
#endif
	
	// Build compiler tables
	// build_comp(); // moved to newcpu.cpp -> build_cpufunctbl
  
	initialized = 1;

#ifdef PROFILE_UNTRANSLATED_INSNS
	bug("<JIT compiler> : gather statistics on untranslated insns count\n");
#endif

#ifdef PROFILE_COMPILE_TIME
	bug("<JIT compiler> : gather statistics on translation time");
	emul_start_time = clock();
#endif
}

void compiler_exit(void)
{
#ifdef PROFILE_COMPILE_TIME
	emul_end_time = clock();
#endif

#if defined(CPU_arm) && !defined(ARMV6T2)
#ifdef DEBUG_DATA_BUFFER
  printf("data_wasted = %d bytes in %d blocks\n", data_wasted, data_buffers_used);
#endif
#endif

	// Deallocate translation cache
	if (compiled_code) {
		cache_free(compiled_code, currprefs.cachesize * 1024);
		compiled_code = 0;
	}

	// Deallocate popallspace
	if (popallspace) {
		cache_free(popallspace, POPALLSPACE_SIZE);
		popallspace = 0;
	}

#ifdef PROFILE_COMPILE_TIME
	bug("### Compile Block statistics");
	bug("Number of calls to compile_block : %d", compile_count);
	uae_u32 emul_time = emul_end_time - emul_start_time;
	bug("Total emulation time   : %.1f sec", double(emul_time)/double(CLOCKS_PER_SEC));
	bug("Total compilation time : %.1f sec (%.1f%%)", double(compile_time)/double(CLOCKS_PER_SEC), 100.0*double(compile_time)/double(emul_time));
#endif

#ifdef PROFILE_UNTRANSLATED_INSNS
	uae_u64 untranslated_count = 0;
	for (int i = 0; i < 65536; i++) {
		opcode_nums[i] = i;
		untranslated_count += raw_cputbl_count[i];
	}
	bug("Sorting out untranslated instructions count...\n");
	qsort(opcode_nums, 65536, sizeof(uae_u16), untranslated_compfn);
	bug("Rank  Opc      Count Name\n");
	for (int i = 0; i < untranslated_top_ten && i < 65536; i++) {
		uae_u32 count = raw_cputbl_count[opcode_nums[i]];
		struct instr *dp;
		struct mnemolookup *lookup;
		if (!count)
			break;
		dp = table68k + opcode_nums[i];
		for (lookup = lookuptab; lookup->mnemo != (instrmnem)dp->mnemo; lookup++)
			;
		if(strcmp(lookup->name, "FPP") == 0
		|| strcmp(lookup->name, "FBcc") == 0
		|| strcmp(lookup->name, "DIVS") == 0
		|| strcmp(lookup->name, "DIVU") == 0
		|| strcmp(lookup->name, "DIVL") == 0) {
		  untranslated_top_ten++; // Ignore this
		}
		else
  		bug("%03d: %04x %10u %s\n", i, opcode_nums[i], count, lookup->name);
	}
#endif
}

void init_comp(void)
{
  int i;
  uae_s8* au = always_used;

#ifdef RECORD_REGISTER_USAGE
  for (i=0; i<16; i++)
  	reg_count_local[i] = 0;
#endif

  for (i=0; i<VREGS; i++) {
	  live.state[i].realreg = -1;
	  live.state[i].val = 0;
	  set_status(i, UNDEF);
  }

#ifdef USE_JIT_FPU
  for (i=0; i<VFREGS; i++) {
	  live.fate[i].status = UNDEF;
	  live.fate[i].realreg = -1;
	  live.fate[i].needflush = NF_SCRATCH;
  }
#endif

  for (i=0; i<VREGS; i++) {
  	if (i < 16) { /* First 16 registers map to 68k registers */
	    live.state[i].mem = &regs.regs[i];
	    set_status(i, INMEM);
  	}
  	else
	    live.state[i].mem = scratch.regs + i;
  }
  live.state[PC_P].mem = (uae_u32*)&(regs.pc_p);
  set_const(PC_P, (uintptr)comp_pc_p);

  live.state[FLAGX].mem = (uae_u32*)&(regs.ccrflags.x);
  set_status(FLAGX, INMEM);
  
#if defined(CPU_arm)
  live.state[FLAGTMP].mem = (uae_u32*)&(regs.ccrflags.nzcv);
#else
  live.state[FLAGTMP].mem = (uae_u32*)&(regs.ccrflags.cznv);
#endif
  set_status(FLAGTMP, INMEM);

  set_status(NEXT_HANDLER, UNDEF);

#ifdef USE_JIT_FPU
  for (i=0; i<VFREGS; i++) {
  	if (i < 8) { /* First 8 registers map to 68k FPU registers */
	    live.fate[i].mem = (uae_u32*)(&regs.fp[i].fp);
	    live.fate[i].needflush = NF_TOMEM;
	    live.fate[i].status = INMEM;
  	}
  	else if (i == FP_RESULT) {
	    live.fate[i].mem = (uae_u32*)(&regs.fp_result);
	    live.fate[i].needflush = NF_TOMEM;
	    live.fate[i].status = INMEM;
  	}
  	else
	    live.fate[i].mem = (uae_u32*)(&scratch.fregs[i]);
  }
#endif

  for (i=0; i<N_REGS; i++) {
	  live.nat[i].touched = 0;
	  live.nat[i].nholds = 0;
	  live.nat[i].locked = 0;
	  if (*au == i) {
	    live.nat[i].locked = 1; 
	    au++;
	  }
  }

#ifdef USE_JIT_FPU
  for (i=0; i<N_FREGS; i++) {
	  live.fat[i].touched = 0;
	  live.fat[i].nholds = 0;
	  live.fat[i].locked = 0;
  }
#endif

  touchcnt = 1;
  m68k_pc_offset = 0;
  live.flags_in_flags = TRASH;
  live.flags_on_stack = VALID;
  live.flags_are_important = 1;

  raw_fp_init();
}

/* Only do this if you really mean it! The next call should be to init!*/
void flush(int save_regs)
{
  int i;

  flush_flags(); /* low level */
  sync_m68k_pc(); /* mid level */

  if (save_regs) {
#ifdef USE_JIT_FPU
  	for (i=0; i<VFREGS; i++) {
	    if (live.fate[i].needflush == NF_SCRATCH ||
    		live.fate[i].status == CLEAN) {
      		f_disassociate(i);
	    }
  	}
#endif
  	for (i=0; i<=FLAGTMP; i++) {
  		switch(live.state[i].status) {
  		  case INMEM:
  	      if (live.state[i].val) {
  		      compemu_raw_add_l_mi((uintptr)live.state[i].mem, live.state[i].val);
  		      live.state[i].val = 0;
  		    }
  		    break;
  	    case CLEAN:
  	    case DIRTY:
  	      tomem(i); 
  	      break;
  	    case ISCONST:
  	      if (i != PC_P)
  		      writeback_const(i);
  	      break;
  	    default: 
  	      break;
	    }
	    Dif (live.state[i].val && i != PC_P) {
	      D(panicbug("JIT: Register %d still has val %x\n", i, live.state[i].val));
      }
	  }
#ifdef USE_JIT_FPU
	  for (i=0; i<VFREGS; i++) {
	    if (live.fate[i].needflush == NF_TOMEM && live.fate[i].status == DIRTY) {
	      f_evict(i);
	    }
	  }
	  raw_fp_cleanup_drop();
#endif
  }
}

void freescratch(void)
{
  int i;
  for (i=0; i<N_REGS; i++)
#if defined(CPU_arm)
  	if (live.nat[i].locked && i != 2 && i != 3 && i != 12) {
#else
  	if (live.nat[i].locked && i!=4) {
#endif
	    D(panicbug("JIT: Warning! %d is locked\n",i));
  	}

  for (i = S1; i < VREGS; i++)
    forget_about(i);

#ifdef USE_JIT_FPU
  for (i = 0; i < VFREGS; i++)
  	if (live.fate[i].needflush == NF_SCRATCH) {
	    f_forget_about(i);
  	}
#endif
}

/********************************************************************
 * Support functions, internal                                      *
 ********************************************************************/


#ifndef ALIGN_NOT_NEEDED
static void align_target(uae_u32 a)
{
	if (!a)
		return;

#if defined(CPU_arm)
	raw_emit_nop_filler(a - (((uintptr)target) & (a - 1)));
#else
  /* Fill with NOPs --- makes debugging with gdb easier */
  while ((uintptr)target&(a-1))
  	*target++ = 0x90; // Attention x86 specific code
#endif
}
#endif

STATIC_INLINE int isinrom(uintptr addr)
{
  return (addr >= (uae_u32)kickmem_bank.baseaddr &&
    addr < (uae_u32)kickmem_bank.baseaddr + 8 * 65536);
}

static void flush_all(void)
{
  int i;

  for (i = 0; i < VREGS; i++)
  	if (live.state[i].status == DIRTY) {
	    if (!call_saved[live.state[i].realreg]) {
    		tomem(i);
	    }
  	}
#ifdef USE_JIT_FPU
  for (i = 0; i < VFREGS; i++)
  	if (f_isinreg(i))
	    f_evict(i);
  raw_fp_cleanup_drop();
#endif
}

/* Make sure all registers that will get clobbered by a call are
   save and sound in memory */
static void prepare_for_call_1(void)
{
  flush_all();  /* If there are registers that don't get clobbered,
  		   * we should be a bit more selective here */
}

/* We will call a C routine in a moment. That will clobber all registers,
   so we need to disassociate everything */
static void prepare_for_call_2(void)
{
  int i;
  for (i = 0; i < N_REGS; i++)
  {
  	if (!call_saved[i] && live.nat[i].nholds > 0)
	    free_nreg(i);
  }

#ifdef USE_JIT_FPU
  for (i = 0; i < N_FREGS; i++)
  	if (live.fat[i].nholds > 0)
	    f_free_nreg(i);
#endif

  live.flags_in_flags = TRASH;  /* Note: We assume we already rescued the
			         flags at the very start of the call_r
			         functions! */
}

/********************************************************************
 * Memory access and related functions, CREATE time                 *
 ********************************************************************/

void register_branch(uae_u32 not_taken, uae_u32 taken, uae_u8 cond)
{
  next_pc_p = not_taken;
  taken_pc_p = taken;
  branch_cc = cond;
}

/* Note: get_handler may fail in 64 Bit environments, if direct_handler_to_use is
 * 		 outside 32 bit
 */
static uintptr get_handler(uintptr addr)
{
  blockinfo* bi = get_blockinfo_addr_new((void*)(uintptr) addr, 0);
  return (uintptr)bi->direct_handler_to_use;
}

/* This version assumes that it is writing *real* memory, and *will* fail
 *  if that assumption is wrong! No branches, no second chances, just
 *  straight go-for-it attitude */

static void writemem_real(int address, int source, int size)
{
  if(currprefs.address_space_24)
  {
  	switch(size) {
  	  case 1: jnf_MEM_WRITE24_OFF_b(address, source); break;
  	  case 2: jnf_MEM_WRITE24_OFF_w(address, source); break;
  	  case 4: jnf_MEM_WRITE24_OFF_l(address, source); break;
  	}
  }
  else
  {
  	switch(size) {
  	  case 1: jnf_MEM_WRITE_OFF_b(address, source); break;
  	  case 2: jnf_MEM_WRITE_OFF_w(address, source); break;
  	  case 4: jnf_MEM_WRITE_OFF_l(address, source); break;
  	}
  }
}

STATIC_INLINE void writemem(int address, int source, int offset, int size, int tmp)
{
  jnf_MEM_GETBANKFUNC(tmp, address, offset);
  /* Now tmp holds the address of the b/w/lput function */
  call_r_02(tmp, address, source, 4, size);
  forget_about(tmp);
}

void writebyte(int address, int source, int tmp)
{
  if (special_mem & S_WRITE)
  	writemem_special(address, source, 20, 1, tmp);
  else
  	writemem_real(address, source, 1);
}

void writeword(int address, int source, int tmp)
{
  if (special_mem & S_WRITE)
  	writemem_special(address, source, 16, 2, tmp);
  else
  	writemem_real(address, source, 2);
}

void writelong(int address, int source, int tmp)
{
  if (special_mem & S_WRITE)
  	writemem_special(address, source, 12, 4, tmp);
  else
  	writemem_real(address, source, 4);
}

// Now the same for clobber variant
STATIC_INLINE void writemem_real_clobber(int address, int source, int size)
{
  if(currprefs.address_space_24)
  {
  	switch(size) {
  	  case 1: jnf_MEM_WRITE24_OFF_b(address, source); break;
  	  case 2: jnf_MEM_WRITE24_OFF_w(address, source); break;
  	  case 4: jnf_MEM_WRITE24_OFF_l(address, source); break;
  	}
  }
  else
  {
  	switch(size) {
  	  case 1: jnf_MEM_WRITE_OFF_b(address, source); break;
  	  case 2: jnf_MEM_WRITE_OFF_w(address, source); break;
  	  case 4: jnf_MEM_WRITE_OFF_l(address, source); break;
  	}
  }
	forget_about(source);
}

void writelong_clobber(int address, int source, int tmp)
{
  if (special_mem & S_WRITE)
  	writemem_special(address, source, 12, 4, tmp);
  else
  	writemem_real_clobber(address, source, 4);
}


/* This version assumes that it is reading *real* memory, and *will* fail
 *  if that assumption is wrong! No branches, no second chances, just
 *  straight go-for-it attitude */

static void readmem_real(int address, int dest, int size)
{
  if(currprefs.address_space_24)
  {
  	switch(size) {
  	  case 1: jnf_MEM_READ24_OFF_b(dest, address); break;
  	  case 2: jnf_MEM_READ24_OFF_w(dest, address); break;
  	  case 4: jnf_MEM_READ24_OFF_l(dest, address); break;
  	}
  }
  else
  {
  	switch(size) {
  	  case 1: jnf_MEM_READ_OFF_b(dest, address); break;
  	  case 2: jnf_MEM_READ_OFF_w(dest, address); break;
  	  case 4: jnf_MEM_READ_OFF_l(dest, address); break;
  	}
  }
}

STATIC_INLINE void readmem(int address, int dest, int offset, int size, int tmp)
{
  jnf_MEM_GETBANKFUNC(tmp, address, offset);
  /* Now tmp holds the address of the b/w/lget function */
  call_r_11(dest, tmp, address, size, 4);
  forget_about(tmp);
}

void readbyte(int address, int dest, int tmp)
{
  if (special_mem & S_READ)
  	readmem_special(address, dest, 8, 1, tmp);
  else
  	readmem_real(address, dest, 1);
}

void readword(int address, int dest, int tmp)
{
  if (special_mem & S_READ)
  	readmem_special(address, dest, 4, 2, tmp);
  else
  	readmem_real(address, dest, 2);
}

void readlong(int address, int dest, int tmp)
{
  if (special_mem & S_READ)
  	readmem_special(address, dest, 0, 4, tmp);
  else
  	readmem_real(address, dest, 4);
}

/* This one might appear a bit odd... */
STATIC_INLINE void get_n_addr_old(int address, int dest, int tmp)
{
  readmem(address, dest, 24, 4, tmp);
}

STATIC_INLINE void get_n_addr_real(int address, int dest)
{
  if(currprefs.address_space_24)
  	jnf_MEM_GETADR24_OFF(dest, address);
	else
  	jnf_MEM_GETADR_OFF(dest, address);
}

void get_n_addr(int address, int dest, int tmp)
{
  if (special_mem)
  	get_n_addr_old(address,dest,tmp);
  else
  	get_n_addr_real(address,dest);
}

void get_n_addr_jmp(int address, int dest, int tmp)
{
  /* For this, we need to get the same address as the rest of UAE
	 would --- otherwise we end up translating everything twice */
  get_n_addr(address, dest, tmp);
}

/* base is a register, but dp is an actual value. 
   target is a register, as is tmp */
void calc_disp_ea_020(int base, uae_u32 dp, int target, int tmp)
{
  int reg = (dp >> 12) & 15;
  int regd_shift=(dp >> 9) & 3;

  if (dp & 0x100) {
	  int ignorebase = (dp&0x80);
	  int ignorereg = (dp&0x40);
	  int addbase = 0;
	  int outer = 0;

	  if ((dp & 0x30) == 0x20) addbase = (uae_s32)(uae_s16)comp_get_iword((m68k_pc_offset+=2)-2);
	  if ((dp & 0x30) == 0x30) addbase = comp_get_ilong((m68k_pc_offset+=4)-4);

	  if ((dp & 0x3) == 0x2) outer = (uae_s32)(uae_s16)comp_get_iword((m68k_pc_offset+=2)-2);
	  if ((dp & 0x3) == 0x3) outer = comp_get_ilong((m68k_pc_offset+=4)-4);

  	if ((dp & 0x4) == 0) {  /* add regd *before* the get_long */
	    if (!ignorereg) {
    		if ((dp & 0x800) == 0)
  		    sign_extend_16_rr(target, reg);
    		else
  		    mov_l_rr(target, reg);
    		shll_l_ri(target, regd_shift);
	    }
	    else
    		mov_l_ri(target, 0);

	    /* target is now regd */
	    if (!ignorebase)
    		add_l(target, base);
	    add_l_ri(target, addbase);
	    if (dp&0x03) readlong(target, target, tmp);
  	} else { /* do the getlong first, then add regd */
	    if (!ignorebase) {
		    mov_l_rr(target, base);
		    add_l_ri(target, addbase);
	    }
	    else
    		mov_l_ri(target, addbase);
	    if (dp&0x03) readlong(target, target, tmp);

	    if (!ignorereg) {
    		if ((dp & 0x800) == 0)
  		    sign_extend_16_rr(tmp, reg);
    		else
  		    mov_l_rr(tmp, reg);
    		shll_l_ri(tmp, regd_shift);
    		/* tmp is now regd */
    		add_l(target, tmp);
      }
	  }
	  add_l_ri(target, outer);
  }
  else { /* 68000 version */
	  if ((dp & 0x800) == 0) { /* Sign extend */
	    sign_extend_16_rr(target, reg);
	    lea_l_brr_indexed(target, base, target, 1 << regd_shift, (uae_s8)dp);
	  }
	  else {
	    lea_l_brr_indexed(target, base, reg, 1 << regd_shift, (uae_s8)dp);
	  }
  }
  forget_about(tmp);
}

void set_cache_state(int enabled)
{
  if (enabled != letit)
  	flush_icache_hard(0, 3);
  letit = enabled;
}

int get_cache_state(void)
{
  return letit;
}

uae_u32 get_jitted_size(void)
{
  if (compiled_code)
  	return current_compile_p - compiled_code;
  return 0;
}

void alloc_cache(void)
{
  if (compiled_code) {
	  flush_icache_hard(0, 3);
	  cache_free(compiled_code, currprefs.cachesize * 1024);
		compiled_code = 0;
  }
   
  if (currprefs.cachesize == 0)
  	return;

  while (!compiled_code && currprefs.cachesize) {
  	compiled_code = cache_alloc(currprefs.cachesize * 1024);
  	if (!compiled_code)
	    currprefs.cachesize /= 2;
  }
  if (compiled_code) {
#if defined(CPU_arm) && !defined(ARMV6T2)
  	max_compile_start = compiled_code + currprefs.cachesize*1024 - BYTES_PER_INST - DATA_BUFFER_SIZE;
#else
  	max_compile_start = compiled_code + currprefs.cachesize*1024 - BYTES_PER_INST;
#endif
  	current_compile_p = compiled_code;
  	current_cache_size = 0;
#if defined(CPU_arm) && !defined(ARMV6T2)
    reset_data_buffer();
#endif
  }
}

static void calc_checksum(blockinfo* bi, uae_u32* c1, uae_u32* c2)
{
  uae_u32 k1 = 0;
  uae_u32 k2 = 0;

#if USE_CHECKSUM_INFO
	checksum_info *csi = bi->csi;
	Dif(!csi) abort();
	while (csi) {
		uae_s32 len = csi->length;
		uintptr tmp = (uintptr)csi->start_p;
#else
    uae_s32 len = bi->len;
    uintptr tmp = (uintptr)bi->min_pcp;
#endif
    uae_u32* pos;

    len += (tmp&3);
		tmp &= ~((uintptr)3);
    pos = (uae_u32*)tmp;

		if (len >= 0 && len <= MAX_CHECKSUM_LEN) {
    	while (len>0) {
	      k1 += *pos;
	      k2 ^= *pos;
	      pos++;
	      len -= 4;
			}
  	}

#if USE_CHECKSUM_INFO
		csi = csi->next;
	}
#endif

	*c1 = k1;
	*c2 = k2;
}


int check_for_cache_miss(void)
{
  blockinfo* bi = get_blockinfo_addr(regs.pc_p);

  if (bi) {
  	int cl = cacheline(regs.pc_p);
  	if (bi != cache_tags[cl+1].bi) {
	    raise_in_cl_list(bi);
     return 1;
	  }
  }
  return 0;
}


static void recompile_block(void)
{
  /* An existing block's countdown code has expired. We need to make
     sure that execute_normal doesn't refuse to recompile due to a
     perceived cache miss... */
  blockinfo* bi = get_blockinfo_addr(regs.pc_p);

  Dif (!bi) 
  	jit_abort(_T("recompile_block"));
  raise_in_cl_list(bi);
  execute_normal();
  return;
}

static void cache_miss(void)
{
  blockinfo* bi = get_blockinfo_addr(regs.pc_p);
#if COMP_DEBUG
  uae_u32 cl = cacheline(regs.pc_p);
  blockinfo* bi2 = get_blockinfo(cl);
#endif

  if (!bi) {
	  execute_normal(); /* Compile this block now */
	  return;
  }
#if COMP_DEBUG
  Dif (!bi2 || bi == bi2) {
  	jit_abort (_T("Unexplained cache miss %p %p\n"), bi, bi2);
  }
#endif
  raise_in_cl_list(bi);
  return;
}

static int called_check_checksum(blockinfo* bi);

STATIC_INLINE int block_check_checksum(blockinfo* bi) 
{
  uae_u32 c1,c2;
  int isgood;

  if (bi->status != BI_NEED_CHECK)
  	return 1;  /* This block is in a checked state */

//  checksum_count++;

  if (bi->c1 || bi->c2)
  	calc_checksum(bi, &c1, &c2);
  else {
  	c1 = c2 = 1;  /* Make sure it doesn't match */
  }

	isgood = (c1 == bi->c1 && c2 == bi->c2);

  if (isgood) {
	  /* This block is still OK. So we reactivate. Of course, that
	     means we have to move it into the needs-to-be-flushed list */
	  bi->handler_to_use = bi->handler;
	  set_dhtu(bi, bi->direct_handler);

	  bi->status = BI_CHECKING;
	  isgood = called_check_checksum(bi) != 0;
  }
  if (isgood) {
  	D2(bug("JIT: reactivate %p/%p (%x %x/%x %x)", bi, bi->pc_p, c1, c2, bi->c1, bi->c2));
	  remove_from_list(bi);
	  add_to_active(bi);
	  raise_in_cl_list(bi);
	  bi->status = BI_ACTIVE;
  }
  else {
  	/* This block actually changed. We need to invalidate it,
  	   and set it up to be recompiled */
  	D2(bug("JIT: discard %p/%p (%x %x/%x %x)", bi, bi->pc_p, c1, c2, bi->c1, bi->c2));
  	invalidate_block(bi);
  	raise_in_cl_list(bi);
  }
  return isgood;
}

static int called_check_checksum(blockinfo* bi) 
{
  int isgood = 1;
  int i;
    
  for (i = 0; i < 2 && isgood; i++) {
  	if (bi->dep[i].jmp_off) {
	    isgood = block_check_checksum(bi->dep[i].target);
  	}
  }
  return isgood;
}

static void check_checksum(void)
{
  blockinfo* bi = get_blockinfo_addr(regs.pc_p);
  uae_u32 cl = cacheline(regs.pc_p);
  blockinfo* bi2 = get_blockinfo(cl);

  /* These are not the droids you are looking for...  */
  if (!bi) {
	  /* Whoever is the primary target is in a dormant state, but
	     calling it was accidental, and we should just compile this
	     new block */
	  execute_normal();
	  return;
  }
  if (bi != bi2) {
	  /* The block was hit accidentally, but it does exist. Cache miss */
	  cache_miss();
	  return;
  }

  if (!block_check_checksum(bi))
  	execute_normal();
}

STATIC_INLINE void match_states(blockinfo* bi)
{
  int i;
  smallstate* s = &(bi->env);
  
  if (bi->status == BI_NEED_CHECK) {
  	block_check_checksum(bi);
  }
  if (bi->status == BI_ACTIVE || 
  	bi->status == BI_FINALIZING) {  /* Deal with the *promises* the 
					 block makes (about not using 
					 certain vregs) */
  	  for (i = 0; i < 16; i++) {
	      if (s->virt[i] == L_UNNEEDED) {
		      D2(panicbug("unneeded reg %d at %p\n", i, target));
		      COMPCALL(forget_about)(i); // FIXME
  	    }
    	}
  }
  flush(1);

  /* And now deal with the *demands* the block makes */
  for (i = 0; i < N_REGS; i++) {
	  int v = s->nat[i];
	  if (v >= 0) {
	    // printf("Loading reg %d into %d at %p\n",v,i,target);
	    readreg_specific(v, 4, i);
	    // do_load_reg(i,v);
	    // setlock(i);
  	}
  }
  for (i = 0; i < N_REGS; i++) {
  	int v = s->nat[i];
  	if (v >= 0) {
	    unlock2(i);
  	}
  }
}

STATIC_INLINE void create_popalls(void)
{
  int i, r;

  if (popallspace == NULL)
  	popallspace = cache_alloc (POPALLSPACE_SIZE);

  int stack_space = STACK_OFFSET;
  for (i = 0; i< N_REGS; i++) {
	  if (need_to_preserve[i])
		  stack_space += sizeof(void *);
  }
  stack_space %= STACK_ALIGN;
  if (stack_space)
	  stack_space = STACK_ALIGN - stack_space;
  current_compile_p = popallspace;
  set_target(current_compile_p);

#if defined(CPU_arm) && !defined(ARMV6T2)
  reset_data_buffer();
  data_long(0, 0); // Make sure we emit the branch over the first buffer outside pushall_call_handler
#endif
  
  /* We need to guarantee 16-byte stack alignment on x86 at any point
     within the JIT generated code. We have multiple exit points
     possible but a single entry. A "jmp" is used so that we don't
     have to generate stack alignment in generated code that has to
     call external functions (e.g. a generic instruction handler).

     In summary, JIT generated code is not leaf so we have to deal
     with it here to maintain correct stack alignment. */
#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  current_compile_p = get_target();
  pushall_call_handler = get_target();
  raw_push_regs_to_preserve();
  raw_dec_sp(stack_space);
  compemu_raw_init_r_regstruct((uintptr)&regs);
  r = REG_PC_TMP;
  compemu_raw_mov_l_rm(r,(uintptr)&regs.pc_p);
  compemu_raw_and_TAGMASK(r);
  compemu_raw_jmp_m_indexed((uintptr)cache_tags, r, SIZEOF_VOID_P);

  /* now the exit points */
#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  popall_do_nothing = get_target();
  raw_inc_sp(stack_space);
  raw_pop_preserved_regs();
  compemu_raw_jmp((uintptr)do_nothing);

#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  popall_execute_normal = get_target();
  raw_inc_sp(stack_space);
  raw_pop_preserved_regs();
  compemu_raw_jmp((uintptr)execute_normal);

#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  popall_cache_miss = get_target();
  raw_inc_sp(stack_space);
  raw_pop_preserved_regs();
  compemu_raw_jmp((uintptr)cache_miss);

#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  popall_recompile_block = get_target();
  raw_inc_sp(stack_space);
  raw_pop_preserved_regs();
  compemu_raw_jmp((uintptr)recompile_block);

#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  popall_exec_nostats = get_target();
  raw_inc_sp(stack_space);
  raw_pop_preserved_regs();
  compemu_raw_jmp((uintptr)exec_nostats);

#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  popall_check_checksum = get_target();
  raw_inc_sp(stack_space);
  raw_pop_preserved_regs();
  compemu_raw_jmp((uintptr)check_checksum);

#if defined(CPU_arm) && !defined(ARMV6T2)
  reset_data_buffer();
#endif

  // No need to flush. Initialized and not modified
  // flush_cpu_icache((void *)popallspace, (void *)target);
}

STATIC_INLINE void reset_lists(void)
{
  int i;

  for (i = 0; i < MAX_HOLD_BI; i++)
  	hold_bi[i] = NULL;
  active = NULL;
  dormant = NULL;
}

static void prepare_block(blockinfo* bi)
{
  int i;

  set_target(current_compile_p);
#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  bi->direct_pen = (cpuop_func *)get_target();
  compemu_raw_mov_l_rm(0, (uintptr)&(bi->pc_p));
  compemu_raw_mov_l_mr((uintptr)&regs.pc_p, 0);
  compemu_raw_jmp((uintptr)popall_execute_normal);

#ifndef ALIGN_NOT_NEEDED
  align_target(align_jumps);
#endif
  bi->direct_pcc = (cpuop_func *)get_target();
  compemu_raw_mov_l_rm(0, (uintptr)&(bi->pc_p));
  compemu_raw_mov_l_mr((uintptr)&regs.pc_p, 0);
  compemu_raw_jmp((uintptr)popall_check_checksum);
  flush_cpu_icache((void *)current_compile_p, (void *)target);
  current_compile_p = get_target();

  bi->deplist = NULL;
  for (i = 0; i < 2; i++) {
	  bi->dep[i].prev_p = NULL;
	  bi->dep[i].next = NULL;
  }
  bi->env = default_ss;
  bi->status = BI_INVALID;
  bi->havestate = 0;
  //bi->env=empty_ss;
}

void compemu_reset(void)
{
	set_cache_state(0);
}

// OPCODE is in big endian format, use cft_map() beforehand, if needed.
STATIC_INLINE void reset_compop(int opcode)
{
	compfunctbl[opcode] = NULL;
	nfcompfunctbl[opcode] = NULL;
}

void build_comp(void)
{
  int i;
  unsigned long opcode;
  const struct comptbl* tbl = op_smalltbl_0_comp_ff;
  const struct comptbl* nftbl = op_smalltbl_0_comp_nf;
  int count;
#ifdef NOFLAGS_SUPPORT
  struct comptbl *nfctbl = (currprefs.cpu_level >= 5 ? op_smalltbl_0_nf
    : currprefs.cpu_level == 4 ? op_smalltbl_1_nf
    : (currprefs.cpu_level == 2 || currprefs.cpu_level == 3) ? op_smalltbl_2_nf
    : currprefs.cpu_level == 1 ? op_smalltbl_3_nf
    : ! currprefs.cpu_compatible ? op_smalltbl_4_nf
    : op_smalltbl_5_nf);
#endif

  for (opcode = 0; opcode < 65536; opcode++) {
		reset_compop(opcode);
#ifdef NOFLAGS_SUPPORT
		nfcpufunctbl[opcode] = _op_illg;
#endif
	  prop[opcode].use_flags = 0x1f;
	  prop[opcode].set_flags = 0x1f;
		prop[opcode].cflow = fl_jump | fl_trap; // ILLEGAL instructions do trap
  }

  for (i = 0; tbl[i].opcode < 65536; i++) {
		int cflow = table68k[tbl[i].opcode].cflow;
		if (follow_const_jumps && (tbl[i].specific & 16))
			cflow = fl_const_jump;
		else
			cflow &= ~fl_const_jump;
		prop[cft_map(tbl[i].opcode)].cflow = cflow;

		int uses_fpu = (tbl[i].specific & 32) != 0;
		if (uses_fpu && avoid_fpu)
			compfunctbl[cft_map(tbl[i].opcode)] = NULL;
		else
			compfunctbl[cft_map(tbl[i].opcode)] = tbl[i].handler;
  }

  for (i = 0; nftbl[i].opcode < 65536; i++) {
		int uses_fpu = tbl[i].specific & 32;
		if (uses_fpu && avoid_fpu)
			nfcompfunctbl[cft_map(nftbl[i].opcode)] = NULL;
		else
			nfcompfunctbl[cft_map(nftbl[i].opcode)] = nftbl[i].handler;
#ifdef NOFLAGS_SUPPORT
	  nfcpufunctbl[cft_map(nftbl[i].opcode)] = nfctbl[i].handler;
#endif
  }

#ifdef NOFLAGS_SUPPORT
  for (i = 0; nfctbl[i].handler; i++) {
  	nfcpufunctbl[cft_map(nfctbl[i].opcode)] = nfctbl[i].handler;
  }
#endif

  for (opcode = 0; opcode < 65536; opcode++) {
	  compop_func *f;
	  compop_func *nff;
#ifdef NOFLAGS_SUPPORT
  	cpuop_func *nfcf;
#endif
	  int isaddx, cflow;

  	int cpu_level = (currprefs.cpu_model - 68000) / 10;
  	if (cpu_level > 4)
	    cpu_level--;
  	if (table68k[opcode].mnemo == i_ILLG || table68k[opcode].clev > cpu_level)
	    continue;

  	if (table68k[opcode].handler != -1) {
	    f = compfunctbl[cft_map(table68k[opcode].handler)];
	    nff = nfcompfunctbl[cft_map(table68k[opcode].handler)];
#ifdef NOFLAGS_SUPPORT
	    nfcf = nfcpufunctbl[cft_map(table68k[opcode].handler)];
#endif
	    cflow = prop[cft_map(table68k[opcode].handler)].cflow;
	    isaddx = prop[cft_map(table68k[opcode].handler)].is_addx;
	    prop[cft_map(opcode)].cflow = cflow;
	    prop[cft_map(opcode)].is_addx = isaddx;
	    compfunctbl[cft_map(opcode)] = f;
	    nfcompfunctbl[cft_map(opcode)] = nff;
#ifdef NOFLAGS_SUPPORT
	    nfcpufunctbl[cft_map(opcode)] = nfcf;
#endif
  	}
	  prop[cft_map(opcode)].set_flags = table68k[opcode].flagdead;
	  prop[cft_map(opcode)].use_flags = table68k[opcode].flaglive;
	  /* Unconditional jumps don't evaluate condition codes, so they
	   * don't actually use any flags themselves */
	  if (prop[cft_map(opcode)].cflow & fl_const_jump)
	    prop[cft_map(opcode)].use_flags = 0;
  }
#ifdef NOFLAGS_SUPPORT
  for (i = 0; nfctbl[i].handler != NULL; i++) {
  	if (nfctbl[i].specific)
	    nfcpufunctbl[cft_map(tbl[i].opcode)] = nfctbl[i].handler;
  }
#endif

  count = 0;
  for (opcode = 0; opcode < 65536; opcode++) {
  	if (compfunctbl[cft_map(opcode)])
	    count++;
  }
//	D(bug("<JIT compiler> : supposedly %d compileable opcodes!",count));

  /* Initialise state */
  create_popalls();
  alloc_cache();
  reset_lists();

  for (i = 0; i < TAGSIZE; i += 2) {
	  cache_tags[i].handler = (cpuop_func *)popall_execute_normal;
	  cache_tags[i+1].bi = NULL;
  }
  compemu_reset();

  for (i = 0; i < VREGS; i++) {
  	empty_ss.virt[i] = L_NEEDED;
  }
  for (i = 0; i < N_REGS; i++) {
  	empty_ss.nat[i] = L_UNKNOWN;
  }
  default_ss = empty_ss;
}

static void flush_icache_none(uaecptr ptr, int n)
{
	/* Nothing to do.  */
}

static void flush_icache_hard(uaecptr ptr, int n)
{
  blockinfo* bi, *dbi;

//    hard_flush_count++;
  D(bug("JIT: Flush Icache_hard(%d/%x/%p), %u KB\n",
	   n, regs.pc, regs.pc_p, current_cache_size / 1024));
	UNUSED(n);
  bi = active;
  while(bi) {
	  cache_tags[cacheline(bi->pc_p)].handler = (cpuop_func *)popall_execute_normal;
	  cache_tags[cacheline(bi->pc_p)+1].bi = NULL;
	  dbi = bi; 
	  bi = bi->next;
	  free_blockinfo(dbi);
  }
  bi = dormant;
  while(bi) {
	  cache_tags[cacheline(bi->pc_p)].handler = (cpuop_func *)popall_execute_normal;
	  cache_tags[cacheline(bi->pc_p)+1].bi = NULL;
	  dbi = bi; 
	  bi = bi->next;
	  free_blockinfo(dbi);
  }

  reset_lists();
  if (!compiled_code)
  	return;

#if defined(CPU_arm) && !defined(ARMV6T2)
  reset_data_buffer();
#endif

  current_compile_p = compiled_code;
  set_special(0); /* To get out of compiled code */
}


/* "Soft flushing" --- instead of actually throwing everything away,
   we simply mark everything as "needs to be checked".
*/

STATIC_INLINE void flush_icache_lazy(uaecptr ptr, int n)
{
  blockinfo* bi;
  blockinfo* bi2;

//  soft_flush_count++;
  if (!active)
	  return;

  bi = active;
  while (bi) {
  	uae_u32 cl = cacheline(bi->pc_p);
		if (bi->status == BI_INVALID ||
			bi->status == BI_NEED_RECOMP) { 
  	    if (bi == cache_tags[cl+1].bi)
      		cache_tags[cl].handler = (cpuop_func *)popall_execute_normal;
  	    bi->handler_to_use = (cpuop_func *)popall_execute_normal;
  	    set_dhtu(bi,bi->direct_pen);
  	    bi->status = BI_INVALID;
		}
		else {
	    if (bi == cache_tags[cl+1].bi)
		    cache_tags[cl].handler = (cpuop_func *)popall_check_checksum;
	    bi->handler_to_use = (cpuop_func *)popall_check_checksum;
	    set_dhtu(bi,bi->direct_pcc);
		  bi->status = BI_NEED_CHECK;
	  }
	  bi2 = bi;
	  bi = bi->next;
  }
  /* bi2 is now the last entry in the active list */
  bi2->next = dormant;
  if (dormant)
  	dormant->prev_p = &(bi2->next);

  dormant = active;
  active->prev_p = &dormant;
  active = NULL;
}

int failure;

STATIC_INLINE unsigned int get_opcode_cft_map(unsigned int f)
{
	return ((f >> 8) & 255) | ((f & 255) << 8);
}
#define DO_GET_OPCODE(a) (get_opcode_cft_map((uae_u16)*(a)))

#ifdef JIT_DEBUG
static uae_u8 *last_regs_pc_p = 0;
static uae_u8 *last_compiled_block_addr = 0;

void compiler_dumpstate(void)
{
	if (!JITDebug)
		return;
	
	bug("### Host addresses");
	bug("MEM_BASE    : %x", NATMEM_OFFSET);
	bug("PC_P        : %p", &regs.pc_p);
	bug("SPCFLAGS    : %p", &regs.spcflags);
	bug("D0-D7       : %p-%p", &regs.regs[0], &regs.regs[7]);
	bug("A0-A7       : %p-%p", &regs.regs[8], &regs.regs[15]);
	bug("");
	
	bug("### M68k processor state");
	m68k_dumpstate(stderr, 0);
	bug("");
	
	bug("### Block in Atari address space");
	bug("M68K block   : %p",
			  (void *)(uintptr)last_regs_pc_p);
	if (last_regs_pc_p != 0) {
		bug("Native block : %p (%d bytes)",
			  (void *)last_compiled_block_addr,
			  get_blockinfo_addr(last_regs_pc_p)->direct_handler_size);
	}
	bug("");
}
#endif

void compile_block(cpu_history* pc_hist, int blocklen, int totcycles)
{
  if (letit && compiled_code && currprefs.cpu_model >= 68020) {
#ifdef PROFILE_COMPILE_TIME
	  compile_count++;
	  clock_t start_time = clock();
#endif
#ifdef JIT_DEBUG
  	bool disasm_block = false;
#endif

	  /* OK, here we need to 'compile' a block */
	  int i;
	  int r;
	  int was_comp = 0;
	  uae_u8 liveflags[MAXRUN+1];
#if USE_CHECKSUM_INFO
	  bool trace_in_rom = isinrom((uintptr)pc_hist[0].location);
	  uintptr max_pcp = (uintptr)pc_hist[blocklen - 1].location;
	  uintptr min_pcp = max_pcp;
#else
	  uintptr max_pcp = (uintptr)pc_hist[0].location;
	  uintptr min_pcp = max_pcp;
#endif
	  uae_u32 cl = cacheline(pc_hist[0].location);
	  void* specflags = (void*)&regs.spcflags;
	  blockinfo* bi = NULL;
	  blockinfo* bi2;
	  int extra_len = 0;

	  redo_current_block = 0;
	  if (current_compile_p >= MAX_COMPILE_PTR)
	    flush_icache_hard(0, 3);

	  alloc_blockinfos();

	  bi = get_blockinfo_addr_new(pc_hist[0].location, 0);
	  bi2 = get_blockinfo(cl);

	  optlev = bi->optlevel;
	  if (bi->status != BI_INVALID) {
	    Dif (bi != bi2) { 
		    /* I don't think it can happen anymore. Shouldn't, in 
		       any case. So let's make sure... */
    		jit_abort (_T("JIT: WOOOWOO count=%d, ol=%d %p %p\n"),
		       bi->count, bi->optlevel, bi->handler_to_use,
		       cache_tags[cl].handler);
	    }

	    Dif (bi->count != -1 && bi->status != BI_NEED_RECOMP) {
		    panicbug("bi->count=%d, bi->status=%d,bi->optlevel=%d\n", bi->count, bi->status, bi->optlevel);
		    /* What the heck? We are not supposed to be here! */
		    jit_abort(_T("BI_TARGETTED"));
	    }
  	}	
	  if (bi->count == -1) {
	    optlev++;
	    while (!currprefs.optcount[optlev])
	    	optlev++;
	    bi->count = currprefs.optcount[optlev] - 1;
	  }
	  current_block_pc_p = (uintptr)pc_hist[0].location;

	  remove_deps(bi); /* We are about to create new code */
	  bi->optlevel = optlev;
	  bi->pc_p = (uae_u8*)pc_hist[0].location;
#if USE_CHECKSUM_INFO
	  free_checksum_info_chain(bi->csi);
	  bi->csi = NULL;
#endif

	  liveflags[blocklen] = 0x1f; /* All flags needed afterwards */
	  i = blocklen;
	  while (i--) {
	    uae_u16* currpcp = pc_hist[i].location;
			uae_u32 op = DO_GET_OPCODE(currpcp);

#if USE_CHECKSUM_INFO
  		trace_in_rom = trace_in_rom && isinrom((uintptr)currpcp);
  		if (follow_const_jumps && is_const_jump(op)) {
  			checksum_info *csi = alloc_checksum_info();
  			csi->start_p = (uae_u8 *)min_pcp;
  			csi->length = max_pcp - min_pcp + LONGEST_68K_INST;
  			csi->next = bi->csi;
  			bi->csi = csi;
  			max_pcp = (uintptr)currpcp;
  		}
  		min_pcp = (uintptr)currpcp;
#else
      if ((uintptr)currpcp < min_pcp)
  		  min_pcp=(uintptr)currpcp;
  	  if ((uintptr)currpcp > max_pcp)
  		  max_pcp=(uintptr)currpcp;
#endif

		  liveflags[i] = ((liveflags[i+1] &
		    (~prop[op].set_flags)) |
		    prop[op].use_flags);
		  if (prop[op].is_addx && (liveflags[i+1] & FLAG_Z) == 0)
		    liveflags[i] &= ~FLAG_Z;
	  }

#if USE_CHECKSUM_INFO
	  checksum_info *csi = alloc_checksum_info();
	  csi->start_p = (uae_u8 *)min_pcp;
	  csi->length = max_pcp - min_pcp + LONGEST_68K_INST;
	  csi->next = bi->csi;
	  bi->csi = csi;
#endif

  	bi->needed_flags = liveflags[0];

  	/* This is the non-direct handler */
#ifndef ALIGN_NOT_NEEDED
  	align_target(align_loops);
#endif
  	was_comp = 0;

	  bi->direct_handler = (cpuop_func *)get_target();
	  set_dhtu(bi,bi->direct_handler);
	  bi->status = BI_COMPILING;
	  current_block_start_target = (uintptr)get_target();

  	if (bi->count >= 0) { /* Need to generate countdown code */
	    compemu_raw_mov_l_mi((uintptr)&regs.pc_p, (uintptr)pc_hist[0].location);
	    compemu_raw_sub_l_mi((uintptr)&(bi->count), 1);
	    compemu_raw_jl((uintptr)popall_recompile_block);
  	}
  	if (optlev == 0) { /* No need to actually translate */
	    /* Execute normally without keeping stats */
	    compemu_raw_mov_l_mi((uintptr)&regs.pc_p, (uintptr)pc_hist[0].location);
	    compemu_raw_jmp((uintptr)popall_exec_nostats);
  	}
  	else {
	    next_pc_p = 0;
	    taken_pc_p = 0;
	    branch_cc = 0; // Only to be initialized. Will be set together with next_pc_p

	    comp_pc_p = (uae_u8*)pc_hist[0].location;
	    init_comp();
	    was_comp = 1;

#ifdef JIT_DEBUG
  		if (JITDebug) {
  			compemu_raw_mov_l_mi((uintptr)&last_regs_pc_p, (uintptr)pc_hist[0].location);
  			compemu_raw_mov_l_mi((uintptr)&last_compiled_block_addr, current_block_start_target);
  		}
#endif

	    for (i = 0; i < blocklen && get_target_noopt() < MAX_COMPILE_PTR; i++) {
  		  cpuop_func **cputbl;
  		  compop_func **comptbl;
  		  uae_u32 opcode = DO_GET_OPCODE(pc_hist[i].location);
  		  needed_flags = (liveflags[i+1] & prop[opcode].set_flags);
  		  special_mem = pc_hist[i].specmem;
        D(bug("  0x%08x: %04x (special_mem=%d, needed_flags=%d)\n", pc_hist[i].location, opcode, special_mem, needed_flags));
    		if (!needed_flags) {
#ifdef NOFLAGS_SUPPORT
		      cputbl=nfcpufunctbl;
#else
		      cputbl=cpufunctbl;
#endif
		      comptbl=nfcompfunctbl;
		    }
		    else {
		      cputbl = cpufunctbl;
		      comptbl = compfunctbl;
		    }

  		  failure = 1; // gb-- defaults to failure state
  		  if (comptbl[opcode] && optlev > 1) {
  		    failure = 0;
  		    if (!was_comp) {
  			    comp_pc_p = (uae_u8*)pc_hist[i].location;
  			    init_comp();
  		    }
  		    was_comp = 1;
  
  		    comptbl[opcode](opcode);
  		    freescratch();
  		    if (!(liveflags[i+1] & FLAG_CZNV)) {
  			    /* We can forget about flags */
  			    dont_care_flags();
  		    }
#if INDIVIDUAL_INST
  		    flush(1);
  		    nop();
  		    flush(1);
  		    was_comp = 0;
#endif
    		}
  		  if (failure) {
  		    if (was_comp) {
  			    flush(1);
  			    was_comp = 0;
  		    }
  		    compemu_raw_mov_l_ri(REG_PAR1, (uae_u32)opcode);
  		    compemu_raw_mov_l_ri(REG_PAR2, (uae_u32)&regs);
  		    compemu_raw_mov_l_mi((uintptr)&regs.pc_p, (uintptr)pc_hist[i].location);
  		    compemu_raw_call((uintptr)cputbl[opcode]);
#ifdef PROFILE_UNTRANSLATED_INSNS
  			  // raw_cputbl_count[] is indexed with plain opcode (in m68k order)
  		    compemu_raw_add_l_mi((uintptr)&raw_cputbl_count[cft_map(opcode)], 1);
#endif

  		    if (i < blocklen - 1) {
      			uae_s8* branchadd;
  
  			    compemu_raw_mov_l_rm(0, (uintptr)specflags);
  			    compemu_raw_test_l_rr(0, 0);
#if defined(CPU_arm) && !defined(ARMV6T2)
            data_check_end(8, 56);
#endif
  			    compemu_raw_jz_b_oponly();
  			    branchadd = (uae_s8 *)get_target();
  			    compemu_raw_sub_l_mi((uintptr)&countdown, scaled_cycles(totcycles));
  			    compemu_raw_jmp((uintptr)popall_do_nothing);
  			    *(branchadd - 4) = (((uintptr)get_target() - (uintptr)branchadd) - 4) >> 2;
            D(bug("  branchadd(byte) to 0x%08x: 0x%02x\n", branchadd, *branchadd));
  		    }
    		}
      }

      if (next_pc_p) { /* A branch was registered */
  		  uintptr t1 = next_pc_p;
  		  uintptr t2 = taken_pc_p;
  		  int cc = branch_cc;
  
  		  uae_u32* branchadd;
  		  uae_u32* tba;
  		  bigstate tmp;
  		  blockinfo* tbi;
  
    		if (taken_pc_p < next_pc_p) {
  	      /* backward branch. Optimize for the "taken" case ---
  		       which means the raw_jcc should fall through when
  		       the 68k branch is taken. */
  		    t1 = taken_pc_p;
  		    t2 = next_pc_p;
  		    cc = branch_cc^1;
    		}
  
    		tmp = live; /* ouch! This is big... */
    		compemu_raw_jcc_l_oponly(cc);
  		  branchadd = (uae_u32*)get_target();
  		  emit_long(0);
		
  		  /* predicted outcome */
  		  tbi = get_blockinfo_addr_new((void*)t1, 1);
  		  match_states(tbi);
		  
#if defined(CPU_arm) && !defined(ARMV6T2)
        data_check_end(4, 56);
#endif
  		  compemu_raw_endblock_pc_isconst(scaled_cycles(totcycles), t1);
  		  tba = (uae_u32*)get_target();
  		  emit_jmp_target(get_handler(t1));
  		  create_jmpdep(bi, 0, tba, t1);

#ifndef ALIGN_NOT_NEEDED
  		  align_target(align_jumps);
#endif
  		  /* not-predicted outcome */
        write_jmp_target(branchadd, (cpuop_func*)get_target());
        D(bug("  write_jmp_target to 0x%08x: 0x%08x\n", branchadd, get_target()));
  		  live = tmp; /* Ouch again */
  		  tbi = get_blockinfo_addr_new((void*)t2, 1);
  		  match_states(tbi);

  		  //flush(1); /* Can only get here if was_comp==1 */
#if defined(CPU_arm) && !defined(ARMV6T2)
        data_check_end(4, 56);
#endif
  	    compemu_raw_endblock_pc_isconst(scaled_cycles(totcycles), t2);
  		  tba = (uae_u32*)get_target();
  		  emit_jmp_target(get_handler(t2));
  		  create_jmpdep(bi, 1, tba, t2);
      }
      else
      {
    		if (was_comp) {
  		    flush(1);
    		}

  		  /* Let's find out where next_handler is... */
  		  if (was_comp && isinreg(PC_P)) {
#if defined(CPU_arm) && !defined(ARMV6T2)
          data_check_end(4, 52);
#endif
		      r = live.state[PC_P].realreg;
		      compemu_raw_endblock_pc_inreg(r, scaled_cycles(totcycles));
  		  }
    		else if (was_comp && isconst(PC_P)) {
  		    uintptr v = live.state[PC_P].val;
  		    uae_u32* tba;
  		    blockinfo* tbi;
  
  		    tbi = get_blockinfo_addr_new((void*)v, 1);
  		    match_states(tbi);

#if defined(CPU_arm) && !defined(ARMV6T2)
          data_check_end(4, 56);
#endif
  		    compemu_raw_endblock_pc_isconst(scaled_cycles(totcycles), v);
  		    tba = (uae_u32*)get_target();
  		    emit_jmp_target(get_handler(v));
  		    create_jmpdep(bi, 0, tba, v);
    		}
    		else {
  		    r = REG_PC_TMP;
  		    compemu_raw_mov_l_rm(r, (uintptr)&regs.pc_p);
#if defined(CPU_arm) && !defined(ARMV6T2)
          data_check_end(4, 52);
#endif
	  	    compemu_raw_endblock_pc_inreg(r, scaled_cycles(totcycles));
		    }
	    }
	  }

#if USE_CHECKSUM_INFO
  	remove_from_list(bi);
  	if (trace_in_rom) {
  		// No need to checksum that block trace on cache invalidation
  		free_checksum_info_chain(bi->csi);
  		bi->csi = NULL;
  		add_to_dormant(bi);
  	}
  	else {
  		calc_checksum(bi, &(bi->c1), &(bi->c2));
  		add_to_active(bi);
  	}
#else
  	if (next_pc_p + extra_len >= max_pcp && next_pc_p + extra_len < max_pcp + LONGEST_68K_INST)
      max_pcp = next_pc_p + extra_len;  /* extra_len covers flags magic */
  	else
  	  max_pcp += LONGEST_68K_INST;
  
  	bi->len = max_pcp - min_pcp;
  	bi->min_pcp = min_pcp;
  
  	remove_from_list(bi);
  	if (isinrom(min_pcp) && isinrom(max_pcp)) {
      add_to_dormant(bi); /* No need to checksum it on cache flush.
                  				   Please don't start changing ROMs in
                  				   flight! */
  	}
  	else {
      calc_checksum(bi, &(bi->c1), &(bi->c2));
      add_to_active(bi);
  	}
#endif

	  current_cache_size += get_target() - (uae_u8 *)current_compile_p;

#ifdef JIT_DEBUG
	  if (JITDebug)
		  bi->direct_handler_size = get_target() - (uae_u8 *)current_block_start_target;
#endif

#ifndef ALIGN_NOT_NEEDED
	  align_target(align_jumps);
#endif
  	/* This is the non-direct handler */
  	bi->handler = 
      bi->handler_to_use = (cpuop_func *)get_target();
  	compemu_raw_cmp_l_mi((uintptr)&regs.pc_p, (uintptr)pc_hist[0].location);
  	compemu_raw_jnz((uintptr)popall_cache_miss);
  	comp_pc_p = (uae_u8*)pc_hist[0].location;

  	bi->status=BI_FINALIZING;
  	init_comp();
  	match_states(bi);
  	flush(1);
  
  	compemu_raw_jmp((uintptr)bi->direct_handler);

  	flush_cpu_icache((void *)current_block_start_target, (void *)target);
  	current_compile_p = get_target();
  	raise_in_cl_list(bi);

  	/* We will flush soon, anyway, so let's do it now */
  	if (current_compile_p >= MAX_COMPILE_PTR)
  	  flush_icache_hard(0, 3);

  	bi->status = BI_ACTIVE;
  	if (redo_current_block)
      block_need_recompile(bi);
	
#ifdef PROFILE_COMPILE_TIME
	  compile_time += (clock() - start_time);
#endif
  }
}


void dump_compiler(uae_u32* sp)
{
  int i, j;
  
  for(i=-16; i<16; i+=4)
  {
    printf("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", sp + i, sp[i], sp[i+1], sp[i+2], sp[i+3]);
  }

  printf("compile cache: 0x%08x - 0x%08x\n", compiled_code, compiled_code + currprefs.cachesize * 1024);
  printf("start_pc_p=0x%08x, start_pc=0x%08x, current_block_pc_p=0x%08x\n", start_pc_p, start_pc, current_block_pc_p);
  printf("current_block_start_target=0x%08x, needed_flags=%d\n", current_block_start_target, needed_flags);
  printf("current_compile_p=0x%08x, max_compile_start=0x%08x\n", current_compile_p, max_compile_start);

/*
  printf("PC history:\n");
  for(i=trace_pc_idx - 16, j=0; i < trace_pc_idx; ++i, ++j)
  {
    printf("0x%08x (%d) \t", i >= 0 ? trace_pc[i] : trace_pc[i + TRACE_PC_HISTORY],
      i >= 0 ? trace_pc_i[i] : trace_pc_i[i + TRACE_PC_HISTORY]);
    if((j & 3) == 3)
      printf("\n");
  }
*/
}


#endif
