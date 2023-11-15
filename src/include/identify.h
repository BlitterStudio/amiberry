 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Tables for labelling amiga internals.
  *
  */

#ifndef UAE_IDENTIFY_H
#define UAE_IDENTIFY_H

#include "uae/types.h"

struct mem_labels
{
    const TCHAR *name;
    uae_u32 adr;
};

#define CD_WO 1
#define CD_ECS_AGNUS 2
#define CD_ECS_DENISE 4
#define CD_AGA 8
#define CD_NONE 16
#define CD_DMA_PTR 32
#define CD_COLOR 64

struct customData
{
    const TCHAR *name;
    uae_u32 adr;
    int special;
	uae_u16 mask[3];
};

extern const struct mem_labels mem_labels[];
extern const struct mem_labels int_labels[];
extern const struct mem_labels trap_labels[];
extern const struct customData custd[];

#endif /* UAE_IDENTIFY_H */
