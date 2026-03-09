//
// Created by midwan on 22/12/2023.
//

#ifndef AMIBERRY_COMPEMU_H
#define AMIBERRY_COMPEMU_H

#if defined(CPU_arm) || defined(CPU_AARCH64)
#include "arm/compemu_arm.h"
#elif defined(__x86_64__) || defined(_M_AMD64)
#include "x86/compemu_x86.h"
#endif

extern volatile int jit_exception_pending;
#if defined(CPU_AARCH64)
#include <setjmp.h>
extern jmp_buf jit_bus_error_jmpbuf;
extern volatile bool jit_in_compiled_code;
#endif

#endif //AMIBERRY_COMPEMU_H
