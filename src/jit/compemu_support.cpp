//
// Created by midwan on 22/12/2023.
//

#if defined(CPU_arm) || defined(CPU_AARCH64)
#include "arm/compemu_support_arm.cpp"
#elif defined(__x86_64__) || defined(_M_AMD64)
#include "x86/compemu_support_x86.cpp"
#endif