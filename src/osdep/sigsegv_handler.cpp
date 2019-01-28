/*
 * sigsegv_linux_arm.cpp - x86_64 Linux SIGSEGV handler
 *
 * Copyright (c) 2014 Jens Heitmann ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Bernie Meyer's UAE-JIT and Gwenole Beauchesne's Basilisk II-JIT
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
 *
 */
#include <string.h>
#include <stdlib.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "jit/compemu.h"
#include "uae.h"

#include <asm/sigcontext.h>
#include <csignal>
#include <dlfcn.h>
#ifndef ANDROID
#include <execinfo.h>
#else
int backtrace(void**,int){ return 0; }
char** backtrace_symbols(void* const*,int){return NULL; }
void backtrace_symbols_fd(void* const*,int,int){} 
#endif

extern uae_u8* current_compile_p;
extern uae_u8* compiled_code;
extern uae_u8* popallspace;
extern blockinfo* active;
extern blockinfo* dormant;
extern void invalidate_block(blockinfo* bi);
extern void raise_in_cl_list(blockinfo* bi);
extern bool write_logfile;

#define SHOW_DETAILS 2

#define output_log  write_log

enum transfer_type_t
{
	TYPE_UNKNOWN,
	TYPE_LOAD,
	TYPE_STORE
};

enum type_size_t
{
	SIZE_UNKNOWN,
	SIZE_BYTE,
	SIZE_WORD,
	SIZE_INT
};

enum style_type_t
{
	STYLE_SIGNED,
	STYLE_UNSIGNED
};

static int in_handler = 0;
static int max_signals = 200;

void init_max_signals(void)
{
	if (write_logfile)
		max_signals = 20;
	else
		max_signals = 200;
}


enum
{
	ARM_REG_PC = 15,
	ARM_REG_CPSR = 16
};

static const char* reg_names[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10/sl", "r11/fp", "r12/ip", "r13/sp", "r14/lr", "r15/pc"
};


static int delete_trigger(blockinfo* bi, void* pc)
{
	while (bi)
	{
		if (bi->handler && (uae_u8*)bi->direct_handler <= pc && (uae_u8*)bi->nexthandler > pc)
		{
			output_log(_T("JIT: Deleted trigger (0x%08x < 0x%08x < 0x%08x) 0x%08x\n"),
				bi->handler, pc, bi->nexthandler, bi->pc_p);
			invalidate_block(bi);
			raise_in_cl_list(bi);
			countdown = 0;
			set_special(0);
			return 1;
		}
		bi = bi->next;
	}
	return 0;
}


#define HANDLE_EXCEPTION_NONE 0
#define HANDLE_EXCEPTION_OK 1
#define HANDLE_EXCEPTION_A4000RAM 2


static int handle_exception(unsigned long* pregs, uintptr fault_addr)
{
	int handled = HANDLE_EXCEPTION_NONE;
	unsigned int* fault_pc = (unsigned int *)pregs[ARM_REG_PC];

	if (fault_pc == nullptr)
	{
		output_log(_T("PC is NULL.\n"));
		return HANDLE_EXCEPTION_NONE;
	}

	// Check for exception in handler
	if (in_handler > 0)
	{
		output_log(_T("Segmentation fault in handler.\n"));
		return HANDLE_EXCEPTION_NONE;
	}
	++in_handler;

	for (;;)
	{
		// We analyse only exceptions from JIT
		if (currprefs.cachesize == 0)
		{
			output_log(_T("JIT not in use.\n"));
			break;
		}

		// Did the error happens in compiled code?
		if ((uae_u8*)fault_pc >= compiled_code && (uae_u8*)fault_pc < current_compile_p)
		output_log(_T("Error in compiled code.\n"));
		else if ((uae_u8*)fault_pc >= popallspace && (uae_u8*)fault_pc < popallspace + POPALLSPACE_SIZE)
		output_log(_T("Error in popallspace code.\n"));
		else
		{
			output_log(_T("Error not in JIT code.\n"));
			break;
		}

		// Get Amiga address of illegal memory address
		auto amiga_addr = uae_u32(fault_addr) - uae_u32(regs.natmem_offset);

		// Check for stupid RAM detection of kickstart
		if (a3000lmem_bank.allocated_size > 0 && amiga_addr >= a3000lmem_bank.start - 0x00100000 && amiga_addr <
			a3000lmem_bank.start - 0x00100000 + 8)
		{
			output_log(_T("  Stupid kickstart detection for size of ramsey_low at 0x%08x.\n"), amiga_addr);
			pregs[ARM_REG_PC] += 4;
			handled = HANDLE_EXCEPTION_A4000RAM;
			break;
		}

		// Check for stupid RAM detection of kickstart
		if (a3000hmem_bank.allocated_size > 0 && amiga_addr >= a3000hmem_bank.start + a3000hmem_bank.allocated_size &&
			amiga_addr < a3000hmem_bank.start + a3000hmem_bank.allocated_size + 8)
		{
			output_log(_T("  Stupid kickstart detection for size of ramsey_high at 0x%08x.\n"), amiga_addr);
			pregs[ARM_REG_PC] += 4;
			handled = HANDLE_EXCEPTION_A4000RAM;
			break;
		}

		// Get memory bank of address
		const auto ab = &get_mem_bank(amiga_addr);
		if (ab)
		output_log(_T("JIT: Address bank: %s, address %08x\n"), ab->name ? ab->name : _T("NONE"), amiga_addr);

		// Analyse ARM instruction
		const auto opcode = fault_pc[0];
		auto transfer_type = TYPE_UNKNOWN;
		int transfer_size = SIZE_UNKNOWN;
		int style = STYLE_UNSIGNED;
		output_log(_T("JIT: ARM opcode = 0x%08x\n"), opcode);

		// Handle load/store instructions only
		switch ((opcode >> 25) & 7)
		{
		case 0: // Halfword and Signed Data Transfer (LDRH, STRH, LDRSB, LDRSH)
			// Determine transfer size (S/H bits)
			switch ((opcode >> 5) & 3)
			{
			case 0: // SWP instruction
				output_log(_T("ARM: SWP Instruction, not supported (0x%08x)\n"), opcode);
				break;
			case 1: // Unsigned halfwords
				transfer_size = SIZE_WORD;
				break;
			case 3: // Signed halfwords
				style = STYLE_SIGNED;
				transfer_size = SIZE_WORD;
				break;
			case 2: // Signed byte
				style = STYLE_SIGNED;
				transfer_size = SIZE_BYTE;
				break;
			}
			break;

		case 2:
		case 3: // Single Data Transfer (LDR, STR)
			style = STYLE_UNSIGNED;
			// Determine transfer size (B bit)
			if (((opcode >> 22) & 1) == 1)
				transfer_size = SIZE_BYTE;
			else
				transfer_size = SIZE_INT;
			break;

		default:
			output_log(_T("ARM: Handling of instruction 0x%08x not supported.\n"), opcode);
		}

		// Determine transfer type (L bit)
		if (((opcode >> 20) & 1) == 1)
			transfer_type = TYPE_LOAD;
		else
			transfer_type = TYPE_STORE;

		// Get ARM register
		int rd = (opcode >> 12) & 0xf;

		output_log(_T("%s %s register %s\n"),
			transfer_size == SIZE_BYTE
				? _T("byte")
				: transfer_size == SIZE_WORD
				? _T("word")
				: transfer_size == SIZE_INT
				? _T("long")
				: _T("unknown"),
			transfer_type == TYPE_LOAD ? _T("load to") : _T("store from"),
			reg_names[rd]);

		if (transfer_size != SIZE_UNKNOWN)
		{
			if (transfer_type == TYPE_LOAD)
			{
				// Perform load via indirect memory call
				uae_u32 oldval = pregs[rd];
				switch (transfer_size)
				{
				case SIZE_BYTE:
					pregs[rd] = style == STYLE_SIGNED ? (uae_s8)get_byte(amiga_addr) : uae_u8(get_byte(amiga_addr));
					break;

				case SIZE_WORD:
					pregs[rd] =	bswap_16(style == STYLE_SIGNED ? (uae_s16)get_word(amiga_addr) : uae_u16(get_word(amiga_addr)));
					break;

				case SIZE_INT:
					pregs[rd] = bswap_32(get_long(amiga_addr));
					break;
				}
				output_log(_T("New value in %s: 0x%08x (old: 0x%08x)\n"), reg_names[rd], pregs[rd], oldval);
			}
			else
			{
				// Perform store via indirect memory call
				switch (transfer_size)
				{
				case SIZE_BYTE:
					{
						put_byte(amiga_addr, pregs[rd]);
						break;
					}
				case SIZE_WORD:
					{
						put_word(amiga_addr, bswap_16(pregs[rd]));
						break;
					}
				case SIZE_INT:
					{
						put_long(amiga_addr, bswap_32(pregs[rd]));
						break;
					}
				}
				output_log(_T("Stored value from %s to 0x%08x\n"), reg_names[rd], amiga_addr);
			}

			// Go to next instruction
			pregs[ARM_REG_PC] += 4;
			handled = HANDLE_EXCEPTION_OK;

			if (!delete_trigger(active, fault_pc))
			{
				/* Not found in the active list. Might be a rom routine that
				   * is in the dormant list */
				delete_trigger(dormant, fault_pc);
			}
		}

		break;
	}

	in_handler--;
	return handled;
}

void signal_segv(int signum, siginfo_t* info, void* ptr)
{
	auto ucontext = (ucontext_t*)ptr;
	Dl_info dlinfo;

	output_log(_T("--- New exception ---\n"));

#ifdef TRACER
	trace_end();
#endif

	auto context = &(ucontext->uc_mcontext);
	const auto regs = &context->arm_r0;
	const auto addr = uintptr(info->si_addr);

	const auto handled = handle_exception(regs, addr);

#if SHOW_DETAILS
	if (handled != HANDLE_EXCEPTION_A4000RAM)
	{
		if (signum == 4)
		output_log(_T("Illegal Instruction\n"));
		else
		output_log(_T("Segmentation Fault\n"));

		output_log(_T("info.si_signo = %d\n"), signum);
		output_log(_T("info.si_errno = %d\n"), info->si_errno);
		output_log(_T("info.si_code = %d\n"), info->si_code);
		output_log(_T("info.si_addr = %08x\n"), info->si_addr);
		if (signum == 4)
		output_log(_T("       value = 0x%08x\n"), *((uae_u32*)(info->si_addr)));
		output_log(_T("r0  = 0x%08x\n"), ucontext->uc_mcontext.arm_r0);
		output_log(_T("r1  = 0x%08x\n"), ucontext->uc_mcontext.arm_r1);
		output_log(_T("r2  = 0x%08x\n"), ucontext->uc_mcontext.arm_r2);
		output_log(_T("r3  = 0x%08x\n"), ucontext->uc_mcontext.arm_r3);
		output_log(_T("r4  = 0x%08x\n"), ucontext->uc_mcontext.arm_r4);
		output_log(_T("r5  = 0x%08x\n"), ucontext->uc_mcontext.arm_r5);
		output_log(_T("r6  = 0x%08x\n"), ucontext->uc_mcontext.arm_r6);
		output_log(_T("r7  = 0x%08x\n"), ucontext->uc_mcontext.arm_r7);
		output_log(_T("r8  = 0x%08x\n"), ucontext->uc_mcontext.arm_r8);
		output_log(_T("r9  = 0x%08x\n"), ucontext->uc_mcontext.arm_r9);
		output_log(_T("r10 = 0x%08x\n"), ucontext->uc_mcontext.arm_r10);
		output_log(_T("FP  = 0x%08x\n"), ucontext->uc_mcontext.arm_fp);
		output_log(_T("IP  = 0x%08x\n"), ucontext->uc_mcontext.arm_ip);
		output_log(_T("SP  = 0x%08x\n"), ucontext->uc_mcontext.arm_sp);
		output_log(_T("LR  = 0x%08x\n"), ucontext->uc_mcontext.arm_lr);
		output_log(_T("PC  = 0x%08x\n"), ucontext->uc_mcontext.arm_pc);
		output_log(_T("CPSR = 0x%08x\n"), ucontext->uc_mcontext.arm_cpsr);
		output_log(_T("Fault Address = 0x%08x\n"), ucontext->uc_mcontext.fault_address);
		output_log(_T("Trap no = 0x%08x\n"), ucontext->uc_mcontext.trap_no);
		output_log(_T("Err Code = 0x%08x\n"), ucontext->uc_mcontext.error_code);
		output_log(_T("Old Mask = 0x%08x\n"), ucontext->uc_mcontext.oldmask);

		auto getaddr = (void *)ucontext->uc_mcontext.arm_lr;
		if (dladdr(getaddr, &dlinfo))
		output_log(_T("LR - 0x%08X: <%s> (%s)\n"), getaddr, dlinfo.dli_sname, dlinfo.dli_fname);
		else
		output_log(_T("LR - 0x%08X: symbol not found\n"), getaddr);
	}
#endif

#if SHOW_DETAILS > 1
	if (handled != HANDLE_EXCEPTION_A4000RAM)
	{
		output_log(_T("Stack trace:\n"));

#define MAX_BACKTRACE 20

		void* array[MAX_BACKTRACE];
		auto size = backtrace(array, MAX_BACKTRACE);
		for (auto i = 0; i < size; ++i)
		{
			if (dladdr(array[i], &dlinfo))
			{
				auto symname = dlinfo.dli_sname;
				output_log(_T("0x%08x <%s + 0x%08x> (%s)\n"), array[i], symname,
					(unsigned long)array[i] - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname);
			}
		}

		auto ip = (void*)ucontext->uc_mcontext.arm_r10;
		auto bp = (void**)ucontext->uc_mcontext.arm_r10;
		auto f = 0;
		while (bp && ip)
		{
			if (!dladdr(ip, &dlinfo))
			{
				output_log(_T("IP out of range\n"));
				break;
			}
			const char* symname = dlinfo.dli_sname;
			output_log(_T("%02d: 0x%08x <%s + 0x%08x> (%s)\n"), ++f, ip, symname,
				(unsigned long)ip - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname);
			if (dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
				break;
			ip = bp[1];
			bp = (void**)bp[0];
		}

		output_log(_T("Stack trace (non-dedicated):\n"));
		char** strings;
		void* bt[100];
		auto sz = backtrace(bt, 100);
		strings = backtrace_symbols(bt, sz);
		for (auto i = 0; i < sz; ++i)
		output_log(_T("%s\n"), strings[i]);
		output_log(_T("End of stack trace.\n"));
	}
#endif

	output_log(_T("--- end exception ---\n"));

	if (handled != HANDLE_EXCEPTION_A4000RAM)
	{
		--max_signals;
		if (max_signals <= 0)
		{
			target_startup_msg(_T("Exception"), _T("Too many access violations. Please turn off JIT."));
			uae_restart(1, nullptr);
			return;
		}
	}

	if (handled != HANDLE_EXCEPTION_NONE)
		return;

	SDL_Quit();
	exit(1);
}


void signal_buserror(int signum, siginfo_t* info, void* ptr)
{
	auto ucontext = (ucontext_t*)ptr;
	Dl_info dlinfo;

	output_log(_T("--- New exception ---\n"));

#ifdef TRACER
	trace_end();
#endif

	auto context = &(ucontext->uc_mcontext);
	auto regs = &context->arm_r0;
	auto addr = uintptr(info->si_addr);

	output_log(_T("info.si_signo = %d\n"), signum);
	output_log(_T("info.si_errno = %d\n"), info->si_errno);
	output_log(_T("info.si_code = %d\n"), info->si_code);
	output_log(_T("info.si_addr = %08x\n"), info->si_addr);
	if (signum == 4)
		output_log(_T("       value = 0x%08x\n"), *((uae_u32*)(info->si_addr)));
	output_log(_T("r0  = 0x%08x\n"), ucontext->uc_mcontext.arm_r0);
	output_log(_T("r1  = 0x%08x\n"), ucontext->uc_mcontext.arm_r1);
	output_log(_T("r2  = 0x%08x\n"), ucontext->uc_mcontext.arm_r2);
	output_log(_T("r3  = 0x%08x\n"), ucontext->uc_mcontext.arm_r3);
	output_log(_T("r4  = 0x%08x\n"), ucontext->uc_mcontext.arm_r4);
	output_log(_T("r5  = 0x%08x\n"), ucontext->uc_mcontext.arm_r5);
	output_log(_T("r6  = 0x%08x\n"), ucontext->uc_mcontext.arm_r6);
	output_log(_T("r7  = 0x%08x\n"), ucontext->uc_mcontext.arm_r7);
	output_log(_T("r8  = 0x%08x\n"), ucontext->uc_mcontext.arm_r8);
	output_log(_T("r9  = 0x%08x\n"), ucontext->uc_mcontext.arm_r9);
	output_log(_T("r10 = 0x%08x\n"), ucontext->uc_mcontext.arm_r10);
	output_log(_T("FP  = 0x%08x\n"), ucontext->uc_mcontext.arm_fp);
	output_log(_T("IP  = 0x%08x\n"), ucontext->uc_mcontext.arm_ip);
	output_log(_T("SP  = 0x%08x\n"), ucontext->uc_mcontext.arm_sp);
	output_log(_T("LR  = 0x%08x\n"), ucontext->uc_mcontext.arm_lr);
	output_log(_T("PC  = 0x%08x\n"), ucontext->uc_mcontext.arm_pc);
	output_log(_T("CPSR = 0x%08x\n"), ucontext->uc_mcontext.arm_cpsr);
	output_log(_T("Fault Address = 0x%08x\n"), ucontext->uc_mcontext.fault_address);
	output_log(_T("Trap no = 0x%08x\n"), ucontext->uc_mcontext.trap_no);
	output_log(_T("Err Code = 0x%08x\n"), ucontext->uc_mcontext.error_code);
	output_log(_T("Old Mask = 0x%08x\n"), ucontext->uc_mcontext.oldmask);

	auto getaddr = (void *)ucontext->uc_mcontext.arm_lr;
	if (dladdr(getaddr, &dlinfo))
		output_log(_T("LR - 0x%08X: <%s> (%s)\n"), getaddr, dlinfo.dli_sname, dlinfo.dli_fname);
	else
		output_log(_T("LR - 0x%08X: symbol not found\n"), getaddr);

	output_log(_T("Stack trace:\n"));

#define MAX_BACKTRACE 20

	void* array[MAX_BACKTRACE];
	auto size = backtrace(array, MAX_BACKTRACE);
	for (auto i = 0; i < size; ++i)
	{
		if (dladdr(array[i], &dlinfo))
		{
			auto symname = dlinfo.dli_sname;
			output_log(_T("0x%08x <%s + 0x%08x> (%s)\n"), array[i], symname,
				(unsigned long)array[i] - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname);
		}
	}

	auto ip = (void*)ucontext->uc_mcontext.arm_r10;
	auto bp = (void**)ucontext->uc_mcontext.arm_r10;
	auto f = 0;
	while (bp && ip)
	{
		if (!dladdr(ip, &dlinfo))
		{
			output_log(_T("IP out of range\n"));
			break;
		}
		auto symname = dlinfo.dli_sname;
		output_log(_T("%02d: 0x%08x <%s + 0x%08x> (%s)\n"), ++f, ip, symname,
			(unsigned long)ip - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname);
		if (dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
			break;
		ip = bp[1];
		bp = (void**)bp[0];
	}

	output_log(_T("Stack trace (non-dedicated):\n"));
	char** strings;
	void* bt[100];
	auto sz = backtrace(bt, 100);
	strings = backtrace_symbols(bt, sz);
	for (int i = 0; i < sz; ++i)
		output_log(_T("%s\n"), strings[i]);
	output_log(_T("End of stack trace.\n"));

	output_log(_T("--- end exception ---\n"));

	SDL_Quit();
	exit(1);
}


void signal_term(int signum, siginfo_t* info, void* ptr)
{
	output_log(_T("--- SIGTERM ---\n"));

#ifdef TRACER
	trace_end();
#endif

	SDL_Quit();
	exit(1);
}
