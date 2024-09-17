/*
 *	PearPC
 *	ppc_exc.h
 *
 *	Copyright (C) 2003 Sebastian Biallas (sb@biallas.net)
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

#ifndef __PPC_EXC_H__
#define __PPC_EXC_H__

#include "system/types.h"

/*
 *	.250
 */
#define PPC_EXC_UNKNOWN 0
#define PPC_EXC_SYS_RESET 0x100
#define PPC_EXC_MACHINE_CHECK 0x00200
#define PPC_EXC_DSI 0x00300
#define PPC_EXC_ISI 0x00400
#define PPC_EXC_EXT_INT 0x00500
#define PPC_EXC_ALIGNMENT 0x00600
#define PPC_EXC_PROGRAM 0x00700
#define PPC_EXC_NO_FPU 0x00800
#define PPC_EXC_DEC 0x00900
//Reserved 0x00A00
//Reserved 0x00B00
#define PPC_EXC_SC 0x00C00
#define PPC_EXC_TRACE2 0x00D00
#define PPC_EXC_FLOAT_ASSIST 0x00E00
#define PPC_EXC_PERF_MON 0xF00
#define PPC_EXC_NO_VEC 0xF20
#define PPC_EXC_ALTIVEC 0xF20
#define PPC_EXC_ALTIVEC_ASSIST 0x1600
#define PPC_EXC_TAU 0x1700

#define PPC_EXC_DSISR_PAGE (1<<30)
#define PPC_EXC_DSISR_PROT (1<<27)
#define PPC_EXC_DSISR_STORE (1<<25)

#define PPC_EXC_SRR1_PAGE PPC_EXC_DSISR_PAGE
#define PPC_EXC_SRR1_GUARD (1<<28)
#define PPC_EXC_SRR1_PROT PPC_EXC_DSISR_PROT

#define PPC_EXC_PROGRAM_FLOAT (1<<20)
#define PPC_EXC_PROGRAM_ILL   (1<<19)
#define PPC_EXC_PROGRAM_PRIV  (1<<18)
#define PPC_EXC_PROGRAM_TRAP  (1<<17)

/* 
 * set if srr0 does not not contain the address of the intruction
 * causing the exception
 */
#define PPC_EXC_PROGRAM_NEXT  (1<<16)

bool FASTCALL ppc_exception(uint32 type=0, uint32 flags=0, uint32 a=0);
void ppc_cpu_raise_ext_exception();
void ppc_cpu_cancel_ext_exception();

#endif

