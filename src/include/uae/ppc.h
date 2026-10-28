/*
 * UAE PPC emulation API and PPC-plugin ABI.
 *
 * The UAE side (src/ppc/ppc.cpp) resolves the plugin's functions by name
 * with uae_dlsym() and patches its own I/O callbacks into the plugin's
 * exported function-pointer variables, so symbol names, signatures and
 * the PPCMemoryRegion layout below are binary ABI.
 *
 * In a UAE build the uae_ppc_* functions have C++ linkage, and the
 * ppc_cpu_* prototypes are plain C++ declarations (no extern "C").
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UAE_PPC_H
#define UAE_PPC_H

#include <cstddef>
#include "uae/api.h"
#include "uae/types.h"

/* Export/import direction is reversed compared to UAEAPI: the plugin
 * imports UAE's services and exports the ppc_cpu_* implementation. */
#ifdef UAE
#define PPCAPI UAE_EXTERN_C UAE_IMPORT
#else
#define PPCAPI UAE_EXTERN_C UAE_EXPORT
#endif
#define PPCCALL UAECALL

/* PPC_CPU_STATE_*: ppc_cpu_set_state/check_state argument */
#define PPC_CPU_STATE_RUNNING 1
#define PPC_CPU_STATE_PAUSED 2

/* PPC_IMPLEMENTATION_*: values of the ppc_implementation preference */
#define PPC_IMPLEMENTATION_AUTO 0
#define PPC_IMPLEMENTATION_DUMMY 1
#define PPC_IMPLEMENTATION_QEMU 2

/* PPC_STATE_*: values of the global ppc_state; callers test `if (ppc_state)`
 * so INACTIVE must be 0. */
#define PPC_STATE_INACTIVE 0
#define PPC_STATE_STOP 1
#define PPC_STATE_ACTIVE 2
#define PPC_STATE_SLEEP 3
#define PPC_STATE_CRASH 4

/* PPC view of the Amiga memory map. Natural alignment, no packing.
 * memory == NULL routes accesses through the I/O callbacks below.
 * alias is the mirrored Amiga address (0 = none); in single-region
 * updates (count == -1) 0xffffffff means "remove this region".
 * name is owned by the caller and only valid for the duration of the
 * ppc_cpu_map_memory call, so the plugin must copy it. */
typedef struct PPCMemoryRegion {
	uint32_t start;
	uint32_t size;
	void *memory;
	char *name;
	uint32_t alias;
} PPCMemoryRegion;

#ifdef __cplusplus
static_assert(offsetof(PPCMemoryRegion, start) == 0, "PPCMemoryRegion ABI");
static_assert(offsetof(PPCMemoryRegion, size) == 4, "PPCMemoryRegion ABI");
static_assert(offsetof(PPCMemoryRegion, memory) == 2 * sizeof(uint32_t), "PPCMemoryRegion ABI");
static_assert(offsetof(PPCMemoryRegion, name) == 2 * sizeof(uint32_t) + sizeof(void *), "PPCMemoryRegion ABI");
static_assert(offsetof(PPCMemoryRegion, alias) == 2 * sizeof(uint32_t) + 2 * sizeof(void *), "PPCMemoryRegion ABI");
static_assert(sizeof(PPCMemoryRegion) == (sizeof(void *) == 8 ? 32 : 20), "PPCMemoryRegion ABI");
#endif

/* PPC CPU functions implemented by the plugin. Resolved by name:
 * ppc_cpu_init, ppc_cpu_map_memory, ppc_cpu_run_continuous,
 * ppc_cpu_set_state, ppc_cpu_reset are required, ppc_cpu_flush_jit is
 * optional. */
void PPCCALL ppc_cpu_version(int *major, int *minor, int *revision);
bool PPCCALL ppc_cpu_init(const char *model, uint32_t hid1);
void PPCCALL ppc_cpu_map_memory(PPCMemoryRegion *regions, int count);
void PPCCALL ppc_cpu_flush_jit(void);
void PPCCALL ppc_cpu_run_continuous(void);
void PPCCALL ppc_cpu_reset(void);
bool PPCCALL ppc_cpu_check_state(int state);
void PPCCALL ppc_cpu_set_state(int state);

/* I/O callbacks for PPC accesses to non-directly-mapped memory. size is
 * 1, 2 or 4 bytes; the 64-bit forms are big-endian pairs of longs. */
typedef bool (UAECALL *uae_ppc_io_mem_read_function)(uint32_t addr, uint32_t *data, int size);
typedef bool (UAECALL *uae_ppc_io_mem_write_function)(uint32_t addr, uint32_t data, int size);
typedef bool (UAECALL *uae_ppc_io_mem_read64_function)(uint32_t addr, uint64_t *data);
typedef bool (UAECALL *uae_ppc_io_mem_write64_function)(uint32_t addr, uint64_t data);

#ifdef UAE

bool UAECALL uae_ppc_io_mem_read(uint32_t addr, uint32_t *data, int size);
bool UAECALL uae_ppc_io_mem_write(uint32_t addr, uint32_t data, int size);
bool UAECALL uae_ppc_io_mem_read64(uint32_t addr, uint64_t *data);
bool UAECALL uae_ppc_io_mem_write64(uint32_t addr, uint64_t data);

void uae_ppc_get_model(const TCHAR **model, uint32_t *hid1);
void uae_ppc_cpu_reboot(void);
void uae_ppc_cpu_stop(void);
void uae_ppc_interrupt(bool active);
void uae_ppc_cpu_lock(void);
bool uae_ppc_cpu_unlock(void);
void uae_ppc_reset(bool hardreset);
void uae_ppc_free(void);
void uae_ppc_wakeup(void);
void ppc_map_banks(uae_u32 start, uae_u32 size, const TCHAR *name, void *addr, bool remove);
void ppc_remap_bank(uae_u32 start, uae_u32 size, const TCHAR *name, void *addr);
bool uae_self_is_ppc(void);
void uae_ppc_wakeup_main(void);
void uae_ppc_execute_quick(void);
void uae_ppc_execute_check(void);
void uae_ppc_mark_code_cache_dirty(void);
void uae_ppc_spinlock_get(void);
void uae_ppc_spinlock_release(void);
void uae_ppc_pause(int pause);

extern volatile int ppc_state;

#else /* plugin build */

/* The plugin exports these as data symbols; UAE writes the addresses of
 * its implementations into them after loading. */
extern uae_ppc_io_mem_read_function uae_ppc_io_mem_read;
extern uae_ppc_io_mem_write_function uae_ppc_io_mem_write;
extern uae_ppc_io_mem_read64_function uae_ppc_io_mem_read64;
extern uae_ppc_io_mem_write64_function uae_ppc_io_mem_write64;

#endif /* UAE */


#endif /* UAE_PPC_H */
