/* 
 *	HT Editor
 *	debug.h
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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "system/types.h"

void NORETURN ht_assert_failed(const char *file, int line, const char *assertion);

#define ASSERT(a) if (!(a)) ht_assert_failed(__FILE__, __LINE__, (#a));
#define HERE __FILE__, __LINE__

void debugDumpMem(void *buf, int len);

#endif /* !__DEBUG_H__ */
