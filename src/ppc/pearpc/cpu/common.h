/*
 *	PearPC
 *	common.h
 *
 *	Copyright (C) 2003-2006 Sebastian Biallas (sb@biallas.net)
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

#ifndef __CPU_COMMON_H__
#define __CPU_COMMON_H__

#include <stddef.h>
#include "system/types.h"

typedef union Vector_t {
        uint64 d[2];
        sint64 sd[2];
        float f[4];
        uint32 w[4];
        sint32 sw[4];
        uint16 h[8];
        sint16 sh[8];
        uint8 b[16];
        sint8 sb[16];
} Vector_t;

/*
cr: .67
 0- 3 cr0
 4- 7 cr1
 8-11 cr2
12-15 cr3
16-19 cr4
20-23 cr5
24-27 cr6
28-31 cr7
*/

#define CR_CR0(v) ((v)>>28)
#define CR_CR1(v) (((v)>>24)&0xf)
#define CR_CRx(v, x) (((v)>>(4*(7-(x))))&0xf)

/*
cr0 bits: .68
lt
gt
eq
so
*/

#define CR_CR0_LT (1<<31)
#define CR_CR0_GT (1<<30)
#define CR_CR0_EQ (1<<29)
#define CR_CR0_SO (1<<28)

/*
cr1 bits: .68
4 Floating-point exception (FX)
5 Floating-point enabled exception (FEX)
6 Floating-point invalid exception (VX)
7 Floating-point overflow exception (OX)
*/

#define CR_CR1_FX (1<<27)
#define CR_CR1_FEX (1<<26)
#define CR_CR1_VX (1<<25)
#define CR_CR1_OX (1<<24)

/*
FPSCR bits: .70

*/
 
#define FPSCR_FX (1<<31)
#define FPSCR_FEX (1<<30)
#define FPSCR_VX (1<<29)
#define FPSCR_OX (1<<28)
#define FPSCR_UX (1<<27)
#define FPSCR_ZX (1<<26)
#define FPSCR_XX (1<<25)
#define FPSCR_VXSNAN (1<<24)
#define FPSCR_VXISI (1<<23)
#define FPSCR_VXIDI (1<<22)
#define FPSCR_VXZDZ (1<<21)
#define FPSCR_VXIMZ (1<<20)
#define FPSCR_VXVC (1<<19)
#define FPSCR_FR (1<<18)
#define FPSCR_FI (1<<17)

#define FPSCR_FPRF(v) (((v)>>12)&0x1f)

#define FPSCR_res0 (1<<11)
#define FPSCR_VXSOFT (1<<10)
#define FPSCR_VXSQRT (1<<9)
#define FPSCR_VXCVI (1<<8)
#define FPSCR_VXVE (1<<7)
#define FPSCR_VXOE (1<<6)
#define FPSCR_VXUE (1<<5)
#define FPSCR_VXZE (1<<4)
#define FPSCR_VXXE (1<<3)
#define FPSCR_VXNI (1<<2)
#define FPSCR_RN(v) ((v)&3)

#define FPSCR_RN_NEAR 0
#define FPSCR_RN_ZERO 1
#define FPSCR_RN_PINF 2
#define FPSCR_RN_MINF 3 

/*
VSCR bits:
        sat = summary saturation
        nj = non-java floating-point mode
*/
#define VSCR_SAT 1
#define VSCR_NJ (1<<16)

/*
xer bits:
0 so
1 ov
2 carry
3-24 res
25-31 number of bytes for lswx/stswx
*/

#define XER_SO (1<<31)
#define XER_OV (1<<30)
#define XER_CA (1<<29)
#define XER_n(v) ((v)&0x7f)

/*
msr: .83
0-12 res
13   POW	power management enabled
14	res
15	ILE	exception little-endian mode
16	EE	enable external interrupt
17	PR	privilege level (0=sv)
18	FP	floating point avail
19	ME	maschine check exception enable
20	FE0	floation point exception mode 0
21	SE   single step enable
22	BE	branch trace enable
23	FE1	floation point exception mode 1
24	res
25   IP	exception prefix
26   IR   intruction address translation
27   DR   data address translation
28-29res
30	RI	recoverable exception
31	LE   little endian mode

*/

#define MSR_UNKNOWN	(1<<30)
#define MSR_UNKNOWN2	(1<<27)
#define MSR_VEC		(1<<25)
#define MSR_KEY		(1<<19)		// 603e
#define MSR_POW		(1<<18)
#define MSR_TGPR	(1<<15)		// 603(e)
#define MSR_ILE		(1<<16)
#define MSR_EE		(1<<15)
#define MSR_PR		(1<<14)
#define MSR_FP		(1<<13)
#define MSR_ME		(1<<12)
#define MSR_FE0		(1<<11)
#define MSR_SE		(1<<10)
#define MSR_BE		(1<<9)
#define MSR_FE1		(1<<8)
#define MSR_IP		(1<<6)
#define MSR_IR		(1<<5)
#define MSR_DR		(1<<4)
#define MSR_PM		(1<<2)
#define MSR_RI		(1<<1)
#define MSR_LE		(1<<0)

//#define PPC_CPU_UNSUPPORTED_MSR_BITS (/*MSR_POW|*/MSR_ILE|MSR_BE|MSR_IP|MSR_LE)
#define PPC_CPU_UNSUPPORTED_MSR_BITS (~(MSR_POW | MSR_UNKNOWN | MSR_UNKNOWN2 | MSR_VEC | MSR_EE | MSR_PR | MSR_FP | MSR_ME | MSR_FE0 | MSR_SE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI | MSR_IP))

#define MSR_RFI_SAVE_MASK (0x87c0ff73) // was ff73

/*
BAT Register: .88
upper:
0-14  BEPI Block effective page index.
15-18 res
19-29 BL   Block length.
30    Vs   Supervisor mode valid bit.
31    Vp   User mode valid bit.
lower:
0-14  BRPN This field is used in conjunction with the BL field to generate highorder bits of the physical address of the block.
15-24 res
25-28 WIMG Memory/cache access mode bits
29    res
30-31 PP   Protection bits for block.

BAT Area
Length		BL Encoding
128 Kbytes	000 0000 0000
256 Kbytes	000 0000 0001
512 Kbytes	000 0000 0011
1 Mbyte		000 0000 0111
2 Mbytes	000 0000 1111
4 Mbytes	000 0001 1111
8 Mbytes	000 0011 1111
16 Mbytes	000 0111 1111
32 Mbytes	000 1111 1111
64 Mbytes	001 1111 1111
128 Mbytes	011 1111 1111
256 Mbytes	111 1111 1111
*/

#define BATU_BEPI(v) ((v)&0xfffe0000)
#define BATU_BL(v)   (((v)&0x1ffc)>>2)
#define BATU_Vs      (1<<1)
#define BATU_Vp      (1)
#define BATL_BRPN(v) ((v)&0xfffe0000)

#define BAT_EA_OFFSET(v) ((v)&0x1ffff)
#define BAT_EA_11(v)     ((v)&0x0ffe0000)
#define BAT_EA_4(v)      ((v)&0xf0000000)

/*
sdr1: .91
0-15 The high-order 16 bits of the 32-bit physical address of the page table
16-22 res
23-31 Mask for page table address
*/

#define SDR1_HTABORG(v) (((v)>>16)&0xffff)
#define SDR1_HTABMASK(v) ((v)&0x1ff)
#define SDR1_PAGETABLE_BASE(v) ((v)&0xffff)

/*
sr: .94
0    T=0:
1    Ks   sv prot
2    Kp   user prot
3    N    No execute
4-7  res
8-31 VSID Virtual Segment ID

0     T=1:
1     Ks
2     Kp
3-11  BUID       Bus Unit ID
12-31 CNTRL_SPEC
 */
#define SR_T			(1<<31)
#define SR_Ks			(1<<30)
#define SR_Kp			(1<<29)
#define SR_N			(1<<28)
#define SR_VSID(v)		((v)&0xffffff)
#define SR_BUID(v)		(((v)>>20)&0x1ff)
#define SR_CNTRL_SPEC(v)	((v)&0xfffff)

#define EA_SR(v)		(((v)>>28)&0xf)
#define EA_PageIndex(v)		(((v)>>12)&0xffff)
#define EA_Offset(v)		((v)&0xfff)
#define EA_API(v)		(((v)>>22)&0x3f)

#define PA_RPN(v)		(((v)>>12)&0xfffff)
#define PA_Offset(v)		((v)&0xfff)

/*
PTE: .364
0     V
1-24  VSID
25    H
26-31 API
*/

#define PTE1_V       (1<<31)
#define PTE1_VSID(v) (((v)>>7)&0xffffff)
#define PTE1_H       (1<<6)
#define PTE1_API(v)  ((v)&0x3f)

#define PTE2_RPN(v)  ((v)&0xfffff000)
#define PTE2_R       (1<<8)
#define PTE2_C       (1<<7)
#define PTE2_WIMG(v) (((v)>>3)&0xf)
#define PTE2_PP(v)   ((v)&3)

#define PPC_L1_CACHE_LINE_SIZE		32
#define PPC_LG_L1_CACHE_LINE_SIZE	5
#define PPC_MAX_L1_COPY_PREFETCH	4

/*
 *	special registers
 */
#define HID0	1008	/* Checkstop and misc enables */
#define HID1	1009	/* Clock configuration */
#define IABR	1010	/* Instruction address breakpoint register */
#define ICTRL	1011	/* Instruction Cache Control */
#define LDSTDB	1012	/* Load/Store Debug */
#define DABR	1013	/* Data address breakpoint register */
#define MSSCR0	1014	/* Memory subsystem control */
#define MSSCR1	1015	/* Memory subsystem debug */
#define MSSSR0	1015	/* Memory Subsystem Status */
#define LDSTCR	1016	/* Load/Store Status/Control */
#define L2CR2	1016	/* L2 Cache control 2 */
#define L2CR	1017	/* L2 Cache control */
#define L3CR	1018	/* L3 Cache control */
#define ICTC	1019	/* I-cache throttling control */
#define THRM1	1020	/* Thermal management 1 */
#define THRM2	1021	/* Thermal management 2 */
#define THRM3	1022	/* Thermal management 3 */
#define PIR	1023	/* Processor ID Register */

//;	hid0 bits
#define HID0_emcp	0
#define HID0_emcpm	0x80000000
#define HID0_dbp	1
#define HID0_dbpm	0x40000000
#define HID0_eba	2
#define HID0_ebam	0x20000000
#define HID0_ebd	3
#define HID0_ebdm	0x10000000
#define HID0_sbclk	4
#define HID0_sbclkm	0x08000000
#define HID0_eclk	6
#define HID0_eclkm	0x02000000
#define HID0_par	7
#define HID0_parm	0x01000000
#define HID0_sten	7
#define HID0_stenm	0x01000000
#define HID0_doze	8
#define HID0_dozem	0x00800000
#define HID0_nap	9
#define HID0_napm	0x00400000
#define HID0_sleep	10
#define HID0_sleepm	0x00200000
#define HID0_dpm	11
#define HID0_dpmm	0x00100000
#define HID0_riseg	12
#define HID0_risegm	0x00080000
#define HID0_eiec	13
#define HID0_eiecm	0x00040000
#define HID0_mum	14
#define HID0_mumm	0x00020000
#define HID0_nhr	15
#define HID0_nhrm	0x00010000
#define HID0_ice	16
#define HID0_icem	0x00008000
#define HID0_dce	17
#define HID0_dcem	0x00004000
#define HID0_ilock	18
#define HID0_ilockm	0x00002000
#define HID0_dlock	19
#define HID0_dlockm	0x00001000
#define HID0_icfi	20
#define HID0_icfim	0x00000800
#define HID0_dcfi	21
#define HID0_dcfim	0x00000400
#define HID0_spd	22
#define HID0_spdm	0x00000200
#define HID0_sge	24
#define HID0_sgem	0x00000080
#define HID0_dcfa	25
#define HID0_dcfam	0x00000040
#define HID0_btic	26
#define HID0_bticm	0x00000020
#define HID0_lrstk	27
#define HID0_lrstkm	0x00000010
#define HID0_abe	28
#define HID0_abem	0x00000008
#define HID0_fold	28
#define HID0_foldm	0x00000008
#define HID0_bht	29
#define HID0_bhtm	0x00000004
#define HID0_nopdst	30
#define HID0_nopdstm	0x00000002
#define HID0_nopti	31
#define HID0_noptim	0x00000001

#endif
 
