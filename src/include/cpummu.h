/*
* cpummu.h - MMU emulation
*
* Copyright (c) 2001-2004 Milan Jurik of ARAnyM dev team (see AUTHORS)
*
* Inspired by UAE MMU patch
*
* This file is part of the ARAnyM project which builds a new and powerful
* TOS/FreeMiNT compatible virtual machine running on almost any hardware.
*
* ARAnyM is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* ARAnyM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ARAnyM; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef UAE_CPUMMU_H
#define UAE_CPUMMU_H

#include "uae/types.h"

#include "mmu_common.h"

#define MMU_TTR_LOGICAL_BASE				0xff000000
#define MMU_TTR_LOGICAL_MASK				0x00ff0000
#define MMU_TTR_BIT_ENABLED					(1 << 15)
#define MMU_TTR_BIT_SFIELD_ENABLED			(1 << 14)
#define MMU_TTR_BIT_SFIELD_SUPER			(1 << 13)
#define MMU_TTR_SFIELD_SHIFT				13
#define MMU_TTR_BIT_WRITE_PROTECT			(1 << 2)

#define MMU_MMUSR_T						(1 << 1)
#define MMU_MMUSR_R						(1 << 0)

#define TTR_NO_MATCH	0
#define TTR_NO_WRITE	1
#define TTR_OK_MATCH	2

#endif /* UAE_CPUMMU_H */
