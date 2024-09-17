/*
 *	PearPC
 *	debug.h
 *
 *	Copyright (C) 2003, 2004 Sebastian Biallas (sb@biallas.net)
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

/*
 *	Debugger Interface
 */
void ppc_set_singlestep_v(bool v, const char *file, int line, const char *infoformat, ...);
void ppc_set_singlestep_nonverbose(bool v);

#define SINGLESTEP(info)	ppc_set_singlestep_v(true, __FILE__, __LINE__, info)

#endif
