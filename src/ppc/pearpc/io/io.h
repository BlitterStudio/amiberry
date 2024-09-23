/*
 *	PearPC
 *	io.h
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

#ifndef __IO_IO_H__
#define __IO_IO_H__

#include "system/types.h"
#include <stdlib.h>
#include <string.h>
#include "cpu/cpu.h"
#include "cpu/debug.h"
#include "cpu/mem.h"
#include "io.h"
//#include "io/graphic/gcard.h"
//#include "io/pic/pic.h"
//#include "io/pci/pci.h"
//#include "io/cuda/cuda.h"
//#include "io/nvram/nvram.h"
#include "tracers.h"

#define IO_MEM_ACCESS_OK	0
#define IO_MEM_ACCESS_EXC	1
#define IO_MEM_ACCESS_FATAL	2

#include "uae/ppc.h"

static inline int io_mem_write(uint32 addr, uint32 data, int size)
{
	if (uae_ppc_io_mem_write(addr, data, size))
		return IO_MEM_ACCESS_OK;
#if 0
	if (addr >= IO_GCARD_FRAMEBUFFER_PA_START && addr < IO_GCARD_FRAMEBUFFER_PA_END) {
		gcard_write(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_PCI_PA_START && addr < IO_PCI_PA_END) {
		pci_write(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_PIC_PA_START && addr < IO_PIC_PA_END) {
		pic_write(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_CUDA_PA_START && addr < IO_CUDA_PA_END) {
		cuda_write(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_NVRAM_PA_START && addr < IO_NVRAM_PA_END) {
		nvram_write(addr, data, size);
		return IO_MEM_ACCESS_OK;		
	}
	// PCI and ISA must be checked at last
	if (addr >= IO_PCI_DEVICE_PA_START && addr < IO_PCI_DEVICE_PA_END) {
		pci_write_device(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_ISA_PA_START && addr < IO_ISA_PA_END) {
		/*
		 * should raise exception here...
		 * but linux dont like this
		 */
		isa_write(addr, data, size);
		return IO_MEM_ACCESS_OK;
		/*if (isa_write(addr, data, size)) {
			return IO_MEM_ACCESS_OK;
		} else {
			ppc_exception(PPC_EXC_MACHINE_CHECK);
			return IO_MEM_ACCESS_EXC;
		}*/
	}
#endif
	IO_CORE_WARN("no one is responsible for address %08x (write: %08x from %08x)\n", addr, data, ppc_cpu_get_pc(0));
	SINGLESTEP("");
	ppc_machine_check_exception();	
	return IO_MEM_ACCESS_EXC;
}

static inline int io_mem_read(uint32 addr, uint32 &data, int size)
{
	if (uae_ppc_io_mem_read(addr, &data, size))
		return IO_MEM_ACCESS_OK;

#if 0
	if (addr >= IO_GCARD_FRAMEBUFFER_PA_START && addr < IO_GCARD_FRAMEBUFFER_PA_END) {
		gcard_read(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_PCI_PA_START && addr < IO_PCI_PA_END) {
		pci_read(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_PIC_PA_START && addr < IO_PIC_PA_END) {
		pic_read(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_CUDA_PA_START && addr < IO_CUDA_PA_END) {
		cuda_read(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_NVRAM_PA_START && addr < IO_NVRAM_PA_END) {
		nvram_read(addr, data, size);
		return IO_MEM_ACCESS_OK;		
	}
	if (addr == 0xff000004) {
		// wtf?
		data = 1;
		return IO_MEM_ACCESS_OK;
	}
	// PCI and ISA must be checked at last
	if (addr >= IO_PCI_DEVICE_PA_START && addr < IO_PCI_DEVICE_PA_END) {
		pci_read_device(addr, data, size);
		return IO_MEM_ACCESS_OK;
	}
	if (addr >= IO_ISA_PA_START && addr < IO_ISA_PA_END) {		
		/*
		 * should raise exception here...
		 * but linux dont like this
		 */
		isa_read(addr, data, size);
		return IO_MEM_ACCESS_OK;
		/*if (isa_read(addr, data, size)) {
			return IO_MEM_ACCESS_OK;
		} else {
			ppc_exception(PPC_EXC_MACHINE_CHECK);
			return IO_MEM_ACCESS_EXC;
		}*/
	}
#endif
	IO_CORE_WARN("no one is responsible for address %08x (read from %08x)\n", addr, ppc_cpu_get_pc(0));
	SINGLESTEP("");
	ppc_machine_check_exception();
	return IO_MEM_ACCESS_EXC;
}

static inline int io_mem_write64(uint32 addr, uint64 data)
{
	if (uae_ppc_io_mem_write64(addr, data))
		return IO_MEM_ACCESS_OK;
#if 0
	if ((addr >= IO_GCARD_FRAMEBUFFER_PA_START) && (addr < (IO_GCARD_FRAMEBUFFER_PA_END))) {
		gcard_write64(addr, data);
		return IO_MEM_ACCESS_OK;
	}
#endif
	IO_CORE_ERR("no one is responsible for address %08x (write64: %016q from %08x)\n", addr, &data, ppc_cpu_get_pc(0));
	return IO_MEM_ACCESS_FATAL;
}

static inline int io_mem_read64(uint32 addr, uint64 &data)
{
	if (uae_ppc_io_mem_read64(addr, &data))
		return IO_MEM_ACCESS_OK;

#if 0
	if ((addr >= IO_GCARD_FRAMEBUFFER_PA_START) && (addr < (IO_GCARD_FRAMEBUFFER_PA_END))) {
		gcard_read64(addr, data);
		return IO_MEM_ACCESS_OK;
	}
#endif
	IO_CORE_ERR("no one is responsible for address %08x (read64 from %08x)\n", addr, ppc_cpu_get_pc(0));
	return IO_MEM_ACCESS_FATAL;
}

static inline int io_mem_write128(uint32 addr, uint128 *data)
{
#if 0
	if ((addr >= IO_GCARD_FRAMEBUFFER_PA_START) && (addr < (IO_GCARD_FRAMEBUFFER_PA_END))) {
		gcard_write128(addr, data);
		return IO_MEM_ACCESS_OK;
	}
#endif
	IO_CORE_ERR("no one is responsible for address %08x (write128: %016q%016q from %08x)\n", addr, data->h, data->l, ppc_cpu_get_pc(0));
	return IO_MEM_ACCESS_FATAL;
}

static inline int io_mem_write128_native(uint32 addr, uint128 *data)
{
#if 0
	if ((addr >= IO_GCARD_FRAMEBUFFER_PA_START) && (addr < (IO_GCARD_FRAMEBUFFER_PA_END))) {
		gcard_write128_native(addr, data);
		return IO_MEM_ACCESS_OK;
	}
#endif
	IO_CORE_ERR("no one is responsible for address %08x (write128: %016q%016q from %08x)\n", addr, data->h, data->l, ppc_cpu_get_pc(0));
	return IO_MEM_ACCESS_FATAL;
}

static inline int io_mem_read128(uint32 addr, uint128 *data)
{
#if 0
	if ((addr >= IO_GCARD_FRAMEBUFFER_PA_START) && (addr < (IO_GCARD_FRAMEBUFFER_PA_END))) {
		gcard_read128(addr, data);
		return IO_MEM_ACCESS_OK;
	}
#endif
	IO_CORE_ERR("no one is responsible for address %08x (read128 from %08x)\n", addr, ppc_cpu_get_pc(0));
	return IO_MEM_ACCESS_FATAL;
}

static inline int io_mem_read128_native(uint32 addr, uint128 *data)
{
#if 0
	if ((addr >= IO_GCARD_FRAMEBUFFER_PA_START) && (addr < (IO_GCARD_FRAMEBUFFER_PA_END))) {
		gcard_read128_native(addr, data);
		return IO_MEM_ACCESS_OK;
	}
#endif
	IO_CORE_ERR("no one is responsible for address %08x (read128 from %08x)\n", addr, ppc_cpu_get_pc(0));
	return IO_MEM_ACCESS_FATAL;
}

void io_init();
void io_done();
void io_init_config();

#endif
