#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "autoconf.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "custom.h"
#include "memory.h"
#include <sys/mman.h>
#include <SDL.h>


uae_u8* natmem_offset = 0;
uae_u32 natmem_size;
static uae_u64 totalAmigaMemSize;
#define MAXAMIGAMEM 0x6000000 // 64 MB (16 MB for standard Amiga stuff, 16 MG RTG, 64 MB Z3 fast)


/*
void TestMemMap(void)
{
  void *starting = (void *)0x60000000;
  void *base;
  void *z3fast;
  void *gfx;
  
  base = mmap(starting, 0x1000000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  printf("TEST: adr for first 16MB: 0x%08x\n", base);
  if(base != MAP_FAILED)
  {
    z3fast = mmap((void *)((int)base + 0x10000000), 0x2000000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("TEST: adr for z3fast 32MB: 0x%08x\n", z3fast);
    if(z3fast != MAP_FAILED)
    {
      gfx = mmap((void *)((int)base + 0x12000000), 0x800000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      printf("TEST: adr for gfx 8MB: 0x%08x\n", gfx);
      if(gfx != MAP_FAILED)
      {
        munmap(gfx, 0x800000);
      }
      munmap(z3fast, 0x2000000);
    }
    munmap(base, 0x1000000);
  }
}
*/


void free_AmigaMem(void)
{
  if(natmem_offset != 0)
  {
    free(natmem_offset);
    natmem_offset = 0;
  }
}


void alloc_AmigaMem(void)
{
	int i;
	uae_u64 total;
	int max_allowed_mman;

  free_AmigaMem();

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
	natmem_offset = (uae_u8*)valloc (natmem_size);

	if (!natmem_offset) {
		for (;;) {
			natmem_offset = (uae_u8*)valloc (natmem_size);
			if (natmem_offset)
				break;
			natmem_size -= 16 * 1024 * 1024;
			if (!natmem_size) {
				write_log("Can't allocate 16M of virtual address space!?\n");
        abort();
			}
		}
	}
	max_z3fastmem = natmem_size - 32 * 1024 * 1024;
	if(max_z3fastmem < 0)
	  max_z3fastmem = 0;
	write_log("Reserved: %p-%p (0x%08x %dM)\n", natmem_offset, (uae_u8*)natmem_offset + natmem_size,
		natmem_size, natmem_size >> 20);
}


uae_u8 *mapped_malloc (size_t s, const char *file)
{
  if(!strcmp(file, "chip"))
    return natmem_offset;

  if(!strcmp(file, "fast"))
    return natmem_offset + 0x200000;

  if(!strcmp(file, "bogo"))
    return natmem_offset + bogomem_start;

  if(!strcmp(file, "rom_f0"))
    return natmem_offset + 0xf00000;
    
  if(!strcmp(file, "rom_e0"))
    return natmem_offset + 0xe00000;

  if(!strcmp(file, "rom_a8"))
    return natmem_offset + 0xa80000;

  if(!strcmp(file, "kick"))
    return natmem_offset + kickmem_start;

  if(!strcmp(file, "z3"))
    return natmem_offset + 0x1000000; //z3fastmem_start;
#ifdef PICASSO96
  if(!strcmp(file, "gfx"))
  {
    p96ram_start = 0x3000000;
    return natmem_offset + p96ram_start;
  }
#endif
  if(!strcmp(file, "rtarea"))
    return natmem_offset + rtarea_base;

  return NULL;
}


void mapped_free (uae_u8 *p)
{
}
