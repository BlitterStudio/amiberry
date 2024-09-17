/*
 *	PearPC
 *	ppc_cpu.cc
 *
 *	Copyright (C) 2003, 2004 Sebastian Biallas (sb@biallas.net)
 *      Portions Copyright (C) 2004 Apple Computer, Inc.
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

#include <cstring>
#include <cstdio>

#include "system/systhread.h"
#include "system/arch/sysendian.h"
//#include "tools/snprintf.h"
#include "cpu/cpu.h"
#include "cpu/debug.h"
#include "info.h"
//#include "io/pic/pic.h"
//#include "debug/debugger.h"
#include "tracers.h"
#include "ppc_cpu.h"
#include "ppc_dec.h"
#include "ppc_fpu.h"
#include "ppc_exc.h"
#include "ppc_mmu.h"
#include "ppc_tools.h"
#include "uae/ppc.h"

//#include "io/graphic/gcard.h"

PPC_CPU_State gCPU;
//Debugger *gDebugger;

static bool gSinglestep = false;

bool activate = false;
static inline void ppc_debug_hook()
{
}

sys_mutex exception_mutex;

void PPCCALL ppc_cpu_atomic_raise_ext_exception()
{
	sys_lock_mutex(exception_mutex);
	gCPU.ext_exception = true;
	gCPU.exception_pending = true;
	sys_unlock_mutex(exception_mutex);
}

void PPCCALL ppc_cpu_atomic_cancel_ext_exception()
{
	sys_lock_mutex(exception_mutex);
	gCPU.ext_exception = false;
	if (!gCPU.dec_exception) gCPU.exception_pending = false;
	sys_unlock_mutex(exception_mutex);
}

static void ppc_cpu_atomic_raise_dec_exception()
{
	sys_lock_mutex(exception_mutex);
	gCPU.dec_exception = true;
	gCPU.exception_pending = true;
	sys_unlock_mutex(exception_mutex);
}


static void ppc_do_dec(int val)
{
	if (gCPU.pdec == 0) {
		gCPU.exception_pending = true;
		gCPU.dec_exception = true;
		gCPU.pdec = (uint64)0xffffffff * TB_TO_PTB_FACTOR;
		uae_ppc_wakeup();
	} else {
		uint64 oldpdec = gCPU.pdec;
		gCPU.pdec -= val;
		if (gCPU.pdec > oldpdec) {
			gCPU.pdec = 0;
		}
	}
}

uint64_t PPCCALL ppc_cpu_get_dec(void)
{
	return gCPU.pdec;
}

void PPCCALL ppc_cpu_do_dec(int val)
{
	ppc_do_dec(val);
}

static uint ops = 0;
static int ppc_trace;

void PPCCALL ppc_cpu_run_single(int count)
{
	while (count != 0) {
		if (count > 0)
			count--;
		gCPU.npc = gCPU.pc+4;
		if ((gCPU.pc & ~0xfff) == gCPU.effective_code_page) {
			gCPU.current_opc = ppc_word_from_BE(*((uint32*)(&gCPU.physical_code_page[gCPU.pc & 0xfff])));
			ppc_debug_hook();
		} else {
			int ret;
			if ((ret = ppc_direct_effective_memory_handle_code(gCPU.pc & ~0xfff, gCPU.physical_code_page))) {
				if (ret == PPC_MMU_EXC) {
					gCPU.pc = gCPU.npc;
					continue;
				} else {
					PPC_CPU_ERR("?\n");
				}
			}
			gCPU.effective_code_page = gCPU.pc & ~0xfff;
			continue;
		}
		if (ppc_trace)
			ht_printf("%08x %04x\n", gCPU.pc, gCPU.current_opc);
		ppc_exec_opc();
		ops++;
		gCPU.ptb++;
		ppc_do_dec(1);
		if ((ops & 0x3ffff)==0) {
/*			if (pic_check_interrupt()) {
				gCPU.exception_pending = true;
				gCPU.ext_exception = true;
			}*/
			if ((ops & 0x0fffff)==0) {
//				uint32 j=0;
//				ppc_read_effective_word(0xc046b2f8, j);

				//ht_printf("@%08x (%u ops) pdec: %08x lr: %08x\n", gCPU.pc, ops, gCPU.pdec, gCPU.lr);
#if 0
				extern uint32 PIC_enable_low;
				extern uint32 PIC_enable_high;
				ht_printf("enable ");
				int x = 1;
				for (int i=0; i<31; i++) {
					if (PIC_enable_low & x) {
						ht_printf("%d ", i);
		    			}
					x<<=1;
				}
				x=1;
				for (int i=0; i<31; i++) {
					if (PIC_enable_high & x) {
						ht_printf("%d ", 32+i);
		    			}
					x<<=1;
				}
				ht_printf("\n");
#endif
			}
		}
		
		gCPU.pc = gCPU.npc;
		
		extern int debugger_active, pause_emulation;
		extern void sleep_millis(int);
		while ((debugger_active || pause_emulation) && count < 0) {
			sleep_millis(10);
		}

		if (gCPU.exception_pending) {
			if (gCPU.stop_exception) {
				gCPU.stop_exception = false;
				if (!gCPU.dec_exception && !gCPU.ext_exception) gCPU.exception_pending = false;
				break;
			}
			if (gCPU.msr & MSR_EE) {
				sys_lock_mutex(exception_mutex);
				if (gCPU.ext_exception) {
					ppc_exception(PPC_EXC_EXT_INT);
					gCPU.ext_exception = false;
					gCPU.pc = gCPU.npc;
					if (!gCPU.dec_exception) gCPU.exception_pending = false;
					sys_unlock_mutex(exception_mutex);
					continue;
				}
				if (gCPU.dec_exception) {
					ppc_exception(PPC_EXC_DEC);
					gCPU.dec_exception = false;
					//ht_printf("pdec exp %08x\n", gCPU.pc);
					gCPU.pc = gCPU.npc;
					gCPU.exception_pending = false;
					sys_unlock_mutex(exception_mutex);
					continue;
				}
				sys_unlock_mutex(exception_mutex);
				PPC_CPU_ERR("no interrupt, but signaled?!\n");
			}
		}
#ifdef PPC_CPU_ENABLE_SINGLESTEP
		if (gCPU.msr & MSR_SE) {
			if (gCPU.singlestep_ignore) {
				gCPU.singlestep_ignore = false;
			} else {
				ppc_exception(PPC_EXC_TRACE2);
				gCPU.pc = gCPU.npc;
				continue;
			}
		}
#endif
	}
}

void PPCCALL ppc_cpu_run_continuous(void)
{
	PPC_CPU_TRACE("execution started at %08x\n", gCPU.pc);
	gCPU.effective_code_page = 0xffffffff;
	ops = 0;
	ppc_cpu_run_single(-1);
}

void PPCCALL ppc_cpu_stop()
{
	sys_lock_mutex(exception_mutex);
	gCPU.stop_exception = true;
	gCPU.exception_pending = true;
	sys_unlock_mutex(exception_mutex);
}

uint64	ppc_get_clock_frequency(int cpu)
{
	return PPC_CLOCK_FREQUENCY;
}

uint64	ppc_get_bus_frequency(int cpu)
{
	return PPC_BUS_FREQUENCY;
}

uint64	ppc_get_timebase_frequency(int cpu)
{
	return PPC_TIMEBASE_FREQUENCY;
}


void ppc_machine_check_exception()
{
	PPC_CPU_ERR("machine check exception\n");
}

uint32	ppc_cpu_get_gpr(int cpu, int i)
{
	return gCPU.gpr[i];
}

void	ppc_cpu_set_gpr(int cpu, int i, uint32 newvalue)
{
	gCPU.gpr[i] = newvalue;
}

void	ppc_cpu_set_msr(int cpu, uint32 newvalue)
{
	gCPU.msr = newvalue;
}

// Handle as CPU reset
void PPCCALL ppc_cpu_set_pc(int cpu, uint32 newvalue)
{
	gCPU.srr[0] = gCPU.pc;
	gCPU.srr[1] = gCPU.msr & 0xff73;
	gCPU.pc = newvalue;
	gCPU.msr &= MSR_ILE | MSR_ME | MSR_IP;
	if (gCPU.msr & MSR_ILE)
		gCPU.msr |= MSR_LE;
}

uint32	ppc_cpu_get_pc(int cpu)
{
	return gCPU.pc;
}

uint32	ppc_cpu_get_pvr(int cpu)
{
	return gCPU.pvr;
}

#if 0
void ppc_cpu_map_framebuffer(uint32 pa, uint32 ea)
{
	// use BAT for framebuffer
	gCPU.dbatu[0] = ea|(7<<2)|0x3;
	gCPU.dbat_bl17[0] = ~(BATU_BL(gCPU.dbatu[0])<<17);
	gCPU.dbatl[0] = pa;
}
#endif

void ppc_set_singlestep_v(bool v, const char *file, int line, const char *format, ...)
{
	char buffer[200];
	va_list arg;
	va_start(arg, format);
	ht_printf("singlestep %s from %s:%d, info: ", v ? "set" : "cleared", file, line);
	vsprintf(buffer, format, arg);
	ht_printf("%s\n", buffer);
	va_end(arg);
	uae_ppc_crash();
	gSinglestep = v;
}

void ppc_set_singlestep_nonverbose(bool v)
{
	gSinglestep = v;
}

#define CPU_KEY_PVR	"cpu_pvr"

//#include "configparser.h"

bool PPCCALL ppc_cpu_init(uint32 pvr)
{
	memset(&gCPU, 0, sizeof gCPU);
	gCPU.pvr = pvr; //gConfig->getConfigInt(CPU_KEY_PVR);
	gCPU.hid[1] = 0x80000000;
	gCPU.msr = MSR_IP;
	
	ppc_dec_init();
	// initialize srs (mostly for prom)
//	for (int i=0; i<16; i++) {
//		gCPU.sr[i] = 0x2aa*i;
//	}
	sys_create_mutex(&exception_mutex);

	PPC_CPU_WARN("You are using the generic CPU!\n");
	PPC_CPU_WARN("This is much slower than the just-in-time compiler and\n");
	PPC_CPU_WARN("should only be used for debugging purposes or if there's\n");
	PPC_CPU_WARN("no just-in-time compiler for your platform.\n");
	
	return true;
}

void PPCCALL ppc_cpu_free(void)
{
	sys_destroy_mutex(exception_mutex);
}

#if 0
void ppc_cpu_init_config()
{
	gConfig->acceptConfigEntryIntDef("cpu_pvr", 0x000c0201);
}
#endif
