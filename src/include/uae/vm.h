/*
 * UAE virtual-memory interface.
 *
 * Thin platform-independent layer over mmap/VirtualAlloc used by natmem,
 * the JIT caches and the various CPU cores. Protection arguments are the
 * combined UAE_VM_READ/WRITE/EXECUTE values (0, 1, 3, 5, 7); anything else
 * is treated as no access. Allocation flags are UAE_VM_32BIT (result must
 * lie below 4 GiB on 64-bit hosts), UAE_VM_WRITE_WATCH (Windows write
 * watching) and UAE_VM_JIT (Apple arm64 MAP_JIT).
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UAE_VM_H
#define UAE_VM_H

#include <cstddef>

#include "uae/types.h"

/* Protection values (the five combinations below are the valid ones) */
#define UAE_VM_NO_ACCESS 0
#define UAE_VM_READ 1
#define UAE_VM_WRITE 2
#define UAE_VM_EXECUTE 4
#define UAE_VM_READ_WRITE 3
#define UAE_VM_READ_EXECUTE 5
#define UAE_VM_READ_WRITE_EXECUTE 7

/* Allocation flags */
#define UAE_VM_32BIT 0x100
#define UAE_VM_WRITE_WATCH 0x200
#define UAE_VM_JIT 0x400

/* mmap-style failure sentinel; callers compare pointers against it */
#define UAE_VM_ALLOC_FAILED NULL

/* Allocate a fresh committed region with `protect` (mmap MAP_PRIVATE |
 * MAP_ANONYMOUS / VirtualAlloc MEM_COMMIT|MEM_RESERVE). Returns the base
 * or NULL. */
void *uae_vm_alloc(size_t size, int flags, int protect);

/* Change the protection of a range (mprotect / VirtualProtect). */
bool uae_vm_protect(void *address, size_t size, int protect);

/* Release a region from uae_vm_alloc or uae_vm_reserve (munmap /
 * VirtualFree MEM_RELEASE; on Windows `size` is ignored). */
bool uae_vm_free(void *address, size_t size);

/* Reserve address space without committing (PROT_NONE / PAGE_NOACCESS).
 * Returns the base or NULL. */
void *uae_vm_reserve(size_t size, int flags);

/* As uae_vm_reserve, but at exactly `address`; fails if the kernel cannot
 * place the reservation there. */
void *uae_vm_reserve_fixed(void *address, size_t size, int flags);

/* Commit a range inside a reservation. POSIX returns `address` even when
 * the protection change fails. */
void *uae_vm_commit(void *address, size_t size, int protect);

/* Discard the contents of a committed range; the next commit sees
 * zero-filled pages. */
bool uae_vm_decommit(void *address, size_t size);

/* OS page size in bytes (> 0). */
int uae_vm_page_size(void);

/* Apple arm64 only: toggle the current thread between MAP_JIT write mode
 * (false) and execute mode (true). No-op elsewhere. */
void uae_vm_jit_write_protect(bool enable_execute_mode);

#endif /* UAE_VM_H */
