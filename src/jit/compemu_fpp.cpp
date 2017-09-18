/*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68881 emulation
  *
  * Copyright 1996 Herman ten Brugge
  * Adapted for JIT compilation (c) Bernd Meyer, 2000
  * Modified 2005 Peter Keunecke
 */

#include <math.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "compemu.h"

#if defined(JIT)

void comp_fdbcc_opp (uae_u32 opcode, uae_u16 extra)
{
  printf("comp_fdbcc_opp not yet implemented\n");
}

void comp_fscc_opp (uae_u32 opcode, uae_u16 extra)
{
  printf("comp_fscc_opp not yet implemented\n");
}

void comp_ftrapcc_opp (uae_u32 opcode, uaecptr oldpc)
{
  printf("comp_ftrapcc_opp not yet implemented\n");
}

void comp_fbcc_opp (uae_u32 opcode)
{
  printf("comp_fbcc_opp not yet implemented\n");
}

void comp_fsave_opp (uae_u32 opcode)
{
  printf("comp_fsave_opp not yet implemented\n");
}

void comp_frestore_opp (uae_u32 opcode)
{
  printf("comp_frestore_opp not yet implemented\n");
}

void comp_fpp_opp (uae_u32 opcode, uae_u16 extra)
{
  printf("comp_fpp_opp not yet implemented\n");
}
#endif
