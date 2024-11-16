/*
 * Multi-platform virtual memory functions for UAE.
 * Copyright (C) 2015 Frode Solheim
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/vm.h"
#include "uae/log.h"
#include "options.h"
#include "memory.h"
#ifdef _WIN32

#else
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

//#if defined(LINUX) && defined(CPU_x86_64)
#if defined(CPU_x86_64) && !defined(__APPLE__)
#define HAVE_MAP_32BIT 1
#endif

// #define CLEAR_MEMORY_ON_COMMIT

// #define LOG_ALLOCATIONS
// #define TRACK_ALLOCATIONS

#ifdef TRACK_ALLOCATIONS

struct alloc_size {
	void *address;
	uae_u32 size;
};

#define MAX_ALLOCATIONS 2048
/* A bit inefficient, but good enough for few and rare allocs. Storing
 * the size at the start of the allocated memory would be better, but this
 * could be awkward if/when you want to allocate page-aligned memory. */
static struct alloc_size alloc_sizes[MAX_ALLOCATIONS];

static void add_allocation(void *address, uae_u32 size)
{
	write_log("VM: add_allocation %p (%d)\n", address, size);
	for (int i = 0; i < MAX_ALLOCATIONS; i++) {
		if (alloc_sizes[i].address == NULL) {
			alloc_sizes[i].address = address;
			alloc_sizes[i].size = size;
			return;
		}
	}
	abort();
}

static uae_u32 find_allocation(void *address)
{
	for (int i = 0; i < MAX_ALLOCATIONS; i++) {
		if (alloc_sizes[i].address == address) {
			return alloc_sizes[i].size;
		}
	}
	abort();
}

static uae_u32 remove_allocation(void *address)
{
	for (int i = 0; i < MAX_ALLOCATIONS; i++) {
		if (alloc_sizes[i].address == address) {
			alloc_sizes[i].address = NULL;
			uae_u32 size = alloc_sizes[i].size;
			alloc_sizes[i].size = 0;
			return size;
		}
	}
	abort();
}

#endif /* TRACK_ALLOCATIONS */

static int protect_to_native(int protect)
{
#ifdef _WIN32
	if (protect == UAE_VM_NO_ACCESS) return PAGE_NOACCESS;
	if (protect == UAE_VM_READ) return PAGE_READONLY;
	if (protect == UAE_VM_READ_WRITE) return PAGE_READWRITE;
	if (protect == UAE_VM_READ_EXECUTE) return PAGE_EXECUTE_READ;
	if (protect == UAE_VM_READ_WRITE_EXECUTE) return PAGE_EXECUTE_READWRITE;
	write_log("VM: Invalid protect value %d\n", protect);
	return PAGE_NOACCESS;
#else
	if (protect == UAE_VM_NO_ACCESS) return PROT_NONE;
	if (protect == UAE_VM_READ) return PROT_READ;
	if (protect == UAE_VM_READ_WRITE) return PROT_READ | PROT_WRITE;
	if (protect == UAE_VM_READ_EXECUTE) return PROT_READ | PROT_EXEC;
	if (protect == UAE_VM_READ_WRITE_EXECUTE) {
		return PROT_READ | PROT_WRITE | PROT_EXEC;
	}
	write_log("VM: Invalid protect value %d\n", protect);
	return PROT_NONE;
#endif
}

static const char *protect_description(int protect)
{
	if (protect == UAE_VM_NO_ACCESS) return "NO_ACCESS";
	if (protect == UAE_VM_READ) return "READ";
	if (protect == UAE_VM_READ_WRITE) return "READ_WRITE";
	if (protect == UAE_VM_READ_EXECUTE) return "READ_EXECUTE";
	if (protect == UAE_VM_READ_WRITE_EXECUTE) return "READ_WRITE_EXECUTE";
	return "UNKNOWN";
}

int uae_vm_page_size(void)
{
	static int page_size = 0;
	if (page_size == 0) {
#ifdef _WIN32
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		page_size = si.dwPageSize;
#else
		page_size = sysconf(_SC_PAGESIZE);
#endif
	}
	return page_size;
}

static void *uae_vm_alloc_with_flags(uae_u32 size, int flags, int protect)
{
	void *address = NULL;
	static bool first_allocation = true;
	if (first_allocation) {
		/* FIXME: log contents of /proc/self/maps on Linux */
		/* FIXME: use VirtualQuery function on Windows? */
		first_allocation = false;
	}
#ifdef LOG_ALLOCATIONS
	write_log("VM: Allocate 0x%-8x bytes [%d] (%s)\n",
			size, flags, protect_description(protect));
#endif

#ifdef _WIN32
	int va_type = MEM_COMMIT | MEM_RESERVE;
	if (flags & UAE_VM_WRITE_WATCH) {
		va_type |= MEM_WRITE_WATCH;
	}
	int va_protect = protect_to_native(protect);
#else
	int mmap_flags = MAP_PRIVATE | MAP_ANON;
	int mmap_prot = protect_to_native(protect);
#endif

#if !defined(CPU_64_BIT) or defined(__APPLE__)
	flags &= ~UAE_VM_32BIT;
#endif
	if (flags & UAE_VM_32BIT) {
		/* Stupid algorithm to find available space, but should
		 * work well enough when there is not a lot of allocations. */
		int step = uae_vm_page_size();
		uae_u8 *p = (uae_u8 *) 0x40000000;
		uae_u8 *p_end = natmem_reserved - size;
		if (size > 1024 * 1024) {
			/* Reserve some space for smaller allocations */
			p += 32 * 1024 * 1024;
			step = 1024 * 1024;
		}
#ifdef HAVE_MAP_32BIT
		address = mmap(0, size, mmap_prot, mmap_flags | MAP_32BIT, -1, 0);
		if (address == MAP_FAILED) {
			address = NULL;
		}
#endif
		while (address == NULL) {
			if (p > p_end) {
				break;
			}
#ifdef _WIN32
			address = VirtualAlloc(p, size, va_type, va_protect);
#else
			address = mmap(p, size, mmap_prot, mmap_flags, -1, 0);
			// write_log("VM: trying %p step is 0x%x = %p\n", p, step, address);
			if (address == MAP_FAILED) {
				address = NULL;
			} else if (((uintptr_t) address) + size > (uintptr_t) 0xffffffff) {
				munmap(address, size);
				address = NULL;
			}
#endif
			p += step;
		}
	} else {
#ifdef _WIN32
		address = VirtualAlloc(NULL, size, va_type, va_protect);
#else
		address = mmap(0, size, mmap_prot, mmap_flags, -1, 0);
		if (address == MAP_FAILED) {
			address = NULL;
		}
#endif
	}

	if (address == NULL) {
		write_log("VM: uae_vm_alloc(%u, %d, %d) mmap failed (%d)\n",
		    size, flags, protect, errno);
	    return NULL;
	}
#ifdef TRACK_ALLOCATIONS
	add_allocation(address, size);
#endif
#ifdef LOG_ALLOCATIONS
	write_log("VM: %p\n", address);
#endif
	return address;
}

void *uae_vm_alloc(uae_u32 size, int flags, int protect)
{
	return uae_vm_alloc_with_flags(size, flags, protect);
}

static bool do_protect(void *address, int size, int protect)
{
#ifdef TRACK_ALLOCATIONS
	uae_u32 allocated_size = find_allocation(address);
	assert(allocated_size == size);
#endif
#ifdef _WIN32
	DWORD old;
	if (VirtualProtect(address, size, protect_to_native(protect), &old) == 0) {
		write_log("VM: uae_vm_protect(%p, %d, %d) VirtualProtect failed (%d)\n",
				address, size, protect, GetLastError());
		return false;
	}
#else
	if (mprotect(address, size, protect_to_native(protect)) != 0) {
		write_log("VM: uae_vm_protect(%p, %d, %d) mprotect failed (%d)\n",
				address, size, protect, errno);
		return false;
	}
#endif
	return true;
}

bool uae_vm_protect(void *address, int size, int protect)
{
	return do_protect(address, size, protect);
}

static bool do_free(void *address, int size)
{
#ifdef TRACK_ALLOCATIONS
	uae_u32 allocated_size = remove_allocation(address);
	assert(allocated_size == size);
#endif
#ifdef _WIN32
	return VirtualFree(address, 0, MEM_RELEASE) != 0;
#else
	if (munmap(address, size) != 0) {
		write_log("VM: uae_vm_free(%p, %d) munmap failed (%d)\n",
				address, size, errno);
		return false;
	}
#endif
	return true;
}

bool uae_vm_free(void *address, int size)
{
	write_log("VM: Free     0x%-8x bytes at %p\n", size, address);
	return do_free(address, size);
}

static void *try_reserve(uintptr_t try_addr, uae_u32 size, int flags)
{
	void *address = NULL;
	if (try_addr) {
		write_log("VM: Reserve  0x%-8x bytes, try address 0x%llx\n",
				size, (uae_u64) try_addr);
	} else {
		write_log("VM: Reserve  0x%-8x bytes\n", size);
	}
#ifdef _WIN32
	int va_type = MEM_RESERVE;
	if (flags & UAE_VM_WRITE_WATCH) {
		va_type |= MEM_WRITE_WATCH;
	}
	int va_protect = protect_to_native(UAE_VM_NO_ACCESS);
	address = VirtualAlloc((void *) try_addr, size, va_type, va_protect);
	if (address == NULL) {
		return NULL;
	}
#else
	int mmap_flags = MAP_PRIVATE | MAP_ANON;
	address = mmap((void *) try_addr, size, PROT_NONE, mmap_flags, -1, 0);
	if (address == MAP_FAILED) {
		return NULL;
	}
#endif
#ifdef CPU_64_BIT
	if (flags & UAE_VM_32BIT) {
		uintptr_t end = (uintptr_t) address + size;
		if (address && end > (uintptr_t) 0x100000000ULL) {
			write_log("VM: Reserve  0x%-8x bytes, got address 0x%llx (> 32-bit)\n",
					size, (uae_u64) (uintptr_t) address);
#ifdef _WIN32
			VirtualFree(address, 0, MEM_RELEASE);
#else
			munmap(address, size);
#endif
			return NULL;
		}
	}
#endif
	return address;
}

void *uae_vm_reserve(uae_u32 size, int flags)
{
	void *address = NULL;
#ifdef _WIN32
	address = try_reserve(0x80000000, size, flags);
	if (address == NULL && (flags & UAE_VM_32BIT)) {
		if (size <= 768 * 1024 * 1024) {
			address = try_reserve(0x78000000 - size, size, flags);
		}
	}
	if (address == NULL && (flags & UAE_VM_32BIT) == 0) {
		address = try_reserve(0, size, flags);
	}
#else
#ifdef CPU_64_BIT
	if (flags & UAE_VM_32BIT) {
#else
	if (true) {
#endif
		uintptr_t try_addr = 0x80000000;
		while (address == NULL) {
			address = try_reserve(try_addr, size, flags);
			if (address == NULL) {
				try_addr -= 0x4000000;
				if (try_addr < 0x20000000) {
					break;
				}
				continue;
			}
		}
	}
	if (address == NULL && (flags & UAE_VM_32BIT) == 0) {
		address = try_reserve(0, size, flags);
	}
#endif
	if (address) {
		write_log("VM: Reserve  0x%-8x bytes, got address 0x%llx\n",
				size, (uae_u64) (uintptr_t) address);
	} else {
		write_log("VM: Reserve  0x%-8x bytes failed!\n", size);
	}
	return address;
}

void *uae_vm_reserve_fixed(void *want_addr, uae_u32 size, int flags)
{
	void *address = NULL;
	//write_log("VM: Reserve  0x%-8x bytes at %p (fixed)\n", size, want_addr);
	address = try_reserve((uintptr_t) want_addr, size, flags);
	if (address == NULL) {
		write_log("VM: Reserve  0x%-8x bytes at %p failed!\n", size, want_addr);
		return NULL;
	}
	if (address != want_addr) {
		do_free(address, size);
		return NULL;
	}
	// write_log("VM: Reserve  0x%-8x bytes, got address 0x%llx\n",
	// 		size, (uae_u64) (uintptr_t) address);
	return address;
}

void *uae_vm_commit(void *address, uae_u32 size, int protect)
{
	// write_log("VM: Commit   0x%-8x bytes at %p (%s)\n",
	// 		size, address, protect_description(protect));
#ifdef _WIN32
	int va_type = MEM_COMMIT ;
	int va_protect = protect_to_native(protect);
	address = VirtualAlloc(address, size, va_type, va_protect);
#else
#ifdef CLEAR_MEMORY_ON_COMMIT
	do_protect(address, size, UAE_VM_READ_WRITE);
	memset(address, 0, size);
#endif
	do_protect(address, size, protect);
#endif
	return address;
}

bool uae_vm_decommit(void *address, uae_u32 size)
{
	//write_log("VM: Decommit 0x%-8x bytes at %p\n", size, address);
#ifdef _WIN32
	return VirtualFree (address, size, MEM_DECOMMIT) != 0;
#else
    /* Re-map the memory so we get fresh unused pages (and the old ones can be
     * released and physical memory reclaimed). We also assume that the new
     * pages will be zero-initialized (tested on Linux and OS X). */
    void *result = mmap(address, size, PROT_NONE,
                        MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (result == MAP_FAILED) {
		write_log("VM: Warning - could not re-map with MAP_FIXED at %p\n",
                address);
        do_protect(address, size, UAE_VM_NO_ACCESS);
    }
    return result != MAP_FAILED;
#endif
}
