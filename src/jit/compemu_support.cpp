//
// Created by midwan on 22/12/2023.
//

#if defined(CPU_arm) || defined(CPU_AARCH64)
#include "arm/compemu_support_arm.cpp"
#elif defined(__x86_64__) || defined(_M_AMD64)
#include "x86/compemu_support_x86.cpp"
#endif
// Simple stub for missing emu8k function
#define MAXSOUNDBUFLEN 8192
typedef struct emu8k_t emu8k_t; // Forward declaration
void emu8k_update(emu8k_t* emu8k) {
    // Stub implementation
}

// Define prop array for non-JIT builds
#ifndef JIT
#include "compemu.h"
op_properties prop[65536] = {{0}};
#endif
