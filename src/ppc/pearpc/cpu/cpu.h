/*
 *	PearPC
 *	cpu.h
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

#ifndef __CPU_H__
#define __CPU_H__

#include "system/types.h"
#include "uae/ppc.h"

uint64	ppc_get_clock_frequency(int cpu);
uint64	ppc_get_bus_frequency(int cpu);
uint64	ppc_get_timebase_frequency(int cpu);

bool	PPCCALL ppc_cpu_init(uint32);
void	ppc_cpu_init_config();
void	PPCCALL ppc_cpu_free(void);

void	PPCCALL ppc_cpu_stop();
void	ppc_cpu_wakeup();

void	ppc_machine_check_exception();

void	ppc_cpu_raise_ext_exception();
void	ppc_cpu_cancel_ext_exception();

/*
 * May only be called from within a CPU thread.
 */

void	PPCCALL ppc_cpu_run_continuous();
void	PPCCALL ppc_cpu_run_single(int);
uint32	ppc_cpu_get_gpr(int cpu, int i);
void	ppc_cpu_set_gpr(int cpu, int i, uint32 newvalue);
void	ppc_cpu_set_msr(int cpu, uint32 newvalue);
void	PPCCALL ppc_cpu_set_pc(int cpu, uint32 newvalue);
uint32	ppc_cpu_get_pc(int cpu);
uint32	ppc_cpu_get_pvr(int cpu);

#endif
