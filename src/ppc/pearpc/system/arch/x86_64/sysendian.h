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

static inline FORCE_INLINE FUNCTION_CONST uint32 ppc_bswap_word(uint32 data)
{
#if 0
	asm (
		"bswapl %0": "=r" (data) : "0" (data)
	);
#endif
	data = (data >> 24) | ((data >> 8) & 0x0000ff00) | ((data << 8) & 0x00ff0000) | (data << 24);
	return data;
}

static inline FORCE_INLINE FUNCTION_CONST uint16 ppc_bswap_half(uint16 data) 
{
#if 0
	asm (
		"rolw $8, %0": "=r" (data): "0" (data)
	);
#endif
	data = (data >> 8) | (data << 8);
	return data;
}

static inline FORCE_INLINE FUNCTION_CONST uint64 ppc_bswap_dword(uint64 data)
{
#if 0
	asm (
		"bswapq %0": "=r" (data) : "0" (data)
	);
#endif
	data = (ppc_bswap_word(data) << 32) | (ppc_bswap_word(data >> 32));
	return data;
}

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

#endif
