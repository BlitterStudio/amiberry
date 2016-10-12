 /*
  * NO Debugger
  */

#ifndef DEBUG_H
#define DEBUG_H

#if defined(DEBUG) && DEBUG
#define DUNUSED(x)
#else
#define DUNUSED(x)	((void)x)
#endif
#ifndef UNUSED
#define UNUSED(x)	((void)x)
#endif

/*
 *  debug.h - Debugging utilities
 *
 *  Basilisk II (C) 1997-2000 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define bug printf
#define panicbug printf

#if defined(DEBUG) && DEBUG
#define D(x) (x);
#else
#define D(x) ;
#endif

#if defined(DEBUG) && DEBUG > 1
#define D2(x) (x);
#else
#define D2(x) ;
#endif

#define infoprint bug
#endif
