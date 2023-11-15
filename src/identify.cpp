/*
* UAE - The Un*x Amiga Emulator
*
* Routines for labelling amiga internals.
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef DEBUGGER

#include "options.h"
#include "memory.h"
#include "identify.h"

const struct mem_labels int_labels[] =
{
	{ _T("Reset:SSP"),  0x0000 },
	{ _T("EXECBASE"),   0x0004 },
	{ _T("BUS ERROR"),  0x0008 },
	{ _T("ADR ERROR"),  0x000C },
	{ _T("ILLEG OPC"),  0x0010 },
	{ _T("DIV BY 0"),   0x0014 },
	{ _T("CHK"),        0x0018 },
	{ _T("TRAPV"),      0x001C },
	{ _T("PRIVIL VIO"), 0x0020 },
	{ _T("TRACE"),      0x0024 },
	{ _T("LINEA EMU"),  0x0028 },
	{ _T("LINEF EMU"),  0x002C },
	{ _T("FORMAT ERR"), 0x0038 },
	{ _T("INT Uninit"), 0x003C },
	{ _T("INT Unjust"), 0x0060 },
	{ _T("Lvl 1 Int"),  0x0064 },
	{ _T("Lvl 2 Int"),  0x0068 },
	{ _T("Lvl 3 Int"),  0x006C },
	{ _T("Lvl 4 Int"),  0x0070 },
	{ _T("Lvl 5 Int"),  0x0074 },
	{ _T("Lvl 6 Int"),  0x0078 },
	{ _T("NMI"),        0x007C },
	{ 0, 0 }
};

const struct mem_labels trap_labels[] =
{
	{ _T("TRAP 00"),    0x0080 },
	{ _T("TRAP 01"),    0x0084 },
	{ _T("TRAP 02"),    0x0088 },
	{ _T("TRAP 03"),    0x008C },
	{ _T("TRAP 04"),    0x0090 },
	{ _T("TRAP 05"),    0x0094 },
	{ _T("TRAP 06"),    0x0098 },
	{ _T("TRAP 07"),    0x009C },
	{ _T("TRAP 08"),    0x00A0 },
	{ _T("TRAP 09"),    0x00A4 },
	{ _T("TRAP 10"),    0x00A8 },
	{ _T("TRAP 11"),    0x00AC },
	{ _T("TRAP 12"),    0x00B0 },
	{ _T("TRAP 13"),    0x00B4 },
	{ _T("TRAP 14"),    0x00B8 },
	{ _T("TRAP 15"),    0x00BC },
	{ 0, 0 }
};

const struct mem_labels extraexp_labels[] =
{
	{ _T("FP BSUN"),	0x00c0 },
	{ _T("FP INEXACT"), 0x00c4 },
	{ _T("FP DIV BY 0"),0x00c8 },
	{ _T("FP UNDERF"),	0x00cc },
	{ _T("FP OPERR"),	0x00d0 },
	{ _T("FP OVERF"),	0x00d4 },
	{ _T("FP SNAN"),	0x00d8 },
	{ _T("FP UNIMP DT"),0x00dc },
	{ _T("MMU CNFERR"),	0x00e0 },
	{ _T("UNIMP EA"),   0x00f0 },
	{ _T("UNIMP INT"),	0x00f4 },
	{ NULL, 0 }
};

const struct mem_labels mem_labels[] =
{
	{ _T("CIAB PRA"),   0xBFD000 },
	{ _T("CIAB PRB"),   0xBFD100 },
	{ _T("CIAB DDRA"),  0xBFD200 },
	{ _T("CIAB DDRB"),  0xBFD300 },
	{ _T("CIAB TALO"),  0xBFD400 },
	{ _T("CIAB TAHI"),  0xBFD500 },
	{ _T("CIAB TBLO"),  0xBFD600 },
	{ _T("CIAB TBHI"),  0xBFD700 },
	{ _T("CIAB TDLO"),  0xBFD800 },
	{ _T("CIAB TDMD"),  0xBFD900 },
	{ _T("CIAB TDHI"),  0xBFDA00 },
	{ _T("CIAB SDR"),   0xBFDC00 },
	{ _T("CIAB ICR"),   0xBFDD00 },
	{ _T("CIAB CRA"),   0xBFDE00 },
	{ _T("CIAB CRB"),   0xBFDF00 },
	{ _T("CIAA PRA"),   0xBFE001 },
	{ _T("CIAA PRB"),   0xBFE101 },
	{ _T("CIAA DDRA"),  0xBFE201 },
	{ _T("CIAA DDRB"),  0xBFE301 },
	{ _T("CIAA TALO"),  0xBFE401 },
	{ _T("CIAA TAHI"),  0xBFE501 },
	{ _T("CIAA TBLO"),  0xBFE601 },
	{ _T("CIAA TBHI"),  0xBFE701 },
	{ _T("CIAA TDLO"),  0xBFE801 },
	{ _T("CIAA TDMD"),  0xBFE901 },
	{ _T("CIAA TDHI"),  0xBFEA01 },
	{ _T("CIAA SDR"),   0xBFEC01 },
	{ _T("CIAA ICR"),   0xBFED01 },
	{ _T("CIAA CRA"),   0xBFEE01 },
	{ _T("CIAA CRB"),   0xBFEF01 },
	{ _T("CLK S1"),     0xDC0000 },
	{ _T("CLK S10"),    0xDC0004 },
	{ _T("CLK MI1"),    0xDC0008 },
	{ _T("CLK MI10"),   0xDC000C },
	{ _T("CLK H1"),     0xDC0010 },
	{ _T("CLK H10"),    0xDC0014 },
	{ _T("CLK D1"),     0xDC0018 },
	{ _T("CLK D10"),    0xDC001C },
	{ _T("CLK MO1"),    0xDC0020 },
	{ _T("CLK MO10"),   0xDC0024 },
	{ _T("CLK Y1"),     0xDC0028 },
	{ _T("CLK Y10"),    0xDC002E },
	{ _T("CLK WEEK"),   0xDC0030 },
	{ _T("CLK CD"),     0xDC0034 },
	{ _T("CLK CE"),     0xDC0038 },
	{ _T("CLK CF"),     0xDC003C },
	{ NULL, 0 }
};

/* This table was generated from the list of AGA chip names in
* AGA.guide available on aminet. It could well have errors in it. */

const struct customData custd[] =
{
	{ _T("BLTDDAT"),  0xdff000, CD_NONE }, /* Blitter dest. early read (dummy address) */
	{ _T("DMACONR"),  0xdff002, 0 }, /* Dma control (and blitter status) read */
	{ _T("VPOSR"),    0xdff004, 0 }, /* Read vert most sig. bits (and frame flop */
	{ _T("VHPOSR"),   0xdff006, 0 }, /* Read vert and horiz position of beam */
	{ _T("DSKDATR"),  0xdff008, CD_NONE }, /* Disk data early read (dummy address) */
	{ _T("JOY0DAT"),  0xdff00A, 0 }, /* Joystick-mouse 0 data (vert,horiz) */
	{ _T("JOY1DAT"),  0xdff00C, 0 }, /* Joystick-mouse 1 data (vert,horiz) */
	{ _T("CLXDAT"),   0xdff00E, 0 }, /* Collision data reg. (read and clear) */
	{ _T("ADKCONR"),  0xdff010, 0 }, /* Audio,disk control register read */
	{ _T("POT0DAT"),  0xdff012, 0 }, /* Pot counter pair 0 data (vert,horiz) */
	{ _T("POT1DAT"),  0xdff014, 0 }, /* Pot counter pair 1 data (vert,horiz) */
	{ _T("POTGOR"),   0xdff016, 0 }, /* Pot pin data read */
	{ _T("SERDATR"),  0xdff018, 0 }, /* Serial port data and status read */
	{ _T("DSKBYTR"),  0xdff01A, 0 }, /* Disk data byte and status read */
	{ _T("INTENAR"),  0xdff01C, 0 }, /* Interrupt enable bits read */
	{ _T("INTREQR"),  0xdff01E, 0 }, /* Interrupt request bits read */
	{ _T("DSKPTH"),   0xdff020, CD_WO | CD_DMA_PTR }, /* Disk pointer (high 5 bits) */
	{ _T("DSKPTL"),   0xdff022, CD_WO | CD_DMA_PTR }, /* Disk pointer (low 15 bits) */
	{ _T("DSKLEN"),   0xdff024, CD_WO }, /* Disk lentgh */
	{ _T("DSKDAT"),   0xdff026, CD_NONE }, /* Disk DMA data write */
	{ _T("REFPTR"),   0xdff028, CD_NONE }, /* Refresh pointer */
	{ _T("VPOSW"),    0xdff02A, CD_WO }, /* Write vert most sig. bits(and frame flop) */
	{ _T("VHPOSW"),   0xdff02C, CD_WO }, /* Write vert and horiz pos of beam */
	{ _T("COPCON"),   0xdff02e, CD_WO }, /* Coprocessor control reg (CDANG) */
	{ _T("SERDAT"),   0xdff030, CD_WO }, /* Serial port data and stop bits write */
	{ _T("SERPER"),   0xdff032, CD_WO }, /* Serial port period and control */
	{ _T("POTGO"),    0xdff034, CD_WO }, /* Pot count start,pot pin drive enable data */
	{ _T("JOYTEST"),  0xdff036, CD_WO }, /* Write to all 4 joystick-mouse counters at once */
	{ _T("STREQU"),   0xdff038, CD_WO }, /* Strobe for horiz sync with VB and EQU */
	{ _T("STRVBL"),   0xdff03A, CD_WO }, /* Strobe for horiz sync with VB (vert blank) */
	{ _T("STRHOR"),   0xdff03C, CD_WO }, /* Strobe for horiz sync */
	{ _T("STRLONG"),  0xdff03E, CD_WO }, /* Strobe for identification of long horiz line */
	{ _T("BLTCON0"),  0xdff040, CD_WO }, /* Blitter control reg 0 */
	{ _T("BLTCON1"),  0xdff042, CD_WO }, /* Blitter control reg 1 */
	{ _T("BLTAFWM"),  0xdff044, CD_WO }, /* Blitter first word mask for source A */
	{ _T("BLTALWM"),  0xdff046, CD_WO }, /* Blitter last word mask for source A */
	{ _T("BLTCPTH"),  0xdff048, CD_WO | CD_DMA_PTR }, /* Blitter pointer to source C (high 5 bits) */
	{ _T("BLTCPTL"),  0xdff04A, CD_WO | CD_DMA_PTR }, /* Blitter pointer to source C (low 15 bits) */
	{ _T("BLTBPTH"),  0xdff04C, CD_WO | CD_DMA_PTR }, /* Blitter pointer to source B (high 5 bits) */
	{ _T("BLTBPTL"),  0xdff04E, CD_WO | CD_DMA_PTR }, /* Blitter pointer to source B (low 15 bits) */
	{ _T("BLTAPTH"),  0xdff050, CD_WO | CD_DMA_PTR }, /* Blitter pointer to source A (high 5 bits) */
	{ _T("BLTAPTL"),  0xdff052, CD_WO | CD_DMA_PTR }, /* Blitter pointer to source A (low 15 bits) */
	{ _T("BLTDPTH"),  0xdff054, CD_WO | CD_DMA_PTR }, /* Blitter pointer to destn  D (high 5 bits) */
	{ _T("BLTDPTL"),  0xdff056, CD_WO | CD_DMA_PTR }, /* Blitter pointer to destn  D (low 15 bits) */
	{ _T("BLTSIZE"),  0xdff058, CD_WO }, /* Blitter start and size (win/width,height) */
	{ _T("BLTCON0L"), 0xdff05A, CD_WO | CD_ECS_AGNUS }, /* Blitter control 0 lower 8 bits (minterms) */
	{ _T("BLTSIZV"), 0xdff05C, CD_WO | CD_ECS_AGNUS, { 0, 0x07ff, 0x07ff } }, /* Blitter V size (for 15 bit vert size) */
	{ _T("BLTSIZH"), 0xdff05E, CD_WO | CD_ECS_AGNUS, { 0, 0x7fff, 0x7fff } }, /* Blitter H size & start (for 11 bit H size) */
	{ _T("BLTCMOD"),  0xdff060, CD_WO }, /* Blitter modulo for source C */
	{ _T("BLTBMOD"),  0xdff062, CD_WO }, /* Blitter modulo for source B */
	{ _T("BLTAMOD"),  0xdff064, CD_WO }, /* Blitter modulo for source A */
	{ _T("BLTDMOD"),  0xdff066, CD_WO }, /* Blitter modulo for destn  D */
	{ _T("-"),  0xdff068, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff06a, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff06c, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff06e, CD_NONE }, /* Unknown or Unused */
	{ _T("BLTCDAT"),  0xdff070, CD_WO }, /* Blitter source C data reg */
	{ _T("BLTBDAT"),  0xdff072, CD_WO }, /* Blitter source B data reg */
	{ _T("BLTADAT"),  0xdff074, CD_WO }, /* Blitter source A data reg */
	{ _T("-"),  0xdff076, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff078, CD_NONE }, /* Ext logic UHRES sprite pointer and data identifier */
	{ _T("-"),  0xdff07A, CD_NONE }, /* Ext logic UHRES bit plane identifier */
	{ _T("LISAID"),   0xdff07C, CD_ECS_DENISE }, /* Chip revision level for Denise/Lisa */
	{ _T("DSKSYNC"),  0xdff07E, CD_WO }, /* Disk sync pattern reg for disk read */
	{ _T("COP1LCH"),  0xdff080, CD_WO | CD_DMA_PTR }, /* Coprocessor first location reg (high 5 bits) */
	{ _T("COP1LCL"),  0xdff082, CD_WO | CD_DMA_PTR }, /* Coprocessor first location reg (low 15 bits) */
	{ _T("COP2LCH"),  0xdff084, CD_WO | CD_DMA_PTR }, /* Coprocessor second reg (high 5 bits) */
	{ _T("COP2LCL"),  0xdff086, CD_WO | CD_DMA_PTR }, /* Coprocessor second reg (low 15 bits) */
	{ _T("COPJMP1"),  0xdff088, CD_WO }, /* Coprocessor restart at first location */
	{ _T("COPJMP2"),  0xdff08A, CD_WO }, /* Coprocessor restart at second location */
	{ _T("COPINS"),   0xdff08C }, /* Coprocessor inst fetch identify */
	{ _T("DIWSTRT"),  0xdff08E, CD_WO }, /* Display window start (upper left vert-hor pos) */
	{ _T("DIWSTOP"),  0xdff090, CD_WO }, /* Display window stop (lower right vert-hor pos) */
	{ _T("DDFSTRT"),  0xdff092, CD_WO, { 0x00fc, 0x00fe, 0x00fe } }, /* Display bit plane data fetch start.hor pos */
	{ _T("DDFSTOP"),  0xdff094, CD_WO, { 0x00fc, 0x00fe, 0x00fe }  }, /* Display bit plane data fetch stop.hor pos */
	{ _T("DMACON"),   0xdff096, CD_WO, { 0x87ff, 0x87ff, 0x87ff } }, /* DMA control write (clear or set) */
	{ _T("CLXCON"),   0xdff098, CD_WO }, /* Collision control */
	{ _T("INTENA"),   0xdff09A, CD_WO }, /* Interrupt enable bits (clear or set bits) */
	{ _T("INTREQ"),   0xdff09C, CD_WO }, /* Interrupt request bits (clear or set bits) */
	{ _T("ADKCON"),   0xdff09E, CD_WO }, /* Audio,disk,UART,control */
	{ _T("AUD0LCH"),  0xdff0A0, CD_WO | CD_DMA_PTR }, /* Audio channel 0 location (high 5 bits) */
	{ _T("AUD0LCL"),  0xdff0A2, CD_WO | CD_DMA_PTR }, /* Audio channel 0 location (low 15 bits) */
	{ _T("AUD0LEN"),  0xdff0A4, CD_WO }, /* Audio channel 0 lentgh */
	{ _T("AUD0PER"),  0xdff0A6, CD_WO }, /* Audio channel 0 period */
	{ _T("AUD0VOL"),  0xdff0A8, CD_WO }, /* Audio channel 0 volume */
	{ _T("AUD0DAT"),  0xdff0AA, CD_WO }, /* Audio channel 0 data */
	{ _T("-"),  0xdff0AC, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff0AE, CD_NONE }, /* Unknown or Unused */
	{ _T("AUD1LCH"),  0xdff0B0, CD_WO | CD_DMA_PTR }, /* Audio channel 1 location (high 5 bits) */
	{ _T("AUD1LCL"),  0xdff0B2, CD_WO | CD_DMA_PTR }, /* Audio channel 1 location (low 15 bits) */
	{ _T("AUD1LEN"),  0xdff0B4, CD_WO }, /* Audio channel 1 lentgh */
	{ _T("AUD1PER"),  0xdff0B6, CD_WO }, /* Audio channel 1 period */
	{ _T("AUD1VOL"),  0xdff0B8, CD_WO }, /* Audio channel 1 volume */
	{ _T("AUD1DAT"),  0xdff0BA, CD_WO }, /* Audio channel 1 data */
	{ _T("-"),  0xdff0BC, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff0BE, CD_NONE }, /* Unknown or Unused */
	{ _T("AUD2LCH"),  0xdff0C0, CD_WO | CD_DMA_PTR }, /* Audio channel 2 location (high 5 bits) */
	{ _T("AUD2LCL"),  0xdff0C2, CD_WO | CD_DMA_PTR }, /* Audio channel 2 location (low 15 bits) */
	{ _T("AUD2LEN"),  0xdff0C4, CD_WO }, /* Audio channel 2 lentgh */
	{ _T("AUD2PER"),  0xdff0C6, CD_WO }, /* Audio channel 2 period */
	{ _T("AUD2VOL"),  0xdff0C8, CD_WO }, /* Audio channel 2 volume */
	{ _T("AUD2DAT"),  0xdff0CA, CD_WO }, /* Audio channel 2 data */
	{ _T("-"),  0xdff0CC, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff0CE, CD_NONE }, /* Unknown or Unused */
	{ _T("AUD3LCH"),  0xdff0D0, CD_WO | CD_DMA_PTR }, /* Audio channel 3 location (high 5 bits) */
	{ _T("AUD3LCL"),  0xdff0D2, CD_WO | CD_DMA_PTR }, /* Audio channel 3 location (low 15 bits) */
	{ _T("AUD3LEN"),  0xdff0D4, CD_WO }, /* Audio channel 3 lentgh */
	{ _T("AUD3PER"),  0xdff0D6, CD_WO }, /* Audio channel 3 period */
	{ _T("AUD3VOL"),  0xdff0D8, CD_WO }, /* Audio channel 3 volume */
	{ _T("AUD3DAT"),  0xdff0DA, CD_WO }, /* Audio channel 3 data */
	{ _T("-"),  0xdff0DC, CD_NONE }, /* Unknown or Unused */
	{ _T("-"),  0xdff0DE, CD_NONE }, /* Unknown or Unused */
	{ _T("BPL1PTH"),  0xdff0E0, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 1 (high 5 bits) */
	{ _T("BPL1PTL"),  0xdff0E2, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 1 (low 15 bits) */
	{ _T("BPL2PTH"),  0xdff0E4, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 2 (high 5 bits) */
	{ _T("BPL2PTL"),  0xdff0E6, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 2 (low 15 bits) */
	{ _T("BPL3PTH"),  0xdff0E8, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 3 (high 5 bits) */
	{ _T("BPL3PTL"),  0xdff0EA, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 3 (low 15 bits) */
	{ _T("BPL4PTH"),  0xdff0EC, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 4 (high 5 bits) */
	{ _T("BPL4PTL"),  0xdff0EE, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 4 (low 15 bits) */
	{ _T("BPL5PTH"),  0xdff0F0, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 5 (high 5 bits) */
	{ _T("BPL5PTL"),  0xdff0F2, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 5 (low 15 bits) */
	{ _T("BPL6PTH"),  0xdff0F4, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 6 (high 5 bits) */
	{ _T("BPL6PTL"),  0xdff0F6, CD_WO | CD_DMA_PTR }, /* Bit plane pointer 6 (low 15 bits) */
	{ _T("BPL7PTH"),  0xdff0F8, CD_WO | CD_AGA | CD_DMA_PTR }, /* Bit plane pointer 7 (high 5 bits) */
	{ _T("BPL7PTL"),  0xdff0FA, CD_WO | CD_AGA | CD_DMA_PTR }, /* Bit plane pointer 7 (low 15 bits) */
	{ _T("BPL8PTH"),  0xdff0FC, CD_WO | CD_AGA | CD_DMA_PTR }, /* Bit plane pointer 8 (high 5 bits) */
	{ _T("BPL8PTL"),  0xdff0FE, CD_WO | CD_AGA | CD_DMA_PTR }, /* Bit plane pointer 8 (low 15 bits) */
	{ _T("BPLCON0"),  0xdff100, CD_WO, { (uae_u16)~0x00f1,  (uae_u16)~0x00b0,  (uae_u16)~0x0080}}, /* Bit plane control reg (misc control bits) */
	{ _T("BPLCON1"),  0xdff102, CD_WO }, /* Bit plane control reg (scroll val PF1,PF2) */
	{ _T("BPLCON2"),  0xdff104, CD_WO, { 0x007f, 0x01ff, 0x7fff } }, /* Bit plane control reg (priority control) */
	{ _T("BPLCON3"),  0xdff106, CD_WO | CD_ECS_DENISE, { 0x003f, 0x003f, 0xffff } }, /* Bit plane control reg (enhanced features) */
	{ _T("BPL1MOD"),  0xdff108, CD_WO }, /* Bit plane modulo (odd planes,or active- fetch lines if bitplane scan-doubling is enabled */
	{ _T("BPL2MOD"),  0xdff10A, CD_WO }, /* Bit plane modulo (even planes or inactive- fetch lines if bitplane scan-doubling is enabled */
	{ _T("BPLCON4"),  0xdff10C, CD_WO | CD_AGA }, /* Bit plane control reg (bitplane and sprite masks) */
	{ _T("CLXCON2"),  0xdff10e, CD_WO | CD_AGA }, /* Extended collision control reg */
	{ _T("BPL1DAT"),  0xdff110, CD_WO }, /* Bit plane 1 data (parallel to serial con- vert) */
	{ _T("BPL2DAT"),  0xdff112, CD_WO }, /* Bit plane 2 data (parallel to serial con- vert) */
	{ _T("BPL3DAT"),  0xdff114, CD_WO }, /* Bit plane 3 data (parallel to serial con- vert) */
	{ _T("BPL4DAT"),  0xdff116, CD_WO }, /* Bit plane 4 data (parallel to serial con- vert) */
	{ _T("BPL5DAT"),  0xdff118, CD_WO }, /* Bit plane 5 data (parallel to serial con- vert) */
	{ _T("BPL6DAT"),  0xdff11a, CD_WO }, /* Bit plane 6 data (parallel to serial con- vert) */
	{ _T("BPL7DAT"),  0xdff11c, CD_WO | CD_AGA }, /* Bit plane 7 data (parallel to serial con- vert) */
	{ _T("BPL8DAT"),  0xdff11e, CD_WO | CD_AGA }, /* Bit plane 8 data (parallel to serial con- vert) */
	{ _T("SPR0PTH"),  0xdff120, CD_WO | CD_DMA_PTR }, /* Sprite 0 pointer (high 5 bits) */
	{ _T("SPR0PTL"),  0xdff122, CD_WO | CD_DMA_PTR }, /* Sprite 0 pointer (low 15 bits) */
	{ _T("SPR1PTH"),  0xdff124, CD_WO | CD_DMA_PTR }, /* Sprite 1 pointer (high 5 bits) */
	{ _T("SPR1PTL"),  0xdff126, CD_WO | CD_DMA_PTR }, /* Sprite 1 pointer (low 15 bits) */
	{ _T("SPR2PTH"),  0xdff128, CD_WO | CD_DMA_PTR }, /* Sprite 2 pointer (high 5 bits) */
	{ _T("SPR2PTL"),  0xdff12A, CD_WO | CD_DMA_PTR }, /* Sprite 2 pointer (low 15 bits) */
	{ _T("SPR3PTH"),  0xdff12C, CD_WO | CD_DMA_PTR }, /* Sprite 3 pointer (high 5 bits) */
	{ _T("SPR3PTL"),  0xdff12E, CD_WO | CD_DMA_PTR }, /* Sprite 3 pointer (low 15 bits) */
	{ _T("SPR4PTH"),  0xdff130, CD_WO | CD_DMA_PTR }, /* Sprite 4 pointer (high 5 bits) */
	{ _T("SPR4PTL"),  0xdff132, CD_WO | CD_DMA_PTR }, /* Sprite 4 pointer (low 15 bits) */
	{ _T("SPR5PTH"),  0xdff134, CD_WO | CD_DMA_PTR }, /* Sprite 5 pointer (high 5 bits) */
	{ _T("SPR5PTL"),  0xdff136, CD_WO | CD_DMA_PTR }, /* Sprite 5 pointer (low 15 bits) */
	{ _T("SPR6PTH"),  0xdff138, CD_WO | CD_DMA_PTR }, /* Sprite 6 pointer (high 5 bits) */
	{ _T("SPR6PTL"),  0xdff13A, CD_WO | CD_DMA_PTR }, /* Sprite 6 pointer (low 15 bits) */
	{ _T("SPR7PTH"),  0xdff13C, CD_WO | CD_DMA_PTR }, /* Sprite 7 pointer (high 5 bits) */
	{ _T("SPR7PTL"),  0xdff13E, CD_WO | CD_DMA_PTR }, /* Sprite 7 pointer (low 15 bits) */
	{ _T("SPR0POS"),  0xdff140, CD_WO }, /* Sprite 0 vert-horiz start pos data */
	{ _T("SPR0CTL"),  0xdff142, CD_WO }, /* Sprite 0 position and control data */
	{ _T("SPR0DATA"), 0xdff144, CD_WO }, /* Sprite 0 image data register A */
	{ _T("SPR0DATB"), 0xdff146, CD_WO }, /* Sprite 0 image data register B */
	{ _T("SPR1POS"),  0xdff148, CD_WO }, /* Sprite 1 vert-horiz start pos data */
	{ _T("SPR1CTL"),  0xdff14A, CD_WO }, /* Sprite 1 position and control data */
	{ _T("SPR1DATA"), 0xdff14C, CD_WO }, /* Sprite 1 image data register A */
	{ _T("SPR1DATB"), 0xdff14E, CD_WO }, /* Sprite 1 image data register B */
	{ _T("SPR2POS"),  0xdff150, CD_WO }, /* Sprite 2 vert-horiz start pos data */
	{ _T("SPR2CTL"),  0xdff152, CD_WO }, /* Sprite 2 position and control data */
	{ _T("SPR2DATA"), 0xdff154, CD_WO }, /* Sprite 2 image data register A */
	{ _T("SPR2DATB"), 0xdff156, CD_WO }, /* Sprite 2 image data register B */
	{ _T("SPR3POS"),  0xdff158, CD_WO }, /* Sprite 3 vert-horiz start pos data */
	{ _T("SPR3CTL"),  0xdff15A, CD_WO }, /* Sprite 3 position and control data */
	{ _T("SPR3DATA"), 0xdff15C, CD_WO }, /* Sprite 3 image data register A */
	{ _T("SPR3DATB"), 0xdff15E, CD_WO }, /* Sprite 3 image data register B */
	{ _T("SPR4POS"),  0xdff160, CD_WO }, /* Sprite 4 vert-horiz start pos data */
	{ _T("SPR4CTL"),  0xdff162, CD_WO }, /* Sprite 4 position and control data */
	{ _T("SPR4DATA"), 0xdff164, CD_WO }, /* Sprite 4 image data register A */
	{ _T("SPR4DATB"), 0xdff166, CD_WO }, /* Sprite 4 image data register B */
	{ _T("SPR5POS"),  0xdff168, CD_WO }, /* Sprite 5 vert-horiz start pos data */
	{ _T("SPR5CTL"),  0xdff16A, CD_WO }, /* Sprite 5 position and control data */
	{ _T("SPR5DATA"), 0xdff16C, CD_WO }, /* Sprite 5 image data register A */
	{ _T("SPR5DATB"), 0xdff16E, CD_WO }, /* Sprite 5 image data register B */
	{ _T("SPR6POS"),  0xdff170, CD_WO }, /* Sprite 6 vert-horiz start pos data */
	{ _T("SPR6CTL"),  0xdff172, CD_WO }, /* Sprite 6 position and control data */
	{ _T("SPR6DATA"), 0xdff174, CD_WO }, /* Sprite 6 image data register A */
	{ _T("SPR6DATB"), 0xdff176, CD_WO }, /* Sprite 6 image data register B */
	{ _T("SPR7POS"),  0xdff178, CD_WO }, /* Sprite 7 vert-horiz start pos data */
	{ _T("SPR7CTL"),  0xdff17A, CD_WO }, /* Sprite 7 position and control data */
	{ _T("SPR7DATA"), 0xdff17C, CD_WO }, /* Sprite 7 image data register A */
	{ _T("SPR7DATB"), 0xdff17E, CD_WO }, /* Sprite 7 image data register B */
	{ _T("COLOR00"),  0xdff180, CD_WO | CD_COLOR }, /* Color table 00 */
	{ _T("COLOR01"),  0xdff182, CD_WO | CD_COLOR }, /* Color table 01 */
	{ _T("COLOR02"),  0xdff184, CD_WO | CD_COLOR }, /* Color table 02 */
	{ _T("COLOR03"),  0xdff186, CD_WO | CD_COLOR }, /* Color table 03 */
	{ _T("COLOR04"),  0xdff188, CD_WO | CD_COLOR }, /* Color table 04 */
	{ _T("COLOR05"),  0xdff18A, CD_WO | CD_COLOR }, /* Color table 05 */
	{ _T("COLOR06"),  0xdff18C, CD_WO | CD_COLOR }, /* Color table 06 */
	{ _T("COLOR07"),  0xdff18E, CD_WO | CD_COLOR }, /* Color table 07 */
	{ _T("COLOR08"),  0xdff190, CD_WO | CD_COLOR }, /* Color table 08 */
	{ _T("COLOR09"),  0xdff192, CD_WO | CD_COLOR }, /* Color table 09 */
	{ _T("COLOR10"),  0xdff194, CD_WO | CD_COLOR }, /* Color table 10 */
	{ _T("COLOR11"),  0xdff196, CD_WO | CD_COLOR }, /* Color table 11 */
	{ _T("COLOR12"),  0xdff198, CD_WO | CD_COLOR }, /* Color table 12 */
	{ _T("COLOR13"),  0xdff19A, CD_WO | CD_COLOR }, /* Color table 13 */
	{ _T("COLOR14"),  0xdff19C, CD_WO | CD_COLOR }, /* Color table 14 */
	{ _T("COLOR15"),  0xdff19E, CD_WO | CD_COLOR }, /* Color table 15 */
	{ _T("COLOR16"),  0xdff1A0, CD_WO | CD_COLOR }, /* Color table 16 */
	{ _T("COLOR17"),  0xdff1A2, CD_WO | CD_COLOR }, /* Color table 17 */
	{ _T("COLOR18"),  0xdff1A4, CD_WO | CD_COLOR }, /* Color table 18 */
	{ _T("COLOR19"),  0xdff1A6, CD_WO | CD_COLOR }, /* Color table 19 */
	{ _T("COLOR20"),  0xdff1A8, CD_WO | CD_COLOR }, /* Color table 20 */
	{ _T("COLOR21"),  0xdff1AA, CD_WO | CD_COLOR }, /* Color table 21 */
	{ _T("COLOR22"),  0xdff1AC, CD_WO | CD_COLOR }, /* Color table 22 */
	{ _T("COLOR23"),  0xdff1AE, CD_WO | CD_COLOR }, /* Color table 23 */
	{ _T("COLOR24"),  0xdff1B0, CD_WO | CD_COLOR }, /* Color table 24 */
	{ _T("COLOR25"),  0xdff1B2, CD_WO | CD_COLOR }, /* Color table 25 */
	{ _T("COLOR26"),  0xdff1B4, CD_WO | CD_COLOR }, /* Color table 26 */
	{ _T("COLOR27"),  0xdff1B6, CD_WO | CD_COLOR }, /* Color table 27 */
	{ _T("COLOR28"),  0xdff1B8, CD_WO | CD_COLOR }, /* Color table 28 */
	{ _T("COLOR29"),  0xdff1BA, CD_WO | CD_COLOR }, /* Color table 29 */
	{ _T("COLOR30"),  0xdff1BC, CD_WO | CD_COLOR }, /* Color table 30 */
	{ _T("COLOR31"),  0xdff1BE, CD_WO | CD_COLOR }, /* Color table 31 */
	{ _T("HTOTAL"),   0xdff1C0, CD_WO | CD_ECS_AGNUS }, /* Highest number count in horiz line (VARBEAMEN = 1) */
	{ _T("HSSTOP"),   0xdff1C2, CD_WO | CD_ECS_DENISE }, /* Horiz line pos for HSYNC stop */
	{ _T("HBSTRT"),   0xdff1C4, CD_WO | CD_ECS_DENISE }, /* Horiz line pos for HBLANK start */
	{ _T("HBSTOP"),   0xdff1C6, CD_WO | CD_ECS_DENISE }, /* Horiz line pos for HBLANK stop */
	{ _T("VTOTAL"),   0xdff1C8, CD_WO | CD_ECS_AGNUS }, /* Highest numbered vertical line (VARBEAMEN = 1) */
	{ _T("VSSTOP"),   0xdff1CA, CD_WO | CD_ECS_AGNUS }, /* Vert line for VBLANK start */
	{ _T("VBSTRT"),   0xdff1CC, CD_WO | CD_ECS_AGNUS }, /* Vert line for VBLANK start */
	{ _T("VBSTOP"),   0xdff1CE, CD_WO | CD_ECS_AGNUS }, /* Vert line for VBLANK stop */
	{ _T("SPRHSTRT"), 0xdff1D0, CD_WO | CD_ECS_AGNUS }, /* UHRES sprite vertical start */
	{ _T("SPRHSTOP"), 0xdff1D2, CD_WO | CD_ECS_AGNUS }, /* UHRES sprite vertical stop */
	{ _T("BPLHSTRT"), 0xdff1D4, CD_WO | CD_ECS_AGNUS }, /* UHRES bit plane vertical stop */
	{ _T("BPLHSTOP"), 0xdff1D6, CD_WO | CD_ECS_AGNUS }, /* UHRES bit plane vertical stop */
	{ _T("HHPOSW"),	  0xdff1D8, CD_WO | CD_ECS_AGNUS }, /* DUAL mode hires H beam counter write */
	{ _T("HHPOSR"),   0xdff1DA,     0 | CD_ECS_AGNUS }, /* DUAL mode hires H beam counter read */
	{ _T("BEAMCON0"), 0xdff1DC, CD_WO | CD_ECS_AGNUS }, /* Beam counter control register (SHRES,UHRES,PAL) */
	{ _T("HSSTRT"),   0xdff1DE, CD_WO | CD_ECS_DENISE }, /* Horizontal sync start (VARHSY) */
	{ _T("VSSTRT"),   0xdff1E0, CD_WO | CD_ECS_DENISE }, /* Vertical sync start (VARVSY) */
	{ _T("HCENTER"),  0xdff1E2, CD_WO | CD_ECS_DENISE }, /* Horizontal pos for vsync on interlace */
	{ _T("DIWHIGH"),  0xdff1E4, CD_WO | CD_ECS_AGNUS | CD_ECS_DENISE }, /* Display window upper bits for start/stop */
	{ _T("-"), 0xdff1E6, CD_NONE }, /* UHRES bit plane modulo */
	{ _T("-"), 0xdff1E8, CD_NONE }, /* UHRES sprite pointer (high 5 bits) */
	{ _T("-"), 0xdff1EA, CD_NONE }, /* UHRES sprite pointer (low 15 bits) */
	{ _T("-"), 0xdff1EC, CD_NONE }, /* VRam (UHRES) bitplane pointer (hi 5 bits) */
	{ _T("-"), 0xdff1EE, CD_NONE }, /* VRam (UHRES) bitplane pointer (lo 15 bits) */
	{ _T("-"), 0xdff1F0, CD_NONE }, /* Reserved (forever i guess!) */
	{ _T("-"), 0xdff1F2, CD_NONE }, /* Reserved (forever i guess!) */
	{ _T("-"), 0xdff1F4, CD_NONE }, /* Reserved (forever i guess!) */
	{ _T("-"), 0xdff1F6, CD_NONE }, /* Reserved (forever i guess!) */
	{ _T("-"), 0xdff1F8, CD_NONE }, /* Reserved (forever i guess!) */
	{ _T("-"), 0xdff1Fa, CD_NONE }, /* Reserved (forever i guess!) */
	{ _T("FMODE"),    0xdff1FC, CD_WO | CD_AGA }, /* Fetch mode register */
	{ _T("NULL"), 0xdff1FE, CD_WO },   /*   Can also indicate last 2 or 3 refresh
									cycles or the restart of the COPPER after lockup.*/
	{ NULL }
};

#endif
