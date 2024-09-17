/*
 *	PearPC
 *	tracers.h
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

#ifndef __TRACERS_H__
#define __TRACERS_H__

#include "system/types.h"
#include "tools/snprintf.h"

extern void uae_ppc_crash(void);

#define PPC_CPU_TRACE(msg, ...) ht_printf("[CPU/CPU] " msg, ##__VA_ARGS__)
#define PPC_ALU_TRACE(msg, ...) ht_printf("[CPU/ALU] " msg, ##__VA_ARGS__)
#define PPC_FPU_TRACE(msg, ...) ht_printf("[CPU/FPU] " msg, ##__VA_ARGS__)
#define PPC_DEC_TRACE(msg, ...) ht_printf("[CPU/DEC] " msg, ##__VA_ARGS__)
#define PPC_ESC_TRACE(msg, ...) ht_printf("[CPU/ESC] " msg, ##__VA_ARGS__)
//#define PPC_EXC_TRACE(msg, ...) ht_printf("[CPU/EXC] " msg, ##__VA_ARGS__)
#define PPC_MMU_TRACE(msg, ...) ht_printf("[CPU/MMU] " msg, ##__VA_ARGS__)
#define PPC_OPC_TRACE(msg, ...) ht_printf("[CPU/OPC] " msg, ##__VA_ARGS__)
//#define IO_PROM_TRACE(msg, ...) ht_printf("[IO/PROM] " msg, ##__VA_ARGS__)
//#define IO_PROM_FS_TRACE(msg, ...) ht_printf("[IO/PROM/FS] " msg, ##__VA_ARGS__)
//#define IO_3C90X_TRACE(msg, ...) ht_printf("[IO/3c90x] " msg, ##__VA_ARGS__)
//#define IO_RTL8139_TRACE(msg, ...) ht_printf("[IO/rtl8139] " msg, ##__VA_ARGS__)
//#define IO_GRAPHIC_TRACE(msg, ...) ht_printf("[IO/GCARD] " msg, ##__VA_ARGS__)
//#define IO_CUDA_TRACE(msg, ...) ht_printf("[IO/CUDA] " msg, ##__VA_ARGS__)
//#define IO_PIC_TRACE(msg, ...) ht_printf("[IO/PIC] " msg, ##__VA_ARGS__)
//#define IO_PCI_TRACE(msg, ...) ht_printf("[IO/PCI] " msg, ##__VA_ARGS__)
//#define IO_MACIO_TRACE(msg, ...) ht_printf("[IO/MACIO] " msg, ##__VA_ARGS__)
//#define IO_NVRAM_TRACE(msg, ...) ht_printf("[IO/NVRAM] " msg, ##__VA_ARGS__)
//#define IO_IDE_TRACE(msg, ...) ht_printf("[IO/IDE] " msg, ##__VA_ARGS__)
//#define IO_USB_TRACE(msg, ...) ht_printf("[IO/USB] " msg, ##__VA_ARGS__)
#define IO_SERIAL_TRACE(msg, ...) ht_printf("[IO/SERIAL] " msg, ##__VA_ARGS__)
#define IO_CORE_TRACE(msg, ...) ht_printf("[IO/Generic] " msg, ##__VA_ARGS__)

#define PPC_CPU_WARN(msg, ...) ht_printf("[CPU/CPU] <Warning> " msg, ##__VA_ARGS__)
#define PPC_ALU_WARN(msg, ...) ht_printf("[CPU/ALU] <Warning> " msg, ##__VA_ARGS__)
#define PPC_FPU_WARN(msg, ...) ht_printf("[CPU/FPU] <Warning> " msg, ##__VA_ARGS__)
#define PPC_DEC_WARN(msg, ...) ht_printf("[CPU/DEC] <Warning> " msg, ##__VA_ARGS__)
#define PPC_ESC_WARN(msg, ...) ht_printf("[CPU/ESC] <Warning> " msg, ##__VA_ARGS__)
#define PPC_EXC_WARN(msg, ...) ht_printf("[CPU/EXC] <Warning> " msg, ##__VA_ARGS__)
#define PPC_MMU_WARN(msg, ...) ht_printf("[CPU/MMU] <Warning> " msg, ##__VA_ARGS__)
#define PPC_OPC_WARN(msg, ...) ht_printf("[CPU/OPC] <Warning> " msg, ##__VA_ARGS__)
#define IO_PROM_WARN(msg, ...) ht_printf("[IO/PROM] <Warning> " msg, ##__VA_ARGS__)
#define IO_PROM_FS_WARN(msg, ...) ht_printf("[IO/PROM/FS] <Warning> " msg, ##__VA_ARGS__)
#define IO_3C90X_WARN(msg, ...) ht_printf("[IO/3c90x] <Warning> " msg, ##__VA_ARGS__)
#define IO_RTL8139_WARN(msg, ...) ht_printf("[IO/rtl8139] <Warning> " msg, ##__VA_ARGS__)
#define IO_GRAPHIC_WARN(msg, ...) ht_printf("[IO/GCARD] <Warning> " msg, ##__VA_ARGS__)
#define IO_CUDA_WARN(msg, ...) ht_printf("[IO/CUDA] <Warning> " msg, ##__VA_ARGS__)
#define IO_PIC_WARN(msg, ...) ht_printf("[IO/PIC] <Warning> " msg, ##__VA_ARGS__)
#define IO_PCI_WARN(msg, ...) ht_printf("[IO/PCI] <Warning> " msg, ##__VA_ARGS__)
#define IO_MACIO_WARN(msg, ...) ht_printf("[IO/MACIO] <Warning> " msg, ##__VA_ARGS__)
#define IO_NVRAM_WARN(msg, ...) ht_printf("[IO/NVRAM] <Warning> " msg, ##__VA_ARGS__)
#define IO_IDE_WARN(msg, ...) ht_printf("[IO/IDE] <Warning> " msg, ##__VA_ARGS__)
#define IO_USB_WARN(msg, ...) ht_printf("[IO/USB] <Warning> " msg, ##__VA_ARGS__)
#define IO_SERIAL_WARN(msg, ...) ht_printf("[IO/SERIAL] <Warning> " msg, ##__VA_ARGS__)
#define IO_CORE_WARN(msg, ...) ht_printf("[IO/Generic] <Warning> " msg, ##__VA_ARGS__)

#define PPC_CPU_ERR(msg, ...) {ht_printf("[CPU/CPU] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); } 
#define PPC_ALU_ERR(msg, ...) {ht_printf("[CPU/ALU] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define PPC_FPU_ERR(msg, ...) {ht_printf("[CPU/FPU] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define PPC_DEC_ERR(msg, ...) {ht_printf("[CPU/DEC] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define PPC_ESC_ERR(msg, ...) {ht_printf("[CPU/ESC] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define PPC_EXC_ERR(msg, ...) {ht_printf("[CPU/EXC] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define PPC_MMU_ERR(msg, ...) {ht_printf("[CPU/MMU] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define PPC_OPC_ERR(msg, ...) {ht_printf("[CPU/OPC] <Error> " msg, ##__VA_ARGS__); }
#define IO_PROM_ERR(msg, ...) {ht_printf("[IO/PROM] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_PROM_FS_ERR(msg, ...) {ht_printf("[IO/PROM/FS] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_3C90X_ERR(msg, ...) {ht_printf("[IO/3c90x] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_RTL8139_ERR(msg, ...) {ht_printf("[IO/rtl8139] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_GRAPHIC_ERR(msg, ...) {ht_printf("[IO/GCARD] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_CUDA_ERR(msg, ...) {ht_printf("[IO/CUDA] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_PIC_ERR(msg, ...) {ht_printf("[IO/PIC] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_PCI_ERR(msg, ...) {ht_printf("[IO/PCI] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_MACIO_ERR(msg, ...) {ht_printf("[IO/MACIO] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_NVRAM_ERR(msg, ...) {ht_printf("[IO/NVRAM] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_IDE_ERR(msg, ...) {ht_printf("[IO/IDE] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_USB_ERR(msg, ...) {ht_printf("[IO/IDE] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_SERIAL_ERR(msg, ...) {ht_printf("[IO/SERIAL] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }
#define IO_CORE_ERR(msg, ...) {ht_printf("[IO/Generic] <Error> " msg, ##__VA_ARGS__); uae_ppc_crash(); }

/*
 *
 */
#ifndef PPC_CPU_TRACE
#define PPC_CPU_TRACE(msg, ...)
#endif

#ifndef PPC_ALU_TRACE
#define PPC_ALU_TRACE(msg, ...)
#endif

#ifndef PPC_FPU_TRACE
#define PPC_FPU_TRACE(msg, ...)
#endif

#ifndef PPC_DEC_TRACE
#define PPC_DEC_TRACE(msg, ...)
#endif

#ifndef PPC_EXC_TRACE
#define PPC_EXC_TRACE(msg, ...)
#endif

#ifndef PPC_ESC_TRACE
#define PPC_ESC_TRACE(msg, ...)
#endif

#ifndef PPC_MMU_TRACE
#define PPC_MMU_TRACE(msg, ...)
#endif

#ifndef PPC_OPC_TRACE
#define PPC_OPC_TRACE(msg, ...)
#endif

#ifndef PPC_OPC_WARN
#define PPC_OPC_WARN(msg, ...)
#endif

#ifndef IO_PROM_TRACE
#define IO_PROM_TRACE(msg, ...)
#endif

#ifndef IO_PROM_FS_TRACE
#define IO_PROM_FS_TRACE(msg, ...)
#endif

#ifndef IO_GRAPHIC_TRACE
#define IO_GRAPHIC_TRACE(msg, ...)
#endif

#ifndef IO_CUDA_TRACE
#define IO_CUDA_TRACE(msg, ...)
#endif

#ifndef IO_PIC_TRACE
#define IO_PIC_TRACE(msg, ...)
#endif

#ifndef IO_PCI_TRACE
#define IO_PCI_TRACE(msg, ...)
#endif

#ifndef IO_MACIO_TRACE
#define IO_MACIO_TRACE(msg, ...)
#endif

#ifndef IO_ISA_TRACE
#define IO_ISA_TRACE(msg, ...)
#endif

#ifndef IO_IDE_TRACE
#define IO_IDE_TRACE(msg, ...)
#endif

#ifndef IO_CORE_TRACE
#define IO_CORE_TRACE(msg, ...)
#endif

#ifndef IO_NVRAM_TRACE
#define IO_NVRAM_TRACE(msg, ...)
#endif

#ifndef IO_USB_TRACE
#define IO_USB_TRACE(msg, ...)
#endif

#ifndef IO_SERIAL_TRACE
#define IO_SERIAL_TRACE(msg, ...)
#endif

#ifndef IO_3C90X_TRACE
#define IO_3C90X_TRACE(msg, ...)
#endif

#ifndef IO_RTL8139_TRACE
#define IO_RTL8139_TRACE(msg, ...)
#endif

#endif

