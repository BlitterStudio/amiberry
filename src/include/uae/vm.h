/*
 * Multi-platform virtual memory functions for UAE.
 * Copyright (C) 2015 Frode Solheim
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#ifndef UAE_VM_H
#define UAE_VM_H

#include "uae/types.h"

#define UAE_VM_WRITE 2
#define UAE_VM_EXECUTE 4

#define UAE_VM_32BIT (1 << 8)
#define UAE_VM_WRITE_WATCH (1 << 9)
#define UAE_VM_ALLOC_FAILED NULL

/* Even though it looks like you can OR together vm protection values,
 * do not do this. Not all combinations are supported (on Windows), and only
 * a few combinations are implemented. Only use the following predefined
 * constants to be safe. */

#define UAE_VM_NO_ACCESS 0
#define UAE_VM_READ 1
#define UAE_VM_READ_WRITE (UAE_VM_READ | UAE_VM_WRITE)
#define UAE_VM_READ_EXECUTE (UAE_VM_READ | UAE_VM_EXECUTE)
#define UAE_VM_READ_WRITE_EXECUTE (UAE_VM_READ | UAE_VM_WRITE | UAE_VM_EXECUTE)

#if 0
void *uae_vm_alloc(uae_u32 size);
void *uae_vm_alloc(uae_u32 size, int flags);
#endif
void *uae_vm_alloc(uae_u32 size, int flags, int protect);
bool uae_vm_protect(void *address, int size, int protect);
bool uae_vm_free(void *address, int size);

void *uae_vm_reserve(uae_u32 size, int flags);
void *uae_vm_reserve_fixed(void *address, uae_u32 size, int flags);
void *uae_vm_commit(void *address, uae_u32 size, int protect);
bool uae_vm_decommit(void *address, uae_u32 size);

int uae_vm_page_size(void);

// void *uae_vm_alloc_with_flags(uae_u32 size, int protect, int flags);

#endif /* UAE_VM_H */
