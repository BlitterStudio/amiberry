/*
 *	PearPC
 *	ppc_alu.h
 *
 *	Copyright (C) 2003 Sebastian Biallas (sb@biallas.net)
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

#ifndef __PPC_ALU_H__
#define __PPC_ALU_H__

void ppc_opc_addx();
void ppc_opc_addcx();
void ppc_opc_addex();
void ppc_opc_addi();
void ppc_opc_addic();
void ppc_opc_addic_();
void ppc_opc_addis();
void ppc_opc_addmex();
void ppc_opc_addzex();
void ppc_opc_addco(); // TW

void ppc_opc_andx();
void ppc_opc_andcx();
void ppc_opc_andi_();
void ppc_opc_andis_();

void ppc_opc_cmp();
void ppc_opc_cmpi();
void ppc_opc_cmpl();
void ppc_opc_cmpli();

void ppc_opc_cntlzwx();

void ppc_opc_crand();
void ppc_opc_crandc();
void ppc_opc_creqv();
void ppc_opc_crnand();
void ppc_opc_crnor();
void ppc_opc_cror();
void ppc_opc_crorc();
void ppc_opc_crxor();

void ppc_opc_divwx();
void ppc_opc_divwux();
void ppc_opc_divwo(); // TW
void ppc_opc_divwuo(); // TW

void ppc_opc_eqvx();

void ppc_opc_extsbx();
void ppc_opc_extshx();

void ppc_opc_mulhwx();
void ppc_opc_mulhwux();
void ppc_opc_mulli();
void ppc_opc_mullwx();
void ppc_opc_mullwo(); // TW

void ppc_opc_nandx();

void ppc_opc_negx();
void ppc_opc_norx();

void ppc_opc_orx();
void ppc_opc_orcx();
void ppc_opc_ori();
void ppc_opc_oris();

void ppc_opc_rlwimix();
void ppc_opc_rlwinmx();
void ppc_opc_rlwnmx();

void ppc_opc_slwx();
void ppc_opc_srawx();
void ppc_opc_srawix();
void ppc_opc_srwx();

void ppc_opc_subfx();
void ppc_opc_subfcx();
void ppc_opc_subfex();
void ppc_opc_subfic();
void ppc_opc_subfmex();
void ppc_opc_subfzex();
void ppc_opc_subfco(); // TW

void ppc_opc_xorx();
void ppc_opc_xori();
void ppc_opc_xoris();

#endif
 
