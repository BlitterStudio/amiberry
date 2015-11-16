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
#include "events.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "jit/comptbl.h"
#include "jit/compemu.h"

#include <asm/sigcontext.h>
#include <signal.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <SDL.h>

#define DEBUG 0
#include "debug.h"


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

static int in_handler=0;

enum {
  ARM_REG_PC = 15,
  ARM_REG_CPSR = 16
};

static inline void unknown_instruction(uae_u32 instr) 
{
	panicbug("Unknown instruction %08x!", instr);
  SDL_Quit();
	abort();
}


static bool handle_arm_instruction(unsigned long *pregs, uintptr addr) 
{
	unsigned int *pc = (unsigned int *)pregs[ARM_REG_PC];

	panicbug("IP: %p [%08x] %p\n", pc, pc[0], addr);
	if (pc == 0) return false;

	if (in_handler > 0) 
	{
    panicbug("Segmentation fault in handler :-(");
    return false;
  }

  in_handler += 1;

	transfer_type_t transfer_type = TYPE_UNKNOWN;
	int transfer_size = SIZE_UNKNOWN;
	enum { SIGNED, UNSIGNED };
	int style = UNSIGNED;

	// Handle load/store instructions only
	const unsigned int opcode = pc[0];
	switch ((opcode >> 25) & 7) {
		case 0: // Halfword and Signed Data Transfer (LDRH, STRH, LDRSB, LDRSH)
			// Determine transfer size (S/H bits)
			switch ((opcode >> 5) & 3) {
				case 0: // SWP instruction
					panicbug("FIXME: SWP Instruction\n");
					break;
				case 1: // Unsigned halfwords
	  				transfer_size = SIZE_WORD;
	  				break;
				case 3: // Signed halfwords
					style = SIGNED;
	  				transfer_size = SIZE_WORD;
	  				break;
				case 2: // Signed byte
					style = SIGNED;
				  	transfer_size = SIZE_BYTE;
	  				break;
				}
			break;
  		case 2:
  		case 3: // Single Data Transfer (LDR, STR)
			style = UNSIGNED;
			// Determine transfer size (B bit)
			if (((opcode >> 22) & 1) == 1)
	  			transfer_size = SIZE_BYTE;
			else
	  			transfer_size = SIZE_INT;
			break;
  		default:
			panicbug("FIXME: support load/store mutliple?\n");
      return false;
  	}

  	// Check for invalid transfer size (SWP instruction?)
  	if (transfer_size == SIZE_UNKNOWN) {
  		panicbug("Invalid transfer size\n");
      return false;
	}

	// Determine transfer type (L bit)
	if (((opcode >> 20) & 1) == 1)
		transfer_type = TYPE_LOAD;
	else
		transfer_type = TYPE_STORE;

  int rd = (opcode >> 12) & 0xf;
#if DEBUG
  static const char * reg_names[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"
  };
  panicbug("%s %s register %s\n",
		 transfer_size == SIZE_BYTE ? "byte" :
		 transfer_size == SIZE_WORD ? "word" :
		 transfer_size == SIZE_INT ? "long" : "unknown",
		 transfer_type == TYPE_LOAD ? "load to" : "store from",
		 reg_names[rd]);
  panicbug("\n");
//  for (int i = 0; i < 16; i++) {
//  	panicbug("%s : %p", reg_names[i], pregs[i]);
//  }
#endif

//  if ((addr < 0x00f00000) || (addr > 0x00ffffff))
//    goto buserr;

	if (transfer_type == TYPE_LOAD) {
		switch(transfer_size) {
		  case SIZE_BYTE: {
		    pregs[rd] = style == SIGNED ? (uae_s8)get_byte(addr) : (uae_u8)get_byte(addr);
		    break;
		  }
		  case SIZE_WORD: {
		    pregs[rd] = do_byteswap_16(style == SIGNED ? (uae_s16)get_word(addr) : (uae_u16)get_word(addr));
		    break;
		  }
		  case SIZE_INT: {
		    pregs[rd] = do_byteswap_32(get_long(addr));
		    break;
		  }
		}
	} else {
		switch(transfer_size) {
		  case SIZE_BYTE: {
		    put_byte(addr, pregs[rd]);
		    break;
		  }
		  case SIZE_WORD: {
		    put_word(addr, do_byteswap_16(pregs[rd]));
		    break;
		  }
		  case SIZE_INT: {
		    put_long(addr, do_byteswap_32(pregs[rd]));
		    break;
		  }
		}
	}

 	pregs[ARM_REG_PC] += 4;
	panicbug("processed: %p \n", pregs[ARM_REG_PC]);

  in_handler--;

	return true;

buserr:
  panicbug("Amiga bus error\n");

//  BUS_ERROR(addr);
	return false;
} 


#define SIG_READ 1
#define SIG_WRITE 2

extern void dump_compiler(uae_u32 *sp);


void signal_segv(int signum, siginfo_t* info, void*ptr) 
{
  int i, f = 0;
  ucontext_t *ucontext = (ucontext_t*)ptr;
	Dl_info dlinfo;

  void **bp = 0;
  void *ip = 0;

	mcontext_t *context = &(ucontext->uc_mcontext);
	unsigned long *regs = &context->arm_r0;
	uintptr addr = (uintptr)info->si_addr;
  addr = (uae_u32) addr - (uae_u32) natmem_offset;
	if (handle_arm_instruction(regs, addr))
	  return;
	  
  if(signum == 4)
    printf("Illegal Instruction!\n");
  else
    printf("Segmentation Fault!\n");
  printf("info.si_signo = %d\n", signum);
  printf("info.si_errno = %d\n", info->si_errno);
//  printf("info.si_code = %d (%s)\n", info->si_code, si_codes[info->si_code]);
  printf("info.si_code = %d\n", info->si_code);
  printf("info.si_addr = %p\n", info->si_addr);
  if(signum == 4)
    printf("       value = 0x%08x\n", *((uae_u32*)(info->si_addr)));
  printf("reg[%02d] = 0x%08x\n",0 , ucontext->uc_mcontext.arm_r0);
  printf("reg[%02d] = 0x%08x\n",1 , ucontext->uc_mcontext.arm_r1);
  printf("reg[%02d] = 0x%08x\n",2 , ucontext->uc_mcontext.arm_r2);
  printf("reg[%02d] = 0x%08x\n",3 , ucontext->uc_mcontext.arm_r3);
  printf("reg[%02d] = 0x%08x\n",4 , ucontext->uc_mcontext.arm_r4);
  printf("reg[%02d] = 0x%08x\n",5 , ucontext->uc_mcontext.arm_r5);
  printf("reg[%02d] = 0x%08x\n",6 , ucontext->uc_mcontext.arm_r6);
  printf("reg[%02d] = 0x%08x\n",7 , ucontext->uc_mcontext.arm_r7);
  printf("reg[%02d] = 0x%08x\n",8 , ucontext->uc_mcontext.arm_r8);
  printf("reg[%02d] = 0x%08x\n",9 , ucontext->uc_mcontext.arm_r9);
  printf("reg[%02d] = 0x%08x\n",10 , ucontext->uc_mcontext.arm_r10);
  printf("FP = 0x%08x\n", ucontext->uc_mcontext.arm_fp);
  printf("IP = 0x%08x\n", ucontext->uc_mcontext.arm_ip);
  printf("SP = 0x%08x\n", ucontext->uc_mcontext.arm_sp);
  printf("LR = 0x%08x\n", ucontext->uc_mcontext.arm_lr);
  printf("PC = 0x%08x\n", ucontext->uc_mcontext.arm_pc);
  printf("CPSR = 0x%08x\n", ucontext->uc_mcontext.arm_cpsr);
  printf("Fault Address = 0x%08x\n", ucontext->uc_mcontext.fault_address);
  printf("Trap no = 0x%08x\n", ucontext->uc_mcontext.trap_no);
  printf("Err Code = 0x%08x\n", ucontext->uc_mcontext.error_code);
  printf("Old Mask = 0x%08x\n", ucontext->uc_mcontext.oldmask);

  dump_compiler((uae_u32*)ucontext->uc_mcontext.arm_pc);
  
//	printf("Stack trace:\n");
//  ip = (void*)ucontext->uc_mcontext.arm_r10;
//  bp = (void**)ucontext->uc_mcontext.arm_r10;
//  while(bp && ip) {
//    if (!dladdr(ip, &dlinfo)) {
//      printf("IP out of range\n");
//      break;
//    }
//    const char *symname = dlinfo.dli_sname;
//	  printf("% 2d: %p <%s + 0x%08x> (%s)\n", ++f, ip, symname,
//      (unsigned long)ip - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname);
//	  if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
//      break;
//    ip = bp[1];
//    bp = (void**)bp[0];
//  }
//
//	printf("Stack trace (non-dedicated):\n");
//  char **strings;
//  void *bt[20];
//  int sz = backtrace(bt, 20);
//  strings = backtrace_symbols(bt, sz);
//  for(i = 0; i < sz; ++i)
//    printf("%s\n", strings[i]);
//	printf("End of stack trace.\n");

  SDL_Quit();
  abort();
}
