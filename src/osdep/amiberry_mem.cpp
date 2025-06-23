#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "uae/mman.h"
#include "uae/vm.h"
#include "autoconf.h"
#include "gfxboard.h"
#include "cpuboard.h"
#include "rommgr.h"
#include "newcpu.h"
#include <sys/mman.h>

#include "gui.h"
#include "sys/types.h"
#ifndef __MACH__
#include "sys/sysinfo.h"
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

#if defined(__x86_64__) || defined(CPU_AARCH64) || defined(__MACH__)
static int os_64bit = 1;
#else
static int os_64bit = 0;
#endif

#define MEM_COMMIT       0x00001000
#define MEM_RESERVE      0x00002000
#define MEM_DECOMMIT         0x4000
#define MEM_RELEASE          0x8000
#define MEM_WRITE_WATCH  0x00200000
#define MEM_TOP_DOWN     0x00100000

#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04

typedef void* LPVOID;
typedef size_t SIZE_T;

typedef struct {
	int dwPageSize;
} SYSTEM_INFO;

static void GetSystemInfo(SYSTEM_INFO* si)
{
	si->dwPageSize = sysconf(_SC_PAGESIZE);
}

#define USE_MMAP

#ifdef USE_MMAP
#ifdef MACOSX
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

static void* VirtualAlloc(void* lpAddress, size_t dwSize, int flAllocationType,
	int flProtect)
{
	write_log("- VirtualAlloc addr=%p size=%zu type=%d prot=%d\n",
		lpAddress, dwSize, flAllocationType, flProtect);
	if (flAllocationType & MEM_RESERVE) {
		write_log("  MEM_RESERVE\n");
	}
	if (flAllocationType & MEM_COMMIT) {
		write_log("  MEM_COMMIT\n");
	}

	int prot = 0;
	if (flProtect == PAGE_READWRITE) {
		write_log("  PAGE_READWRITE\n");
		prot = UAE_VM_READ_WRITE;
	}
	else if (flProtect == PAGE_READONLY) {
		write_log("  PAGE_READONLY\n");
		prot = UAE_VM_READ;
	}
	else if (flProtect == PAGE_EXECUTE_READWRITE) {
		write_log("  PAGE_EXECUTE_READWRITE\n");
		prot = UAE_VM_READ_WRITE_EXECUTE;
	}
	else {
		write_log("  WARNING: unknown protection\n");
	}

	void* address = nullptr;

	if (flAllocationType == MEM_COMMIT && lpAddress == nullptr) {
		write_log("NATMEM: Allocated non-reserved memory size %zu\n", dwSize);
		void* memory = uae_vm_alloc(dwSize, 0, UAE_VM_READ_WRITE);
		if (memory == nullptr) {
			write_log("memory allocated failed errno %d\n", errno);
		}
		return memory;
	}

	if (flAllocationType & MEM_RESERVE) {
		address = uae_vm_reserve(dwSize, 0);
	}
	else {
		address = lpAddress;
	}

	if (flAllocationType & MEM_COMMIT) {
		write_log("commit prot=%d\n", prot);
		uae_vm_commit(address, dwSize, prot);
	}

	return address;
}

static int VirtualProtect(void* lpAddress, int dwSize, int flNewProtect,
	unsigned int* lpflOldProtect)
{
	write_log("- VirtualProtect addr=%p size=%d prot=%d\n",
		lpAddress, dwSize, flNewProtect);
	int prot = 0;
	if (flNewProtect == PAGE_READWRITE) {
		write_log("  PAGE_READWRITE\n");
		prot = UAE_VM_READ_WRITE;
	}
	else if (flNewProtect == PAGE_READONLY) {
		write_log("  PAGE_READONLY\n");
		prot = UAE_VM_READ;
	}
	else {
		write_log("  -- unknown protection --\n");
	}
	if (uae_vm_protect(lpAddress, dwSize, prot) == 0) {
		write_log("mprotect failed errno %d\n", errno);
		return 0;
	}
	return 1;
}

static bool VirtualFree(void* lpAddress, size_t dwSize, int dwFreeType)
{
	if (dwFreeType == MEM_DECOMMIT) {
		return uae_vm_decommit(lpAddress, dwSize);
	}
	if (dwFreeType == MEM_RELEASE) {
		return uae_vm_free(lpAddress, dwSize);
	}
	return false;
}

static int GetLastError()
{
	return errno;
}

static int my_getpagesize()
{
	return uae_vm_page_size();
}

#define getpagesize my_getpagesize

uae_u32 max_z3fastmem;
uae_u32 max_physmem;

/* BARRIER is used in case Amiga memory is access across memory banks,
 * for example move.l $1fffffff,d0 when $10000000-$1fffffff is mapped and
 * $20000000+ is not mapped.
 * Note: BARRIER will probably effectively be rounded up the host memory
 * page size.
 */
#define BARRIER 32

#define MAXZ3MEM32 0x7F000000
#define MAXZ3MEM64 0xF0000000

static struct uae_shmid_ds shmids[MAX_SHMID];
uae_u8 *natmem_reserved, *natmem_offset;
uae_u32 natmem_reserved_size;
static uae_u8 *p96mem_offset;
static int p96mem_size;
static uae_u32 p96base_offset;
static SYSTEM_INFO si;
static uaecptr start_rtg = 0;
static uaecptr end_rtg = 0;
int maxmem;
bool jit_direct_compatible_memory;

bool can_have_1gb()
{
	#if defined(__linux__)
	struct sysinfo mem_info{};
	sysinfo(&mem_info);
	long long total_phys_mem = mem_info.totalram;
	total_phys_mem *= mem_info.mem_unit;
	// Do we have more than 2GB in the system?
	if (total_phys_mem > 2147483648LL)
		return true;
	return false;
	#else
	// On OSX just return true, there's no M1 mac with less than 8G of RAM
	return true;
	#endif
}

static uae_u8 *virtualallocwithlock (LPVOID addr, SIZE_T size, unsigned int allocationtype, unsigned int protect)
{
	auto p = static_cast<uae_u8*>(VirtualAlloc(addr, size, allocationtype, protect));
	return p;
}
static void virtualfreewithlock (LPVOID addr, SIZE_T size, unsigned int freetype)
{
	VirtualFree(addr, size, freetype);
}

static uae_u32 lowmem ()
{
	uae_u32 change = 0;
	return change;
}

static uae_u64 size64;

static void clear_shm ()
{
	shm_start = nullptr;
	for (auto & shmid : shmids) {
		memset (&shmid, 0, sizeof(struct uae_shmid_ds));
		shmid.key = -1;
	}
}

bool preinit_shm ()
{
	uae_u64 total64;
	uae_u64 totalphys64;
#ifdef _WIN32
	MEMORYSTATUS memstats;
	GLOBALMEMORYSTATUSEX pGlobalMemoryStatusEx;
	MEMORYSTATUSEX memstatsex;
#endif

	if (natmem_reserved)
		VirtualFree (natmem_reserved, 0, MEM_RELEASE);

	natmem_reserved = nullptr;
	natmem_offset = nullptr;

	GetSystemInfo (&si);
	uae_u32 max_allowed_mman = 512 + 256;
	if (os_64bit) {
		// Higher than 2G to support G-REX PCI VRAM
		max_allowed_mman = 2560;
	}
	max_allowed_mman = std::max<uae_u32>(maxmem, max_allowed_mman);

#ifdef _WIN32
	memstats.dwLength = sizeof(memstats);
	GlobalMemoryStatus(&memstats);
	totalphys64 = memstats.dwTotalPhys;
	total64 = (uae_u64)memstats.dwAvailPageFile + (uae_u64)memstats.dwTotalPhys;
#ifdef AMIBERRY
	pGlobalMemoryStatusEx = GlobalMemoryStatusEx;
#else
	pGlobalMemoryStatusEx = (GLOBALMEMORYSTATUSEX)GetProcAddress (GetModuleHandle (_T("kernel32.dll")), "GlobalMemoryStatusEx");
#endif
	if (pGlobalMemoryStatusEx) {
		memstatsex.dwLength = sizeof (MEMORYSTATUSEX);
		if (pGlobalMemoryStatusEx(&memstatsex)) {
			totalphys64 = memstatsex.ullTotalPhys;
			total64 = memstatsex.ullAvailPageFile + memstatsex.ullTotalPhys;
		}
	}
#else
#ifdef AMIBERRY
#ifdef __APPLE__
	int mib[2];
	size_t len;

	mib[0] = CTL_HW;
	// FIXME: check 64-bit compat
	mib[1] = HW_MEMSIZE; /* gives a 64 bit int */
	len = sizeof(totalphys64);
	sysctl(mib, 2, &totalphys64, &len, nullptr, 0);
	total64 = (uae_u64) totalphys64;
#else
	totalphys64 = sysconf (_SC_PHYS_PAGES) * (uae_u64)getpagesize();
	total64 = (uae_u64)sysconf (_SC_PHYS_PAGES) * (uae_u64)getpagesize();
#endif
#endif
#endif
	size64 = total64;
	if (os_64bit) {
		if (size64 > MAXZ3MEM64)
			size64 = MAXZ3MEM64;
	} else {
		if (size64 > MAXZ3MEM32)
			size64 = MAXZ3MEM32;
	}
	if (maxmem < 0) {
		size64 = MAXZ3MEM64;
		if (!os_64bit) {
			if (totalphys64 < 1536 * 1024 * 1024)
				max_allowed_mman = 256;
			max_allowed_mman = std::max<uae_u32>(max_allowed_mman, 256);
		}
	} else if (maxmem > 0) {
		size64 = static_cast<uae_u64>(maxmem) * 1024 * 1024;
	}
	size64 = std::max<uae_u64>(size64, 8 * 1024 * 1024);
	if (static_cast<uae_u64>(max_allowed_mman) * 1024 * 1024 > size64)
		max_allowed_mman = static_cast<uae_u32>(size64 / (1024 * 1024));

	uae_u32 natmem_size = (max_allowed_mman + 1) * 1024 * 1024;
	natmem_size = std::max<uae_u32>(natmem_size, 17 * 1024 * 1024);

#if WIN32_NATMEM_TEST
	natmem_size = WIN32_NATMEM_TEST * 1024 * 1024;
#endif

	natmem_size = std::min(natmem_size, 0xc0000000);

	write_log (_T("MMAN: Total physical RAM %llu MB, all RAM %llu MB\n"),
	           totalphys64 >> 20, total64 >> 20);
	write_log(_T("MMAN: Attempting to reserve: %u MB\n"), natmem_size >> 20);

#if 1
	natmem_reserved = static_cast<uae_u8*>(uae_vm_reserve(natmem_size, UAE_VM_32BIT | UAE_VM_WRITE_WATCH));
#else
	natmem_size = 0x20000000;
	natmem_reserved = (uae_u8 *) uae_vm_reserve_fixed(
		(void *) 0x90000000, natmem_size, UAE_VM_32BIT | UAE_VM_WRITE_WATCH);
#endif

	if (!natmem_reserved) {
		if (natmem_size <= 768 * 1024 * 1024) {
			uae_u32 p = 0x78000000 - natmem_size;
			for (;;) {
				natmem_reserved = static_cast<uae_u8*>(VirtualAlloc(reinterpret_cast<void*>(static_cast<intptr_t>(p)), natmem_size, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE));
				if (natmem_reserved)
					break;
				p -= 128 * 1024 * 1024;
				if (p <= 128 * 1024 * 1024)
					break;
			}
		}
	}
	if (!natmem_reserved) {
		unsigned int vaflags = MEM_RESERVE | MEM_WRITE_WATCH;
		for (;;) {
			natmem_reserved = static_cast<uae_u8*>(VirtualAlloc(nullptr, natmem_size, vaflags, PAGE_READWRITE));
			if (natmem_reserved)
				break;
			natmem_size -= 64 * 1024 * 1024;
			if (!natmem_size) {
				write_log (_T("MMAN: Can't allocate 257M of virtual address space!?\n"));
				natmem_size = 17 * 1024 * 1024;
				natmem_reserved = static_cast<uae_u8*>(VirtualAlloc(nullptr, natmem_size, vaflags, PAGE_READWRITE));
				if (!natmem_size) {
					write_log (_T("MMAN: Can't allocate 17M of virtual address space!? Something is seriously wrong\n"));
					notify_user(NUMSG_NOMEMORY);
					return false;
				}
				break;
			}
		}
	}
	natmem_reserved_size = natmem_size;
	natmem_offset = natmem_reserved;
	if (natmem_size <= 257 * 1024 * 1024) {
		max_z3fastmem = 0;
	} else {
		max_z3fastmem = natmem_size;
	}
	max_physmem = natmem_size;
	write_log (_T("MMAN: Reserved %p-%p (0x%08x %dM)\n"),
			   natmem_reserved, natmem_reserved + natmem_reserved_size,
			   natmem_reserved_size, natmem_reserved_size / (1024 * 1024));

	clear_shm ();

	canbang = true;
	return true;
}

static void resetmem (bool decommit)
{
	if (!shm_start)
		return;
	for (auto & shmid : shmids) {
		struct uae_shmid_ds *s = &shmid;
		int size = s->size;

		if (!s->attached)
			continue;
		if (!s->natmembase)
			continue;
		if (s->fake)
			continue;
		if (!decommit && (static_cast<uae_u8*>(s->attached) - static_cast<uae_u8*>(s->natmembase)) >= 0x10000000)
			continue;
		uae_u8* shmaddr = natmem_offset + (static_cast<uae_u8*>(s->attached) - static_cast<uae_u8*>(s->natmembase));
		if (decommit) {
			VirtualFree (shmaddr, size, MEM_DECOMMIT);
		} else {
			const uae_u8* result = virtualallocwithlock(shmaddr, size, decommit ? MEM_DECOMMIT : MEM_COMMIT, PAGE_READWRITE);
			if (result != shmaddr)
				write_log (_T("MMAN: realloc(%p-%p,%d,%d,%s) failed, err=%d\n"), shmaddr, shmaddr + size, size, s->mode, s->name, GetLastError ());
			else
				write_log (_T("MMAN: rellocated(%p-%p,%d,%s)\n"), shmaddr, shmaddr + size, size, s->name);
		}
	}
}

static uae_u8 *va (uae_u32 offset, uae_u32 len, unsigned int alloc, unsigned int protect)
{
	auto* addr = static_cast<uae_u8*>(VirtualAlloc(natmem_offset + offset, len, alloc, protect));
	if (addr) {
		write_log (_T("VA(%p - %p, %4uM, %s)\n"),
			natmem_offset + offset, natmem_offset + offset + len, len >> 20, (alloc & MEM_WRITE_WATCH) ? _T("WATCH") : _T("RESERVED"));
		return addr;
	}
	write_log (_T("VA(%p - %p, %4uM, %s) failed %d\n"),
		natmem_offset + offset, natmem_offset + offset + len, len >> 20, (alloc & MEM_WRITE_WATCH) ? _T("WATCH") : _T("RESERVED"), GetLastError ());
	return nullptr;
}

static int doinit_shm ()
{
	struct rtgboardconfig *rbc = &changed_prefs.rtgboards[0];
	struct rtgboardconfig *crbc = &currprefs.rtgboards[0];
	uae_u32 extra = 65536;
	struct uae_prefs *p = &changed_prefs;

	changed_prefs.z3autoconfig_start = currprefs.z3autoconfig_start = 0;
	set_expamem_z3_hack_mode(0);
	expansion_scan_autoconfig(&currprefs, true);

	canbang = true;
	natmem_offset = natmem_reserved;

	uae_u32 align = 16 * 1024 * 1024 - 1;
	uae_u32 totalsize = 0x01000000;

	uae_u32 z3rtgmem_size = gfxboard_get_configtype(rbc) == 3 ? rbc->rtgmem_size : 0;

	if (p->cpu_model >= 68020)
		totalsize = 0x10000000;
	totalsize += (p->z3chipmem.size + align) & ~align;
	uae_u32 totalsize_z3 = totalsize;

	start_rtg = 0;
	end_rtg = 0;

	jit_direct_compatible_memory = p->cachesize && (!p->comptrustbyte || !p->comptrustword || !p->comptrustlong);

	// 1G Z3chip?
	if ((Z3BASE_UAE + p->z3chipmem.size > Z3BASE_REAL) ||
		// real wrapped around
		(expamem_z3_highram_real == 0xffffffff) ||
		// Real highram > 0x80000000 && UAE highram <= 0x80000000 && Automatic
		(expamem_z3_highram_real > 0x80000000 && expamem_z3_highram_uae <= 0x80000000 && p->z3_mapping_mode == Z3MAPPING_AUTO) ||
		// Wanted UAE || Blizzard RAM
		p->z3_mapping_mode == Z3MAPPING_UAE || cpuboard_memorytype(&changed_prefs) == BOARD_MEMORY_BLIZZARD_12xx ||
		// JIT && Automatic && Real does not fit in NATMEM && UAE fits in NATMEM
		(expamem_z3_highram_real + extra >= natmem_reserved_size && expamem_z3_highram_uae + extra <= natmem_reserved_size && p->z3_mapping_mode == Z3MAPPING_AUTO && jit_direct_compatible_memory)) {
		changed_prefs.z3autoconfig_start = currprefs.z3autoconfig_start = Z3BASE_UAE;
		if (p->z3_mapping_mode == Z3MAPPING_AUTO)
			write_log(_T("MMAN: Selected UAE Z3 mapping mode\n"));
		set_expamem_z3_hack_mode(Z3MAPPING_UAE);
		totalsize_z3 = std::max(expamem_z3_highram_uae, totalsize_z3);
	} else {
		if (p->z3_mapping_mode == Z3MAPPING_AUTO)
			write_log(_T("MMAN: Selected REAL Z3 mapping mode\n"));
		changed_prefs.z3autoconfig_start = currprefs.z3autoconfig_start = Z3BASE_REAL;
		set_expamem_z3_hack_mode(Z3MAPPING_REAL);
		if (expamem_z3_highram_real > totalsize_z3 && jit_direct_compatible_memory) {
			totalsize_z3 = expamem_z3_highram_real;
			if (totalsize_z3 + extra >= natmem_reserved_size) {
				jit_direct_compatible_memory = false;
				write_log(_T("MMAN: Not enough direct memory for Z3REAL. Switching off JIT Direct.\n"));
			}
		}
	}
	write_log(_T("Total %uM Z3 Total %uM, HM %uM\n"), totalsize >> 20, totalsize_z3 >> 20, expamem_highmem_pointer >> 20);

	totalsize_z3 = std::max(totalsize_z3, expamem_highmem_pointer);

	expansion_scan_autoconfig(&currprefs, true);

	if (jit_direct_compatible_memory && (totalsize > size64 || totalsize + extra >= natmem_reserved_size)) {
		jit_direct_compatible_memory = false;
		write_log(_T("MMAN: Not enough direct memory. Switching off JIT Direct.\n"));
	}

	int idx = 0;
	for (;;) {
		struct autoconfig_info *aci = expansion_get_autoconfig_data(&currprefs, idx++);
		if (!aci)
			break;
		addrbank *ab = aci->addrbank;
		if (!ab)
			continue;
		if (aci->direct_vram && aci->start != 0xffffffff) {
			if (!start_rtg)
				start_rtg = aci->start;
			end_rtg = aci->start + aci->size;
		}
	}

	// rtg outside of natmem?
	if (start_rtg > 0 && start_rtg < 0xffffffff && end_rtg > natmem_reserved_size) {
		if (jit_direct_compatible_memory) {
			write_log(_T("MMAN: VRAM outside of natmem (%08x > %08x), switching off JIT Direct.\n"), end_rtg, natmem_reserved_size);
			jit_direct_compatible_memory = false;
		}
		if (end_rtg - start_rtg > natmem_reserved_size) {
			write_log(_T("MMAN: VRAMs don't fit in natmem space! (%08x > %08x)\n"), end_rtg - start_rtg, natmem_reserved_size);
			notify_user(NUMSG_NOMEMORY);
			return -1;
		}
#ifdef _WIN64
		// 64-bit can't do natmem_offset..
		notify_user(NUMSG_NOMEMORY);
		return -1;
#else

		p96base_offset = start_rtg;
		p96mem_size = end_rtg - start_rtg;
		write_log("MMAN: rtgbase_offset = %08x, size %08x\n", p96base_offset, p96mem_size);
		// adjust p96mem_offset to beginning of natmem
		// by subtracting start of original p96mem_offset from natmem_offset
		if (p96base_offset >= 0x10000000) {
			natmem_offset = natmem_reserved - p96base_offset;
			p96mem_offset = natmem_offset + p96base_offset;
		}
#endif
	} else {
		start_rtg = 0;
		end_rtg = 0;
	}

	idx = 0;
	for (;;) {
		struct autoconfig_info *aci = expansion_get_autoconfig_data(&currprefs, idx++);
		if (!aci)
			break;
		addrbank *ab = aci->addrbank;
		// disable JIT direct from Z3 boards that are outside of natmem
		for (auto & i : z3fastmem_bank) {
			if (&i == ab) {
				ab->flags &= ~ABFLAG_ALLOCINDIRECT;
				ab->jit_read_flag = 0;
				ab->jit_write_flag = 0;
				if (aci->start + aci->size > natmem_reserved_size) {
					write_log(_T("%s %08x-%08x: not JIT direct capable (>%08x)!\n"), ab->name, aci->start, aci->start + aci->size - 1, natmem_reserved_size);
					ab->flags |= ABFLAG_ALLOCINDIRECT;
					ab->jit_read_flag = S_READ;
					ab->jit_write_flag = S_WRITE;
				}
			}
		}
	}

	if (!natmem_offset) {
		write_log (_T("MMAN: No special area could be allocated! err=%d\n"), GetLastError ());
	} else {
		write_log(_T("MMAN: Our special area: %p-%p (0x%08x %dM)\n"),
			natmem_offset, natmem_offset + totalsize,
			totalsize, totalsize / (1024 * 1024));
		canbang = jit_direct_compatible_memory;
	}

	return canbang;
}

static uae_u32 oz3fastmem_size[MAX_RAM_BOARDS];
static uae_u32 ofastmem_size[MAX_RAM_BOARDS];
static uae_u32 oz3chipmem_size;
static uae_u32 ortgmem_size[MAX_RTG_BOARDS];
static int ortgmem_type[MAX_RTG_BOARDS];

bool init_shm()
{
	auto changed = false;

	for (auto i = 0; i < MAX_RAM_BOARDS; i++)
	{
		if (oz3fastmem_size[i] != changed_prefs.z3fastmem[i].size)
			changed = true;
		if (ofastmem_size[i] != changed_prefs.fastmem[i].size)
			changed = true;
	}
	for (auto i = 0; i < MAX_RTG_BOARDS; i++)
	{
		if (ortgmem_size[i] != changed_prefs.rtgboards[i].rtgmem_size)
			changed = true;
		if (ortgmem_type[i] != changed_prefs.rtgboards[i].rtgmem_type)
			changed = true;
	}
	if (!changed && oz3chipmem_size == changed_prefs.z3chipmem.size)
		return true;

	for (auto i = 0; i < MAX_RAM_BOARDS; i++)
	{
		oz3fastmem_size[i] = changed_prefs.z3fastmem[i].size;
		ofastmem_size[i] = changed_prefs.fastmem[i].size;
	}
	for (auto i = 0; i < MAX_RTG_BOARDS; i++)
	{
		ortgmem_size[i] = changed_prefs.rtgboards[i].rtgmem_size;
		ortgmem_type[i] = changed_prefs.rtgboards[i].rtgmem_type;
	}
	oz3chipmem_size = changed_prefs.z3chipmem.size;

	if (doinit_shm() < 0)
		return false;

	resetmem (false);
	clear_shm ();

	memory_hardreset(2);
	return true;
}

void free_shm ()
{
	resetmem (true);
	clear_shm ();
	for (int & i : ortgmem_type) {
		i = -1;
	}
}

void mapped_free (addrbank *ab)
{
	shmpiece *x = shm_start;
	bool rtgmem = (ab->flags & ABFLAG_RTG) != 0;

	ab->flags &= ~ABFLAG_MAPPED;
	if (ab->baseaddr == nullptr)
		return;

	if (ab->flags & ABFLAG_INDIRECT) {
		while(x) {
			if (ab->baseaddr == x->native_address) {
				int shmid = x->id;
				shmids[shmid].key = -1;
				shmids[shmid].name[0] = '\0';
				shmids[shmid].size = 0;
				shmids[shmid].attached = nullptr;
				shmids[shmid].mode = 0;
				shmids[shmid].natmembase = nullptr;
				if (!(ab->flags & ABFLAG_NOALLOC)) {
					xfree(ab->baseaddr);
					ab->baseaddr = nullptr;
				}
			}
			x = x->next;
		}
		ab->baseaddr = nullptr;
		ab->flags &= ~ABFLAG_DIRECTMAP;
		ab->allocated_size = 0;
		write_log(_T("mapped_free indirect %s\n"), ab->name);
		return;
	}

	if (!(ab->flags & ABFLAG_DIRECTMAP)) {
		if (!(ab->flags & ABFLAG_NOALLOC)) {
			xfree(ab->baseaddr);
		}
		ab->baseaddr = nullptr;
		ab->allocated_size = 0;
		write_log(_T("mapped_free nondirect %s\n"), ab->name);
		return;
	}

	while(x) {
		if(ab->baseaddr == x->native_address)
			uae_shmdt (x->native_address);
		x = x->next;
	}
	x = shm_start;
	while(x) {
		uae_shmid_ds blah{};
		if (ab->baseaddr == x->native_address) {
			if (uae_shmctl (x->id, UAE_IPC_STAT, &blah) == 0)
				uae_shmctl (x->id, UAE_IPC_RMID, &blah);
		}
		x = x->next;
	}
	ab->baseaddr = nullptr;
	ab->allocated_size = 0;
	write_log(_T("mapped_free direct %s\n"), ab->name);
}

static uae_key_t get_next_shmkey ()
{
	uae_key_t result = -1;
	for (int i = 0; i < MAX_SHMID; i++) {
		if (shmids[i].key == -1) {
			shmids[i].key = i;
			result = i;
			break;
		}
	}
	return result;
}

STATIC_INLINE uae_key_t find_shmkey (uae_key_t key)
{
	int result = -1;
	if(shmids[key].key == key) {
		result = key;
	}
	return result;
}

bool uae_mman_info(addrbank* ab, struct uae_mman_data* md)
{
	auto got = false;
	bool readonly = false;
	bool directsupport = true;
	uaecptr start;
	auto size = ab->reserved_size;
	auto readonlysize = size;
	auto barrier = false;

	if (!_tcscmp(ab->label, _T("*")))
	{
		start = ab->start;
		got = true;
		if (expansion_get_autoconfig_by_address(&currprefs, ab->start, 0)) {
			struct autoconfig_info* aci = expansion_get_autoconfig_by_address(&currprefs, ab->start + size, 0);
			if (!aci || aci->indirect) {
				barrier = true;
			}
		}
	}
	else if (!_tcscmp(ab->label, _T("*B")))
	{
		start = ab->start;
		got = true;
		barrier = true;
	}
	else if (!_tcscmp(ab->label, _T("chip")))
	{
		start = 0;
		got = true;
		if (!expansion_get_autoconfig_by_address(&currprefs, 0x00200000, 0) && currprefs.chipmem.size == 2 * 1024 * 1024)
			barrier = true;
		if (currprefs.chipmem.size > 2 * 1024 * 1024)
			barrier = true;
	}
	else if (!_tcscmp(ab->label, _T("kick")))
	{
		start = 0xf80000;
		got = true;
		barrier = true;
		readonly = true;
	}
	else if (!_tcscmp(ab->label, _T("rom_a8")))
	{
		start = 0xa80000;
		got = true;
		readonly = true;
	}
	else if (!_tcscmp(ab->label, _T("rom_e0")))
	{
		start = 0xe00000;
		got = true;
		readonly = true;
	}
	else if (!_tcscmp(ab->label, _T("rom_f0")))
	{
		start = 0xf00000;
		got = true;
		readonly = true;
	}
	else if (!_tcscmp(ab->label, _T("rom_f0_ppc")))
	{
		// this is flash and also contains IO
		start = 0xf00000;
		got = true;
		readonly = false;
	}
	else if (!_tcscmp(ab->label, _T("rtarea")))
	{
		start = rtarea_base;
		got = true;
		readonly = true;
		readonlysize = RTAREA_TRAPS;
	}
	else if (!_tcscmp(ab->label, _T("ramsey_low")))
	{
		start = a3000lmem_bank.start;
		if (!a3000hmem_bank.start)
			barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("csmk1_maprom")))
	{
		start = 0x07f80000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("25bitram")))
	{
		start = 0x01000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("ramsey_high")))
	{
		start = 0x08000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("dkb")))
	{
		start = 0x10000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("fusionforty")))
	{
		start = 0x11000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("blizzard_40")))
	{
		start = 0x40000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("blizzard_48")))
	{
		start = 0x48000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("blizzard_68")))
	{
		start = 0x68000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("blizzard_70")))
	{
		start = 0x70000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("cyberstorm")))
	{
		start = 0x0c000000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("cyberstormmaprom")))
	{
		start = 0xfff00000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("bogo")))
	{
		start = 0x00C00000;
		got = true;
		if (currprefs.bogomem.size <= 0x100000)
			barrier = true;
	}
	else if (!_tcscmp(ab->label, _T("custmem1")))
	{
		start = currprefs.custom_memory_addrs[0];
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("custmem2")))
	{
		start = currprefs.custom_memory_addrs[1];
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("hrtmem")))
	{
		start = 0x00a10000;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("arhrtmon")))
	{
		start = 0x00800000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("xpower_e2")))
	{
		start = 0x00e20000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("xpower_f2")))
	{
		start = 0x00f20000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("nordic_f0")))
	{
		start = 0x00f00000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("nordic_f4")))
	{
		start = 0x00f40000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("nordic_f6")))
	{
		start = 0x00f60000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("superiv_b0")))
	{
		start = 0x00b00000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("superiv_d0")))
	{
		start = 0x00d00000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("superiv_e0")))
	{
		start = 0x00e00000;
		barrier = true;
		got = true;
	}
	else if (!_tcscmp(ab->label, _T("ram_a8")))
	{
		start = 0x00a80000;
		barrier = true;
		got = true;
	}
	else
	{
		directsupport = false;
	}
	if (got)
	{
		bool maprom = false;
		md->start = start;
		md->size = size;
		md->readonly = readonly;
		md->readonlysize = readonlysize;
		md->maprom = maprom;
		md->hasbarrier = barrier;

		if (start_rtg && end_rtg) {
			if (start < start_rtg || start + size > end_rtg)
				directsupport = false;
		} else if (start >= natmem_reserved_size || start + size > natmem_reserved_size) {
			// start + size may cause 32-bit overflow
			directsupport = false;
		}
		md->directsupport = directsupport;
		if (md->hasbarrier) {
			md->size += BARRIER;
		}
	}
	return got;
}

void *uae_shmat (addrbank *ab, int shmid, void *shmaddr, int shmflg, struct uae_mman_data *md)
{
	void *result = reinterpret_cast<void*>(-1);
	bool readonly = false, maprom = false;
	int p96special = FALSE;
	struct uae_mman_data md2{};

#ifdef NATMEM_OFFSET

	unsigned int size = shmids[shmid].size;
	unsigned int readonlysize = size;

	if (shmids[shmid].attached)
		return shmids[shmid].attached;

	if (ab->flags & ABFLAG_INDIRECT) {
		shmids[shmid].attached = ab->baseaddr;
		shmids[shmid].fake = true;
		return shmids[shmid].attached;
	}

	if (static_cast<uae_u8*>(shmaddr) < natmem_offset) {
		if (!md) {
			if (!uae_mman_info(ab, &md2))
				return nullptr;
			md = &md2;
		}
		if (!shmaddr) {
			shmaddr = natmem_offset + md->start;
			size = md->size;
			readonlysize = md->readonlysize;
			readonly = md->readonly;
			maprom = md->maprom;
		}
	}

	uintptr_t natmem_end = reinterpret_cast<uintptr_t>(natmem_reserved) + natmem_reserved_size;
	if (md && md->hasbarrier && reinterpret_cast<uintptr_t>(shmaddr) + size > natmem_end && reinterpret_cast<uintptr_t>(shmaddr) <= natmem_end) {
		/* We cannot add a barrier beyond the end of the reserved memory. */
		//assert((uintptr_t) shmaddr + size - natmem_end == BARRIER);
		write_log(_T("NATMEM: Removing barrier (%d bytes) beyond reserved memory\n"), BARRIER);
		size -= BARRIER;
		md->size -= BARRIER;
		md->hasbarrier = false;
	}

#endif

	if (shmids[shmid].key == shmid && shmids[shmid].size) {
		unsigned int protect = readonly ? PAGE_READONLY : PAGE_READWRITE;
		shmids[shmid].mode = protect;
		shmids[shmid].rosize = readonlysize;
		shmids[shmid].natmembase = natmem_offset;
		shmids[shmid].maprom = maprom ? 1 : 0;
		if (shmaddr)
			virtualfreewithlock (shmaddr, size, MEM_DECOMMIT);
		result = virtualallocwithlock (shmaddr, size, MEM_COMMIT, PAGE_READWRITE);
		if (result == nullptr)
			virtualfreewithlock (shmaddr, 0, MEM_DECOMMIT);
		result = virtualallocwithlock (shmaddr, size, MEM_COMMIT, PAGE_READWRITE);
		if (result == nullptr) {
			result = reinterpret_cast<void*>(-1);
			error_log (_T("Memory %s (%s) failed to allocate %p: VA %08X - %08X %x (%dk). Error %d."),
				shmids[shmid].name, ab ? ab->name : _T("?"), shmaddr,
				static_cast<uae_u8*>(shmaddr) - natmem_offset, static_cast<uae_u8*>(shmaddr) - natmem_offset + size,
				size, size >> 10, GetLastError ());
		} else {
			shmids[shmid].attached = result;
			write_log (_T("%p: VA %08lX - %08lX %x (%dk) ok (%p)%s\n"),
				shmaddr, static_cast<uae_u8*>(shmaddr) - natmem_offset, static_cast<uae_u8*>(shmaddr) - natmem_offset + size,
				size, size >> 10, shmaddr, p96special ? _T(" RTG") : _T(""));
		}
	}
	return result;
}

void unprotect_maprom()
{
	bool protect = false;
	for (auto & shmid : shmids) {
		struct uae_shmid_ds *shm = &shmid;
		if (shm->mode != PAGE_READONLY)
			continue;
		if (!shm->attached || !shm->rosize)
			continue;
		if (shm->maprom <= 0)
			continue;
		shm->maprom = -1;
		unsigned int old;
		if (!VirtualProtect (shm->attached, shm->rosize, protect ? PAGE_READONLY : PAGE_READWRITE, &old)) {
			write_log (_T("unprotect_maprom VP %08lX - %08lX %x (%dk) failed %d\n"),
				static_cast<uae_u8*>(shm->attached) - natmem_offset, static_cast<uae_u8*>(shm->attached) - natmem_offset + shm->size,
				shm->size, shm->size >> 10, GetLastError ());
		}
	}
}

void protect_roms(bool protect)
{
	if (protect) {
		// protect only if JIT enabled, always allow unprotect
		if (!currprefs.cachesize || currprefs.comptrustbyte || currprefs.comptrustword || currprefs.comptrustlong)
			return;
	}
	for (auto & shmid : shmids) {
		struct uae_shmid_ds *shm = &shmid;
		if (shm->mode != PAGE_READONLY)
			continue;
		if (!shm->attached || !shm->rosize)
			continue;
		if (shm->maprom < 0 && protect)
			continue;
		unsigned int old;
		if (!VirtualProtect (shm->attached, shm->rosize, protect ? PAGE_READONLY : PAGE_READWRITE, &old)) {
			write_log (_T("protect_roms VP %08lX - %08lX %x (%dk) failed %d\n"),
				static_cast<uae_u8*>(shm->attached) - natmem_offset, static_cast<uae_u8*>(shm->attached) - natmem_offset + shm->rosize,
				shm->rosize, shm->rosize >> 10, GetLastError ());
		} else {
			write_log(_T("ROM VP %08lX - %08lX %x (%dk) %s\n"),
				static_cast<uae_u8*>(shm->attached) - natmem_offset, static_cast<uae_u8*>(shm->attached) - natmem_offset + shm->rosize,
				shm->rosize, shm->rosize >> 10, protect ? _T("WPROT") : _T("UNPROT"));
		}
	}
}

// Mark indirect regions (indirect VRAM) as non-accessible when JIT direct is active.
// Beginning of region might have barrier region which is not marked as non-accessible,
// allowing JIT direct to think it is directly accessible VRAM.
void mman_set_barriers(bool disable)
{
#ifndef AMIBERRY
#ifdef JIT
	addrbank* abprev = NULL;
	int maxbank = currprefs.address_space_24 ? MEMORY_BANKS_24 : MEMORY_BANKS;
	for (int i = 0; i < maxbank; i++) {
		uaecptr addr = i * 0x10000;
		addrbank* ab = &get_mem_bank(addr);
		if (ab == abprev) {
			continue;
		}
		int size = 0x10000;
		for (int j = i + 1; j < maxbank; j++) {
			uaecptr addr2 = j * 0x10000;
			addrbank* ab2 = &get_mem_bank(addr2);
			if (ab2 != ab) {
				break;
			}
			size += 0x10000;
		}
		abprev = ab;
		if (ab && ab->baseaddr == NULL && (ab->flags & ABFLAG_ALLOCINDIRECT)) {
			unsigned int old;
			if (disable || !currprefs.cachesize || currprefs.comptrustbyte || currprefs.comptrustword || currprefs.comptrustlong) {
				if (!ab->protectmode) {
					ab->protectmode = PAGE_READWRITE;
				}
				if (!VirtualProtect(addr + natmem_offset, size, ab->protectmode, &old)) {
					size = 0x1000;
					VirtualProtect(addr + natmem_offset, size, ab->protectmode, &old);
				}
				write_log("%08x-%08x = access restored (%08x)\n", addr, size, ab->protectmode);
			} else {
				if (VirtualProtect(addr + natmem_offset, size, PAGE_NOACCESS, &old)) {
					ab->protectmode = old;
					write_log("%08x-%08x = set to no access\n", addr, addr + size);
				} else {
					size = 0x1000;
					if (VirtualProtect(addr + natmem_offset, size, PAGE_NOACCESS, &old)) {
						ab->protectmode = old;
						write_log("%08x-%08x = set to no access\n", addr, addr + size);
					}
				}
			}
		}
	}
#endif
#endif
}

int uae_shmdt (const void *shmaddr)
{
	return 0;
}

int uae_shmget (uae_key_t key, const addrbank *ab, int shmflg)
{
	int result = -1;

	if ((key == UAE_IPC_PRIVATE) || ((shmflg & UAE_IPC_CREAT) && (find_shmkey (key) == -1))) {
		write_log (_T("shmget of size %zd (%zdk) for %s (%s)\n"), ab->reserved_size, ab->reserved_size >> 10, ab->label, ab->name);
		if ((result = get_next_shmkey ()) != -1) {
			shmids[result].size = ab->reserved_size;
			_tcscpy (shmids[result].name, ab->label);
		} else {
			result = -1;
		}
	}
	return result;
}

int uae_shmctl (int shmid, int cmd, struct uae_shmid_ds *buf)
{
	int result = -1;

	if ((find_shmkey (shmid) != -1) && buf) {
		switch (cmd)
		{
		case UAE_IPC_STAT:
			*buf = shmids[shmid];
			result = 0;
			break;
		case UAE_IPC_RMID:
			VirtualFree (shmids[shmid].attached, shmids[shmid].size, MEM_DECOMMIT);
			shmids[shmid].key = -1;
			shmids[shmid].name[0] = '\0';
			shmids[shmid].size = 0;
			shmids[shmid].attached = nullptr;
			shmids[shmid].mode = 0;
			result = 0;
			break;
		default: /* Invalid command */
			break;
		}
	}
	return result;
}
