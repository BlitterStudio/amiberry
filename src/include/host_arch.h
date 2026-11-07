#ifndef AMIBERRY_HOST_ARCH_H
#define AMIBERRY_HOST_ARCH_H

#ifdef __cplusplus
static_assert(sizeof(void *) == 8, "Amiberry requires a 64-bit target ABI");
#else
_Static_assert(sizeof(void *) == 8, "Amiberry requires a 64-bit target ABI");
#endif

#endif
