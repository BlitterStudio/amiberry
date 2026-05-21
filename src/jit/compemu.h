//
// Created by midwan on 22/12/2023.
//

#ifndef AMIBERRY_COMPEMU_H
#define AMIBERRY_COMPEMU_H

#if defined(CPU_arm) || defined(CPU_AARCH64) || defined(__arm__) || defined(_M_ARM) || \
	defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#include "arm/compemu_arm.h"
#elif defined(__x86_64__) || defined(_M_AMD64)
#include "x86/compemu_x86.h"
#endif

extern volatile int jit_exception_pending;
#if defined(CPU_AARCH64) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC) || \
	defined(CPU_x86_64) || defined(__x86_64__) || defined(_M_AMD64)
#define JIT_HAS_BUS_ERROR_RECOVERY 1
#if defined(_WIN32) && (defined(CPU_AARCH64) || defined(__aarch64__) || defined(_M_ARM64) || \
	defined(_M_ARM64EC)) && defined(D)
/* mingw-w64 ARM64 setjmp.h declares a member named D in _JUMP_BUFFER.
 * UAE debug headers define D as an empty marker macro, which corrupts that
 * system declaration unless the macro is hidden around the include. */
#define AMIBERRY_RESTORE_D_AFTER_SETJMP
#pragma push_macro("D")
#undef D
#endif
#include <setjmp.h>
#ifdef AMIBERRY_RESTORE_D_AFTER_SETJMP
#pragma pop_macro("D")
#undef AMIBERRY_RESTORE_D_AFTER_SETJMP
#endif
extern jmp_buf jit_bus_error_jmpbuf;
extern volatile bool jit_in_compiled_code;
#endif

#endif //AMIBERRY_COMPEMU_H
