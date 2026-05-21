//
// Created by midwan on 22/12/2023.
//

#ifndef AMIBERRY_COMPTBL_H
#define AMIBERRY_COMPTBL_H

#if defined(CPU_arm) || defined(CPU_AARCH64) || defined(__arm__) || defined(_M_ARM) || \
	defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#include "arm/comptbl_arm.h"
#elif defined(__x86_64__) || defined(_M_AMD64)
#include "x86/comptbl_x86.h"
#endif

#endif //AMIBERRY_COMPTBL_H
