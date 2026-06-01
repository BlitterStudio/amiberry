//
// Created by midwan on 22/12/2023.
//

#if defined(CPU_arm) || defined(CPU_AARCH64) || defined(__arm__) || defined(_M_ARM) || \
	defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#include "arm/compstbl_arm.cpp"
#elif defined(__x86_64__) || defined(_M_AMD64)
#include "x86/compstbl_x86.cpp"
#endif
