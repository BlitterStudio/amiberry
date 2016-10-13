#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "autoconf.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "akiko.h"
#include <sys/mman.h>
#include <SDL.h>


uae_u8* natmem_offset = 0;
uae_u32 natmem_size;
static uae_u64 totalAmigaMemSize;
#define MAXAMIGAMEM 0x6000000 // 64 MB (16 MB for standard Amiga stuff, 16 MG RTG, 64 MB Z3 fast)
uae_u32 max_z3fastmem;

/* JIT can access few bytes outside of memory block of it executes code at the very end of memory block */
#define BARRIER 32

static uae_u8* additional_mem = (uae_u8*) MAP_FAILED;
#define ADDITIONAL_MEMSIZE (128 + 16) * 1024 * 1024

int z3_start_adr = 0;
int rtg_start_adr = 0;


void free_AmigaMem(void)
{
  if(natmem_offset != 0)
  {
    free(natmem_offset);
    natmem_offset = 0;
  }
  if(additional_mem != MAP_FAILED)
  {
    munmap(additional_mem, ADDITIONAL_MEMSIZE);
    additional_mem = (uae_u8*) MAP_FAILED;
  }
}


void alloc_AmigaMem(void)
{
	int i;
	uae_u64 total;
	int max_allowed_mman;

  free_AmigaMem();

  // First attempt: allocate 16 MB for all memory in 24-bit area 
  // and additional mem for Z3 and RTG at correct offset
  natmem_size = 16 * 1024 * 1024;
  natmem_offset = (uae_u8*)valloc (natmem_size);
  max_z3fastmem = ADDITIONAL_MEMSIZE - (16 * 1024 * 1024);
	if (!natmem_offset) {
		write_log("Can't allocate 16M of virtual address space!?\n");
    abort();
	}
  additional_mem = (uae_u8*) mmap(natmem_offset + 0x10000000, ADDITIONAL_MEMSIZE + BARRIER,
    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  if(additional_mem != MAP_FAILED)
  {
    // Allocation successful -> we can use natmem_offset for entire memory access
    z3_start_adr = 0x10000000;
    rtg_start_adr = 0x18000000;
    write_log("Allocated 16 MB for 24-bit area and %d MB for Z3 and RTG\n", ADDITIONAL_MEMSIZE / (1024 * 1024));
    return;
  }
  free(natmem_offset);
  
  // Second attempt: allocate huge memory block for entire area
  natmem_size = ADDITIONAL_MEMSIZE + 256 * 1024 * 1024;
  natmem_offset = (uae_u8*)valloc (natmem_size + BARRIER);
  if(natmem_offset)
  {
    // Allocation successful
    z3_start_adr = 0x10000000;
    rtg_start_adr = 0x18000000;
    write_log("Allocated %d MB for entire memory\n", natmem_size / (1024 * 1024));
    return;
  }

  // Third attempt: old style: 64 MB allocated and Z3 and RTG at wrong address

  // Get max. available size
	total = (uae_u64)sysconf (_SC_PHYS_PAGES) * (uae_u64)getpagesize();
  
  // Limit to max. 64 MB
	natmem_size = total;
	if (natmem_size > MAXAMIGAMEM)
		natmem_size = MAXAMIGAMEM;

  // We need at least 16 MB
	if (natmem_size < 16 * 1024 * 1024)
		natmem_size = 16 * 1024 * 1024;

	write_log("Total physical RAM %lluM. Attempting to reserve: %uM.\n", total >> 20, natmem_size >> 20);
	natmem_offset = (uae_u8*)valloc (natmem_size + BARRIER);

	if (!natmem_offset) {
		for (;;) {
			natmem_offset = (uae_u8*)valloc (natmem_size + BARRIER);
			if (natmem_offset)
				break;
			natmem_size -= 16 * 1024 * 1024;
			if (!natmem_size) {
				write_log("Can't allocate 16M of virtual address space!?\n");
        abort();
			}
		}
	}

  z3_start_adr = 0x01000000;
  rtg_start_adr = 0x03000000;
	max_z3fastmem = natmem_size - 32 * 1024 * 1024;
	if(max_z3fastmem <= 0)
  {
    z3_start_adr = 0x00000000; // No mem for Z3
    if(max_z3fastmem == 0)
      rtg_start_adr = 0x01000000; // We have mem for RTG
    else
      rtg_start_adr = 0x00000000; // No mem for expansion at all
	  max_z3fastmem = 0;
	}
	write_log("Reserved: %p-%p (0x%08x %dM)\n", natmem_offset, (uae_u8*)natmem_offset + natmem_size,
		natmem_size, natmem_size >> 20);
}


static uae_u32 getz2rtgaddr (void)
{
	uae_u32 start;
	start = currprefs.fastmem_size;
	while (start & (currprefs.rtgmem_size - 1) && start < 4 * 1024 * 1024)
		start += 1024 * 1024;
	return start + 2 * 1024 * 1024;
}


uae_u8 *mapped_malloc (size_t s, const char *file)
{
  if(!strcmp(file, "chip"))
    return natmem_offset + chipmem_start_addr;

  if(!strcmp(file, "fast"))
    return natmem_offset + 0x200000;

  if(!strcmp(file, "bogo"))
    return natmem_offset + bogomem_start_addr;

  if(!strcmp(file, "rom_f0"))
    return natmem_offset + 0xf00000;
    
  if(!strcmp(file, "rom_e0"))
    return natmem_offset + 0xe00000;

  if(!strcmp(file, "rom_a8"))
    return natmem_offset + 0xa80000;

  if(!strcmp(file, "kick"))
    return natmem_offset + kickmem_start_addr;

  if(!strcmp(file, "z3"))
    return natmem_offset + z3_start_adr; //z3fastmem_start;

  if(!strcmp(file, "z3_gfx"))
  {
    gfxmem_bank.start = rtg_start_adr;
    return natmem_offset + rtg_start_adr;
  }

  if(!strcmp(file, "z2_gfx"))
  {
    gfxmem_bank.start = getz2rtgaddr();
    return natmem_offset + gfxmem_bank.start;
  }

  if(!strcmp(file, "rtarea"))
    return natmem_offset + rtarea_base;

  return NULL;
}


void mapped_free (uae_u8 *p)
{
}


void protect_roms (bool protect)
{
/*
  If this code is enabled, we can't switch back from JIT to nonJIT emulation...
  
	if (protect) {
		// protect only if JIT enabled, always allow unprotect
		if (!currprefs.cachesize)
			return;
	}

  // Protect all regions, which contains ROM
  if(extendedkickmem_bank.baseaddr != NULL)
    mprotect(extendedkickmem_bank.baseaddr, 0x80000, protect ? PROT_READ : PROT_READ | PROT_WRITE);
  if(extendedkickmem2_bank.baseaddr != NULL)
    mprotect(extendedkickmem2_bank.baseaddr, 0x80000, protect ? PROT_READ : PROT_READ | PROT_WRITE);
  if(kickmem_bank.baseaddr != NULL)
    mprotect(kickmem_bank.baseaddr, 0x80000, protect ? PROT_READ : PROT_READ | PROT_WRITE);
  if(rtarea != NULL)
    mprotect(rtarea, RTAREA_SIZE, protect ? PROT_READ : PROT_READ | PROT_WRITE);
  if(filesysory != NULL)
    mprotect(filesysory, 0x10000, protect ? PROT_READ : PROT_READ | PROT_WRITE);
*/
}
