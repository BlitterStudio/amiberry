/*
 *	PearPC
 *	ppc_dec.h
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

#ifndef __PPC_DEC_H__
#define __PPC_DEC_H__

#include "system/types.h"

void FASTCALL ppc_exec_opc();
void ppc_dec_init();

typedef void (*ppc_opc_function)();

#define PPC_OPC_ASSERT(v)

#define PPC_OPC_MAIN(opc)		(((opc)>>26)&0x3f)
#define PPC_OPC_EXT(opc)		(((opc)>>1)&0x3ff)
#define PPC_OPC_Rc			1
#define PPC_OPC_OE			(1<<10)
#define PPC_OPC_LK			1
#define PPC_OPC_AA			(1<<1)

#define PPC_OPC_TEMPL_A(opc, rD, rA, rB, rC) {rD=((opc)>>21)&0x1f;rA=((opc)>>16)&0x1f;rB=((opc)>>11)&0x1f;rC=((opc)>>6)&0x1f;}
#define PPC_OPC_TEMPL_B(opc, BO, BI, BD) {BO=((opc)>>21)&0x1f;BI=((opc)>>16)&0x1f;BD=(opc)&0xfffc;if (BD&0x8000) BD |= 0xffff0000;}
#define PPC_OPC_TEMPL_D_SImm(opc, rD, rA, imm) {rD=((opc)>>21)&0x1f;rA=((opc)>>16)&0x1f;imm=(opc)&0xffff;if (imm & 0x8000) imm |= 0xffff0000;}
#define PPC_OPC_TEMPL_D_UImm(opc, rD, rA, imm) {rD=((opc)>>21)&0x1f;rA=((opc)>>16)&0x1f;imm=(opc)&0xffff;}
#define PPC_OPC_TEMPL_D_Shift16(opc, rD, rA, imm) {rD=((opc)>>21)&0x1f;rA=((opc)>>16)&0x1f;imm=(opc)<<16;}
#define PPC_OPC_TEMPL_I(opc, LI) {LI=(opc)&0x3fffffc;if (LI&0x02000000) LI |= 0xfc000000;}
#define PPC_OPC_TEMPL_M(opc, rS, rA, SH, MB, ME) {rS=((opc)>>21)&0x1f;rA=((opc)>>16)&0x1f;SH=((opc)>>11)&0x1f;MB=((opc)>>6)&0x1f;ME=((opc)>>1)&0x1f;}
#define PPC_OPC_TEMPL_X(opc, rS, rA, rB) {rS=((opc)>>21)&0x1f;rA=((opc)>>16)&0x1f;rB=((opc)>>11)&0x1f;}
#define PPC_OPC_TEMPL_XFX(opc, rS, CRM) {rS=((opc)>>21)&0x1f;CRM=((opc)>>12)&0xff;}
#define PPC_OPC_TEMPL_XO(opc, rS, rA, rB) {rS=((opc)>>21)&0x1f;rA=((opc)>>16)&0x1f;rB=((opc)>>11)&0x1f;}
#define PPC_OPC_TEMPL_XL(opc, BO, BI, BD) {BO=((opc)>>21)&0x1f;BI=((opc)>>16)&0x1f;BD=((opc)>>11)&0x1f;}
#define PPC_OPC_TEMPL_XFL(opc, rB, FM) {rB=((opc)>>11)&0x1f;FM=((opc)>>17)&0xff;}

#endif
