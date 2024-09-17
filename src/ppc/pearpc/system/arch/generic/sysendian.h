/*
 *	PearPC
 *	sysendian.h
 *
 *	Copyright (C) 1999-2004 Stefan Weyergraf
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SYSTEM_ARCH_SPECIFIC_SYSENDIAN_H__
#define __SYSTEM_ARCH_SPECIFIC_SYSENDIAN_H__

#include "system/types.h"
#include "pearpc_config.h"

static inline FUNCTION_CONST uint32 ppc_bswap_word(uint32 data)
{
	return (data>>24)|((data>>8)&0xff00)|((data<<8)&0xff0000)|(data<<24);
}

static inline FUNCTION_CONST uint64 ppc_bswap_dword(uint64 data)
{
	return (((uint64)ppc_bswap_word(data)) << 32) | (uint64)ppc_bswap_word(data >> 32);
}

static inline FUNCTION_CONST uint16 ppc_bswap_half(uint16 data)
{
	return (data<<8)|(data>>8);
}

#if HOST_ENDIANESS == HOST_ENDIANESS_LE

/*
 *		Little-endian machine
 */

#	define ppc_dword_from_BE(data)	(ppc_bswap_dword(data))
#	define ppc_word_from_BE(data)	(ppc_bswap_word(data))
#	define ppc_half_from_BE(data)	(ppc_bswap_half(data))

#	define ppc_dword_from_LE(data)	((uint64)(data))
#	define ppc_word_from_LE(data)	((uint32)(data))
#	define ppc_half_from_LE(data)	((uint16)(data))

#	define ppc_dword_to_LE(data)	ppc_dword_from_LE(data)
#	define ppc_word_to_LE(data)	ppc_word_from_LE(data)
#	define ppc_half_to_LE(data)	ppc_half_from_LE(data)

#	define ppc_dword_to_BE(data)	ppc_dword_from_BE(data)
#	define ppc_word_to_BE(data)	ppc_word_from_BE(data)
#	define ppc_half_to_BE(data)	ppc_half_from_BE(data)

#elif HOST_ENDIANESS == HOST_ENDIANESS_BE

/*
 *		Big-endian machine
 */
#	define ppc_dword_from_BE(data)	((uint64)(data))
#	define ppc_word_from_BE(data)	((uint32)(data))
#	define ppc_half_from_BE(data)	((uint16)(data))

#	define ppc_dword_from_LE(data)	(ppc_bswap_dword(data))
#	define ppc_word_from_LE(data)	(ppc_bswap_word(data))
#	define ppc_half_from_LE(data)	(ppc_bswap_half(data))

#	define ppc_dword_to_LE(data)	ppc_dword_from_LE(data)
#	define ppc_word_to_LE(data)	ppc_word_from_LE(data)
#	define ppc_half_to_LE(data)	ppc_half_from_LE(data)

#	define ppc_dword_to_BE(data)	ppc_dword_from_BE(data)
#	define ppc_word_to_BE(data)	ppc_word_from_BE(data)
#	define ppc_half_to_BE(data)	ppc_half_from_BE(data)

#else

/*
 *		Weird-endian machine
 *	HOST_ENDIANESS is neither little- nor big-endian?
 */
#	error "What kind of a weird machine do you have? It's neither little- nor big-endian??? This is unsupported."

#endif

#endif
