/*
* UAE - The Un*x Amiga Emulator
*
* Cirrus Logic based graphics board emulation
* PCem/86box glue interface + boards.
*
* Copyright 2013-2024 Toni Wilen
*
*/

#define VRAMLOG 0
#define MEMLOGR 0
#define MEMLOGW 0
#define MEMLOGINDIRECT 0
#define REGDEBUG 0
#define MEMDEBUG 0
#define MEMDEBUGMASK 0x7fffff
#define MEMDEBUGTEST 0x3fc000
#define MEMDEBUGCLEAR 0
#define SPCDEBUG 0
#define PICASSOIV_DEBUG_IO 0

#if MEMLOGR
static bool memlogr = true;
static bool memlogw = true;
#endif

#define BYTESWAP_WORD -1
#define BYTESWAP_LONG 1

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"

#ifdef USE_PCEM
#include "pcem/device.h"
#include "pcem/vid_s3_virge.h"
#include "pcem/vid_cl5429.h"
#include "pcem/vid_s3.h"
#include "pcem/vid_voodoo_banshee.h"
#include "pcem/vid_ncr.h"
#include "pcem/vid_permedia2.h"
#include "pcem/vid_inmos.h"
#include "pcem/vid_et4000.h"
#endif

#include "memory.h"
#include "debug.h"
#include "custom.h"
#include "newcpu.h"
#include "picasso96.h"
#include "statusline.h"
#include "rommgr.h"
#include "zfile.h"
#include "gfxboard.h"
#include "xwin.h"
#include "devices.h"
#include "gfxfilter.h"
#include "flashrom.h"
#include "pci.h"
#include "pci_hw.h"
#ifdef USE_PCEM
#include "pcem/pcemglue.h"
#endif
#include "qemuvga/qemuuaeglue.h"
#include "qemuvga/vga.h"
#ifdef WITH_DRACO
#include "draco.h"
#endif
#include "autoconf.h"

#ifdef USE_PCEM
extern void put_io_pcem(uaecptr, uae_u32, int);
extern uae_u32 get_io_pcem(uaecptr, int);
extern void put_mem_pcem(uaecptr, uae_u32, int);
extern uae_u32 get_mem_pcem(uaecptr, int);
#else
// Provide no-op stubs when PCem support is not compiled in
static inline void put_io_pcem(uaecptr, uae_u32, int) {}
static inline uae_u32 get_io_pcem(uaecptr, int) { return 0; }
static inline void put_mem_pcem(uaecptr, uae_u32, int) {}
static inline uae_u32 get_mem_pcem(uaecptr, int) { return 0; }
#endif

#define MONITOR_SWITCH_DELAY 25

#define GFXBOARD_AUTOCONFIG_SIZE 131072

#define BOARD_REGISTERS_SIZE 0x00010000

#define BOARD_MANUFACTURER_PICASSO 2167
#define BOARD_MODEL_MEMORY_PICASSOII 11
#define BOARD_MODEL_REGISTERS_PICASSOII 12

#define BOARD_MODEL_MEMORY_PICASSOIV 24
#define BOARD_MODEL_REGISTERS_PICASSOIV 23
#define PICASSOIV_REG  0x00600000
#define PICASSOIV_IO   0x00200000
#define PICASSOIV_VRAM1 0x01000000
#define PICASSOIV_VRAM2 0x00800000
#define PICASSOIV_ROM_OFFSET 0x0200
#define PICASSOIV_FLASH_OFFSET 0x8000
#define PICASSOIV_FLASH_BANK 0x10000
#define PICASSOIV_MAX_FLASH (GFXBOARD_AUTOCONFIG_SIZE - 32768)

#define PICASSOIV_BANK_UNMAPFLASH 2
#define PICASSOIV_BANK_MAPRAM 4
#define PICASSOIV_BANK_FLASHBANK 128

#define PICASSOIV_INT_VBLANK 128

#define BOARD_MANUFACTURER_PICCOLO 2195
#define BOARD_MODEL_MEMORY_PICCOLO 5
#define BOARD_MODEL_REGISTERS_PICCOLO 6
#define BOARD_MODEL_MEMORY_PICCOLO64 10
#define BOARD_MODEL_REGISTERS_PICCOLO64 11

#define BOARD_MANUFACTURER_SPECTRUM 2193
#define BOARD_MODEL_MEMORY_SPECTRUM 1
#define BOARD_MODEL_REGISTERS_SPECTRUM 2

struct gfxboard
{
	int id;
	const TCHAR *name;
	const TCHAR *manufacturername;
	const TCHAR *configname;
	int manufacturer;
	int model_memory;
	int model_registers;
	int model_extra;
	int serial;
	int vrammin;
	int vrammax;
	int banksize;
	int chiptype;
	int configtype;
	int irq;
	bool swap;
	bool hasswitcher;
	uae_u32 romtype;
	uae_u8 er_type;
	struct gfxboard_func *func;
#ifdef USE_PCEM
	const device_t *pcemdev;
#else
	bool pcemdev = false;
#endif
	uae_u8 er_flags;
	int bustype;
};

#define ISP4() (gb->rbc->rtgmem_type == GFXBOARD_ID_PICASSO4_Z2 || gb->rbc->rtgmem_type == GFXBOARD_ID_PICASSO4_Z3)

// Picasso II: 8* 4x256 (1M) or 16* 4x256 (2M)
// Piccolo: 8* 4x256 + 2* 16x256 (2M)

static const struct gfxboard boards[] =
{
	{
		GFXBOARD_ID_A2410,
		_T("A2410 [Zorro II]"), _T("Commodore"), _T("A2410"),
		1030, 0, 0, 0,
		0x00000000, 0x00200000, 0x00200000, 0x10000, 0, 2, 2, false, false,
		0, 0xc1, &a2410_func
	},
#ifdef USE_PCEM
	{
		GFXBOARD_ID_SPECTRUM_Z2,
		_T("Spectrum 28/24 [Zorro II]"), _T("Great Valley Products"), _T("Spectrum28/24_Z2"),
		BOARD_MANUFACTURER_SPECTRUM, BOARD_MODEL_MEMORY_SPECTRUM, BOARD_MODEL_REGISTERS_SPECTRUM, 0,
		0x00000000, 0x00100000, 0x00200000, 0x00200000, CIRRUS_ID_CLGD5428, 2, 6, true, true,
		0, 0, NULL, &gd5428_swapped_device
	},
	{
		GFXBOARD_ID_SPECTRUM_Z3,
		_T("Spectrum 28/24 [Zorro III]"), _T("Great Valley Products"), _T("Spectrum28/24_Z3"),
		BOARD_MANUFACTURER_SPECTRUM, BOARD_MODEL_MEMORY_SPECTRUM, BOARD_MODEL_REGISTERS_SPECTRUM, 0,
		0x00000000, 0x00100000, 0x00200000, 0x00200000, CIRRUS_ID_CLGD5428, 3, 6, true, true,
		0, 0, NULL, &gd5428_swapped_device
	},
	{
		GFXBOARD_ID_PICCOLO_Z2,
		_T("Piccolo [Zorro II]"), _T("Ingenieurbüro Helfrich"), _T("Piccolo_Z2"),
		BOARD_MANUFACTURER_PICCOLO, BOARD_MODEL_MEMORY_PICCOLO, BOARD_MODEL_REGISTERS_PICCOLO, 0,
		0x00000000, 0x00100000, 0x00200000, 0x00200000, CIRRUS_ID_CLGD5426, 2, 6, true, true,
		0, 0, NULL, &gd5426_swapped_device
	},
	{
		GFXBOARD_ID_PICCOLO_Z3,
		_T("Piccolo [Zorro III]"), _T("Ingenieurbüro Helfrich"), _T("Piccolo_Z3"),
		BOARD_MANUFACTURER_PICCOLO, BOARD_MODEL_MEMORY_PICCOLO, BOARD_MODEL_REGISTERS_PICCOLO, 0,
		0x00000000, 0x00100000, 0x00200000, 0x00200000, CIRRUS_ID_CLGD5426, 3, 6, true, true,
		0, 0, NULL, &gd5426_swapped_device
	},
	{
		GFXBOARD_ID_SD64_Z2,
		_T("Piccolo SD64 [Zorro II]"), _T("Ingenieurbüro Helfrich"), _T("PiccoloSD64_Z2"),
		BOARD_MANUFACTURER_PICCOLO, BOARD_MODEL_MEMORY_PICCOLO64, BOARD_MODEL_REGISTERS_PICCOLO64, 0,
		0x00000000, 0x00200000, 0x00400000, 0x00400000, CIRRUS_ID_CLGD5434, 2, 6, true, true,
		0, 0, NULL, &gd5434_vlb_swapped_device
	},
	{
		GFXBOARD_ID_SD64_Z3,
		_T("Piccolo SD64 [Zorro III]"), _T("Ingenieurbüro Helfrich"), _T("PiccoloSD64_Z3"),
		BOARD_MANUFACTURER_PICCOLO, BOARD_MODEL_MEMORY_PICCOLO64, BOARD_MODEL_REGISTERS_PICCOLO64, 0,
		0x00000000, 0x00200000, 0x00400000, 0x00400000, CIRRUS_ID_CLGD5434, 3, 6, true, true,
		0, 0, NULL, &gd5434_vlb_swapped_device
	},
	{
		GFXBOARD_ID_CV64_Z3,
		_T("CyberVision 64 [Zorro III]"), _T("Phase 5"), _T("CV64_Z3"),
		8512, 34, 0, 0,
		0x00000000, 0x00200000, 0x00400000, 0x20000000, 0, 3, 2, false, false,
		0, 0, NULL, &s3_cybervision_trio64_device, 0x40
	},
	{
		GFXBOARD_ID_CV643D_Z2,
		_T("CyberVision 64/3D [Zorro II]"), _T("Phase 5"), _T("CV643D_Z2"),
		8512, 67, 0, 0,
		0x00000000, 0x00400000, 0x00400000, 0x00400000, 0, 2, 2, false, false,
		0, 0, NULL, &s3_virge_device, 0xc0
	},
	{
		GFXBOARD_ID_CV643D_Z3,
		_T("CyberVision 64/3D [Zorro III]"), _T("Phase 5"), _T("CV643D_Z3"),
		8512, 67, 0, 0,
		0x00000000, 0x00400000, 0x00400000, 0x10000000, 0, 3, 2, false, false,
		0, 0, NULL, &s3_virge_device, 0x40
	},
	{
		GFXBOARD_ID_PERMEDIA2_PCI,
		_T("BlizzardVision/CyberVision PPC (Permedia2) [PCI]"), _T("3DLabs"), _T("PERMEDIA2_PCI"),
		0, 0, 0, 0,
		0x00000000, 0x00800000, 0x00800000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &permedia2_device, 0, GFXBOARD_BUSTYPE_PCI
	},
	{
		GFXBOARD_ID_PICASSO2,
		_T("Picasso II [Zorro II]"), _T("Village Tronic"), _T("PicassoII"),
		BOARD_MANUFACTURER_PICASSO, BOARD_MODEL_MEMORY_PICASSOII, BOARD_MODEL_REGISTERS_PICASSOII, 0,
		0x00020000, 0x00100000, 0x00200000, 0x00200000, CIRRUS_ID_CLGD5426, 2, 0, false, true,
		0, 0, NULL, &gd5426_device
	},
	{
		GFXBOARD_ID_PICASSO2PLUS,
		_T("Picasso II+ [Zorro II]"), _T("Village Tronic"), _T("PicassoII+"),
		BOARD_MANUFACTURER_PICASSO, BOARD_MODEL_MEMORY_PICASSOII, BOARD_MODEL_REGISTERS_PICASSOII, 0,
		0x00100000, 0x00100000, 0x00200000, 0x00200000, CIRRUS_ID_CLGD5428, 2, 2, false, true,
		0, 0, NULL, &gd5428_device
	},
	{
		GFXBOARD_ID_PICASSO4_Z2,
		_T("Picasso IV [Zorro II]"), _T("Village Tronic"), _T("PicassoIV_Z2"),
		BOARD_MANUFACTURER_PICASSO, BOARD_MODEL_MEMORY_PICASSOIV, BOARD_MODEL_REGISTERS_PICASSOIV, 0,
		0x00000000, 0x00200000, 0x00400000, 0x00400000, CIRRUS_ID_CLGD5446, 2, 2, false, true,
		ROMTYPE_PICASSOIV,
		0, NULL, &gd5446_device
	},
	{
		GFXBOARD_ID_PICASSO4_Z3,
		_T("Picasso IV [Zorro III]"), _T("Village Tronic"), _T("PicassoIV_Z3"),
		BOARD_MANUFACTURER_PICASSO, BOARD_MODEL_MEMORY_PICASSOIV, 0, 0,
		0x00000000, 0x00400000, 0x00400000, 0x02000000, CIRRUS_ID_CLGD5446, 3, 2, false, true,
		ROMTYPE_PICASSOIV,
		0, NULL, &gd5446_device
	},
	{
		GFXBOARD_ID_RETINA_Z2,
		_T("Retina [Zorro II]"), _T("MacroSystem"), _T("Retina_Z2"),
		18260, 6, 0, 0,
		0x00000000, 0x00100000, 0x00400000, 0x00020000, 0, 2, 2, false, false,
		0, 0, NULL, &ncr_retina_z2_device
	},
	{
		GFXBOARD_ID_RETINA_Z3,
		_T("Retina [Zorro III]"), _T("MacroSystem"), _T("Retina_Z3"),
		18260, 16, 0, 0,
		0x00000000, 0x00100000, 0x00400000, 0x00400000, 0, 3, 2, false, false,
		0, 0, NULL, &ncr_retina_z3_device
	},
	{
		GFXBOARD_ID_ALTAIS_Z3,
		_T("Altais [DracoBus]"), _T("MacroSystem"), _T("Altais"),
		18260, 19, 0, 0,
		0x00000000, 0x00400000, 0x00400000, 0x00400000, 0, BOARD_NONAUTOCONFIG_BEFORE, 3, false, false,
		0, 0, NULL, &ncr_retina_z3_device, 0, GFXBOARD_BUSTYPE_DRACO
	},
	{
		GFXBOARD_ID_MERLIN_Z2,
		_T("Merlin [Zorro II]"), _T("X-Pert Computer Services"), _T("MerlinZ2"),
		2117, 3, 4, 0,
		0x00000000, 0x00200000, 0x00200000, 0x00200000, 0, 2, 6, false, true,
		ROMTYPE_MERLIN,
		0, NULL, &et4000w32_merlin_z2_device
	},
	{
		GFXBOARD_ID_MERLIN_Z3,
		_T("Merlin [Zorro III]"), _T("X-Pert Computer Services"), _T("MerlinZ3"),
		2117, 3, 4, 0,
		0x00000000, 0x00200000, 0x00400000, 0x02000000, 0, 3, 6, false, true,
		ROMTYPE_MERLIN,
		0, NULL, &et4000w32_merlin_z3_device
	},
	{
		GFXBOARD_ID_GRAFFITY_Z2,
		_T("Graffity [Zorro II]"), _T("Atéo Concepts"), _T("GraffityZ2"),
		2092, 34, 33, 0,
		0x00000000, 0x00100000, 0x00200000, 0x00200000, CIRRUS_ID_CLGD5428, 2, 2, false, true,
		0, 0, NULL, &gd5428_device
	},
	{
		GFXBOARD_ID_GRAFFITY_Z3,
		_T("Graffity [Zorro III]"), _T("Atéo Concepts"), _T("GraffityZ3"),
		2092, 33, 0, 0,
		0x00000000, 0x00100000, 0x00200000, 0x01000000, CIRRUS_ID_CLGD5428, 3, 2, false, true,
		0, 0, NULL, &gd5428_device
	},
	{
		GFXBOARD_ID_EGS_110_24,
		_T("EGS 110/24 [GVP local bus]"), _T("GVP"), _T("EGS_110_24"),
		2193, 0, 0, 0,
		0x00000000, 0x00400000, 0x00800000, 0x00800000, 0, BOARD_NONAUTOCONFIG_BEFORE, 2, false, false,
		0, 0, NULL, &inmos_egs_110_24_device
	},
	{
		GFXBOARD_ID_VISIONA,
		_T("Visiona [Zorro II]"), _T("X-Pert Computer Services"), _T("Visiona"),
		2117, 2, 1, 0,
		0x00000000, 0x00200000, 0x00400000, 0x00400000, 0, 2, 6, false, false,
		0, 0, NULL, &inmos_visiona_z2_device
	},
	{
		GFXBOARD_ID_RAINBOWIII,
		_T("Rainbow III [Zorro III]"), _T("Ingenieurbüro Helfrich"), _T("RainbowIII"),
		2145, 33, 0, 0,
		0x00000000, 0x00400000, 0x00400000, 0x02000000, 0, 3, 6, false, false,
		0, 0, NULL, &inmos_rainbow3_z3_device
	},
	{
		GFXBOARD_ID_DOMINO,
		_T("Domino [Zorro II]"), _T("X-Pert Computer Services"), _T("Domino"),
		2167, 1, 2, 0,
		0x00000000, 0x00100000, 0x00100000, 0x00100000, 0, 2, 0, false, false,
		0, 0, NULL, &et4000_domino_device
	},
	{
		GFXBOARD_ID_PIXEL64,
		_T("Pixel64 [AteoBus]"), _T("Atéo Concepts"), _T("Pixel64"),
		2026, 255, 254, 0, // 255: type=$c7 flags=$40, 254: type=$c2 flags=$40 128k, 252: type=$c2 flags=$40, 128k
		0x00000000, 0x00200000, 0x00200000, 0x00400000, CIRRUS_ID_CLGD5434, 2, 0, false, false,
		0, 0, NULL, &gd5434_vlb_device
	},
	{
		GFXBOARD_ID_OMNIBUS_ET4000,
		_T("oMniBus ET4000AX [Zorro II]"), _T("ArMax"), _T("OmnibusET4000"),
		2181, 0, 0x100, 0,
		0x00000000, 0x00100000, 0x00100000, 0x00100000, 0, 2, 0, false, false,
		0, 0, NULL, &et4000_omnibus_device
	},
	{
		GFXBOARD_ID_OMNIBUS_ET4000W32,
		_T("oMniBus ET4000W32 [Zorro II]"), _T("ArMax"), _T("OmnibusET4000W32"),
		2181, 0, 0x100, 0,
		0x00000000, 0x00100000, 0x00100000, 0x00100000, 0, 2, 0, false, false,
		0, 0, NULL, &et4000w32_omnibus_device
	},
	{
		GFXBOARD_ID_HARLEQUIN,
		_T("Harlequin [Zorro II]"), _T("ACS"), _T("Harlequin_PAL"),
		2118, 100, 0, 0,
		0x00000000, 0x00200000, 0x00200000, 0x10000, 0, 0, 2, false, false,
		ROMTYPE_HARLEQUIN, 0xc2, &harlequin_func
	},
	{
		GFXBOARD_ID_RAINBOWII,
		_T("Rainbow II [Zorro II]"), _T("Ingenieurbüro Helfrich"), _T("RainbowII"),
		2145, 32, 0, 0,
		0x00000000, 0x00200000, 0x00200000, 0x00200000, 0, 0, 0, false, false,
		ROMTYPE_RAINBOWII, 0xc6, &rainbowii_func
	},
#if 0
	{
		_T("Resolver"), _T("DMI"), _T("Resolver"),
		2129, 1, 0,
		0x00000000, 0x00200000, 0x00200000, 0x10000, 0, 0, 2, false,
		0, 0xc1, &a2410_func
	},
#endif
	{
		GFXBOARD_ID_VOODOO3_PCI,
		_T("Voodoo 3 3000 [PCI]"), _T("3dfx"), _T("V3_3000"),
		0, 0, 0, 0,
		0x00000000, 0x01000000, 0x01000000, 0x01000000, 0, BOARD_PCI, -1, false, false,
		ROMTYPE_VOODOO3,
		0, NULL, &voodoo_3_3000_device, 0, GFXBOARD_BUSTYPE_PCI
	},
	{
		GFXBOARD_ID_S3VIRGE_PCI,
		_T("Virge [PCI]"), _T("S3"), _T("S3VIRGE_PCI"),
		0, 0, 0, 0,
		0x00000000, 0x00400000, 0x00400000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &s3_virge_device, 0, GFXBOARD_BUSTYPE_PCI
	},
	{
		GFXBOARD_ID_S3TRIO64_PCI,
		_T("Trio64 [PCI]"), _T("S3"), _T("S3TRIO64_PCI"),
		0, 0, 0, 0,
		0x00000000, 0x00200000, 0x00400000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &s3_trio64_device, 0, GFXBOARD_BUSTYPE_PCI
	},
	{
		GFXBOARD_ID_MATROX_MILLENNIUM_PCI,
		_T("Matrox Millennium [PCI]"), _T("Matrox"), _T("Matrox_Millennium"),
		0, 0, 0, 0,
		0x00000000, 0x00200000, 0x00400000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &millennium_device, 0, GFXBOARD_BUSTYPE_PCI
	},
	{
		GFXBOARD_ID_MATROX_MILLENNIUM_II_PCI,
		_T("Matrox Millennium II [PCI]"), _T("Matrox"), _T("Matrox_Millennium_II"),
		0, 0, 0, 0,
		0x00000000, 0x00200000, 0x01000000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &millennium_ii_device, 0, GFXBOARD_BUSTYPE_PCI
	},
	{
		GFXBOARD_ID_MATROX_MYSTIQUE_PCI,
		_T("Matrox Mystique [PCI]"), _T("Matrox"), _T("Matrox_Mystique"),
		0, 0, 0, 0,
		0x00000000, 0x00200000, 0x00800000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &mystique_device, 0, GFXBOARD_BUSTYPE_PCI
	},
	{
		GFXBOARD_ID_MATROX_MYSTIQUE220_PCI,
		_T("Matrox Mystique 220 [PCI]"), _T("Matrox"), _T("Matrox_Mystique220"),
		0, 0, 0, 0,
		0x00000000, 0x00200000, 0x00800000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &mystique_220_device, 0, GFXBOARD_BUSTYPE_PCI
	},
#endif
#if 0
	{
		GFXBOARD_ID_GD5446_PCI,
		_T("GD5446 [PCI]"), _T("Cirrus Logic"), _T("GD5446_PCI"),
		0, 0, 0, 0,
		0x00000000, 0x00400000, 0x00400000, 0x10000000, 0, BOARD_PCI, -1, false, false,
		0, 0, NULL, &gd5446_device, 0, GFXBOARD_BUSTYPE_PCI
	},
#endif
	{
		GFXBOARD_ID_VGA,
		_T("x86 Bridgeboard VGA [ISA]"), _T("x86"), _T("VGA"),
		0, 0, 0, 0,
		0x00000000, 0x00100000, 0x00200000, 0x00000000, CIRRUS_ID_CLGD5426, 0, 0, false, false,
		ROMTYPE_x86_VGA
	},
	{
	}
};

struct rtggfxboard
{
	bool active;
	int rtg_index;
	int monitor_id;
	struct rtgboardconfig *rbc;
	TCHAR memorybankname[100];
	TCHAR memorybanknamenojit[100];
	TCHAR wbsmemorybankname[100];
	TCHAR lbsmemorybankname[100];
	TCHAR regbankname[100];

	int configured_mem, configured_regs;
	const struct gfxboard *board;
	uae_u8 expamem_lo;
	uae_u8 *automemory;
	uae_u32 banksize_mask;
	uaecptr io_start, io_end;
	uaecptr mem_start[2], mem_end[2];

	uae_u8 picassoiv_bank, picassoiv_flifi;
	uae_u8 p4autoconfig[256];
	struct zfile *p4rom;
	void *p4flashrom;
	bool p4z2;
	uae_u32 p4_mmiobase;
	uae_u32 p4_special_mask;
	uae_u32 p4_special_start;
	uae_u32 p4_vram_bank[2];
	uae_u32 device_data;

	CirrusVGAState vga;
	uae_u8 *vram, *vramend, *vramrealstart;
	int vram_start_offset;
	int vrammask;
	uae_u32 gfxboardmem_start;
	bool monswitch_current, monswitch_new;
	bool monswitch_keep_trying;
	bool monswitch_reset;
	int monswitch_delay;
	int fullrefresh;
	int resolutionchange;
	uae_u8 *gfxboard_surface, *fakesurface_surface;
	bool gfxboard_intreq;
	bool gfxboard_intreq_status;
	bool gfxboard_intreq_marked;
	bool gfxboard_external_interrupt;
	int gfxboard_intena;
	bool vram_enabled, vram_offset_enabled;
	hwaddr vram_offset[2];
	uae_u8 cirrus_pci[0x44];
	uae_u8 p4i2c;
	uae_u8 p4_pci[0x44];
	int vga_width, vga_height, vga_width_mult, vga_height_mult;
	bool vga_refresh_active;
	int device_settings;
	uae_u32 extradata[16];

	uae_u32 vgaioregionptr, vgavramregionptr, vgabank0regionptr, vgabank1regionptr;

	const MemoryRegionOps *vgaio, *vgaram, *vgalowram, *vgammio;
	MemoryRegion vgaioregion, vgavramregion;
	DisplaySurface gfxsurface, fakesurface;

	addrbank gfxboard_bank_memory;
	addrbank gfxboard_bank_memory_nojit;
	addrbank gfxboard_bank_wbsmemory;
	addrbank gfxboard_bank_lbsmemory;
	addrbank gfxboard_bank_nbsmemory;
	addrbank gfxboard_bank_registers;
	addrbank gfxboard_bank_registers2;
	addrbank gfxboard_bank_special;

	addrbank gfxboard_bank_vram_pcem;
	addrbank gfxboard_bank_vram_normal_pcem;
	addrbank gfxboard_bank_vram_wordswap_pcem;
	addrbank gfxboard_bank_vram_longswap_pcem;
	addrbank gfxboard_bank_vram_cv_1_pcem;
	addrbank gfxboard_bank_vram_p4z2_pcem;
	addrbank gfxboard_bank_io_pcem;
	addrbank gfxboard_bank_io_swap_pcem;
	addrbank gfxboard_bank_io_swap2_pcem;
	addrbank gfxboard_bank_pci_pcem;
	addrbank gfxboard_bank_mmio_pcem;
	addrbank gfxboard_bank_mmio_wbs_pcem;
	addrbank gfxboard_bank_mmio_lbs_pcem;
	addrbank gfxboard_bank_special_pcem;
	addrbank gfxboard_bank_bios;

	addrbank *original_pci_bank;

	addrbank *gfxmem_bank;
	uae_u8 *vram_back;
	
	struct autoconfig_info *aci;

	struct gfxboard_func *func;

#ifdef USE_PCEM
	const device_t *pcemdev;
#else
	bool pcemdev = false;
#endif
	void *pcemobject; // device_t
	void *pcemobject2; // svga_t
	int pcem_mmio_offset;
	int pcem_mmio_mask;
	bool pcem_pci_configured;
	int pcem_data[2];
	int pcem_vram_offset;
	int pcem_vram_mask;
	int pcem_io_mask;
	int pcem_vblank;
	bool p4_revb;
	uae_u8 *bios;
	uae_u32 bios_mask;
	int lfbbyteswapmode;
	int mmiobyteswapmode;
	struct pci_board_state *pcibs;
	bool pcem_direct;
	int lfbbar;

	int width_redraw, height_redraw;

	void *userdata;
};

static struct rtggfxboard rtggfxboards[MAX_RTG_BOARDS];
static struct rtggfxboard *only_gfx_board;
static int rtg_visible[MAX_AMIGADISPLAYS];
static int rtg_initial[MAX_AMIGADISPLAYS];
static int total_active_gfx_boards;
static int vram_ram_a8;
static DisplaySurface fakesurface;

DECLARE_MEMORY_FUNCTIONS(gfxboard);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, mem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, mem_nojit);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, bsmem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, wbsmem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, lbsmem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, nbsmem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, regs);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboards, regs);

#ifdef USE_PCEM
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, vram_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, vram_normal_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, vram_wordswap_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, vram_longswap_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, vram_cv_1_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, vram_p4z2_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, io_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, io_swap_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, io_swap2_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, pci_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, mmio_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, mmio_wbs_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, mmio_lbs_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, special_pcem);
DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(gfxboard, bios);
#endif

static const addrbank tmpl_gfxboard_bank_memory = {
	gfxboard_lget_mem, gfxboard_wget_mem, gfxboard_bget_mem,
	gfxboard_lput_mem, gfxboard_wput_mem, gfxboard_bput_mem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, NULL,
	gfxboard_lget_mem, gfxboard_wget_mem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL, 0, 0
};

static const addrbank tmpl_gfxboard_bank_memory_nojit = {
	gfxboard_lget_mem_nojit, gfxboard_wget_mem_nojit, gfxboard_bget_mem_nojit,
	gfxboard_lput_mem_nojit, gfxboard_wput_mem_nojit, gfxboard_bput_mem_nojit,
	gfxboard_xlate, gfxboard_check, NULL, NULL, NULL,
	gfxboard_lget_mem_nojit, gfxboard_wget_mem_nojit,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE | ABFLAG_CACHE_ENABLE_ALL, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_wbsmemory = {
	gfxboard_lget_wbsmem, gfxboard_wget_wbsmem, gfxboard_bget_wbsmem,
	gfxboard_lput_wbsmem, gfxboard_wput_wbsmem, gfxboard_bput_wbsmem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, NULL,
	gfxboard_lget_wbsmem, gfxboard_wget_wbsmem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE | ABFLAG_CACHE_ENABLE_ALL, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_lbsmemory = {
	gfxboard_lget_lbsmem, gfxboard_wget_lbsmem, gfxboard_bget_lbsmem,
	gfxboard_lput_lbsmem, gfxboard_wput_lbsmem, gfxboard_bput_lbsmem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, NULL,
	gfxboard_lget_lbsmem, gfxboard_wget_lbsmem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE | ABFLAG_CACHE_ENABLE_ALL, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_nbsmemory = {
	gfxboard_lget_nbsmem, gfxboard_wget_nbsmem, gfxboard_bget_bsmem,
	gfxboard_lput_nbsmem, gfxboard_wput_nbsmem, gfxboard_bput_bsmem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, _T("Picasso IV banked VRAM"),
	gfxboard_lget_nbsmem, gfxboard_wget_nbsmem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE | ABFLAG_CACHE_ENABLE_ALL, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_registers = {
	gfxboard_lget_regs, gfxboard_wget_regs, gfxboard_bget_regs,
	gfxboard_lput_regs, gfxboard_wput_regs, gfxboard_bput_regs,
	default_xlate, default_check, NULL, NULL, NULL,
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_registers2 = {
	gfxboard_lget_regs, gfxboard_wget_regs, gfxboard_bget_regs,
	gfxboard_lput_regs, gfxboard_wput_regs, gfxboard_bput_regs,
	default_xlate, default_check, NULL, NULL, NULL,
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};


static const addrbank tmpl_gfxboard_bank_special = {
	gfxboards_lget_regs, gfxboards_wget_regs, gfxboards_bget_regs,
	gfxboards_lput_regs, gfxboards_wput_regs, gfxboards_bput_regs,
	default_xlate, default_check, NULL, NULL, _T("Picasso IV MISC"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

#ifdef USE_PCEM
static const addrbank tmpl_gfxboard_bank_vram_pcem = {
	gfxboard_lget_vram_pcem, gfxboard_wget_vram_pcem, gfxboard_bget_vram_pcem,
	gfxboard_lput_vram_pcem, gfxboard_wput_vram_pcem, gfxboard_bput_vram_pcem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, _T("PCem SVGA VRAM"),
	gfxboard_lget_vram_pcem, gfxboard_wget_vram_pcem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_vram_normal_pcem = {
	gfxboard_lget_vram_normal_pcem, gfxboard_wget_vram_normal_pcem, gfxboard_bget_vram_normal_pcem,
	gfxboard_lput_vram_normal_pcem, gfxboard_wput_vram_normal_pcem, gfxboard_bput_vram_normal_pcem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, _T("PCem SVGA VRAM (NOSWAP)"),
	gfxboard_lget_vram_normal_pcem, gfxboard_wget_vram_normal_pcem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_vram_wordswap_pcem = {
	gfxboard_lget_vram_wordswap_pcem, gfxboard_wget_vram_wordswap_pcem, gfxboard_bget_vram_wordswap_pcem,
	gfxboard_lput_vram_wordswap_pcem, gfxboard_wput_vram_wordswap_pcem, gfxboard_bput_vram_wordswap_pcem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, _T("PCem SVGA VRAM (WORDSWAP)"),
	gfxboard_lget_vram_wordswap_pcem, gfxboard_wget_vram_wordswap_pcem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_vram_longswap_pcem = {
	gfxboard_lget_vram_longswap_pcem, gfxboard_wget_vram_longswap_pcem, gfxboard_bget_vram_longswap_pcem,
	gfxboard_lput_vram_longswap_pcem, gfxboard_wput_vram_longswap_pcem, gfxboard_bput_vram_longswap_pcem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, _T("PCem SVGA VRAM (LONGSWAP)"),
	gfxboard_lget_vram_longswap_pcem, gfxboard_wget_vram_longswap_pcem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_vram_cv_1_pcem = {
	gfxboard_lget_vram_cv_1_pcem, gfxboard_wget_vram_cv_1_pcem, gfxboard_bget_vram_cv_1_pcem,
	gfxboard_lput_vram_cv_1_pcem, gfxboard_wput_vram_cv_1_pcem, gfxboard_bput_vram_cv_1_pcem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, _T("PCem SVGA VRAM (CV64)"),
	gfxboard_lget_vram_cv_1_pcem, gfxboard_wget_vram_cv_1_pcem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};
static const addrbank tmpl_gfxboard_bank_vram_p4z2_pcem = {
	gfxboard_lget_vram_p4z2_pcem, gfxboard_wget_vram_p4z2_pcem, gfxboard_bget_vram_p4z2_pcem,
	gfxboard_lput_vram_p4z2_pcem, gfxboard_wput_vram_p4z2_pcem, gfxboard_bput_vram_p4z2_pcem,
	gfxboard_xlate, gfxboard_check, NULL, NULL, _T("PCem SVGA VRAM (PIVZ2)"),
	gfxboard_lget_vram_p4z2_pcem, gfxboard_wget_vram_p4z2_pcem,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_PPCIOSPACE, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static const addrbank tmpl_gfxboard_bank_io_pcem = {
	gfxboard_lget_io_pcem, gfxboard_wget_io_pcem, gfxboard_bget_io_pcem,
	gfxboard_lput_io_pcem, gfxboard_wput_io_pcem, gfxboard_bput_io_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA IO SWAP"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_io_swap_pcem = {
	gfxboard_lget_io_swap_pcem, gfxboard_wget_io_swap_pcem, gfxboard_bget_io_swap_pcem,
	gfxboard_lput_io_swap_pcem, gfxboard_wput_io_swap_pcem, gfxboard_bput_io_swap_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA IO SWAP"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_io_swap2_pcem = {
	gfxboard_lget_io_swap2_pcem, gfxboard_wget_io_swap2_pcem, gfxboard_bget_io_swap2_pcem,
	gfxboard_lput_io_swap2_pcem, gfxboard_wput_io_swap2_pcem, gfxboard_bput_io_swap2_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA IO SWAP2"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_pci_pcem = {
	gfxboard_lget_pci_pcem, gfxboard_wget_pci_pcem, gfxboard_bget_pci_pcem,
	gfxboard_lput_pci_pcem, gfxboard_wput_pci_pcem, gfxboard_bput_pci_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA PCI"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_mmio_pcem = {
	gfxboard_lget_mmio_pcem, gfxboard_wget_mmio_pcem, gfxboard_bget_mmio_pcem,
	gfxboard_lput_mmio_pcem, gfxboard_wput_mmio_pcem, gfxboard_bput_mmio_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA MMIO"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_mmio_wbs_pcem = {
	gfxboard_lget_mmio_wbs_pcem, gfxboard_wget_mmio_wbs_pcem, gfxboard_bget_mmio_wbs_pcem,
	gfxboard_lput_mmio_wbs_pcem, gfxboard_wput_mmio_wbs_pcem, gfxboard_bput_mmio_wbs_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA MMIO WORDSWAP"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static const addrbank tmpl_gfxboard_bank_mmio_lbs_pcem = {
	gfxboard_lget_mmio_lbs_pcem, gfxboard_wget_mmio_lbs_pcem, gfxboard_bget_mmio_lbs_pcem,
	gfxboard_lput_mmio_lbs_pcem, gfxboard_wput_mmio_lbs_pcem, gfxboard_bput_mmio_lbs_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA MMIO WORDSWAP"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};


static const addrbank tmpl_gfxboard_bank_special_pcem = {
	gfxboard_lget_special_pcem, gfxboard_wget_special_pcem, gfxboard_bget_special_pcem,
	gfxboard_lput_special_pcem, gfxboard_wput_special_pcem, gfxboard_bput_special_pcem,
	default_xlate, default_check, NULL, NULL, _T("PCem SVGA SPC"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};
#endif

static void REGPARAM2 dummy_lput(uaecptr addr, uae_u32 l)
{
}
static void REGPARAM2 dummy_wput(uaecptr addr, uae_u32 l)
{
}
static void REGPARAM2 dummy_bput(uaecptr addr, uae_u32 l)
{
}

#ifdef USE_PCEM
static const addrbank tmpl_gfxboard_bank_bios = {
	gfxboard_lget_bios, gfxboard_wget_bios, gfxboard_bget_bios,
	dummy_lput, dummy_wput, dummy_bput,
	default_xlate, default_check, NULL, NULL, _T("SVGA BIOS"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_ROM | ABFLAG_SAFE, S_READ, S_WRITE
};
#endif

static void ew(struct rtggfxboard *gb, int addr, uae_u32 value)
{
	addr &= 0xffff;
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		gb->automemory[addr] = (value & 0xf0);
		gb->automemory[addr + 2] = (value & 0x0f) << 4;
	} else {
		gb->automemory[addr] = ~(value & 0xf0);
		gb->automemory[addr + 2] = ~((value & 0x0f) << 4);
	}
}

int gfxboard_get_devnum(struct uae_prefs *p, int index)
{
	int devnum = 0;
	uae_u32 romtype = gfxboard_get_romtype(&p->rtgboards[index]);
	if (!romtype)
		return devnum;
	for (int i = 0; i < index; i++) {
		if (gfxboard_get_romtype(&p->rtgboards[i]) == romtype)
			devnum++;
	}
	return devnum;
}

void gfxboard_get_a8_vram(int index)
{
	addrbank *ab = gfxmem_banks[index];
	if (vram_ram_a8 > 0) {
		addrbank *prev = gfxmem_banks[vram_ram_a8 - 1];
		ab->baseaddr = prev->baseaddr;
	} else {
		mapped_malloc(ab);
		vram_ram_a8 = index + 1;
	}
}

void gfxboard_free_vram(int index)
{
	addrbank *ab = gfxmem_banks[index];
	if (vram_ram_a8 - 1 == index || vram_ram_a8 == 0) {
		mapped_free(ab);
	}
	if (vram_ram_a8 - 1 == index)
		vram_ram_a8 = 0;
}

extern uae_u8 *getpcembuffer32(int, int, int);
extern int svga_get_vtotal(void *p);
extern int svga_poll(void *p);
extern void voodoo_callback(void *p);

// PCEM
#ifdef USE_PCEM
static void pcem_flush(struct rtggfxboard* gb, int index)
{
	if (gb->pcem_direct) {
		uae_u8 **buf;
		uae_u8 *start;
		int cnt = picasso_getwritewatch(index, gb->vram_start_offset, &buf, &start);
		if (cnt < 0) {
			gb->pcemdev->force_redraw(gb->pcemobject);
		} else {
			for (int i = 0; i < cnt; i++) {
				int offset = (int)(buf[i] - start);
				pcem_linear_mark(offset);
			}
		}
	}
}

void video_blit_memtoscreen(int x, int y, int y1, int y2, int w, int h)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (gb->pcemdev && gb->pcemobject) {
			pcem_flush(gb, i);
			if (rtg_visible[gb->monitor_id] == i && gb->monswitch_delay == 0 && gb->monswitch_current == gb->monswitch_new) {
				if (gb->gfxboard_surface == NULL) {
					gb->gfxboard_surface = gfx_lock_picasso(gb->monitor_id, false);
				}
				if (gb->gfxboard_surface) {
					struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[gb->monitor_id];
					if (w != gb->width_redraw || h != gb->height_redraw) {
						for (int y = 0; y < vidinfo->maxheight; y++) {
							uae_u8 *d = gb->gfxboard_surface + y * vidinfo->rowbytes;
							if (y < h) {
								if (vidinfo->maxwidth > w) {
									memset(d + w * vidinfo->pixbytes, 0, (vidinfo->maxwidth - w) * vidinfo->pixbytes);
								}
							} else {
								memset(d, 0, vidinfo->maxwidth * vidinfo->pixbytes);
							}
						}
						gb->width_redraw = w;
						gb->height_redraw = h;
						y1 = 0;
						y2 = h;
					}
					for (int yy = y1; yy < y2 && yy < vidinfo->maxheight; yy++) {
						uae_u8 *d = gb->gfxboard_surface + yy * vidinfo->rowbytes;
						uae_u8 *s = getpcembuffer32(x, y, yy);
						int ww = w > vidinfo->maxwidth ? vidinfo->maxwidth : w;
						memcpy(d, s, ww * vidinfo->pixbytes);
					}
				}
			}
			if (gb->pcem_direct) {
				picasso_getwritewatch(i, 0, NULL, NULL);
			}
		}
	}
}

static int gfxboard_pcem_poll(struct rtggfxboard *gb)
{
	static int toggle;
	if (!gb->vram)
		return 0;
	// 1, 3, 1, 3,.. because some software needs to see hsync period.
	int v = svga_poll(gb->pcemobject2);
	if (toggle) {
		if (!v) {
			v |= svga_poll(gb->pcemobject2);
			if (!v) {
				v |= svga_poll(gb->pcemobject2);
			}
		}
		toggle = 0;
	} else {
		toggle = 1;
	}
	return v;
}
#endif

static void gfxboard_rethink(void)
{
#ifdef USE_PCEM
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (gb->pcemdev && gb->pcemobject) {
			int irq = 0;
			if (gb->board->bustype == GFXBOARD_BUSTYPE_DRACO) {
#ifdef WITH_DRACO
				void draco_svga_irq(bool state);
				draco_svga_irq(gb->gfxboard_intreq);
#endif
			} else if (gb->gfxboard_intena) {
				bool intreq = gb->gfxboard_intreq;
				if (gb->gfxboard_external_interrupt) {
					intreq |= gb->gfxboard_intreq_marked;
				}
				if (intreq) {
					if (gb->board->irq == 2 && gb->gfxboard_intena != 6)
						irq = 2;
					else
						irq = 6;
					if (irq > 0) {
						safe_interrupt_set(IRQ_SOURCE_GFX, gb->monitor_id, irq == 6);
					}
				}
			}
		}
	}
#endif
}

extern int p96syncrate;

static void gfxboard_hsync_handler(void)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (!gb->active)
			continue;
		if (gb->func && gb->userdata) {
			gb->func->hsync(gb->userdata);
		}
		if (gb->pcemdev && gb->pcemobject2 && !gb->pcem_vblank) {
#ifdef USE_PCEM
			static int pollcnt;
			bool pirq = gb->gfxboard_intreq_status;
			int total = svga_get_vtotal(gb->pcemobject2);
			if (total <= 0)
				total = p96syncrate;
			int pollsize = (total << 8) / p96syncrate;
			pollcnt += pollsize;
			while (pollcnt >= 256) {
				if (gfxboard_pcem_poll(gb)) {
					gb->pcem_vblank = 1;
					pollcnt &= 0xff;
					break;
				}
				bool nirq = gb->gfxboard_intreq_status;
				// exit immediately if new interrupt was generated. It might be used for screen split.
				if (!pirq && nirq) {
					pollcnt &= 0xff;
					break;
				}
				pollcnt -= 256;
			}
#endif
		}
	}
#ifdef USE_PCEM
	pcemglue_hsync();
#endif
}

static void reinit_vram(struct rtggfxboard *gb, uaecptr vram, bool direct)
{
	if (vram == gb->gfxmem_bank->start)
		return;
	gb->vram = NULL;
	mapped_free(gb->gfxmem_bank);
	gb->gfxmem_bank->flags &= ~ABFLAG_ALLOCINDIRECT;
	if (!direct) {
		gb->gfxmem_bank->flags |= ABFLAG_ALLOCINDIRECT;
	}
	gb->gfxmem_bank->label = _T("*");
	gb->gfxmem_bank->start = vram;
	mapped_malloc(gb->gfxmem_bank);
	gb->vram = gb->gfxmem_bank->baseaddr;
	gb->vramend = gb->gfxmem_bank->baseaddr + gb->gfxmem_bank->reserved_size;
	gb->vramrealstart = gb->vram;
	gb->vram += gb->vram_start_offset;
	gb->vramend += gb->vram_start_offset;
	if (gb->pcemdev) {
#ifdef USE_PCEM
		void svga_setvram(void *p, uint8_t *vram);
		svga_setvram(gb->pcemobject2, gb->vram);
		if (gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO3_PCI || gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO5_PCI) {
			void voodoo_update_vram(void *p);
			voodoo_update_vram(gb->pcemobject);
		}
#endif
	}
}

static void init_board (struct rtggfxboard *gb)
{
	struct rtgboardconfig *rbc = gb->rbc;
	int vramsize = gb->board->vrammax;
	int chiptype = gb->board->chiptype;

	if (gb->board->romtype == ROMTYPE_x86_VGA) {
		struct romconfig *rc = get_device_romconfig(&currprefs, gb->board->romtype, 0);
		chiptype = CIRRUS_ID_CLGD5426;
		if (rc && rc->device_settings == 1) {
			chiptype = CIRRUS_ID_CLGD5429;
		}
	}

	gb->active = true;
	gb->vga_width = 0;
	gb->vga_height = 0;
	mapped_free(gb->gfxmem_bank);
	gb->vram_start_offset = 0;
	if (ISP4() && !gb->p4z2) { // JIT direct compatibility hack
		gb->vram_start_offset = 0x01000000;
	}
	vramsize += gb->vram_start_offset;
	xfree (gb->fakesurface_surface);
	gb->fakesurface_surface = xmalloc (uae_u8, 4 * 10000);
	gb->vram_offset[0] = gb->vram_offset[1] = 0;
	gb->vram_enabled = true;
	gb->vram_offset_enabled = false;
	gb->gfxmem_bank->reserved_size = vramsize;
	gb->gfxmem_bank->start = gb->gfxboardmem_start;
	if (gb->board->manufacturer && gb->board->banksize >= 0x40000) {
		gb->gfxmem_bank->flags |= ABFLAG_ALLOCINDIRECT | ABFLAG_PPCIOSPACE;
		gb->gfxmem_bank->label = _T("*");
		mapped_malloc(gb->gfxmem_bank);
	} else if (gb->board->bustype == GFXBOARD_BUSTYPE_PCI) {
		// We don't know VRAM address until PCI bridge and
		// PCI display card's BARs have been initialized
		;
	} else {
		gb->gfxmem_bank->flags |= ABFLAG_ALLOCINDIRECT | ABFLAG_PPCIOSPACE;
		gb->gfxmem_bank->label = _T("*");
		gb->vram_back = xmalloc(uae_u8, vramsize);
		if (&get_mem_bank(0x800000) == &dummy_bank)
			gb->gfxmem_bank->start = 0x800000;
		else
			gb->gfxmem_bank->start = 0xa00000;
		gfxboard_get_a8_vram(gb->rbc->rtg_index);
	}
	gb->vrammask = vramsize - 1;
	gb->vram = gb->gfxmem_bank->baseaddr;
	gb->vramend = gb->gfxmem_bank->baseaddr + vramsize;
	gb->vramrealstart = gb->vram;
	gb->vram += gb->vram_start_offset;
	gb->vramend += gb->vram_start_offset;
	//gb->gfxmem_bank->baseaddr = gb->vram;
	// restore original value because this is checked against
	// configured size in expansion.cpp
	if (!gb->board->bustype) {
		gb->gfxmem_bank->allocated_size = rbc->rtgmem_size;
		gb->gfxmem_bank->reserved_size = rbc->rtgmem_size;
	}
	gb->vga.vga.vram_size_mb = rbc->rtgmem_size >> 20;
	gb->vgaioregion.opaque = &gb->vgaioregionptr;
	gb->vgaioregion.data = gb;
	gb->vgavramregion.opaque = &gb->vgavramregionptr;
	gb->vgavramregion.data = gb;
	gb->vga.vga.vram.opaque = &gb->vgavramregionptr;
	gb->vga.vga.vram.data = gb;
	gb->vga.cirrus_vga_io.data = gb;
	gb->vga.low_mem_container.data = gb;
	gb->vga.low_mem.data = gb;
	gb->vga.cirrus_bank[0].data = gb;
	gb->vga.cirrus_bank[1].data = gb;
	gb->vga.cirrus_linear_io.data = gb;
	gb->vga.cirrus_linear_bitblt_io.data = gb;
	gb->vga.cirrus_mmio_io.data = gb;
	gb->gfxsurface.data = gb;
	gb->fakesurface.data = gb;
	vga_common_init(&gb->vga.vga);
	gb->vga.vga.con = (void*)gb;
	if (chiptype) {
		cirrus_init_common(&gb->vga, chiptype, 0, NULL, NULL, gb->board->manufacturer == 0, gb->board->romtype == ROMTYPE_x86_VGA);
	}
	gb->pcemdev = gb->board->pcemdev;
	gb->pcem_pci_configured = false;
	if (gb->pcemdev) {
#ifdef USE_PCEM
		extern void initpcemvideo(void*, bool);
		extern void *svga_get_object(void);
		initpcemvideo(gb, gb->board->swap);
		gb->pcemobject = gb->pcemdev->init(gb->pcemdev);
		gb->pcemobject2 = svga_get_object();
#endif
	}
	picasso_allocatewritewatch(gb->rbc->rtg_index, gb->rbc->rtgmem_size);

	device_add_hsync(gfxboard_hsync_handler);
	device_add_rethink(gfxboard_rethink);
}

static void merlin_init(struct rtggfxboard *gb)
{
	struct boardromconfig *brc = get_device_rom(&currprefs, ROMTYPE_MERLIN, 0, NULL);
	if (brc) {
		const TCHAR *ser = brc->roms[0].configtext;
		gb->extradata[0] = _tstol(ser);
	} else {
		gb->extradata[0] = 1;
	}
	gb->extradata[0] ^= 0x5554452A;
}

static int GetBytesPerPixel(RGBFTYPE RGBfmt)
{
	switch (RGBfmt)
	{
		case RGBFB_CLUT:
		return 1;

		case RGBFB_A8R8G8B8:
		case RGBFB_A8B8G8R8:
		case RGBFB_R8G8B8A8:
		case RGBFB_B8G8R8A8:
		return 4;

		case RGBFB_B8G8R8:
		case RGBFB_R8G8B8:
		return 3;

		case RGBFB_R5G5B5:
		case RGBFB_R5G6B5:
		case RGBFB_R5G6B5PC:
		case RGBFB_R5G5B5PC:
		case RGBFB_B5G6R5PC:
		case RGBFB_B5G5R5PC:
		return 2;
	default: return 0;
	}
	return 0;
}

static void gfxboard_unlock(struct rtggfxboard *gb)
{
	if (gb->gfxboard_surface) {
		gfx_unlock_picasso(gb->monitor_id, true);
		gb->gfxboard_surface = NULL;
	}
}

static bool gfxboard_setmode(struct rtggfxboard *gb, struct gfxboard_mode *mode)
{
	struct amigadisplay *ad = &adisplays[gb->monitor_id];
	struct picasso96_state_struct *state = &picasso96_state[gb->monitor_id];

	state->Width = mode->width;
	state->Height = mode->height;
	state->VirtualWidth = state->Width;
	state->VirtualHeight = state->Height;
	state->HLineDBL = mode->hlinedbl ? mode->hlinedbl : 1;
	state->VLineDBL = mode->vlinedbl ? mode->vlinedbl : 1;
	int bpp = GetBytesPerPixel(mode->mode);
	state->BytesPerPixel = bpp;
	state->BytesPerRow = mode->width * bpp;
	state->RGBFormat = mode->mode;
	write_log(_T("GFXBOARD %dx%dx%d\n"), mode->width, mode->height, bpp);
	if (!ad->picasso_requested_on && !ad->picasso_on) {
		ad->picasso_requested_on = true;
		set_config_changed();
	}
	gfxboard_unlock(gb);
	return true;
}

static void gfxboard_free_slot2(struct rtggfxboard *gb)
{
	struct amigadisplay *ad = &adisplays[gb->monitor_id];
	gb->active = false;
	if (rtg_visible[gb->monitor_id] == gb->rtg_index) {
		rtg_visible[gb->monitor_id] = -1;
		ad->picasso_requested_on = false;
		set_config_changed();
	}
	if (gb->pcemdev) {
#ifdef USE_PCEM
		if (gb->pcemobject) {
			pcemfreeaddeddevices();
			gb->pcemdev->close(gb->pcemobject);
			gb->pcemobject = NULL;
		}
#endif
	}
	gb->userdata = NULL;
	gb->func = NULL;
	xfree(gb->automemory);
	gb->automemory = NULL;
}

static const struct gfxboard *find_board(int id)
{
	for (int i = 0; boards[i].id; i++) {
		if (id == boards[i].id)
			return &boards[i];
	}
	return NULL;
}

bool gfxboard_allocate_slot(int board, int idx)
{
	struct rtggfxboard *gb = &rtggfxboards[idx];
	const struct gfxboard *gfxb = find_board(board);
	if (!gfxb)
		return false;
	gb->active = true;
	gb->rtg_index = idx;
	gb->board = gfxb;
	return true;
}
void gfxboard_free_slot(int idx)
{
	struct rtggfxboard *gb = &rtggfxboards[idx];
	gfxboard_free_slot2(gb);
}

static int gfx_temp_bank_idx;

static uae_u32 REGPARAM2 gtb_wget(uaecptr addr)
{
	struct rtggfxboard *gb = &rtggfxboards[gfx_temp_bank_idx];
	addr &= 0xffff;
	return 0;
}
static uae_u32 REGPARAM2 gtb_bget(uaecptr addr)
{
	struct rtggfxboard *gb = &rtggfxboards[gfx_temp_bank_idx];
	addr &= 0xffff;
	if (addr < GFXBOARD_AUTOCONFIG_SIZE)
		return gb->automemory[addr];
	return 0xff;
}
static void REGPARAM2 gtb_bput(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = &rtggfxboards[gfx_temp_bank_idx];
	b &= 0xff;
	addr &= 0xffff;
	if (addr == 0x48) {
		gfx_temp_bank_idx++;
		map_banks_z2(gb->gfxmem_bank, expamem_board_pointer >> 16, expamem_board_size >> 16);
		gb->func->configured(gb->userdata, expamem_board_pointer);
		expamem_next(gb->gfxmem_bank, NULL);
		return;
	}
	if (addr == 0x4c) {
		expamem_shutup(gb->gfxmem_bank);
		return;
	}
}
static void REGPARAM2 gtb_wput(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = &rtggfxboards[gfx_temp_bank_idx];
	b &= 0xffff;
	addr &= 0xffff;
	if (addr == 0x44) {
		gfx_temp_bank_idx++;
		map_banks_z3(gb->gfxmem_bank, expamem_board_pointer >> 16, expamem_board_size >> 16);
		gb->func->configured(gb->userdata, expamem_board_pointer);
		expamem_next(gb->gfxmem_bank, NULL);
		return;
	}
}

static addrbank gfx_temp_bank =
{
	gtb_wget, gtb_wget, gtb_bget,
	gtb_wput, gtb_wput, gtb_bput,
	default_xlate, default_check, NULL, NULL, _T("GFXBOARD_AUTOCONFIG"),
	gtb_wget, gtb_wget,
	ABFLAG_IO, S_READ, S_WRITE
};

bool gfxboard_init_board(struct autoconfig_info *aci)
{
	const struct gfxboard *gfxb = find_board(aci->prefs->rtgboards[aci->devnum].rtgmem_type);
	if (!gfxb)
		return false;
	struct rtggfxboard *gb = &rtggfxboards[aci->devnum];
	gb->func = gfxb->func;
	gb->pcemdev = gfxb->pcemdev;
	gb->monitor_id = aci->prefs->rtgboards[aci->devnum].monitor_id;
	memset(aci->autoconfig_bytes, 0xff, sizeof aci->autoconfig_bytes);
	if (!gb->automemory)
		gb->automemory = xmalloc(uae_u8, GFXBOARD_AUTOCONFIG_SIZE);
	memset(gb->automemory, 0xff, GFXBOARD_AUTOCONFIG_SIZE);
	ew(gb, 0x00, gfxb->er_type);
	ew(gb, 0x04, gfxb->model_memory);
	ew(gb, 0x10, (gfxb->manufacturer >> 8) & 0xff);
	ew(gb, 0x14, (gfxb->manufacturer >> 0) & 0xff);
	ew(gb, 0x18, (gfxb->serial >> 24) & 0xff);
	ew(gb, 0x1c, (gfxb->serial >> 16) & 0xff);
	ew(gb, 0x20, (gfxb->serial >>  8) & 0xff);
	ew(gb, 0x24, (gfxb->serial >>  0) & 0xff);
	memcpy(aci->autoconfig_raw, gb->automemory, sizeof aci->autoconfig_raw);
	if (!gb->func->init(aci))
		return false;
	for(int i = 0; i < sizeof aci->autoconfig_bytes; i++) {
		if (aci->autoconfig_bytes[i] != 0xff)
			ew(gb, i * 4, aci->autoconfig_bytes[i]);
	}
	memcpy(aci->autoconfig_raw, gb->automemory, sizeof aci->autoconfig_raw);
	if (!aci->doinit)
		return true;
	gb->banksize_mask = gfxb->banksize - 1;
	gb->userdata = aci->userdata;
	gb->active = true;
	gb->rtg_index = aci->devnum;
	gb->board = gfxb;
	gfx_temp_bank_idx = aci->devnum;
	gb->gfxmem_bank = aci->addrbank;
	aci->addrbank = &gfx_temp_bank;

	device_add_hsync(gfxboard_hsync_handler);
	device_add_rethink(gfxboard_rethink);

	return true;
}

static void vga_update_size_ext(struct rtggfxboard *gb)
{
	if (!gb->pcemdev) {
		// this forces qemu_console_resize() call
		gb->vga.vga.graphic_mode = -1;
		gb->vga.vga.monid = gb->monitor_id;
		gb->vga.vga.hw_ops->gfx_update(&gb->vga);
	}
}

static void gfxboard_set_fullrefresh(struct rtggfxboard *gb, int cnt)
{
	gb->fullrefresh = cnt;
	if (gb->func) {
		gb->func->refresh(gb->userdata);
	}
}

static bool gfxboard_setmode_ext(struct rtggfxboard *gb)
{
	int bpp;

	if (gb->pcemdev) {
		bpp = 32;
	} else {
		bpp = gb->vga.vga.get_bpp(&gb->vga.vga);
		if (bpp == 0)
			bpp = 8;
		vga_update_size_ext(gb);
	}
	if (gb->vga_width <= 16 || gb->vga_height <= 16)
		return false;
	struct gfxboard_mode mode;
	mode.width = gb->vga_width;
	mode.height = gb->vga_height;
	mode.hlinedbl = gb->vga_width_mult ? gb->vga_width_mult : 1;
	mode.vlinedbl = gb->vga_height_mult ? gb->vga_height_mult : 1;
	mode.mode = RGBFB_NONE;
	for (int i = 0; i < RGBFB_MaxFormats; i++) {
		RGBFTYPE t = (RGBFTYPE)i;
		if (GetBytesPerPixel(t) == bpp / 8) {
			mode.mode = t;
			break;
		}
	}
	gfxboard_setmode(gb, &mode);
	gfx_set_picasso_modeinfo(gb->monitor_id, mode.mode);
	gfxboard_set_fullrefresh(gb, 2);
	return true;
}

bool gfxboard_set(int monid, bool rtg)
{
	bool r;
	struct amigadisplay *ad = &adisplays[monid];
	r = ad->picasso_on;
	if (rtg) {
		ad->picasso_requested_on = 1;
	} else {
		ad->picasso_requested_on = 0;
	}
	set_config_changed();
	return r;
}

void gfxboard_rtg_disable(int monid, int index)
{
	if (monid > 0)
		return;
	if (index == rtg_visible[monid] && rtg_visible[monid] >= 0) {
		struct rtggfxboard *gb = &rtggfxboards[index];
		if (rtg_visible[monid] >= 0 && gb->func) {
			gb->func->toggle(gb->userdata, 0);
		}
		rtg_visible[monid] = -1;
	}
}

bool gfxboard_rtg_enable_initial(int monid, int index)
{
	struct amigadisplay *ad = &adisplays[monid];
	// if some RTG already enabled and located in monitor 0, don't override
	if ((rtg_visible[monid] >= 0 || rtg_initial[monid] >= 0) && !monid)
		return false;
	if (ad->picasso_on)
		return false;
	rtg_initial[monid] = index;
	gfxboard_toggle(monid, index, false);
	// check_prefs_picasso() calls gfxboard_toggle when ready
	return true;
}

bool gfxboard_switch_away(int monid)
{
	if (!monid) {
		struct amigadisplay *ad = &adisplays[monid];
		if (ad->picasso_requested_on) {
			ad->picasso_requested_on = false;
			set_config_changed();
		}
		return true;
	}
	return false;
}

int gfxboard_toggle(int monid, int index, int log)
{
	bool initial = false;
	struct rtggfxboard *gb;

	if (rtg_visible[monid] < 0 && rtg_initial[monid] >= 0 && rtg_initial[monid] < MAX_RTG_BOARDS) {
		index = rtg_initial[monid];
		initial = true;
	}

	gfxboard_rtg_disable(monid, rtg_visible[monid]);

	rtg_visible[monid] = -1;

	if (index < 0)
		goto end;

	gb = &rtggfxboards[index];
	if (!gb->active)
		goto end;

	if (gb->func) {
		bool r = gb->func->toggle(gb->userdata, 1);
		if (r) {
			rtg_initial[monid] = MAX_RTG_BOARDS;
			rtg_visible[monid] = gb->rtg_index;
			if (log && !monid)
				statusline_add_message(STATUSTYPE_DISPLAY, _T("RTG %d: %s"), index + 1, gb->board->name);
			return index;
		}
		goto end;
	}

	if (gb->vram == NULL)
		return -1;
	vga_update_size_ext(gb);
	if (gb->vga_width > 16 && gb->vga_height > 16) {
		if (!gfxboard_setmode_ext(gb))
			goto end;
		rtg_initial[monid] = MAX_RTG_BOARDS;
		rtg_visible[monid] = gb->rtg_index;
		gb->monswitch_new = true;
		gb->monswitch_delay = 1;
		if (log && !monid)
			statusline_add_message(STATUSTYPE_DISPLAY, _T("RTG %d: %s"), index + 1, gb->board->name);
		return index;
	}
end:
	if (initial) {
		rtg_initial[monid] = -1;
		return -2;
	}
	return -1;
}

static bool gfxboard_checkchanged(struct rtggfxboard *gb)
{
	struct picasso96_state_struct *state = &picasso96_state[gb->monitor_id];
	int bpp;
	if (gb->pcemdev) {
		bpp = 32;
	} else {
		bpp = gb->vga.vga.get_bpp(&gb->vga.vga);
		if (bpp == 0)
			bpp = 8;
	}
	if (gb->vga_width <= 16 || gb->vga_height <= 16)
		return false;
	if (state->Width != gb->vga_width ||
		state->Height != gb->vga_height ||
		state->BytesPerPixel != bpp / 8)
		return true;
	return false;
}

DisplaySurface *qemu_console_surface(QemuConsole *con)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)con;
	return &gb->gfxsurface;
}

void gfxboard_resize(int width, int height, int hmult, int vmult, void *p)
{
	struct rtggfxboard *gb = (struct rtggfxboard *)p;
	if (width != gb->vga_width || gb->vga_height != height) {
		gb->resolutionchange = 5;
	}
	gb->vga_width = width;
	gb->vga_height = height;
	gb->vga_width_mult = hmult;
	gb->vga_height_mult = vmult;
}

void qemu_console_resize(QemuConsole *con, int width, int height)
{
	struct rtggfxboard *gb = (struct rtggfxboard *)con;
	if (width != gb->vga_width || gb->vga_height != height) {
		gb->resolutionchange = 5;
	}
	gb->vga_width = width;
	gb->vga_height = height;
	gb->vga_width_mult = 1;
	gb->vga_height_mult = 1;
}

void linear_memory_region_set_dirty(MemoryRegion *mr, hwaddr addr, hwaddr size)
{
}

void vga_memory_region_set_dirty(MemoryRegion *mr, hwaddr addr, hwaddr size)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (gb->vga.vga.graphic_mode != 1)
		return;
	if (!gb->fullrefresh) {
		gfxboard_set_fullrefresh(gb, 1);
	}
}

DisplaySurface* qemu_create_displaysurface_from(int width, int height, int bpp,
                                                int linesize, uint8_t *data,
                                                bool byteswap)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (data >= gb->vram && data < gb->vramend) {
			struct picasso96_state_struct *state = &picasso96_state[gb->monitor_id];
			state->XYOffset = (int)((gb->vram - data) + gfxmem_banks[gb->rtg_index]->start);
			gb->resolutionchange = 1;
			return &gb->fakesurface;
		}
	}
	return NULL;
}

int surface_bits_per_pixel(DisplaySurface *s)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)s->data;
	if (rtg_visible[gb->monitor_id] < 0)
		return 32;
	if (s == &gb->fakesurface)
		return 32;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[gb->monitor_id];
	return vidinfo->pixbytes * 8;
}
int surface_bytes_per_pixel(DisplaySurface *s)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)s->data;
	if (rtg_visible[gb->monitor_id] < 0)
		return 4;
	if (s == &gb->fakesurface)
		return 4;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[gb->monitor_id];
	return vidinfo->pixbytes;
}

int surface_stride(DisplaySurface *s)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)s->data;
	if (rtg_visible[gb->monitor_id] < 0)
		return 0;
	if (s == &gb->fakesurface || !gb->vga_refresh_active)
		return 0;
	if (gb->gfxboard_surface == NULL)
		gb->gfxboard_surface = gfx_lock_picasso(gb->monitor_id, false);
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[gb->monitor_id];
	return vidinfo->rowbytes;
}
uint8_t *surface_data(DisplaySurface *s)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)s->data;
	if (!gb)
		return NULL;
	if (rtg_visible[gb->monitor_id] < 0)
		return NULL;
	if (gb->resolutionchange)
		return NULL;
	if (s == &gb->fakesurface || !gb->vga_refresh_active)
		return gb->fakesurface_surface;
	if (gb->gfxboard_surface == NULL)
		gb->gfxboard_surface = gfx_lock_picasso(gb->monitor_id, false);
	return gb->gfxboard_surface;
}

void gfxboard_refresh(int monid)
{
	if (monid >= 0) {
		for (int i = 0; i < MAX_RTG_BOARDS; i++) {
			struct rtgboardconfig *rbc = &currprefs.rtgboards[i];
			if (rbc->monitor_id == monid && rbc->rtgmem_size) {
				if (rbc->rtgmem_type >= GFXBOARD_HARDWARE) {
					struct rtggfxboard *gb = &rtggfxboards[i];
					gfxboard_set_fullrefresh(gb, 2);
				} else {
					picasso_refresh(monid);
				}
			}
		}
	} else {
		for (int i = 0; i < MAX_RTG_BOARDS; i++) {
			struct rtgboardconfig *rbc = &currprefs.rtgboards[i];
			if (rbc->rtgmem_size) {
				gfxboard_refresh(rbc->monitor_id);
			}
		}
	}
}

static void set_monswitch(struct rtggfxboard *gb, bool newval)
{
	if (gb->monswitch_new == newval)
		return;
	gb->monswitch_new = newval;
	gb->monswitch_delay = MONITOR_SWITCH_DELAY;
}

void gfxboard_intreq(void *p, int act, bool safe)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)p;
	if (act) {
		if (gb->board->irq && gb->gfxboard_intena) {
			gb->gfxboard_intreq_status = true;
			if (gb->board->irq > 0) {
				gb->gfxboard_intreq = 1;
				gb->gfxboard_intreq_marked = 1;
			} else {
				gb->pcibs->irq_callback(gb->pcibs, true);
			}
		}
	} else {
		gb->gfxboard_intreq = 0;
		gb->gfxboard_intreq_status = false;
		if (gb->board->irq < 0) {
			gb->pcibs->irq_callback(gb->pcibs, false);
		}
	}
	gfxboard_rethink();
}

void gfxboard_vsync_handler(bool full_redraw_required, bool redraw_required)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		struct amigadisplay *ad = &adisplays[gb->monitor_id];
		struct picasso96_state_struct *state = &picasso96_state[gb->monitor_id];

		if (gb->func) {

			if (gb->userdata) {
				struct gfxboard_mode mode = { 0 };
				mode.redraw_required = full_redraw_required;
				gb->func->vsync(gb->userdata, &mode);
				if (mode.mode && mode.width && mode.height) {
					if (state->Width != mode.width ||
						state->Height != mode.height ||
						state->RGBFormat != mode.mode ||
						!ad->picasso_on) {
						if (mode.width && mode.height && mode.mode) {
							gfxboard_setmode(gb, &mode);
							gfx_set_picasso_modeinfo(gb->monitor_id, mode.mode);
						}
					}
				}
			}

		} else 	if (gb->configured_mem > 0 && gb->configured_regs > 0) {

			if (gb->pcemdev) {
#ifdef USE_PCEM
				static int fcount;
				if (!gb->pcem_vblank && gb->pcemobject2) {
					int max = svga_get_vtotal(gb->pcemobject2);
					while (max-- > 0) {
						if (gfxboard_pcem_poll(gb))
							break;
					}
				}
				gb->pcem_vblank = 0;
				extern int console_logging;
#if 0
				if (console_logging) {
					fcount++;
					if ((fcount % 50) == 0) {
						if (gb->pcemobject && gb->pcemdev->add_status_info) {
							char txt[1024];
							txt[0] = 0;
							gb->pcemdev->add_status_info(txt, sizeof(txt), gb->pcemobject);
							TCHAR *s = au(txt);
							write_log(_T("%s"), s);
							xfree(s);
						}
					}
				}
#endif
				if ((!gb->board->hasswitcher && gb->rbc->autoswitch) && gb->vram) {
					bool svga_on(void *p);
					bool on = svga_on(gb->pcemobject2);
					set_monswitch(gb, on);
				}
#endif
			}

			gfxboard_unlock(gb);

			if (gb->monswitch_keep_trying) {
				vga_update_size_ext(gb);
				if (gb->vga_width > 16 && gb->vga_height > 16) {
					gb->monswitch_keep_trying = false;
					gb->monswitch_new = true;
					gb->monswitch_delay = 0;
				}
			}

			if (gb->monswitch_new != gb->monswitch_current) {
				if (gb->monswitch_delay > 0)
					gb->monswitch_delay--;
				if (gb->monswitch_delay == 0) {
					if (!gb->monswitch_new && rtg_visible[gb->monitor_id] == i) {
						gfxboard_rtg_disable(gb->monitor_id, i);
					}
					gb->monswitch_current = gb->monswitch_new;
					vga_update_size_ext(gb);
					write_log(_T("GFXBOARD %d MONITOR=%d ACTIVE=%d\n"), i, gb->monitor_id, gb->monswitch_current);
					if (gb->monitor_id > 0) {
						if (gb->monswitch_new)
							gfxboard_toggle(gb->monitor_id, i, 0);
					} else {
						if (gb->monswitch_current) {
							if (!gfxboard_rtg_enable_initial(gb->monitor_id, i)) {
								// Nothing visible? Re-enable our display.
								if (rtg_visible[gb->monitor_id] < 0) {
									gfxboard_toggle(gb->monitor_id, i, 0);
								}
							}
						} else {
							gfxboard_switch_away(gb->monitor_id);
						}
					}
				}
			} else {
				gb->monswitch_delay = 0;
			}
			// Vertical Sync End Register, 0x20 = Disable Vertical Interrupt, 0x10 = Clear Vertical Interrupt.
			if (!gb->pcemdev && gb->board->irq) {
				if ((!(gb->vga.vga.cr[0x11] & 0x20) && (gb->vga.vga.cr[0x11] & 0x10) && !(gb->vga.vga.gr[0x17] & 4))) {
					if (gb->gfxboard_intena) {
						gb->gfxboard_intreq = true;
						//write_log(_T("VGA interrupt %d\n"), gb->board->irq);
						if (gb->board->irq == 2)
							INTREQ(0x8000 | 0x0008);
						else
							INTREQ(0x8000 | 0x2000);
					}
				}
			}
		}
	}

	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		struct amigadisplay *ad = &adisplays[gb->monitor_id];
		struct picasso96_state_struct *state = &picasso96_state[gb->monitor_id];

		if (gb->monitor_id == 0) {
			// if default monitor: show rtg_visible
			if (rtg_visible[gb->monitor_id] < 0)
				continue;
			if (i != rtg_visible[gb->monitor_id])
				continue;
		}

		if (gb->configured_mem <= 0 || gb->configured_regs <= 0)
			continue;

		if (gb->monswitch_current && gb->resolutionchange) {
			if (gb->resolutionchange == 1) {
				gb->resolutionchange = 0;
				if (gfxboard_checkchanged(gb)) {
					if (!gfxboard_setmode_ext(gb)) {
						gfxboard_rtg_disable(gb->monitor_id, i);
					} else {
						state->ModeChanged = true;
					}
				}
			} else if (gb->resolutionchange > 1) {
				gb->resolutionchange--;
			}
			continue;
		}

		if (!redraw_required)
			continue;

		if (!gb->monswitch_delay && gb->monswitch_current && ad->picasso_on && ad->picasso_requested_on && !gb->resolutionchange) {
			if (!gb->pcemdev) {
				if (picasso_getwritewatch(i, gb->vram_start_offset, NULL, NULL) < 0) {
					gb->fullrefresh = 1;
				}
				if (gb->fullrefresh)
					gb->vga.vga.graphic_mode = -1;
				gb->vga_refresh_active = true;
				gb->vga.vga.hw_ops->gfx_update(&gb->vga);
				gb->vga_refresh_active = false;
			} else {
				if (gb->pcemobject) {
#ifdef USE_PCEM
					if (gb->fullrefresh) {
						gb->pcemdev->force_redraw(gb->pcemobject);
					} else {
						pcem_flush(gb, i);
					}
#endif
				}
			}
		}

		if (ad->picasso_on && !gb->resolutionchange) {
			if (gb->fullrefresh > 0)
				gb->fullrefresh--;
		}
		gfxboard_unlock(gb);
	}
}

double gfxboard_get_vsync (void)
{
	return vblank_hz; // FIXME
}

void dpy_gfx_update(QemuConsole *con, int x, int y, int w, int h)
{
	picasso_invalidate(0, x, y, w, h);
}

void memory_region_init_alias(MemoryRegion *mr,
                              const char *name,
                              MemoryRegion *orig,
                              hwaddr offset,
                              uint64_t size)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (!stricmp(name, "vga.bank0")) {
		mr->opaque = &gb->vgabank0regionptr;
	} else if (!stricmp(name, "vga.bank1")) {
		mr->opaque = &gb->vgabank1regionptr;
	}
}

static void jit_reset (void)
{
#ifdef JIT
	if (currprefs.cachesize && (!currprefs.comptrustbyte || !currprefs.comptrustword || !currprefs.comptrustlong)) {
		flush_icache (3);
	}
#endif
}

static void remap_vram (struct rtggfxboard *gb, hwaddr offset0, hwaddr offset1, bool enabled)
{
	jit_reset ();
	gb->vram_offset[0] = offset0;
	gb->vram_offset[1] = offset1;
#if VRAMLOG
	if (gb->vram_enabled != enabled)
		write_log (_T("VRAM state=%d\n"), enabled);
	bool was_vram_offset_enabled = gb->vram_offset_enabled;
#endif
	gb->vram_enabled = enabled && (gb->vga.vga.sr[0x07] & 0x01);
#if 0
	vram_enabled = false;
#endif
	// offset==0 and offset1==0x8000: linear vram mapping
	gb->vram_offset_enabled = offset0 != 0 || offset1 != 0x8000;
#if VRAMLOG
	if (gb->vram_offset_enabled || was_vram_offset_enabled)
		write_log (_T("VRAM offset %08x and %08x\n"), offset0, offset1);
#endif
}

void memory_region_set_alias_offset(MemoryRegion *mr, hwaddr offset)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (mr->opaque == &gb->vgabank0regionptr) {
		if (offset != gb->vram_offset[0]) {
			//write_log (_T("vgavramregion0 %08x\n"), offset);
			remap_vram (gb, offset, gb->vram_offset[1], gb->vram_enabled);
		}
	} else if (mr->opaque == &gb->vgabank1regionptr) {
		if (offset != gb->vram_offset[1]) {
			//write_log (_T("vgavramregion1 %08x\n"), offset);
			remap_vram (gb, gb->vram_offset[0], offset, gb->vram_enabled);
		}
	} else if (mr->opaque == &gb->vgaioregionptr) {
		write_log (_T("vgaioregion %d\n"), offset);
	} else {
		write_log (_T("unknown region %d\n"), offset);
	}

}
void memory_region_set_enabled(MemoryRegion *mr, bool enabled)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (mr->opaque == &gb->vgabank0regionptr || mr->opaque == &gb->vgabank1regionptr) {
		if (enabled != gb->vram_enabled)  {
			//write_log (_T("enable vgavramregion %d\n"), enabled);
			remap_vram (gb, gb->vram_offset[0], gb->vram_offset[1], enabled);
		}
	} else if (mr->opaque == &gb->vgaioregionptr) {
		write_log (_T("enable vgaioregion %d\n"), enabled);
	} else {
		write_log (_T("enable unknown region %d\n"), enabled);
	}
}
void memory_region_reset_dirty(MemoryRegion *mr, hwaddr addr,
                               hwaddr size, unsigned client)
{
	//write_log (_T("memory_region_reset_dirty %08x %08x\n"), addr, size);
}
bool memory_region_get_dirty(MemoryRegion *mr, hwaddr addr,
                             hwaddr size, unsigned client)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (mr->opaque != &gb->vgavramregionptr)
		return false;
	//write_log (_T("memory_region_get_dirty %08x %08x\n"), addr, size);
	if (gb->fullrefresh)
		return true;
	return picasso_is_vram_dirty (gb->rtg_index, addr + gb->gfxmem_bank->start, size);
}

static QEMUResetHandler *reset_func;
static void *reset_parm;
void qemu_register_reset(QEMUResetHandler *func, void *opaque)
{
	reset_func = func;
	reset_parm = opaque;
	reset_func (reset_parm);
}

static void p4_pci_check (struct rtggfxboard *gb)
{
	uaecptr b0, b1;

	b0 = gb->p4_pci[0x10 + 2] << 16;
	b1 = gb->p4_pci[0x14 + 2] << 16;

	gb->p4_vram_bank[0] = b0;
	gb->p4_vram_bank[1] = b1;

//	write_log (_T("%08X %08X\n"), gb->p4_vram_bank[0], gb->p4_vram_bank[1]);
}

static void reset_pci (struct rtggfxboard *gb)
{
	gb->cirrus_pci[0] = 0x00;
	gb->cirrus_pci[1] = 0xb8;
	gb->cirrus_pci[2] = 0x10;
	gb->cirrus_pci[3] = 0x13;

	gb->cirrus_pci[4] = 2;
	gb->cirrus_pci[5] = 0;
	gb->cirrus_pci[6] = 0;
	gb->cirrus_pci[7] &= ~(1 | 2 | 32);

	gb->cirrus_pci[8] = 3;
	gb->cirrus_pci[9] = 0;
	gb->cirrus_pci[10] = 0;
	gb->cirrus_pci[11] = 68;

	gb->cirrus_pci[0x10] &= ~1; // B revision
	gb->cirrus_pci[0x13] &= ~1; // memory

	gb->p4i2c = 0xff;
}

static void picassoiv_checkswitch (struct rtggfxboard *gb)
{
	if (ISP4()) {
		uae_u8 v = 0;
		bool rtg_active = (gb->picassoiv_flifi & 1) == 0;
		if (gb->pcemdev) {
#ifdef USE_PCEM
			if (gb->pcemobject) {
				extern uae_u8 pcem_read_io(int, int);
				v = pcem_read_io(0x3d5, 0x51);
			}
#endif
		} else {
			v = gb->vga.vga.cr[0x51];
		}
		if (!(v & 8))
			rtg_active = true;
		// do not switch to P4 RTG until monitor switch is set to native at least
		// once after reset.
		if (gb->monswitch_reset && rtg_active)
			return;
		set_monswitch(gb, rtg_active);
		gb->monswitch_reset = false;
	}
}

static void bput_regtest (struct rtggfxboard *gb, uaecptr addr, uae_u8 v)
{
	addr += 0x3b0;
	if (addr == 0x3d5) { // CRxx
		if (gb->vga.vga.cr_index == 0x11) {
			if (!(gb->vga.vga.cr[0x11] & 0x10)) {
				gb->gfxboard_intreq = false;
			}
		}
	}
	if (!(gb->vga.vga.sr[0x07] & 0x01) && gb->vram_enabled) {
		remap_vram (gb, gb->vram_offset[0], gb->vram_offset[1], false);
	}
	picassoiv_checkswitch (gb);
}

static uae_u8 bget_regtest (struct rtggfxboard *gb, uaecptr addr, uae_u8 v)
{
	addr += 0x3b0;
	// Input Status 0
	if (addr == 0x3c2) {
		if (gb->gfxboard_intreq) {
			// Disable Vertical Interrupt == 0?
			// Clear Vertical Interrupt == 1
			// GR17 bit 2 = INTR disable
			if (!(gb->vga.vga.cr[0x11] & 0x20) && (gb->vga.vga.cr[0x11] & 0x10) && !(gb->vga.vga.gr[0x17] & 4)) {
				v |= 0x80; // VGA Interrupt Pending
			}
		}
		v |= 0x10; // DAC sensing
	}
	if (addr == 0x3c5) {
		if (gb->vga.vga.sr_index == 8) {
			// TODO: DDC
		}
	}
	return v;
}

void vga_io_put(int board, int portnum, uae_u8 v)
{
	struct rtggfxboard *gb = &rtggfxboards[board];
	if (!gb->vgaio)
		return;
	portnum -= 0x3b0;
	bput_regtest(gb, portnum, v);
	gb->vgaio->write(&gb->vga, portnum, v, 1);
}
uae_u8 vga_io_get(int board, int portnum)
{
	struct rtggfxboard *gb = &rtggfxboards[board];
	uae_u8 v = 0xff;
	if (!gb->vgaio)
		return v;
	portnum -= 0x3b0;
	v = (uae_u8)gb->vgaio->read(&gb->vga, portnum, 1);
	v = bget_regtest(gb, portnum, v);
	return v;
}
void vga_ram_put(int board, int offset, uae_u8 v)
{
	struct rtggfxboard *gb = &rtggfxboards[board];
	if (!gb->vgalowram)
		return;
	offset -= 0xa0000;
	gb->vgalowram->write(&gb->vga, offset, v, 1);
}
uae_u8 vga_ram_get(int board, int offset)
{
	struct rtggfxboard *gb = &rtggfxboards[board];
	if (!gb->vgalowram)
		return 0xff;
	offset -= 0xa0000;
	return (uae_u8)gb->vgalowram->read(&gb->vga, offset, 1);
}
void vgalfb_ram_put(int board, int offset, uae_u8 v)
{
	struct rtggfxboard *gb = &rtggfxboards[board];
	if (!gb->vgaram)
		return;
	gb->vgaram->write(&gb->vga, offset, v, 1);
}
uae_u8 vgalfb_ram_get(int board, int offset)
{
	struct rtggfxboard *gb = &rtggfxboards[board];
	if (!gb->vgaram)
		return 0xff;
	return (uae_u8)gb->vgaram->read(&gb->vga, offset, 1);
}

void *memory_region_get_ram_ptr(MemoryRegion *mr)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (mr->opaque == &gb->vgavramregionptr)
		return gb->vram;
	return NULL;
}

void memory_region_init_ram(MemoryRegion *mr,
                            const char *name,
                            uint64_t size)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (!stricmp (name, "vga.vram")) {
		gb->vgavramregion.opaque = mr->opaque;
	}
}

void memory_region_init_io(MemoryRegion *mr,
                           const MemoryRegionOps *ops,
                           void *opaque,
                           const char *name,
                           uint64_t size)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)mr->data;
	if (!stricmp (name, "cirrus-io")) {
		gb->vgaio = ops;
		mr->opaque = gb->vgaioregion.opaque;
	} else if (!stricmp (name, "cirrus-linear-io")) {
		gb->vgaram = ops;
	} else if (!stricmp (name, "cirrus-low-memory")) {
		gb->vgalowram = ops;
	} else if (!stricmp (name, "cirrus-mmio")) {
		gb->vgammio = ops;
	}
}

int is_surface_bgr(DisplaySurface *surface)
{
	if (!surface || !surface->data)
		return 0;
	struct rtggfxboard *gb = (struct rtggfxboard*)surface->data;
	return gb->board->swap;
}

static uaecptr fixaddr_bs (struct rtggfxboard *gb, uaecptr addr, int mask, int *bs)
{
	bool swapped = false;
	addr &= gb->gfxmem_bank->mask;
	if (gb->p4z2) {
		if (addr < 0x200000) {
			addr |= gb->p4_vram_bank[0];
			if (addr >= 0x400000 && addr < 0x600000) {
				*bs = BYTESWAP_WORD;
				swapped = true;
			} else if (addr >= 0x800000 && addr < 0xa00000) {
				*bs = BYTESWAP_LONG;
				swapped = true;
			}
		} else {
			addr |= gb->p4_vram_bank[1];
			if (addr >= 0x600000 && addr < 0x800000) {
				*bs = BYTESWAP_WORD;
				swapped = true;
			} else if (addr >= 0xa00000 && addr < 0xc00000) {
				*bs = BYTESWAP_LONG;
				swapped = true;
			}
		}
	}
#ifdef JIT
	if (mask && (gb->vram_offset_enabled || !gb->vram_enabled || swapped || gb->p4z2))
		special_mem |= mask;
#endif
	if (gb->vram_offset_enabled) {
		if (addr & 0x8000) {
			addr += gb->vram_offset[1] & ~0x8000;
		} else {
			addr += gb->vram_offset[0];
		}
	}
	addr &= gb->gfxmem_bank->mask;
	return addr;
}

STATIC_INLINE uaecptr fixaddr (struct rtggfxboard *gb, uaecptr addr, int mask)
{
#ifdef JIT
	if (mask && (gb->vram_offset_enabled || !gb->vram_enabled))
		special_mem |= mask;
#endif
	if (gb->vram_offset_enabled) {
		if (addr & 0x8000) {
			addr += gb->vram_offset[1] & ~0x8000;
		} else {
			addr += gb->vram_offset[0];
		}
	}
	addr &= gb->gfxmem_bank->mask;
	return addr;
}

STATIC_INLINE uaecptr fixaddr (struct rtggfxboard *gb, uaecptr addr)
{
	if (gb->vram_offset_enabled) {
		if (addr & 0x8000) {
			addr += gb->vram_offset[1] & ~0x8000;
		} else {
			addr += gb->vram_offset[0];
		}
	}
	addr &= gb->gfxmem_bank->mask;
	return addr;
}

STATIC_INLINE const MemoryRegionOps *getvgabank (struct rtggfxboard *gb, uaecptr *paddr)
{
	uaecptr addr = *paddr;
	addr &= gb->gfxmem_bank->mask;
	*paddr = addr;
	return gb->vgaram;
}

static uae_u32 gfxboard_lget_vram (struct rtggfxboard *gb, uaecptr addr, int bs)
{
	uae_u32 v;
	if (!gb->vram_enabled) {
		const MemoryRegionOps *bank = getvgabank(gb, &addr);
		if (bs < 0) { // WORD
			v  = ((uae_u8)bank->read(&gb->vga, addr + 1, 1)) << 24;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 0, 1)) << 16;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 3, 1)) <<  8;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 2, 1)) <<  0;
		} else if (bs > 0) { // LONG
			v  = ((uae_u8)bank->read(&gb->vga, addr + 3, 1)) << 24;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 2, 1)) << 16;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 1, 1)) <<  8;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 0, 1)) <<  0;
		} else {
			v  = ((uae_u8)bank->read(&gb->vga, addr + 0, 1)) << 24;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 1, 1)) << 16;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 2, 1)) <<  8;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 3, 1)) <<  0;
		}
	} else {
		uae_u8 *m = gb->vram + addr;
		if (bs < 0) {
			v  = (*((uae_u16*)(m + 0))) << 16;
			v |= (*((uae_u16*)(m + 2))) << 0;
		} else if (bs > 0) {
			v = *((uae_u32*)m);
		} else {
			v = do_get_mem_long((uae_u32*)m);
		}
	}
#if MEMLOGR
#if MEMLOGINDIRECT
	if (!vram_enabled || vram_offset_enabled)
#endif
	if (memlogr)
		write_log (_T("R %08X L %08X BS=%d EN=%d\n"), addr, v, bs, gb->vram_enabled);
#endif
	return v;
}
static uae_u16 gfxboard_wget_vram (struct rtggfxboard *gb, uaecptr addr, int bs)
{
	uae_u32 v;
	if (!gb->vram_enabled) {
		const MemoryRegionOps *bank = getvgabank (gb, &addr);
		if (bs) {
			v  = ((uae_u8)bank->read(&gb->vga, addr + 0, 1)) <<  0;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 1, 1)) <<  8;
		} else {
			v  = ((uae_u8)bank->read(&gb->vga, addr + 0, 1)) <<  8;
			v |= ((uae_u8)bank->read(&gb->vga, addr + 1, 1)) <<  0;
		}
	} else {
		uae_u8 *m = gb->vram + addr;
		if (bs)
			v = *((uae_u16*)m);
		else
			v = do_get_mem_word ((uae_u16*)m);
	}
#if MEMLOGR
#if MEMLOGINDIRECT
	if (!vram_enabled || vram_offset_enabled)
#endif
	if (memlogr)
		write_log (_T("R %08X W %08X BS=%d EN=%d\n"), addr, v, bs, gb->vram_enabled);
#endif
	return v;
}
static uae_u8 gfxboard_bget_vram (struct rtggfxboard *gb, uaecptr addr, int bs)
{
	uae_u32 v;
	if (!gb->vram_enabled) {
		const MemoryRegionOps *bank = getvgabank (gb, &addr);
		if (bs)
			v = (uae_u8)bank->read(&gb->vga, addr ^ 1, 1);
		else
			v = (uae_u8)bank->read(&gb->vga, addr + 0, 1);
	} else {
		if (bs)
			v = gb->vram[addr ^ 1];
		else
			v = gb->vram[addr];
	}
#if MEMLOGR
#if MEMLOGINDIRECT
	if (!vram_enabled || vram_offset_enabled)
#endif
	if (memlogr)
		write_log (_T("R %08X B %08X BS=0 EN=%d\n"), addr, v, gb->vram_enabled);
#endif
	return v;
}

static void gfxboard_lput_vram (struct rtggfxboard *gb, uaecptr addr, uae_u32 l, int bs)
{
#if MEMDEBUG
	if ((addr & MEMDEBUGMASK) >= MEMDEBUGTEST && (MEMDEBUGCLEAR || l))
		write_log (_T("%08X L %08X %08X\n"), addr, l, M68K_GETPC);
#endif
#if MEMLOGW
#if MEMLOGINDIRECT
	if (!vram_enabled || vram_offset_enabled)
#endif
	if (memlogw)
		write_log (_T("W %08X L %08X\n"), addr, l);
#endif
	if (!gb->vram_enabled) {
		const MemoryRegionOps *bank = getvgabank (gb, &addr);
		if (bs < 0) { // WORD
			bank->write (&gb->vga, addr + 1, l >> 24, 1);
			bank->write (&gb->vga, addr + 0, (l >> 16) & 0xff, 1);
			bank->write (&gb->vga, addr + 3, (l >> 8) & 0xff, 1);
			bank->write (&gb->vga, addr + 2, (l >> 0) & 0xff, 1);
		} else if (bs > 0) { // LONG
			bank->write (&gb->vga, addr + 3, l >> 24, 1);
			bank->write (&gb->vga, addr + 2, (l >> 16) & 0xff, 1);
			bank->write (&gb->vga, addr + 1, (l >> 8) & 0xff, 1);
			bank->write (&gb->vga, addr + 0, (l >> 0) & 0xff, 1);
		} else {
			bank->write (&gb->vga, addr + 0, l >> 24, 1);
			bank->write (&gb->vga, addr + 1, (l >> 16) & 0xff, 1);
			bank->write (&gb->vga, addr + 2, (l >> 8) & 0xff, 1);
			bank->write (&gb->vga, addr + 3, (l >> 0) & 0xff, 1);
		}
	} else {
		uae_u8 *m = gb->vram + addr;
		if (bs < 0) {
			*((uae_u16*)(m + 0)) = l >> 16;
			*((uae_u16*)(m + 2)) = l >>  0;
		} else if (bs > 0) {
			*((uae_u32*)m) = l;
		} else {
			do_put_mem_long ((uae_u32*) m, l);
		}
	}
}
static void gfxboard_wput_vram (struct rtggfxboard *gb, uaecptr addr, uae_u16 w, int bs)
{
#if MEMDEBUG
	if ((addr & MEMDEBUGMASK) >= MEMDEBUGTEST && (MEMDEBUGCLEAR || w))
		write_log (_T("%08X W %04X %08X\n"), addr, w & 0xffff, M68K_GETPC);
#endif
#if MEMLOGW
#if MEMLOGINDIRECT
	if (!vram_enabled || vram_offset_enabled)
#endif
	if (memlogw)
		write_log (_T("W %08X W %04X\n"), addr, w & 0xffff);
#endif
	if (!gb->vram_enabled) {
		const MemoryRegionOps *bank = getvgabank (gb, &addr);
		if (bs) {
			bank->write(&gb->vga, addr + 0, (w >> 0) & 0xff, 1);
			bank->write(&gb->vga, addr + 1, w >> 8, 1);
		} else {
			bank->write(&gb->vga, addr + 0, w >> 8, 1);
			bank->write(&gb->vga, addr + 1, (w >> 0) & 0xff, 1);
		}
	} else {
		uae_u8 *m = gb->vram + addr;
		if (bs)
			*((uae_u16*)m) = w;
		else
			do_put_mem_word ((uae_u16*)m, w);
	}
}
static void gfxboard_bput_vram (struct rtggfxboard *gb, uaecptr addr, uae_u8 b, int bs)
{
#if MEMDEBUG
	if ((addr & MEMDEBUGMASK) >= MEMDEBUGTEST && (MEMDEBUGCLEAR || b))
		write_log (_T("%08X B %02X %08X\n"), addr, b & 0xff, M68K_GETPC);
#endif
#if MEMLOGW
#if MEMLOGINDIRECT
	if (!vram_enabled || vram_offset_enabled)
#endif
	if (memlogw)
		write_log (_T("W %08X B %02X\n"), addr, b & 0xff);
#endif
	if (!gb->vram_enabled) {
		const MemoryRegionOps *bank = getvgabank (gb, &addr);
#ifdef JIT
		special_mem |= S_WRITE;
#endif
		if (bs)
			bank->write(&gb->vga, addr ^ 1, b, 1);
		else
			bank->write(&gb->vga, addr, b, 1);
	} else {
		if (bs)
			gb->vram[addr ^ 1] = b;
		else
			gb->vram[addr] = b;
	}
}

static struct rtggfxboard *lastgetgfxboard;
static rtggfxboard *getgfxboard(uaecptr addr)
{
#ifdef JIT
	special_mem = S_WRITE | S_READ;
#endif
	if (only_gfx_board)
		return only_gfx_board;
	if (lastgetgfxboard) {
		if (addr >= lastgetgfxboard->mem_start[0] && addr < lastgetgfxboard->mem_end[0])
			return lastgetgfxboard;
		if (addr >= lastgetgfxboard->io_start && addr < lastgetgfxboard->io_end)
			return lastgetgfxboard;
		if (addr >= lastgetgfxboard->mem_start[1] && addr < lastgetgfxboard->mem_end[1])
			return lastgetgfxboard;
		lastgetgfxboard = NULL;
	}
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (!gb->active)
			continue;
		if (gb->configured_mem < 0 || gb->configured_regs < 0) {
			lastgetgfxboard = gb;
			return gb;
		} else {
			if (addr >= gb->io_start && addr < gb->io_end) {
				lastgetgfxboard = gb;
				return gb;
			}
			if (addr >= gb->mem_start[0] && addr < gb->mem_end[0]) {
				lastgetgfxboard = gb;
				return gb;
			}
			if (addr >= gb->mem_start[1] && addr < gb->mem_end[1]) {
				lastgetgfxboard = gb;
				return gb;
			}
		}
	}
	return NULL;
}

// LONG byteswapped VRAM
static uae_u32 REGPARAM2 gfxboard_lget_lbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return 0;
	return gfxboard_lget_vram (gb, addr, BYTESWAP_LONG);
}
static uae_u32 REGPARAM2 gfxboard_wget_lbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return 0;
	return gfxboard_wget_vram (gb, addr, BYTESWAP_LONG);
}
static uae_u32 REGPARAM2 gfxboard_bget_lbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return 0;
	return gfxboard_bget_vram (gb, addr, BYTESWAP_LONG);
}

static void REGPARAM2 gfxboard_lput_lbsmem (uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return;
	gfxboard_lput_vram (gb, addr, l, BYTESWAP_LONG);
}
static void REGPARAM2 gfxboard_wput_lbsmem (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return;
	gfxboard_wput_vram (gb, addr, w, BYTESWAP_LONG);
}
static void REGPARAM2 gfxboard_bput_lbsmem (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return;
	gfxboard_bput_vram (gb, addr, w, BYTESWAP_LONG);
}

// WORD byteswapped VRAM
static uae_u32 REGPARAM2 gfxboard_lget_wbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return 0;
	return gfxboard_lget_vram (gb, addr, BYTESWAP_WORD);
}
static uae_u32 REGPARAM2 gfxboard_wget_wbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return 0;
	return gfxboard_wget_vram (gb, addr, BYTESWAP_WORD);
}
static uae_u32 REGPARAM2 gfxboard_bget_wbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return 0;
	return gfxboard_bget_vram (gb, addr, BYTESWAP_WORD);
}

static void REGPARAM2 gfxboard_lput_wbsmem (uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return;
	gfxboard_lput_vram (gb, addr, l, BYTESWAP_WORD);
}
static void REGPARAM2 gfxboard_wput_wbsmem (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return;
	gfxboard_wput_vram (gb, addr, w, BYTESWAP_WORD);
}
static void REGPARAM2 gfxboard_bput_wbsmem (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return;
	gfxboard_bput_vram (gb, addr, w, BYTESWAP_WORD);
}

// normal or byteswapped (banked) vram
static uae_u32 REGPARAM2 gfxboard_lget_nbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int bs = 0;
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr_bs (gb, addr, 0, &bs);
	if (addr == -1)
		return 0;
	return gfxboard_lget_vram (gb, addr, bs);
}
static uae_u32 REGPARAM2 gfxboard_wget_nbsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int bs = 0;
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr_bs (gb, addr, 0, &bs);
	if (addr == -1)
		return 0;
	return gfxboard_wget_vram (gb, addr, bs);
}

static void REGPARAM2 gfxboard_lput_nbsmem (uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int bs = 0;
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr_bs (gb, addr, 0, &bs);
	if (addr == -1)
		return;
	gfxboard_lput_vram (gb, addr, l, bs);
}
static void REGPARAM2 gfxboard_wput_nbsmem (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int bs = 0;
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr_bs (gb, addr, 0, &bs);
	if (addr == -1)
		return;
	gfxboard_wput_vram (gb, addr, w, bs);
}

static uae_u32 REGPARAM2 gfxboard_bget_bsmem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int bs = 0;
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, 0);
	if (addr == -1)
		return 0;
	return gfxboard_bget_vram (gb, addr, bs);
}
static void REGPARAM2 gfxboard_bput_bsmem (uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int bs = 0;
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr_bs (gb, addr, 0, &bs);
	if (addr == -1)
		return;
	gfxboard_bput_vram (gb, addr, b, bs);
}

// normal vram
static uae_u32 REGPARAM2 gfxboard_lget_mem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, S_READ);
	if (addr == -1)
		return 0;
	return gfxboard_lget_vram (gb, addr, 0);
}
static uae_u32 REGPARAM2 gfxboard_wget_mem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, S_READ);
	if (addr == -1)
		return 0;
	return gfxboard_wget_vram (gb, addr, 0);
}
static uae_u32 REGPARAM2 gfxboard_bget_mem (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, S_READ);
	if (addr == -1)
		return 0;
	return gfxboard_bget_vram (gb, addr, 0);
}
static void REGPARAM2 gfxboard_lput_mem (uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, S_WRITE);
	if (addr == -1)
		return;
	gfxboard_lput_vram (gb, addr, l, 0);
}
static void REGPARAM2 gfxboard_wput_mem (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, S_WRITE);
	if (addr == -1)
		return;
	gfxboard_wput_vram (gb, addr, w, 0);
}
static void REGPARAM2 gfxboard_bput_mem (uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr(gb, addr, S_WRITE);
	if (addr == -1)
		return;
	gfxboard_bput_vram (gb, addr, b, 0);
}

// normal vram, no jit direct
static uae_u32 REGPARAM2 gfxboard_lget_mem_nojit (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr (gb, addr);
	if (addr == -1)
		return 0;
	return gfxboard_lget_vram (gb, addr, 0);
}
static uae_u32 REGPARAM2 gfxboard_wget_mem_nojit (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr (gb, addr);
	if (addr == -1)
		return 0;
	return gfxboard_wget_vram (gb, addr, 0);
}
static uae_u32 REGPARAM2 gfxboard_bget_mem_nojit (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr (gb, addr);
	if (addr == -1)
		return 0;
	return gfxboard_bget_vram (gb, addr, 0);
}
static void REGPARAM2 gfxboard_lput_mem_nojit (uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr (gb, addr);
	if (addr == -1)
		return;
	gfxboard_lput_vram (gb, addr, l, 0);
}
static void REGPARAM2 gfxboard_wput_mem_nojit (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr (gb, addr);
	if (addr == -1)
		return;
	gfxboard_wput_vram (gb, addr, w, 0);
}
static void REGPARAM2 gfxboard_bput_mem_nojit (uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr = fixaddr (gb, addr);
	if (addr == -1)
		return;
	gfxboard_bput_vram (gb, addr, b, 0);
}

static int REGPARAM2 gfxboard_check (uaecptr addr, uae_u32 size)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr &= gb->gfxmem_bank->mask;
	return (addr + size) <= gb->rbc->rtgmem_size;
}
static uae_u8 *REGPARAM2 gfxboard_xlate (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr -= gb->gfxboardmem_start & gb->gfxmem_bank->mask;
	addr &= gb->gfxmem_bank->mask;
	return gb->vram + addr;
}

static uae_u32 REGPARAM2 gfxboard_bget_mem_autoconfig (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v = 0;
	addr &= 65535;
	if (addr < GFXBOARD_AUTOCONFIG_SIZE)
		v = gb->automemory[addr];
	return v;
}

static void copyvrambank(addrbank *dst, const addrbank *src, bool unsafe)
{
	dst->start = src->start;
	dst->startmask = src->startmask;
	dst->mask = src->mask;
	dst->allocated_size = src->allocated_size;
	dst->reserved_size = src->reserved_size;
	dst->baseaddr = src->baseaddr;
	dst->flags = src->flags;
	dst->jit_read_flag = src->jit_read_flag;
	dst->jit_write_flag = src->jit_write_flag;
	if (unsafe) {
		dst->jit_read_flag |= S_READ | S_N_ADDR;
		dst->jit_write_flag |= S_WRITE | S_N_ADDR;
		dst->flags |= ABFLAG_ALLOCINDIRECT | ABFLAG_PPCIOSPACE;
	}
}

static void REGPARAM2 gfxboard_wput_mem_autoconfig (uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	if (gb->board->configtype == 2)
		return;
	int boardnum = gb->rbc->rtgmem_type;
	b &= 0xffff;
	addr &= 65535;
	if (addr == 0x44) {
		uae_u32 start = expamem_board_pointer;
		if (!expamem_z3hack(&currprefs)) {
			start = ((b & 0xff00) | gb->expamem_lo) << 16;
		}
		gb->gfxboardmem_start = start;
		gb->gfxboard_bank_memory.bget = gfxboard_bget_mem;
		gb->gfxboard_bank_memory.bput = gfxboard_bput_mem;
		gb->gfxboard_bank_memory.wput = gfxboard_wput_mem;
		if (gb->board->pcemdev) {
			if (boardnum == GFXBOARD_ID_CV643D_Z3) {
				gb->gfxboardmem_start += 0x4000000;
			}
		}
		init_board(gb);
		copyvrambank(&gb->gfxboard_bank_memory, gb->gfxmem_bank, true);
		copyvrambank(&gb->gfxboard_bank_vram_pcem, gb->gfxmem_bank, true);
		if (ISP4() && !gb->board->pcemdev) {
			if (validate_banks_z3(&gb->gfxboard_bank_memory, gb->gfxmem_bank->start >> 16, expamem_board_size >> 16)) {
				// main vram
				map_banks_z3(&gb->gfxboard_bank_memory, (gb->gfxmem_bank->start + PICASSOIV_VRAM1) >> 16, 0x400000 >> 16);
				map_banks_z3(&gb->gfxboard_bank_wbsmemory, (gb->gfxmem_bank->start + PICASSOIV_VRAM1 + 0x400000) >> 16, 0x400000 >> 16);
				// secondary
				map_banks_z3(&gb->gfxboard_bank_memory_nojit, (gb->gfxmem_bank->start + PICASSOIV_VRAM2) >> 16, 0x400000 >> 16);
				map_banks_z3(&gb->gfxboard_bank_wbsmemory, (gb->gfxmem_bank->start + PICASSOIV_VRAM2 + 0x400000) >> 16, 0x400000 >> 16);
				// regs
				map_banks_z3(&gb->gfxboard_bank_registers, (gb->gfxmem_bank->start + PICASSOIV_REG) >> 16, 0x200000 >> 16);
				map_banks_z3(&gb->gfxboard_bank_special, gb->gfxmem_bank->start >> 16, PICASSOIV_REG >> 16);
			}
			gb->picassoiv_bank = 0;
			gb->picassoiv_flifi = 1;
			gb->configured_regs = gb->gfxmem_bank->start >> 16;

		} else if (gb->board->pcemdev) {

			if (boardnum == GFXBOARD_ID_CV64_Z3) {

				map_banks_z3(&gb->gfxboard_bank_special_pcem, (start + 0x40000) >> 16, 1);
				map_banks_z3(&gb->gfxboard_bank_vram_cv_1_pcem, (start + 0x1400000) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				map_banks_z3(&gb->gfxboard_bank_vram_cv_1_pcem, (start + 0x3400000) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				map_banks_z3(&gb->gfxboard_bank_io_pcem, (start + 0x2000000) >> 16, 1);
				gb->pcem_mmio_offset = 0x1000000;
				gb->pcem_mmio_mask = 0xffff;
				gb->pcem_io_mask = 0xffff;
				gb->configured_regs = gb->gfxmem_bank->start >> 16;
				gb->pcem_vram_mask = 0x3fffff;

			} else if (boardnum == GFXBOARD_ID_CV643D_Z3) {

				map_banks_z3(&gb->gfxboard_bank_vram_pcem, (start + 0x4000000) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				map_banks_z3(&gb->gfxboard_bank_vram_pcem, (start + 0x4400000) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				map_banks_z3(&gb->gfxboard_bank_vram_pcem, (start + 0x4800000) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				map_banks_z3(&gb->gfxboard_bank_vram_pcem, (start + 0x4c00000) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				map_banks_z3(&gb->gfxboard_bank_mmio_pcem, (start + 0x5000000) >> 16, 1);
				map_banks_z3(&gb->gfxboard_bank_mmio_wbs_pcem, (start + 0x5800000) >> 16, 1);
				map_banks_z3(&gb->gfxboard_bank_mmio_lbs_pcem, (start + 0x7000000) >> 16, 1);
				map_banks_z3(&gb->gfxboard_bank_special_pcem, (start + 0x8000000) >> 16, 1);
				map_banks_z3(&gb->gfxboard_bank_io_swap2_pcem, (start + 0xc000000) >> 16, 4);
				map_banks_z3(&gb->gfxboard_bank_pci_pcem, (start + 0xc0e0000) >> 16, 4);
				gb->pcem_mmio_offset = 0x1000000;
				gb->pcem_mmio_mask = 0xffff;
				gb->pcem_io_mask = 0x3fff;
				gb->configured_regs = gb->gfxmem_bank->start >> 16;
				gb->pcem_vram_mask = 0x3fffff;

			} else if (boardnum == GFXBOARD_ID_PICASSO4_Z3) {

				gb->p4_special_start = start;
				map_banks_z3(&gb->gfxboard_bank_special_pcem, start >> 16, PICASSOIV_REG >> 16);
				map_banks_z3(&gb->gfxboard_bank_io_swap_pcem, (start + PICASSOIV_REG) >> 16, 4);
				map_banks_z3(&gb->gfxboard_bank_vram_longswap_pcem, (start + PICASSOIV_VRAM1) >> 16, 0x400000 >> 16);
				map_banks_z3(&gb->gfxboard_bank_vram_wordswap_pcem, (start + PICASSOIV_VRAM1 + 0x400000) >> 16, 0x400000 >> 16);
				map_banks_z3(&gb->gfxboard_bank_vram_longswap_pcem, (start + PICASSOIV_VRAM2) >> 16, 0x400000 >> 16);
				map_banks_z3(&gb->gfxboard_bank_vram_wordswap_pcem, (start + PICASSOIV_VRAM2 + 0x400000) >> 16, 0x400000 >> 16);
				gb->pcem_mmio_offset = 0xb8000;
				gb->pcem_mmio_mask = 0xffff;
				gb->pcem_io_mask = 0x3fff;
				gb->pcem_vram_mask = 0x3fffff;

				gb->picassoiv_bank = 0;
				gb->picassoiv_flifi = 1;
				gb->configured_regs = gb->gfxmem_bank->start >> 16;

			} else if (boardnum == GFXBOARD_ID_PICCOLO_Z3 || boardnum == GFXBOARD_ID_SPECTRUM_Z3) {

				// VRAM +0x800000. WORD/LONG endian swap hardware.
				map_banks_z3(&gb->gfxboard_bank_vram_pcem, start  >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				gb->pcem_vram_offset = 0x800000;
				gb->pcem_vram_mask = 0x1fffff;
				gb->pcem_io_mask = 0x3fff;

			} else if (boardnum == GFXBOARD_ID_SD64_Z3) {

				map_banks_z3(&gb->gfxboard_bank_vram_pcem, start >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				gb->pcem_vram_offset = 0x800000;
				gb->pcem_vram_mask = 0x3fffff;
				gb->pcem_io_mask = 0x3fff;

			} else if (boardnum == GFXBOARD_ID_RETINA_Z3) {

				uae_u32 offset = 0xc00000;
				for (;;) {
					map_banks_z3(&gb->gfxboard_bank_vram_pcem, (start + offset) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
					offset += gb->gfxboard_bank_vram_pcem.allocated_size;
					if (offset >= 0x1000000)
						break;
				}
				map_banks_z3(&gb->gfxboard_bank_mmio_wbs_pcem, (start + 0xb00000) >> 16, 1);
				map_banks_z3(&gb->gfxboard_bank_io_swap_pcem, (start + 0x000000) >> 16, 1);
				gb->pcem_vram_offset = 0x800000;
				gb->pcem_vram_mask = 0x3fffff;
				gb->pcem_io_mask = 0x3fff;
				gb->pcem_mmio_offset = 0x00300000;
				gb->pcem_mmio_mask = 0xff;
				gb->configured_regs = gb->gfxmem_bank->start >> 16;
				gb->gfxboard_intena = 1;

			} else if (boardnum == GFXBOARD_ID_RAINBOWIII) {

				map_banks_z3(&gb->gfxboard_bank_vram_pcem, start >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				gb->pcem_mmio_offset = 0x1000000;
				gb->pcem_vram_mask = 0x3fffff;
				gb->pcem_vram_offset = 0x800000;
				map_banks_z3(&gb->gfxboard_bank_special_pcem, (start + 0x400000) >> 16, 0xc00000 >> 16);
				map_banks_z3(&gb->gfxboard_bank_special_pcem, (start + 0x1000000) >> 16, 0x1000000 >> 16);
				gb->pcem_mmio_mask = 0xffff;
				gb->configured_regs = gb->gfxmem_bank->start >> 16;
				gb->gfxboard_external_interrupt = true;

			} else if (boardnum == GFXBOARD_ID_MERLIN_Z3) {

				// uses MMIO because ET4000 can have different apertures in VRAM space
				copyvrambank(&gb->gfxboard_bank_mmio_wbs_pcem, &gb->gfxboard_bank_vram_pcem, true);
				map_banks_z3(&gb->gfxboard_bank_mmio_wbs_pcem, start >> 16, gb->gfxboard_bank_mmio_wbs_pcem.allocated_size >> 16);
				gb->pcem_mmio_offset = 0x00000000;
				gb->pcem_mmio_mask = 0x3fffff;
				map_banks_z3(&gb->gfxboard_bank_special_pcem, (start + 0x1000000) >> 16, 0x1000000 >> 16);
				gb->configured_regs = gb->gfxmem_bank->start >> 16;
				gb->gfxboard_intena = 1;
				merlin_init(gb);

			} else if (boardnum == GFXBOARD_ID_GRAFFITY_Z3) {

				map_banks_z3(&gb->gfxboard_bank_vram_pcem, (start + 0xc00000) >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16);
				map_banks_z3(&gb->gfxboard_bank_special_pcem, (start + 0x800000) >> 16, 65536 >> 16);
				map_banks_z3(&gb->gfxboard_bank_special_pcem, (start + 0x400000) >> 16, 65536 >> 16);
				gb->pcem_vram_offset = -0x400000;
				gb->pcem_vram_mask = 0x1fffff;
				gb->pcem_io_mask = 0x3fff;
				gb->configured_regs = gb->gfxmem_bank->start >> 16;

			}

		} else {
			map_banks_z3(&gb->gfxboard_bank_memory, gb->gfxmem_bank->start >> 16, gb->board->banksize >> 16);
		}

		gb->mem_start[0] = start;
		gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
		gb->configured_mem = gb->gfxmem_bank->start >> 16;
		expamem_next (&gb->gfxboard_bank_memory, NULL);
		return;
	}
	if (addr == 0x4c) {
		gb->configured_mem = 0xff;
		expamem_shutup(&gb->gfxboard_bank_memory);
		return;
	}
}

static void REGPARAM2 gfxboard_bput_mem_autoconfig (uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int boardnum = gb->rbc->rtgmem_type;
	b &= 0xff;
	addr &= 65535;
	if (addr == 0x48) {
		if (gb->board->configtype == 2) {
			addrbank *ab;
			if (ISP4() && !gb->board->pcemdev) {
				ab = &gb->gfxboard_bank_nbsmemory;
				copyvrambank(ab, gb->gfxmem_bank, true);
				map_banks_z2 (ab, b, 0x00200000 >> 16);
				if (gb->configured_mem <= 0) {
					gb->configured_mem = b;
					gb->gfxboardmem_start = b << 16;
					init_board(gb);
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + 0x00200000;
				} else {
					gb->gfxboard_bank_memory.bget = gfxboard_bget_mem;
					gb->gfxboard_bank_memory.bput = gfxboard_bput_mem;
					gb->mem_start[1] = b << 16;
					gb->mem_end[1] = gb->mem_start[1] + 0x00200000;
				}

			} else if (gb->board->pcemdev) {
				
				copyvrambank(&gb->gfxboard_bank_vram_pcem, gb->gfxmem_bank, true);

				if (boardnum == GFXBOARD_ID_CV643D_Z2) {

					ab = &gb->gfxboard_bank_memory;
					copyvrambank(ab, gb->gfxmem_bank, true);
					uaecptr start = b << 16;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(&gb->gfxboard_bank_vram_pcem, start >> 16, (gb->board->banksize - (0x400000 - 0x3a0000)) >> 16);
					map_banks_z2(&gb->gfxboard_bank_special_pcem, (start + 0x3a0000) >> 16, (0x400000 - 0x3a0000) >> 16);
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;

					init_board(gb);

					gb->pcem_vram_offset = 0;
					gb->pcem_vram_mask = 0x3fffff;
					gb->pcem_mmio_offset = 0x1000000;
					gb->pcem_mmio_mask = 0xffff;
					gb->pcem_io_mask = 0x3fff;
					gb->configured_mem = gb->gfxmem_bank->start >> 16;
					gb->configured_regs = gb->gfxmem_bank->start >> 16;

				} else if (boardnum == GFXBOARD_ID_PICASSO4_Z2) {

					ab = &gb->gfxboard_bank_memory;
					copyvrambank(ab, gb->gfxmem_bank, true);
					map_banks_z2(&gb->gfxboard_bank_vram_p4z2_pcem, b, 0x00200000 >> 16);
					if (gb->configured_mem <= 0) {
						gb->configured_mem = b;
						gb->gfxboardmem_start = b << 16;
						init_board(gb);
						gb->mem_start[0] = b << 16;
						gb->mem_end[0] = gb->mem_start[0] + 0x00200000;
						gb->pcem_vram_offset = 0;
						gb->pcem_vram_mask = 0x3fffff;
						gb->pcem_mmio_offset = 0xb8000;
						gb->pcem_mmio_mask = 0xffff;
						gb->pcem_io_mask = 0x3fff;
					} else {
						gb->gfxboard_bank_memory.bget = gfxboard_bget_mem;
						gb->gfxboard_bank_memory.bput = gfxboard_bput_mem;
						gb->mem_start[1] = b << 16;
						gb->mem_end[1] = gb->mem_start[1] + 0x00200000;
					}

				} else if (boardnum == GFXBOARD_ID_PICCOLO_Z2 || boardnum == GFXBOARD_ID_SPECTRUM_Z2) {

					ab = &gb->gfxboard_bank_vram_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;

					gb->pcem_vram_offset = 0x800000;
					gb->pcem_vram_mask = 0x1fffff;
					gb->pcem_io_mask = 0x3fff;

				} else if (boardnum == GFXBOARD_ID_SD64_Z2) {

					ab = &gb->gfxboard_bank_vram_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
					gb->pcem_vram_offset = 0x800000;
					gb->pcem_vram_mask = 0x3fffff;
					gb->pcem_io_mask = 0x3fff;

				} else if (boardnum == GFXBOARD_ID_PIXEL64) {

					ab = &gb->gfxboard_bank_vram_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, 0x200000 >> 16);
					map_banks_z2(&gb->gfxboard_bank_vram_wordswap_pcem, b + (0x200000 >> 16), 0x200000 >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;

					gb->pcem_vram_offset = 0x800000;
					gb->pcem_vram_mask = 0x1fffff;
					gb->pcem_io_mask = 0x3fff;

				} else if (boardnum == GFXBOARD_ID_OMNIBUS_ET4000) {

					ab = &gb->gfxboard_bank_vram_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, 0x200000 >> 16);
					map_banks_z2(&gb->gfxboard_bank_vram_wordswap_pcem, b + (0x200000 >> 16), 0x200000 >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;

					gb->pcem_vram_offset = 0x800000;
					gb->pcem_vram_mask = 0x1fffff;
					gb->pcem_io_mask = 0x3fff;

				} else if (boardnum == GFXBOARD_ID_RETINA_Z2) {

					gb->configured_mem = b;
					gb->configured_regs = b;

					ab = &gb->gfxboard_bank_special_pcem;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
					gb->pcem_vram_offset = 0x800000;
					gb->pcem_vram_mask = 0x3fffff;
					gb->pcem_io_mask = 0x3fff;

				} else if (boardnum == GFXBOARD_ID_VISIONA) {


					ab = &gb->gfxboard_bank_vram_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
					gb->pcem_vram_offset = 0x800000;
					gb->pcem_vram_mask = 0x3fffff;
					gb->pcem_mmio_mask = 0x3fff;
					gb->pcem_mmio_offset = 0x800000;
					gb->gfxboard_external_interrupt = true;

				} else if (boardnum == GFXBOARD_ID_DOMINO) {


					ab = &gb->gfxboard_bank_vram_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
					gb->pcem_vram_offset = 0x800000;
					gb->pcem_vram_mask = 0x3fffff;

				} else if (boardnum == GFXBOARD_ID_MERLIN_Z2) {

					// uses MMIO because ET4000 can have different apertures in VRAM space
					ab = &gb->gfxboard_bank_mmio_wbs_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
					gb->pcem_mmio_offset = 0x00000000;
					gb->pcem_mmio_mask = 0x1fffff;
					gb->pcem_io_mask = 0xffff;
					gb->gfxboard_intena = 1;
					merlin_init(gb);

				} else if (boardnum == GFXBOARD_ID_OMNIBUS_ET4000W32) {

					// uses MMIO because ET4000 can have different apertures in VRAM space
					ab = &gb->gfxboard_bank_mmio_wbs_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
					gb->pcem_mmio_offset = 0x00000000;
					gb->pcem_mmio_mask = 0x0fffff;
					gb->pcem_io_mask = 0xffff;
					gb->gfxboard_intena = 1;

				} else {

					// Picasso II, Picasso II+
					// Piccolo Z2
					// Graffity Z2

					ab = &gb->gfxboard_bank_vram_pcem;
					gb->gfxboardmem_start = b << 16;
					map_banks_z2(ab, b, gb->board->banksize >> 16);

					init_board(gb);

					gb->configured_mem = b;
					gb->mem_start[0] = b << 16;
					gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;
					gb->pcem_vram_offset = 0x00200000 + 0x800000;
					gb->pcem_vram_mask = 0x3fffff;
					gb->pcem_io_mask = 0x3fff;

				}

			} else {

				ab = &gb->gfxboard_bank_memory;
				gb->gfxboard_bank_memory.bget = gfxboard_bget_mem;
				gb->gfxboard_bank_memory.bput = gfxboard_bput_mem;
				gb->gfxboard_bank_memory.wget = gfxboard_wget_mem;
				gb->gfxboard_bank_memory.wput = gfxboard_wput_mem;
				gb->gfxboardmem_start = b << 16;
				init_board (gb);
				copyvrambank(ab, gb->gfxmem_bank, true);
				map_banks_z2 (ab, b, gb->board->banksize >> 16);
				gb->configured_mem = b;
				gb->mem_start[0] = b << 16;
				gb->mem_end[0] = gb->mem_start[0] + gb->board->banksize;

			}
			expamem_next (ab, NULL);
		} else {
			gb->expamem_lo = b & 0xff;
		}
		return;
	}
	if (addr == 0x4c) {
		gb->configured_mem = 0xff;
		expamem_shutup(&gb->gfxboard_bank_memory);
		return;
	}
}

static uaecptr mungeaddr (struct rtggfxboard *gb, uaecptr addr, bool write)
{
	addr &= 65535;
	if (addr >= 0x2000) {
		if (addr == 0x46e8) {
			// wakeup register
			return 0;
		}
		write_log (_T("GFXBOARD: %c unknown IO address %x\n"), write ? 'W' : 'R', addr);
		return 0;
	}
	if (addr >= 0x1000) {
		if (gb->board->manufacturer == BOARD_MANUFACTURER_PICASSO) {
			if (addr == 0x1001) {
				gb->gfxboard_intena = 1;
				return 0;
			}
			if (addr == 0x1000) {
				gb->gfxboard_intena = 0;
				return 0;
			}
		}
		if ((addr & 0xfff) < 0x3b0) {
			write_log (_T("GFXBOARD: %c unknown IO address %x\n"), write ? 'W' : 'R', addr);
			return 0;
		}
		addr++;
	}
	addr &= 0x0fff;
	if (addr == 0x102) {
		// POS102
		return 0;
	}
	if (addr < 0x3b0) {
		write_log (_T("GFXBOARD: %c unknown IO address %x\n"), write ? 'W' : 'R', addr);
		return 0;
	}
	addr -= 0x3b0;
	return addr;
}

static uae_u32 REGPARAM2 gfxboard_lget_regs (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v = 0xffffffff;
	addr = mungeaddr (gb, addr, false);
	if (addr)
		v = (uae_u32)gb->vgaio->read (&gb->vga, addr, 4);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_regs (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u16 v = 0xffff;
	addr = mungeaddr (gb, addr, false);
	if (addr) {
		uae_u8 v1, v2;
		v1 = (uae_u8)gb->vgaio->read (&gb->vga, addr + 0, 1);
		v1 = bget_regtest (gb, addr + 0, v1);
		v2 = (uae_u8)gb->vgaio->read (&gb->vga, addr + 1, 1);
		v2 = bget_regtest (gb, addr + 1, v2);
		v = (v1 << 8) | v2;
	}
	return v;
}
static uae_u32 REGPARAM2 gfxboard_bget_regs (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u8 v = 0xff;
	addr &= 65535;
	if (addr >= 0x8000) {
		write_log (_T("GFX SPECIAL BGET IO %08X\n"), addr);
		return 0;
	}
	addr = mungeaddr (gb, addr, false);
	if (addr) {
		v = (uae_u8)gb->vgaio->read (&gb->vga, addr, 1);
		v = bget_regtest (gb, addr, v);
#if REGDEBUG
		write_log(_T("GFX VGA BYTE GET IO %04X = %02X PC=%08x\n"), addr & 65535, v & 0xff, M68K_GETPC);
#endif
	}
	return v;
}

static void REGPARAM2 gfxboard_lput_regs (uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);

	addr = mungeaddr (gb, addr, true);
	if (addr) {
		gb->vgaio->write (&gb->vga, addr + 0, l >> 24, 1);
		bput_regtest (gb, addr + 0, (l >> 24));
		gb->vgaio->write (&gb->vga, addr + 1, (l >> 16) & 0xff, 1);
		bput_regtest (gb, addr + 0, (l >> 16));
		gb->vgaio->write (&gb->vga, addr + 2, (l >>  8) & 0xff, 1);
		bput_regtest (gb, addr + 0, (l >>  8));
		gb->vgaio->write (&gb->vga, addr + 3, (l >>  0) & 0xff, 1);
		bput_regtest (gb, addr + 0, (l >>  0));
#if REGDEBUG
		write_log(_T("GFX VGA LONG PUT IO %04X = %04X PC=%08x\n"), addr & 65535, l, M68K_GETPC);
#endif
	}
}
static void REGPARAM2 gfxboard_wput_regs (uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = mungeaddr (gb, addr, true);
	if (addr) {
		gb->vgaio->write (&gb->vga, addr + 0, (w >> 8) & 0xff, 1);
		bput_regtest (gb, addr + 0, (w >> 8));
		gb->vgaio->write (&gb->vga, addr + 1, (w >> 0) & 0xff, 1);
		bput_regtest (gb, addr + 1, (w >> 0));
#if REGDEBUG
		write_log(_T("GFX VGA WORD PUT IO %04X = %04X PC=%08x\n"), addr & 65535, w & 0xffff, M68K_GETPC);
#endif
	}
}
static void REGPARAM2 gfxboard_bput_regs (uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= 65535;
	if (addr >= 0x8000) {
		write_log (_T("GFX SPECIAL BPUT IO %08X = %02X\n"), addr, b & 0xff);
		switch (gb->board->manufacturer)
		{
		case BOARD_MANUFACTURER_PICASSO:
			{
				if ((addr & 1) == 0) {
					int idx = addr >> 12;
					if (idx == 0x0b || idx == 0x09) {
						set_monswitch(gb, false);
					} else if (idx == 0x0a || idx == 0x08) {
						set_monswitch(gb, true);
					}
				}
			}
		break;
		case BOARD_MANUFACTURER_PICCOLO:
		case BOARD_MANUFACTURER_SPECTRUM:
			set_monswitch(gb, (b & 0x20) != 0);
			gb->gfxboard_intena = (b & 0x40) != 0;
		break;
		}
		return;
	}
	addr = mungeaddr (gb, addr, true);
	if (addr) {
		gb->vgaio->write (&gb->vga, addr, b & 0xff, 1);
		bput_regtest (gb, addr, b);
#if REGDEBUG
		write_log(_T("GFX VGA BYTE PUT IO %04X = %02X PC=%08x\n"), addr & 65535, b & 0xff, M68K_GETPC);
#endif
	}
}

static uae_u32 REGPARAM2 gfxboard_bget_regs_autoconfig (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v = 0;
	addr &= 65535;
	if (addr < GFXBOARD_AUTOCONFIG_SIZE)
		v = gb->automemory[addr];
	return v;
}

static void REGPARAM2 gfxboard_bput_regs_autoconfig (uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int boardnum = gb->rbc->rtgmem_type;
	addrbank *ab;
	b &= 0xff;
	addr &= 65535;
	if (addr == 0x48) {
		gb->gfxboard_bank_registers.bget = gfxboard_bget_regs;
		gb->gfxboard_bank_registers.bput = gfxboard_bput_regs;

		if (gb->p4z2 && !gb->pcemdev) {

			ab = &gb->gfxboard_bank_special;
			map_banks_z2(ab, b, gb->gfxboard_bank_special.reserved_size >> 16);
			gb->io_start = b << 16;
			gb->io_end = gb->io_start + gb->gfxboard_bank_special.reserved_size;

		} else if (gb->pcemdev) {

			ab = &gb->gfxboard_bank_special_pcem;
			int size = (boardnum == GFXBOARD_ID_PICASSO4_Z2 || boardnum == GFXBOARD_ID_PIXEL64) ? 0x20000 : 0x10000;
			map_banks_z2(ab, b, size >> 16);
			gb->io_start = b << 16;
			gb->io_end = gb->io_start + size;
			gb->p4_special_start = gb->io_start;

		} else {

			ab = &gb->gfxboard_bank_registers;
			map_banks_z2(ab, b, gb->gfxboard_bank_registers.reserved_size >> 16);
			gb->io_start = b << 16;
			gb->io_end = gb->io_start + gb->gfxboard_bank_registers.reserved_size;

		}
		gb->configured_regs = b;
		expamem_next (ab, NULL);
		return;
	}
	if (addr == 0x4c) {
		gb->configured_regs = 0xff;
		expamem_next (NULL, NULL);
		return;
	}
}

static void gfxboard_free_board(struct rtggfxboard *gb)
{
	if (gb->rbc) {
		if (gb->func) {
			if (gb->userdata)
				gb->func->free(gb->userdata);
			gfxboard_free_slot2(gb);
			gb->rbc = NULL;
			return;
		}
	}
	if (gb->pcemdev) {
		if (gb->pcemobject) {
#ifdef USE_PCEM
			pcemfreeaddeddevices();
			gb->pcemdev->close(gb->pcemobject);
#endif
			gb->pcemobject = NULL;
			gb->pcemobject2 = NULL;
		}
	}
	if (gb->vram && gb->gfxmem_bank->baseaddr) {
		gb->gfxmem_bank->baseaddr = gb->vramrealstart;
		gfxboard_free_vram(gb->rbc->rtg_index);
	}
	gb->gfxmem_bank = NULL;
	gb->vram = NULL;
	gb->vramrealstart = NULL;
	xfree(gb->fakesurface_surface);
	gb->fakesurface_surface = NULL;
	xfree(gb->bios);
	gb->bios = NULL;
	gb->configured_mem = 0;
	gb->configured_regs = 0;
	gb->monswitch_new = false;
	gb->monswitch_current = false;
	gb->monswitch_delay = -1;
	gb->monswitch_reset = true;
	gb->resolutionchange = 0;
	gb->gfxboard_intreq = false;
	gb->gfxboard_intena = 0;
	gb->picassoiv_bank = 0;
	gb->active = false;
	flash_free(gb->p4flashrom);
	gb->p4flashrom = NULL;
	zfile_fclose(gb->p4rom);
	gb->p4rom = NULL;
}

void gfxboard_free(void)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		gfxboard_free_board(gb);
	}
}

void gfxboard_reset (void)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		gb->rbc = &currprefs.rtgboards[gb->rtg_index];
		if (gb->func) {
			if (gb->userdata) {
				gb->func->reset(gb->userdata);
			}
			gfxboard_free_board(gb);
		} else {
			if (gb->rbc->rtgmem_type >= GFXBOARD_HARDWARE) {
				gb->board = find_board(gb->rbc->rtgmem_type);
			}
			gfxboard_free_board(gb);
			if (gb->board) {
				if (gb->board->configtype == 3)
					gb->gfxboard_bank_memory.wput = gfxboard_wput_mem_autoconfig;
				if (reset_func) 
					reset_func (reset_parm);
			}
		}
		if (gb->monitor_id > 0) {
			close_rtg(gb->monitor_id, true);
		}
	}
	for (int i = 0; i < MAX_AMIGADISPLAYS; i++) {
		rtg_visible[i] = -1;
		rtg_initial[i] = -1;
	}
}

static uae_u32 REGPARAM2 gfxboards_lget_regs (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v = 0;
	addr &= gb->p4_special_mask;
	// pci config
	if (addr >= 0x400000 || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
		uae_u32 addr2 = addr & 0xffff;
		if (addr2 >= 0x0800 && addr2 < 0x840) {
			addr2 -= 0x800;
			v = gb->p4_pci[addr2 + 0] << 24;
			v |= gb->p4_pci[addr2 + 1] << 16;
			v |= gb->p4_pci[addr2 + 2] <<  8;
			v |= gb->p4_pci[addr2 + 3] <<  0;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV PCI LGET %08x %08x\n"), addr, v);
#endif
		} else if (addr2 >= 0x1000 && addr2 < 0x1040) {
			addr2 -= 0x1000;
			v = gb->cirrus_pci[addr2 + 0] << 24;
			v |= gb->cirrus_pci[addr2 + 1] << 16;
			v |= gb->cirrus_pci[addr2 + 2] <<  8;
			v |= gb->cirrus_pci[addr2 + 3] <<  0;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV CL PCI LGET %08x %08x\n"), addr, v);
#endif
		}
		return v;
	}
	if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
		// memory mapped io
		if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
			uae_u32 addr2 = addr - gb->p4_mmiobase;
			v  = ((uae_u8)gb->vgammio->read(&gb->vga, addr2 + 0, 1)) << 24;
			v |= ((uae_u8)gb->vgammio->read(&gb->vga, addr2 + 1, 1)) << 16;
			v |= ((uae_u8)gb->vgammio->read(&gb->vga, addr2 + 2, 1)) <<  8;
			v |= ((uae_u8)gb->vgammio->read(&gb->vga, addr2 + 3, 1)) <<  0;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV MMIO LGET %08x %08x\n"), addr, v);
#endif
			return v;
		}
	}
#if PICASSOIV_DEBUG_IO
	write_log (_T("PicassoIV LGET %08x %08x\n"), addr, v);
#endif
	return v;
}
static uae_u32 REGPARAM2 gfxboards_wget_regs (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u16 v = 0;
	addr &= gb->p4_special_mask;
	// pci config
	if (addr >= 0x400000 || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
		uae_u32 addr2 = addr & 0xffff;
		if (addr2 >= 0x0800 && addr2 < 0x840) {
			addr2 -= 0x800;
			v = gb->p4_pci[addr2 + 0] << 8;
			v |= gb->p4_pci[addr2 + 1] << 0;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV PCI WGET %08x %04x\n"), addr, v);
#endif
		} else if (addr2 >= 0x1000 && addr2 < 0x1040) {
			addr2 -= 0x1000;
			v = gb->cirrus_pci[addr2 + 0] << 8;
			v |= gb->cirrus_pci[addr2 + 1] << 0;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV CL PCI WGET %08x %04x\n"), addr, v);
#endif
		}
		return v;
	}
	if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
		// memory mapped io
		if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
			uae_u32 addr2 = addr - gb->p4_mmiobase;
			v  = ((uae_u8)gb->vgammio->read(&gb->vga, addr2 + 0, 1)) << 8;
			v |= ((uae_u8)gb->vgammio->read(&gb->vga, addr2 + 1, 1)) << 0;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV MMIO WGET %08x %04x\n"), addr, v & 0xffff);
#endif
			return v;
		}
	}
#if PICASSOIV_DEBUG_IO
	write_log (_T("PicassoIV WGET %04x %04x\n"), addr, v);
#endif
	return v;
}
static uae_u32 REGPARAM2 gfxboards_bget_regs (uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u8 v = 0xff;
	addr &= gb->p4_special_mask;

#if PICASSOIV_DEBUG_IO > 1
	write_log(_T("PicassoIV CL REG BGET %08x PC=%08x\n"), addr, M68K_GETPC);
#endif

	// pci config
	if (addr >= 0x400000 || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
		uae_u32 addr2 = addr & 0xffff;
		v = 0;
		if (addr2 >= 0x0800 && addr2 < 0x840) {
			if (addr2 == 0x802) {
				v = 2; // bridge version?
			} else if (addr2 == 0x808) {
				v = 4; // bridge revision
			} else {
				addr2 -= 0x800;
				v = gb->p4_pci[addr2];
			}
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV PCI BGET %08x %02x\n"), addr, v);
#endif
		} else if (addr2 >= 0x1000 && addr2 <= 0x1040) {
			addr2 -= 0x1000;
			v = gb->cirrus_pci[addr2];
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV CL PCI BGET %08x %02x\n"), addr, v);
#endif
		}
		return v;
	}

	if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
		// memory mapped io
		if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
			uae_u32 addr2 = addr - gb->p4_mmiobase;
			v = (uae_u8)gb->vgammio->read(&gb->vga, addr2, 1);
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV MMIO BGET %08x %02x\n"), addr, v & 0xff);
#endif
			return v;
		}
	}
	if (addr == 0) {
		v = gb->picassoiv_bank;
		return v;
	}
	if (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) {
		v = 0;
		if (addr == 0x404) {
			v = 0x70; // FLIFI revision
			// FLIFI type in use
			if (currprefs.chipset_mask & CSMASK_AGA)
				v |= 4 | 8;
			else
				v |= 8;
		} else if (addr == 0x406) {
			// FLIFI I2C
			// bit 0 = clock out
			// bit 1 = data out
			// bit 2 = clock in
			// bit 7 = data in
			v = gb->p4i2c & 3;
			if (v & 1)
				v |= 4;
			if (v & 2)
				v |= 0x80;
		} else if (addr == 0x408) {
			v = gb->gfxboard_intreq ? 0x80 : 0;
		} else if (gb->p4z2 && addr >= 0x10000) {
			addr -= 0x10000;
			uaecptr addr2 = mungeaddr (gb, addr, true);
			if (addr2) {
				v = (uae_u8)gb->vgaio->read(&gb->vga, addr2, 1);
				v = bget_regtest(gb, addr2, v);
				//write_log(_T("P4 VGA read %08X=%02X PC=%08x\n"), addr2, v, M68K_GETPC);
			}
			//write_log (_T("PicassoIV IO %08x %02x\n"), addr, v);
			return v;
		}
#if PICASSOIV_DEBUG_IO
		if (addr != 0x408)
			write_log (_T("PicassoIV BGET %08x %02x\n"), addr, v);
#endif
	} else {
		addr += ((gb->picassoiv_bank & PICASSOIV_BANK_FLASHBANK) ? 0x8000 * 2 : 0);
		addr /= 2;
		v = flash_read(gb->p4flashrom, addr);
	}
	return v;
}
static void REGPARAM2 gfxboards_lput_regs (uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->p4_special_mask;
	if (addr >= 0x400000 || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
		uae_u32 addr2 = addr & 0xffff;
		if (addr2 >= 0x0800 && addr2 < 0x840) {
			addr2 -= 0x800;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV PCI LPUT %08x %08x\n"), addr, l);
#endif
			gb->p4_pci[addr2 + 0] = l >> 24;
			gb->p4_pci[addr2 + 1] = l >> 16;
			gb->p4_pci[addr2 + 2] = l >>  8;
			gb->p4_pci[addr2 + 3] = l >>  0;
			p4_pci_check (gb);
		} else if (addr2 >= 0x1000 && addr2 < 0x1040) {
			addr2 -= 0x1000;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV CL PCI LPUT %08x %08x\n"), addr, l);
#endif
			gb->cirrus_pci[addr2 + 0] = l >> 24;
			gb->cirrus_pci[addr2 + 1] = l >> 16;
			gb->cirrus_pci[addr2 + 2] = l >>  8;
			gb->cirrus_pci[addr2 + 3] = l >>  0;
			reset_pci (gb);
		}
		return;
	}
	if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
		// memory mapped io
		if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV MMIO LPUT %08x %08x\n"), addr, l);
#endif
			uae_u32 addr2 = addr - gb->p4_mmiobase;
			gb->vgammio->write(&gb->vga, addr2 + 0, l >> 24, 1);
			gb->vgammio->write(&gb->vga, addr2 + 1, l >> 16, 1);
			gb->vgammio->write(&gb->vga, addr2 + 2, l >>  8, 1);
			gb->vgammio->write(&gb->vga, addr2 + 3, l >>  0, 1);
			return;
		}
	}
#if PICASSOIV_DEBUG_IO
	write_log (_T("PicassoIV LPUT %08x %08x\n"), addr, l);
#endif
}
static void REGPARAM2 gfxboards_wput_regs (uaecptr addr, uae_u32 v)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u16 w = (uae_u16)v;
	addr &= gb->p4_special_mask;
	if (addr >= 0x400000 || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
		uae_u32 addr2 = addr & 0xffff;
		if (addr2 >= 0x0800 && addr2 < 0x840) {
			addr2 -= 0x800;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV PCI WPUT %08x %04x\n"), addr, w & 0xffff);
#endif
			gb->p4_pci[addr2 + 0] = w >> 8;
			gb->p4_pci[addr2 + 1] = w >> 0;
			p4_pci_check (gb);
		} else if (addr2 >= 0x1000 && addr2 < 0x1040) {
			addr2 -= 0x1000;
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV CL PCI WPUT %08x %04x\n"), addr, w & 0xffff);
#endif
			gb->cirrus_pci[addr2 + 0] = w >> 8;
			gb->cirrus_pci[addr2 + 1] = w >> 0;
			reset_pci (gb);
		}
		return;
	}
	if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
		// memory mapped io
		if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV MMIO LPUT %08x %08x\n"), addr, w & 0xffff);
#endif
			uae_u32 addr2 = addr - gb->p4_mmiobase;
			gb->vgammio->write(&gb->vga, addr2 + 0, w >> 8, 1);
			gb->vgammio->write(&gb->vga, addr2 + 1, (w >> 0) & 0xff, 1);
			return;
		}
	}
	if (gb->p4z2 && addr >= 0x10000) {
		addr -= 0x10000;
		addr = mungeaddr (gb, addr, true);
		if (addr) {
			gb->vgaio->write (&gb->vga, addr + 0, w >> 8, 1);
			bput_regtest (gb, addr + 0, w >> 8);
			gb->vgaio->write (&gb->vga, addr + 1, (w >> 0) & 0xff, 1);
			bput_regtest (gb, addr + 1, w >> 0);
		}
		return;
	}
#if PICASSOIV_DEBUG_IO
	write_log (_T("PicassoIV WPUT %08x %04x\n"), addr, w & 0xffff);
#endif
}

static void REGPARAM2 gfxboards_bput_regs (uaecptr addr, uae_u32 v)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u8 b = (uae_u8)v;
	addr &= gb->p4_special_mask;

#if PICASSOIV_DEBUG_IO > 1
	write_log(_T("PicassoIV CL REG BPUT %08x %02x PC=%08x\n"), addr, v, M68K_GETPC);
#endif

	if (addr >= 0x400000 || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
		uae_u32 addr2 = addr & 0xffff;
		if (addr2 >= 0x0800 && addr2 < 0x840) {
			addr2 -= 0x800;
			gb->p4_pci[addr2] = b;
			p4_pci_check (gb);
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV PCI BPUT %08x %02x\n"), addr, b & 0xff);
#endif
		} else if (addr2 >= 0x1000 && addr2 < 0x1040) {
			addr2 -= 0x1000;
			gb->cirrus_pci[addr2] = b;
			reset_pci (gb);
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV CL PCI BPUT %08x %02x\n"), addr, b & 0xff);
#endif
		}
		return;
	}
	if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
		// memory mapped io
		if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
#if PICASSOIV_DEBUG_IO
			write_log (_T("PicassoIV MMIO BPUT %08x %08x\n"), addr, b & 0xff);
#endif
			uae_u32 addr2 = addr - gb->p4_mmiobase;
			gb->vgammio->write(&gb->vga, addr2, b, 1);
			return;
		}
	}
	if (gb->p4z2 && addr >= 0x10000 && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH)) {
		addr -= 0x10000;
		addr = mungeaddr (gb, addr, true);
		if (addr) {
			gb->vgaio->write (&gb->vga, addr, b & 0xff, 1);
			bput_regtest (gb, addr, b);
			//write_log(_T("P4 VGA write %08x=%02x PC=%08x\n"), addr, b & 0xff, M68K_GETPC);
		}
		return;
	}

	if (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) {
		if (addr == 0x404) {
			gb->picassoiv_flifi = b;
			picassoiv_checkswitch(gb);
		} else if (addr == 0x406) {
			gb->p4i2c = b;
		}
	}

#if PICASSOIV_DEBUG_IO
	write_log (_T("PicassoIV BPUT %08x %02X\n"), addr, b & 0xff);
#endif
	if (addr == 0) {
		gb->picassoiv_bank = b;
	}
}

void gfxboard_voodoo_lfb_endianswap(int m)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (gb->active && gb->board->bustype == GFXBOARD_BUSTYPE_PCI) {
			if (gb->lfbbyteswapmode != m) {
				gb->lfbbyteswapmode = m;
				if (gb->original_pci_bank) {
					if (!m) {
						gb->pcem_direct = true;
						write_log(_T("Voodoo direct VRAM mode\n"));
						map_banks(gb->gfxmem_bank, gb->gfxmem_bank->start >> 16, 0x01000000 >> 16, 0);
						map_banks(gb->gfxmem_bank, (gb->gfxmem_bank->start + 0x01000000) >> 16, 0x01000000 >> 16, 0);
					} else {
						gb->pcem_direct = false;
						write_log(_T("Voodoo indirect VRAM mode (%d)\n"), m);
						map_banks(gb->original_pci_bank, gb->gfxmem_bank->start >> 16, 0x01000000 >> 16, 0);
						map_banks(gb->original_pci_bank, (gb->gfxmem_bank->start + 0x01000000) >> 16, 0x01000000 >> 16, 0);
					}
				}
			}
			return;
		}
	}
}

void gfxboard_matrox_lfb_endianswap(int m)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (gb->active && gb->board->bustype == GFXBOARD_BUSTYPE_PCI) {
			if (m == 1) {
				m = 3;
			}
			gb->lfbbyteswapmode = m;
		}
	}
}

static void pci_change_config(struct pci_board_state *pci)
{
	struct rtggfxboard *gb = (struct rtggfxboard *)pci->userdata;
	struct romconfig *rc = get_device_romconfig(&currprefs, gb->board->romtype, 0);
	if (gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO3_PCI) {
		if (pci->memory_map_active) {
			struct pci_bridge *pcib = pci->bridge;
			// direct access, bypass PCI emulation redirection for performance reasons
			if (rc && (rc->device_settings & 1) && pci_validate_address(pci->bar[1] + pcib->memory_start_offset[pcib->windowindex], 0x02000000, false)) {
				if (!gb->original_pci_bank) {
					gb->original_pci_bank = &get_mem_bank(pci->bar[1] + pcib->memory_start_offset[pcib->windowindex]);
				}
				reinit_vram(gb, pci->bar[1] + pcib->memory_start_offset[pcib->windowindex], true);
				int m = gb->lfbbyteswapmode;
				gb->lfbbyteswapmode = -1;
				gfxboard_voodoo_lfb_endianswap(m);
			} else {
				reinit_vram(gb, pci->bar[1] + pcib->memory_start_offset[pcib->windowindex], false);
			}
		}
	} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO5_PCI) {
		if (pci->memory_map_active) {
			struct pci_bridge *pcib = pci->bridge;
			// direct access, bypass PCI emulation redirection for performance reasons
			if (pci_validate_address(pci->bar[1] + pcib->memory_start_offset[0], gb->rbc->rtgmem_size, false)) {
				reinit_vram(gb, pci->bar[1] + pcib->memory_start_offset[pcib->windowindex], true);
				gb->original_pci_bank = &get_mem_bank(pci->bar[1] + pcib->memory_start_offset[pcib->windowindex]);
			} else {
				reinit_vram(gb, pci->bar[1] + pcib->memory_start_offset[pcib->windowindex], false);
			}
		}
	} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_S3VIRGE_PCI ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_S3TRIO64_PCI ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_PERMEDIA2_PCI ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_GD5446_PCI ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MILLENNIUM_PCI ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MILLENNIUM_II_PCI ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MYSTIQUE_PCI ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MYSTIQUE220_PCI) {
			if (pci->memory_map_active) {
			struct pci_bridge *pcib = pci->bridge;
			reinit_vram(gb, pci->bar[0] + pcib->memory_start_offset[pcib->windowindex], false);
		}
	}
}

static void REGPARAM2 voodoo3_io_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_io_pcem(addr, b, 2);
}
static void REGPARAM2 voodoo3_io_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_io_pcem(addr, b, 1);
}
static void REGPARAM2 voodoo3_io_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_io_pcem(addr, b, 0);
}
static uae_u32 REGPARAM2 voodoo3_io_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_io_pcem(addr, 2);
}
static uae_u32 REGPARAM2 voodoo3_io_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_io_pcem(addr, 1);
}
static uae_u32 REGPARAM2 voodoo3_io_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_io_pcem(addr, 0);
}

static void REGPARAM2 voodoo3_mb0_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 2);
}
static void REGPARAM2 voodoo3_mb0_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 1);
}
static void REGPARAM2 voodoo3_mb0_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 0);
}
static uae_u32 REGPARAM2 voodoo3_mb0_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_mem_pcem(addr, 2);
}
static uae_u32 REGPARAM2 voodoo3_mb0_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_mem_pcem(addr, 1);
}
static uae_u32 REGPARAM2 voodoo3_mb0_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_mem_pcem(addr, 0);
}

static void REGPARAM2 voodoo3_mb1_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)pcibs->userdata;
	int m = gb->lfbbyteswapmode;
	addr -= pcibs->bar[gb->lfbbar];
	addr &= 0x00ffffff;
	switch (m)
	{
	case 0:
		//((uae_u32*)(addr + gb->vram))[0] = b;
#ifdef USE_PCEM
		pcem_linear_write_l(addr, b, pcem_mapping_linear_priv);
#endif
		break;
	case 1:
		//do_put_mem_long((uae_u32*)(addr + gb->vram), b);
		b = do_byteswap_32(b);
#ifdef USE_PCEM
		pcem_linear_write_l(addr, b, pcem_mapping_linear_priv);
#endif
		break;
	case 2:
		//((uae_u32*)(addr + gb->vram))[0] = b;
		b = do_byteswap_32(b);
#ifdef USE_PCEM
		pcem_linear_write_l(addr, b, pcem_mapping_linear_priv);
#endif
		break;
	case 3:
		b = (b >> 16) | (b << 16);
		b = do_byteswap_32(b);
#ifdef USE_PCEM
		pcem_linear_write_l(addr, b, pcem_mapping_linear_priv);
#endif
		//do_put_mem_long((uae_u32*)(addr + gb->vram), b);
		break;
	}
}
static uae_u32 REGPARAM2 voodoo3_mb1_lget(struct pci_board_state* pcibs, uaecptr addr)
{
	struct rtggfxboard* gb = (struct rtggfxboard*)pcibs->userdata;
	int m = gb->lfbbyteswapmode;
	uae_u32 v = 0;
	addr -= pcibs->bar[gb->lfbbar];
	addr &= 0x00ffffff;
	switch (m)
	{
	case 0:
		//v = ((uae_u32*)(addr + gb->vram))[0];
#ifdef USE_PCEM
		v = pcem_linear_read_l(addr, pcem_mapping_linear_priv);
#endif
		break;
	case 1:
		//v = do_get_mem_long((uae_u32*)(addr + gb->vram));
#ifdef USE_PCEM
		v = pcem_linear_read_l(addr, pcem_mapping_linear_priv);
		v = do_byteswap_32(v);
#endif
		break;
	case 2:
		//v = ((uae_u32*)(addr + gb->vram))[0];
#ifdef USE_PCEM
		v = pcem_linear_read_l(addr, pcem_mapping_linear_priv);
		v = do_byteswap_32(v);
#endif
		break;
	case 3:
		//v = do_get_mem_long((uae_u32*)(addr + gb->vram));
#ifdef USE_PCEM
		v = pcem_linear_read_l(addr, pcem_mapping_linear_priv);
		v = do_byteswap_32(v);
		v = (v >> 16) | (v << 16);
#endif
		break;
	}
	return v;
}

static void REGPARAM2 voodoo3_mb1_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)pcibs->userdata;
	int m = gb->lfbbyteswapmode;
	addr -= pcibs->bar[gb->lfbbar];
	addr &= 0x00ffffff;
	switch (m)
	{
	case 0:
		//*((uae_u16*)(addr + gb->vram)) = b;
#ifdef USE_PCEM
		pcem_linear_write_w(addr, b, pcem_mapping_linear_priv);
#endif
		break;
	case 1:
		addr ^= 2;
		b = do_byteswap_16(b);
		//do_put_mem_word((uae_u16*)(addr + gb->vram), b);
#ifdef USE_PCEM
		pcem_linear_write_w(addr, b, pcem_mapping_linear_priv);
#endif
		break;
	case 2:
		//do_put_mem_word((uae_u16*)(addr + gb->vram), b);
		b = do_byteswap_16(b);
#ifdef USE_PCEM
		pcem_linear_write_w(addr, b, pcem_mapping_linear_priv);
#endif
		break;
	case 3:
		//do_put_mem_word((uae_u16*)(addr + gb->vram), b);
		b = do_byteswap_16(b);
#ifdef USE_PCEM
		pcem_linear_write_w(addr, b, pcem_mapping_linear_priv);
#endif
		break;
	}
}
static uae_u32 REGPARAM2 voodoo3_mb1_wget(struct pci_board_state* pcibs, uaecptr addr)
{
	struct rtggfxboard* gb = (struct rtggfxboard*)pcibs->userdata;
	int m = gb->lfbbyteswapmode;
	uae_u32 v = 0;
	addr -= pcibs->bar[gb->lfbbar];
	addr &= 0x00ffffff;
	switch (m)
	{
	case 0:
		//v = *((uae_u16*)(addr + gb->vram));
#ifdef USE_PCEM
		v = pcem_linear_read_w(addr, pcem_mapping_linear_priv);
#endif
		break;
	case 1:
		addr ^= 2;
		//v = *((uae_u16*)(addr + gb->vram));
#ifdef USE_PCEM
		v = pcem_linear_read_w(addr, pcem_mapping_linear_priv);
		v = do_byteswap_16(v);
#endif
		break;
	case 2:
		//v = do_get_mem_word((uae_u16*)(addr + gb->vram));
#ifdef USE_PCEM
		v = pcem_linear_read_w(addr, pcem_mapping_linear_priv);
		v = do_byteswap_16(v);
#endif
		break;
	case 3:
		//v = do_get_mem_word((uae_u16*)(addr + gb->vram));
#ifdef USE_PCEM
		v = pcem_linear_read_w(addr, pcem_mapping_linear_priv);
		v = do_byteswap_16(v);
#endif
		break;
	}
	return v;
}

static void REGPARAM2 voodoo3_mb1_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)pcibs->userdata;
	int m = gb->lfbbyteswapmode;
	addr -= pcibs->bar[gb->lfbbar];
	addr &= 0x00ffffff;
	switch (m)
	{
	case 1:
	case 3:
		addr ^= 3;
		break;
	}
#ifdef USE_PCEM
	pcem_linear_write_b(addr, b, pcem_mapping_linear_priv);
#endif
	//do_put_mem_byte((uae_u8*)(addr + gb->vram), b);
}
static uae_u32 REGPARAM2 voodoo3_mb1_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	struct rtggfxboard *gb = (struct rtggfxboard*)pcibs->userdata;
	int m = gb->lfbbyteswapmode;
	addr -= pcibs->bar[gb->lfbbar];
	addr &= 0x00ffffff;
	switch (m)
	{
	case 1:
	case 3:
		addr ^= 3;
		break;
	}
#ifdef USE_PCEM
	uae_u32 v = pcem_linear_read_b(addr, pcem_mapping_linear_priv);
#else
	uae_u32 v =  do_get_mem_byte((uae_u8*)(addr + gb->vram));
#endif
	return v;
}


static uae_u32 REGPARAM2 voodoo3_bios_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	struct rtggfxboard* gb = getgfxboard(addr);
	if (!gb->bios)
		return 0;
	addr &= gb->bios_mask;
	return gb->bios[addr];
}
static uae_u32 REGPARAM2 voodoo3_bios_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	return (voodoo3_bios_bget(pcibs, addr) << 0) | (voodoo3_bios_bget(pcibs, addr + 1) << 8);
}
static uae_u32 REGPARAM2 voodoo3_bios_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	return (voodoo3_bios_wget(pcibs, addr + 0) << 24) | (voodoo3_bios_wget(pcibs, addr + 1) << 16) | (voodoo3_bios_wget(pcibs, addr + 2) << 8) | (voodoo3_bios_wget(pcibs, addr + 3) << 0);
}

#ifdef USE_PCEM
uae_u8 get_pci_pcem(uaecptr addr);
void put_pci_pcem(uaecptr addr, uae_u8 v);
#else
static inline uae_u8 get_pci_pcem(uaecptr addr) { return 0; };
static inline void put_pci_pcem(uaecptr addr, uae_u8 v) {};
#endif

static const struct pci_config voodoo3_pci_config =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 1, 0, 0, 0, 0 }
};

static const struct pci_board voodoo3_pci_board =
{
	_T("VOODOO3"),
	&voodoo3_pci_config, NULL, NULL, NULL, NULL, NULL,
	{
		{ voodoo3_mb0_lget, voodoo3_mb0_wget, voodoo3_mb0_bget, voodoo3_mb0_lput, voodoo3_mb0_wput, voodoo3_mb0_bput },
		{ voodoo3_mb1_lget, voodoo3_mb1_wget, voodoo3_mb1_bget, voodoo3_mb1_lput, voodoo3_mb1_wput, voodoo3_mb1_bput },
		{ voodoo3_io_lget, voodoo3_io_wget, voodoo3_io_bget, voodoo3_io_lput, voodoo3_io_wput, voodoo3_io_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ voodoo3_bios_lget, voodoo3_bios_wget, voodoo3_bios_bget, NULL, NULL, NULL },
		{ NULL }
	},
	true,
	get_pci_pcem, put_pci_pcem, pci_change_config
};

static void REGPARAM2 permedia2_mmio_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 2);
}
static void REGPARAM2 permedia2_mmio_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 1);
}
static void REGPARAM2 permedia2_mmio_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 0);
}
static uae_u32 REGPARAM2 permedia2_mmio_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_mem_pcem(addr, 2);
}
static uae_u32 REGPARAM2 permedia2_mmio_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_mem_pcem(addr, 1);
}
static uae_u32 REGPARAM2 permedia2_mmio_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_mem_pcem(addr, 0);
}


static const struct pci_config permedia2_pci_config =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }
};
static const struct pci_board permedia2_pci_board =
{
	_T("PERMEDIA2"),
	&permedia2_pci_config, NULL, NULL, NULL, NULL, NULL,
	{
		{ permedia2_mmio_lget, permedia2_mmio_wget, permedia2_mmio_bget, permedia2_mmio_lput, permedia2_mmio_wput, permedia2_mmio_bput },
		{ voodoo3_mb0_lget, voodoo3_mb0_wget, voodoo3_mb0_bget, voodoo3_mb0_lput, voodoo3_mb0_wput, voodoo3_mb0_bput },
		{ voodoo3_mb0_lget, voodoo3_mb0_wget, voodoo3_mb0_bget, voodoo3_mb0_lput, voodoo3_mb0_wput, voodoo3_mb0_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ voodoo3_bios_lget, voodoo3_bios_wget, voodoo3_bios_bget, NULL, NULL, NULL },
		{ NULL }
	},
	true,
	get_pci_pcem, put_pci_pcem, pci_change_config
};


void gfxboard_s3virge_lfb_endianswap(int m)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (gb->active && gb->board->bustype == GFXBOARD_BUSTYPE_PCI) {
			gb->lfbbyteswapmode = m;
		}
	}
}
void gfxboard_s3virge_lfb_endianswap2(int m)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (gb->active && gb->board->bustype == GFXBOARD_BUSTYPE_PCI) {
			gb->mmiobyteswapmode = m;
		}
	}
}

static int s3virgeaddr(struct pci_board_state *pcibs, uaecptr *addrp)
{
	uaecptr addr = *addrp;
	int swap = 0;
	if (addr >= pcibs->bar[0] + 0x02000000 && addr < pcibs->bar[0] + 0x03000000) {
		// LFB BE
		addr = ((addr - pcibs->bar[0]) & 0x3fffff) + pcibs->bar[0];
		swap = -1;
	} else if (addr >= pcibs->bar[0] + 0x03000000 && addr < pcibs->bar[0] + 0x04000000) {
		// MMIO BE
		addr = ((addr - pcibs->bar[0]) & 0xffff) + pcibs->bar[0] + 0x01000000;
		swap = addr & 0x8000 ? 1 : 2;
	} else if (addr >= pcibs->bar[0] + 0x00000000 && addr < pcibs->bar[0] + 0x01000000) {
		// LFB LE
		addr = ((addr - pcibs->bar[0]) & 0x3fffff) + pcibs->bar[0];
		swap = -2;
	} else if (addr >= pcibs->bar[0] + 0x01000000 && addr < pcibs->bar[0] + 0x02000000) {
		// MMIO LE
		addr = ((addr - pcibs->bar[0]) & 0xffff) + pcibs->bar[0] + 0x01000000;
	}
	*addrp = addr;
	return swap;
}

static void REGPARAM2 s3virge_mb0_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	int swap = s3virgeaddr(pcibs, &addr);
	if (swap > 0) {
		if (swap == 1)
			b = do_byteswap_32(b);
	} else if (swap == -1) {
		struct rtggfxboard *gb = getgfxboard(addr);
		int m = gb->lfbbyteswapmode;
		switch (m)
		{
		case 0:
		default:
			break;
		case 1:
			b = do_byteswap_32(b);
			b = (b >> 16) | (b << 16);
			break;
		case 2:
			b = do_byteswap_32(b);
			break;
		}
	} else if (swap < -1) {

	}
	put_mem_pcem(addr, b, 2);
}
static void REGPARAM2 s3virge_mb0_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	int swap = s3virgeaddr(pcibs, &addr);
	if (swap > 0) {
		if (swap == 1)
			b = do_byteswap_16(b);
	} else if (swap <= -1) {
		struct rtggfxboard *gb = getgfxboard(addr);
		int m = gb->lfbbyteswapmode;
		switch (m)
		{
		case 0:
		default:
			break;
		case 1:
			b = do_byteswap_16(b);
			break;
		}
	} else if (swap < -1) {

	}
	put_mem_pcem(addr, b, 1);
}
static void REGPARAM2 s3virge_mb0_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	int swap = s3virgeaddr(pcibs, &addr);
	put_mem_pcem(addr, b, 0);
}
static uae_u32 REGPARAM2 s3virge_mb0_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	int swap = s3virgeaddr(pcibs, &addr);
	uae_u32 v = get_mem_pcem(addr, 2);
	if (swap > 0) {
		if (swap == 1)
			v = do_byteswap_32(v);
	} else if (swap == -1) {
		struct rtggfxboard *gb = getgfxboard(addr);
		int m = gb->lfbbyteswapmode;
		switch (m)
		{
		case 0:
		default:
			break;
		case 1:
			v = (v >> 16) | (v << 16);
			v = do_byteswap_32(v);
			break;
		case 2:
			v = do_byteswap_32(v);
			break;
		}
	} else if (swap < -1) {

	}
	return v;
}
static uae_u32 REGPARAM2 s3virge_mb0_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	int swap = s3virgeaddr(pcibs, &addr);
	uae_u32 v = get_mem_pcem(addr, 1);
	if (swap > 0) {
		if (swap == 1)
			v = do_byteswap_16(v);
	} else if (swap == -1) {
		struct rtggfxboard *gb = getgfxboard(addr);
		int m = gb->lfbbyteswapmode;
		switch (m)
		{
		case 0:
		default:
			break;
		case 1:
			v = do_byteswap_16(v);
			break;
		}
	} else if (swap < -1) {

	}
	return v;
}
static uae_u32 REGPARAM2 s3virge_mb0_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	int swap = s3virgeaddr(pcibs, &addr);
	uae_u32 v = get_mem_pcem(addr, 0);
	return v;
}

static void REGPARAM2 s3virge_io_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_io_pcem(addr, b, 2);
}
static void REGPARAM2 s3virge_io_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_io_pcem(addr, b, 1);
}
static void REGPARAM2 s3virge_io_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	//write_log(_T("s3virge_io_bput(%08x,%02x) PC=%08x\n"), addr, b & 0xff, M68K_GETPC);
	put_io_pcem(addr, b, 0);
}
static uae_u32 REGPARAM2 s3virge_io_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_io_pcem(addr, 2);
}
static uae_u32 REGPARAM2 s3virge_io_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	return get_io_pcem(addr, 1);
}
static uae_u32 REGPARAM2 s3virge_io_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = get_io_pcem(addr, 0);
	//write_log(_T("s3virge_io_bget(%08x,%02x) PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
	return v;
}

static void REGPARAM2 matrox_io_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 2);
}
static void REGPARAM2 matrox_io_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 1);
}
static void REGPARAM2 matrox_io_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	put_mem_pcem(addr, b, 0);
}
static uae_u32 REGPARAM2 matrox_io_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 b = get_mem_pcem(addr, 2);
	return b;
}
static uae_u32 REGPARAM2 matrox_io_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u16 b = get_mem_pcem(addr, 1);
	return b;
}
static uae_u32 REGPARAM2 matrox_io_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = get_mem_pcem(addr, 0);
	return v;
}

static const struct pci_config matrox_pci_config =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }
};

static const struct pci_board matrox_pci_board2 =
{
	_T("MATROX"),
	&matrox_pci_config, NULL, NULL, NULL, NULL, NULL,
	{
		{ voodoo3_mb1_lget, voodoo3_mb1_wget, voodoo3_mb1_bget, voodoo3_mb1_lput, voodoo3_mb1_wput, voodoo3_mb1_bput },
		{ matrox_io_lget, matrox_io_wget, matrox_io_bget, matrox_io_lput, matrox_io_wput, matrox_io_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ voodoo3_bios_lget, voodoo3_bios_wget, voodoo3_bios_bget, NULL, NULL, NULL },
		{ NULL },
	},
	true,
	get_pci_pcem, put_pci_pcem, pci_change_config
};

static const struct pci_board matrox_pci_board =
{
	_T("MATROX"),
	&matrox_pci_config, NULL, NULL, NULL, NULL, NULL,
	{
		{ matrox_io_lget, matrox_io_wget, matrox_io_bget, matrox_io_lput, matrox_io_wput, matrox_io_bput },
		{ voodoo3_mb1_lget, voodoo3_mb1_wget, voodoo3_mb1_bget, voodoo3_mb1_lput, voodoo3_mb1_wput, voodoo3_mb1_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ voodoo3_bios_lget, voodoo3_bios_wget, voodoo3_bios_bget, NULL, NULL, NULL },
		{ NULL },
	},
	true,
	get_pci_pcem, put_pci_pcem, pci_change_config
};

static const struct pci_config s3virge_pci_config =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }
};

static const struct pci_board s3virge_pci_board =
{
	_T("S3VIRGE"),
	&s3virge_pci_config, NULL, NULL, NULL, NULL, NULL,
	{
		{ voodoo3_mb0_lget, voodoo3_mb0_wget, voodoo3_mb0_bget, voodoo3_mb0_lput, voodoo3_mb0_wput, voodoo3_mb0_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ voodoo3_bios_lget, voodoo3_bios_wget, voodoo3_bios_bget, NULL, NULL, NULL },
		{ s3virge_io_lget, s3virge_io_wget, s3virge_io_bget, s3virge_io_lput, s3virge_io_wput, s3virge_io_bput }
	},
	true,
	get_pci_pcem, put_pci_pcem, pci_change_config
};

static const struct pci_config s3trio_pci_config =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }
};

static const struct pci_board s3trio_pci_board =
{
	_T("S3TRIO"),
	&s3trio_pci_config, NULL, NULL, NULL, NULL, NULL,
	{
		{ voodoo3_mb0_lget, voodoo3_mb0_wget, voodoo3_mb0_bget, voodoo3_mb0_lput, voodoo3_mb0_wput, voodoo3_mb0_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ voodoo3_bios_lget, voodoo3_bios_wget, voodoo3_bios_bget, NULL, NULL, NULL },
		{ s3virge_io_lget, s3virge_io_wget, s3virge_io_bget, s3virge_io_lput, s3virge_io_wput, s3virge_io_bput }
	},
	true,
	get_pci_pcem, put_pci_pcem, pci_change_config
};

static const struct pci_config gd5446_pci_config =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }
};

static const struct pci_board gd5446_pci_board =
{
	_T("GD5446"),
	&gd5446_pci_config, NULL, NULL, NULL, NULL, NULL,
	{
		{ voodoo3_mb0_lget, voodoo3_mb0_wget, voodoo3_mb0_bget, voodoo3_mb0_lput, voodoo3_mb0_wput, voodoo3_mb0_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ voodoo3_bios_lget, voodoo3_bios_wget, voodoo3_bios_bget, NULL, NULL, NULL },
		{ s3virge_io_lget, s3virge_io_wget, s3virge_io_bget, s3virge_io_lput, s3virge_io_wput, s3virge_io_bput }
	},
	true,
	get_pci_pcem, put_pci_pcem, pci_change_config
};

int gfxboard_get_index_from_id(int id)
{
	if (id == GFXBOARD_UAE_Z2)
		return GFXBOARD_UAE_Z2;
	if (id == GFXBOARD_UAE_Z3)
		return GFXBOARD_UAE_Z3;
	const struct gfxboard *b = find_board(id);
	return (int)(b - &boards[0] + GFXBOARD_HARDWARE);
}

int gfxboard_get_id_from_index(int index)
{
	if (index == GFXBOARD_UAE_Z2)
		return GFXBOARD_UAE_Z2;
	if (index == GFXBOARD_UAE_Z3)
		return GFXBOARD_UAE_Z3;
	const struct gfxboard *b = &boards[index - GFXBOARD_HARDWARE];
	if (!b->id)
		return -1;
	return b->id;
}

const TCHAR *gfxboard_get_name(int type)
{
	if (type == GFXBOARD_UAE_Z2)
		return _T("UAE [Zorro II]");
	if (type == GFXBOARD_UAE_Z3)
		return _T("UAE [Zorro III]");
	const struct gfxboard *b = find_board(type);
	if (!b)
		return NULL;
	return b->name;
}

const TCHAR *gfxboard_get_manufacturername(int type)
{
	if (type == GFXBOARD_UAE_Z2)
		return NULL;
	if (type == GFXBOARD_UAE_Z3)
		return NULL;
	const struct gfxboard *b = find_board(type);
	if (!b)
		return NULL;
	return b->manufacturername;
}

const TCHAR *gfxboard_get_configname(int type)
{
	if (type == GFXBOARD_UAE_Z2)
		return _T("ZorroII");
	if (type == GFXBOARD_UAE_Z3)
		return _T("ZorroIII");
	const struct gfxboard *b = find_board(type);
	if (!b)
		return NULL;
	return b->configname;
}

struct gfxboard_func *gfxboard_get_func(struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type == GFXBOARD_UAE_Z2)
		return NULL;
	if (type == GFXBOARD_UAE_Z3)
		return NULL;
	const struct gfxboard *b = find_board(type);
	if (!b)
		return NULL;
	return b->func;
}

int gfxboard_get_configtype(struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type == GFXBOARD_UAE_Z2)
		return 2;
	if (type == GFXBOARD_UAE_Z3)
		return 3;
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	gb->board = find_board(type);
	return gb->board->configtype;
}

bool gfxboard_get_switcher(struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type < GFXBOARD_HARDWARE)
		return true;
	const struct gfxboard *b = find_board(type);
	return b->hasswitcher;
}

bool gfxboard_need_byteswap (struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type < GFXBOARD_HARDWARE)
		return false;
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	gb->board = find_board(type);
	return gb->board->swap;
}

int gfxboard_get_autoconfig_size(struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type == GFXBOARD_ID_PICASSO4_Z3)
		return 32 * 1024 * 1024;
	return -1;
}

int gfxboard_get_vram_min (struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type < GFXBOARD_HARDWARE)
		return -1;
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	gb->board = find_board(type);
	return gb->board->vrammin;
}

int gfxboard_get_vram_max (struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type < GFXBOARD_HARDWARE)
		return -1;
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	gb->board = find_board(type);
	return gb->board->vrammax;
}

int gfxboard_is_registers (struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type < 2)
		return 0;
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	gb->board = find_board(type);
	if (gb->board->model_extra > 0)
		return 2;
	return gb->board->model_registers > 0 ? 1 : 0;
}

int gfxboard_num_boards (struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type < 2)
		return 1;
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	gb->board = find_board(type);
	if (gb->board->id == GFXBOARD_ID_PICASSO4_Z2) {
		if (rbc->rtgmem_size < 0x400000)
			return 2;
		return 3;
	}
	if (gb->board->model_registers == 0)
		return 1;
	return 2;
}

uae_u32 gfxboard_get_romtype(struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type < 2)
		return 0;
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	gb->board = find_board(type);
	return gb->board->romtype;
}

static void gfxboard_init (struct autoconfig_info *aci, struct rtggfxboard *gb)
{
	struct uae_prefs *p = aci->prefs;
	gb->rtg_index = aci->devnum;
	if (!gb->automemory)
		gb->automemory = xmalloc (uae_u8, GFXBOARD_AUTOCONFIG_SIZE);
	memset (gb->automemory, 0xff, GFXBOARD_AUTOCONFIG_SIZE);
	struct rtgboardconfig *rbc = &p->rtgboards[gb->rtg_index];
	gb->rbc = rbc;
	gb->monitor_id = rbc->monitor_id;
	if (!aci->doinit)
		return;
	gb->gfxmem_bank = gfxmem_banks[gb->rtg_index];
	gb->gfxmem_bank->mask = gb->rbc->rtgmem_size - 1;
	gb->p4z2 = false;
	gb->banksize_mask = gb->board->banksize - 1;
	memset (gb->cirrus_pci, 0, sizeof gb->cirrus_pci);
	reset_pci (gb);
}

static void copyp4autoconfig (struct rtggfxboard *gb, int startoffset)
{
	int size = 0;
	int offset = 0;
	memset (gb->automemory, 0xff, 64);
	while (size < 32) {
		uae_u8 b = gb->p4autoconfig[size + startoffset];
		gb->automemory[offset] = b;
		offset += 2;
		size++;
	}
}

static void loadp4rom (struct rtggfxboard *gb)
{
	int size, offset;
	uae_u8 b;
	// rom loader code
	zfile_fseek (gb->p4rom, 256, SEEK_SET);
	offset = PICASSOIV_ROM_OFFSET;
	size = 0;
	while (size < 4096 - 256) {
		if (!zfile_fread (&b, 1, 1, gb->p4rom))
			break;
		gb->automemory[offset] = b;
		offset += 2;
		size++;
	}
	// main flash code
	zfile_fseek (gb->p4rom, 16384, SEEK_SET);
	zfile_fread (&gb->automemory[PICASSOIV_FLASH_OFFSET / 2], 1, PICASSOIV_MAX_FLASH, gb->p4rom);
	gb->p4flashrom = flash_new(gb->automemory, 131072, 131072, 0x01, 0x20, gb->p4rom, 0);
	write_log (_T("PICASSOIV: flash rom loaded\n"));
}

bool gfxboard_init_memory (struct autoconfig_info *aci)
{
	struct rtggfxboard *gb = &rtggfxboards[aci->devnum];
	int bank;
	uae_u8 z2_flags, z3_flags, type;
	struct uae_prefs *p = aci->prefs;
	uae_u8 flags = 0;
	bool ext_size = false;

	gfxboard_init (aci, gb);

	memset (gb->automemory, 0xff, GFXBOARD_AUTOCONFIG_SIZE);
	
	z2_flags = 0x05; // 1M
	z3_flags = 0;
	type = 0;
	bank = gb->board->banksize;
	if (bank <= 0x40000) {
		z2_flags = 0x00;
		while (bank >= 0x10000) {
			z2_flags++;
			bank /= 2;
		}
		type = z2_flags | 0xc0;
	} else {
		bank /= 0x00100000;
		if (bank >= 16) {
			ext_size = true;
			bank /= 16;
			while (bank > 1) {
				z3_flags++;
				bank >>= 1;
			}
		} else {
			while (bank > 1) {
				z2_flags++;
				bank >>= 1;
			}
			z2_flags &= 7;
		}
		if (gb->board->configtype == 3) {
			type = 0x80;
			flags |= 0x10;
			if (ext_size) {
				flags |= 0x20;
				type |= z3_flags;
			} else {
				type |= z2_flags;
			}
		} else {
			type = z2_flags | 0xc0;
		}
	}
	type |= gb->board->model_registers && !gb->board->model_extra ? 0x08 : 0x00;
	flags |= gb->board->er_flags;

	ew(gb, 0x04, gb->board->model_memory);
	ew(gb, 0x10, gb->board->manufacturer >> 8);
	ew(gb, 0x14, gb->board->manufacturer);

	uae_u32 ser = gb->board->serial;
	ew(gb, 0x18, ser >> 24); /* ser.no. Byte 0 */
	ew(gb, 0x1c, ser >> 16); /* ser.no. Byte 1 */
	ew(gb, 0x20, ser >>  8); /* ser.no. Byte 2 */
	ew(gb, 0x24, ser >>  0); /* ser.no. Byte 3 */

	ew(gb, 0x00, type);
	ew(gb, 0x08, flags);

	if (ISP4()) {
		int roms[] = { 91, -1 };
		struct romlist *rl = getromlistbyids(roms, NULL);
		TCHAR path[MAX_DPATH];
		get_rom_path (path, sizeof path / sizeof (TCHAR));

		if (rl) {
			gb->p4rom = flashromfile_open(rl->path);
		}

		if (!gb->p4rom && p->picassoivromfile[0] && zfile_exists(p->picassoivromfile))
			gb->p4rom = flashromfile_open(p->picassoivromfile);

		if (!gb->p4rom) {
			_tcscat (path, _T("picasso_iv_flash.rom"));
			gb->p4rom = flashromfile_open(path);
			if (!gb->p4rom)
				gb->p4rom = flashromfile_open(_T("picasso_iv_flash.rom"));
		}
		if (gb->p4rom) {
			zfile_fread (gb->p4autoconfig, sizeof gb->p4autoconfig, 1, gb->p4rom);
			copyp4autoconfig (gb, gb->board->configtype == 3 ? 192 : 0);
			if (gb->board->configtype == 3) {
				loadp4rom (gb);
				gb->p4_mmiobase = 0x200000;
				gb->p4_special_mask = 0x7fffff;
			} else {
				gb->p4z2 = true;
				gb->p4_mmiobase = 0x8000;
				gb->p4_special_mask = 0x1ffff;
			}
			gb->gfxboard_intena = 1;
		} else {
			error_log (_T("Picasso IV: '%s' flash rom image not found!\nAvailable from http://www.sophisticated-development.de/\nPIV_FlashImageXX -> picasso_iv_flash.rom"), path);
			gui_message (_T("Picasso IV: '%s' flash rom image not found!\nAvailable from http://www.sophisticated-development.de/\nPIV_FlashImageXX -> picasso_iv_flash.rom"), path);
		}
	}

	aci->label = gb->board->name;
	aci->direct_vram = true;
	aci->addrbank = &gb->gfxboard_bank_memory;
	if (gb->rbc->rtgmem_type == GFXBOARD_ID_VGA) {
		aci->zorro = -1;
		if (gb->monitor_id > 0) {
			gb->monswitch_keep_trying = true;
		}
	}
	if (gb->board->bustype > 0) {
		aci->zorro = -1;
	}
	aci->parent = aci;
	if (!aci->doinit) {
		if (gb->rbc->rtgmem_type == GFXBOARD_ID_ALTAIS_Z3) {
			aci->start = 0x20000000;
			aci->size = 0x1000000;
		} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_EGS_110_24) {
			aci->start = 0x0d000000;
			aci->size = 0x01000000;
		} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_VGA) {
			static const int parent[] = { ROMTYPE_A1060, ROMTYPE_A2088, ROMTYPE_A2088T, ROMTYPE_A2286, ROMTYPE_A2386, 0 };
			aci->parent_romtype = parent;
		} else if (gb->board->bustype == GFXBOARD_BUSTYPE_PCI) {
			static const int parent[] = { ROMTYPE_GREX, ROMTYPE_MEDIATOR, ROMTYPE_PROMETHEUS, ROMTYPE_PROMETHEUSFS, 0 };
			aci->parent_romtype = parent;
		} else {
			memcpy(aci->autoconfig_raw, gb->automemory, sizeof aci->autoconfig_raw);
		}
		return true;
	}

	gb->configured_mem = -1;
	gb->rtg_index = aci->devnum;

	total_active_gfx_boards = 0;
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		if (p->rtgboards[i].rtgmem_size && p->rtgboards[i].rtgmem_type >= GFXBOARD_HARDWARE) {
			total_active_gfx_boards++;
		}
	}
	only_gfx_board = NULL;
	if (total_active_gfx_boards == 1)
		only_gfx_board = gb;


	_sntprintf(gb->memorybankname, sizeof gb->memorybankname, _T("%s VRAM"), gb->board->name);
	_sntprintf(gb->memorybanknamenojit, sizeof gb->memorybanknamenojit, _T("%s VRAM NOJIT"), gb->board->name);
	_sntprintf(gb->wbsmemorybankname, sizeof gb->wbsmemorybankname, _T("%s VRAM WORDSWAP"), gb->board->name);
	_sntprintf(gb->lbsmemorybankname, sizeof gb->lbsmemorybankname, _T("%s VRAM LONGSWAP"), gb->board->name);
	_sntprintf(gb->regbankname, sizeof gb->regbankname, _T("%s REG"), gb->board->name);

	memcpy(&gb->gfxboard_bank_memory, &tmpl_gfxboard_bank_memory, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_wbsmemory, &tmpl_gfxboard_bank_wbsmemory, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_lbsmemory, &tmpl_gfxboard_bank_lbsmemory, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_nbsmemory, &tmpl_gfxboard_bank_nbsmemory, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_registers, &tmpl_gfxboard_bank_registers, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_registers2, &tmpl_gfxboard_bank_registers2, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_special, &tmpl_gfxboard_bank_special, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_memory_nojit, &tmpl_gfxboard_bank_memory_nojit, sizeof(addrbank));

#ifdef USE_PCEM
	memcpy(&gb->gfxboard_bank_vram_pcem, &tmpl_gfxboard_bank_vram_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_vram_normal_pcem, &tmpl_gfxboard_bank_vram_normal_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_vram_wordswap_pcem, &tmpl_gfxboard_bank_vram_wordswap_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_vram_longswap_pcem, &tmpl_gfxboard_bank_vram_longswap_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_vram_cv_1_pcem, &tmpl_gfxboard_bank_vram_cv_1_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_vram_p4z2_pcem, &tmpl_gfxboard_bank_vram_p4z2_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_io_swap_pcem, &tmpl_gfxboard_bank_io_swap_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_io_swap2_pcem, &tmpl_gfxboard_bank_io_swap2_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_io_pcem, &tmpl_gfxboard_bank_io_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_pci_pcem, &tmpl_gfxboard_bank_pci_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_mmio_pcem, &tmpl_gfxboard_bank_mmio_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_mmio_wbs_pcem, &tmpl_gfxboard_bank_mmio_wbs_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_mmio_lbs_pcem, &tmpl_gfxboard_bank_mmio_lbs_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_special_pcem, &tmpl_gfxboard_bank_special_pcem, sizeof(addrbank));
	memcpy(&gb->gfxboard_bank_bios, &tmpl_gfxboard_bank_bios, sizeof(addrbank));
#endif

	gb->gfxboard_bank_memory.name = gb->memorybankname;
	gb->gfxboard_bank_memory_nojit.name = gb->memorybanknamenojit;
	gb->gfxboard_bank_wbsmemory.name = gb->wbsmemorybankname;
	gb->gfxboard_bank_lbsmemory.name = gb->lbsmemorybankname;
	gb->gfxboard_bank_registers.name = gb->regbankname;
	gb->gfxboard_bank_registers2.name = gb->regbankname;

	gb->gfxboard_bank_memory.bget = gfxboard_bget_mem_autoconfig;
	gb->gfxboard_bank_memory.bput = gfxboard_bput_mem_autoconfig;
	gb->gfxboard_bank_memory.wput = gfxboard_wput_mem_autoconfig;

	gb->active = true;

	if (gb->board->bustype == GFXBOARD_BUSTYPE_PCI) {

		TCHAR path[MAX_DPATH];
		get_rom_path(path, sizeof path / sizeof(TCHAR));
		if (gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO3_PCI || gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO5_PCI)
			_tcscat(path, _T("voodoo3.rom"));
		else
			_tcscat(path, _T("s3virge.rom"));
		struct zfile *zf = read_rom_name(path, false);
		if (zf) {
			gb->bios = xcalloc(uae_u8, 65536);
			gb->bios_mask = 65535;
			int size = (int)zfile_fread(gb->bios, 1, 65536, zf);
			zfile_fclose(zf);
			write_log(_T("PCI RTG board BIOS load, %d bytes\n"), size);
		} else {
			error_log(_T("PCI RTG board BIOS ROM (%s) failed to load!\n"), path);
		}
		gb->gfxboard_bank_memory.bget = gfxboard_bget_mem;
		gb->gfxboard_bank_memory.bput = gfxboard_bput_mem;
		gb->gfxboard_bank_memory.wput = gfxboard_wput_mem;
		init_board(gb);
		copyvrambank(&gb->gfxboard_bank_memory, gb->gfxmem_bank, false);
		gb->configured_mem = 1;
		gb->configured_regs = 1;
		gb->lfbbar = 1;
		struct pci_bridge *b = pci_bridge_get();
		if (b) {
			if (gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO3_PCI || gb->rbc->rtgmem_type == GFXBOARD_ID_VOODOO5_PCI) {
				gb->pcibs = pci_board_add(b, &voodoo3_pci_board, -1, -1, aci, gb);
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_PERMEDIA2_PCI) {
				if (is_device_rom(p, ROMTYPE_GREX, 0) >= 0) {
					gb->pcibs = pci_board_add(b, &permedia2_pci_board, 0, -1, aci, gb);
				} else {
					gb->pcibs = pci_board_add(b, &permedia2_pci_board, -1, -1, aci, gb);
				}
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_GD5446_PCI) {
				gb->pcibs = pci_board_add(b, &gd5446_pci_board, -1, -1, aci, gb);
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_S3TRIO64_PCI) {
				gb->pcibs = pci_board_add(b, &s3trio_pci_board, -1, -1, aci, gb);
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_S3VIRGE_PCI) {
				gb->pcibs = pci_board_add(b, &s3virge_pci_board, -1, -1, aci, gb);
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MILLENNIUM_PCI) {
				gb->pcibs = pci_board_add(b, &matrox_pci_board, -1, -1, aci, gb);
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MILLENNIUM_II_PCI) {
				gb->pcibs = pci_board_add(b, &matrox_pci_board2, -1, -1, aci, gb);
				gb->lfbbar = 0;
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MYSTIQUE_PCI) {
				gb->pcibs = pci_board_add(b, &matrox_pci_board, -1, -1, aci, gb);
			} else if (gb->rbc->rtgmem_type == GFXBOARD_ID_MATROX_MYSTIQUE220_PCI) {
				gb->pcibs = pci_board_add(b, &matrox_pci_board2, -1, -1, aci, gb);
				gb->lfbbar = 0;
			}
		}
		gb->gfxboard_intena = 1;
		return true;
	}
	if (gb->board->bustype == GFXBOARD_BUSTYPE_DRACO) {
		gb->gfxboard_bank_memory.bget = gfxboard_bget_mem;
		gb->gfxboard_bank_memory.bput = gfxboard_bput_mem;
		gb->gfxboard_bank_memory.wput = gfxboard_wput_mem;
		uaecptr start = 0x20000000;
		gb->gfxboardmem_start = start + 0xc00000;
		init_board(gb);
		copyvrambank(&gb->gfxboard_bank_memory, gb->gfxmem_bank, true);
		copyvrambank(&gb->gfxboard_bank_vram_pcem, gb->gfxmem_bank, true);
		map_banks(&gb->gfxboard_bank_vram_pcem, gb->gfxboardmem_start >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16, 0);
		map_banks(&gb->gfxboard_bank_mmio_wbs_pcem, (start + 0xb00000) >> 16, 1, 0);
		map_banks(&gb->gfxboard_bank_special_pcem, (start + 0x000000) >> 16, 1, 0);
		gb->pcem_vram_offset = 0x800000;
		gb->pcem_vram_mask = 0x3fffff;
		gb->pcem_io_mask = 0x3fff;
		gb->pcem_mmio_offset = 0x00300000;
		gb->pcem_mmio_mask = 0xff;
		gb->configured_regs = gb->gfxmem_bank->start >> 16;
		gb->gfxboard_intena = 1;
		gb->configured_mem = 1;
		gb->configured_regs = 1;
	}
	if (gb->rbc->rtgmem_type == GFXBOARD_ID_EGS_110_24) {
		gb->gfxboard_bank_memory.bget = gfxboard_bget_mem;
		gb->gfxboard_bank_memory.bput = gfxboard_bput_mem;
		gb->gfxboard_bank_memory.wput = gfxboard_wput_mem;
		uaecptr start = 0x0d000000;
		gb->gfxboardmem_start = start;
		init_board(gb);
		copyvrambank(&gb->gfxboard_bank_memory, gb->gfxmem_bank, true);
		copyvrambank(&gb->gfxboard_bank_vram_pcem, gb->gfxmem_bank, true);
		map_banks(&gb->gfxboard_bank_vram_pcem, gb->gfxboardmem_start >> 16, gb->gfxboard_bank_vram_pcem.allocated_size >> 16, 0);
		map_banks(&gb->gfxboard_bank_special_pcem, 0xc080000 >> 16, 1, 0);
		gb->pcem_vram_offset = 0x800000;
		gb->pcem_vram_mask = gb->rbc->rtgmem_size - 1;
		gb->pcem_mmio_offset = 0x1000000;
		gb->pcem_mmio_mask = 0x3fff;
		gb->configured_regs = gb->gfxmem_bank->start >> 16;
		gb->configured_mem = 1;
		gb->configured_regs = 1;
	}

	if (gb->rbc->rtgmem_type == GFXBOARD_ID_VGA) {
		init_board(gb);
		gb->configured_mem = 1;
		gb->configured_regs = 1;
		return true;
	}

	return true;
}

bool gfxboard_init_memory_p4_z2 (struct autoconfig_info *aci)
{
	struct rtggfxboard *gb = &rtggfxboards[aci->devnum];
	if (gb->board->configtype == 3) {
		aci->addrbank = &expamem_null;
		return true;
	}

	copyp4autoconfig (gb, 64);
	memcpy(aci->autoconfig_raw, gb->automemory, sizeof aci->autoconfig_raw);
	aci->addrbank = &gb->gfxboard_bank_memory;
	aci->label = gb->board->name;
	aci->parent_of_previous = true;
	aci->direct_vram = true;

	if (!aci->doinit)
		return true;

	memcpy(&gb->gfxboard_bank_memory, &tmpl_gfxboard_bank_memory, sizeof(addrbank));
	gb->gfxboard_bank_memory.bget = gfxboard_bget_mem_autoconfig;
	gb->gfxboard_bank_memory.bput = gfxboard_bput_mem_autoconfig;
	return true;
}

static bool gfxboard_init_registersx(struct autoconfig_info *aci, int regnum)
{
	struct rtggfxboard *gb = &rtggfxboards[aci->devnum];
	int size;

	if (regnum && !gb->board->model_extra) {
		aci->addrbank = &expamem_null;
		return true;
	}
	if (!regnum && !gb->board->model_registers) {
		aci->addrbank = &expamem_null;
		return true;
	}

	memset (gb->automemory, 0xff, GFXBOARD_AUTOCONFIG_SIZE);
	if (gb->rbc->rtgmem_type == GFXBOARD_ID_PIXEL64 ||
		gb->rbc->rtgmem_type == GFXBOARD_ID_RETINA_Z2 || 
		gb->rbc->rtgmem_type == GFXBOARD_ID_GRAFFITY_Z2) {
		ew(gb, 0x00, 0xc0 | 0x02); // 128 Z2
		size = BOARD_REGISTERS_SIZE * 2;
	} else {
		ew(gb, 0x00, 0xc0 | 0x01); // 64k Z2
		size = BOARD_REGISTERS_SIZE;
	}

	ew (gb, 0x04, regnum ? gb->board->model_extra : gb->board->model_registers & 0xff);
	ew (gb, 0x10, gb->board->manufacturer >> 8);
	ew (gb, 0x14, gb->board->manufacturer);

	uae_u32 ser = gb->board->serial;
	ew (gb, 0x18, ser >> 24); /* ser.no. Byte 0 */
	ew (gb, 0x1c, ser >> 16); /* ser.no. Byte 1 */
	ew (gb, 0x20, ser >>  8); /* ser.no. Byte 2 */
	ew (gb, 0x24, ser >>  0); /* ser.no. Byte 3 */

	if (ISP4()) {
		uae_u8 v;
		copyp4autoconfig (gb, 128);
		loadp4rom (gb);
		if (aci->doinit) {
			v = (((gb->automemory[0] & 0xf0) | (gb->automemory[2] >> 4)) & 3) - 1;
			gb->gfxboard_bank_special.reserved_size = 0x10000 << v;
		}
	}

	memcpy(aci->autoconfig_raw, gb->automemory, sizeof aci->autoconfig_raw);
	aci->label = gb->board->name;

	aci->addrbank = regnum ? &gb->gfxboard_bank_registers2 : &gb->gfxboard_bank_registers;
	aci->parent_of_previous = true;

	if (!aci->doinit)
		return true;

	if (regnum) {
		gb->gfxboard_bank_registers2.bget = gfxboard_bget_regs_autoconfig;
		gb->gfxboard_bank_registers2.bput = gfxboard_bput_regs_autoconfig;
		gb->gfxboard_bank_registers2.reserved_size = size;
		gb->configured_regs = -2;
	} else {
		gb->gfxboard_bank_registers.bget = gfxboard_bget_regs_autoconfig;
		gb->gfxboard_bank_registers.bput = gfxboard_bput_regs_autoconfig;
		gb->gfxboard_bank_registers.reserved_size = size;
		gb->configured_regs = -1;
	}


	return true;
}

bool gfxboard_init_registers(struct autoconfig_info *aci)
{
	return gfxboard_init_registersx(aci, 0);
}
bool gfxboard_init_registers2(struct autoconfig_info *aci)
{
	return gfxboard_init_registersx(aci, 1);
}


static uae_u32 REGPARAM2 gfxboard_bget_bios(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->bios_mask;
	return gb->bios[addr];
}
static uae_u32 REGPARAM2 gfxboard_wget_bios(uaecptr addr)
{
	return (gfxboard_bget_bios(addr) << 8) | (gfxboard_bget_bios(addr + 1) << 0);
}
static uae_u32 REGPARAM2 gfxboard_lget_bios(uaecptr addr)
{
	return (gfxboard_bget_bios(addr + 0) << 24) | (gfxboard_bget_bios(addr + 1) << 16) | (gfxboard_bget_bios(addr + 2) << 8) | (gfxboard_bget_bios(addr + 3) << 0);
}

// PCem wrapper
#ifdef USE_PCEM
static uae_u32 REGPARAM2 gfxboard_bget_vram_normal_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u8 	v = pcem_linear_read_b(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_vram_normal_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u16 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	v = pcem_linear_read_w(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_vram_normal_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	v = pcem_linear_read_l(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	v = do_byteswap_32(v);
	return v;
}
static void REGPARAM2 gfxboard_bput_vram_normal_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_b(addr + pcem_mapping_linear_offset, b, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_wput_vram_normal_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_w(addr + pcem_mapping_linear_offset, w, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_lput_vram_normal_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	l = do_byteswap_32(l);
	pcem_linear_write_l(addr + pcem_mapping_linear_offset, l, pcem_mapping_linear_priv);
}


static uae_u32 REGPARAM2 gfxboard_bget_vram_longswap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u8 v = pcem_linear_read_b(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_vram_longswap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u16 v = pcem_linear_read_w(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	v = do_byteswap_16(v);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_vram_longswap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u32 v = pcem_linear_read_l(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	v = do_byteswap_32(v);
	return v;
}
static void REGPARAM2 gfxboard_bput_vram_longswap_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_b(addr + pcem_mapping_linear_offset, b, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_wput_vram_longswap_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	w = do_byteswap_16(w);
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_w(addr + pcem_mapping_linear_offset, w, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_lput_vram_longswap_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	l = do_byteswap_32(l);
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_l(addr + pcem_mapping_linear_offset, l, pcem_mapping_linear_priv);
}


static uae_u32 REGPARAM2 gfxboard_bget_vram_wordswap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr ^= 1;
	uae_u8 v = pcem_linear_read_b(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_vram_wordswap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u16 v = pcem_linear_read_w(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_vram_wordswap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u32 v = pcem_linear_read_l(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	v = (v >> 16) | (v << 16);
	return v;
}
static void REGPARAM2 gfxboard_bput_vram_wordswap_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	addr ^= 1;
	pcem_linear_write_b(addr + pcem_mapping_linear_offset, b, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_wput_vram_wordswap_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_w(addr + pcem_mapping_linear_offset, w, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_lput_vram_wordswap_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	l = (l >> 16) | (l << 16);
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_l(addr + pcem_mapping_linear_offset, l, pcem_mapping_linear_priv);
}



static uae_u32 REGPARAM2 gfxboard_bget_vram_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u8 	v = pcem_linear_read_b((addr & gb->pcem_vram_mask) + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_vram_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u16 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	if (addr & 0x800000) {
		;
	} else if (addr & 0x400000) {
		;
	} else {
		addr ^= 2;
	}
	v = pcem_linear_read_w((addr & gb->pcem_vram_mask) + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	if (addr & 0x800000) {
		v = do_byteswap_16(v);
	} else if (addr & 0x400000) {
		;
	} else {
		;
	}
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_vram_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	v = pcem_linear_read_l((addr & gb->pcem_vram_mask) + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	if (addr & 0x800000) {
		v = do_byteswap_32(v);
	} else if (addr & 0x400000) {
		v = (v >> 16) | (v << 16);
	} else {
		;
	}
	return v;
}
static void REGPARAM2 gfxboard_bput_vram_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_b(addr + pcem_mapping_linear_offset, b, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_wput_vram_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	if (addr & 0x800000) {
		w = do_byteswap_16(w);
	} else if (addr & 0x400000) {
		;
	} else {
		addr ^= 2;
	}
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_w(addr + pcem_mapping_linear_offset, w, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_lput_vram_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	if (addr & 0x800000) {
		l = do_byteswap_32(l);
	} else if (addr & 0x400000) {
		l = (l >> 16) | (l << 16);
	} else {
		;
	}
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_l(addr + pcem_mapping_linear_offset, l, pcem_mapping_linear_priv);
}

static uae_u32 REGPARAM2 gfxboard_bget_vram_cv_1_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	uae_u8 	v = pcem_linear_read_b(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_vram_cv_1_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u16 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	if (gb->device_data & 0x40) {
		addr ^= 2;
	} else if (gb->device_data & 0x20) {
		;
	}
	v = pcem_linear_read_w(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	if (gb->device_data & 0x40) {
		;
	} else if (gb->device_data & 0x20) {
		;
	} else {
		v = do_byteswap_16(v);
	}
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_vram_cv_1_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	v = pcem_linear_read_l(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	if (gb->device_data & 0x40) {
		;
	} else if (gb->device_data & 0x20) {
		v = (v >> 16) | (v << 16);
	} else {
		v = do_byteswap_32(v);
	}
	return v;
}
static void REGPARAM2 gfxboard_bput_vram_cv_1_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	pcem_linear_write_b(addr + pcem_mapping_linear_offset, b, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_wput_vram_cv_1_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	if (gb->device_data & 0x40) {
		addr ^= 2;
	} else if (gb->device_data & 0x20) {
		;
	} else {
		w = do_byteswap_16(w);
	}
	pcem_linear_write_w(addr + pcem_mapping_linear_offset, w, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_lput_vram_cv_1_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	addr &= gb->pcem_vram_mask;
	if (gb->device_data & 0x40) {
		;
	} else if (gb->device_data & 0x20) {
		l = (l >> 16) | (l << 16);
	} else {
		l = do_byteswap_32(l);
	}
	pcem_linear_write_l(addr + pcem_mapping_linear_offset, l, pcem_mapping_linear_priv);
}


static int p4z2swap(struct rtggfxboard *gb, uaecptr addr)
{
	uae_u32 offset;
	if (addr & 0x200000) {
		offset = gb->p4_vram_bank[1];
	} else {
		offset = gb->p4_vram_bank[0];
	}
	if (offset & 0x800000)
		return 1;
	if (offset & 0x400000)
		return 2;
	return 0;
}

static uae_u32 REGPARAM2 gfxboard_bget_vram_p4z2_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	addr += gb->pcem_vram_offset;
	int swap = p4z2swap(gb, addr);
	if (swap) {
		addr ^= 1;
	}
	uae_u8 	v = pcem_linear_read_b(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_vram_p4z2_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u16 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	v = pcem_linear_read_w(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	int swap = p4z2swap(gb, addr);
	if (swap == 0 || swap == 1) {
		v = do_byteswap_16(v);
	}
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_vram_p4z2_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	uae_u32 v;
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	v = pcem_linear_read_l(addr + pcem_mapping_linear_offset, pcem_mapping_linear_priv);
	int swap = p4z2swap(gb, addr);
	if (swap == 1) {
		v = do_byteswap_32(v);
	} else if (swap == 0) {
		v = (do_byteswap_16(v >> 16) << 0) | (do_byteswap_16(v >> 0) << 16);
	} else {
		v = (v >> 16) | (v << 16);
	}
	return v;
}
static void REGPARAM2 gfxboard_bput_vram_p4z2_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	int swap = p4z2swap(gb, addr);
	if (swap) {
		addr ^= 1;
	}
	pcem_linear_write_b(addr + pcem_mapping_linear_offset, b, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_wput_vram_p4z2_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	int swap = p4z2swap(gb, addr);
	if (swap == 0 || swap == 1) {
		w = do_byteswap_16(w);
	}
	pcem_linear_write_w(addr + pcem_mapping_linear_offset, w, pcem_mapping_linear_priv);
}
static void REGPARAM2 gfxboard_lput_vram_p4z2_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
	int swap = p4z2swap(gb, addr);
	if (swap == 1) {
		l = do_byteswap_32(l);
	} else if (swap == 0) {
		l = (do_byteswap_16(l >> 16) << 0) | (do_byteswap_16(l >> 0) << 16);
	} else {
		l = (l >> 16) | (l << 16);
	}
	pcem_linear_write_l(addr + pcem_mapping_linear_offset, l, pcem_mapping_linear_priv);
}

#endif


static uae_u32 REGPARAM2 gfxboard_lget_io_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	uae_u32 v = get_io_pcem(addr, 2);
	v = do_byteswap_32(v);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_io_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	uae_u16 v = get_io_pcem(addr, 1);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_bget_io_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	uae_u32 v = get_io_pcem(addr, 0);
	return v;
}
static void REGPARAM2 gfxboard_lput_io_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	l = do_byteswap_32(l);
	put_io_pcem(addr, l, 2);
	picassoiv_checkswitch(gb);
}
static void REGPARAM2 gfxboard_wput_io_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	put_io_pcem(addr, w, 1);
	picassoiv_checkswitch(gb);
}
static void REGPARAM2 gfxboard_bput_io_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	put_io_pcem(addr, b, 0);
	picassoiv_checkswitch(gb);
}

static uae_u32 REGPARAM2 gfxboard_lget_io_swap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	uae_u32 v = get_io_pcem(addr, 2);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_io_swap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	uae_u16 v = get_io_pcem(addr, 1);
	v = do_byteswap_16(v);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_bget_io_swap_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	uae_u32 v = get_io_pcem(addr, 0);
	return v;
}
static void REGPARAM2 gfxboard_lput_io_swap_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	put_io_pcem(addr, l, 2);
	picassoiv_checkswitch(gb);
}
static void REGPARAM2 gfxboard_wput_io_swap_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	w = do_byteswap_16(w);
	put_io_pcem(addr, w, 1);
	picassoiv_checkswitch(gb);
}
static void REGPARAM2 gfxboard_bput_io_swap_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	put_io_pcem(addr, b, 0);
	picassoiv_checkswitch(gb);
}


static uae_u32 REGPARAM2 gfxboard_lget_io_swap2_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	uae_u32 v = get_io_pcem(addr, 2);
	v = do_byteswap_32(v);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_io_swap2_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	addr ^= 2;
	uae_u16 v = get_io_pcem(addr, 1);
	v = do_byteswap_16(v);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_bget_io_swap2_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	addr ^= 3;
	uae_u32 v = get_io_pcem(addr, 0);
	return v;
}
static void REGPARAM2 gfxboard_lput_io_swap2_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	l = do_byteswap_32(l);
	put_io_pcem(addr, l, 2);
}
static void REGPARAM2 gfxboard_wput_io_swap2_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	addr ^= 2;
	w = do_byteswap_16(w);
	put_io_pcem(addr, w, 1);
}
static void REGPARAM2 gfxboard_bput_io_swap2_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_io_mask;
	addr ^= 3;
	put_io_pcem(addr, b, 0);
}


void put_pci_pcem(uaecptr addr, uae_u8 v);
uae_u8 get_pci_pcem(uaecptr addr);

static uae_u32 REGPARAM2 gfxboard_bget_pci_pcem(uaecptr addr)
{
	addr ^= 3;
	uae_u8 v = get_pci_pcem(addr & 0xfff);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_pci_pcem(uaecptr addr)
{
	uae_u16 v;
	v = gfxboard_bget_pci_pcem(addr) << 8;
	v |= gfxboard_bget_pci_pcem(addr + 1) << 0;
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_pci_pcem(uaecptr addr)
{
	uae_u32 v;
	v = gfxboard_wget_pci_pcem(addr) << 16;
	v |= gfxboard_wget_pci_pcem(addr + 2) << 0;
	return v;
}
static void REGPARAM2 gfxboard_bput_pci_pcem(uaecptr addr, uae_u32 b)
{
	addr ^= 3;
	put_pci_pcem(addr & 0xfff, b);
}
static void REGPARAM2 gfxboard_wput_pci_pcem(uaecptr addr, uae_u32 w)
{
	gfxboard_bput_pci_pcem(addr, w >> 8);
	gfxboard_bput_pci_pcem(addr + 1, w & 0xff);
}
static void REGPARAM2 gfxboard_lput_pci_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	gfxboard_wput_pci_pcem(addr, l >> 16);
	gfxboard_wput_pci_pcem(addr + 2, l & 0xffff);
}


static uae_u32 REGPARAM2 gfxboard_bget_mmio_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	addr ^= 3;
	uae_u8 v = get_mem_pcem(addr, 0);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_mmio_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	addr ^= 2;
	uae_u16 v;
	v = get_mem_pcem(addr, 1);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_mmio_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	uae_u32 v;
	v = get_mem_pcem(addr, 2);
	return v;
}
static void REGPARAM2 gfxboard_bput_mmio_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	addr ^= 3;
#if 0
	write_log(_T("gfxboard_bput_mmio_wbs_pcem(%08x, %08x)\n"), addr, b);
#endif
	put_mem_pcem(addr, b, 0);
}
static void REGPARAM2 gfxboard_wput_mmio_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	addr ^= 2;
#if 0
	write_log(_T("gfxboard_wput_mmio_wbs_pcem(%08x, %08x)\n"), addr, w);
#endif
	put_mem_pcem(addr, w, 1);
}
static void REGPARAM2 gfxboard_lput_mmio_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
#if 0
	write_log(_T("gfxboard_lput_mmio_wbs_pcem(%08x, %08x)\n"), addr, l);
#endif
	put_mem_pcem(addr, l, 2);
}


static uae_u32 REGPARAM2 gfxboard_bget_mmio_wbs_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	uae_u8 v = get_mem_pcem(addr, 0);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_mmio_wbs_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	uae_u16 v;
	v = get_mem_pcem(addr, 1);
	v = do_byteswap_16(v);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_mmio_wbs_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	uae_u32 v;
	v = get_mem_pcem(addr, 2);
	v = do_byteswap_32(v);
	return v;
}
static void REGPARAM2 gfxboard_bput_mmio_wbs_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
#if 0
	write_log(_T("gfxboard_bput_mmio_wbs_pcem(%08x, %08x)\n"), addr, b);
#endif
	put_mem_pcem(addr, b, 0);
}
static void REGPARAM2 gfxboard_wput_mmio_wbs_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
#if 0
	write_log(_T("gfxboard_wput_mmio_wbs_pcem(%08x, %08x)\n"), addr, w);
#endif
	w = do_byteswap_16(w);
	put_mem_pcem(addr, w, 1);
}
static void REGPARAM2 gfxboard_lput_mmio_wbs_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
#if 0
	write_log(_T("gfxboard_lput_mmio_wbs_pcem(%08x, %08x)\n"), addr, l);
#endif
	l = do_byteswap_32(l);
	put_mem_pcem(addr, l, 2);
}


static uae_u32 REGPARAM2 gfxboard_bget_mmio_lbs_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	uae_u8 v = get_mem_pcem(addr, 0);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_mmio_lbs_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	uae_u16 v;
	v = get_mem_pcem(addr, 1);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_mmio_lbs_pcem(uaecptr addr)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
	uae_u32 v;
	v = get_mem_pcem(addr, 2);
	return v;
}
static void REGPARAM2 gfxboard_bput_mmio_lbs_pcem(uaecptr addr, uae_u32 b)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
#if 0
	write_log(_T("gfxboard_bput_mmio_wbs_pcem(%08x, %08x)\n"), addr, b);
#endif
	put_mem_pcem(addr, b, 0);
}
static void REGPARAM2 gfxboard_wput_mmio_lbs_pcem(uaecptr addr, uae_u32 w)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
#if 0
	write_log(_T("gfxboard_wput_mmio_wbs_pcem(%08x, %08x)\n"), addr, w);
#endif
	put_mem_pcem(addr, w, 1);
}
static void REGPARAM2 gfxboard_lput_mmio_lbs_pcem(uaecptr addr, uae_u32 l)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	addr &= gb->pcem_mmio_mask;
	addr += gb->pcem_mmio_offset;
#if 0
	write_log(_T("gfxboard_lput_mmio_wbs_pcem(%08x, %08x)\n"), addr, l);
#endif
	put_mem_pcem(addr, l, 2);
}

static uae_u8 get_io_merlin(struct rtggfxboard *gb, uae_u32 addr)
{
	uae_u8 v = 0;

	if (addr < 0x5000) {
		v = get_io_pcem(addr, 0);
	}

	// 2M vs 4M config bits
	if (addr == 0x3ca) {
		v &= ~0x0c;
		if (gb->gfxboard_bank_vram_pcem.allocated_size >= 0x400000) {
			v |= 4;
		}
	} else if (addr == 0x3c2) {
		v |= 0x20;
	}
	// serial eeprom?
	if (addr == 0x81 || addr == 0x01) {
		gb->extradata[2] >>= 1;
		gb->extradata[1]++;
		if (gb->extradata[1] <= 10) {
			if (addr == 0x81) {
				gb->extradata[2] |= 0x200;
			}
			if (gb->extradata[1] == 10) {
				v = 0;
				uae_u16 a = gb->extradata[2];
				uae_u8 aa = a >> 3;
				uae_u8 d = 0xff;
				if ((a & 7) == 3) {
					uae_u8 ser[4] = { uae_u8(gb->extradata[0] >> 24), uae_u8(gb->extradata[0] >> 16), uae_u8(gb->extradata[0] >> 8), uae_u8(gb->extradata[0] >> 0) };
					if (aa == 0x7c) {
						d = ser[0];
					} else if (aa == 0x7d) {
						d = ser[1];
					} else if (aa == 0x7b) {
						d = ser[2];
					} else if (aa == 0x7e) {
						d = ser[3];
					} else if (aa == 0x7f) {
						// checksum
						d = 0;
						for (int i = 0; i < 0x7f; i++) {
							if (i == 0x7c) {
								d += ser[0];
							} else if (i == 0x7d) {
								d += ser[1];
							} else if (i == 0x7b) {
								d += ser[2];
							} else if (i == 0x7e) {
								d += ser[3];
							} else {
								d += 0xff;
							}
						}
						d = 0x100 - d;
					}
				}
				gb->extradata[3] = d;
				write_log("Merlin serial eeprom read address %04x (%02x) = %02x\n", a, aa, d);
			}
		} else {
			v = ((gb->extradata[3] >> 7) & 1) ? 2 : 0;
			gb->extradata[3] <<= 1;
		}
	}
	if (addr == 0x0401) {
		gb->extradata[2] = 0;
		gb->extradata[1] = 0;
	}

	return v;
}

static void special_pcem_put(uaecptr addr, uae_u32 v, int size)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int boardnum = gb->rbc->rtgmem_type;

#if SPCDEBUG
	//if ((addr & 0xfffff) != 0x3da)
		write_log(_T("PCEM SPECIAL PUT %08x %08x %d PC=%08x\n"), addr, v, size, M68K_GETPC);
#endif

	if (boardnum == GFXBOARD_ID_ALTAIS_Z3) {

		if ((addr & 0xffff) < 0x100) {
#ifdef WITH_DRACO
			draco_bustimeout(addr);
#endif
			return;
		}
		addr &= 0xffff;
		if (size == 2) {
			gfxboard_lput_io_swap_pcem(addr, v);
		} else if (size == 1) {
			gfxboard_wput_io_swap_pcem(addr, v);
		} else {
			gfxboard_bput_io_swap_pcem(addr, v);
		}

	} else if (boardnum == GFXBOARD_ID_RETINA_Z2) {
	
		addr &= 0x1ffff;
		if (addr & 0x10000) {
			// VRAM banks
			uaecptr mem = addr & 0xffff;
			if (size == 2) {
				v = do_byteswap_32(v);
			} else if (size == 1) {
				v = do_byteswap_16(v);
			}
			put_mem_pcem(mem + 0xa0000, v, size);
		} else if (addr & 0x8000) {
			// RAMDAC
			int dac = (addr & 15) >> 1;
			if (dac == 6) {
				put_io_pcem(0x3c6, v, 0);
			} else if (dac == 0) {
				put_io_pcem(0x3c8, v, 0);
			} else if (dac == 1) {
				put_io_pcem(0x3c9, v, 0);
			}
		} else {
			// IO
			int io = addr & 0x3fff;
			if (!(addr & 0x4000)) {
				io++;
			}
			put_io_pcem(io, v, 0);
		}

	} else if (boardnum == GFXBOARD_ID_PIXEL64) {

		addr &= 0xffff;
		if (size) {
			put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
			put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
		} else if (size == 0) {
			put_io_pcem(addr, v & 0xff, 0);
		}

	} else if (boardnum == GFXBOARD_ID_OMNIBUS_ET4000) {

		addr &= 0xffff;
		if (size) {
			put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
			put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
		} else if (size == 0) {
			put_io_pcem(addr, v & 0xff, 0);
		}

	} else if (boardnum == GFXBOARD_ID_CV643D_Z2) {

		uaecptr addr2 = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
		uaecptr addrd = addr2 + gb->gfxboardmem_start;
		if (gb->pcem_pci_configured) {
			if (addr2 >= 0x3c0000 && addr2 < 0x3d0000) {
				addrd &= 0x3fff;
				if (size == 1) {
					addrd ^= 2;
				} else {
					addrd ^= 3;
				}
				put_io_pcem(addrd, v, size);
				return;
			} else if (addr2 == 0x3a0000 + 4) {
				// Z2 IO/MMIO (What does this do?)
				gb->pcem_data[0] = v;
				return;
			} else if (addr2 == 0x3a0000 + 8) {
				// Z2 endian select
				gb->pcem_data[1] = v;
				gb->pcem_vram_offset = (v & 0xc0) << 16;
				return;
			} else if (addr2 == 0x3a0000 + 0) {
				if (size == 1) {
					set_monswitch(gb, (v & 1) == 0);
				}
				return;
			} else if (addr2 == 0x3a0000 + 12) {
				if (size == 1) {
					gb->gfxboard_intena = (v & 2) != 0;
				}
				return;
			}
		} else {
			if (addr2 >= 0x3c0000 && addr2 < 0x3c8000) {
				addrd &= 0xfff;
				if (size == 0)
					gfxboard_bput_pci_pcem(addrd, v);
				else if (size == 1)
					gfxboard_wput_pci_pcem(addrd, v);
				else
					gfxboard_lput_pci_pcem(addrd, v);
				if (size == 2) {
					gb->pcem_pci_configured = true;
				}
				return;
			}
		}
		if (addr2 >= 0x3c8000 && addr2 < 0x3d0000) {
			if (size == 0)
				gfxboard_bput_mmio_pcem(addrd, v);
			else if (size == 1)
				gfxboard_wput_mmio_pcem(addrd, v);
			else
				gfxboard_lput_mmio_pcem(addrd, v);
		} else if (addr2 >= 0x3e8000 && addr2 < 0x3f0000) {
			if (size == 0)
				gfxboard_bput_mmio_pcem(addrd, v);
			else if (size == 1)
				gfxboard_wput_mmio_pcem(addrd, v);
			else
				gfxboard_lput_mmio_pcem(addrd, v);
		} else if (addr2 >= 0x3e0000 && addr2 < 0x3e8000) {
			if (size == 0)
				gfxboard_bput_mmio_wbs_pcem(addrd, v);
			else if (size == 1)
				gfxboard_wput_mmio_wbs_pcem(addrd, v);
			else
				gfxboard_lput_mmio_wbs_pcem(addrd, v);
		}

	} else if (boardnum == GFXBOARD_ID_CV64_Z3) {
		addr &= 0xffff;
		if (addr == 1) {
			// bit 3 = interrupt enable
			// bit 4 = switcher
			// bit 5 = byteswap (1 = word swap)
			// bit 6 = byteswap (1 = long swap)
			// bit 7 = interrupt level (0=6, 1 = 2)
			if (size == 0) {
				set_monswitch(gb, (v & 0x10) != 0);
				gb->gfxboard_intena = (v & 0x08) != 0;
				if (gb->gfxboard_intena) {
					if (v & 0x80)
						gb->gfxboard_intena = 2;
					else
						gb->gfxboard_intena = 6;
				}
				gb->device_data = v;
			}
		}

	} else if (boardnum == GFXBOARD_ID_CV643D_Z3) {
		addr &= 0xffff;
		if (addr == 0) {
			if (size == 1) {
				set_monswitch(gb, (v & 1) == 0);
			}
		} else if (addr == 12) {
			if (size == 1) {
				gb->gfxboard_intena = (v & 2) != 0;
			}
		}

	} else if (boardnum == GFXBOARD_ID_PICASSO2 || boardnum == GFXBOARD_ID_PICASSO2PLUS) {
		// CL54xx
		addr &= 0xffff;
		if (addr & 0x8000) {
			if (size == 0) {
				write_log(_T("GFX SPECIAL BPUT IO %08X = %02X\n"), addr, v & 0xff);
				if ((addr & 0x2001) == 0x2000) {
					int idx = addr >> 12;
					if (idx == 0x0b || idx == 0x09) {
						set_monswitch(gb, false);
					} else if (idx == 0x0a || idx == 0x08) {
						set_monswitch(gb, true);
					}
				}
				if (boardnum == GFXBOARD_ID_PICASSO2PLUS) {
					if ((addr & 0x1001) == 0x1000) {
						gb->gfxboard_intena = false;
					}
					if ((addr & 0x1001) == 0x1001) {
						gb->gfxboard_intena = true;
					}
				}
			}
		} else {
			if (addr & 0x1000) {
				addr |= 1;
			}
			addr &= 0x0fff;
			if (size == 1) {
				put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
				put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
			} else if (size == 0) {
				put_io_pcem(addr, v & 0xff, 0);
			}
		}
	} else if (boardnum == GFXBOARD_ID_PICCOLO_Z3 || boardnum == GFXBOARD_ID_PICCOLO_Z2 || boardnum == GFXBOARD_ID_SPECTRUM_Z3 || boardnum == GFXBOARD_ID_SPECTRUM_Z2
		|| boardnum == GFXBOARD_ID_SD64_Z2 || boardnum == GFXBOARD_ID_SD64_Z3) {
		if ((addr & 0x8001) == 0x8000) {
			set_monswitch(gb, (v & 0x20) != 0);
			gb->gfxboard_intena = (v & 0x40) != 0;
		} else {
			addr &= 0x0fff;
			if (size == 1) {
				put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
				put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
			} else if (size == 0) {
				put_io_pcem(addr, v & 0xff, 0);
			} else if (size == 2) {
				put_io_pcem(addr + 0, (v >> 24) & 0xff, 0);
				put_io_pcem(addr + 1, (v >> 16) & 0xff, 0);
				put_io_pcem(addr + 2, (v >> 8) & 0xff, 0);
				put_io_pcem(addr + 3, (v >> 0) & 0xff, 0);
			}
		}
	} else if (boardnum == GFXBOARD_ID_PICASSO4_Z3 || boardnum == GFXBOARD_ID_PICASSO4_Z2) {

		addr = (addr - gb->p4_special_start) & gb->p4_special_mask;

#if PICASSOIV_DEBUG_IO > 1
		write_log(_T("PicassoIV CL REG PUT %08x %02x PC=%08x\n"), addr, v, M68K_GETPC);
#endif

		if ((addr >= 0x400000 && addr < 0xa00000) || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
			uae_u32 addr2 = addr & 0xffff;
			if (addr2 >= 0x0800 && addr2 < 0x840) {
				addr2 -= 0x800;
				if (size == 2) {
					gb->p4_pci[addr2 + 0] = v >> 24;
					gb->p4_pci[addr2 + 1] = v >> 16;
					gb->p4_pci[addr2 + 2] = v >>  8;
					gb->p4_pci[addr2 + 3] = v >>  0;
					p4_pci_check(gb);
				}
#if PICASSOIV_DEBUG_IO
				write_log(_T("PicassoIV PCI PUT %08x %08x %d\n"), addr, v, size);
#endif
			} else if (addr2 >= 0x1000 && addr2 < 0x1040) {
				addr2 -= 0x1000;
				if (size == 0) {
					gfxboard_bput_pci_pcem(addr2 ^ 3, v);
				} else if (size == 1) {
					gfxboard_wput_pci_pcem(addr2 ^ 3, v);
				} else {
					gfxboard_lput_pci_pcem(addr2 ^ 3, v);
				}
				//gb->cirrus_pci[addr2] = b;
//				reset_pci(gb);
#if PICASSOIV_DEBUG_IO
				write_log(_T("PicassoIV CL PCI PUT %08x %08x %d\n"), addr, v, size);
#endif
			}
			return;
		}
		if (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) {
			if (size == 0) {
				if (addr == 0x404) {
					gb->picassoiv_flifi = v;
					picassoiv_checkswitch(gb);
				} else if (addr == 0x406) {
					gb->p4i2c = v;
				}
			}
		}
		if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
			// memory mapped io
			if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
#if PICASSOIV_DEBUG_IO
				write_log(_T("PicassoIV MMIO PUT %08x %08x %d PC=%08x\n"), addr, v, size, M68K_GETPC);
#endif
				addr -= gb->p4_mmiobase;
				if (gb->p4_revb) {
					if (addr < 0x100) {
						goto end;
					} else {
						addr -= 0x100;
					}
				}
				if (size == 2) {
					v = do_byteswap_32(v);
				} else if (size == 1) {
					v = do_byteswap_16(v);
				}
				put_mem_pcem(addr + gb->pcem_mmio_offset, v, size);
				picassoiv_checkswitch(gb);
//				uae_u32 addr2 = addr - gb->p4_mmiobase;
//				gb->vgammio->write(&gb->vga, addr2, b, 1);
				goto end;
			}
		}
		if (gb->p4z2 && addr >= 0x10000 && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH)) {
			addr -= 0x10000;
			if (size == 2) {
				v = do_byteswap_32(v);
			} else if (size == 1) {
				v = do_byteswap_16(v);
			}
			put_io_pcem(addr, v, size);
			picassoiv_checkswitch(gb);
//			addr = mungeaddr(gb, addr, true);
//			if (addr) {
//				gb->vgaio->write(&gb->vga, addr, b & 0xff, 1);
//				bput_regtest(gb, addr, b);
				//write_log(_T("P4 VGA write %08x=%02x PC=%08x\n"), addr, b & 0xff, M68K_GETPC);
//			}
			goto end;
		}
		if (!(gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH)) {
			if (addr >= PICASSOIV_FLASH_OFFSET / 2) {
				addr /= 2;
				addr += ((gb->picassoiv_bank & PICASSOIV_BANK_FLASHBANK) ? PICASSOIV_FLASH_BANK : 0);
				flash_write(gb->p4flashrom, addr, v);
			}
		}

#if PICASSOIV_DEBUG_IO
		write_log(_T("PicassoIV BPUT %08x %08X %d\n"), addr, v, size);
#endif
		if (addr == 0) {
			if (size == 0) {
				gb->picassoiv_bank = v;
			}
		}
	end:;

	} else if (boardnum == GFXBOARD_ID_VISIONA) {

		addr &= 0xffff;
		if (!(addr & (0x2000 | 0x40000))) {
			gfxboard_lput_mmio_pcem(addr, v);
		}
		if (addr == 0x204c) {
			gb->gfxboard_intena = (v & 1) != 0;
		}
		if (addr == 0x2054) {
			gb->gfxboard_intreq_marked = false;
		}

	} else if (boardnum == GFXBOARD_ID_DOMINO) {

		addr &= 0xffff;
		if (addr & 0x1000) {
			addr++;
		}
		addr &= 0xfff;
		if (size) {
			put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
			put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
		} else if (size == 0) {
			put_io_pcem(addr, v & 0xff, 0);
		}

	} else if (boardnum == GFXBOARD_ID_MERLIN_Z2 || boardnum == GFXBOARD_ID_MERLIN_Z3) {

		addr &= 0xffff;

		if (addr == 0x401) {
			set_monswitch(gb, (v & 0x01) != 0);
			gb->extradata[2] = 0;
		}
		if (addr < 0x5000) {
			if (size) {
				put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
				put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
			} else if (size == 0) {
				put_io_pcem(addr, v & 0xff, 0);
			}
		}

	} else if (boardnum == GFXBOARD_ID_OMNIBUS_ET4000W32) {

		addr &= 0xffff;

		if (size) {
			put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
			put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
		} else if (size == 0) {
			put_io_pcem(addr, v & 0xff, 0);
		}

	} else if (boardnum == GFXBOARD_ID_EGS_110_24) {

		addr &= 0xffff;
		if (!(addr & (0x1000 | 0x2000 | 0x40000))) {
			gfxboard_lput_mmio_pcem(addr, v);
		}
		if (addr == 0x2050) {
			gb->gfxboard_intena = (v & 1) != 0;
		}
		if (addr == 0x2058) {
			gb->gfxboard_intreq_marked = false;
		}

	} else if (boardnum == GFXBOARD_ID_RAINBOWIII) {

		addr &= 0xffff;
		if (!(addr & (0x2000 | 0x40000))) {
			gfxboard_lput_mmio_pcem(addr, v);
		}
		if (addr == 0x2000) {
			gb->gfxboard_intena = (v & 0x20) != 0;
		}
		if (addr == 0x2008) {
			gb->gfxboard_intreq_marked = false;
		}

	} else if (boardnum == GFXBOARD_ID_GRAFFITY_Z2 || boardnum == GFXBOARD_ID_GRAFFITY_Z3) {

		if (boardnum == GFXBOARD_ID_GRAFFITY_Z3) {
			if (addr & 0x400000) {
				if ((addr & 0x60) == 0x60) {
					set_monswitch(gb, true);
				} else if ((addr & 0x60) == 0x40) {
					set_monswitch(gb, false);
				}
				return;
			}
		} else {
			if (addr & 0x8000) {
				if ((addr & 0x60) == 0x60) {
					set_monswitch(gb, true);
				} else if ((addr & 0x60) == 0x40) {
					set_monswitch(gb, false);
				}
				return;
			}
		}

		addr &= 0xffff;
		if (addr < 0x4000) {
			if (size == 1) {
				put_io_pcem(addr + 0, (v >> 8) & 0xff, 0);
				put_io_pcem(addr + 1, (v >> 0) & 0xff, 0);
			} else if (size == 0) {
				put_io_pcem(addr, v & 0xff, 0);
			} else if (size == 2) {
				put_io_pcem(addr + 0, (v >> 24) & 0xff, 0);
				put_io_pcem(addr + 1, (v >> 16) & 0xff, 0);
				put_io_pcem(addr + 2, (v >> 8) & 0xff, 0);
				put_io_pcem(addr + 3, (v >> 0) & 0xff, 0);
			}
		}
	}
}

static uae_u32 special_pcem_get(uaecptr addr, int size)
{
	struct rtggfxboard *gb = getgfxboard(addr);
	int boardnum = gb->rbc->rtgmem_type;
	uae_u32 v = 0;

#if SPCDEBUG
	//if ((addr & 0xfffff) != 0x3da)
		write_log(_T("PCEM SPECIAL GET %08x %d PC=%08x\n"), addr, size, M68K_GETPC);
#endif

	if (boardnum == GFXBOARD_ID_ALTAIS_Z3) {

		if ((addr & 0xffff) < 0x100) {
#ifdef WITH_DRACO
			draco_bustimeout(addr);
#endif
			return v;
		}
		addr &= 0xffff;
		if (size == 2) {
			v = gfxboard_lget_io_swap_pcem(addr);
		} else if (size == 1) {
			v = gfxboard_wget_io_swap_pcem(addr);
		} else {
			v = gfxboard_bget_io_swap_pcem(addr);
		}

	} else if (boardnum == GFXBOARD_ID_RETINA_Z2) {

		addr &= 0x1ffff;
		if (addr & 0x10000) {
			// VRAM banks
			uaecptr mem = addr & 0xffff;
			v = get_mem_pcem(mem + 0xa0000, size);
			if (size == 2) {
				v = do_byteswap_32(v);
			} else if (size == 1) {
				v = do_byteswap_16(v);
			}
		} else if (addr & 0x8000) {
			// RAMDAC
			int dac = (addr & 15) >> 1;
			if (dac == 6) {
				v = get_io_pcem(0x3c6, 0);
			} else if (dac == 0) {
				v = get_io_pcem(0x3c8, 0);
			} else if (dac == 1) {
				v = get_io_pcem(0x3c9, 0);
			}
		} else {
			// IO
			int io = addr & 0x3fff;
			if (!(addr & 0x4000)) {
				io++;
			}
			v = get_io_pcem(io, 0);
		}

	} else if (boardnum == GFXBOARD_ID_PIXEL64) {

		if (size) {
			v = get_io_pcem(addr + 0, 0) << 8;
			v |= get_io_pcem(addr + 1, 0) << 0;
		} else if (size == 0) {
			v = get_io_pcem(addr, 0);
		}

	} else if (boardnum == GFXBOARD_ID_OMNIBUS_ET4000) {

		if (size) {
			v = get_io_pcem(addr + 0, 0) << 8;
			v |= get_io_pcem(addr + 1, 0) << 0;
		} else if (size == 0) {
			v = get_io_pcem(addr, 0);
		}

	} else if (boardnum == GFXBOARD_ID_CV643D_Z2) {

		uaecptr addr2 = (addr - gb->gfxboardmem_start) & gb->banksize_mask;
		uaecptr addrd = addr2 + gb->gfxboardmem_start;
		if (gb->pcem_pci_configured) {
			// -> IO
			if (addr2 >= 0x3c0000 && addr2 < 0x3c8000) {
				addrd &= 0x3fff;
				if (size == 0) {
					addrd ^= 3;
				} else if (size == 1) {
					addrd ^= 2;
				}
				v = get_io_pcem(addrd, size);
			}
		} else {
			if (addr2 >= 0x3c0000 && addr2 < 0x3c8000) {
				if (size == 0)
					v = gfxboard_bget_pci_pcem(addrd);
				else if (size == 1)
					v = gfxboard_wget_pci_pcem(addrd);
				else
					v = gfxboard_lget_pci_pcem(addrd);
			}
		}
		if (addr2 >= 0x3c8000 && addr2 < 0x3d0000) {
			if (size == 0)
				v = gfxboard_bget_mmio_pcem(addrd);
			else if (size == 1)
				v = gfxboard_wget_mmio_pcem(addrd);
			else
				v = gfxboard_lget_mmio_pcem(addrd);
		} else if (addr2 >= 0x3e8000 && addr2 < 0x3f0000) {
			if (size == 0)
				v = gfxboard_bget_mmio_pcem(addrd);
			else if (size == 1)
				v = gfxboard_wget_mmio_pcem(addrd);
			else
				v = gfxboard_lget_mmio_pcem(addrd);
		} else if (addr2 >= 0x3e0000 && addr2 < 0x3e8000) {
			if (size == 0)
				v = gfxboard_bget_mmio_wbs_pcem(addrd);
			else if (size == 1)
				v = gfxboard_wget_mmio_wbs_pcem(addrd);
			else
				v = gfxboard_lget_mmio_wbs_pcem(addrd);
		}

	} else if (boardnum == GFXBOARD_ID_CV64_Z3) {

		v = 0;

	} else if (boardnum == GFXBOARD_ID_CV643D_Z3) {

		v = 0;

	} else if (boardnum == GFXBOARD_ID_PICASSO2 || boardnum == GFXBOARD_ID_PICASSO2PLUS) {
		// CL54xx
		addr &= 0xffff;
		if (addr & 0x8000) {
			v = 0;
		} else {
			if (addr & 0x1000)
				addr |= 1;
			addr &= 0x0fff;
			if (size == 1) {
				v = get_io_pcem(addr + 0, 0) << 8;
				v |= get_io_pcem(addr + 1, 0) << 0;
			} else if (size == 0) {
				v = get_io_pcem(addr, 0);
			}
		}
	} else if (boardnum == GFXBOARD_ID_PICCOLO_Z3 || boardnum == GFXBOARD_ID_PICCOLO_Z2 || boardnum == GFXBOARD_ID_SPECTRUM_Z3 || boardnum == GFXBOARD_ID_SPECTRUM_Z2
		|| boardnum == GFXBOARD_ID_SD64_Z2 || boardnum == GFXBOARD_ID_SD64_Z3) {
		addr &= 0x0fff;
		if (size == 1) {
			v = get_io_pcem(addr + 0, 0) << 8;
			v |= get_io_pcem(addr + 1, 0) << 0;
		} else if (size == 0) {
			v = get_io_pcem(addr, 0);
		} else if (size == 2) {
			v = get_io_pcem(addr + 0, 0) << 24;
			v |= get_io_pcem(addr + 1, 0) << 16;
			v |= get_io_pcem(addr + 2, 0) << 8;
			v |= get_io_pcem(addr + 3, 0) << 0;
		}
	} else if (boardnum == GFXBOARD_ID_PICASSO4_Z3 || boardnum == GFXBOARD_ID_PICASSO4_Z2) {

		addr = (addr - gb->p4_special_start) & gb->p4_special_mask;

		// pci config
		if (addr >= 0x400000 || (gb->p4z2 && !(gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) && (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) && ((addr >= 0x800 && addr < 0xc00) || (addr >= 0x1000 && addr < 0x2000)))) {
			uae_u32 addr2 = addr & 0xffff;
			v = 0;
			if (addr2 >= 0x0800 && addr2 < 0x840) {
				if (addr2 == 0x802) {
					v = 2; // bridge version?
				} else if (addr2 == 0x808) {
					v = 4; // bridge revision
				} else {
					addr2 -= 0x800;
					v = gb->p4_pci[addr2];
				}
#if PICASSOIV_DEBUG_IO
				write_log(_T("PicassoIV PCI GET %08x %02x\n"), addr, v);
#endif
			} else if (addr2 >= 0x1000 && addr2 <= 0x1040) {
				if (size == 0) {
					v = gfxboard_bget_pci_pcem(addr2 ^ 3);
				} else if (size == 1) {
					v = gfxboard_wget_pci_pcem(addr2 ^ 3);
				} else {
					v = gfxboard_lget_pci_pcem(addr2 ^ 3);
				}
				//	addr2 -= 0x1000;
				//	v = gb->cirrus_pci[addr2];
#if PICASSOIV_DEBUG_IO
				write_log(_T("PicassoIV CL PCI GET %08x %02x\n"), addr, v);
#endif
			}
			return v;
		}

		if (gb->picassoiv_bank & PICASSOIV_BANK_MAPRAM) {
			// memory mapped io
			if (addr >= gb->p4_mmiobase && addr < gb->p4_mmiobase + 0x8000) {
				addr -= gb->p4_mmiobase;
				if (gb->p4_revb) {
					if (addr < 0x100) {
						return 0;
					} else {
						addr -= 0x100;
					}
				}
				v = get_mem_pcem(addr + gb->pcem_mmio_offset, size);
				if (size == 2) {
					v = do_byteswap_32(v);
				} else if (size == 1) {
					v = do_byteswap_16(v);
				}
//				uae_u32 addr2 = addr - gb->p4_mmiobase;
//				v = gb->vgammio->read(&gb->vga, addr2, 1);
#if PICASSOIV_DEBUG_IO
				write_log(_T("PicassoIV MMIO GET %08x %02x\n"), addr, v & 0xff);
#endif
				return v;
			}
		}
		if (addr == 0) {
			v = gb->picassoiv_bank;
			return v;
		}
		if (gb->picassoiv_bank & PICASSOIV_BANK_UNMAPFLASH) {
			v = 0;
			if (addr == 0x404) {
				v = 0x70; // FLIFI revision
				// FLIFI type in use
				if (currprefs.chipset_mask & CSMASK_AGA)
					v |= 4 | 8;
				else
					v |= 8;
			} else if (addr == 0x406) {
				// FLIFI I2C
				// bit 0 = clock out
				// bit 1 = data out
				// bit 2 = clock in
				// bit 7 = data in
				v = gb->p4i2c & 3;
				if (v & 1)
					v |= 4;
				if (v & 2)
					v |= 0x80;
			} else if (addr == 0x408) {
				v = gb->gfxboard_intreq ? 0x80 : 0;
			} else if (gb->p4z2 && addr >= 0x10000) {
				addr -= 0x10000;
				v = get_io_pcem(addr, size);
				if (size == 2) {
					v = do_byteswap_32(v);
				} else if (size == 1) {
					v = do_byteswap_16(v);
				}
//				uaecptr addr2 = mungeaddr(gb, addr, true);
//				if (addr2) {
//					v = gb->vgaio->read(&gb->vga, addr2, 1);
//					v = bget_regtest(gb, addr2, v);
					//write_log(_T("P4 VGA read %08X=%02X PC=%08x\n"), addr2, v, M68K_GETPC);
//				}
				//write_log (_T("PicassoIV IO %08x %02x\n"), addr, v);
				return v;
			}
#if PICASSOIV_DEBUG_IO
			if (addr != 0x408)
				write_log(_T("PicassoIV BGET %08x %02x\n"), addr, v);
#endif
		} else {
			if (addr < PICASSOIV_FLASH_OFFSET / 2) {
				v = gb->automemory[addr];
				return v;
			}
			addr /= 2;
			addr += (gb->picassoiv_bank & PICASSOIV_BANK_FLASHBANK) ? PICASSOIV_FLASH_BANK : 0;
			v = flash_read(gb->p4flashrom, addr);
		}

	} else if (boardnum == GFXBOARD_ID_VISIONA) {

		addr &= 0xffff;
		if (!(addr & (0x2000 | 0x40000))) {
			if (size == 2) {
				v = gfxboard_lget_mmio_pcem(addr);
			} else if (size == 1) {
				v = gfxboard_wget_mmio_pcem(addr);
			} else {
				v = gfxboard_bget_mmio_pcem(addr);
			}
		}
		if (addr == 0x2044) {
			v = gb->gfxboard_intreq_marked ? 4 : 0;
		}

	} else if (boardnum == GFXBOARD_ID_DOMINO) {

		addr &= 0xffff;
		if (addr & 0x1000) {
			addr++;
		}
		addr &= 0xfff;
		if (size) {
			v = get_io_pcem(addr + 0, 0) << 8;
			v |= get_io_pcem(addr + 1, 0) << 0;
		} else if (size == 0) {
			v = get_io_pcem(addr, 0);
		}

	} else if (boardnum == GFXBOARD_ID_MERLIN_Z2 || boardnum == GFXBOARD_ID_MERLIN_Z3) {

		addr &= 0xffff;
		if (size) {
			v = get_io_merlin(gb, addr + 0) << 8;
			v |= get_io_merlin(gb, addr + 1) << 0;
		} else if (size == 0) {
			v = get_io_merlin(gb, addr);
		}

	} else if (boardnum == GFXBOARD_ID_OMNIBUS_ET4000W32) {

		addr &= 0xffff;
		if (size) {
			v = get_io_pcem(addr + 0, 0) << 8;
			v |= get_io_pcem(addr + 1, 0) << 0;
		} else if (size == 0) {
			v = get_io_pcem(addr, 0);
		}

	} else if (boardnum == GFXBOARD_ID_EGS_110_24) {

		addr &= 0xffff;
		if (!(addr & (0x2000 | 0x40000))) {
			if (size == 2) {
				v = gfxboard_lget_mmio_pcem(addr);
			} else if (size == 1) {
				v = gfxboard_wget_mmio_pcem(addr);
			} else {
				v = gfxboard_bget_mmio_pcem(addr);
			}
		} else if (addr == 0x2044) {
			v = gb->gfxboard_intreq_marked ? 4 : 0;
		} else if (addr == 0x204c) {
			v = 0xf0ffffff;
		} else if (addr == 0x207c) {
			v = 0xf9ffffff;
		}

	} else if (boardnum == GFXBOARD_ID_RAINBOWIII) {

		addr &= 0xffff;
		if (!(addr & (0x2000 | 0x40000))) {
			if (size == 2) {
				v = gfxboard_lget_mmio_pcem(addr);
			} else if (size == 1) {
				v = gfxboard_wget_mmio_pcem(addr);
			} else {
				v = gfxboard_bget_mmio_pcem(addr);
			}
		}
		if (addr & 0x4000) {
			v = gb->gfxboard_intreq_marked ? 0x80 : 00;
		}
	
	} else if (boardnum == GFXBOARD_ID_GRAFFITY_Z2 || boardnum == GFXBOARD_ID_GRAFFITY_Z3) {

		addr &= 0xffff;
		if (addr < 0x1000) {
			if (size == 1) {
				v = get_io_pcem(addr + 0, 0) << 8;
				v |= get_io_pcem(addr + 1, 0) << 0;
			} else if (size == 0) {
				v = get_io_pcem(addr, 0);
			} else if (size == 2) {
				v = get_io_pcem(addr + 0, 0) << 24;
				v |= get_io_pcem(addr + 1, 0) << 16;
				v |= get_io_pcem(addr + 2, 0) << 8;
				v |= get_io_pcem(addr + 3, 0) << 0;
			}
		}
	}

	return v;
}

static uae_u32 REGPARAM2 gfxboard_bget_special_pcem(uaecptr addr)
{
	uae_u32 v = special_pcem_get(addr, 0);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_wget_special_pcem(uaecptr addr)
{
	uae_u32 v = special_pcem_get(addr, 1);
	return v;
}
static uae_u32 REGPARAM2 gfxboard_lget_special_pcem(uaecptr addr)
{
	uae_u32 v = special_pcem_get(addr, 2);
	return v;
}

static void REGPARAM2 gfxboard_bput_special_pcem(uaecptr addr, uae_u32 b)
{
	special_pcem_put(addr, b, 0);
}
static void REGPARAM2 gfxboard_wput_special_pcem(uaecptr addr, uae_u32 w)
{
	special_pcem_put(addr, w, 1);
}
static void REGPARAM2 gfxboard_lput_special_pcem(uaecptr addr, uae_u32 l)
{
	special_pcem_put(addr, l, 2);
}

void *pcem_getvram(int size)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtggfxboard *gb = &rtggfxboards[i];
		if (gb->pcemdev) {
			return gb->vram;
		}
	}
	return NULL;
}
int pcem_getvramsize(void)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtgboardconfig *c = &currprefs.rtgboards[i];
		int type = c->rtgmem_type;
		if (type >= GFXBOARD_HARDWARE) {
			const struct gfxboard *gfxb = find_board(type);
			if (gfxb->pcemdev) {
				return c->rtgmem_size;
			}
		}
	}
	return 0;
}


bool gfxboard_isgfxboardscreen(int monid)
{
	int index = rtg_visible[monid];
	if (index < 0)
		return false;
	struct rtgboardconfig *rbc = &currprefs.rtgboards[index];
	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	return gb->active && rtg_visible[monid] >= 0;
}
uae_u8 *gfxboard_getrtgbuffer(int monid, int *widthp, int *heightp, int *pitch, int *depth, uae_u8 *palette)
{
	int index = rtg_visible[monid];
	if (index < 0)
		return NULL;
	struct rtgboardconfig *rbc = &currprefs.rtgboards[index];

	if (rbc->rtgmem_type < GFXBOARD_HARDWARE) {
		return uaegfx_getrtgbuffer(monid, widthp, heightp, pitch, depth, palette);
	}

	struct rtggfxboard *gb = &rtggfxboards[rbc->rtg_index];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	struct picasso96_state_struct *state = &picasso96_state[monid];

	if (!gb->pcemdev)
		return NULL;
#ifdef USE_PCEM
	int width = state->VirtualWidth;
	int height = state->VirtualHeight;
	if (!width || !height)
		return NULL;

	uae_u8 *dst = xmalloc(uae_u8, width * height * 4);
	if (!dst)
		return NULL;

	uae_u8 *d = dst;
	for (int yy = 0; yy < height; yy++) {
		uae_u8 *s = getpcembuffer32(32, 0, yy);
		memcpy(d, s, width * 4);
		d += width * 4;
	}

	*widthp = width;
	*heightp = height;
	*pitch = width * 4;
	*depth = 4 * 8;

	return dst;
#else
	return NULL;
#endif
}
void gfxboard_freertgbuffer(int monid, uae_u8 *dst)
{
	int index = rtg_visible[monid];
	if (index < 0) {
		uaegfx_freertgbuffer(monid, dst);
		return;
	}
	struct rtgboardconfig *rbc = &currprefs.rtgboards[index];

	if (rbc->rtgmem_type < GFXBOARD_HARDWARE) {
		uaegfx_freertgbuffer(monid, dst);
		return;
	}

	xfree(dst);
}
