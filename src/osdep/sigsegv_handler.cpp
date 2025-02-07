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

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#ifdef JIT
#include "jit/comptbl.h"
#include "jit/compemu.h"
#endif
#include "uae.h"

#if !defined(__MACH__) && !defined(CPU_AMD64) && !defined(__x86_64__) && !defined(__riscv)
#include <asm/sigcontext.h>
#else
#include <sys/ucontext.h>
#endif
#include <csignal>
#include <dlfcn.h>

#ifdef __GLIBC__
#define HAVE_EXECINFO
#endif

#ifdef HAVE_EXECINFO
#include <execinfo.h>
#endif
#include <SDL.h>

#ifdef JIT
extern uae_u8* current_compile_p;
extern uae_u8* compiled_code;
extern uae_u8 *popallspace;
extern blockinfo* active;
extern blockinfo* dormant;
extern void invalidate_block(blockinfo* bi);
extern void raise_in_cl_list(blockinfo* bi);
#endif
  

#define SHOW_DETAILS 2

#define output_log write_log

enum transfer_type_t {
	TYPE_UNKNOWN,
	TYPE_LOAD,
	TYPE_STORE
};

enum type_size_t {
	SIZE_UNKNOWN,
	SIZE_BYTE,
	SIZE_WORD,
	SIZE_INT
};

enum style_type_t { 
  STYLE_SIGNED, 
  STYLE_UNSIGNED 
};

#define HANDLE_EXCEPTION_NONE 0
#define HANDLE_EXCEPTION_OK 1
#define HANDLE_EXCEPTION_A4000RAM 2

static int in_handler = 0;
static int max_signals = 200;  

void init_max_signals()
{
#ifdef WITH_LOGGING
	max_signals = 20;
#else
	max_signals = 200;
#endif
}


#if defined(CPU_AARCH64)

#ifdef JIT
static int delete_trigger(blockinfo *bi, void *pc)
{
	while (bi) {
		if (bi->handler && (uae_u8*)bi->direct_handler <= pc &&	(uae_u8*)bi->nexthandler > pc) {
			output_log(_T("JIT: Deleted trigger (0x%016x < 0x%016x < 0x%016x) 0x%016x\n"),
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
#endif


typedef uae_u64 uintptr;
#ifndef __MACH__
static int handle_exception(mcontext_t* sigcont, long fault_addr)
#else
static int handle_exception(mcontext_t sigcont, long fault_addr)
#endif
{
	int handled = HANDLE_EXCEPTION_NONE;
// Mac OS X struct for this is different
#ifndef __MACH__
	auto fault_pc = static_cast<uintptr>(sigcont->pc);
#else
	auto fault_pc = static_cast<uintptr>(sigcont->__ss.__pc);
#endif

	if (fault_pc == 0) {
		output_log(_T("PC is NULL.\n"));
		return HANDLE_EXCEPTION_NONE;
	}

	// Check for exception in handler
	if (in_handler > 0) {
		output_log(_T("Segmentation fault in handler.\n"));
		return HANDLE_EXCEPTION_NONE;
	}
	++in_handler;

#ifdef JIT
	for (;;) {
		// We analyze only exceptions from JIT
		if (currprefs.cachesize == 0) {
			output_log(_T("JIT not in use.\n"));
			break;
		}

		// Did the error happen in compiled code?
		if (fault_pc >= uintptr(compiled_code) && fault_pc < uintptr(current_compile_p))
			output_log(_T("Error in compiled code.\n"));
		else if (fault_pc >= uintptr(popallspace) && fault_pc < uintptr(popallspace + POPALLSPACE_SIZE))
			output_log(_T("Error in popallspace code.\n"));
		else {
			output_log(_T("Error not in JIT code.\n"));
			break;
		}

		// Get Amiga address of illegal memory address
		long amiga_addr = long(fault_addr) - long(natmem_offset);

		// Check for stupid RAM detection of kickstart
		if (a3000lmem_bank.allocated_size > 0 && amiga_addr >= a3000lmem_bank.start - 0x00100000 && amiga_addr < a3000lmem_bank.start - 0x00100000 + 8) {
			output_log(_T("  Stupid kickstart detection for size of ramsey_low at 0x%08lx.\n"), amiga_addr);
#ifndef __MACH__
			sigcont->pc += 4;
#else
			sigcont->__ss.__pc += 4;
#endif
			handled = HANDLE_EXCEPTION_A4000RAM;
			break;
		}

		// Check for stupid RAM detection of kickstart
		if (a3000hmem_bank.allocated_size > 0 && amiga_addr >= a3000hmem_bank.start + a3000hmem_bank.allocated_size && amiga_addr < a3000hmem_bank.start + a3000hmem_bank.allocated_size + 8) {
			output_log(_T("  Stupid kickstart detection for size of ramsey_high at 0x%08lx.\n"), amiga_addr);
#ifndef __MACH__
			sigcont->pc += 4;
#else
			sigcont->__ss.__pc += 4;
#endif
			handled = HANDLE_EXCEPTION_A4000RAM;
			break;
		}

		// Get memory bank of address
		addrbank* ab = &get_mem_bank(amiga_addr);
		if (ab)
			output_log(_T("JIT: Address bank: %s, address %08lx\n"), ab->name ? ab->name : _T("NONE"), amiga_addr);

		// Analyze AARCH64 instruction
		const unsigned int opcode = ((uae_u32*)fault_pc)[0];
		output_log(_T("JIT: AARCH64 opcode = 0x%08x\n"), opcode);
#ifdef JIT_DEBUG
		extern void disam_range(void* start, void* stop);
		disam_range((void*)(fault_pc - 128), (void*)(fault_pc + 32));
#endif
		transfer_type_t transfer_type = TYPE_UNKNOWN;
		int transfer_size = SIZE_UNKNOWN;

		unsigned int masked_op = opcode & 0xfffffc00;
		switch (masked_op) {
		case 0x383b6800: // STRB_wXx
			transfer_size = SIZE_BYTE;
			transfer_type = TYPE_STORE;
			break;

		case 0x783b6800: // STRH_wXx
			transfer_size = SIZE_WORD;
			transfer_type = TYPE_STORE;
			break;

		case 0xb83b6800: // STR_wXx
			transfer_size = SIZE_INT;
			transfer_type = TYPE_STORE;
			break;

		case 0x387b6800: // LDRB_wXx
			transfer_size = SIZE_BYTE;
			transfer_type = TYPE_LOAD;
			break;

		case 0x787b6800: // LDRH_wXx
			transfer_size = SIZE_WORD;
			transfer_type = TYPE_LOAD;
			break;

		case 0xb87b6800: // LDR_wXx
			transfer_size = SIZE_INT;
			transfer_type = TYPE_LOAD;
			break;

		default:
			output_log(_T("AARCH64: Handling of instruction 0x%08x not supported.\n"), opcode);
		}

		if (transfer_size != SIZE_UNKNOWN) {
			// Get AARCH64 register
			int rd = opcode & 0x1f;

			output_log(_T("%s %s register x%d\n"),
				transfer_size == SIZE_BYTE ? _T("byte") : transfer_size == SIZE_WORD ? _T("word") : transfer_size == SIZE_INT ? _T("long") : _T("unknown"),
				transfer_type == TYPE_LOAD ? _T("load to") : _T("store from"),
				rd);

			if (transfer_type == TYPE_LOAD) {
				// Perform load via indirect memory call
#ifndef __MACH__
				uae_u32 oldval = sigcont->regs[rd];
#else
				uae_u32 oldval = sigcont->__ss.__x[rd];
#endif
				switch (transfer_size) {
				case SIZE_BYTE:
#ifndef __MACH__
					sigcont->regs[rd] = (uae_u8)get_byte(amiga_addr);
#else
					sigcont->__ss.__x[rd] = (uae_u8)get_byte(amiga_addr);
#endif
					break;

				case SIZE_WORD:
#ifndef __MACH__
					sigcont->regs[rd] = uae_bswap_16((uae_u16)get_word(amiga_addr));
#else
					sigcont->__ss.__x[rd] = uae_bswap_16((uae_u16)get_word(amiga_addr));
#endif
					break;

				case SIZE_INT:
#ifndef __MACH__
					sigcont->regs[rd] = uae_bswap_32(get_long(amiga_addr));
#else
					sigcont->__ss.__x[rd] = uae_bswap_32(get_long(amiga_addr));
#endif
					break;
				}
#ifndef __MACH__
				output_log(_T("New value in x%d: 0x%08llx (old: 0x%08x)\n"), rd, sigcont->regs[rd], oldval);
#else
				output_log(_T("New value in x%d: 0x%08llx (old: 0x%08x)\n"), rd, sigcont->__ss.__x[rd], oldval);
#endif
			}
			else {
				// Perform store via indirect memory call
				switch (transfer_size) {
				case SIZE_BYTE: {
#ifndef __MACH__
					put_byte(amiga_addr, sigcont->regs[rd]);
#else
					put_byte(amiga_addr, sigcont->__ss.__x[rd]);
#endif
					break;
				}
				case SIZE_WORD: {
#ifndef __MACH__
					put_word(amiga_addr, uae_bswap_16(sigcont->regs[rd]));
#else
					put_word(amiga_addr, uae_bswap_16(sigcont->__ss.__x[rd]));
#endif
					break;
				}
				case SIZE_INT: {
#ifndef __MACH__
					put_long(amiga_addr, uae_bswap_32(sigcont->regs[rd]));
#else
					put_long(amiga_addr, uae_bswap_32(sigcont->__ss.__x[rd]));
#endif
					break;
				}
				}
				output_log(_T("Stored value from x%d to 0x%08lx\n"), rd, amiga_addr);
			}

			// Go to next instruction
#ifndef __MACH__
			sigcont->pc += 4;
#else
			sigcont->__ss.__pc += 4;
#endif
			handled = HANDLE_EXCEPTION_OK;

			if (!delete_trigger(active, (void*)fault_pc)) {
				/* Not found in the active list. Might be a rom routine that
				 * is in the dormant list */
				delete_trigger(dormant, (void*)fault_pc);
			}
		}

		break;
	}
#endif

	in_handler--;
	return handled;
}

void signal_segv(int signum, siginfo_t* info, void* ptr)
{
	int handled = HANDLE_EXCEPTION_NONE;

	auto ucontext = static_cast<ucontext_t*>(ptr);
	Dl_info dlinfo;

	output_log(_T("--- New exception ---\n"));

#ifdef TRACER
	trace_end();
#endif

#ifndef __MACH__
	mcontext_t* context = &(ucontext->uc_mcontext);
	unsigned long long* regs = context->regs;
#else
	mcontext_t context = ucontext->uc_mcontext;
	unsigned long long* regs = context->__ss.__x;
#endif

	const auto addr = reinterpret_cast<uintptr>(info->si_addr);

	handled = handle_exception(context, addr);

#if SHOW_DETAILS
	if (handled != HANDLE_EXCEPTION_A4000RAM) {
		if (signum == 4)
			output_log(_T("Illegal Instruction\n"));
		else
			output_log(_T("Segmentation Fault\n"));

		output_log(_T("info.si_signo = %d\n"), signum);
		output_log(_T("info.si_errno = %d\n"), info->si_errno);
		output_log(_T("info.si_code = %d\n"), info->si_code);
		output_log(_T("info.si_addr = %08x\n"), info->si_addr);
		if (signum == 4)
			output_log(_T("       value = 0x%08x\n"), *static_cast<uae_u32*>(info->si_addr));

		for (int i = 0; i < 31; ++i)
#ifndef __MACH__
			output_log(_T("x%02d  = 0x%016llx\n"), i, ucontext->uc_mcontext.regs[i]);
#else
			output_log(_T("x%02d  = 0x%016llx\n"), i, context->__ss.__x[i]);
#endif
#ifndef __MACH__
		output_log(_T("SP  = 0x%016llx\n"), ucontext->uc_mcontext.sp);
		output_log(_T("PC  = 0x%016llx\n"), ucontext->uc_mcontext.pc);
		output_log(_T("Fault Address = 0x%016llx\n"), ucontext->uc_mcontext.fault_address);
		output_log(_T("pstate  = 0x%016llx\n"), ucontext->uc_mcontext.pstate);
		void* getaddr = (void*)ucontext->uc_mcontext.regs[30];
#else
		output_log(_T("SP  = 0x%016llx\n"), context->__ss.__sp);
		output_log(_T("PC  = 0x%016llx\n"), context->__ss.__pc);
		// I can't find an obvious way to get at the fault address from the OS X side
		//output_log(_T("Fault Address = 0x%016llx\n"), ucontext->uc_mcontext.fault_address);
		output_log(_T("pstate  = 0x%016llx\n"), context->__ss.__cpsr);
		void* getaddr = reinterpret_cast<void*>(context->__ss.__x[30]);
#endif

		if (dladdr(getaddr, &dlinfo))
			output_log(_T("LR - 0x%08X: <%s> (%s)\n"), getaddr, dlinfo.dli_sname, dlinfo.dli_fname);
		else
			output_log(_T("LR - 0x%08X: symbol not found\n"), getaddr);
	}
#endif

#if SHOW_DETAILS > 1 && defined(HAVE_EXECINFO)
	if (handled != HANDLE_EXCEPTION_A4000RAM) {
		output_log(_T("Stack trace:\n"));

#define MAX_BACKTRACE 20

		void* array[MAX_BACKTRACE];
		int size = backtrace(array, MAX_BACKTRACE);
		for (int i = 0; i < size; ++i)
		{
			if (dladdr(array[i], &dlinfo)) {
				const char* symname = dlinfo.dli_sname;
				output_log(_T("0x%08x <%s + 0x%08x> (%s)\n"), array[i], symname,
					reinterpret_cast<unsigned long>(array[i]) - reinterpret_cast<unsigned long>(dlinfo.dli_saddr), dlinfo.dli_fname);
			}
		}

		output_log(_T("Stack trace (non-dedicated):\n"));
		char** strings;
		void* bt[100];
		int sz = backtrace(bt, 100);
		strings = backtrace_symbols(bt, sz);
		for (int i = 0; i < sz; ++i)
			output_log(_T("%s\n"), strings[i]);
		output_log(_T("End of stack trace.\n"));
	}
#endif

	output_log(_T("--- end exception ---\n"));

	if (handled != HANDLE_EXCEPTION_A4000RAM) {
		--max_signals;
		if (max_signals <= 0) {
			target_startup_msg(_T("Exception"), _T("Too many access violations. Please turn off JIT."));
			uae_restart(&currprefs, 1, nullptr);
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
	auto ucontext = static_cast<ucontext_t*>(ptr);
	Dl_info dlinfo;

	output_log(_T("--- New exception ---\n"));

#ifdef TRACER
	trace_end();
#endif

#ifndef __MACH__
	mcontext_t* context = &(ucontext->uc_mcontext);
	unsigned long long* regs = context->regs;
#else
	mcontext_t context = ucontext->uc_mcontext;
	unsigned long long* regs = context->__ss.__x;
#endif

	auto addr = reinterpret_cast<uintptr_t>(info->si_addr);

	output_log(_T("info.si_signo = %d\n"), signum);
	output_log(_T("info.si_errno = %d\n"), info->si_errno);
	output_log(_T("info.si_code = %d\n"), info->si_code);
	output_log(_T("info.si_addr = %08x\n"), info->si_addr);
	if (signum == 4)
		output_log(_T("       value = 0x%08x\n"), *static_cast<uae_u32*>(info->si_addr));

	for (int i = 0; i < 31; ++i)
#ifndef __MACH__
		output_log(_T("x%02d  = 0x%016llx\n"), i, ucontext->uc_mcontext.regs[i]);
	output_log(_T("SP  = 0x%016llx\n"), ucontext->uc_mcontext.sp);
	output_log(_T("PC  = 0x%016llx\n"), ucontext->uc_mcontext.pc);
	output_log(_T("Fault Address = 0x%016llx\n"), ucontext->uc_mcontext.fault_address);
	output_log(_T("pstate  = 0x%016llx\n"), ucontext->uc_mcontext.pstate);

	void* getaddr = (void*)ucontext->uc_mcontext.regs[30];
#else
		output_log(_T("x%02d  = 0x%016llx\n"), i, context->__ss.__x[i]);
	output_log(_T("SP  = 0x%016llx\n"), context->__ss.__sp);
	output_log(_T("PC  = 0x%016llx\n"), context->__ss.__pc);
	// Can't find an obvious way to get at the fault address from the OS X side
	//output_log(_T("Fault Address = 0x%016llx\n"), context->uc_mcontext.fault_address);
	output_log(_T("pstate  = 0x%016llx\n"), context->__ss.__cpsr);

	void* getaddr = reinterpret_cast<void*>(context->__ss.__x[30]);
#endif


	if (dladdr(getaddr, &dlinfo))
		output_log(_T("LR - 0x%08X: <%s> (%s)\n"), getaddr, dlinfo.dli_sname, dlinfo.dli_fname);
	else
		output_log(_T("LR - 0x%08X: symbol not found\n"), getaddr);

#if defined(HAVE_EXECINFO)
	output_log(_T("Stack trace:\n"));

#define MAX_BACKTRACE 20

	void* array[MAX_BACKTRACE];
	int size = backtrace(array, MAX_BACKTRACE);
	for (int i = 0; i < size; ++i)
	{
		if (dladdr(array[i], &dlinfo)) {
			const char* symname = dlinfo.dli_sname;
			output_log(_T("0x%08x <%s + 0x%08x> (%s)\n"), array[i], symname,
				reinterpret_cast<unsigned long>(array[i]) - reinterpret_cast<unsigned long>(dlinfo.dli_saddr), dlinfo.dli_fname);
		}
	}

	output_log(_T("Stack trace (non-dedicated):\n"));
	void* bt[100];
	const int sz = backtrace(bt, 100);
	char** strings = backtrace_symbols(bt, sz);
	for (int i = 0; i < sz; ++i)
		output_log(_T("%s\n"), strings[i]);
	output_log(_T("End of stack trace.\n"));
#endif /* HAVE_EXECINFO */
	output_log(_T("--- end exception ---\n"));

	SDL_Quit();
	exit(1);
}

#elif defined(__arm__)

enum {
	ARM_REG_PC = 15,
	ARM_REG_CPSR = 16
};

static const char* reg_names[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10/sl", "r11/fp", "r12/ip", "r13/sp", "r14/lr", "r15/pc"
};

#ifdef JIT
static int delete_trigger(blockinfo *bi, void *pc)
{
	while (bi) {
		if (bi->handler && (uae_u8*)bi->direct_handler <= pc &&	(uae_u8*)bi->nexthandler > pc) {
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
#endif

typedef uae_u32 uintptr;

static int handle_exception(unsigned long* pregs, long fault_addr)
{
	auto handled = HANDLE_EXCEPTION_NONE;
	auto* fault_pc = (unsigned int*)pregs[ARM_REG_PC];

	if (fault_pc == 0) {
		output_log(_T("PC is NULL.\n"));
		return HANDLE_EXCEPTION_NONE;
	}

	// Check for exception in handler
	if (in_handler > 0) {
		output_log(_T("Segmentation fault in handler.\n"));
		return HANDLE_EXCEPTION_NONE;
	}
	++in_handler;

#ifdef JIT
	for (;;) {
		// We analyze only exceptions from JIT
		if (currprefs.cachesize == 0) {
			output_log(_T("JIT not in use.\n"));
			break;
		}

		// Did the error happen in compiled code?
		if ((uae_u8*)fault_pc >= compiled_code && (uae_u8*)fault_pc < current_compile_p)
			output_log(_T("Error in compiled code.\n"));
		else if ((uae_u8*)fault_pc >= popallspace && (uae_u8*)fault_pc < popallspace + POPALLSPACE_SIZE)
			output_log(_T("Error in popallspace code.\n"));
		else {
			output_log(_T("Error not in JIT code.\n"));
			break;
		}

		// Get Amiga address of illegal memory address
		auto amiga_addr = (long)fault_addr - (long)natmem_offset;

		// Check for stupid RAM detection of kickstart
		if (a3000lmem_bank.allocated_size > 0 && amiga_addr >= a3000lmem_bank.start - 0x00100000 && amiga_addr < a3000lmem_bank.start - 0x00100000 + 8) {
			output_log(_T("  Stupid kickstart detection for size of ramsey_low at 0x%08x.\n"), amiga_addr);
			pregs[ARM_REG_PC] += 4;
			handled = HANDLE_EXCEPTION_A4000RAM;
			break;
		}

		// Check for stupid RAM detection of kickstart
		if (a3000hmem_bank.allocated_size > 0 && amiga_addr >= a3000hmem_bank.start + a3000hmem_bank.allocated_size && amiga_addr < a3000hmem_bank.start + a3000hmem_bank.allocated_size + 8) {
			output_log(_T("  Stupid kickstart detection for size of ramsey_high at 0x%08x.\n"), amiga_addr);
			pregs[ARM_REG_PC] += 4;
			handled = HANDLE_EXCEPTION_A4000RAM;
			break;
		}
  
		// Get memory bank of address
		auto* ab = &get_mem_bank(amiga_addr);
		if (ab)
			output_log(_T("JIT: Address bank: %s, address %08x\n"), ab->name ? ab->name : _T("NONE"), amiga_addr);

		// Analyse ARM instruction
		const auto opcode = fault_pc[0];
		auto transfer_type = TYPE_UNKNOWN;
		int transfer_size = SIZE_UNKNOWN;
		int style = STYLE_UNSIGNED;
		output_log(_T("JIT: ARM opcode = 0x%08x\n"), opcode);

		// Handle load/store instructions only
		switch ((opcode >> 25) & 7) {
		case 0: // Halfword and Signed Data Transfer (LDRH, STRH, LDRSB, LDRSH)
			// Determine transfer size (S/H bits)
			switch ((opcode >> 5) & 3) {
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
			transfer_size == SIZE_BYTE ? _T("byte") : transfer_size == SIZE_WORD ? _T("word") : transfer_size == SIZE_INT ? _T("long") : _T("unknown"),
			transfer_type == TYPE_LOAD ? _T("load to") : _T("store from"),
			reg_names[rd]);

		if (transfer_size != SIZE_UNKNOWN) {
			if (transfer_type == TYPE_LOAD) {
				// Perform load via indirect memory call
				uae_u32 oldval = pregs[rd];
				switch (transfer_size) {
				case SIZE_BYTE:
					pregs[rd] = style == STYLE_SIGNED ? (uae_s8)get_byte(amiga_addr) : (uae_u8)get_byte(amiga_addr);
					break;

				case SIZE_WORD:
					pregs[rd] = uae_bswap_16(style == STYLE_SIGNED ? (uae_s16)get_word(amiga_addr) : (uae_u16)get_word(amiga_addr));
					break;

				case SIZE_INT:
					pregs[rd] = uae_bswap_32(get_long(amiga_addr));
					break;
				}
				output_log(_T("New value in %s: 0x%08x (old: 0x%08x)\n"), reg_names[rd], pregs[rd], oldval);
			}
			else {
				// Perform store via indirect memory call
				switch (transfer_size) {
				case SIZE_BYTE: {
					put_byte(amiga_addr, pregs[rd]);
					break;
				}
				case SIZE_WORD: {
					put_word(amiga_addr, uae_bswap_16(pregs[rd]));
					break;
				}
				case SIZE_INT: {
					put_long(amiga_addr, uae_bswap_32(pregs[rd]));
					break;
				}
				}
				output_log(_T("Stored value from %s to 0x%08x\n"), reg_names[rd], amiga_addr);
			}

			// Go to next instruction
			pregs[ARM_REG_PC] += 4;
			handled = HANDLE_EXCEPTION_OK;

			if (!delete_trigger(active, fault_pc)) {
				/* Not found in the active list. Might be a rom routine that
				 * is in the dormant list */
				delete_trigger(dormant, fault_pc);
			}
		}

		break;
  }
#endif

  in_handler--;
  return handled;
}

void signal_segv(int signum, siginfo_t* info, void* ptr)
{
	int handled = HANDLE_EXCEPTION_NONE;
	ucontext_t* ucontext = (ucontext_t*)ptr;
	Dl_info dlinfo;

	output_log(_T("--- New exception ---\n"));

#ifdef TRACER
	trace_end();
#endif

#if !defined (CPU_AMD64) && !defined (__x86_64__)
	mcontext_t* context = &(ucontext->uc_mcontext);

	unsigned long* regs = &context->arm_r0;
	uintptr addr = (uintptr)info->si_addr;

	handled = handle_exception(regs, addr);

#if SHOW_DETAILS
	if (handled != HANDLE_EXCEPTION_A4000RAM) {
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

		void* getaddr = (void*)ucontext->uc_mcontext.arm_lr;
		if (dladdr(getaddr, &dlinfo))
			output_log(_T("LR - 0x%08X: <%s> (%s)\n"), getaddr, dlinfo.dli_sname, dlinfo.dli_fname);
		else
			output_log(_T("LR - 0x%08X: symbol not found\n"), getaddr);
	}
#endif

#if SHOW_DETAILS > 1 && defined(HAVE_EXECINFO)
	if (handled != HANDLE_EXCEPTION_A4000RAM) {
		output_log(_T("Stack trace:\n"));

#define MAX_BACKTRACE 20

		void* array[MAX_BACKTRACE];
		int size = backtrace(array, MAX_BACKTRACE);
		for (int i = 0; i < size; ++i)
		{
			if (dladdr(array[i], &dlinfo)) {
				const char* symname = dlinfo.dli_sname;
				output_log(_T("0x%08x <%s + 0x%08x> (%s)\n"), array[i], symname,
					(unsigned long)array[i] - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname);
			}
		}

		void* ip = (void*)ucontext->uc_mcontext.arm_r10;
		void** bp = (void**)ucontext->uc_mcontext.arm_r10;
		int f = 0;
		while (bp && ip) {
			if (!dladdr(ip, &dlinfo)) {
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
		int sz = backtrace(bt, 100);
		strings = backtrace_symbols(bt, sz);
		for (int i = 0; i < sz; ++i)
			output_log(_T("%s\n"), strings[i]);
		output_log(_T("End of stack trace.\n"));
	}
#endif

	output_log(_T("--- end exception ---\n"));

	if (handled != HANDLE_EXCEPTION_A4000RAM) {
		--max_signals;
		if (max_signals <= 0) {
			target_startup_msg(_T("Exception"), _T("Too many access violations. Please turn off JIT."));
			uae_restart(&currprefs, 1, NULL);
			return;
		}
	}

	if (handled != HANDLE_EXCEPTION_NONE)
		return;
#endif

	SDL_Quit();
	exit(1);
}


void signal_buserror(int signum, siginfo_t* info, void* ptr)
{
	ucontext_t* ucontext = (ucontext_t*)ptr;
	Dl_info dlinfo;

	output_log(_T("--- New exception ---\n"));

#ifdef TRACER
	trace_end();
#endif

	mcontext_t* context = &(ucontext->uc_mcontext);

#if !defined (CPU_AMD64) && !defined (__x86_64__)
	unsigned long* regs = &context->arm_r0;
	uintptr_t addr = (uintptr_t)info->si_addr;

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
	output_log(_T("Trap no = 0x%08x\n"), ucontext->uc_mcontext.trap_no);
	output_log(_T("Err Code = 0x%08x\n"), ucontext->uc_mcontext.error_code);
	output_log(_T("Old Mask = 0x%08x\n"), ucontext->uc_mcontext.oldmask);
	output_log(_T("Fault Address = 0x%08x\n"), ucontext->uc_mcontext.fault_address);

	void* getaddr = (void*)ucontext->uc_mcontext.arm_lr;
	if (dladdr(getaddr, &dlinfo))
		output_log(_T("LR - 0x%08X: <%s> (%s)\n"), getaddr, dlinfo.dli_sname, dlinfo.dli_fname);
	else
		output_log(_T("LR - 0x%08X: symbol not found\n"), getaddr);

	output_log(_T("Stack trace:\n"));

#if defined(HAVE_EXECINFO)
#define MAX_BACKTRACE 20

	void* array[MAX_BACKTRACE];
	int size = backtrace(array, MAX_BACKTRACE);
	for (int i = 0; i < size; ++i)
	{
		if (dladdr(array[i], &dlinfo)) {
			const char* symname = dlinfo.dli_sname;
			output_log(_T("0x%08x <%s + 0x%08x> (%s)\n"), array[i], symname,
				(unsigned long)array[i] - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname);
		}
	}

	void* ip = (void*)ucontext->uc_mcontext.arm_r10;
	void** bp = (void**)ucontext->uc_mcontext.arm_r10;
	int f = 0;
	while (bp && ip) {
		if (!dladdr(ip, &dlinfo)) {
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
	int sz = backtrace(bt, 100);
	strings = backtrace_symbols(bt, sz);
	for (int i = 0; i < sz; ++i)
		output_log(_T("%s\n"), strings[i]);
	output_log(_T("End of stack trace.\n"));
#endif /* HAVE_EXECINFO */

#endif
	output_log(_T("--- end exception ---\n"));

	SDL_Quit();
	exit(1);
}

#endif

void signal_term(int signum, siginfo_t* info, void* ptr)
{
	output_log(_T("--- SIGTERM ---\n"));

#ifdef TRACER
	trace_end();
#endif

	SDL_Quit();
	exit(1);
}
