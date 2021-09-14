#include <cstdlib>
#include "string.h"

#include "sysdeps.h"
#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "autoconf.h"
#include "uae/mman.h"
#include <sys/mman.h>
#include "sys/types.h"
#include "sys/sysinfo.h"

#ifdef ANDROID
#define valloc(x) memalign(getpagesize(), x)
#endif

static uae_u32 natmem_size;
uae_u32 max_z3fastmem;

/* JIT can access few bytes outside of memory block of it executes code at the very end of memory block */
#define BARRIER 32

static uae_u8* additional_mem = static_cast<uae_u8*>(MAP_FAILED);
#define MAX_RTG_MEM (128 * 1024 * 1024)
#define ADDITIONAL_MEMSIZE (max_z3fastmem + MAX_RTG_MEM)

static uae_u8* a3000_mem = static_cast<uae_u8*>(MAP_FAILED);
static unsigned int a3000_totalsize = 0;
#define A3000MEM_START 0x08000000

static unsigned int last_low_size = 0;
static unsigned int last_high_size = 0;
int z3_base_adr = 0;

void free_AmigaMem(void)
{
	if (regs.natmem_offset != nullptr)
	{
#ifdef AMIBERRY
		munmap(regs.natmem_offset, natmem_size + BARRIER);
#else
		free(regs.natmem_offset);
#endif
		regs.natmem_offset = nullptr;
	}
	if (additional_mem != MAP_FAILED)
	{
		munmap(additional_mem, ADDITIONAL_MEMSIZE + BARRIER);
		additional_mem = static_cast<uae_u8*>(MAP_FAILED);
	}
	if (a3000_mem != MAP_FAILED)
	{
		munmap(a3000_mem, a3000_totalsize);
		a3000_mem = static_cast<uae_u8*>(MAP_FAILED);
		a3000_totalsize = 0;
	}
}

bool can_have_1gb()
{
	struct sysinfo mem_info{};
	sysinfo(&mem_info);
	long long total_phys_mem = mem_info.totalram;
	total_phys_mem *= mem_info.mem_unit;
	// Do we have more than 2GB in the system?
	if (total_phys_mem > 2147483648LL)
		return true;
	return false;
}

void alloc_AmigaMem(void)
{
	free_AmigaMem();
	set_expamem_z3_hack_mode(Z3MAPPING_AUTO);

// This crashes on Android
#ifndef ANDROID
	// First attempt: allocate 16 MB for all memory in 24-bit area
	// and additional mem for Z3 and RTG at correct offset
	natmem_size = 16 * 1024 * 1024;
#ifdef AMIBERRY
	// address returned by valloc() too high for later mmap() calls. Use mmap() also for first area.
	regs.natmem_offset = static_cast<uae_u8*>(mmap(reinterpret_cast<void *>(0x20000000), natmem_size + BARRIER,
	                                               PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0));
#else
	regs.natmem_offset = (uae_u8*)valloc(natmem_size + BARRIER);
#endif

	if (can_have_1gb())
		max_z3fastmem = 1024 * 1024 * 1024;
	else
		max_z3fastmem = 512 * 1024 * 1024;

	if (!regs.natmem_offset)
	{
		write_log("Can't allocate 16M of virtual address space!?\n");
		abort();
	}
	additional_mem = static_cast<uae_u8*>(mmap(regs.natmem_offset + Z3BASE_REAL, ADDITIONAL_MEMSIZE + BARRIER,
	                                           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0));
	if (additional_mem != MAP_FAILED)
	{
		// Allocation successful -> we can use natmem_offset for entire memory access at real address
		changed_prefs.z3autoconfig_start = currprefs.z3autoconfig_start = Z3BASE_REAL;
		z3_base_adr = Z3BASE_REAL;
#if defined(CPU_AARCH64)
		write_log("Allocated 16 MB for 24-bit area (0x%016lx) and %d MB for Z3 and RTG at real address (0x%016lx - 0x%016lx)\n",
			regs.natmem_offset, ADDITIONAL_MEMSIZE / (1024 * 1024), additional_mem, additional_mem + ADDITIONAL_MEMSIZE + BARRIER);
#else
		write_log("Allocated 16 MB for 24-bit area (0x%08x) and %d MB for Z3 and RTG at real address (0x%08x - 0x%08x)\n",
			regs.natmem_offset, ADDITIONAL_MEMSIZE / (1024 * 1024), additional_mem, additional_mem + ADDITIONAL_MEMSIZE + BARRIER
		);
#endif
		set_expamem_z3_hack_mode(Z3MAPPING_REAL);
		return;
	}

	additional_mem = static_cast<uae_u8*>(mmap(regs.natmem_offset + Z3BASE_UAE, ADDITIONAL_MEMSIZE + BARRIER,
	                                           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0));
	if (additional_mem != MAP_FAILED)
	{
		// Allocation successful -> we can use natmem_offset for entire memory access at fake address
		changed_prefs.z3autoconfig_start = currprefs.z3autoconfig_start = Z3BASE_UAE;
		z3_base_adr = Z3BASE_UAE;
#if defined(CPU_AARCH64)
		write_log("Allocated 16 MB for 24-bit area (0x%016lx) and %d MB for Z3 and RTG at fake address (0x%016lx - 0x%016lx)\n",
			regs.natmem_offset, ADDITIONAL_MEMSIZE / (1024 * 1024), additional_mem, additional_mem + ADDITIONAL_MEMSIZE + BARRIER);
#else
		write_log("Allocated 16 MB for 24-bit area (0x%08x) and %d MB for Z3 and RTG at fake address (0x%08x - 0x%08x)\n",
			regs.natmem_offset, ADDITIONAL_MEMSIZE / (1024 * 1024), additional_mem, additional_mem + ADDITIONAL_MEMSIZE + BARRIER);
#endif
		set_expamem_z3_hack_mode(Z3MAPPING_UAE);
		return;
	}
#ifdef AMIBERRY
	munmap(regs.natmem_offset, natmem_size + BARRIER);
#else
	free(regs.natmem_offset);
#endif

	// Next attempt: allocate huge memory block for entire area
	natmem_size = ADDITIONAL_MEMSIZE + 256 * 1024 * 1024;
	regs.natmem_offset = static_cast<uae_u8*>(valloc(natmem_size + BARRIER));
	if (regs.natmem_offset)
	{
		// Allocation successful
		changed_prefs.z3autoconfig_start = currprefs.z3autoconfig_start = Z3BASE_UAE;
		z3_base_adr = Z3BASE_UAE;
		write_log("Allocated %d MB for entire memory\n", natmem_size / (1024 * 1024));
#if defined(CPU_AARCH64)
		if (((uae_u64)(regs.natmem_offset + natmem_size + BARRIER) & 0xffffffff00000000) != 0)
			write_log("Memory address is higher than 32 bit. JIT will crash\n");
#endif
		return;
	}
#endif

	// No mem for Z3 or RTG at all
	natmem_size = 16 * 1024 * 1024;
	regs.natmem_offset = static_cast<uae_u8*>(valloc(natmem_size + BARRIER));

	if (!regs.natmem_offset)
	{
		write_log("Can't allocate 16M of virtual address space!?\n");
		abort();
	}

	changed_prefs.z3autoconfig_start = currprefs.z3autoconfig_start = 0x00000000; // No mem for Z3
	z3_base_adr = 0x00000000;
	max_z3fastmem = 0;

	write_log("Reserved: %p-%p (0x%08x %dM)\n", regs.natmem_offset, static_cast<uae_u8*>(regs.natmem_offset) + natmem_size,
		natmem_size, natmem_size >> 20);
#if defined(CPU_AARCH64)
	if (((uae_u64)(regs.natmem_offset + natmem_size + BARRIER) & 0xffffffff00000000) != 0)
		write_log("Memory address is higher than 32 bit. JIT will crash\n");
#endif
}

static bool HandleA3000Mem(unsigned int lowsize, unsigned int highsize)
{
	auto result = true;

	if (lowsize == last_low_size && highsize == last_high_size)
		return result;

	if (a3000_mem != MAP_FAILED)
	{
		write_log("HandleA3000Mem(): Free A3000 memory (0x%08x). %d MB.\n", a3000_mem, a3000_totalsize / (1024 * 1024));
		munmap(a3000_mem, a3000_totalsize);
		a3000_mem = static_cast<uae_u8*>(MAP_FAILED);
		a3000_totalsize = 0;
		last_low_size = 0;
		last_high_size = 0;
	}
	if (lowsize + highsize > 0)
	{
		// Try to get memory for A3000 motherboard
		write_log("Try to get A3000 memory at correct place (0x%08x). %d MB and %d MB.\n", A3000MEM_START,
			lowsize / (1024 * 1024), highsize / (1024 * 1024));
		a3000_totalsize = lowsize + highsize;
		a3000_mem = static_cast<uae_u8*>(mmap(regs.natmem_offset + (A3000MEM_START - lowsize), a3000_totalsize,
		                                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0));
		if (a3000_mem != MAP_FAILED)
		{
			last_low_size = lowsize;
			last_high_size = highsize;
			write_log(_T("Succeeded: location at 0x%08x (Amiga: 0x%08x)\n"), a3000_mem, (A3000MEM_START - lowsize));
		}
		else
		{
			write_log("Failed.\n");
			a3000_totalsize = 0;
			result = false;
		}
	}

	return result;
}

static bool A3000MemAvailable(void)
{
	return a3000_mem != MAP_FAILED;
}

bool uae_mman_info(addrbank* ab, struct uae_mman_data* md)
{
	auto got = false;
	auto readonly = false;
	uaecptr start;
	auto size = ab->reserved_size;
	auto readonlysize = size;
	auto barrier = false;

	if (!_tcscmp(ab->label, _T("*")))
	{
		start = ab->start;
		got = true;
		if (expansion_get_autoconfig_by_address(&currprefs, ab->start, 0) && !expansion_get_autoconfig_by_address(&currprefs, ab->start + size, 0))
			barrier = true;
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
		if (ab->reserved_size != last_low_size)
			HandleA3000Mem(ab->reserved_size, last_high_size);
		if (A3000MemAvailable())
		{
			start = a3000lmem_bank.start;
			got = true;
		}
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
		if (ab->reserved_size != last_high_size)
			HandleA3000Mem(last_low_size, ab->reserved_size);
		if (A3000MemAvailable())
		{
			start = 0x08000000;
			got = true;
		}
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
	if (got)
	{
		md->start = start;
		md->size = size;
		md->readonly = readonly;
		md->readonlysize = readonlysize;
		md->hasbarrier = barrier;

		if (md->hasbarrier)
		{
			md->size += BARRIER;
		}
	}
	return got;
}

bool mapped_malloc(addrbank* ab)
{
	if (ab->allocated_size)
	{
		write_log(_T("mapped_malloc with memory bank '%s' already allocated!?\n"), ab->name);
	}
	ab->allocated_size = 0;

	if (ab->label && ab->label[0] == '*')
	{
		if (ab->start == 0 || ab->start == 0xffffffff)
		{
			write_log(_T("mapped_malloc(*) without start address!\n"));
			return false;
		}
	}

	struct uae_mman_data md = {0};
	if (uae_mman_info(ab, &md))
	{
		const auto start = md.start;
		ab->baseaddr = regs.natmem_offset + start;
	}

	if (ab->baseaddr)
	{
		if (md.hasbarrier)
		{
			// fill end of ram with ILLEGAL to catch direct PC falling out of RAM.
			put_long_host(ab->baseaddr + ab->reserved_size, 0x4afc4afc);
		}
		ab->startaccessmask = ab->start & ab->mask;
		ab->allocated_size = ab->reserved_size;
		write_log("mapped_malloc(): 0x%08x - 0x%08x (0x%08x - 0x%08x) -> %s (%s)\n",
			ab->baseaddr - regs.natmem_offset, ab->baseaddr - regs.natmem_offset + ab->allocated_size,
			ab->baseaddr, ab->baseaddr + ab->allocated_size, ab->name, ab->label);
	}
	ab->flags |= ABFLAG_DIRECTMAP;

	return ab->baseaddr != nullptr;
}

void mapped_free(addrbank* ab)
{
	ab->flags &= ~ABFLAG_MAPPED;
	if (ab->baseaddr == nullptr)
		return;
	
	if (ab->label != nullptr && !strcmp(ab->label, "filesys") && ab->baseaddr != nullptr)
	{		
		write_log("mapped_free(): 0x%08x - 0x%08x (0x%08x - 0x%08x) -> %s (%s)\n",
			ab->baseaddr - regs.natmem_offset, ab->baseaddr - regs.natmem_offset + ab->allocated_size,
			ab->baseaddr, ab->baseaddr + ab->allocated_size, ab->name, ab->label);
		free(ab->baseaddr);
	}
	ab->baseaddr = nullptr;
	ab->allocated_size = 0;
}

void protect_roms(bool protect)
{
	//If this code is enabled, we can't switch back from JIT to nonJIT emulation...

	//if (protect) {
	//	// protect only if JIT enabled, always allow unprotect
	//	if (!currprefs.cachesize || currprefs.comptrustbyte || currprefs.comptrustword || currprefs.comptrustlong)
	//		return;
	//}

	// Protect all regions, which contains ROM
	//if (extendedkickmem_bank.baseaddr != NULL)
		//mprotect(extendedkickmem_bank.baseaddr, 0x80000, protect ? PROT_READ : PROT_READ | PROT_WRITE);
	//if (extendedkickmem2_bank.baseaddr != NULL)
		//mprotect(extendedkickmem2_bank.baseaddr, 0x80000, protect ? PROT_READ : PROT_READ | PROT_WRITE);
	//if (kickmem_bank.baseaddr != NULL)
		//mprotect(kickmem_bank.baseaddr, 0x80000, protect ? PROT_READ : PROT_READ | PROT_WRITE);
	//if (rtarea != NULL)
	//	mprotect(rtarea, RTAREA_SIZE, protect ? PROT_READ : PROT_READ | PROT_WRITE);
	//if (filesysory != NULL)
	//	mprotect(filesysory, 0x10000, protect ? PROT_READ : PROT_READ | PROT_WRITE);

}

static int doinit_shm(void)
{
	expansion_scan_autoconfig(&currprefs, true);

	return 1;
}

static uae_u32 oz3fastmem_size[MAX_RAM_BOARDS];
static uae_u32 ofastmem_size[MAX_RAM_BOARDS];
static uae_u32 ortgmem_size[MAX_RTG_BOARDS];
static int ortgmem_type[MAX_RTG_BOARDS];

bool init_shm(void)
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
	if (!changed)
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

	if (doinit_shm() < 0)
		return false;

	memory_hardreset(2);
	return true;
}

void free_shm (void)
{
	for (auto& i : ortgmem_type)
	{
		i = -1;
	}
}
