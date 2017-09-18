/* fsusage.h -- declarations for filesystem space usage info
 * Copyright (C) 1991, 1992 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef UAE_FSUSAGE_H
#define UAE_FSUSAGE_H

#include "uae/types.h"

/* Space usage statistics for a filesystem.  Blocks are 512-byte. */
struct fs_usage
{
	uae_u64 total;
	uae_u64 avail;
};

int get_fs_usage (const TCHAR *path, const TCHAR *disk, struct fs_usage *fsp);

#endif /* UAE_FSUSAGE_H */
