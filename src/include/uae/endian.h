#ifndef UAE_ENDIAN_H
#define UAE_ENDIAN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "uae/types.h"

#ifdef _WIN32

/* Use custom conversion routines for Windows */
#ifdef WORDS_BIGENDIAN
#error big-endian windows not supported here
#endif

#ifndef be16toh
static inline uint16_t be16toh_impl(uint16_t v)
{
	return (v << 8) | (v >> 8);
}
#define be16toh be16toh_impl
#endif

#ifndef le16toh
static inline uint16_t le16toh_impl(uint16_t v)
{
	return v;
}
#define le16toh le16toh_impl
#endif

#ifndef le32toh
static inline uint32_t le32toh_impl(uint32_t v)
{
	return v;
}
#define le32toh le32toh_impl
#endif

#elif defined(HAVE_LIBKERN_OSBYTEORDER_H)

/* OS X lacks endian.h, but has something similar */
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#elif defined(HAVE_ENDIAN_H)

/* Linux has endian.h */
#include <endian.h>

#elif defined(HAVE_SYS_ENDIAN_H)

/* BSD's generally have sys/endian.h */
#include <sys/endian.h>

#endif

#endif /* UAE_ENDIAN_H */
