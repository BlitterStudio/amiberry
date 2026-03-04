/*************************************************************************
* Handling mistaken direct memory access                                *
*************************************************************************/

#ifdef NATMEM_OFFSET
#ifdef _WIN32 // %%% BRIAN KING WAS HERE %%%
#include <winbase.h>
#else
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sys/ucontext.h>
#endif
#include <signal.h>

#define SIG_READ 1
#define SIG_WRITE 2

static int in_handler = 0;
static uae_u8 *veccode;

#if defined(JIT_DEBUG)
#define DEBUG_ACCESS
#endif

#if defined(_WIN32) && defined(CPU_x86_64)

typedef LPEXCEPTION_POINTERS CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (context->ContextRecord->Rip)
#define CONTEXT_RAX(context) (context->ContextRecord->Rax)
#define CONTEXT_RCX(context) (context->ContextRecord->Rcx)
#define CONTEXT_RDX(context) (context->ContextRecord->Rdx)
#define CONTEXT_RBX(context) (context->ContextRecord->Rbx)
#define CONTEXT_RSP(context) (context->ContextRecord->Rsp)
#define CONTEXT_RBP(context) (context->ContextRecord->Rbp)
#define CONTEXT_RSI(context) (context->ContextRecord->Rsi)
#define CONTEXT_RDI(context) (context->ContextRecord->Rdi)
#define CONTEXT_R8(context)  (context->ContextRecord->R8)
#define CONTEXT_R9(context)  (context->ContextRecord->R9)
#define CONTEXT_R10(context) (context->ContextRecord->R10)
#define CONTEXT_R11(context) (context->ContextRecord->R11)
#define CONTEXT_R12(context) (context->ContextRecord->R12)
#define CONTEXT_R13(context) (context->ContextRecord->R13)
#define CONTEXT_R14(context) (context->ContextRecord->R14)
#define CONTEXT_R15(context) (context->ContextRecord->R15)

#elif defined(_WIN32) && defined(CPU_i386)

typedef LPEXCEPTION_POINTERS CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (context->ContextRecord->Eip)
#define CONTEXT_RAX(context) (context->ContextRecord->Eax)
#define CONTEXT_RCX(context) (context->ContextRecord->Ecx)
#define CONTEXT_RDX(context) (context->ContextRecord->Edx)
#define CONTEXT_RBX(context) (context->ContextRecord->Ebx)
#define CONTEXT_RSP(context) (context->ContextRecord->Esp)
#define CONTEXT_RBP(context) (context->ContextRecord->Ebp)
#define CONTEXT_RSI(context) (context->ContextRecord->Esi)
#define CONTEXT_RDI(context) (context->ContextRecord->Edi)

#elif defined(HAVE_STRUCT_UCONTEXT_T_UC_MCONTEXT_GREGS) && defined(CPU_x86_64)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RIP])
#define CONTEXT_RAX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RAX])
#define CONTEXT_RCX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RCX])
#define CONTEXT_RDX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RDX])
#define CONTEXT_RBX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RBX])
#define CONTEXT_RSP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RSP])
#define CONTEXT_RBP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RBP])
#define CONTEXT_RSI(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RSI])
#define CONTEXT_RDI(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RDI])
#define CONTEXT_R8(context)  (((ucontext_t *) context)->uc_mcontext.gregs[REG_R8])
#define CONTEXT_R9(context)  (((ucontext_t *) context)->uc_mcontext.gregs[REG_R9])
#define CONTEXT_R10(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R10])
#define CONTEXT_R11(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R11])
#define CONTEXT_R12(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R12])
#define CONTEXT_R13(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R13])
#define CONTEXT_R14(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R14])
#define CONTEXT_R15(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R15])

#elif defined(HAVE_STRUCT_UCONTEXT_T_UC_MCONTEXT_GREGS) && defined(CPU_i386)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_EIP])
#define CONTEXT_RAX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_EAX])
#define CONTEXT_RCX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_ECX])
#define CONTEXT_RDX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_EDX])
#define CONTEXT_RBX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_EBX])
#define CONTEXT_RSP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_ESP])
#define CONTEXT_RBP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_EBP])
#define CONTEXT_RSI(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_ESI])
#define CONTEXT_RDI(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_EDI])

#elif defined(HAVE_STRUCT_UCONTEXT_UC_MCONTEXT_GREGS) && defined(CPU_x86_64)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RIP])
#define CONTEXT_RAX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RAX])
#define CONTEXT_RCX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RCX])
#define CONTEXT_RDX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RDX])
#define CONTEXT_RBX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RBX])
#define CONTEXT_RSP(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RSP])
#define CONTEXT_RBP(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RBP])
#define CONTEXT_RSI(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RSI])
#define CONTEXT_RDI(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_RDI])
#define CONTEXT_R8(context)  (((struct ucontext *) context)->uc_mcontext.gregs[REG_R8])
#define CONTEXT_R9(context)  (((struct ucontext *) context)->uc_mcontext.gregs[REG_R9])
#define CONTEXT_R10(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_R10])
#define CONTEXT_R11(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_R11])
#define CONTEXT_R12(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_R12])
#define CONTEXT_R13(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_R13])
#define CONTEXT_R14(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_R14])
#define CONTEXT_R15(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_R15])

#elif defined(HAVE_STRUCT_UCONTEXT_UC_MCONTEXT_GREGS) && defined(CPU_i386)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_EIP])
#define CONTEXT_RAX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_EAX])
#define CONTEXT_RCX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_ECX])
#define CONTEXT_RDX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_EDX])
#define CONTEXT_RBX(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_EBX])
#define CONTEXT_RSP(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_ESP])
#define CONTEXT_RBP(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_EBP])
#define CONTEXT_RSI(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_ESI])
#define CONTEXT_RDI(context) (((struct ucontext *) context)->uc_mcontext.gregs[REG_EDI])

#elif defined(__DARWIN_UNIX03) && defined(CPU_x86_64)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (*((unsigned long *) &((ucontext_t *) context)->uc_mcontext->__ss.__rip))
#define CONTEXT_RAX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__rax)
#define CONTEXT_RCX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__rcx)
#define CONTEXT_RDX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__rdx)
#define CONTEXT_RBX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__rbx)
#define CONTEXT_RSP(context) (*((unsigned long *) &((ucontext_t *) context)->uc_mcontext->__ss.__rsp))
#define CONTEXT_RBP(context) (((ucontext_t *) context)->uc_mcontext->__ss.__rbp)
#define CONTEXT_RSI(context) (((ucontext_t *) context)->uc_mcontext->__ss.__rsi)
#define CONTEXT_RDI(context) (((ucontext_t *) context)->uc_mcontext->__ss.__rdi)
#define CONTEXT_R8(context) (((ucontext_t *)  context)->uc_mcontext->__ss.__r8)
#define CONTEXT_R9(context) (((ucontext_t *)  context)->uc_mcontext->__ss.__r9)
#define CONTEXT_R10(context) (((ucontext_t *) context)->uc_mcontext->__ss.__r10)
#define CONTEXT_R11(context) (((ucontext_t *) context)->uc_mcontext->__ss.__r11)
#define CONTEXT_R12(context) (((ucontext_t *) context)->uc_mcontext->__ss.__r12)
#define CONTEXT_R13(context) (((ucontext_t *) context)->uc_mcontext->__ss.__r13)
#define CONTEXT_R14(context) (((ucontext_t *) context)->uc_mcontext->__ss.__r14)
#define CONTEXT_R15(context) (((ucontext_t *) context)->uc_mcontext->__ss.__r15)

#elif defined(__DARWIN_UNIX03) && defined(CPU_i386)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (*((unsigned long *) &((ucontext_t *) context)->uc_mcontext->__ss.__eip))
#define CONTEXT_RAX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__eax)
#define CONTEXT_RCX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__ecx)
#define CONTEXT_RDX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__edx)
#define CONTEXT_RBX(context) (((ucontext_t *) context)->uc_mcontext->__ss.__ebx)
#define CONTEXT_RSP(context) (*((unsigned long *) &((ucontext_t *) context)->uc_mcontext->__ss.__esp))
#define CONTEXT_RBP(context) (((ucontext_t *) context)->uc_mcontext->__ss.__ebp)
#define CONTEXT_RSI(context) (((ucontext_t *) context)->uc_mcontext->__ss.__esi)
#define CONTEXT_RDI(context) (((ucontext_t *) context)->uc_mcontext->__ss.__edi)

#elif defined (AMIBERRY) && defined(CPU_x86_64) && defined(__FreeBSD__)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) ((( ucontext_t *) context)->uc_mcontext.mc_rip)
#define CONTEXT_RAX(context) ((( ucontext_t *) context)->uc_mcontext.mc_rax)
#define CONTEXT_RCX(context) ((( ucontext_t *) context)->uc_mcontext.mc_rcx)
#define CONTEXT_RDX(context) ((( ucontext_t *) context)->uc_mcontext.mc_rdx)
#define CONTEXT_RBX(context) ((( ucontext_t *) context)->uc_mcontext.mc_rbx)
#define CONTEXT_RSP(context) ((( ucontext_t *) context)->uc_mcontext.mc_rsp)
#define CONTEXT_RBP(context) ((( ucontext_t *) context)->uc_mcontext.mc_rbp)
#define CONTEXT_RSI(context) ((( ucontext_t *) context)->uc_mcontext.mc_rsi)
#define CONTEXT_RDI(context) ((( ucontext_t *) context)->uc_mcontext.mc_rdi)
#define CONTEXT_R8(context)  ((( ucontext_t *) context)->uc_mcontext.mc_r8)
#define CONTEXT_R9(context)  ((( ucontext_t *) context)->uc_mcontext.mc_r9)
#define CONTEXT_R10(context) ((( ucontext_t *) context)->uc_mcontext.mc_r10)
#define CONTEXT_R11(context) ((( ucontext_t *) context)->uc_mcontext.mc_r11)
#define CONTEXT_R12(context) ((( ucontext_t *) context)->uc_mcontext.mc_r12)
#define CONTEXT_R13(context) ((( ucontext_t *) context)->uc_mcontext.mc_r13)
#define CONTEXT_R14(context) ((( ucontext_t *) context)->uc_mcontext.mc_r14)
#define CONTEXT_R15(context) ((( ucontext_t *) context)->uc_mcontext.mc_r15)

#elif defined (AMIBERRY) && defined(CPU_x86_64)

typedef void *CONTEXT_T;
#define HAVE_CONTEXT_T 1
#define CONTEXT_RIP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RIP])
#define CONTEXT_RAX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RAX])
#define CONTEXT_RCX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RCX])
#define CONTEXT_RDX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RDX])
#define CONTEXT_RBX(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RBX])
#define CONTEXT_RSP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RSP])
#define CONTEXT_RBP(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RBP])
#define CONTEXT_RSI(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RSI])
#define CONTEXT_RDI(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_RDI])
#define CONTEXT_R8(context)  (((ucontext_t *) context)->uc_mcontext.gregs[REG_R8])
#define CONTEXT_R9(context)  (((ucontext_t *) context)->uc_mcontext.gregs[REG_R9])
#define CONTEXT_R10(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R10])
#define CONTEXT_R11(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R11])
#define CONTEXT_R12(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R12])
#define CONTEXT_R13(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R13])
#define CONTEXT_R14(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R14])
#define CONTEXT_R15(context) (((ucontext_t *) context)->uc_mcontext.gregs[REG_R15])

#endif

#define CONTEXT_PC(context) CONTEXT_RIP(context)

static int delete_trigger(blockinfo *bi, void *pc)
{
	while (bi) {
		if (bi->handler && (uae_u8*)bi->direct_handler <= pc &&
				(uae_u8*)bi->nexthandler > pc) {
#ifdef DEBUG_ACCESS
			write_log(_T("JIT: Deleted trigger (%p < %p < %p) %p\n"),
					bi->handler, pc, bi->nexthandler, bi->pc_p);
#endif
			invalidate_block(bi);
			raise_in_cl_list(bi);
			set_special(0);
			return 1;
		}
		bi = bi->next;
	}
	return 0;
}

/* Opcode register id list:
 *
 * 8 Bit:               16 Bit:               32 Bit:
 *
 *     0: AL                 0: AX                 0: EAX
 *     1: CL                 1: CX                 1: ECX
 *     2: DL                 2: DX                 2: EDX
 *     3: BL                 3: BX                 3: EBX
 *     4: AH (SPL, if REX)   4: SP                 4: ESP
 *     5: CH (BPL, if REX)   5: BP                 5: EBP
 *     6: DH (SIL, if REX)   6: SI                 6: ESI
 *     7: BH (DIL, if REX)   7: DI                 7: EDI
 *     8: R8L                8: R8W                8: R8D
 *     9: R9L                9: R9W                9: R9D
 *    10: R10L              10: R10W              10: R10D
 *    11: R11L              11: R11W              11: R11D
 *    12: R12L              12: R12W              12: R12D
 *    13: R13L              13: R13W              13: R13D
 *    14: R14L              14: R14W              14: R14D
 *    15: R15L              15: R15W              15: R15D
 */

static bool decode_instruction(
		uae_u8 *pc, int *r, int *dir, int *size, int *len, int *rex)
{
	*r = -1;
	*size = 4;
	*dir = -1;
	*len = 0;
	*rex = 0;

#ifdef CPU_x86_64
	/* Skip address-size override prefix. */
	if (*pc == 0x67) {
		pc += 1;
		*len += 1;
	}
#endif
	/* Operand size override prefix. */
	if (*pc == 0x66) {
		pc += 1;
		*size = 2;
		*len += 1;
	}
#ifdef CPU_x86_64
	/* Handle x86-64 REX prefix. */
	if ((pc[0] & 0xf0) == 0x40) {
		*rex = pc[0];
		pc += 1;
		*len += 1;
		if (*rex & (1 << 3)) {
			*size = 8;
			/* 64-bit operand size not supported. */
			return 0;
		}
	}
#endif

	switch (pc[0]) {
	case 0x8a: /* MOV r8, m8 */
		if ((pc[1] & 0xc0) == 0x80) {
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_READ;
			*size = 1;
			*len += 6;
			break;
		}
		break;
	case 0x88: /* MOV m8, r8 */
		if ((pc[1] & 0xc0) == 0x80) {
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_WRITE;
			*size = 1;
			*len += 6;
			break;
		}
		break;
	case 0x8b: /* MOV r32, m32 */
		switch (pc[1] & 0xc0) {
		case 0x80:
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_READ;
			*len += 6;
			break;
		case 0x40:
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_READ;
			*len += 3;
			break;
		case 0x00:
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_READ;
			*len += 2;
			break;
		default:
			break;
		}
		break;
	case 0x89: /* MOV m32, r32 */
		switch (pc[1] & 0xc0) {
		case 0x80:
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_WRITE;
			*len += 6;
			break;
		case 0x40:
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_WRITE;
			*len += 3;
			break;
		case 0x00:
			*r = (pc[1] >> 3) & 7;
			*dir = SIG_WRITE;
			*len += 2;
			break;
		}
		break;
	}
#ifdef CPU_x86_64
	/* SIB byte present when ModR/M rm field = 100 (binary).
	 * This occurs in JIT-generated memory accesses like
	 * MOV r32, [R15 + reg*1 + disp32] where R15 = natmem_offset.
	 * Note: does not handle mod=00, SIB.base=101 (disp32-only base case);
	 * safe because JIT only generates mod=0x80 patterns. */
	if (*r != -1 && (pc[1] & 0x07) == 0x04) {
		*len += 1;
	}
	if (*rex & (1 << 2)) {
		/* Use x86-64 extended registers R8..R15. */
		*r += 8;
	}
#endif
	return *r != -1;
}

#ifdef HAVE_CONTEXT_T

static void *get_pr_from_context(CONTEXT_T context, int r, int size, int rex)
{
	switch (r) {
	case 0:
		return &(CONTEXT_RAX(context));
	case 1:
		return &(CONTEXT_RCX(context));
	case 2:
		return &(CONTEXT_RDX(context));
	case 3:
		return &(CONTEXT_RBX(context));
	case 4:
		if (size > 1 || rex) {
			return NULL;
		} else {
			return (((uae_u8 *) &(CONTEXT_RAX(context))) + 1); /* AH */
		}
	case 5:
		if (size > 1 || rex) {
			return &(CONTEXT_RBP(context));
		} else {
			return (((uae_u8 *) &(CONTEXT_RCX(context))) + 1); /* CH */
		}
	case 6:
		if (size > 1 || rex) {
			return &(CONTEXT_RSI(context));
		} else {
			return (((uae_u8 *) &(CONTEXT_RDX(context))) + 1); /* DH */
		}
	case 7:
		if (size > 1 || rex) {
			return &(CONTEXT_RDI(context));
		} else {
			return (((uae_u8 *) &(CONTEXT_RBX(context))) + 1); /* BH */
		}
#ifdef CPU_x86_64
	case 8:
		return &(CONTEXT_R8(context));
	case 9:
		return &(CONTEXT_R9(context));
	case 10:
		return &(CONTEXT_R10(context));
	case 11:
		return &(CONTEXT_R11(context));
	case 12:
		return &(CONTEXT_R12(context));
	case 13:
		return &(CONTEXT_R13(context));
	case 14:
		return &(CONTEXT_R14(context));
	case 15:
		return &(CONTEXT_R15(context));
#endif
	default:
		abort ();
	}
}

static void log_unhandled_access(uae_u8 *fault_pc)
{
	write_log(_T("JIT: Can't handle access PC=%p!\n"), fault_pc);
	if (fault_pc) {
		write_log(_T("JIT: Instruction bytes"));
		for (int j = 0; j < 10; j++) {
			write_log(_T(" %02x"), fault_pc[j]);
		}
		write_log(_T("\n"));
	}
}

#ifdef WIN32

static int handle_access(uintptr_t fault_addr, CONTEXT_T context)
{
	uae_u8 *fault_pc = (uae_u8 *) CONTEXT_PC(context);
#ifdef CPU_64_BIT
	/* Compute Amiga address first, then range-check. The host fault address
	 * can exceed 32-bit when natmem_offset + amiga_addr > 4GB, but the
	 * Amiga address itself is always 32-bit. */
	uintptr_t amiga_addr_wide = fault_addr - (uintptr_t) NATMEM_OFFSET;
	if (amiga_addr_wide > (uintptr_t) 0xffffffff) {
		return 0;
	}
#endif

#ifdef DEBUG_ACCESS
	write_log(_T("JIT: Fault address is 0x%lx at PC=%p\n"), fault_addr, fault_pc);
#endif
	if (!canbang || !currprefs.cachesize)
		return 0;

	if (in_handler)
		write_log(_T("JIT: Argh --- Am already in a handler. Shouldn't happen!\n"));

	if (fault_pc < compiled_code || fault_pc > current_compile_p) {
		return 0;
	}

	int r = -1, size = 4, dir = -1, len = 0, rex = 0;
	decode_instruction(fault_pc, &r, &dir, &size, &len, &rex);
	if (r == -1) {
		log_unhandled_access(fault_pc);
		return 0;
	}

#ifdef DEBUG_ACCESS
	write_log (_T("JIT: Register was %d, direction was %d, size was %d\n"),
			   r, dir, size);
#endif

	void *pr = get_pr_from_context(context, r, size, rex);
	if (pr == NULL) {
		log_unhandled_access(fault_pc);
		return 0;
	}

	uae_u32 addr = (uae_u32)(fault_addr - (uintptr_t) NATMEM_OFFSET);
	addrbank *ab = &get_mem_bank(addr);
#ifdef DEBUG_ACCESS
	if (addr >= 0x80000000) {
		write_log (_T("JIT: Suspicious address 0x%x in SEGV handler.\n"), addr);
	}
	if (ab)
		write_log(_T("JIT: Address bank: %s, address %08x\n"), ab->name ? ab->name : _T("NONE"), addr);
#endif

	/* dummy_bank handler is NOT safe in exception context: it calls
	 * gary_nonrange() which can trigger hardware_exception2() -> THROW(2).
	 * For dummy_bank, return 0 for reads and silently ignore writes —
	 * matching committed natmem gap behavior. */
	if (ab == &dummy_bank) {
		if (dir == SIG_READ) {
			switch (size) {
			case 1: *((uae_u8*)pr) = 0; break;
			case 2: *((uae_u16*)pr) = 0; break;
			case 4: *((uae_u32*)pr) = 0; break;
			default: break;
			}
		}
		CONTEXT_PC(context) += len;
		if (delete_trigger(active, fault_pc))
			return 1;
		if (delete_trigger(dormant, fault_pc))
			return 1;
		set_special(0);
		return 1;
	}

	if (dir == SIG_READ) {
		switch (size) {
		case 1:
			*((uae_u8*)pr) = get_byte(addr);
			break;
		case 2:
			*((uae_u16*)pr) = do_byteswap_16(get_word(addr));
			break;
		case 4:
			*((uae_u32*)pr) = do_byteswap_32(get_long(addr));
			break;
		default:
			abort();
		}
	} else {
		switch (size) {
		case 1:
			put_byte(addr, *((uae_u8 *) pr));
			break;
		case 2:
			put_word(addr, do_byteswap_16(*((uae_u16 *) pr)));
			break;
		case 4:
			put_long(addr, do_byteswap_32(*((uae_u32 *) pr)));
			break;
		default: abort();
		}
	}
	CONTEXT_PC(context) += len;

	if (delete_trigger(active, fault_pc)) {
		return 1;
	}
	/* Not found in the active list. Might be a rom routine that
	 * is in the dormant list */
	if (delete_trigger(dormant, fault_pc)) {
		return 1;
	}
#ifdef DEBUG_ACCESS
	// Can happen if MOVEM causes multiple faults
	write_log (_T("JIT: Huh? Could not find trigger!\n"));
#endif
	set_special(0);
	return 1;
}

#else

/*
 * Try to handle faulted memory access in compiled code
 *
 * Returns 1 if handled, 0 otherwise
 */
static int handle_access(uintptr_t fault_addr, CONTEXT_T context)
{
	uae_u8 *fault_pc = (uae_u8 *) CONTEXT_PC(context);
#ifdef CPU_64_BIT
	/* Compute Amiga address first, then range-check. The host fault address
	 * can exceed 32-bit when natmem_offset + amiga_addr > 4GB, but the
	 * Amiga address itself is always 32-bit. Example: natmem at 0x80000000,
	 * Amiga addr 0x80000004 -> host 0x100000004 (>32-bit on host, valid Amiga). */
	uintptr_t amiga_addr_wide = fault_addr - (uintptr_t) NATMEM_OFFSET;
	if (amiga_addr_wide > (uintptr_t) 0xffffffff) {
		return 0;
	}
#endif

#ifdef DEBUG_ACCESS
	write_log(_T("JIT: Fault address is 0x%lx at PC=%p\n"), fault_addr, fault_pc);
#endif
	if (!canbang || !currprefs.cachesize)
		return 0;

	if (in_handler)
		write_log(_T("JIT: Argh --- Am already in a handler. Shouldn't happen!\n"));

	if (fault_pc < compiled_code || fault_pc > current_compile_p) {
		return 0;
	}

	int r = -1, size = 4, dir = -1, len = 0, rex = 0;
	decode_instruction(fault_pc, &r, &dir, &size, &len, &rex);
	if (r == -1) {
		log_unhandled_access(fault_pc);
		return 0;
	}

#ifdef DEBUG_ACCESS
	write_log (_T("JIT: Register was %d, direction was %d, size was %d\n"),
			   r, dir, size);
#endif

	void *pr = get_pr_from_context(context, r, size, rex);
	if (pr == NULL) {
		log_unhandled_access(fault_pc);
		return 0;
	}

	uae_u32 addr = (uae_u32)(fault_addr - (uintptr_t) NATMEM_OFFSET);
	addrbank *ab = &get_mem_bank(addr);
#ifdef DEBUG_ACCESS
	if (addr >= 0x80000000) {
			write_log (_T("JIT: Suspicious address 0x%x in SEGV handler.\n"), addr);
	}
	if (ab)
		write_log(_T("JIT: Address bank: %s, address %08x\n"), ab->name ? ab->name : _T("NONE"), addr);
#endif

	/* dummy_bank handler is NOT signal-safe: it calls gary_nonrange() which
	 * can trigger hardware_exception2() -> THROW(2) (longjmp from signal
	 * handler = undefined behavior). For dummy_bank, return 0 for reads
	 * and silently ignore writes — matching committed natmem gap behavior. */
	if (ab == &dummy_bank) {
		if (dir == SIG_READ) {
			switch (size) {
			case 1: *((uae_u8*)pr) = 0; break;
			case 2: *((uae_u16*)pr) = 0; break;
			case 4: *((uae_u32*)pr) = 0; break;
			default: break;
			}
		}
		CONTEXT_PC(context) += len;
		if (delete_trigger(active, fault_pc))
			return 1;
		if (delete_trigger(dormant, fault_pc))
			return 1;
		set_special(0);
		return 1;
	}

	uae_u8 *original_target = target;
	target = (uae_u8*) CONTEXT_PC(context);

#if X86_TARGET_64BIT
	uae_u8 vecbuf[16]; /* MOVQir(10) + JMPsr(3) = 13 bytes on x86-64 */
#else
	uae_u8 vecbuf[5];  /* JMPm = 5 bytes on 32-bit */
#endif
	for (int i = 0; i < sizeof(vecbuf); i++) {
		vecbuf[i] = target[i];
	}
	raw_jmp(uae_p32(veccode));

#ifdef DEBUG_ACCESS
	write_log(_T("JIT: Create jump to %p\n"), veccode);
#endif

	target = veccode;
	if (dir == SIG_READ) {
		switch (size) {
		case 1:
			raw_mov_b_ri(r, get_byte(addr));
			break;
		case 2:
			raw_mov_w_ri(r, uae_bswap_16(get_word(addr)));
			break;
		case 4:
			raw_mov_l_ri(r, uae_bswap_32(get_long(addr)));
			break;
		default:
			abort();
		}
	} else {
		switch (size) {
		case 1:
			put_byte(addr, *((uae_u8 *) pr));
			break;
		case 2:
			put_word(addr, uae_bswap_16(*((uae_u16 *) pr)));
			break;
		case 4:
			put_long(addr, uae_bswap_32(*((uae_u32 *) pr)));
			break;
		default: abort();
		}
	}
	for (int i = 0; i < sizeof(vecbuf); i++) {
		raw_mov_b_mi(JITPTR CONTEXT_PC(context) + i, vecbuf[i]);
	}
	raw_mov_l_mi(uae_p32(&in_handler), 0);
	raw_jmp(uae_p32(CONTEXT_PC(context)) + len);

	in_handler = 1;
	target = original_target;
	if (delete_trigger(active, fault_pc)) {
		return 1;
	}
	/* Not found in the active list. Might be a rom routine that
	 * is in the dormant list */
	if (delete_trigger(dormant, fault_pc)) {
		return 1;
	}
#ifdef DEBUG_ACCESS
	write_log (_T("JIT: Huh? Could not find trigger!\n"));
#endif
	set_special(0);
	return 1;
}
#endif

#endif /* CONTEXT_T */

#ifdef _WIN32

#if defined(CPU_x86_64)
static LONG WINAPI JITUnhandledExceptionFilter(PEXCEPTION_POINTERS pExInfo)
{
	PEXCEPTION_RECORD er = pExInfo->ExceptionRecord;
	PCONTEXT ctx = pExInfo->ContextRecord;

	write_log(_T("JIT: UNHANDLED EXCEPTION!\n"));
	write_log(_T("JIT:   Code: 0x%08X\n"), (unsigned int)er->ExceptionCode);
	write_log(_T("JIT:   Address: %p\n"), er->ExceptionAddress);
	write_log(_T("JIT:   RIP: 0x%llX\n"), (unsigned long long)ctx->Rip);
	write_log(_T("JIT:   RSP: 0x%llX\n"), (unsigned long long)ctx->Rsp);
	write_log(_T("JIT:   RAX: 0x%llX  RBX: 0x%llX\n"),
		(unsigned long long)ctx->Rax, (unsigned long long)ctx->Rbx);
	write_log(_T("JIT:   RCX: 0x%llX  RDX: 0x%llX\n"),
		(unsigned long long)ctx->Rcx, (unsigned long long)ctx->Rdx);
	write_log(_T("JIT:   RSI: 0x%llX  RDI: 0x%llX\n"),
		(unsigned long long)ctx->Rsi, (unsigned long long)ctx->Rdi);
	write_log(_T("JIT:   R8:  0x%llX  R9:  0x%llX\n"),
		(unsigned long long)ctx->R8, (unsigned long long)ctx->R9);
	write_log(_T("JIT:   R10: 0x%llX  R11: 0x%llX\n"),
		(unsigned long long)ctx->R10, (unsigned long long)ctx->R11);
	write_log(_T("JIT:   R12: 0x%llX  R13: 0x%llX\n"),
		(unsigned long long)ctx->R12, (unsigned long long)ctx->R13);
	write_log(_T("JIT:   R14: 0x%llX  R15: 0x%llX\n"),
		(unsigned long long)ctx->R14, (unsigned long long)ctx->R15);
	write_log(_T("JIT:   RBP: 0x%llX\n"), (unsigned long long)ctx->Rbp);

	if (er->ExceptionCode == STATUS_ACCESS_VIOLATION && er->NumberParameters >= 2) {
		write_log(_T("JIT:   AV type: %s  target: %p\n"),
			er->ExceptionInformation[0] ? _T("WRITE") : _T("READ"),
			(void*)er->ExceptionInformation[1]);
	}

	/* Check if RIP is in JIT code range */
	uintptr rip = (uintptr)ctx->Rip;
	if (compiled_code && rip >= (uintptr)compiled_code &&
		rip < (uintptr)compiled_code + currprefs.cachesize * 1024) {
		write_log(_T("JIT:   ** Crash is INSIDE JIT compiled code **\n"));
		/* Dump instruction bytes at crash site */
		uae_u8 *pc = (uae_u8 *)rip;
		write_log(_T("JIT:   Instruction bytes at RIP:"));
		for (int j = 0; j < 16; j++) {
			write_log(_T(" %02x"), pc[j]);
		}
		write_log(_T("\n"));
	} else if (popallspace && rip >= (uintptr)popallspace &&
			   rip < (uintptr)popallspace + POPALLSPACE_SIZE) {
		write_log(_T("JIT:   ** Crash is in popallspace trampolines **\n"));
	} else {
		write_log(_T("JIT:   ** Crash is in native C/C++ code **\n"));
	}

	write_log(_T("JIT:   M68K PC: 0x%08X\n"), (unsigned int)M68K_GETPC);
	write_log(_T("JIT:   regs.pc_p: %p\n"), regs.pc_p);
	write_log(_T("JIT:   natmem_offset: %p\n"), NATMEM_OFFSET);

	return EXCEPTION_CONTINUE_SEARCH;  /* Let Windows default handler take over */
}
#endif /* CPU_x86_64 */

LONG WINAPI EvalException(LPEXCEPTION_POINTERS info)
{
	DWORD code = info->ExceptionRecord->ExceptionCode;
	if (code != STATUS_ACCESS_VIOLATION || !canbang || currprefs.cachesize == 0)
		return EXCEPTION_CONTINUE_SEARCH;

	uintptr_t address = info->ExceptionRecord->ExceptionInformation[1];
	if (handle_access(address, info)) {
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	if (currprefs.comp_catchfault) {
		// setup fake exception
		exception2_setup(regs.opcode, uae_p32(address) - uae_p32(NATMEM_OFFSET), info->ExceptionRecord->ExceptionInformation[0] == 0, 1, regs.s ? 4 : 0);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static void *installed_vector_handler;

static LONG CALLBACK JITVectoredHandler(PEXCEPTION_POINTERS info)
{
//	write_log(_T("JitVectoredHandler\n"));
	return EvalException(info);
}

#elif defined(HAVE_CONTEXT_T)

static void sigsegv_handler(int signum, siginfo_t *info, void *context)
{
	uae_u8 *i = (uae_u8 *) CONTEXT_PC(context);
	uintptr_t address = (uintptr_t) info->si_addr;

	if (i >= compiled_code && i <= current_compile_p) {
		if (handle_access(address, context)) {
			return;
		}

		/* handle_access() failed — log diagnostics */
		write_log(_T("JIT: SIGSEGV at PC=%p, fault addr=%p\n"), i, (void *)address);
		if (i) {
			write_log(_T("JIT: Instruction bytes at PC:"));
			for (int j = 0; j < 16; j++) {
				write_log(_T(" %02x"), i[j]);
			}
			write_log(_T("\n"));
		}
		write_log(_T("JIT: compiled_code=%p, current_compile_p=%p\n"),
			compiled_code, current_compile_p);
		write_log(_T("JIT: M68K PC=%08x, natmem_offset=%p\n"),
			(unsigned int)M68K_GETPC, NATMEM_OFFSET);

		if (currprefs.comp_catchfault) {
			/* Determine read/write direction from Linux REG_ERR */
			bool is_read = true;
#if defined(__linux__) && defined(REG_ERR)
			{
				ucontext_t *uc = (ucontext_t *)context;
				/* Bit 1 of REG_ERR: 0=read, 1=write */
				is_read = (uc->uc_mcontext.gregs[REG_ERR] & 0x2) == 0;
			}
#endif
			uae_u32 amiga_addr = (uae_u32)(address - (uintptr_t) NATMEM_OFFSET);
			write_log(_T("JIT: SIGSEGV comp_catchfault: addr=%08x %s, redirecting to popall_do_nothing\n"),
				amiga_addr, is_read ? _T("read") : _T("write"));

			/* Store Amiga fault info — signal-safe (only writes globals) */
			exception2_setup(regs.opcode, amiga_addr, is_read, 1, regs.s ? 4 : 0);

			/* Signal m68k_run_jit() to generate Exception(2) after return */
			jit_exception_pending = 2;

			/* Redirect to popall_do_nothing which unwinds the JIT stack
			 * frame (inc SP + pop preserved regs + ret) and returns to
			 * m68k_run_jit()'s inner loop */
			CONTEXT_PC(context) = (uintptr_t)popall_do_nothing;
			return;
		}

		write_log(_T("JIT: No comp_catchfault — aborting\n"));
	} else {
		write_log(_T("JIT: Caught SIGSEGV outside JIT code: access to %p at PC=%p\n"),
			(void *)address, i);
	}

	exit(EXIT_FAILURE);
}

#endif

// #define TEST_EXCEPTION_HANDLER

#ifdef TEST_EXCEPTION_HANDLER
#include "test_exception_handler.cpp"
#endif

static void install_exception_handler(void)
{
#ifdef TEST_EXCEPTION_HANDLER
	test_exception_handler();
#endif

#ifdef JIT_EXCEPTION_HANDLER
	if (veccode == NULL) {
		/* veccode generates RIP-relative references to globals, so it
		 * must be within ±2GB of the .data segment. Use the anchor-based
		 * JIT allocator to place it near .data/JIT cache, then set RWX. */
		veccode = (uae_u8 *) jit_vm_acquire(256, 0);
		if (veccode) {
			uae_vm_protect(veccode, 256, UAE_VM_READ_WRITE_EXECUTE);
		} else {
			write_log("JIT: FATAL: veccode allocation failed — "
				"cannot install exception handler, disabling JIT\n");
			changed_prefs.cachesize = 0;
			currprefs.cachesize = 0;
			return;
		}
	}
#endif
#ifdef USE_STRUCTURED_EXCEPTION_HANDLING
	/* Structured exception handler is installed in main.cpp */
#elif defined(_WIN32)
#if 1
	write_log(_T("JIT: Installing vectored exception handler\n"));
	installed_vector_handler = AddVectoredExceptionHandler(
		0, JITVectoredHandler);
#if defined(CPU_x86_64)
	write_log(_T("JIT: Installing unhandled exception filter (crash diagnostics)\n"));
	SetUnhandledExceptionFilter(JITUnhandledExceptionFilter);
#endif
#else
	write_log(_T("JIT: Installing unhandled exception filter\n"));
	SetUnhandledExceptionFilter(EvalException);
#endif
#elif defined(HAVE_CONTEXT_T)
	write_log (_T("JIT: Installing segfault handler\n"));
	struct sigaction act;
	act.sa_sigaction = (void (*)(int, siginfo_t*, void*)) sigsegv_handler;
	sigemptyset (&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, NULL);
#if defined(MACOSX) || defined(__FreeBSD__)
	sigaction(SIGBUS, &act, NULL);
#endif
#else
	write_log (_T("JIT: No segfault handler installed\n"));
#endif
}

#endif /* NATMEM_OFFSET */
