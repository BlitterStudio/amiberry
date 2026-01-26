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
 * JIT compiler m68k -> ARM
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
#include "sysdeps.h"

#include <math.h>

#include "sysconfig.h"
#include "sysdeps.h"

#if defined(JIT)

#include "options.h"
#include "events.h"
#include "include/memory.h"
#include "newcpu.h"
#include "comptbl_arm.h"
#include "compemu_arm.h"
#include <SDL.h>

#if defined(__pie__) || defined (__PIE__)
#error Position-independent code (PIE) cannot be used with JIT
#endif

#ifdef __MACH__
// Needed for sys_cache_invalidate to on the JIT space region, Mac OS X specific
#include <libkern/OSCacheControl.h>
#endif

#include "uae/vm.h"
#define VM_PAGE_READ UAE_VM_READ
#define VM_PAGE_WRITE UAE_VM_WRITE
#define VM_PAGE_EXECUTE UAE_VM_EXECUTE
#define VM_MAP_FAILED UAE_VM_ALLOC_FAILED
#define VM_MAP_DEFAULT 1
#define VM_MAP_32BIT 1
#define vm_protect(address, size, protect) uae_vm_protect(address, size, protect)
#define vm_release(address, size) uae_vm_free(address, size)

static inline void *vm_acquire(uae_u32 size, int options = VM_MAP_DEFAULT)
{
	assert(options == (VM_MAP_DEFAULT | VM_MAP_32BIT));
	return uae_vm_alloc(size, UAE_VM_32BIT, UAE_VM_READ_WRITE);
}

#define UNUSED(x)
#if defined(CPU_AARCH64)
#define PRINT_PTR "%016llx"
#else
#define PRINT_PTR "%08x"
#endif

#define jit_log(format, ...) \
  write_log("JIT: " format "\n", ##__VA_ARGS__);
#define jit_log2(format, ...)

#define MEMBaseDiff uae_p32(NATMEM_OFFSET)

#ifdef NATMEM_OFFSET
#define FIXED_ADDRESSING 1
#endif

// %%% BRIAN KING WAS HERE %%%
extern bool canbang;

#include "../compemu_prefs.cpp"

#define uint32 uae_u32
#define uint8 uae_u8

static inline int distrust_check(int value)
{
#ifdef JIT_ALWAYS_DISTRUST
    return 1;
#else
    int distrust = value;
#ifdef FSUAE
    switch (value) {
    case 0: distrust = 0; break;
    case 1: distrust = 1; break;
    case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
    case 3: distrust = !have_done_picasso; break;
    default: abort();
    }
#endif
    return distrust;
#endif
}

static inline int distrust_byte(void)
{
    return distrust_check(currprefs.comptrustbyte);
}

static inline int distrust_word(void)
{
    return distrust_check(currprefs.comptrustword);
}

static inline int distrust_long(void)
{
    return distrust_check(currprefs.comptrustlong);
}

static inline int distrust_addr(void)
{
    return distrust_check(currprefs.comptrustnaddr);
}

//#if DEBUG
//#define PROFILE_COMPILE_TIME        1
//#endif
//#define PROFILE_UNTRANSLATED_INSNS    1

#ifdef JIT_DEBUG
#undef abort
#define abort() do { \
  fprintf(stderr, "Abort in file %s at line %d\n", __FILE__, __LINE__); \
  SDL_Quit();  \
  exit(EXIT_FAILURE); \
} while (0)
#endif

#ifdef RECORD_REGISTER_USAGE
static uint64 reg_count[16];
static uint64 reg_count_local[16];

static int reg_count_compare(const void* ap, const void* bp)
{
    const int a = *((int*)ap);
    const int b = *((int*)bp);
    return reg_count[b] - reg_count[a];
}
#endif

#ifdef PROFILE_COMPILE_TIME
#include <time.h>
static uae_u32 compile_count    = 0;
static clock_t compile_time     = 0;
static clock_t emul_start_time  = 0;
static clock_t emul_end_time    = 0;
#endif

#ifdef PROFILE_UNTRANSLATED_INSNS
static const int untranslated_top_ten = 50;
static uae_u32 raw_cputbl_count[65536] = { 0, };
static uae_u16 opcode_nums[65536];


static int __cdecl untranslated_compfn(const void* e1, const void* e2)
{
    int v1 = *(const uae_u16*)e1;
    int v2 = *(const uae_u16*)e2;
    return (int)raw_cputbl_count[v2] - (int)raw_cputbl_count[v1];
}
#endif

static compop_func *compfunctbl[65536];
static compop_func *nfcompfunctbl[65536];
#ifdef NOFLAGS_SUPPORT_GENCOMP
static cpuop_func* nfcpufunctbl[65536];
#endif
uae_u8* comp_pc_p;

// gb-- Extra data for Basilisk II/JIT
#define follow_const_jumps (currprefs.comp_constjump != 0)

static uae_u32 cache_size = 0;            // Size of total cache allocated for compiled blocks
static uae_u32 current_cache_size   = 0;  // Cache grows upwards: how much has been consumed already
#ifdef USE_JIT_FPU
#define avoid_fpu (!currprefs.compfpu)
#define lazy_flush (!currprefs.comp_hardflush)
#else
#define avoid_fpu (true)
#define lazy_flush (true)
#endif
static bool		have_cmov = false;	// target has CMOV instructions ?
static bool		have_rat_stall = true;	// target has partial register stalls ?
const bool		tune_alignment = true;	// Tune code alignments for running CPU ?
const bool		tune_nop_fillers = true;	// Tune no-op fillers for architecture
static bool		setzflg_uses_bsf = false;	// setzflg virtual instruction can use native BSF instruction correctly?
static int		align_loops = 32;	// Align the start of loops
static int		align_jumps = 32;	// Align the start of jumps
static int		optcount[10] = {
#ifdef UAE
    4,		// How often a block has to be executed before it is translated
#else
    10,		// How often a block has to be executed before it is translated
#endif
    0,		// How often to use naive translation
    0, 0, 0, 0,
    -1, -1, -1, -1
};

op_properties prop[65536];

#ifdef AMIBERRY
bool may_raise_exception = false;
static bool flags_carry_inverted = false;
static bool disasm_this = false;
#endif

static inline bool is_const_jump(uae_u32 opcode)
{
    return (prop[opcode].cflow == fl_const_jump);
}

static inline unsigned int cft_map(unsigned int f)
{
#ifdef UAE
#if !defined(HAVE_GET_WORD_UNSWAPPED)
    return f;
#else
    return do_byteswap_16(f);
#endif
#else
#if !defined(HAVE_GET_WORD_UNSWAPPED) || defined(FULLMMU)
    return f;
#else
    return ((f >> 8) & 255) | ((f & 255) << 8);
#endif
#endif
}

uae_u8* start_pc_p;
uae_u32 start_pc;
uae_u32 current_block_pc_p;
static uintptr current_block_start_target;
uae_u32 needed_flags;
static uintptr next_pc_p;
static uintptr taken_pc_p;
static int     branch_cc;
static int redo_current_block;

#ifdef UAE
int segvcount = 0;
#endif
uae_u8* current_compile_p = NULL;
static uae_u8* max_compile_start;
uae_u8* compiled_code = NULL;
static uae_s32 reg_alloc_run;
const int POPALLSPACE_SIZE = 2048; /* That should be enough space */
uae_u8* popallspace = NULL;

void* pushall_call_handler = NULL;
static void* popall_do_nothing = NULL;
static void* popall_exec_nostats = NULL;
static void* popall_execute_normal = NULL;
static void* popall_cache_miss = NULL;
static void* popall_recompile_block = NULL;
static void* popall_check_checksum = NULL;

#ifdef AMIBERRY
static void* popall_exec_nostats_setpc = NULL;
static void* popall_execute_normal_setpc = NULL;
static void* popall_check_checksum_setpc = NULL;
static void* popall_execute_exception = NULL;
#endif

/* The 68k only ever executes from even addresses. So right now, we
 * waste half the entries in this array
 * UPDATE: We now use those entries to store the start of the linked
 * lists that we maintain for each hash result.
 */
static cacheline cache_tags[TAGSIZE];
static int cache_enabled = 0;
static blockinfo* hold_bi[MAX_HOLD_BI];
blockinfo* active;
blockinfo* dormant;

#ifdef NOFLAGS_SUPPORT_GENCOMP
/* 68040 */
extern const struct cputbl op_smalltbl_0[];
#endif
extern const struct comptbl op_smalltbl_0_comp_nf[];
extern const struct comptbl op_smalltbl_0_comp_ff[];

static void flush_icache_hard(int);
static void flush_icache_lazy(int);
static void flush_icache_none(int);

static bigstate live;
static smallstate empty_ss;
static smallstate default_ss;
static int optlev;

static int writereg(int r);
static void unlock2(int r);
static void setlock(int r);
static int readreg_specific(int r, int size, int spec);
static int writereg_specific(int r, int size, int spec);

#ifdef AMIBERRY
static int readreg(int r);
static void prepare_for_call_1(void);
static void prepare_for_call_2(void);

STATIC_INLINE void flush_cpu_icache(void *from, void *to);
#endif
STATIC_INLINE void write_jmp_target(uae_u32 *jmpaddr, uintptr a);

uae_u32 m68k_pc_offset;

/* Some arithmetic operations can be optimized away if the operands
 * are known to be constant. But that's only a good idea when the
 * side effects they would have on the flags are not important. This
 * variable indicates whether we need the side effects or not
 */
static uae_u32 needflags = 0;

/* Flag handling is complicated.
 *
 * ARM instructions create flags, which quite often are exactly what we
 * want. So at times, the "68k" flags are actually in the ARM flags.
 * Exception: carry is often inverted.
 *
 * Then again, sometimes we do ARM instructions that clobber the ARM
 * flags, but don't represent a corresponding m68k instruction. In that
 * case, we have to save them.
 *
 * We used to save them to the stack, but now store them back directly
 * into the regflags.nzcv of the traditional emulation. Thus some odd
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

static inline blockinfo* get_blockinfo(uae_u32 cl)
{
    return cache_tags[cl + 1].bi;
}

static inline blockinfo* get_blockinfo_addr(void* addr)
{
    blockinfo* bi = get_blockinfo(cacheline(addr));

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

static inline void remove_from_cl_list(blockinfo* bi)
{
    uae_u32 cl = cacheline(bi->pc_p);

    if (bi->prev_same_cl_p)
        *(bi->prev_same_cl_p) = bi->next_same_cl;
    if (bi->next_same_cl)
        bi->next_same_cl->prev_same_cl_p = bi->prev_same_cl_p;
    if (cache_tags[cl + 1].bi)
        cache_tags[cl].handler = cache_tags[cl + 1].bi->handler_to_use;
    else
        cache_tags[cl].handler = (cpuop_func*)popall_execute_normal;
}

static inline void remove_from_list(blockinfo* bi)
{
    if (bi->prev_p)
        *(bi->prev_p) = bi->next;
    if (bi->next)
        bi->next->prev_p = bi->prev_p;
}

static inline void add_to_cl_list(blockinfo* bi)
{
    uae_u32 cl = cacheline(bi->pc_p);

    if (cache_tags[cl + 1].bi)
        cache_tags[cl + 1].bi->prev_same_cl_p = &(bi->next_same_cl);
    bi->next_same_cl = cache_tags[cl + 1].bi;

    cache_tags[cl + 1].bi = bi;
    bi->prev_same_cl_p = &(cache_tags[cl + 1].bi);

    cache_tags[cl].handler = bi->handler_to_use;
}

void raise_in_cl_list(blockinfo* bi)
{
    remove_from_cl_list(bi);
    add_to_cl_list(bi);
}

static inline void add_to_active(blockinfo* bi)
{
    if (active)
        active->prev_p = &(bi->next);
    bi->next = active;

    active = bi;
    bi->prev_p = &active;
}

static inline void add_to_dormant(blockinfo* bi)
{
    if (dormant)
        dormant->prev_p = &(bi->next);
    bi->next = dormant;

    dormant = bi;
    bi->prev_p = &dormant;
}

static inline void remove_dep(dependency* d)
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
static inline void remove_deps(blockinfo* bi)
{
    remove_dep(&(bi->dep[0]));
    remove_dep(&(bi->dep[1]));
}

static inline void adjust_jmpdep(dependency* d, cpuop_func* a)
{
    write_jmp_target(d->jmp_off, (uintptr)a);
}

/********************************************************************
 * Soft flush handling support functions                            *
 ********************************************************************/

static inline void set_dhtu(blockinfo* bi, cpuop_func* dh)
{
    jit_log2("bi is %p", bi);
    if (dh != bi->direct_handler_to_use) {
        dependency* x = bi->deplist;
        jit_log2("bi->deplist=%p", bi->deplist);
        while (x) {
            jit_log2("x is %p", x);
            jit_log2("x->next is %p", x->next);
            jit_log2("x->prev_p is %p", x->prev_p);

            if (x->jmp_off) {
                adjust_jmpdep(x, dh);
            }
            x = x->next;
        }
        bi->direct_handler_to_use = (cpuop_func*)dh;
    }
}

void invalidate_block(blockinfo* bi)
{
    int i;

    bi->optlevel = 0;
    bi->count = optcount[0] - 1;
    bi->handler = NULL;
    bi->handler_to_use = (cpuop_func*)popall_execute_normal;
    bi->direct_handler = NULL;
    set_dhtu(bi, bi->direct_pen);
    bi->needed_flags = 0xff;
    bi->status = BI_INVALID;
    for (i = 0; i < 2; i++) {
        bi->dep[i].jmp_off = NULL;
        bi->dep[i].target = NULL;
    }
    remove_deps(bi);
}

static inline void create_jmpdep(blockinfo* bi, int i, uae_u32* jmpaddr, uae_u32 target)
{
    blockinfo* tbi = get_blockinfo_addr((void*)(uintptr)target);

    Dif(!tbi) {
        jit_abort("Could not create jmpdep!");
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

static inline void block_need_recompile(blockinfo* bi)
{
    uae_u32 cl = cacheline(bi->pc_p);

    set_dhtu(bi, bi->direct_pen);
    bi->direct_handler = bi->direct_pen;

    bi->handler_to_use = (cpuop_func*)popall_execute_normal;
    bi->handler = (cpuop_func*)popall_execute_normal;
    if (bi == cache_tags[cl + 1].bi)
        cache_tags[cl].handler = (cpuop_func*)popall_execute_normal;
    bi->status = BI_NEED_RECOMP;
}

static inline blockinfo* get_blockinfo_addr_new(void* addr)
{
    blockinfo* bi = get_blockinfo_addr(addr);
    int i;

    if (!bi) {
        for (i = 0; i < MAX_HOLD_BI && !bi; i++) {
            if (hold_bi[i]) {
                (void)cacheline(addr);

                bi = hold_bi[i];
                hold_bi[i] = NULL;
                bi->pc_p = (uae_u8*)addr;
                invalidate_block(bi);
                add_to_active(bi);
                add_to_cl_list(bi);
            }
        }
    }
    if (!bi) {
        jit_abort(_T("JIT: Looking for blockinfo, can't find free one\n"));
    }
    return bi;
}

static void prepare_block(blockinfo* bi);

/* Management of blockinfos.

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
        kPoolSize = 1 + (16384 - sizeof(T) - sizeof(void*)) / sizeof(T)
    };
    struct Pool {
        T chunk[kPoolSize];
        Pool* next;
    };
    Pool* mPools;
    T* mChunks;
public:
    LazyBlockAllocator() : mPools(0), mChunks(0) { }
#ifdef UAE
#else
    ~LazyBlockAllocator();
#endif
    T* acquire();
    void release(T* const);
};

#ifdef UAE
/* uae_vm_release may do logging, which isn't safe to do when the application
 * is shutting down. Better to release memory manually with a function call
 * to a release_all method on shutdown, or even simpler, just let the OS
 * handle it (we're shutting down anyway). */
#else
template< class T >
LazyBlockAllocator<T>::~LazyBlockAllocator()
{
    Pool* currentPool = mPools;
    while (currentPool) {
        Pool* deadPool = currentPool;
        currentPool = currentPool->next;
        vm_release(deadPool, sizeof(Pool));
    }
}
#endif

template< class T >
T* LazyBlockAllocator<T>::acquire()
{
    if (!mChunks) {
        // There is no chunk left, allocate a new pool and link the
        // chunks into the free list
		Pool * newPool = (Pool *)vm_acquire(sizeof(Pool), VM_MAP_DEFAULT | VM_MAP_32BIT);
		if (newPool == VM_MAP_FAILED) {
			jit_abort("Could not allocate block pool!\n");
        }
        for (T* chunk = &newPool->chunk[0]; chunk < &newPool->chunk[kPoolSize]; chunk++) {
            chunk->next = mChunks;
            mChunks = chunk;
        }
        newPool->next = mPools;
        mPools = newPool;
    }
    T* chunk = mChunks;
    mChunks = chunk->next;
    return chunk;
}

template< class T >
void LazyBlockAllocator<T>::release(T* const chunk)
{
    chunk->next = mChunks;
    mChunks = chunk;
}

template< class T >
class HardBlockAllocator
{
public:
    T* acquire() {
        T* data = (T*)current_compile_p;
        current_compile_p += sizeof(T);
        return data;
    }

    void release(T* const) {
        // Deallocated on invalidation
    }
};

static LazyBlockAllocator<blockinfo> BlockInfoAllocator;
static LazyBlockAllocator<checksum_info> ChecksumInfoAllocator;

static inline checksum_info* alloc_checksum_info(void)
{
    checksum_info* csi = ChecksumInfoAllocator.acquire();
    csi->next = NULL;
    return csi;
}

static inline void free_checksum_info(checksum_info* csi)
{
    csi->next = NULL;
    ChecksumInfoAllocator.release(csi);
}

static inline void free_checksum_info_chain(checksum_info* csi)
{
    while (csi != NULL) {
        checksum_info* csi2 = csi->next;
        free_checksum_info(csi);
        csi = csi2;
    }
}

static inline blockinfo* alloc_blockinfo(void)
{
    blockinfo* bi = BlockInfoAllocator.acquire();
    bi->csi = NULL;
    return bi;
}

static inline void free_blockinfo(blockinfo* bi)
{
    free_checksum_info_chain(bi->csi);
    bi->csi = NULL;
    BlockInfoAllocator.release(bi);
}

static inline void alloc_blockinfos(void)
{
    int i;
    blockinfo* bi;

    for (i = 0; i < MAX_HOLD_BI; i++) {
        if (hold_bi[i])
            return;
        bi = hold_bi[i] = alloc_blockinfo();
#ifdef __MACH__
        // Turn off write protect (which prevents execution) on JIT cache while the blocks are prepared, this is Mac OS X specific, it will work on x86-64, but as a noop
        pthread_jit_write_protect_np(false);
#endif
        prepare_block(bi);
    }
}

/********************************************************************
 * Functions to emit data into memory, and other general support    *
 ********************************************************************/

static uae_u8* target;

static inline void emit_byte(uae_u8 x)
{
    *target++ = x;
}

static inline void skip_n_bytes(int n) {
    target += n;
}

static inline void skip_byte()
{
    skip_n_bytes(1);
}

static inline void skip_word()
{
    skip_n_bytes(2);
}

static inline void skip_long()
{
    skip_n_bytes(4);
}

static inline void skip_quad()
{
    skip_n_bytes(8);
}

static inline void emit_word(uae_u16 x)
{
    *((uae_u16*)target) = x;
    skip_word();
}

static inline void emit_long(uae_u32 x)
{
    *((uae_u32*)target) = x;
    skip_long();
}

static inline void emit_quad(uae_u64 x)
{
    *((uae_u64*)target) = x;
    skip_quad();
}

static inline void emit_block(const uae_u8* block, uae_u32 blocklen)
{
    memcpy((uae_u8*)target, block, blocklen);
    target += blocklen;
}

#define MAX_COMPILE_PTR max_compile_start

static inline uae_u32 reverse32(uae_u32 v)
{
    return uae_bswap_32(v);
}

static void set_target(uae_u8* t)
{
    target = t;
}

static inline uae_u8* get_target_noopt(void)
{
    return target;
}

inline uae_u8* get_target(void)
{
    return get_target_noopt();
}

/********************************************************************
 * New version of data buffer: interleave data and code             *
 ********************************************************************/
#if defined(USE_DATA_BUFFER)

#define DATA_BUFFER_SIZE 768             // Enlarge POPALLSPACE_SIZE if this value is greater than 768
#define DATA_BUFFER_MAXOFFSET 4096 - 32  // max range between emit of data and use of data
static uae_u8* data_writepos = 0;
static uae_u8* data_endpos = 0;
#ifdef DEBUG_DATA_BUFFER
static uae_u32 data_wasted = 0;
static uae_u32 data_buffers_used = 0;
#endif

STATIC_INLINE void compemu_raw_branch(IM32 d);

STATIC_INLINE void data_check_end(uae_s32 n, uae_s32 codesize)
{
    if (data_writepos + n > data_endpos || get_target() + codesize - data_writepos > DATA_BUFFER_MAXOFFSET) {
        // Start new buffer
#ifdef DEBUG_DATA_BUFFER
        if (data_writepos < data_endpos)
            data_wasted += data_endpos - data_writepos;
        data_buffers_used++;
#endif
        compemu_raw_branch(DATA_BUFFER_SIZE);
        data_writepos = get_target();
        data_endpos = data_writepos + DATA_BUFFER_SIZE;
        set_target(get_target() + DATA_BUFFER_SIZE);
    }
}

STATIC_INLINE uae_s32 data_word_offs(uae_u16 x)
{
    data_check_end(4, 4);
    *((uae_u16*)data_writepos) = x;
    data_writepos += 2;
    *((uae_u16*)data_writepos) = 0;
    data_writepos += 2;
    return (uae_s32)data_writepos - (uae_s32)get_target() - 12;
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
    return (uae_s32)data_writepos - (uae_s32)get_target() - 12;
}

STATIC_INLINE uae_s32 get_data_offset(uae_s32 t)
{
    return t - (uae_s32)get_target() - 8;
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
#ifdef AMIBERRY
STATIC_INLINE void clobber_flags(void);
#endif

#if defined(CPU_AARCH64) 
#include "codegen_arm64.cpp"
#elif defined(CPU_arm) 
#include "codegen_arm.cpp"
#endif

/********************************************************************
 * Flags status handling. EMIT TIME!                                *
 ********************************************************************/

static void bt_l_ri_noclobber(RR4 r, IM8 i);

static void make_flags_live_internal(void)
{
    if (live.flags_in_flags == VALID)
        return;
    Dif(live.flags_on_stack == TRASH) {
        jit_abort("Want flags, got something on stack, but it is TRASH");
    }
    if (live.flags_on_stack == VALID) {
        int tmp;
        tmp = readreg(FLAGTMP);
        raw_reg_to_flags(tmp);
        unlock2(tmp);
        flags_carry_inverted = false;

        live.flags_in_flags = VALID;
        return;
    }
    jit_abort("Huh? live.flags_in_flags=%d, live.flags_on_stack=%d, but need to make live",
        live.flags_in_flags, live.flags_on_stack);
}

static void flags_to_stack(void)
{
    if (live.flags_on_stack == VALID) {
        flags_carry_inverted = false;
        return;
    }
    if (!live.flags_are_important) {
        live.flags_on_stack = VALID;
        flags_carry_inverted = false;
        return;
    }
    Dif(live.flags_in_flags != VALID)
        jit_abort("flags_to_stack != VALID");
    else {
        int tmp = writereg(FLAGTMP);
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
static inline void flush_flags(void)
{
    flags_to_stack();
}

static int touchcnt;

/********************************************************************
 * Partial register flushing for optimized calls                    *
 ********************************************************************/

struct regusage {
    uae_u16 rmask;
    uae_u16 wmask;
};

/********************************************************************
 * register allocation per block logging                            *
 ********************************************************************/

static uae_s8 vstate[VREGS];
static uae_s8 vwritten[VREGS];
static uae_s8 nstate[N_REGS];

#define L_UNKNOWN -127
#define L_UNAVAIL -1
#define L_NEEDED -2
#define L_UNNEEDED -3

static inline void log_startblock(void)
{
    int i;

    for (i = 0; i < VREGS; i++) {
        vstate[i] = L_UNKNOWN;
        vwritten[i] = 0;
    }
    for (i = 0; i < N_REGS; i++)
        nstate[i] = L_UNKNOWN;
}

/* Using an n-reg for a temp variable */
static inline void log_isused(int n)
{
    if (nstate[n] == L_UNKNOWN)
        nstate[n] = L_UNAVAIL;
}

static inline void log_visused(int r)
{
    if (vstate[r] == L_UNKNOWN)
        vstate[r] = L_NEEDED;
}

STATIC_INLINE void do_load_reg(int n, int r)
{
    compemu_raw_mov_l_rm(n, (uintptr)live.state[r].mem);
}

static inline void log_vwrite(int r)
{
    vwritten[r] = 1;
}

/* Using an n-reg to hold a v-reg */
static inline void log_isreg(int n, int r)
{
    if (nstate[n] == L_UNKNOWN && r < 16 && !vwritten[r] && 0)
        nstate[n] = r;
    else {
        do_load_reg(n, r);
        if (nstate[n] == L_UNKNOWN)
            nstate[n] = L_UNAVAIL;
    }
    if (vstate[r] == L_UNKNOWN)
        vstate[r] = L_NEEDED;
}

static inline void log_clobberreg(int r)
{
    if (vstate[r] == L_UNKNOWN)
        vstate[r] = L_UNNEEDED;
}

/* This ends all possibility of clever register allocation */

static inline void log_flush(void)
{
    int i;

    for (i = 0; i < VREGS; i++)
        if (vstate[i] == L_UNKNOWN)
            vstate[i] = L_NEEDED;
    for (i = 0; i < N_REGS; i++)
        if (nstate[i] == L_UNKNOWN)
            nstate[i] = L_UNAVAIL;
}

static inline void log_dump(void)
{
    return;
}

/********************************************************************
 * register status handling. EMIT TIME!                             *
 ********************************************************************/

static inline void set_status(int r, int status)
{
    if (status == ISCONST)
        log_clobberreg(r);
    live.state[r].status = status;
}

static inline int isinreg(int r)
{
    return live.state[r].status == CLEAN || live.state[r].status == DIRTY;
}

static void tomem(int r)
{
    int rr = live.state[r].realreg;

    if (live.state[r].status == DIRTY) {
        compemu_raw_mov_l_mr((uintptr)live.state[r].mem, live.state[r].realreg);
        set_status(r, CLEAN);
    }
}

static inline int isconst(int r)
{
    return live.state[r].status == ISCONST;
}

int is_const(int r)
{
    return isconst(r);
}

static inline void writeback_const(int r)
{
    if (!isconst(r))
        return;
    Dif(live.state[r].needflush == NF_HANDLER) {
        jit_abort("Trying to write back constant NF_HANDLER!");
    }

    compemu_raw_mov_l_mi((uintptr)live.state[r].mem, live.state[r].val);
    log_vwrite(r);
    live.state[r].val = 0;
    set_status(r, INMEM);
}

static inline void tomem_c(int r)
{
    if (isconst(r)) {
        writeback_const(r);
    }
    else
        tomem(r);
}

static void evict(int r)
{
    if (!isinreg(r))
        return;
    tomem(r);
    int rr = live.state[r].realreg;

    Dif(live.nat[rr].locked &&
        live.nat[rr].nholds == 1) {
        jit_abort("register %d in nreg %d is locked!", r, live.state[r].realreg);
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

static inline void free_nreg(int r)
{
    int i = live.nat[r].nholds;

    while (i) {
        int vr;

        --i;
        vr = live.nat[r].holds[i];
        evict(vr);
    }
    Dif(live.nat[r].nholds != 0) {
        jit_abort("Failed to free nreg %d, nholds is %d", r, live.nat[r].nholds);
    }
}

/* Use with care! */
static inline void isclean(int r)
{
    if (!isinreg(r))
        return;
    live.state[r].val = 0;
    set_status(r, CLEAN);
}

static inline void disassociate(int r)
{
    isclean(r);
    evict(r);
}

static inline void set_const(int r, uae_u32 val)
{
    disassociate(r);
    live.state[r].val = val;
    set_status(r, ISCONST);
}

static inline uae_u32 get_offset(int r)
{
    return live.state[r].val;
}

#ifdef AMIBERRY
bool has_free_reg(void)
{
    for (int i = N_REGS - 1; i >= 0; i--) {
        if (!live.nat[i].locked) {
            if (live.nat[i].nholds == 0)
                return true;
        }
    }
    return false;
}
#endif

static int alloc_reg_hinted(int r, int willclobber, int hint)
{
    int bestreg = -1;
    uae_s32 when = 2000000000;
    int i;

    for (i = N_REGS - 1; i >= 0; i--) {
        if (!live.nat[i].locked) {
            uae_s32 badness = live.nat[i].touched;
            if (live.nat[i].nholds == 0)
                badness = 0;
            if (i == hint)
                badness -= 200000000;
            if (badness < when) {
                bestreg = i;
                when = badness;
                if (live.nat[i].nholds == 0 && hint < 0)
                    break;
                if (i == hint)
                    break;
            }
        }
    }
    Dif(bestreg == -1)
        jit_abort("alloc_reg_hinted bestreg=-1");

    if (live.nat[bestreg].nholds > 0) {
        free_nreg(bestreg);
    }

    if (!willclobber) {
        if (live.state[r].status != UNDEF) {
            if (isconst(r)) {
                compemu_raw_mov_l_ri(bestreg, live.state[r].val);
                live.state[r].val = 0;
                set_status(r, DIRTY);
                log_isused(bestreg);
            } else {
                do_load_reg(bestreg, r);
                set_status(r, CLEAN);
            }
        } else {
            live.state[r].val = 0;
            set_status(r, CLEAN);
            log_isused(bestreg);
        }
    } else { /* this is the easiest way, but not optimal. */
        live.state[r].val = 0;
        set_status(r, DIRTY);
    }
    live.state[r].realreg = bestreg;
    live.state[r].realind = 0;
    live.nat[bestreg].touched = touchcnt++;
    live.nat[bestreg].holds[0] = r;
    live.nat[bestreg].nholds = 1;

    return bestreg;
}


static void unlock2(int r)
{
    Dif(!live.nat[r].locked)
        jit_abort("unlock2 %d not locked", r);
    live.nat[r].locked--;
}

static void setlock(int r)
{
    live.nat[r].locked++;
}


static void mov_nregs(int d, int s)
{
    if (s == d)
        return;

    if (live.nat[d].nholds > 0)
        free_nreg(d);

    log_isused(d);
    compemu_raw_mov_l_rr(d, s);

    for (int i = 0; i < live.nat[s].nholds; i++) {
        int vs = live.nat[s].holds[i];

        live.state[vs].realreg = d;
        live.state[vs].realind = i;
        live.nat[d].holds[i] = vs;
    }
    live.nat[d].nholds = live.nat[s].nholds;

    live.nat[s].nholds = 0;
}


static inline void make_exclusive(int r, int needcopy)
{
    reg_status oldstate;
    int rr = live.state[r].realreg;
    int nr;
    int nind;

    if (!isinreg(r))
        return;
    if (live.nat[rr].nholds == 1)
        return;

    /* We have to split the register */
    oldstate = live.state[r];

    setlock(rr); /* Make sure this doesn't go away */
    /* Forget about r being in the register rr */
    disassociate(r);
    /* Get a new register, that we will clobber completely */
    nr = alloc_reg_hinted(r, 1, -1);
    nind = live.state[r].realind;
    live.state[r] = oldstate;   /* Keep all the old state info */
    live.state[r].realreg = nr;
    live.state[r].realind = nind;

    if (needcopy) {
        compemu_raw_mov_l_rr(nr, rr);  /* Make another copy */
    }
    unlock2(rr);
}

static inline int readreg_general(int r, int spec)
{
    int answer = -1;

    if (live.state[r].status == UNDEF) {
        jit_log("WARNING: Unexpected read of undefined register %d", r);
    }

    if (isinreg(r)) {
        answer = live.state[r].realreg;
    } else {
        /* the value is in memory to start with */
        answer = alloc_reg_hinted(r, 0, spec);
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


static int readreg(int r)
{
    return readreg_general(r, -1);
}

static int readreg_specific(int r, int spec)
{
    return readreg_general(r, spec);
}

/* writereg(r)
 *
 * INPUT
 * - r    : mid-layer register
 *
 * OUTPUT
 * - hard (physical, x86 here) register allocated to virtual register r
 */
static int writereg(int r)
{
    int answer = -1;

    make_exclusive(r, 0);
    if (isinreg(r)) {
        answer = live.state[r].realreg;
    } else {
        /* the value is in memory to start with */
        answer = alloc_reg_hinted(r, 1, -1);
    }

    live.nat[answer].locked++;
    live.nat[answer].touched = touchcnt++;
    live.state[r].val = 0;
    set_status(r, DIRTY);
    return answer;
}

static int rmw(int r)
{
    int answer = -1;

    if (live.state[r].status == UNDEF) {
        jit_log("WARNING: Unexpected read of undefined register %d", r);
    }
    make_exclusive(r, 1);

    if (isinreg(r)) {
        answer = live.state[r].realreg;
    } else {
        /* the value is in memory to start with */
        answer = alloc_reg_hinted(r, 0, -1);
    }

    set_status(r, DIRTY);

    live.nat[answer].locked++;
    live.nat[answer].touched = touchcnt++;

    return answer;
}

/********************************************************************
 * FPU register status handling. EMIT TIME!                         *
 ********************************************************************/

static void f_tomem_drop(int r)
{
    if (live.fate[r].status == DIRTY) {
        compemu_raw_fmov_mr_drop((uintptr)live.fate[r].mem, live.fate[r].realreg);
        live.fate[r].status = INMEM;
    }
}


static int f_isinreg(int r)
{
    return live.fate[r].status == CLEAN || live.fate[r].status == DIRTY;
}

static void f_evict(int r)
{
    int rr;

    if (!f_isinreg(r))
        return;
    rr = live.fate[r].realreg;
    f_tomem_drop(r);

    live.fat[rr].nholds = 0;
    live.fate[r].status = INMEM;
    live.fate[r].realreg = -1;
}

static inline void f_free_nreg(int r)
{
    int vr = live.fat[r].holds;
    f_evict(vr);
}


/* Use with care! */
static inline void f_isclean(int r)
{
    if (!f_isinreg(r))
        return;
    live.fate[r].status = CLEAN;
}

static inline void f_disassociate(int r)
{
    f_isclean(r);
    f_evict(r);
}

static int f_alloc_reg(int r, int willclobber)
{
    int bestreg;

    if (r < 8)
        bestreg = r + 8;   // map real Amiga reg to ARM VFP reg 8-15 (call save)
    else if (r == FP_RESULT)
        bestreg = 6;         // map FP_RESULT to ARM VFP reg 6
    else // FS1
        bestreg = 7;         // map FS1 to ARM VFP reg 7

    if (!willclobber) {
        if (live.fate[r].status == INMEM) {
            compemu_raw_fmov_rm(bestreg, (uintptr)live.fate[r].mem);
            live.fate[r].status = CLEAN;
        }
    } else {
        live.fate[r].status = DIRTY;
    }
    live.fate[r].realreg = bestreg;
    live.fat[bestreg].holds = r;
    live.fat[bestreg].nholds = 1;

    return bestreg;
}

static void f_unlock(int r)
{
}

static inline int f_readreg(int r)
{
    int answer;

    if (f_isinreg(r)) {
        answer = live.fate[r].realreg;
    } else {
        /* the value is in memory to start with */
        answer = f_alloc_reg(r, 0);
    }

    return answer;
}

static inline int f_writereg(int r)
{
    int answer;

    if (f_isinreg(r)) {
        answer = live.fate[r].realreg;
    }  else {
        answer = f_alloc_reg(r, 1);
    }
    live.fate[r].status = DIRTY;
    return answer;
}

STATIC_INLINE int f_rmw(int r)
{
    int n;

    if (f_isinreg(r)) {
        n = live.fate[r].realreg;
    } else {
        n = f_alloc_reg(r, 0);
    }
    live.fate[r].status = DIRTY;
    return n;
}

static void fflags_into_flags_internal(void)
{
    int r = f_readreg(FP_RESULT);
    raw_fflags_into_flags(r);
    f_unlock(r);
    live_flags();
}

/********************************************************************
 * Support functions, internal                                      *
 ********************************************************************/

static inline int isinrom(uintptr addr)
{
#ifdef UAE
    return (addr >= uae_p32(kickmem_bank.baseaddr) &&
        addr < uae_p32(kickmem_bank.baseaddr + 8 * 65536));
#else
    return ((addr >= (uintptr)ROMBaseHost) && (addr < (uintptr)ROMBaseHost + ROMSize));
#endif
}

#if defined(UAE) || defined(FLIGHT_RECORDER)
static void flush_all(void)
{
    int i;

    for (i = 0; i < VREGS; i++) {
        if (live.state[i].status == DIRTY) {
            if (!call_saved[live.state[i].realreg]) {
                tomem(i);
            }
        }
    }

    if (f_isinreg(FP_RESULT))
        f_evict(FP_RESULT);
    if (f_isinreg(FS1))
        f_evict(FS1);
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
    for (i = 0; i < N_REGS; i++) {
#if defined(CPU_AARCH64)
        if (live.nat[i].nholds > 0) // in aarch64: first 18 regs not call saved
#else
        if (!call_saved[i] && live.nat[i].nholds > 0)
#endif
            free_nreg(i);
    }

#ifdef USE_JIT_FPU
    for (i = 6; i <= 7; i++) // only FP_RESULT and FS1, FP0-FP7 are call save
        if (live.fat[i].nholds > 0)
            f_free_nreg(i);
#endif
    live.flags_in_flags = TRASH;  /* Note: We assume we already rescued the
                                     flags at the very start of the call_r functions! */
}
#endif

#if defined(CPU_AARCH64) 
#include "compemu_midfunc_arm64.cpp"
#include "compemu_midfunc_arm64_2.cpp"
#elif defined(CPU_arm)
#include "compemu_midfunc_arm.cpp"
#include "compemu_midfunc_arm2.cpp"
#endif

/********************************************************************
 * Support functions exposed to gencomp. CREATE time                *
 ********************************************************************/

#ifdef __MACH__
void cache_invalidate(void) {
        // Invalidate cache on the JIT cache
        sys_icache_invalidate(popallspace, POPALLSPACE_SIZE + MAX_JIT_CACHE * 1024);
}
#endif

uae_u32 get_const(int r)
{
    Dif(!isconst(r)) {
        jit_abort("Register %d should be constant, but isn't", r);
    }
    return live.state[r].val;
}

void sync_m68k_pc(void)
{
    if (m68k_pc_offset) {
        arm_ADD_l_ri(PC_P, m68k_pc_offset);
        comp_pc_p += m68k_pc_offset;
        m68k_pc_offset = 0;
    }
}

/********************************************************************
 * Support functions exposed to newcpu                              *
 ********************************************************************/

void compiler_init(void)
{
    static bool initialized = false;
    if (initialized)
        return;

    flush_icache = flush_icache_none;

    flush_icache = lazy_flush ? flush_icache_lazy : flush_icache_hard;

    initialized = true;

#ifdef PROFILE_UNTRANSLATED_INSNS
    jit_log("<JIT compiler> : gather statistics on untranslated insns count");
#endif

#ifdef PROFILE_COMPILE_TIME
    jit_log("<JIT compiler> : gather statistics on translation time");
    emul_start_time = clock();
#endif
}

void compiler_exit(void)
{
#ifdef PROFILE_COMPILE_TIME
    emul_end_time = clock();
#endif

#ifdef UAE
#else
#if DEBUG
#if defined(USE_DATA_BUFFER)
    jit_log("data_wasted = %ld bytes", data_wasted);
#endif
#endif

    // Deallocate translation cache
    if (compiled_code) {
        vm_release(compiled_code, cache_size * 1024);
        compiled_code = 0;
    }

    // Deallocate popallspace
    if (popallspace) {
        vm_release(popallspace, POPALLSPACE_SIZE);
        popallspace = 0;
    }
#endif

#ifdef PROFILE_COMPILE_TIME
    jit_log("### Compile Block statistics");
    jit_log("Number of calls to compile_block : %d", compile_count);
    uae_u32 emul_time = emul_end_time - emul_start_time;
    jit_log("Total emulation time   : %.1f sec", double(emul_time) / double(CLOCKS_PER_SEC));
    jit_log("Total compilation time : %.1f sec (%.1f%%)", double(compile_time) / double(CLOCKS_PER_SEC), 100.0 * double(compile_time) / double(emul_time));
#endif

#ifdef PROFILE_UNTRANSLATED_INSNS
    uae_u64 untranslated_count = 0;
    for (int i = 0; i < 65536; i++) {
        opcode_nums[i] = i;
        untranslated_count += raw_cputbl_count[i];
    }
    bug("Sorting out untranslated instructions count, total %llu...\n", untranslated_count);
    qsort(opcode_nums, 65536, sizeof(uae_u16), untranslated_compfn);
    jit_log("Rank  Opc      Count Name\n");
    for (int i = 0; i < untranslated_top_ten; i++) {
        uae_u32 count = raw_cputbl_count[opcode_nums[i]];
        struct instr* dp;
        struct mnemolookup* lookup;
        if (!count)
            break;
        dp = table68k + opcode_nums[i];
        for (lookup = lookuptab; lookup->mnemo != (instrmnem)dp->mnemo; lookup++)
            ;
        bug(_T("%03d: %04x %10u %s\n"), i, opcode_nums[i], count, lookup->name);
    }
#endif

#ifdef RECORD_REGISTER_USAGE
    int reg_count_ids[16];
    uint64 tot_reg_count = 0;
    for (int i = 0; i < 16; i++) {
        reg_count_ids[i] = i;
        tot_reg_count += reg_count[i];
    }
    qsort(reg_count_ids, 16, sizeof(int), reg_count_compare);
    uint64 cum_reg_count = 0;
    for (int i = 0; i < 16; i++) {
        int r = reg_count_ids[i];
        cum_reg_count += reg_count[r];
        jit_log("%c%d : %16ld %2.1f%% [%2.1f]", r < 8 ? 'D' : 'A', r % 8,
            reg_count[r],
            100.0 * double(reg_count[r]) / double(tot_reg_count),
            100.0 * double(cum_reg_count) / double(tot_reg_count));
    }
#endif

	exit_table68k();
}

static void init_comp(void)
{
    int i;
    uae_s8* au = always_used;

    for (i = 0; i < VREGS; i++) {
        live.state[i].realreg = -1;
        live.state[i].val = 0;
        set_status(i, UNDEF);
    }
    for (i = 0; i < SCRATCH_REGS; ++i)
        live.scratch_in_use[i] = 0;

    for (i = 0; i < VFREGS; i++) {
        live.fate[i].status = UNDEF;
        live.fate[i].realreg = -1;
        live.fate[i].needflush = NF_SCRATCH;
    }

    for (i = 0; i < VREGS; i++) {
        if (i < 16) { /* First 16 registers map to 68k registers */
            live.state[i].mem = &regs.regs[i];
            set_status(i, INMEM);
        } else if (i >= S1) {
            live.state[i].mem = &regs.scratchregs[i - S1];
        }
    }
    live.state[PC_P].mem = (uae_u32*)&(regs.pc_p);
    set_const(PC_P, (uintptr)comp_pc_p);

    live.state[FLAGX].mem = (uae_u32*)&(regflags.x);
    set_status(FLAGX, INMEM);
  
    live.state[FLAGTMP].mem = (uae_u32*)&(regflags.nzcv);

    set_status(FLAGTMP, INMEM);
    flags_carry_inverted = false;

    for (i = 0; i < VFREGS; i++) {
        if (i < 8) { /* First 8 registers map to 68k FPU registers */
            live.fate[i].mem = (uae_u32*)(&regs.fp[i].fp);
            live.fate[i].needflush = NF_TOMEM;
            live.fate[i].status = INMEM;
        } else if (i == FP_RESULT) {
            live.fate[i].mem = (uae_u32*)(&regs.fp_result.fp);
            live.fate[i].needflush = NF_TOMEM;
            live.fate[i].status = INMEM;
        } else {
            live.fate[i].mem = (uae_u32*)(&regs.scratchfregs[i - 8]);
        }
    }

    for (i = 0; i < N_REGS; i++) {
        live.nat[i].touched = 0;
        live.nat[i].nholds = 0;
        live.nat[i].locked = 0;
        if (*au == i) {
            live.nat[i].locked = 1;
            au++;
        }
    }

    for (i = 0; i < N_FREGS; i++) {
        live.fat[i].nholds = 0;
    }

    touchcnt = 1;
    m68k_pc_offset = 0;
    live.flags_in_flags = TRASH;
    live.flags_on_stack = VALID;
    live.flags_are_important = 1;

    regs.jit_exception = 0;
}

/* Only do this if you really mean it! The next call should be to init!*/
static void flush(int save_regs)
{
    int i;

    log_flush();
    flush_flags(); /* low level */
    sync_m68k_pc(); /* mid level */

    if (save_regs) {
        for (i = 0; i < VFREGS; i++) {
            if (live.fate[i].needflush == NF_SCRATCH || live.fate[i].status == CLEAN) {
                f_disassociate(i);
            }
        }
        for (i = 0; i <= FLAGTMP; i++) {
            switch (live.state[i].status) {
            case INMEM:
                if (live.state[i].val) {
                    write_log("JIT: flush INMEM and val != 0!\n");
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
        }
        for (i = 0; i <= FP_RESULT; i++) {
            if (live.fate[i].status == DIRTY) {
                f_evict(i);
            }
        }
    }
}

int alloc_scratch(void)
{
    for (int i = 0; i < SCRATCH_REGS; ++i) {
        if (live.scratch_in_use[i] == 0) {
            live.scratch_in_use[i] = 1;
            return S1 + i;
        }
    }
    jit_log("Running out of scratch register.");
    abort();
}

void release_scratch(int i)
{
    if (i < S1 || i >= S1 + SCRATCH_REGS)
        jit_log("release_scratch(): %d is not a scratch reg.", i);
    if (live.scratch_in_use[i - S1]) {
        forget_about(i);
        live.scratch_in_use[i - S1] = 0;
    }
    else {
        jit_log("release_scratch(): %d not in use.", i);
    }
}

static void freescratch(void)
{
    int i;
    for (i = 0; i < N_REGS; i++) {
#if defined(CPU_AARCH64) 
        if (live.nat[i].locked && i > 5 && i < 18) {
#elif defined(CPU_arm)
        if (live.nat[i].locked && i != 2 && i != 3 && i != 10 && i != 11 && i != 12) {
#else
        if (live.nat[i].locked && i != 4 && i != 12) {
#endif
            jit_log("Warning! %d is locked", i);
        }
    }

    for (i = S1; i < VREGS; i++)
        forget_about(i);
    for (i = 0; i < SCRATCH_REGS; ++i)
        live.scratch_in_use[i] = 0;

#ifdef USE_JIT_FPU
    f_forget_about(FS1);
#endif
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

void register_possible_exception(void)
{
    may_raise_exception = true;
}

/* Note: get_handler may fail in 64 Bit environments, if direct_handler_to_use is
 *       outside 32 bit
 */
static uintptr get_handler(uintptr addr)
{
    blockinfo* bi = get_blockinfo_addr_new((void*)(uintptr)addr);
    return (uintptr)bi->direct_handler_to_use;
}

/* This version assumes that it is writing *real* memory, and *will* fail
 *  if that assumption is wrong! No branches, no second chances, just
 *  straight go-for-it attitude */

static void writemem_real(int address, int source, int size)
{
    if (currprefs.address_space_24) {
        switch (size) {
        case 1: jnf_MEM_WRITE24_OFF_b(address, source); break;
        case 2: jnf_MEM_WRITE24_OFF_w(address, source); break;
        case 4: jnf_MEM_WRITE24_OFF_l(address, source); break;
        }
    } else {
        switch (size) {
        case 1: jnf_MEM_WRITE_OFF_b(address, source); break;
        case 2: jnf_MEM_WRITE_OFF_w(address, source); break;
        case 4: jnf_MEM_WRITE_OFF_l(address, source); break;
        }
    }
}

static inline void writemem_special(int address, int source, int offset)
{
    jnf_MEM_WRITEMEMBANK(address, source, offset);
}

void writebyte(int address, int source)
{
    if (special_mem & S_WRITE)
        writemem_special(address, source, SIZEOF_VOID_P * 5);
    else
        writemem_real(address, source, 1);
}

void writeword(int address, int source)
{
    if (special_mem & S_WRITE)
        writemem_special(address, source, SIZEOF_VOID_P * 4);
    else
        writemem_real(address, source, 2);
}

void writelong(int address, int source)
{
    if (special_mem & S_WRITE)
        writemem_special(address, source, SIZEOF_VOID_P * 3);
    else
        writemem_real(address, source, 4);
}

// Now the same for clobber variant
void writeword_clobber(int address, int source)
{
    if (special_mem & S_WRITE)
        writemem_special(address, source, SIZEOF_VOID_P * 4);
    else
        writemem_real(address, source, 2);
    forget_about(source);
}

void writelong_clobber(int address, int source)
{
    if (special_mem & S_WRITE)
        writemem_special(address, source, SIZEOF_VOID_P * 3);
    else
        writemem_real(address, source, 4);
    forget_about(source);
}


/* This version assumes that it is reading *real* memory, and *will* fail
 *  if that assumption is wrong! No branches, no second chances, just
 *  straight go-for-it attitude */

static void readmem_real(int address, int dest, int size)
{
    if (currprefs.address_space_24) {
        switch (size) {
        case 1: jnf_MEM_READ24_OFF_b(dest, address); break;
        case 2: jnf_MEM_READ24_OFF_w(dest, address); break;
        case 4: jnf_MEM_READ24_OFF_l(dest, address); break;
        }
    } else {
        switch (size) {
        case 1: jnf_MEM_READ_OFF_b(dest, address); break;
        case 2: jnf_MEM_READ_OFF_w(dest, address); break;
        case 4: jnf_MEM_READ_OFF_l(dest, address); break;
        }
    }
}

static inline void readmem_special(int address, int dest, int offset)
{
    jnf_MEM_READMEMBANK(dest, address, offset);
}

void readbyte(int address, int dest)
{
    if (special_mem & S_READ)
        readmem_special(address, dest, SIZEOF_VOID_P * 2);
    else
        readmem_real(address, dest, 1);
}

void readword(int address, int dest)
{
    if (special_mem & S_READ)
        readmem_special(address, dest, SIZEOF_VOID_P * 1);
    else
        readmem_real(address, dest, 2);
}

void readlong(int address, int dest)
{
    if (special_mem & S_READ)
        readmem_special(address, dest, SIZEOF_VOID_P * 0);
    else
        readmem_real(address, dest, 4);
}

/* This one might appear a bit odd... */
STATIC_INLINE void get_n_addr_old(int address, int dest)
{
    readmem_special(address, dest, SIZEOF_VOID_P * 6);
}

STATIC_INLINE void get_n_addr_real(int address, int dest)
{
    if (currprefs.address_space_24)
        jnf_MEM_GETADR24_OFF(dest, address);
    else
        jnf_MEM_GETADR_OFF(dest, address);
}

void get_n_addr(int address, int dest)
{
    if (special_mem)
        get_n_addr_old(address, dest);
    else
        get_n_addr_real(address, dest);
}

void get_n_addr_jmp(int address, int dest)
{
    /* For this, we need to get the same address as the rest of UAE
       would --- otherwise we end up translating everything twice */
    if (special_mem)
        get_n_addr_old(address, dest);
    else
        get_n_addr_real(address, dest);
}

/* base is a register, but dp is an actual value. 
   target is a register */
void calc_disp_ea_020(int base, uae_u32 dp, int target)
{
    int reg = (dp >> 12) & 15;
    int regd_shift = (dp >> 9) & 3;

    if (dp & 0x100) {
        int ignorebase = (dp & 0x80);
        int ignorereg = (dp & 0x40);
        int addbase = 0;
        int outer = 0;

        if ((dp & 0x30) == 0x20)
            addbase = (uae_s32)(uae_s16)comp_get_iword((m68k_pc_offset += 2) - 2);
        if ((dp & 0x30) == 0x30)
            addbase = comp_get_ilong((m68k_pc_offset += 4) - 4);

        if ((dp & 0x3) == 0x2)
            outer = (uae_s32)(uae_s16)comp_get_iword((m68k_pc_offset += 2) - 2);
        if ((dp & 0x3) == 0x3)
            outer = comp_get_ilong((m68k_pc_offset += 4) - 4);

        if ((dp & 0x4) == 0) {  /* add regd *before* the get_long */
            if (!ignorereg) {
                disp_ea20_target_mov(target, reg, regd_shift, ((dp & 0x800) == 0));
            } else {
                mov_l_ri(target, 0);
            }

            /* target is now regd */
            if (!ignorebase)
                arm_ADD_l(target, base);
            arm_ADD_l_ri(target, addbase);
            if (dp & 0x03)
                readlong(target, target);
        } else { /* do the getlong first, then add regd */
            if (!ignorebase) {
                mov_l_rr(target, base);
                arm_ADD_l_ri(target, addbase);
            } else {
                mov_l_ri(target, addbase);
            }
            if (dp & 0x03)
                readlong(target, target);

            if (!ignorereg) {
                disp_ea20_target_add(target, reg, regd_shift, ((dp & 0x800) == 0));
            }
        }
        arm_ADD_l_ri(target, outer);
    } else { /* 68000 version */
        if ((dp & 0x800) == 0) { /* Sign extend */
            sign_extend_16_rr(target, reg);
            lea_l_brr_indexed(target, base, target, 1 << regd_shift, (uae_s8)dp);
        } else {
            lea_l_brr_indexed(target, base, reg, 1 << regd_shift, (uae_s8)dp);
        }
    }
}

void set_cache_state(int enabled)
{
    if (enabled != cache_enabled)
        flush_icache_hard(3);
    cache_enabled = enabled;
}

int get_cache_state(void)
{
	return cache_enabled;
}

uae_u32 get_jitted_size(void)
{
	if (compiled_code)
		return JITPTR current_compile_p - JITPTR compiled_code;
	return 0;
}

static uint8 *do_alloc_code(uint32 size, int depth)
{
	UNUSED(depth);
    uint8*code = (uint8 *)vm_acquire(size, VM_MAP_DEFAULT | VM_MAP_32BIT);
	return code == VM_MAP_FAILED ? NULL : code;
}

static inline uint8 *alloc_code(uint32 size)
{
    uint8 *ptr = do_alloc_code(size, 0);
	/* allocated code must fit in 32-bit boundaries */
	assert((uintptr)ptr <= 0xffffffff);
	return ptr;
}

void alloc_cache(void)
{
    if (compiled_code) {
        flush_icache_hard(3);
		vm_release(compiled_code, cache_size * 1024);
        compiled_code = 0;
    }

    cache_size = currprefs.cachesize;
    if (cache_size == 0)
        return;

	while (!compiled_code && cache_size) {
		if ((compiled_code = alloc_code(cache_size * 1024)) == NULL) {
			compiled_code = 0;
			cache_size /= 2;
		}
	}
	vm_protect(compiled_code, cache_size * 1024, VM_PAGE_READ | VM_PAGE_WRITE | VM_PAGE_EXECUTE);

    if (compiled_code) {
        jit_log("<JIT compiler> : actual translation cache size : %d KB at %p-%p\n", cache_size, compiled_code, compiled_code + cache_size * 1024);
#ifdef USE_DATA_BUFFER
        max_compile_start = compiled_code + cache_size * 1024 - BYTES_PER_INST - DATA_BUFFER_SIZE;
#else
        max_compile_start = compiled_code + cache_size * 1024 - BYTES_PER_INST;
#endif
        current_compile_p = compiled_code;
        current_cache_size = 0;
#if defined(USE_DATA_BUFFER)
        reset_data_buffer();
#endif
    }
}

static void calc_checksum(blockinfo* bi, uae_u32* c1, uae_u32* c2)
{
    uae_u32 k1 = 0;
    uae_u32 k2 = 0;

    checksum_info* csi = bi->csi;
    Dif(!csi) abort();
    while (csi) {
        uae_s32 len = csi->length;
        uintptr tmp = (uintptr)csi->start_p;
        uae_u32* pos;

        len += (tmp & 3);
        tmp &= ~((uintptr)3);
        pos = (uae_u32*)tmp;

        if (len >= 0 && len <= MAX_CHECKSUM_LEN) {
            while (len > 0) {
                k1 += *pos;
                k2 ^= *pos;
                pos++;
                len -= 4;
            }
        }

        csi = csi->next;
    }

    *c1 = k1;
    *c2 = k2;
}

int check_for_cache_miss(void)
{
    blockinfo* bi = get_blockinfo_addr(regs.pc_p);

    if (bi) {
        int cl = cacheline(regs.pc_p);
        if (bi != cache_tags[cl + 1].bi) {
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

    Dif(!bi)
        jit_abort("recompile_block");
    raise_in_cl_list(bi);
    execute_normal();
}

static void cache_miss(void)
{
    blockinfo* bi = get_blockinfo_addr(regs.pc_p);
#if COMP_DEBUG
    uae_u32     cl = cacheline(regs.pc_p);
    blockinfo* bi2 = get_blockinfo(cl);
#endif

    if (!bi) {
        execute_normal(); /* Compile this block now */
        return;
    }
    Dif(!bi2 || bi == bi2) {
        jit_abort("Unexplained cache miss %p %p", bi, bi2);
    }
    raise_in_cl_list(bi);
}

static int called_check_checksum(blockinfo* bi);

static inline int block_check_checksum(blockinfo* bi)
{
    uae_u32 c1, c2;
    int isgood;

    if (bi->status != BI_NEED_CHECK)
        return 1;  /* This block is in a checked state */

    if (bi->c1 || bi->c2)
        calc_checksum(bi, &c1, &c2);
    else
        c1 = c2 = 1;  /* Make sure it doesn't match */

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
        jit_log2("reactivate %p/%p (%x %x/%x %x)", bi, bi->pc_p, c1, c2, bi->c1, bi->c2);
        remove_from_list(bi);
        add_to_active(bi);
        raise_in_cl_list(bi);
        bi->status = BI_ACTIVE;
    } else {
        /* This block actually changed. We need to invalidate it,
           and set it up to be recompiled */
        jit_log2("discard %p/%p (%x %x/%x %x)", bi, bi->pc_p, c1, c2, bi->c1, bi->c2);
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
    if (bi->status == BI_NEED_CHECK) {
        block_check_checksum(bi);
    }
}

STATIC_INLINE void create_popalls(void)
{
    int i, r;

    if (popallspace == NULL) {
        if ((popallspace = alloc_code(POPALLSPACE_SIZE)) == NULL) {
            jit_log("WARNING: Could not allocate popallspace!");
            if (currprefs.cachesize > 0)
            {
                jit_abort("Could not allocate popallspace!");
            }
            /* This is not fatal if JIT is not used. If JIT is
             * turned on, it will crash, but it would have crashed
             * anyway. */
            return;
        }
    }
    vm_protect(popallspace, POPALLSPACE_SIZE, VM_PAGE_READ | VM_PAGE_WRITE);

    current_compile_p = popallspace;
    set_target(current_compile_p);

#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
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
    current_compile_p = get_target();
    pushall_call_handler = get_target();
    raw_push_regs_to_preserve();
#ifdef JIT_DEBUG
    write_log("Address of regs: 0x%016x, regs.pc_p: 0x%016x\n", &regs, &regs.pc_p);
    write_log("Address of natmem_offset: 0x%016x, natmem_offset = 0x%016x\n", &regs.natmem_offset, regs.natmem_offset);
    write_log("Address of cache_tags: 0x%016x\n", cache_tags);
#endif
    compemu_raw_init_r_regstruct((uintptr)&regs);
    compemu_raw_jmp_pc_tag();

    /* now the exit points */
    popall_execute_normal_setpc = get_target();
    uintptr idx = (uintptr) & (regs.pc_p) - (uintptr)&regs;
#if defined(CPU_AARCH64)
    STR_xXi(REG_WORK1, R_REGSTRUCT, idx);
#else
    STR_rRI(REG_WORK1, R_REGSTRUCT, idx);
#endif
    popall_execute_normal = get_target();
    raw_pop_preserved_regs();
    compemu_raw_jmp((uintptr)execute_normal);

    popall_check_checksum_setpc = get_target();
#if defined(CPU_AARCH64)
    STR_xXi(REG_WORK1, R_REGSTRUCT, idx);
#else
    STR_rRI(REG_WORK1, R_REGSTRUCT, idx);
#endif
    popall_check_checksum = get_target();
    raw_pop_preserved_regs();
    compemu_raw_jmp((uintptr)check_checksum);

    popall_exec_nostats_setpc = get_target();
#if defined(CPU_AARCH64)
    STR_xXi(REG_WORK1, R_REGSTRUCT, idx);
#else
    STR_rRI(REG_WORK1, R_REGSTRUCT, idx);
#endif
    raw_pop_preserved_regs();
    compemu_raw_jmp((uintptr)exec_nostats);

    popall_recompile_block = get_target();
    raw_pop_preserved_regs();
    compemu_raw_jmp((uintptr)recompile_block);

    popall_do_nothing = get_target();
    raw_pop_preserved_regs();
    compemu_raw_jmp((uintptr)do_nothing);

    popall_cache_miss = get_target();
    raw_pop_preserved_regs();
    compemu_raw_jmp((uintptr)cache_miss);

    popall_execute_exception = get_target();
    raw_pop_preserved_regs();
    compemu_raw_jmp((uintptr)execute_exception);

#if defined(USE_DATA_BUFFER)
    reset_data_buffer();
#endif

    // no need to further write into popallspace
	vm_protect(popallspace, POPALLSPACE_SIZE, VM_PAGE_READ | VM_PAGE_EXECUTE);
    // No need to flush. Initialized and not modified
    // flush_cpu_icache((void *)popallspace, (void *)target);
}

static inline void reset_lists(void)
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
    bi->direct_pen = (cpuop_func*)get_target();
    compemu_raw_execute_normal((uintptr) & (bi->pc_p));

    bi->direct_pcc = (cpuop_func*)get_target();
    compemu_raw_check_checksum((uintptr) & (bi->pc_p));

    flush_cpu_icache((void*)current_compile_p, (void*)target);
    current_compile_p = get_target();

    bi->deplist = NULL;
    for (i = 0; i < 2; i++) {
        bi->dep[i].prev_p = NULL;
        bi->dep[i].next = NULL;
    }
    bi->status = BI_INVALID;
}

void compemu_reset(void)
{
    flush_icache = lazy_flush ? flush_icache_lazy : flush_icache_hard;
    set_cache_state(0);
}

// OPCODE is in big endian format
static inline void reset_compop(int opcode)
{
    compfunctbl[opcode] = NULL;
    nfcompfunctbl[opcode] = NULL;
}

void build_comp(void)
{
    int i, j;
    unsigned long opcode;
    const struct comptbl* tbl = op_smalltbl_0_comp_ff;
    const struct comptbl* nftbl = op_smalltbl_0_comp_nf;
    unsigned int cpu_level = (currprefs.cpu_model - 68000) / 10;
    if (cpu_level > 4)
        cpu_level--;
#ifdef NOFLAGS_SUPPORT_GENCOMP
    const struct cputbl* nfctbl = uaegetjitcputbl();
#endif

    regs.mem_banks = (uintptr)mem_banks;
    regs.cache_tags = (uintptr)cache_tags;

    jit_log("<JIT compiler> : building compiler function tables");

    for (opcode = 0; opcode < 65536; opcode++) {
        reset_compop(opcode);
#ifdef NOFLAGS_SUPPORT_GENCOMP
        nfcpufunctbl[opcode] = op_illg;
#endif
        prop[opcode].use_flags = FLAG_ALL;
        prop[opcode].set_flags = FLAG_ALL;
        prop[opcode].cflow = fl_trap; // ILLEGAL instructions do trap
    }

    for (i = 0; tbl[i].opcode < 65536; i++) {
        int cflow = table68k[tbl[i].opcode].cflow;
        if (follow_const_jumps && (tbl[i].specific & COMP_OPCODE_ISCJUMP))
            cflow = fl_const_jump;
        else
            cflow &= ~fl_const_jump;
        prop[tbl[i].opcode].cflow = cflow;

        bool uses_fpu = (tbl[i].specific & COMP_OPCODE_USES_FPU) != 0;
        if (uses_fpu && avoid_fpu)
            compfunctbl[tbl[i].opcode] = NULL;
        else
            compfunctbl[tbl[i].opcode] = tbl[i].handler;
    }

    for (i = 0; nftbl[i].opcode < 65536; i++) {
		bool uses_fpu = (tbl[i].specific & COMP_OPCODE_USES_FPU) != 0;
        if (uses_fpu && avoid_fpu)
            nfcompfunctbl[nftbl[i].opcode] = NULL;
        else
            nfcompfunctbl[nftbl[i].opcode] = nftbl[i].handler;
        if (!nfctbl[j].handler_ff && currprefs.cachesize) {
            int mnemo = table68k[nftbl[i].opcode].mnemo;
            struct mnemolookup* lookup;
            for (lookup = lookuptab; lookup->mnemo != mnemo; lookup++)
                ;
            char* s = ua(lookup->name);
            jit_log("%04x (%s) unavailable", nftbl[i].opcode, s);
            xfree(s);
        }
    }

    for (i = 0; nfctbl[i].handler_ff; i++) {
        nfcpufunctbl[cft_map(nfctbl[i].opcode)] = nfctbl[i].handler_ff;
    }

    for (opcode = 0; opcode < 65536; opcode++) {
        compop_func* f;
        compop_func* nff;
#ifdef NOFLAGS_SUPPORT_GENCOMP
        cpuop_func* nfcf;
#endif
        int isaddx, cflow;

        if (table68k[opcode].mnemo == i_ILLG || table68k[opcode].clev > cpu_level)
            continue;

        if (table68k[opcode].handler != -1) {
            f = compfunctbl[cft_map(table68k[opcode].handler)];
            nff = nfcompfunctbl[cft_map(table68k[opcode].handler)];
#ifdef NOFLAGS_SUPPORT_GENCOMP
            nfcf = nfcpufunctbl[cft_map(table68k[opcode].handler)];
#endif
            isaddx = prop[cft_map(table68k[opcode].handler)].is_addx;
            prop[cft_map(opcode)].is_addx = isaddx;
            cflow = prop[cft_map(table68k[opcode].handler)].cflow;
            prop[cft_map(opcode)].cflow = cflow;
            compfunctbl[cft_map(opcode)] = f;
            nfcompfunctbl[cft_map(opcode)] = nff;
#ifdef NOFLAGS_SUPPORT_GENCOMP
            Dif(nfcf == op_illg)
                abort();
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

    for (i = 0; nfctbl[i].handler_ff != NULL; i++) {
        if (nfctbl[i].specific)
            nfcpufunctbl[cft_map(tbl[i].opcode)] = nfctbl[i].handler_ff;
    }

    int count = 0;
    for (opcode = 0; opcode < 65536; opcode++) {
        if (compfunctbl[cft_map(opcode)])
            count++;
    }
    jit_log("<JIT compiler> : supposedly %d compileable opcodes!", count);

    /* Initialise state */
    create_popalls();
    alloc_cache();
    reset_lists();

    for (i = 0; i < TAGSIZE; i += 2) {
        cache_tags[i].handler = (cpuop_func*)popall_execute_normal;
        cache_tags[i + 1].bi = NULL;
    }
    compemu_reset();
}

static void flush_icache_none(int v)
{
    /* Nothing to do.  */
}

void flush_icache_hard(int n)
{
    blockinfo* bi, * dbi;

    bi = active;
    while (bi) {
        cache_tags[cacheline(bi->pc_p)].handler = (cpuop_func*)popall_execute_normal;
        cache_tags[cacheline(bi->pc_p) + 1].bi = NULL;
        dbi = bi;
        bi = bi->next;
        free_blockinfo(dbi);
    }
    bi = dormant;
    while (bi) {
        cache_tags[cacheline(bi->pc_p)].handler = (cpuop_func*)popall_execute_normal;
        cache_tags[cacheline(bi->pc_p) + 1].bi = NULL;
        dbi = bi;
        bi = bi->next;
        free_blockinfo(dbi);
    }

    reset_lists();
    if (!compiled_code)
        return;

#if defined(USE_DATA_BUFFER)
    reset_data_buffer();
#endif

    current_compile_p = compiled_code;
    set_special(0); /* To get out of compiled code */
}

/* "Soft flushing" --- instead of actually throwing everything away,
we simply mark everything as "needs to be checked".
*/

static inline void flush_icache_lazy(int v)
{
    blockinfo* bi;
    blockinfo* bi2;

    if (!active)
        return;

    bi = active;
    while (bi) {
        uae_u32 cl = cacheline(bi->pc_p);
        if (bi->status == BI_INVALID || bi->status == BI_NEED_RECOMP) {
            if (bi == cache_tags[cl + 1].bi)
                cache_tags[cl].handler = (cpuop_func*)popall_execute_normal;
            bi->handler_to_use = (cpuop_func*)popall_execute_normal;
            set_dhtu(bi, bi->direct_pen);
            bi->status = BI_INVALID;
        } else {
            if (bi == cache_tags[cl + 1].bi)
                cache_tags[cl].handler = (cpuop_func*)popall_check_checksum;
            bi->handler_to_use = (cpuop_func*)popall_check_checksum;
            set_dhtu(bi, bi->direct_pcc);
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

static inline unsigned int get_opcode_cft_map(unsigned int f)
{
    return uae_bswap_16(f);
}
#define DO_GET_OPCODE(a) (get_opcode_cft_map((uae_u16)*(a)))

void compile_block(cpu_history* pc_hist, int blocklen, int totcycles)
{
    if (cache_enabled && compiled_code && currprefs.cpu_model >= 68020) {
#ifdef PROFILE_COMPILE_TIME
        compile_count++;
        clock_t start_time = clock();
#endif

        /* OK, here we need to 'compile' a block */
        int i;
        int r;
        int was_comp = 0;
        uae_u8 liveflags[MAXRUN + 1];
        bool trace_in_rom = isinrom((uintptr)pc_hist[0].location) != 0;
        uintptr max_pcp = (uintptr)pc_hist[blocklen - 1].location;
        uintptr min_pcp = max_pcp;
        uae_u32 cl = cacheline(pc_hist[0].location);
		void* specflags=(void*)&regs.spcflags;
        blockinfo* bi = NULL;
        blockinfo* bi2;
		int extra_len=0;

        redo_current_block = 0;
        if (current_compile_p >= MAX_COMPILE_PTR)
            flush_icache_hard(3);

        alloc_blockinfos();

        bi = get_blockinfo_addr_new(pc_hist[0].location);
        bi2 = get_blockinfo(cl);

        optlev = bi->optlevel;
        if (bi->status != BI_INVALID) {
            Dif(bi != bi2) {
                /* I don't think it can happen anymore. Shouldn't, in
                   any case. So let's make sure... */
                jit_abort("WOOOWOO count=%d, ol=%d %p %p", bi->count, bi->optlevel, bi->handler_to_use, cache_tags[cl].handler);
            }

            Dif(bi->count != -1 && bi->status != BI_NEED_RECOMP) {
                jit_abort("bi->count=%d, bi->status=%d,bi->optlevel=%d", bi->count, bi->status, bi->optlevel);
                /* What the heck? We are not supposed to be here! */
            }
        }
        if (bi->count == -1) {
            optlev++;
            while (!optcount[optlev])
                optlev++;
            bi->count = optcount[optlev] - 1;
        }
        current_block_pc_p = JITPTR pc_hist[0].location;

        remove_deps(bi); /* We are about to create new code */
        bi->optlevel = optlev;
        bi->pc_p = (uae_u8*)pc_hist[0].location;
        free_checksum_info_chain(bi->csi);
        bi->csi = NULL;

        liveflags[blocklen] = FLAG_ALL; /* All flags needed afterwards */
        i = blocklen;
        while (i--) {
            uae_u16* currpcp = pc_hist[i].location;
            uae_u32 op = DO_GET_OPCODE(currpcp);

            trace_in_rom = trace_in_rom && isinrom((uintptr)currpcp);
            if (follow_const_jumps && is_const_jump(op)) {
                checksum_info* csi = alloc_checksum_info();
                csi->start_p = (uae_u8*)min_pcp;
                csi->length = JITPTR max_pcp - JITPTR min_pcp + LONGEST_68K_INST;
                csi->next = bi->csi;
                bi->csi = csi;
                max_pcp = (uintptr)currpcp;
            }
            min_pcp = (uintptr)currpcp;

            if (!currprefs.compnf) {
                liveflags[i] = FLAG_ALL;
            }
            else
            {
                liveflags[i] = ((liveflags[i + 1] & (~prop[op].set_flags)) | prop[op].use_flags);
                if (prop[op].is_addx && (liveflags[i + 1] & FLAG_Z) == 0)
                    liveflags[i] &= ~FLAG_Z;
            }
        }

        checksum_info* csi = alloc_checksum_info();
        csi->start_p = (uae_u8*)min_pcp;
        csi->length = max_pcp - min_pcp + LONGEST_68K_INST;
        csi->next = bi->csi;
        bi->csi = csi;

        bi->needed_flags = liveflags[0];

        /* This is the non-direct handler */
        was_comp = 0;

        bi->direct_handler = (cpuop_func*)get_target();
        set_dhtu(bi, bi->direct_handler);
        bi->status = BI_COMPILING;
        current_block_start_target = (uintptr)get_target();

        if (bi->count >= 0) { /* Need to generate countdown code */
            compemu_raw_set_pc_i((uintptr)pc_hist[0].location);
            compemu_raw_dec_m((uintptr) & (bi->count));
            compemu_raw_maybe_recompile();
        }
        if (optlev == 0) { /* No need to actually translate */
          /* Execute normally without keeping stats */
            compemu_raw_exec_nostats((uintptr)pc_hist[0].location);
        } else {
            reg_alloc_run = 0;
            next_pc_p = 0;
            taken_pc_p = 0;
            branch_cc = 0; // Only to be initialized. Will be set together with next_pc_p

            comp_pc_p = (uae_u8*)pc_hist[0].location;
            init_comp();
            was_comp = 1;

            for (i = 0; i < blocklen && get_target() < MAX_COMPILE_PTR; i++) {
                may_raise_exception = false;
                cpuop_func** cputbl;
                compop_func** comptbl;
                uae_u32 opcode = DO_GET_OPCODE(pc_hist[i].location);
                needed_flags = (liveflags[i + 1] & prop[opcode].set_flags);
                special_mem = pc_hist[i].specmem;
                if (!needed_flags && currprefs.compnf) {
#ifdef NOFLAGS_SUPPORT_GENCOMP
                    cputbl = nfcpufunctbl;
#else
                    cputbl = cpufunctbl;
#endif
                    comptbl = nfcompfunctbl;
                } else {
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
                    if (!(liveflags[i + 1] & FLAG_CZNV)) {
                        /* We can forget about flags */
                        dont_care_flags();
                    }
                }

                if (failure) {
                    if (was_comp) {
                        flush(1);
                        was_comp = 0;
                    }
                    compemu_raw_mov_l_ri(REG_PAR1, (uae_u32)opcode);
                    compemu_raw_mov_l_rr(REG_PAR2, R_REGSTRUCT);
                    compemu_raw_set_pc_i((uintptr)pc_hist[i].location);
                    compemu_raw_call((uintptr)cputbl[opcode]);
#ifdef PROFILE_UNTRANSLATED_INSNS
                    // raw_cputbl_count[] is indexed with plain opcode (in m68k order)
                    compemu_raw_inc_opcount(opcode);
#endif

                    if (i < blocklen - 1) {
                        uae_u8* branchadd;

                        /* if (SPCFLAGS_TEST(SPCFLAG_ALL)) popall_do_nothing() */
                        compemu_raw_mov_l_rm(0, (uintptr)specflags);
#if defined(USE_DATA_BUFFER)
                        data_check_end(8, 64);
#endif
                        compemu_raw_maybe_do_nothing(scaled_cycles(totcycles));
                    }
                } else if (may_raise_exception) {
#if defined(USE_DATA_BUFFER)
                    data_check_end(8, 64);
#endif
                    compemu_raw_handle_except(scaled_cycles(totcycles));
                    may_raise_exception = false;
                }
            }
#if 1 /* This isn't completely kosher yet; It really needs to be
         integrated into a general inter-block-dependency scheme */
            if (next_pc_p && taken_pc_p &&
                was_comp && taken_pc_p == current_block_pc_p)
            {
                blockinfo* bi1 = get_blockinfo_addr_new((void*)next_pc_p);
                blockinfo* bi2 = get_blockinfo_addr_new((void*)taken_pc_p);
                uae_u8 x = bi1->needed_flags;

                if (x == 0xff || 1) {  /* To be on the safe side */
                    uae_u16* next = (uae_u16*)next_pc_p;
                    uae_u32 op = DO_GET_OPCODE(next);

                    x = FLAG_ALL;
                    x &= (~prop[op].set_flags);
                    x |= prop[op].use_flags;
                }

                x |= bi2->needed_flags;
                if (!(x & FLAG_CZNV)) {
                    /* We can forget about flags */
                    dont_care_flags();
                    extra_len += 2; /* The next instruction now is part of this block */
                }
            }
#endif
            log_flush();

            if (next_pc_p) { /* A branch was registered */
                uintptr t1 = next_pc_p;
                uintptr t2 = taken_pc_p;
                int cc = branch_cc; // this is native (ARM) condition code

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
                    if (cc < NATIVE_CC_AL)
                        cc = branch_cc ^ 1;
                    else if (cc > NATIVE_CC_AL)
                        cc = 0x10 | (branch_cc ^ 0xf);
                }

#if defined(USE_DATA_BUFFER)
                data_check_end(8, 128);
#endif
                flush(1);                       // Emitted code of this call doesn't modify flags
                compemu_raw_jcc_l_oponly(cc);   // Last emitted opcode is branch to target
                branchadd = (uae_u32*)get_target() - 1;

                /* predicted outcome */
                tbi = get_blockinfo_addr_new((void*)t1);
                match_states(tbi);

                tba = compemu_raw_endblock_pc_isconst(scaled_cycles(totcycles), t1);
                write_jmp_target(tba, get_handler(t1));
                create_jmpdep(bi, 0, tba, t1);

                /* not-predicted outcome */
                write_jmp_target(branchadd, (uintptr)get_target());
                tbi = get_blockinfo_addr_new((void*)t2);
                match_states(tbi);

                tba = compemu_raw_endblock_pc_isconst(scaled_cycles(totcycles), t2);
                write_jmp_target(tba, get_handler(t2));
                create_jmpdep(bi, 1, tba, t2);
            } else {
                if (was_comp) {
                    flush(1);
                }

                /* Let's find out where next_handler is... */
                if (was_comp && isinreg(PC_P)) {
#if defined(USE_DATA_BUFFER)
                    data_check_end(4, 64);
#endif
                    r = live.state[PC_P].realreg;
                    compemu_raw_endblock_pc_inreg(r, scaled_cycles(totcycles));
                } else if (was_comp && isconst(PC_P)) {
                    uintptr v = live.state[PC_P].val;
                    uae_u32* tba;
                    blockinfo* tbi;

                    tbi = get_blockinfo_addr_new((void*)v);
                    match_states(tbi);

#if defined(USE_DATA_BUFFER)
                    data_check_end(4, 64);
#endif
                    tba = compemu_raw_endblock_pc_isconst(scaled_cycles(totcycles), v);
                    write_jmp_target(tba, get_handler(v));
                    create_jmpdep(bi, 0, tba, v);
                } else {
                    r = REG_PC_TMP;
                    compemu_raw_mov_l_rm(r, (uintptr)&regs.pc_p);
#if defined(USE_DATA_BUFFER)
                    data_check_end(4, 64);
#endif
                    compemu_raw_endblock_pc_inreg(r, scaled_cycles(totcycles));
                }
            }
        }

        remove_from_list(bi);
        if (trace_in_rom) {
            // No need to checksum that block trace on cache invalidation
            free_checksum_info_chain(bi->csi);
            bi->csi = NULL;
            add_to_dormant(bi);
        } else {
            calc_checksum(bi, &(bi->c1), &(bi->c2));
            add_to_active(bi);
        }

        current_cache_size += JITPTR get_target() - JITPTR current_compile_p;

        /* This is the non-direct handler */
        bi->handler = bi->handler_to_use = (cpuop_func*)get_target();
        compemu_raw_cmp_pc((uintptr)pc_hist[0].location);
        compemu_raw_maybe_cachemiss();
        comp_pc_p = (uae_u8*)pc_hist[0].location;

        bi->status = BI_FINALIZING;
        init_comp();
        match_states(bi);
        flush(1);

        compemu_raw_jmp((uintptr)bi->direct_handler);

        flush_cpu_icache((void*)current_block_start_target, (void*)target);
        current_compile_p = get_target();
        raise_in_cl_list(bi);
        bi->nexthandler = current_compile_p;

        /* We will flush soon, anyway, so let's do it now */
        if (current_compile_p >= MAX_COMPILE_PTR)
            flush_icache_hard(3);

        bi->status = BI_ACTIVE;
        if (redo_current_block)
            block_need_recompile(bi);

#ifdef PROFILE_COMPILE_TIME
        compile_time += (clock() - start_time);
#endif
        /* Account for compilation time */
        do_extra_cycles(totcycles);
    }
}

#endif /* JIT */
