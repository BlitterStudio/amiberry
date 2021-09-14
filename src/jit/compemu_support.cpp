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

#include <math.h>

#include "sysdeps.h"

#if defined(JIT)

#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "comptbl.h"
#include "compemu.h"
#include <SDL.h>


#if DEBUG
#define PROFILE_COMPILE_TIME        1
#endif
//#define PROFILE_UNTRANSLATED_INSNS    1

#if defined(CPU_AARCH64)
#define PRINT_PTR "%016llx"
#else
#define PRINT_PTR "%08x"
#endif

#define jit_log(format, ...) \
  write_log("JIT: " format "\n", ##__VA_ARGS__);

#ifdef JIT_DEBUG
#undef abort
#define abort() do { \
  fprintf(stderr, "Abort in file %s at line %d\n", __FILE__, __LINE__); \
  SDL_Quit();  \
  exit(EXIT_FAILURE); \
} while (0)
#endif

#ifdef PROFILE_COMPILE_TIME
#include <time.h>
static uae_u32 compile_count    = 0;
static clock_t compile_time     = 0;
static clock_t emul_start_time  = 0;
static clock_t emul_end_time    = 0;
#endif

#ifdef PROFILE_UNTRANSLATED_INSNS
static int untranslated_top_ten = 20;
static uae_u32 raw_cputbl_count[65536] = { 0, };
static uae_u16 opcode_nums[65536];


static int untranslated_compfn(const void *e1, const void *e2)
{
    return raw_cputbl_count[*(const uae_u16*)e1] < raw_cputbl_count[*(const uae_u16*)e2];
    int v1 = *(const uae_u16*)e1;
    int v2 = *(const uae_u16*)e2;
    return (int)raw_cputbl_count[v2] - (int)raw_cputbl_count[v1];
}
#endif

static compop_func *compfunctbl[65536];
static compop_func *nfcompfunctbl[65536];
uae_u8* comp_pc_p;

// gb-- Extra data for Basilisk II/JIT
#define follow_const_jumps (true)

static uae_u32 cache_size = 0;            // Size of total cache allocated for compiled blocks
static uae_u32 current_cache_size   = 0;  // Cache grows upwards: how much has been consumed already
#ifdef USE_JIT_FPU
#define avoid_fpu (!currprefs.compfpu)
#else
#define avoid_fpu (true)
#endif
static const int optcount   = 4;          // How often a block has to be executed before it is translated

op_properties prop[65536];

bool may_raise_exception = false;
static bool flags_carry_inverted = false;
static bool disasm_this = false;


STATIC_INLINE bool is_const_jump(uae_u32 opcode)
{
    return (prop[opcode].cflow == fl_const_jump);
}

uae_u8* start_pc_p;
uae_u32 start_pc;
static uintptr current_block_start_target;
uae_u32 needed_flags;
static uintptr next_pc_p;
static uintptr taken_pc_p;
static int branch_cc;

uae_u8* current_compile_p = NULL;
static uae_u8* max_compile_start;
uae_u8* compiled_code = NULL;
const int POPALLSPACE_SIZE = 2048; /* That should be enough space */
uae_u8* popallspace = NULL;

void* pushall_call_handler = NULL;
static void* popall_execute_normal = NULL;
static void* popall_check_checksum = NULL;
static void* popall_execute_normal_setpc = NULL;
static void* popall_check_checksum_setpc = NULL;
static void* popall_exec_nostats_setpc = NULL;
static void* popall_recompile_block = NULL;
static void* popall_do_nothing = NULL;
static void* popall_cache_miss = NULL;
static void* popall_execute_exception = NULL;

/* The 68k only ever executes from even addresses. So right now, we
 * waste half the entries in this array
 * UPDATE: We now use those entries to store the start of the linked
 * lists that we maintain for each hash result.
 */
static cacheline cache_tags[TAGSIZE];
static int cache_enabled=0;
static blockinfo* hold_bi[MAX_HOLD_BI];
blockinfo* active;
blockinfo* dormant;

#if !defined (WIN32) || !defined(ANDROID)
#include <sys/mman.h>

static void cache_free (uae_u8 *cache, int size)
{
  munmap(cache, size);
}

static uae_u8 *cache_alloc (int size)
{
  size = size < getpagesize() ? getpagesize() : size;

  void *cache = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (!cache) {
    printf ("Cache_Alloc of %d failed. ERR=%d\n", size, errno);
  } else {
    memset(cache, 0, size);
  }
  return (uae_u8 *) cache;
}

#endif

extern const struct comptbl op_smalltbl_0_comp_nf[];
extern const struct comptbl op_smalltbl_0_comp_ff[];

static bigstate live;

static int writereg(int r);
static void unlock2(int r);
static void setlock(int r);
static int readreg(int r);
static void prepare_for_call_1(void);
static void prepare_for_call_2(void);

STATIC_INLINE void flush_cpu_icache(void *from, void *to);
STATIC_INLINE void write_jmp_target(uae_u32 *jmpaddr, uintptr a);

uae_u32 m68k_pc_offset;

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

STATIC_INLINE blockinfo* get_blockinfo(uae_u32 cl)
{
    return cache_tags[cl + 1].bi;
}

STATIC_INLINE blockinfo* get_blockinfo_addr(void* addr)
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

STATIC_INLINE void remove_from_cl_list(blockinfo* bi)
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

STATIC_INLINE void remove_from_list(blockinfo* bi)
{
    if (bi->prev_p)
        *(bi->prev_p) = bi->next;
    if (bi->next)
        bi->next->prev_p = bi->prev_p;
}

STATIC_INLINE void add_to_cl_list(blockinfo* bi)
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
    write_jmp_target(d->jmp_off, (uintptr)a);
}

/********************************************************************
 * Soft flush handling support functions                            *
 ********************************************************************/

STATIC_INLINE void set_dhtu(blockinfo* bi, cpuop_func* dh)
{
    if (dh != bi->direct_handler_to_use) {
        dependency* x = bi->deplist;
        while (x) {
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
    bi->count = optcount - 1;
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

STATIC_INLINE void create_jmpdep(blockinfo* bi, int i, uae_u32* jmpaddr, uae_u32 target)
{
    blockinfo* tbi = get_blockinfo_addr((void*)(uintptr)target);

    bi->dep[i].jmp_off = jmpaddr;
    bi->dep[i].source = bi;
    bi->dep[i].target = tbi;
    bi->dep[i].next = tbi->deplist;
    if (bi->dep[i].next)
        bi->dep[i].next->prev_p = &(bi->dep[i].next);
    bi->dep[i].prev_p = &(tbi->deplist);
    tbi->deplist = &(bi->dep[i]);
}

STATIC_INLINE blockinfo* get_blockinfo_addr_new(void* addr)
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
    ~LazyBlockAllocator();
    T* acquire();
    void release(T* const);
};

template< class T >
LazyBlockAllocator<T>::~LazyBlockAllocator()
{
    Pool* currentPool = mPools;
    while (currentPool) {
        Pool* deadPool = currentPool;
        currentPool = currentPool->next;
        free(deadPool);
    }
}

template< class T >
T* LazyBlockAllocator<T>::acquire()
{
    if (!mChunks) {
        // There is no chunk left, allocate a new pool and link the
        // chunks into the free list
        Pool* newPool = (Pool*)malloc(sizeof(Pool));
        if (newPool == NULL) {
            jit_abort(_T("FATAL: Could not allocate block pool!\n"));
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


static LazyBlockAllocator<blockinfo> BlockInfoAllocator;
static LazyBlockAllocator<checksum_info> ChecksumInfoAllocator;

STATIC_INLINE checksum_info* alloc_checksum_info(void)
{
    checksum_info* csi = ChecksumInfoAllocator.acquire();
    csi->next = NULL;
    return csi;
}

STATIC_INLINE void free_checksum_info(checksum_info* csi)
{
    csi->next = NULL;
    ChecksumInfoAllocator.release(csi);
}

STATIC_INLINE void free_checksum_info_chain(checksum_info* csi)
{
    while (csi != NULL) {
        checksum_info* csi2 = csi->next;
        free_checksum_info(csi);
        csi = csi2;
    }
}

STATIC_INLINE blockinfo* alloc_blockinfo(void)
{
    blockinfo* bi = BlockInfoAllocator.acquire();
    bi->csi = NULL;
    return bi;
}

STATIC_INLINE void free_blockinfo(blockinfo* bi)
{
    free_checksum_info_chain(bi->csi);
    bi->csi = NULL;
    BlockInfoAllocator.release(bi);
}

STATIC_INLINE void alloc_blockinfos(void)
{
    int i;
    blockinfo* bi;

    for (i = 0; i < MAX_HOLD_BI; i++) {
        if (hold_bi[i])
            return;
        bi = hold_bi[i] = alloc_blockinfo();
        prepare_block(bi);
    }
}

bool check_prefs_changed_comp(bool checkonly)
{
    bool changed = 0;
    static int cachesize_prev, comptrust_prev;
    static bool canbang_prev;

    if (currprefs.comptrustbyte != changed_prefs.comptrustbyte ||
        currprefs.comptrustword != changed_prefs.comptrustword ||
        currprefs.comptrustlong != changed_prefs.comptrustlong ||
        currprefs.comptrustnaddr != changed_prefs.comptrustnaddr ||
        currprefs.compnf != changed_prefs.compnf ||
        currprefs.comp_hardflush != changed_prefs.comp_hardflush ||
        currprefs.comp_constjump != changed_prefs.comp_constjump ||
        currprefs.compfpu != changed_prefs.compfpu ||
        currprefs.fpu_strict != changed_prefs.fpu_strict ||
        currprefs.cachesize != changed_prefs.cachesize)
        changed = 1;

    if (checkonly)
        return changed;

    currprefs.comptrustbyte = changed_prefs.comptrustbyte;
    currprefs.comptrustword = changed_prefs.comptrustword;
    currprefs.comptrustlong = changed_prefs.comptrustlong;
    currprefs.comptrustnaddr = changed_prefs.comptrustnaddr;
    currprefs.compnf = changed_prefs.compnf;
    currprefs.comp_hardflush = changed_prefs.comp_hardflush;
    currprefs.comp_constjump = changed_prefs.comp_constjump;
    currprefs.compfpu = changed_prefs.compfpu;
    currprefs.fpu_strict = changed_prefs.fpu_strict;

    if (currprefs.cachesize != changed_prefs.cachesize) {
        if (currprefs.cachesize && !changed_prefs.cachesize) {
            cachesize_prev = currprefs.cachesize;
            comptrust_prev = currprefs.comptrustbyte;
            canbang_prev = canbang;
        }
        else if (!currprefs.cachesize && changed_prefs.cachesize == cachesize_prev) {
            changed_prefs.comptrustbyte = currprefs.comptrustbyte = comptrust_prev;
            changed_prefs.comptrustword = currprefs.comptrustword = comptrust_prev;
            changed_prefs.comptrustlong = currprefs.comptrustlong = comptrust_prev;
            changed_prefs.comptrustnaddr = currprefs.comptrustnaddr = comptrust_prev;
        }
        currprefs.cachesize = changed_prefs.cachesize;
        if (currprefs.cachesize && currprefs.fast_copper)
            chipmem_bank.jit_write_flag = S_WRITE;
        else
            chipmem_bank.jit_write_flag = 0;
        alloc_cache();
        changed = 1;
    }

    // Turn off illegal-mem logging when using JIT...
    if (currprefs.cachesize)
        currprefs.illegal_mem = changed_prefs.illegal_mem;// = 0;

    if ((!canbang || !currprefs.cachesize) && currprefs.comptrustbyte != 1) {
        // Set all of these to indirect when canbang == 0
        currprefs.comptrustbyte = 1;
        currprefs.comptrustword = 1;
        currprefs.comptrustlong = 1;
        currprefs.comptrustnaddr = 1;

        changed_prefs.comptrustbyte = 1;
        changed_prefs.comptrustword = 1;
        changed_prefs.comptrustlong = 1;
        changed_prefs.comptrustnaddr = 1;

        changed = 1;

        if (currprefs.cachesize)
            write_log(_T("JIT: Reverting to \"indirect\" access, because canbang is zero!\n"));
    }

    if (changed)
        write_log(_T("JIT: cache=%d. b=%d w=%d l=%d fpu=%d nf=%d inline=%d hard=%d\n"),
            currprefs.cachesize,
            currprefs.comptrustbyte, currprefs.comptrustword, currprefs.comptrustlong,
            currprefs.compfpu, currprefs.compnf, currprefs.comp_constjump, currprefs.comp_hardflush);

    return changed;
}

/********************************************************************
 * Functions to emit data into memory, and other general support    *
 ********************************************************************/

static uae_u8* target;

STATIC_INLINE void emit_long(uae_u32 x)
{
    *(uae_u32*)target = x;
    target += 4;
}

STATIC_INLINE void emit_longlong(uae_u64 x)
{
    *(uae_u64*)target = x;
    target += 8;
}

#define MAX_COMPILE_PTR max_compile_start

static void set_target(uae_u8* t)
{
    target = t;
}

STATIC_INLINE uae_u8* get_target(void)
{
    return target;
}

/********************************************************************
 * New version of data buffer: interleave data and code             *
 ********************************************************************/
#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)

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
STATIC_INLINE void clobber_flags(void);

#if defined(CPU_AARCH64) 
#include "codegen_armA64.cpp.in"
#elif defined(CPU_arm) 
#include "codegen_arm.cpp.in"
#endif
#if defined(CPU_i386) || defined(CPU_x86_64)
#include "codegen_x86.cpp"
#endif


/********************************************************************
 * Flags status handling. EMIT TIME!                                *
 ********************************************************************/

static void make_flags_live_internal(void)
{
    if (live.flags_in_flags == VALID)
        return;
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

    int tmp = writereg(FLAGTMP);
    raw_flags_to_reg(tmp);
    unlock2(tmp);

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

static int touchcnt;

/********************************************************************
 * register allocation per block logging                            *
 ********************************************************************/

STATIC_INLINE void do_load_reg(int n, int r)
{
    compemu_raw_mov_l_rm(n, (uintptr)live.state[r].mem);
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
    if (live.state[r].status == DIRTY) {
        compemu_raw_mov_l_mr((uintptr)live.state[r].mem, live.state[r].realreg);
        set_status(r, CLEAN);
    }
}

STATIC_INLINE int isconst(int r)
{
    return live.state[r].status == ISCONST;
}

STATIC_INLINE void writeback_const(int r)
{
    if (!isconst(r))
        return;

    compemu_raw_mov_l_mi((uintptr)live.state[r].mem, live.state[r].val);
    live.state[r].val = 0;
    set_status(r, INMEM);
}

static void evict(int r)
{
    if (!isinreg(r))
        return;
    tomem(r);
    int rr = live.state[r].realreg;

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
}

/* Use with care! */
STATIC_INLINE void isclean(int r)
{
    if (!isinreg(r))
        return;
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

bool has_free_reg(void)
{
  for (int i = N_REGS - 1; i >= 0; i--) {
    if(!live.nat[i].locked) {
      if (live.nat[i].nholds == 0)
        return true;
    }
  }
  return false;
}

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

    if (live.nat[bestreg].nholds > 0) {
        free_nreg(bestreg);
    }

    if (!willclobber) {
        if (live.state[r].status != UNDEF) {
            if (isconst(r)) {
                compemu_raw_mov_l_ri(bestreg, live.state[r].val);
                live.state[r].val = 0;
                set_status(r, DIRTY);
            } else {
                do_load_reg(bestreg, r);
                set_status(r, CLEAN);
            }
        } else {
            live.state[r].val = 0;
            set_status(r, CLEAN);
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


STATIC_INLINE void make_exclusive(int r, int needcopy)
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

STATIC_INLINE int readreg_general(int r, int spec)
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
#ifdef USE_JIT_FPU

STATIC_INLINE void f_tomem_drop(int r)
{
    if (live.fate[r].status == DIRTY) {
        compemu_raw_fmov_mr_drop((uintptr)live.fate[r].mem, live.fate[r].realreg);
        live.fate[r].status = INMEM;
    }
}


STATIC_INLINE int f_isinreg(int r)
{
    return live.fate[r].status == CLEAN || live.fate[r].status == DIRTY;
}

STATIC_INLINE void f_evict(int r)
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

STATIC_INLINE void f_free_nreg(int r)
{
    int vr = live.fat[r].holds;
    f_evict(vr);
}


/* Use with care! */
STATIC_INLINE void f_isclean(int r)
{
    if (!f_isinreg(r))
        return;
    live.fate[r].status = CLEAN;
}

STATIC_INLINE void f_disassociate(int r)
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

STATIC_INLINE void f_unlock(int r)
{
}

STATIC_INLINE int f_readreg(int r)
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

STATIC_INLINE int f_writereg(int r)
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

#endif

#if defined(CPU_AARCH64) 
#include "compemu_midfunc_armA64.cpp.in"
#include "compemu_midfunc_armA64_2.cpp.in"
#elif defined(CPU_arm)
#include "compemu_midfunc_arm.cpp.in"
#include "compemu_midfunc_arm2.cpp.in"
#endif

#if defined(CPU_i386) || defined(CPU_x86_64)
#include "compemu_midfunc_x86.cpp"
#endif


/********************************************************************
 * Support functions exposed to gencomp. CREATE time                *
 ********************************************************************/

uae_u32 get_const(int r)
{
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

void compiler_exit(void)
{
  //if(current_compile_p != 0 && compiled_code != 0 && current_compile_p > compiled_code)
  //  jit_log("used size: %8d bytes", current_compile_p - compiled_code);

#ifdef PROFILE_COMPILE_TIME
    emul_end_time = clock();
#endif

#if defined(CPU_arm) && !defined(ARMV6T2)
#ifdef DEBUG_DATA_BUFFER
    printf("data_wasted = %d bytes in %d blocks\n", data_wasted, data_buffers_used);
#endif
#endif

    // Deallocate translation cache
    compiled_code = 0;

    // Deallocate popallspace
    if (popallspace) {
        cache_free(popallspace, POPALLSPACE_SIZE + MAX_JIT_CACHE * 1024);
        popallspace = 0;
    }

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
    jit_log("Sorting out untranslated instructions count, total %llu...\n", untranslated_count);
    qsort(opcode_nums, 65536, sizeof(uae_u16), untranslated_compfn);
    jit_log("Rank  Opc      Count Name\n");
    for (int i = 0; i < untranslated_top_ten && i < 65536; i++) {
        uae_u32 count = raw_cputbl_count[opcode_nums[i]];
        struct instr* dp;
        struct mnemolookup* lookup;
        if (!count)
            break;
        dp = table68k + opcode_nums[i];
        for (lookup = lookuptab; lookup->mnemo != (instrmnem)dp->mnemo; lookup++)
            ;
        jit_log(_T("%03d: %04x %10u %s\n"), i, opcode_nums[i], count, lookup->name);
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

    live.state[FLAGX].mem = (uae_u32*)&(regs.ccrflags.x);
    set_status(FLAGX, INMEM);
  
#if defined(CPU_arm)
    live.state[FLAGTMP].mem = (uae_u32*)&(regs.ccrflags.nzcv);
#else
    live.state[FLAGTMP].mem = (uae_u32*)&(regs.ccrflags.cznv);
#endif
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

    flush_flags(); /* low level */
    sync_m68k_pc(); /* mid level */

    if (save_regs) {
#ifdef USE_JIT_FPU
        for (i = 0; i < VFREGS; i++) {
            if (live.fate[i].needflush == NF_SCRATCH || live.fate[i].status == CLEAN) {
                f_disassociate(i);
            }
        }
#endif
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
#ifdef USE_JIT_FPU
        for (i = 0; i <= FP_RESULT; i++) {
            if (live.fate[i].status == DIRTY) {
                f_evict(i);
            }
        }
#endif
    }
}

int alloc_scratch(void)
{
	for(int i = 0; i < SCRATCH_REGS; ++i) {
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
	if(live.scratch_in_use[i - S1]) {
		forget_about(i);
		live.scratch_in_use[i - S1] = 0;
	} else {
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
 * Support functions, internal                                      *
 ********************************************************************/


STATIC_INLINE int isinrom(uintptr addr)
{
    return (addr >= (uintptr)kickmem_bank.baseaddr &&
        addr < (uintptr)(kickmem_bank.baseaddr + 8 * 65536));
}

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

#ifdef USE_JIT_FPU
    if (f_isinreg(FP_RESULT))
        f_evict(FP_RESULT);
    if (f_isinreg(FS1))
        f_evict(FS1);
#endif
}

/* Make sure all registers that will get clobbered by a call are
   save and sound in memory */
static void prepare_for_call_1(void)
{
    flush_all();
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

STATIC_INLINE void writemem_special(int address, int source, int offset)
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

STATIC_INLINE void readmem_special(int address, int dest, int offset)
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

void alloc_cache(void)
{
    if (compiled_code) {
        flush_icache_hard(3);
        compiled_code = 0;
    }

    cache_size = currprefs.cachesize;
    if (cache_size == 0)
        return;

    if (popallspace)
        compiled_code = popallspace + POPALLSPACE_SIZE;

    if (compiled_code) {
        write_log("Actual translation cache size : %d KB at %p-%p\n", cache_size, compiled_code, compiled_code + cache_size * 1024);
#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
        max_compile_start = compiled_code + cache_size * 1024 - BYTES_PER_INST - DATA_BUFFER_SIZE;
#else
        max_compile_start = compiled_code + cache_size * 1024 - BYTES_PER_INST;
#endif
        current_compile_p = compiled_code;
        current_cache_size = 0;
#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
        reset_data_buffer();
#endif
    }
}

static void calc_checksum(blockinfo* bi, uae_u32* c1, uae_u32* c2)
{
    uae_u32 k1 = 0;
    uae_u32 k2 = 0;

    checksum_info* csi = bi->csi;
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

    raise_in_cl_list(bi);
    execute_normal();
}

static void cache_miss(void)
{
    blockinfo* bi = get_blockinfo_addr(regs.pc_p);

    if (!bi) {
        execute_normal(); /* Compile this block now */
        return;
    }
    raise_in_cl_list(bi);
}

static int called_check_checksum(blockinfo* bi);

STATIC_INLINE int block_check_checksum(blockinfo* bi)
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
        remove_from_list(bi);
        add_to_active(bi);
        raise_in_cl_list(bi);
        bi->status = BI_ACTIVE;
    } else {
        /* This block actually changed. We need to invalidate it,
           and set it up to be recompiled */
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
        if ((popallspace = cache_alloc(POPALLSPACE_SIZE + MAX_JIT_CACHE * 1024)) == NULL) {
            jit_log("WARNING: Could not allocate popallspace!");
            /* This is not fatal if JIT is not used. If JIT is
             * turned on, it will crash, but it would have crashed
             * anyway. */
            return;
        }
    }
    write_log("JIT popallspace: %p-%p\n", popallspace, popallspace + POPALLSPACE_SIZE);

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

#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
    reset_data_buffer();
#endif

    // no need to further write into popallspace
    //TODO vm_protect(popallspace, POPALLSPACE_SIZE, VM_PAGE_READ | VM_PAGE_EXECUTE);
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
    set_cache_state(0);
}

// OPCODE is in big endian format
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
    unsigned int cpu_level = (currprefs.cpu_model - 68000) / 10;
    if (cpu_level > 4)
        cpu_level--;

#ifdef PROFILE_UNTRANSLATED_INSNS
    regs.raw_cputbl_count = raw_cputbl_count;
#endif
    regs.mem_banks = (uintptr)mem_banks;
    regs.cache_tags = (uintptr)cache_tags;

    for (opcode = 0; opcode < 65536; opcode++) {
        reset_compop(opcode);
    prop[opcode].use_flags = FLAG_ALL;
    prop[opcode].set_flags = FLAG_ALL;
        prop[opcode].cflow = fl_jump | fl_trap; // ILLEGAL instructions do trap
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
    }

    for (opcode = 0; opcode < 65536; opcode++) {
        compop_func* f;
        compop_func* nff;
        int isaddx, cflow;

        if (table68k[opcode].mnemo == i_ILLG || table68k[opcode].clev > cpu_level)
            continue;

        if (table68k[opcode].handler != -1) {
            f = compfunctbl[table68k[opcode].handler];
            nff = nfcompfunctbl[table68k[opcode].handler];
            isaddx = prop[table68k[opcode].handler].is_addx;
            prop[opcode].is_addx = isaddx;
            cflow = prop[table68k[opcode].handler].cflow;
            prop[opcode].cflow = cflow;
            compfunctbl[opcode] = f;
            nfcompfunctbl[opcode] = nff;
        }
        prop[opcode].set_flags = table68k[opcode].flagdead;
        prop[opcode].use_flags = table68k[opcode].flaglive;
        /* Unconditional jumps don't evaluate condition codes, so they
         * don't actually use any flags themselves */
        if (prop[opcode].cflow & fl_const_jump)
            prop[opcode].use_flags = 0;
    }

#ifdef JIT_DEBUG
    int count = 0;
    for (opcode = 0; opcode < 65536; opcode++) {
        if (compfunctbl[opcode])
            count++;
    }
    jit_log("Supposedly %d compileable opcodes!", count);
#endif

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

void flush_icache_hard(int n)
{
  //if(current_compile_p != 0 && compiled_code != 0 && current_compile_p > compiled_code)
  //  jit_log("used size: %8d bytes", current_compile_p - compiled_code);

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

#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
    reset_data_buffer();
#endif

    current_compile_p = compiled_code;
    set_special(0); /* To get out of compiled code */
}


/* "Soft flushing" --- instead of actually throwing everything away,
   we simply mark everything as "needs to be checked".
*/

void flush_icache(int n)
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

STATIC_INLINE unsigned int get_opcode_cft_map(unsigned int f)
{
    return ((f >> 8) & 255) | ((f & 255) << 8);
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
        blockinfo* bi = NULL;
        blockinfo* bi2;

        if (current_compile_p >= MAX_COMPILE_PTR)
            flush_icache_hard(3);

        alloc_blockinfos();

        bi = get_blockinfo_addr_new(pc_hist[0].location);
        bi2 = get_blockinfo(cl);

        int optlev = bi->optlevel;
        if (bi->count == -1) {
            optlev = 2;
            bi->count = -2;
        }

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
                csi->length = max_pcp - min_pcp + LONGEST_68K_INST;
                csi->next = bi->csi;
                bi->csi = csi;
                max_pcp = (uintptr)currpcp;
            }
            min_pcp = (uintptr)currpcp;

            liveflags[i] = ((liveflags[i + 1] & (~prop[op].set_flags)) | prop[op].use_flags);
            if (prop[op].is_addx && (liveflags[i + 1] & FLAG_Z) == 0)
                liveflags[i] &= ~FLAG_Z;
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
#ifdef JIT_DEBUG
                write_log("    location=0x" PRINT_PTR ", opcode=0x%04x, target=0x" PRINT_PTR ", need_flags=%d\n", pc_hist[i].location, opcode, get_target(), needed_flags);
#endif
                if (!needed_flags) {
                    cputbl = cpufunctbl;
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
#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
                        data_check_end(8, 64);
#endif
                        compemu_raw_maybe_do_nothing(scaled_cycles(totcycles));
                    }
                } else if (may_raise_exception) {
#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
                    data_check_end(8, 64);
#endif
                    compemu_raw_handle_except(scaled_cycles(totcycles));
                    may_raise_exception = false;
                }
            }

            if (next_pc_p) { /* A branch was registered */
                uintptr t1 = next_pc_p;
                uintptr t2 = taken_pc_p;
                int cc = branch_cc; // this is native (ARM) condition code

                uae_u32* branchadd;
                uae_u32* tba;
                bigstate tmp;
                blockinfo* tbi;
#ifdef JIT_DEBUG
                write_log("    branch detected: t1=0x%016llx, t2=0x" PRINT_PTR ", cc=%d\n", t1, t2, cc);
#endif

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

#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
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
#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
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

#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
                    data_check_end(4, 64);
#endif
                    tba = compemu_raw_endblock_pc_isconst(scaled_cycles(totcycles), v);
                    write_jmp_target(tba, get_handler(v));
                    create_jmpdep(bi, 0, tba, v);
                } else {
                    r = REG_PC_TMP;
                    compemu_raw_mov_l_rm(r, (uintptr)&regs.pc_p);
#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)
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

        current_cache_size += get_target() - (uae_u8*)current_compile_p;

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

#ifdef PROFILE_COMPILE_TIME
        compile_time += (clock() - start_time);
#endif
        /* Account for compilation time */
        do_extra_cycles(totcycles);
    }
}

#endif /* JIT */
