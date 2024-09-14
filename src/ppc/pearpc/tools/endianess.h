/* 
 *	HT Editor
 *	endianess.h
 *
 *	Copyright (C) 1999-2002 Stefan Weyergraf
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

#ifndef __ENDIANESS_H__
#define __ENDIANESS_H__

#include "system/types.h"

// This is _NOT_ a bitmask!
#define STRUCT_ENDIAN_8		1
#define STRUCT_ENDIAN_16	2
#define STRUCT_ENDIAN_32	4
#define STRUCT_ENDIAN_64	8

// This is a bitmask.
#define STRUCT_ENDIAN_HOST	128

typedef enum {
	big_endian,
	little_endian
} Endianess;

#ifdef __cplusplus
extern "C" {
#endif

void createForeignInt(void *buf, int i, int size, Endianess to_endianess);
void createForeignInt64(void *buf, uint64 i, int size, Endianess to_endianess);
int createHostInt(const void *buf, int size, Endianess from_endianess);
uint64 createHostInt64(const void *buf, int size, Endianess from_endianess);
void createHostStructx(void *buf, uint bufsize, const uint8 *table, Endianess from_endianess);

#ifdef __cplusplus
}
#endif

#endif /* __ENDIANESS_H__ */
