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

#endif /* UAE_VM_H */
