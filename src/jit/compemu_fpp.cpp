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
#include "ersatz.h"
#include "compemu.h"

#if defined(JIT)
uae_u32 temp_fp[] = {0,0,0};  /* To convert between FP and <EA> */

/* 128 words, indexed through the low byte of the 68k fpu control word */
static const uae_u16 x86_fpucw[]={
    0x137f, 0x137f, 0x137f, 0x137f, 0x137f, 0x137f, 0x137f, 0x137f, /* E-RN */
    0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, /* E-RZ */
    0x177f, 0x177f, 0x177f, 0x177f, 0x177f, 0x177f, 0x177f, 0x177f, /* E-RD */
    0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, /* E-RU */

    0x107f, 0x107f, 0x107f, 0x107f, 0x107f, 0x107f, 0x107f, 0x107f, /* S-RN */
    0x1c7f, 0x1c7f, 0x1c7f, 0x1c7f, 0x1c7f, 0x1c7f, 0x1c7f, 0x1c7f, /* S-RZ */
    0x147f, 0x147f, 0x147f, 0x147f, 0x147f, 0x147f, 0x147f, 0x147f, /* S-RD */
    0x187f, 0x187f, 0x187f, 0x187f, 0x187f, 0x187f, 0x187f, 0x187f, /* S-RU */

    0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, /* D-RN */
    0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, /* D-RZ */
    0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, /* D-RD */
    0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, /* D-RU */

    0x137f, 0x137f, 0x137f, 0x137f, 0x137f, 0x137f, 0x137f, 0x137f, /* ?-RN */
    0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, 0x1f7f, /* ?-RZ */
    0x177f, 0x177f, 0x177f, 0x177f, 0x177f, 0x177f, 0x177f, 0x177f, /* ?-RD */
    0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f, 0x1b7f  /* ?-RU */
};
static const int sz1[8] = { 4, 4, 12, 12, 2, 8, 1, 0 };
static const int sz2[8] = { 4, 4, 12, 12, 2, 8, 2, 0 };

/* return the required floating point precision or -1 for failure, 0=E, 1=S, 2=D */
STATIC_INLINE int comp_fp_get (uae_u32 opcode, uae_u16 extra, int treg)
{
  printf("comp_fp_get not yet implemented\n");
  return -1;
}

/* return of -1 means failure, >=0 means OK */
STATIC_INLINE int comp_fp_put (uae_u32 opcode, uae_u16 extra)
{
  printf("comp_fp_put not yet implemented\n");
  return -1;
}

/* return -1 for failure, or register number for success */
STATIC_INLINE int comp_fp_adr (uae_u32 opcode)
{
  printf("comp_fp_adr not yet implemented\n");
  return -1;
}

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
