/*
 *  Copyright (C) 2002-2004  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef UAE_EPSONPRINTER_H
#define UAE_EPSONPRINTER_H

#define Bit16u uae_u16
#define Bit16s uae_s16
#define Bit8u uae_u8
#define Real64 float
#define Bitu uae_u32
#define Bits uae_s32
#define Bit32u uae_u32

#ifndef WINFONT
#include "ft2build.h"
#include FT_FREETYPE_H
#endif

#if defined (WIN32)
#include <windows.h>
#include <winspool.h>
#endif

#define STYLE_PROP 0x01
#define STYLE_CONDENSED 0x02
#define STYLE_BOLD 0x04
#define STYLE_DOUBLESTRIKE 0x08
#define STYLE_DOUBLEWIDTH 0x10
#define STYLE_ITALICS 0x20
#define STYLE_UNDERLINE 0x40
#define STYLE_SUPERSCRIPT 0x80
#define STYLE_SUBSCRIPT 0x100
#define STYLE_STRIKETHROUGH 0x200
#define STYLE_OVERSCORE 0x400
#define STYLE_DOUBLEWIDTHONELINE 0x800
#define STYLE_DOUBLEHEIGHT 0x1000

#define SCORE_NONE 0x00
#define SCORE_SINGLE 0x01
#define SCORE_DOUBLE 0x02
#define SCORE_SINGLEBROKEN 0x05
#define SCORE_DOUBLEBROKEN 0x06

#define QUALITY_DRAFT 0x01
#define QUALITY_LQ 0x02

#define JUST_LEFT 0
#define JUST_CENTER 1
#define JUST_RIGHT 2
#define JUST_FULL 3

enum Typeface
{
	roman = 0,
	sansserif,
	courier,
	prestige,
	script,
	ocrb,
	ocra,
	orator,
	orators,
	scriptc,
	romant,
	sansserifh,
	svbusaba = 30,
	svjittra = 31
};

#endif /* UAE_EPSONPRINTER_H */
