/*
 * UAE virtual-memory implementation (see uae/vm.h).
 *
 * POSIX path: private anonymous mmap everywhere. Reservations are mapped
 * PROT_NONE and committed lazily by raising the protection; commit never
 * zeroes pages because fresh anonymous mappings are already zero-filled.
 * Decommit replaces the range in place with a new PROT_NONE mapping, which
 * both discards the old pages and re-zeroes the next commit.
 *
 * Windows path: VirtualAlloc/VirtualFree with proper reserve/commit
 * separation; uae_vm_free releases the whole allocation and ignores `size`.
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "memory.h"
#include "uae/log.h"
#include "uae/vm.h"

#ifdef _WIN32

/* Win32 memory APIs are already visible through sysconfig.h. */

#else /* POSIX */

#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#if defined(__APPLE__) && defined(CPU_AARCH64)
#include <pthread.h>
#endif

#endif

/* natmem_offset/natmem_reserved: the big natmem reservation made by
 * preinit_shm() (osdep/amiberry_mem.cpp). Low-memory (32-bit) allocations
 * must stay below it so they never overlap the Amiga address space. It is
 * null until natmem is reserved. */
extern uae_u8 *natmem_reserved;

/* ------------------------------------------------------------------ */
/* Protection translation                                             */
/* ------------------------------------------------------------------ */
#ifndef _WIN32

static int vm_prot_posix(int protect)
{
	switch (protect) {
	case UAE_VM_NO_ACCESS:
		return PROT_NONE;
	case UAE_VM_READ:
		return PROT_READ;
	case UAE_VM_READ_WRITE:
		return PROT_READ | PROT_WRITE;
	case UAE_VM_READ_EXECUTE:
		return PROT_READ | PROT_EXEC;
	case UAE_VM_READ_WRITE_EXECUTE:
		return PROT_READ | PROT_WRITE | PROT_EXEC;
	default:
		write_log("VM: invalid protect value %d\n", protect);
		return PROT_NONE;
	}
}

/* Extra mmap flags implied by the allocation flags. Only Apple arm64 has
 * MAP_JIT; everywhere else (including macOS x86-64 and Windows ARM64)
 * UAE_VM_JIT is ignored. */
static int vm_map_flags(int flags)
{
	int map_flags = MAP_PRIVATE | MAP_ANONYMOUS;
#if defined(__APPLE__) && defined(CPU_AARCH64)
	if (flags & UAE_VM_JIT) {
#ifdef MAP_JIT
		map_flags |= MAP_JIT;
#else
		write_log("VM: MAP_JIT not available, allocating without it\n");
#endif
	}
#endif
	return map_flags;
}

#else /* _WIN32 */

static DWORD vm_prot_win(int protect)
{
	switch (protect) {
	case UAE_VM_NO_ACCESS:
		return PAGE_NOACCESS;
	case UAE_VM_READ:
		return PAGE_READONLY;
	case UAE_VM_READ_WRITE:
		return PAGE_READWRITE;
	case UAE_VM_READ_EXECUTE:
		return PAGE_EXECUTE_READ;
	case UAE_VM_READ_WRITE_EXECUTE:
		return PAGE_EXECUTE_READWRITE;
	default:
		write_log("VM: invalid protect value %d\n", protect);
		return PAGE_NOACCESS;
	}
}

#endif /* _WIN32 */


/* ------------------------------------------------------------------ */
/* Page size                                                          */
/* ------------------------------------------------------------------ */

int uae_vm_page_size(void)
{
	/* Function-local static initialisation is thread-safe (C++11) and
	 * makes concurrent first calls return the same cached value. */
	static const int page_size = [] {
#ifdef _WIN32
		/* dwPageSize (4 KiB), not the 64 KiB allocation granularity. */
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		return static_cast<int>(system_info.dwPageSize);
#else
		return static_cast<int>(sysconf(_SC_PAGESIZE));
#endif
	}();
	return page_size;
}

/* ------------------------------------------------------------------ */
/* Allocation                                                         */
/* ------------------------------------------------------------------ */

/* Try to place a below-4-GiB allocation by hinting candidate base
 * addresses (non-fixed: the kernel may place the mapping elsewhere, in
 * which case it is kept only if it still ends below 4 GiB). */
#ifndef _WIN32
static void *vm_alloc_32bit_posix(size_t size, int flags, int protect)
{
	const int prot = vm_prot_posix(protect);
	const int map_flags = vm_map_flags(flags);

#ifdef MAP_32BIT
	/* x86-64 Linux/Android/FreeBSD/BSD: ask the kernel directly for a
	 * mapping below 2 GiB (absolute addressing in the x86 JIT needs it).
	 * Some kernels (Rosetta's linux/amd64 emulation, for example) ignore
	 * the flag, so the promise is verified rather than trusted. */
	void *address = mmap(nullptr, size, prot, map_flags | MAP_32BIT, -1, 0);
	if (address != MAP_FAILED) {
		if (reinterpret_cast<uintptr_t>(address) + size <= 0xFFFFFFFFULL) {
			return address;
		}
		munmap(address, size);
	}
#endif

	const uintptr_t page = static_cast<uintptr_t>(uae_vm_page_size());
	const uintptr_t megabyte = 1024 * 1024;
	/* Large blocks start higher and step by 1 MiB so the scan cannot take
	 * millions of iterations; small blocks step by one page. */
	const uintptr_t step = size > megabyte ? megabyte : page;
	uintptr_t base;
	uintptr_t limit;
#ifdef __HAIKU__
	base = 0x10000000;
	limit = 0x7F000000 - size;
#else
	base = size > megabyte ? 0x42000000 : 0x40000000;
	/* Never return anything that ends above 4 GiB, even when natmem is
	 * not reserved (or reserved above 4 GiB) yet. */
	limit = 0x100000000ULL - size;
#endif
	if (natmem_reserved != nullptr) {
		const uintptr_t natmem_limit = reinterpret_cast<uintptr_t>(natmem_reserved) - size;
		if (natmem_limit < limit && natmem_limit >= base) {
			limit = natmem_limit;
		}
	}

	for (uintptr_t hint = base; hint <= limit; hint += step) {
		void *address = mmap(reinterpret_cast<void *>(hint), size, prot, map_flags, -1, 0);
		if (address == MAP_FAILED) {
			continue;
		}
		if (reinterpret_cast<uintptr_t>(address) + size <= 0xFFFFFFFFULL) {
			return address;
		}
		munmap(address, size);
	}
	return nullptr;
}
#endif


#ifdef _WIN32
/* Windows twin of the probing allocator: VirtualAlloc with a non-null
 * base is an exact-address request, so a failed candidate is simply
 * followed by the next one. */
static void *vm_alloc_32bit_windows(size_t size, int flags, int protect)
{
	const DWORD prot = vm_prot_win(protect);
	int allocation_type = MEM_COMMIT | MEM_RESERVE;
	if (flags & UAE_VM_WRITE_WATCH) {
		allocation_type |= MEM_WRITE_WATCH;
	}
	const uintptr_t page = static_cast<uintptr_t>(uae_vm_page_size());
	const uintptr_t megabyte = 1024 * 1024;
	const uintptr_t step = size > megabyte ? megabyte : page;
	const uintptr_t base = size > megabyte ? 0x42000000 : 0x40000000;
	uintptr_t limit = 0x100000000ULL - size;
	if (natmem_reserved != nullptr) {
		const uintptr_t natmem_limit = reinterpret_cast<uintptr_t>(natmem_reserved) - size;
		if (natmem_limit < limit && natmem_limit >= base) {
			limit = natmem_limit;
		}
	}
	for (uintptr_t candidate = base; candidate <= limit; candidate += step) {
		void *address = VirtualAlloc(reinterpret_cast<void *>(candidate), size, allocation_type, prot);
		if (address != nullptr) {
			return address;
		}
	}
	return nullptr;
}
#endif

void *uae_vm_alloc(size_t size, int flags, int protect)
{
	/* The 32-bit placement request is meaningless (and impossible to
	 * honour) on 32-bit hosts. */
#ifndef CPU_64_BIT
	flags &= ~UAE_VM_32BIT;
#endif

#ifdef _WIN32

#ifdef CPU_64_BIT
	if (flags & UAE_VM_32BIT) {
		void *address = vm_alloc_32bit_windows(size, flags, protect);
		if (address != nullptr) {
			return address;
		}
		write_log("VM: 32-bit alloc failed size=%zu flags=%d protect=%d error=%lu\n",
			size, flags, protect, GetLastError());
		return nullptr;
	}
#endif

	int allocation_type = MEM_COMMIT | MEM_RESERVE;
	if (flags & UAE_VM_WRITE_WATCH) {
		allocation_type |= MEM_WRITE_WATCH;
	}
	void *address = VirtualAlloc(nullptr, size, allocation_type, vm_prot_win(protect));
	if (address == nullptr) {
		write_log("VM: alloc failed size=%zu flags=%d protect=%d error=%lu\n",
			size, flags, protect, GetLastError());
#if defined(CPU_AARCH64)
		if (protect == UAE_VM_READ_WRITE_EXECUTE) {
			write_log("VM: RWX allocation failed; VBS/HVCI or Arbitrary Code Guard may block executable memory\n");
		}
#endif
		return nullptr;
	}
	return address;

#else /* POSIX */

	int map_flags = vm_map_flags(flags);

#if defined(CPU_64_BIT)
	if (flags & UAE_VM_32BIT) {
		void *address = vm_alloc_32bit_posix(size, flags, protect);
		if (address != nullptr) {
			return address;
		}
		write_log("VM: 32-bit alloc failed size=%zu flags=%d protect=%d errno=%d\n",
			size, flags, protect, errno);
		return nullptr;
	}
#endif

	void *address = mmap(nullptr, size, vm_prot_posix(protect), map_flags, -1, 0);
	if (address == MAP_FAILED) {
		write_log("VM: alloc failed size=%zu flags=%d protect=%d errno=%d\n",
			size, flags, protect, errno);
#if defined(__APPLE__) && defined(CPU_AARCH64)
		if ((flags & UAE_VM_JIT) && (errno == EPERM || errno == EINVAL)) {
			write_log("VM: MAP_JIT allocation failed; check that MAP_JIT is available and the com.apple.security.cs.allow-jit entitlement is enabled\n");
		}
#endif
#if defined(__ANDROID__) && defined(CPU_AARCH64)
		if (!(flags & UAE_VM_JIT) && protect == UAE_VM_READ_WRITE_EXECUTE
			&& (errno == EPERM || errno == EACCES)) {
			write_log("VM: SELinux may be denying PROT_EXEC memory on this Android build\n");
		}
#endif
		return nullptr;
	}
	return address;

#endif
}

/* ------------------------------------------------------------------ */
/* Protection / free                                                  */
/* ------------------------------------------------------------------ */

bool uae_vm_protect(void *address, size_t size, int protect)
{
#ifdef _WIN32
	DWORD old_protect;
	if (VirtualProtect(address, size, vm_prot_win(protect), &old_protect)) {
		return true;
	}
	write_log("VM: protect failed address=%p size=%zu protect=%d error=%lu\n",
		address, size, protect, GetLastError());
	return false;
#else
	if (mprotect(address, size, vm_prot_posix(protect)) == 0) {
		return true;
	}
	write_log("VM: protect failed address=%p size=%zu protect=%d errno=%d\n",
		address, size, protect, errno);
	return false;
#endif
}

bool uae_vm_free(void *address, size_t size)
{
	if (address == nullptr) {
		return false;
	}
#ifdef _WIN32
	/* MEM_RELEASE frees the whole reservation; the size must be 0. */
	if (VirtualFree(address, 0, MEM_RELEASE)) {
		return true;
	}
	write_log("VM: free failed address=%p error=%lu\n", address, GetLastError());
	return false;
#else
	if (munmap(address, size) == 0) {
		return true;
	}
	write_log("VM: free failed address=%p size=%zu errno=%d\n", address, size, errno);
	return false;
#endif
}

/* ------------------------------------------------------------------ */
/* Reservation                                                        */
/* ------------------------------------------------------------------ */

/* Single-attempt reservation primitive. `candidate` is the requested base
 * (0 lets the OS choose); the mapping is a non-fixed hint on POSIX and an
 * exact-address request on Windows. */
static void *vm_reserve_at(void *candidate, size_t size, int flags)
{
#ifdef _WIN32

	int allocation_type = MEM_RESERVE;
	if (flags & UAE_VM_WRITE_WATCH) {
		allocation_type |= MEM_WRITE_WATCH;
	}
	return VirtualAlloc(candidate, size, allocation_type, PAGE_NOACCESS);

#else /* POSIX */

	uintptr_t hint = reinterpret_cast<uintptr_t>(candidate);
	int map_flags = vm_map_flags(flags);
#ifdef __FreeBSD__
	/* ASLR scatters hinted mappings all over the address space; force the
	 * reservation into the low 2 GiB where the x86 JIT can reach it with
	 * absolute addressing. */
	hint = 0x10000000;
#ifdef MAP_32BIT
	map_flags |= MAP_32BIT;
#endif
#elif defined(__HAIKU__)
	if (hint == 0 || hint > 0x7F000000) {
		hint = 0x10000000;
	}
#endif

	void *address = mmap(reinterpret_cast<void *>(hint), size, PROT_NONE, map_flags, -1, 0);
	if (address == MAP_FAILED) {
		return nullptr;
	}
#ifdef CPU_64_BIT
	if ((flags & UAE_VM_32BIT)
		&& reinterpret_cast<uintptr_t>(address) + size > 0x100000000ULL) {
		write_log("VM: reserve at %p (size %zu) ends above 4 GiB, rejected\n", address, size);
		munmap(address, size);
		return nullptr;
	}
#endif
	return address;

#endif
}

/* Probe candidate bases downwards from 2 GiB in 64 MiB steps. Used for
 * 32-bit placements on POSIX. */
static void *vm_reserve_probe_downward(size_t size, int flags)
{
	for (uintptr_t base = 0x80000000; base >= 0x20000000; base -= 0x04000000) {
		void *address = vm_reserve_at(reinterpret_cast<void *>(base), size, flags);
		if (address != nullptr) {
			return address;
		}
	}
	return nullptr;
}

void *uae_vm_reserve(size_t size, int flags)
{
#ifndef CPU_64_BIT
	/* Below 4 GiB is the only option on a 32-bit host. */
	flags &= ~UAE_VM_32BIT;
#endif

	void *address = nullptr;

#ifdef _WIN32

	const bool want_32bit = (flags & UAE_VM_32BIT) != 0;
#if defined(CPU_AARCH64)
	if (!want_32bit) {
		/* Above 4 GiB, so bit 31 of the low 32 bits of natmem pointers is
		 * clear — the layout the arm64 JIT expects, matching macOS arm64. */
		address = vm_reserve_at(reinterpret_cast<void *>(0x100000000ULL), size, flags);
		if (address != nullptr) {
			return address;
		}
	}
#endif
	address = vm_reserve_at(reinterpret_cast<void *>(0x80000000), size, flags);
	if (address == nullptr && want_32bit && size <= 768 * 1024 * 1024) {
		address = vm_reserve_at(reinterpret_cast<void *>(0x78000000 - size), size, flags);
	}
	if (address == nullptr && !want_32bit) {
		address = vm_reserve_at(nullptr, size, flags);
	}

#else /* POSIX */

#ifdef CPU_64_BIT
	if (flags & UAE_VM_32BIT) {
		address = vm_reserve_probe_downward(size, flags);
	} else {
		address = vm_reserve_at(nullptr, size, flags);
	}
#else
	/* 32-bit host: the downward probe normally lands on the first hint;
	 * fall back to an OS-chosen placement when 32-bit placement was not
	 * explicitly requested. */
	address = vm_reserve_probe_downward(size, flags);
	if (address == nullptr && !(flags & UAE_VM_32BIT)) {
		address = vm_reserve_at(nullptr, size, flags);
	}
#endif

#endif

	if (address == nullptr) {
		write_log("VM: reserve failed size=%zu flags=%d\n", size, flags);
	} else {
		write_log("VM: reserved %p (size %zu, flags %d)\n", address, size, flags);
	}
	return address;
}

void *uae_vm_reserve_fixed(void *address, size_t size, int flags)
{
	void *result = vm_reserve_at(address, size, flags);
	if (result == nullptr) {
		write_log("VM: fixed reserve failed at %p size=%zu flags=%d\n", address, size, flags);
		return nullptr;
	}
	if (result != address) {
		/* The kernel placed the hinted mapping elsewhere. */
		write_log("VM: fixed reserve at %p got %p instead, rejected\n", address, result);
#ifdef _WIN32
		VirtualFree(result, 0, MEM_RELEASE);
#else
		munmap(result, size);
#endif
		return nullptr;
	}
	return result;
}

/* ------------------------------------------------------------------ */
/* Commit / decommit                                                  */
/* ------------------------------------------------------------------ */

void *uae_vm_commit(void *address, size_t size, int protect)
{
#ifdef _WIN32
	void *result = VirtualAlloc(address, size, MEM_COMMIT, vm_prot_win(protect));
	if (result == nullptr) {
		write_log("VM: commit failed address=%p size=%zu protect=%d error=%lu\n",
			address, size, protect, GetLastError());
	}
	return result;
#else
	/* The pages exist since reserve time (PROT_NONE); committing is just
	 * making them accessible. Fresh anonymous pages are already zero. */
	if (mprotect(address, size, vm_prot_posix(protect)) != 0) {
		write_log("VM: commit failed address=%p size=%zu protect=%d errno=%d (ignored)\n",
			address, size, protect, errno);
	}
	return address;
#endif
}

bool uae_vm_decommit(void *address, size_t size)
{
#ifdef _WIN32
	if (VirtualFree(address, size, MEM_DECOMMIT)) {
		return true;
	}
	write_log("VM: decommit failed address=%p size=%zu error=%lu\n",
		address, size, GetLastError());
	return false;
#else
	/* Replace the range in place with a fresh zero-filled PROT_NONE
	 * mapping, discarding the old pages. */
	void *result = mmap(address, size, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	if (result == MAP_FAILED) {
		write_log("VM: decommit remap failed address=%p size=%zu errno=%d\n",
			address, size, errno);
		mprotect(address, size, PROT_NONE);
		return false;
	}
	return true;
#endif
}

/* ------------------------------------------------------------------ */
/* JIT write protection (Apple arm64 W^X)                             */
/* ------------------------------------------------------------------ */

void uae_vm_jit_write_protect(bool enable_execute_mode)
{
#if defined(__APPLE__) && defined(CPU_AARCH64) && !defined(AMIBERRY_IOS)
	/* Per-thread MAP_JIT toggle: execute-only when true, write-only when
	 * false. Cheap and safe to call repeatedly, even with no MAP_JIT
	 * memory allocated. */
	pthread_jit_write_protect_np(enable_execute_mode ? 1 : 0);
#else
	(void)enable_execute_mode;
#endif
}
