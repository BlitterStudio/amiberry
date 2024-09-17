/*
 *	PearPC
 *	mem.h
 *
 *	Copyright (C) 2003, 2004 Sebastian Biallas (sb@biallas.net)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __MEM_H__
#define __MEM_H__

#include "system/types.h"

bool FASTCALL ppc_init_physical_memory(uint size);

uint32  ppc_get_memory_size();

bool	ppc_dma_write(uint32 dest, const void *src, uint32 size);
bool	ppc_dma_read(void *dest, uint32 src, uint32 size);
bool	ppc_dma_set(uint32 dest, int c, uint32 size);

void	ppc_cpu_map_framebuffer(uint32 pa, uint32 ea);

/*
 *	These functions will be removed once we switch to openbios.
 */

bool	DEPRECATED ppc_prom_set_sdr1(uint32 newval, bool quiesce);
bool	DEPRECATED ppc_prom_effective_to_physical(uint32 &result, uint32 ea);
bool	DEPRECATED ppc_prom_page_create(uint32 ea, uint32 pa);

#endif
