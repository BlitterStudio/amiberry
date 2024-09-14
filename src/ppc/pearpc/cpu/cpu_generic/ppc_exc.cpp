/*
 *	PearPC
 *	ppc_exc.cc
 *
 *	Copyright (C) 2003 Sebastian Biallas (sb@biallas.net)
 *	Copyright (C) 2004 Daniel Foesch (dfoesch@cs.nmsu.edu)
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

/*	Pages marked: v.???
 *	From: IBM PowerPC MicroProcessor Family: Altivec(tm) Technology...
 *		Programming Environments Manual
 */

#include "tools/snprintf.h"
#include "tracers.h"
#include "cpu/debug.h"
#include "info.h"
#include "ppc_cpu.h"
#include "ppc_exc.h"
#include "ppc_mmu.h"

/*
 *	.247
 */
bool FASTCALL ppc_exception(uint32 type, uint32 flags, uint32 a)
{
	if (type != PPC_EXC_DEC) {
		PPC_EXC_TRACE("@%08x: type = %08x (%08x, %08x)\n", gCPU.pc, type, flags, a);
	}
	switch (type) {
	case PPC_EXC_DSI: { // .271
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		gCPU.dar = a;
		gCPU.dsisr = flags;
		break;
	}
	case PPC_EXC_ISI: { // .274
		if (gCPU.pc == 0) {
			PPC_EXC_WARN("pc == 0 in ISI\n");
			SINGLESTEP("");
		}
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = (gCPU.msr & 0x87c0ffff) | flags;
		break;
	}
	case PPC_EXC_DEC: { // .284
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		break;
	}
	case PPC_EXC_EXT_INT: {
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		break;
	}
	case PPC_EXC_SC: {  // .285
		gCPU.srr[0] = gCPU.npc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		break;
	}
	case PPC_EXC_NO_FPU: { // .284
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		break;
	}
	case PPC_EXC_NO_VEC: {	// v.41
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		break;
	}
	case PPC_EXC_PROGRAM: { // .283
		if (flags & PPC_EXC_PROGRAM_NEXT) {
			gCPU.srr[0] = gCPU.npc;
		} else {
			gCPU.srr[0] = gCPU.pc;
		}
		gCPU.srr[1] = (gCPU.msr & 0x87c0ffff) | flags;
		break;
	}
	case PPC_EXC_FLOAT_ASSIST: { // .288
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		break;
	}
	case PPC_EXC_MACHINE_CHECK: { // .270
		if (!(gCPU.msr & MSR_ME)) {
			PPC_EXC_ERR("machine check exception and MSR[ME]=0.\n");
		}
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = (gCPU.msr & 0x87c0ffff) | MSR_RI;
		break;
	}
	case PPC_EXC_TRACE2: { // .286
		gCPU.srr[0] = gCPU.pc;
		gCPU.srr[1] = gCPU.msr & 0x87c0ffff;
		break;
	}
	default:
		PPC_EXC_ERR("unknown\n");
		return false;
	}
	ppc_mmu_tlb_invalidate();
	// MSR_IP: 0=0x000xxxxx 1=0xfffxxxxx
	if (gCPU.msr & MSR_IP)
		type |= 0xfff00000;
	// MSR_IP is not cleared when exception starts (was wrong in original PearPC)
	gCPU.msr &= MSR_IP;
	gCPU.npc = type;
	return true;
}

void ppc_cpu_raise_ext_exception()
{
	ppc_cpu_atomic_raise_ext_exception();
}

void ppc_cpu_cancel_ext_exception()
{
	ppc_cpu_atomic_cancel_ext_exception();
}
