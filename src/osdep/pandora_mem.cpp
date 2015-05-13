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

const int totalAmigaMemSize = 0x1000000;

void free_AmigaMem(void)
{
  if(natmem_offset != 0)
  {
//    munlock(natmem_offset, totalAmigaMemSize);
    free(natmem_offset);
    natmem_offset;
  }
}


void alloc_AmigaMem(void)
{
  int res;
  
  free_AmigaMem();
  
  natmem_offset = (uae_u8*)malloc(totalAmigaMemSize);
  if(natmem_offset == 0)
  {
    printf("Failed to alloc complete Amiga memory.\n");
    abort();
  }
  
//  res = mlock(natmem_offset, totalAmigaMemSize);
//  if(res != 0)
//  {
//    printf("Failed to lock Amiga memory.\n");
//    free(natmem_offset);
//    abort();
//  }
}
