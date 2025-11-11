/*
* UAE - The Un*x Amiga Emulator
*
* Arcadia
* American Laser games
* Cubo
* Picmatic
*
* Copyright 2005-2024 Toni Wilen
*
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "newcpu.h"
#include "debug.h"
#include "arcadia.h"
#include "zfile.h"
#ifdef VIDEOGRAB
#include "videograb.h"
#endif
#include "xwin.h"
#include "statusline.h"
#include "flashrom.h"
#include "savestate.h"
#include "devices.h"

#define CUBO_DEBUG 1


/* supported roms
*
* (mame 0.90)
*
* - ar_airh
* - ar_bowl
* - ar_fast
* - ar_ldrb
* - ar_ldrba
* - ar_ninj
* - ar_rdwr
* - ar_socc (99u8)
* - ar_sdwr
* - ar_spot
* - ar_sprg
* - ar_xeon
*
* mame 0.152
*
* - ar_blast
* - ar_dlta
* - ar_pm
*
* mame 0.153
*
* - ar_argh

*/

static void multigame (int);

int arcadia_flag, arcadia_coin[2];
struct arcadiarom *arcadia_bios, *arcadia_game;
static int arcadia_hsync_cnt;

static struct arcadiarom roms[]	= {

	// oneplay 2.11
	{ 49, _T("ar_bios.zip"), _T("scpa_01_v2.11"), _T("scpa_01"),ARCADIA_BIOS, 3, 6, 1, 0, 2, 3, 4, 5, 7, _T("_v2.11."), { _T("u12"), _T("u16") } },
	// tenplay 2.11
	{ 50, _T("ar_bios.zip"), _T("gcp"), _T("gcp-"),				ARCADIA_BIOS, 7, 7, 6, 5, 4, 3, 2, 1, 0 },
	// oneplay 2.20
	{ 75, _T("ar_bios.zip"), _T("scpa_01_v2.20"), _T("scpa_01"),ARCADIA_BIOS, 3, 6, 1, 0, 2, 3, 4, 5, 7, _T("_v2.20."), { _T("u12"), _T("u16") } },
	// oneplay 3.00
	{ 51, _T("ar_bios.zip"), _T("scpa_01_v3.00"), _T("scpa_01"),ARCADIA_BIOS, 3, 6, 1, 0, 2, 3, 4, 5, 7, _T("_v3.0."), { _T("u12"), _T("u16") } },
	// tenplay 3.11
	{ 76, _T("ar_bios.zip"), _T("gcp_v11"), _T("gcp_v311_"),	ARCADIA_BIOS, 7, 7, 6, 5, 4, 3, 2, 1, 0, NULL, { _T(".u16"), _T(".u11"), _T(".u17"), _T(".u12") } },
	// tenplay 4.00
	{ 77, _T("ar_bios.zip"), _T("gcp_v400"), _T("gcp_v400_"),	ARCADIA_BIOS, 7, 7, 6, 5, 4, 3, 2, 1, 0, NULL, { _T(".u16"), _T(".u11"), _T(".u17"), _T(".u12") } },

	{ 33, _T("ar_airh.zip"), _T("airh"), _T("airh_"),			ARCADIA_GAME, 5|16, 5, 0, 2, 4, 7, 6, 1, 3 },
	{ 81, _T("ar_airh2.zip"), _T("airh2"), _T("arcadia4"),		ARCADIA_GAME, 0, 5, 0, 2, 4, 7, 6, 1, 3, NULL, { _T(".u10"), _T(".u6") } },
	{ 34, _T("ar_bowl.zip"), _T("bowl"), _T("bowl_"),			ARCADIA_GAME, 5|16, 7, 6, 0, 1, 2, 3, 4, 5 },
	{ 35, _T("ar_dart.zip"), _T("dart"), _T("dart_"),			ARCADIA_GAME, 5|16, 4, 0, 7, 6, 3, 1, 2, 5 },
	{ 82, _T("ar_dart2.zip"), _T("dart2"), _T("arcadia3"),		ARCADIA_GAME, 0, 4, 0, 7, 6, 3, 1, 2, 5, NULL, { _T(".u10"), _T(".u6"), _T(".u11"), _T(".u7"), _T(".u12"), _T(".u8"), _T(".u13"), _T(".u9"), _T(".u19"), _T(".u15"), _T(".u20"), _T(".u16") } },
	{ 36, _T("ar_fast.zip"), _T("fast-v28"), _T("fast-v28_"),	ARCADIA_GAME, 7, 7, 6, 5, 4, 3, 2, 1, 0, NULL, { _T(".u11"), _T(".u15"), _T(".u10"), _T(".u14"), _T(".u9"), _T(".u13"), _T(".u20"), _T(".u24"), _T(".u19"), _T(".u23"), _T(".u18"), _T(".u22"), _T(".u17"), _T(".u21"), _T(".u28"), _T(".u32")  } },
	{ 83, _T("ar_fasta.zip"), _T("fast-v27"), _T("fast-v27_"),	ARCADIA_GAME, 7, 7, 6, 5, 4, 3, 2, 1, 0, NULL, { _T(".u11"), _T(".u15"), _T(".u10"), _T(".u14"), _T(".u9"), _T(".u13"), _T(".u20"), _T(".u24"), _T(".u19"), _T(".u23"), _T(".u18"), _T(".u22"), _T(".u17"), _T(".u21"), _T(".u28"), _T(".u32")  } },
	{ 37, _T("ar_ldrba.zip"), _T("ldra"), _T("leader_board_0"),	ARCADIA_GAME, 7, 7, 6, 5, 4, 3, 2, 1, 0, _T("_v2.4."), { _T("u11"), _T("u15"), _T("u10"), _T("u14"), _T("u9"), _T("u13"), _T("u20"), _T("u24"), _T("u19"), _T("u23"), _T("u18"), _T("u22"), _T("u17"), _T("u21"), _T("u28"), _T("u32")  } },
	{ 38, _T("ar_ldrbb.zip"),_T("ldrbb"), _T("ldrb_"),			ARCADIA_GAME, 5, 7, 6, 5, 4, 3, 2, 1, 0, NULL, { _T(".u11"), _T("_gcp_22.u15"), _T(".u10"), _T(".u14"), _T(".u9"), _T(".u13"), _T(".u20"), _T(".u24"), _T(".u19"), _T(".u23"), _T(".u18"), _T(".u22"), _T(".u17"), _T(".u21"), _T(".u28"), _T(".u32")  } },
	{ 86, _T("ar_ldrb.zip"),_T("ldrb"), _T("leader_board_0"),	ARCADIA_GAME, 7, 2, 3, 4, 1, 0, 7, 5, 6, _T("_v2.5."), { _T("u11"), _T("u15"), _T("u10"), _T("u14"), _T("u9"), _T("u13"), _T("u20"), _T("u24"), _T("u19"), _T("u23"), _T("u18"), _T("u22"), _T("u17"), _T("u21"), _T("u28"), _T("u32")  } },
	{ 39, _T("ar_ninj.zip"), _T("ninj"), _T("ninj_"),			ARCADIA_GAME, 5|16, 1, 6, 5, 7, 4, 2, 0, 3 },
	{ 84, _T("ar_ninj2.zip"), _T("ninj2"), _T("arcadia5"),		ARCADIA_GAME, 0, 1, 6, 5, 7, 4, 2, 0, 3, NULL, { _T(".u10"), _T(".u6"), _T(".u11"), _T(".u7"), _T(".u12"), _T(".u8"), _T(".u13"), _T(".u9"), _T(".u19"), _T(".u15"), _T(".u20"), _T(".u16") } },
	{ 40, _T("ar_rdwr.zip"), _T("rdwr"), _T("rdwr_"),			ARCADIA_GAME, 5|16, 3, 1, 6, 4, 0, 5, 2, 7 },
	{ 41, _T("ar_sdwr.zip"), _T("sdwr"), _T("sdwr_"),			ARCADIA_GAME, 5|16, 6, 3, 4, 5, 2, 1, 0, 7 },
	{ 85, _T("ar_sdwr2.zip"), _T("sdwr2"), _T("arcadia1"),		ARCADIA_GAME, 0, 6, 3, 4, 5, 2, 1, 0, 7, NULL, { _T(".u10"), _T(".u6"), _T(".u11"), _T(".u7"), _T(".u12"), _T(".u8"), _T(".u13"), _T(".u9"), _T(".u19"), _T(".u15"), _T(".u20"), _T(".u16") } },
	{ 42, _T("ar_spot.zip"), _T("spotv2"), _T("spotv2."),		ARCADIA_GAME, 5, 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 43, _T("ar_sprg.zip"), _T("sprg"), _T("sprg_"),			ARCADIA_GAME, 5|16, 4, 7, 3, 0, 6, 5, 2, 1 },
	{ 44, _T("ar_xeon.zip"), _T("xeon"), _T("xeon_"),			ARCADIA_GAME, 5|16, 3, 1, 2, 4, 0, 5, 6, 7 },
	{ 45, _T("ar_socc.zip"), _T("socc30"), _T("socc30."),		ARCADIA_GAME, 6, 0, 7, 1, 6, 5, 4, 3, 2 },
	{ 78, _T("ar_blast.zip"), _T("blsb-v2-1"),_T("blsb-v2-1_"),	ARCADIA_GAME, 7|16, 4, 1, 7, 6, 2, 0, 3, 5 },
	{ 79, _T("ar_dlta.zip"),  _T("dlta_v3"), _T("dlta_v3_"),	ARCADIA_GAME, 7|16, 4, 1, 7, 6, 2, 0, 3, 5 },
	{ 80, _T("ar_pm.zip"), _T("pm"), _T("pm-"),					ARCADIA_GAME, 2|4|16, 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 88, _T("ar_argh.zip"), _T("argh"), _T("argh-"),			ARCADIA_GAME, 7, 5, 0, 2, 4, 7, 6, 1, 3, _T("-11-28-87"), { _T(".u12"), _T(".u16"), _T(".u11"), _T(".u15"), _T(".u10"), _T(".u14"), _T(".u9"), _T(".u13"), _T(".u20"), _T(".u24"), _T(".u19"), _T(".u23"), _T(".u18"), _T(".u22"), _T(".u17"), _T(".u21"), _T(".u28"), _T(".u32"), _T(".u27"), _T(".u31"), _T(".u26"), _T(".u30"), _T(".u25"), _T(".u29") } },

	{ -1 }
};

static uae_u8 *arbmemory, *arbbmemory;
static int boot_read;

#define	arb_start 0x800000
#define	arb_mask 0x1fffff
#define	allocated_arbmemory 0x200000

#define arbb_start 0xf00000
#define	arbb_mask 0x7ffff
#define	allocated_arbbmemory 0x80000

#define	nvram_offset 0x1fc000
#define	bios_offset 0x180000
#define	NVRAM_SIZE 0x4000

static int nvwrite;

static int load_rom8 (const TCHAR *xpath, uae_u8 *mem, int extra, const TCHAR *ext, const TCHAR **exts)
{
	struct zfile *zf;
	TCHAR path[MAX_DPATH];
	int i;
	uae_u8 *tmp = xmalloc (uae_u8, 131072);
	const TCHAR *bin = (extra & 16) ? _T(".bin") : _T("");

	extra &= 3;
	memset (tmp, 0xff, 131072);
	_sntprintf (path, sizeof path, _T("%s%s%s"), xpath, extra == 3 ? _T("-hi") : (extra == 2 ? _T("hi") : (extra == 1 ? _T("h") : _T(""))), bin);
	if (ext)
		_tcscat(path, ext);
	if (exts) {
		if (exts[0] == NULL)
			goto end;
		_tcscat (path, exts[0]);
	}
	//write_log (_T("%s\n"), path);
	zf = zfile_fopen (path, _T("rb"), ZFD_NORMAL);
	if (!zf)
		goto end;
	if (zfile_fread (tmp, 65536, 1, zf) == 0)
		goto end;
	zfile_fclose (zf);
	_sntprintf (path, sizeof path, _T("%s%s%s"), xpath, extra == 3 ? _T("-lo") : (extra == 2 ? _T("lo") : (extra == 1 ? _T("l") : _T(""))), bin);
	if (ext)
		_tcscat(path, ext);
	if (exts)
		_tcscat (path, exts[1]);
	//write_log (_T("%s\n"), path);
	zf = zfile_fopen (path, _T("rb"), ZFD_NORMAL);
	if (!zf)
		goto end;
	if (zfile_fread (tmp + 65536, 65536, 1, zf) == 0)
		goto end;
	zfile_fclose (zf);
	for (i = 0; i < 65536; i++) {
		mem[i * 2 + 0] = tmp[i];
		mem[i * 2 + 1] = tmp[i + 65536];
	}
	xfree (tmp);
	return 1;
end:
	xfree (tmp);
	return 0;
}

static struct arcadiarom *is_arcadia (const TCHAR *xpath, int cnt)
{
	TCHAR path[MAX_DPATH], *p;
	struct arcadiarom *rom = NULL;
	int i;

	_tcscpy (path, xpath);
	p = path;
	for (i = uaetcslen (xpath) - 1; i > 0; i--) {
		if (path[i] == '\\' || path[i] == '/') {
			path[i++] = 0;
			p = path + i;
			break;
		}
	}
	for (i = 0; roms[i].romid > 0; i++) {
		if (!_tcsicmp (p, roms[i].name) || !_tcsicmp (p, roms[i].romid1)) {
			if (cnt > 0) {
				cnt--;
				continue;
			}
			rom = &roms[i];
			break;
		}
	}
	if (!rom)
		return 0;
	return rom;
}

static int load_roms (struct arcadiarom *rom)
{
	TCHAR path[MAX_DPATH], path3[MAX_DPATH], *p;
	int i, offset;
	TCHAR *xpath;

	offset = 0;
	if (rom->type == ARCADIA_BIOS) {
		xpath = currprefs.romextfile;
		offset = bios_offset;
	} else {
		xpath = currprefs.cartfile;
	}

	_tcscpy (path3, xpath);
	p = path3 + _tcslen (path3) - 1;
	while (p > path3) {
		if (p[0] == '\\' || p[0] == '/') {
			*p = 0;
			break;
		}
		p--;
	}
	if (p == path3)
		*p = 0;

	xpath = path3;
	_tcscat (path3, FSDB_DIR_SEPARATOR_S);
	_tcscat (path3, rom->romid2);

	i = 0;
	for (;;) {
		if (rom->extra & 4)
			_sntprintf (path, sizeof path, _T("%s%d"), xpath, i + 1);
		else
			_tcscpy (path, xpath);
		if (!load_rom8 (path, arbmemory + 2 * 65536 * i + offset, rom->extra, rom->ext, rom->exts && rom->exts[0] ? &rom->exts[i * 2] : NULL)) {
			if (i == 0)
				write_log (_T("Arcadia: %s rom load failed ('%s')\n"), rom->type == ARCADIA_BIOS ? _T("bios") : _T("game"), path);
			break;
		}
		i++;
	}
	if (i == 0)
		return 0;
	write_log (_T("Arcadia: %s rom %s loaded\n"), rom->type == ARCADIA_BIOS ? _T("bios") : _T("game"), xpath);
	return 1;
}

static uae_u8 bswap (uae_u8 v,int b7,int b6,int	b5,int b4,int b3,int b2,int b1,int b0)
{
	uae_u8 b = 0;

	b |= ((v >> b7) & 1) << 7;
	b |= ((v >> b6) & 1) << 6;
	b |= ((v >> b5) & 1) << 5;
	b |= ((v >> b4) & 1) << 4;
	b |= ((v >> b3) & 1) << 3;
	b |= ((v >> b2) & 1) << 2;
	b |= ((v >> b1) & 1) << 1;
	b |= ((v >> b0) & 1) << 0;
	return b;
}

static void decrypt_roms (struct arcadiarom *rom)
{
	int i, start = 1, end = 0x20000;

	if (rom->type == ARCADIA_BIOS) {
		start += bios_offset;
		end += bios_offset;
	}
	for (i = start; i < end; i += 2) {
		arbmemory[i] = bswap (arbmemory[i],
			rom->b7,rom->b6,rom->b5,rom->b4,rom->b3,rom->b2,rom->b1,rom->b0);
		if (rom->extra == 2)
			arbmemory[i - 1] = bswap (arbmemory[i - 1],7,6,5,4,3,2,1,0);
	}
	if (rom->romid == 88) {
		memcpy(arbmemory + bios_offset, arbmemory, 524288);
	}
}

static uae_u32 REGPARAM2 arbb_lget (uaecptr addr)
{
	uae_u32 *m;

	addr -= arbb_start & arbb_mask;
	addr &= arbb_mask;
	m = (uae_u32 *)(arbbmemory + addr);
	return do_get_mem_long (m);
}
static uae_u32 REGPARAM2 arbb_wget (uaecptr addr)
{
	uae_u16 *m;

	addr -= arbb_start & arbb_mask;
	addr &= arbb_mask;
	m = (uae_u16 *)(arbbmemory + addr);
	return do_get_mem_word (m);
}
static uae_u32 REGPARAM2 arbb_bget (uaecptr addr)
{
	addr -= arbb_start & arbb_mask;
	addr &= arbb_mask;
	return arbbmemory[addr];
}

static void REGPARAM2 arbb_lput (uaecptr addr, uae_u32 l)
{
}

static void REGPARAM2 arbb_wput (uaecptr addr, uae_u32 w)
{
}

static void REGPARAM2 arbb_bput (uaecptr addr, uae_u32 b)
{
}

static int REGPARAM2 arbb_check (uaecptr addr, uae_u32 size)
{
	addr &= arbb_mask;
	return (addr + size) <= allocated_arbbmemory;
}

static uae_u8 *REGPARAM2 arbb_xlate (uaecptr addr)
{
	addr &= arbb_mask;
	return arbbmemory + addr;
}

static addrbank arcadia_boot_bank = {
	arbb_lget, arbb_wget, arbb_bget,
	arbb_lput, arbb_wput, arbb_bput,
	arbb_xlate, arbb_check, NULL, NULL, _T("Arcadia BIOS"),
	arbb_lget, arbb_wget, ABFLAG_ROM | ABFLAG_SAFE, S_READ, S_WRITE,
	NULL, arbb_mask
};

static uae_u32 REGPARAM2 arb_lget (uaecptr addr)
{
	uae_u32 *m;

	addr -= arb_start & arb_mask;
	addr &= arb_mask;
	m = (uae_u32 *)(arbmemory + addr);
	return do_get_mem_long (m);
}
static uae_u32 REGPARAM2 arb_wget (uaecptr addr)
{
	uae_u16 *m;

	addr -= arb_start & arb_mask;
	addr &= arb_mask;
	m = (uae_u16 *)(arbmemory + addr);
	return do_get_mem_word (m);
}
static uae_u32 REGPARAM2 arb_bget (uaecptr addr)
{
	addr -= arb_start & arb_mask;
	addr &= arb_mask;
	return arbmemory[addr];
}
static void REGPARAM2 arb_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	addr -= arb_start & arb_mask;
	addr &= arb_mask;
	if (addr >= nvram_offset) {
		m = (uae_u32 *)(arbmemory + addr);
		do_put_mem_long (m, l);
		nvwrite++;
	}
}
static void REGPARAM2 arb_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr -= arb_start & arb_mask;
	addr &= arb_mask;
	if (addr >= nvram_offset) {
		m = (uae_u16 *)(arbmemory + addr);
		do_put_mem_word (m, w);
		nvwrite++;
		if (addr == 0x1ffffe)
			multigame(w);
	}
}
static void REGPARAM2 arb_bput (uaecptr addr, uae_u32 b)
{
	addr -= arb_start & arb_mask;
	addr &= arb_mask;
	if (addr >= nvram_offset) {
		arbmemory[addr] = b;
		nvwrite++;
	}
}

static int REGPARAM2 arb_check (uaecptr addr, uae_u32 size)
{
	addr &= arb_mask;
	return (addr + size) <= allocated_arbmemory;
}

static uae_u8 *REGPARAM2 arb_xlate (uaecptr addr)
{
	addr &= arb_mask;
	return arbmemory + addr;
}

static addrbank arcadia_rom_bank = {
	arb_lget, arb_wget, arb_bget,
	arb_lput, arb_wput, arb_bput,
	arb_xlate, arb_check, NULL, NULL, _T("Arcadia Game ROM"),
	arb_lget, arb_wget, ABFLAG_ROM | ABFLAG_SAFE, S_READ, S_WRITE,
	NULL, arb_mask
};


static void multigame (int v)
{
	if (v != 0)
		map_banks (&kickmem_bank, arb_start >> 16, 8, 0);
	else
		map_banks (&arcadia_rom_bank, arb_start >> 16, allocated_arbmemory >> 16, 0);
}

int is_arcadia_rom (const TCHAR *path)
{
	struct arcadiarom *rom;

	rom = is_arcadia (path, 0);
	if (!rom || rom->type == NO_ARCADIA_ROM)
		return NO_ARCADIA_ROM;
	if (rom->type == ARCADIA_BIOS) {
		arcadia_bios = rom;
		return ARCADIA_BIOS;
	}
	arcadia_game = rom;
	return ARCADIA_GAME;
}

static void nvram_write (void)
{
	TCHAR path[MAX_DPATH];
	cfgfile_resolve_path_out_load(currprefs.flashfile, path, MAX_DPATH, PATH_ROM);
	struct zfile *f = zfile_fopen (path, _T("rb+"), ZFD_NORMAL);
	if (!f) {
		f = zfile_fopen (path, _T("wb"), 0);
		if (!f)
			return;
	}
	zfile_fwrite (arbmemory + nvram_offset, NVRAM_SIZE, 1, f);
	zfile_fclose (f);
}

static void nvram_read (void)
{
	struct zfile *f;

	TCHAR path[MAX_DPATH];
	cfgfile_resolve_path_out_load(currprefs.flashfile, path, MAX_DPATH, PATH_ROM);
	f = zfile_fopen (path, _T("rb"), ZFD_NORMAL);
	memset (arbmemory + nvram_offset, 0, NVRAM_SIZE);
	if (!f)
		return;
	zfile_fread (arbmemory + nvram_offset, NVRAM_SIZE, 1, f);
	zfile_fclose (f);
}

static void alg_vsync(void);
static void cubo_vsync(void);

static void arcadia_hsync(void)
{
	arcadia_hsync_cnt++;
}
static void arcadia_vsync(void)
{
	if (alg_flag)
		alg_vsync();
	if (cubo_enabled)
		cubo_vsync();

	if (arcadia_bios) {
		static int cnt;
		cnt--;
		if (cnt > 0)
			return;
		cnt = 50;
		if (!nvwrite)
			return;
		nvram_write();
		nvwrite = 0;
	}
}

void arcadia_unmap (void)
{
	xfree (arbmemory);
	arbmemory = NULL;
	arcadia_bios = NULL;
	arcadia_game = NULL;
}

int arcadia_map_banks (void)
{
	if (!arcadia_bios)
		return 0;
	arbmemory = xmalloc (uae_u8, allocated_arbmemory + allocated_arbbmemory);
	arbbmemory = arbmemory + bios_offset;
	memset (arbmemory, 0, allocated_arbmemory);
	if (!load_roms (arcadia_bios)) {
		arcadia_unmap ();
		return 0;
	}
	if (arcadia_game)
		load_roms (arcadia_game);
	decrypt_roms (arcadia_bios);
	if (arcadia_game)
		decrypt_roms (arcadia_game);
	nvram_read ();
	multigame (0);
	map_banks (&arcadia_boot_bank, 0xf0, 8, 0);
	device_add_vsync_pre(arcadia_vsync);
	device_add_hsync(arcadia_hsync);
	arcadia_hsync_cnt = 0;
	return 1;
}

uae_u8 arcadia_parport (int port, uae_u8 pra, uae_u8 dra)
{
	uae_u8 v;

	v = (pra & dra) | (dra ^ 0xff);
	if (port) {
		if (dra & 1)
			arcadia_coin[0] = arcadia_coin[1] = 0;
		return 0;
	}
	v = 0;
	v |= (arcadia_flag & 1) ? 0 : 2;
	v |= (arcadia_flag & 2) ? 0 : 4;
	v |= (arcadia_flag & 4) ? 0 : 8;
	v |= (arcadia_coin[0] & 3) << 4;
	v |= (arcadia_coin[1] & 3) << 6;
	return v;
}

struct romdata *scan_arcadia_rom(TCHAR *path, int cnt)
{
	struct romdata *rd = 0;
	struct romlist **arc_rl;
	struct arcadiarom *arcadia_rom;
	int i;

	arcadia_rom = is_arcadia(path, cnt);
	if (arcadia_rom) {
		arc_rl = getarcadiaroms(0);
		for (i = 0; arc_rl[i]; i++) {
			if (arc_rl[i]->rd->id == arcadia_rom->romid) {
				rd = arc_rl[i]->rd;
				_tcscat(path, FSDB_DIR_SEPARATOR_S);
				_tcscat(path, arcadia_rom->romid1);
				break;
			}
		}
		xfree(arc_rl);
	}
	return rd;
}

// American Laser Games

/*

Port 1:
- Joy Up = Service mode
- Joy Down = Start
- Joy Left = Left Coin
- Joy Right = Right Coin

Port 2:
- Light Gun
- 2nd button = player select (output)
- 3rd button = trigger
- Joy Up = holster

*/


int alg_flag;
int log_ld = 0;

#define ALG_NVRAM_SIZE 4096
#define ALG_NVRAM_MASK (ALG_NVRAM_SIZE - 1)
static uae_u8 algmemory[ALG_NVRAM_SIZE];
static int algmemory_modified;
static int algmemory_initialized;
static int alg_game_id;
static int alg_picmatic_nova;
static uae_u8 picmatic_io, picmatic_ply;

static void alg_nvram_write (void)
{
	TCHAR path[MAX_DPATH];
	cfgfile_resolve_path_out_load(currprefs.flashfile, path, MAX_DPATH, PATH_ROM);
	struct zfile *f = zfile_fopen (path, _T("rb+"), ZFD_NORMAL);
	if (!f) {
		f = zfile_fopen (path, _T("wb"), 0);
		if (!f)
			return;
	}
	zfile_fwrite (algmemory, ALG_NVRAM_SIZE, 1, f);
	zfile_fclose (f);
}

static void alg_nvram_read (void)
{
	struct zfile *f;

	TCHAR path[MAX_DPATH];
	cfgfile_resolve_path_out_load(currprefs.flashfile, path, MAX_DPATH, PATH_ROM);
	f = zfile_fopen (path, _T("rb"), ZFD_NORMAL);
	memset (algmemory, 0, ALG_NVRAM_SIZE);
	if (!f)
		return;
	zfile_fread (algmemory, ALG_NVRAM_SIZE, 1, f);
	zfile_fclose (f);
}

static uae_u32 REGPARAM2 alg_lget (uaecptr addr)
{
	return 0;
}
static uae_u32 REGPARAM2 alg_wget (uaecptr addr)
{
	return 0;
}

static uae_u32 REGPARAM2 alg_bget (uaecptr addr)
{
	if ((addr & 0xffff0001) == 0xf60001) {
		if (alg_picmatic_nova == 1) {
			// Picmatic 100Hz games
			int reg = (addr >> 12) & 15;
			uae_u8 v = 0;
			uae_s16 x = lightpen_x[1 - picmatic_ply];
			x <<= 1;
			x >>= currprefs.gfx_resolution;
			uae_s16 y = lightpen_y[1 - picmatic_ply] >> currprefs.gfx_vresolution;
			if (reg == 3) {
				v = 0xff;
				// left trigger
				if (alg_flag & 64)
					v &= ~0x20;
				// right trigger
				if (alg_flag & 128)
					v &= ~0x10;
				// left holster
				if (alg_flag & 256)
					v &= ~0x80;
				// right holster
				if (alg_flag & 512)
					v &= ~0x40;
			} else if (reg == 0) {
				v = y & 0xff;
			} else if (reg == 1) {
				v = x & 0xff;
			} else if (reg == 2) {
				v = ((x >> 8) << 4) | (y >> 8); 
			}
			return v;
		}
	}
	uaecptr addr2 = addr;
	addr >>= 1;
	addr &= ALG_NVRAM_MASK;
	uae_u8 v = algmemory[addr];
	//write_log(_T("ALG BGET %08X %04X %02X %08X\n"), addr2, addr, v, M68K_GETPC);
	return v;
}

static void REGPARAM2 alg_lput (uaecptr addr, uae_u32 l)
{
}
static void REGPARAM2 alg_wput (uaecptr addr, uae_u32 w)
{
}

static void REGPARAM2 alg_bput (uaecptr addr, uae_u32 b)
{
	if (alg_picmatic_nova == 1 && (addr & 0xffff0000) == 0xf60000) {
		int reg = (addr >> 12) & 15;
		if (reg == 0) {
			picmatic_io = b;
		} if (reg == 5) {
			// Picmatic 100Hz games
			picmatic_ply = 0;
		} else if (reg == 6) {
			// Picmatic 100Hz games
			picmatic_ply = 1;
		}
		//write_log(_T("ALG BPUT %08X %02X %08X\n"), addr, b & 255, M68K_GETPC);
		return;
	}
	uaecptr addr2 = addr;
	addr >>= 1;
	addr &= ALG_NVRAM_MASK;
	//write_log(_T("ALG BPUT %08X %04X %02X %08X\n"), addr2, addr, b & 255, M68K_GETPC);
	if (algmemory[addr] != b) {
		algmemory[addr] = b;
		algmemory_modified = 100;
	}
}

static addrbank alg_ram_bank = {
	alg_lget, alg_wget, alg_bget,
	alg_lput, alg_wput, alg_bput,
	default_xlate, default_check, NULL, NULL, _T("ALG RAM"),
	dummy_lgeti, dummy_wgeti, ABFLAG_RAM | ABFLAG_SAFE, S_READ, S_WRITE,
	NULL, 65536
};

static int ld_mode, ld_mode_value;
static uae_u32 ld_address, ld_value;
static uae_u32 ld_startaddress, ld_endaddress;
static uae_s32 ld_repcnt, ld_mark;
static int ld_direction;
static bool ld_save_restore;
static uae_s8 ld_ui, ld_ui_cnt;
#define LD_MODE_SEARCH 1
#define LD_MODE_PLAY 2
#define LD_MODE_STILL 3
#define LD_MODE_STOP 4
#define LD_MODE_REPEAT 5
#define LD_MODE_REPEAT2 6
#define LD_MODE_MARK 7

#define ALG_SER_BUF_SIZE 16
static uae_u8 alg_ser_buf[ALG_SER_BUF_SIZE];
static int ser_buf_offset;
static int ld_wait_ack, ld_wait_seek, ld_wait_seek_status;
static int ld_audio, ld_audio_mute;
static bool ld_video;
static int ld_vsync;
static int alg_hsync_delay;
static uae_u8 ld_uidx_config[3], ld_uidx_offset, ld_uidx_offsetd;
static char ld_uidx_data[32];
static uae_u8 ld_user_index;

void genlock_infotext(uae_u8 *d, struct vidbuffer *dst)
{
	if (!ld_user_index) {
#if 0
		int x = 26;
		int y = 12;
		x -= 12;
		y -= 10;
		int dx = dst->inwidth * x / (86 - 12);
		int dy = dst->inheight * y / (59 - 10);
		int mx = 1, my = 1;
		mx <<= currprefs.gfx_resolution;
		my <<= currprefs.gfx_vresolution;
		ldp_render("ABCDE12345ABCDE12345", 20, d, dst, dx, dy, mx ,my);
		y += 4;
		dy = dst->inheight * y / (59 - 10);
		ldp_render("ABCDE12345ABCDE12345", 20, d, dst, dx, dy, mx, my);
#endif
		return;
	}
	int x = ld_uidx_config[0] & 63;
	int y = ld_uidx_config[1] & 63;
	int mx = (ld_uidx_config[2] & 3) + 1;
	int my = ((ld_uidx_config[2] >> 2) & 3) + 1;
	bool dm = (ld_uidx_config[2] & 0x10) != 0;
	int offset = ld_uidx_offsetd & 31;
	int len = 32 - offset;

	x -= 12;
	y -= 10;

	int dx = dst->inwidth * x / (86 - 12);
	int dy = dst->inheight * y / (59 - 10);

	mx <<= currprefs.gfx_resolution;
	my <<= currprefs.gfx_vresolution;

	if (dm) {
		ldp_render(ld_uidx_data, 10, d, dst, dx, dy, mx, my);
		y += 4;
		dy = dst->inheight * y / (59 - 10);
		ldp_render(ld_uidx_data + 10, 10, d, dst, dx, dy, mx, my);
		y += 4;
		dy = dst->inheight * y / (59 - 10);
		ldp_render(ld_uidx_data + 20, 10, d, dst, dx, dy, mx, my);
	} else {
		ldp_render(ld_uidx_data, 20, d, dst, dx, dy, mx, my);
	}

}

static void sb(uae_u8 v)
{
	if (ser_buf_offset < ALG_SER_BUF_SIZE) {
		alg_ser_buf[ser_buf_offset++] = v;
	}
}
static void ack(void)
{
	ld_wait_ack = alg_hsync_delay + 30;
	sb(0x0a); // ACK
}

static void sony_serial_read(uae_u16 w)
{
	w &= 0xff;

	if (ld_ui >= 0 && ld_ui_cnt >= 0) {
		if (ld_ui == 0) {
			ld_uidx_config[ld_ui_cnt] = (uae_u8)w;
			ld_ui_cnt++;
			if (ld_ui_cnt == 3) {
				ld_ui = -1;
			}
		} else if (ld_ui == 1) {
			if (ld_ui_cnt == 0) {
				ld_uidx_offset = (uae_u8)w;
			} else {
				int idx = ld_uidx_offset & 31;
				ld_uidx_data[idx] = (uae_u8)w;
				ld_uidx_offset++;
			}
			ld_ui_cnt++;
			if (ld_ui_cnt >= 34 || (ld_ui_cnt > 1 && w == 0x1a)) {
				ld_ui = -1;
			}
		} else if (ld_ui == 2) {
			ld_uidx_offsetd = (uae_u8)w;
			ld_ui_cnt++;
			ld_ui = -1;
		}
		ack();
		return;
	}

	switch (w)
	{
	case 0: // User Index 00
	case 1: // User Index 01
	case 2: // User Index 02
		if (ld_ui < 0) {
			ld_ui = (uae_s8)w;
			ld_ui_cnt = 0;
			ack();
		}
		break;
	case 0x24: // Audio mute
		ld_audio_mute = true;
		ack();
		break;
	case 0x25: // Audio mute off
		ld_audio_mute = false;
		ack();
		break;
	case 0x26: // Video off
		ld_video = false;
		ack();
		if (log_ld)
			write_log(_T("LD: Video off\n"));
		break;
	case 0x27: // Video on
		ld_video = true;
		ack();
		if (log_ld)
			write_log(_T("LD: Video on\n"));
		break;
	case 0x2b: // FWD Step
		ld_address++;
#ifdef VIDEOGRAB
		getsetpositionvideograb(ld_address);
#endif
		ld_mode = LD_MODE_STILL;
		ld_direction = 0;
		ack();
		if (log_ld)
			write_log(_T("LD: FWD step\n"));
		break;
	case 0x2c: // REV Step
		if (ld_address) {
			ld_address--;
		}
#ifdef VIDEOGRAB
		getsetpositionvideograb(ld_address);
#endif
		ld_mode = LD_MODE_STILL;
		ld_direction = 0;
		ack();
		if (log_ld)
			write_log(_T("LD: REV step\n"));
		break;
	case 0x30: // '0'
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39: // '9'
	{
		uae_u8 v = w - 0x30;
		ld_value *= 10;
		ld_value += v;
		ack();
	}
	break;
	case 0x3a: // F-PLAY ':'
	if (ld_mode != LD_MODE_PLAY) {
		ld_mode = LD_MODE_PLAY;
		ld_direction = 0;
		ld_repcnt = -1;
#ifdef VIDEOGRAB
		pausevideograb(0);
#endif
	}
	ack();
	if (log_ld)
		write_log(_T("LD: PLAY\n"));
	break;
	case 0x3b: // FAST FORWARD PLAY ';'
	ld_mode = LD_MODE_PLAY;
	ld_direction = 1;
	ack();
	if (log_ld)
		write_log(_T("LD: FAST FORWARD PLAY\n"));
	break;
	case 0x3f: // STOP '?'
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif
	ld_direction = 0;
	ld_repcnt = -1;
	ld_mode = LD_MODE_STOP;
	ack();
	if (log_ld)
		write_log(_T("LD: STOP\n"));
	break;
	case 0x40: // ENTER '@'
		if (ld_mode_value == LD_MODE_MARK) {
			ld_mode_value = 0;
			ld_mark = ld_value;
			ack();
			if (log_ld) {
				write_log(_T("LD: SET MARK %d\n"), ld_mark);
			}
		} else if (ld_mode_value == LD_MODE_REPEAT) {
			ld_direction = 0;
			ld_endaddress = ld_value;
			ld_startaddress = ld_address;
			ld_value = 0;
			ld_mode_value = LD_MODE_REPEAT2;
			ack();
		} else if (ld_mode_value == LD_MODE_REPEAT2) {
			ld_repcnt = ld_value;
			ld_address = ld_startaddress;
			if (ld_startaddress > ld_endaddress) {
				ld_direction = -1;
			}
			ld_mode_value = 0;
			ld_mode = LD_MODE_PLAY;
#ifdef VIDEOGRAB
			pausevideograb(0);
#endif
			ack();
			if (log_ld) {
				write_log(_T("LD: REPEAT CNT=%d, %d TO %d\n"), ld_repcnt, ld_startaddress, ld_endaddress);
			}
		} else if (ld_mode_value == LD_MODE_SEARCH) {
		ld_address = ld_value;
		ack();
		// delay seek status response by 2 frames (Platoon requires this)
		ld_wait_seek = arcadia_hsync_cnt + 2 * maxvpos;
#ifdef VIDEOGRAB
		uae_s32 endpos = (uae_s32)getdurationvideograb();
		if (ld_address > endpos) {
			ld_address = endpos;
			getsetpositionvideograb(ld_address);
			ld_wait_seek_status = 0x06; // NO FRAME NUMBER
		} else {
			getsetpositionvideograb(ld_address);
			ld_wait_seek_status = 0x01; // COMPLETION
		}
#endif
		ld_mode = LD_MODE_STILL;
		ld_mode_value = 0;
		ld_direction = 0;
		if (log_ld)
			write_log(_T("LD: SEARCH %d\n"), ld_value);
	}
	break;
	case 0x41: // Clear entry
		ld_value = 0;
		ack();
		if (log_ld)
			write_log(_T("LD: CLEAR ENTRY\n"));
		break;
	case 0x4a: // R-PLAY 'J'
	ld_mode = LD_MODE_PLAY;
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif
	ld_direction = -1;
	ack();
	if (log_ld)
		write_log(_T("LD: R-PLAY\n"));
	break;
	case 0x4b: // Fast reverse play 'K'
	ld_mode = LD_MODE_PLAY;
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif
	ld_direction = -2;
	ack();
	if (log_ld)
		write_log(_T("LD: FAST R-PLAY\n"));
	break;
	case 0x4f: // STILL 'O'
	ld_mode = LD_MODE_STILL;
	ld_direction = 0;
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif
	ack();
	if (log_ld)
		write_log(_T("LD: PAUSE\n"));
	break;
	case 0x43: // SEARCH 'C'
	ack();
	ld_mode = LD_MODE_STILL;
	ld_mode_value = LD_MODE_SEARCH;
	ld_direction = 0;
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif
	ld_value = 0;
	if (log_ld)
		write_log(_T("LD: SEARCH\n"));
	break;
	case 0x44: // Repeat 'D'
	ack();
	ld_mode_value = LD_MODE_REPEAT;
	ld_direction = 0;
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif
	ld_value = 0;
	if (log_ld)
		write_log(_T("LD: REPEAT\n"));
	break;
	case 0x46: // CH-1 ON 'F'
	ack();
	ld_audio |= 1;
#ifdef VIDEOGRAB
	setchflagsvideograb(ld_audio, false);
#endif
	if (log_ld)
		write_log(_T("LD: CH-1 ON\n"));
	break;
	case 0x48: // CH-2 ON 'H'
	ack();
	ld_audio |= 2;
#ifdef VIDEOGRAB
	setchflagsvideograb(ld_audio, false);
#endif
	if (log_ld)
		write_log(_T("LD: CH-2 ON\n"));
	break;
	case 0x47: // CH-1 OFF 'G'
	ack();
	ld_audio &= ~1;
#ifdef VIDEOGRAB
	setchflagsvideograb(ld_audio, false);
#endif
	if (log_ld)
		write_log(_T("LD: CH-1 OFF\n"));
	break;
	case 0x49: // CH-2 OFF 'I'
	ack();
	ld_audio &= ~2;
#ifdef VIDEOGRAB
	setchflagsvideograb(ld_audio, false);
#endif
	if (log_ld)
		write_log(_T("LD: CH-2 OFF\n"));
	break;
	case 0x50: // INDEX ON 'P'
	ack();
	if (log_ld)
		write_log(_T("LD: INDEX ON\n"));
	break;
	case 0x51: // INDEX OFF 'O'
	ack();
	if (log_ld)
		write_log(_T("LD: INDEX OFF\n"));
	break;
	case 0x55: // Frame mode 'U'
	ack();
	if (log_ld)
		write_log(_T("LD: Frame Mode\n"));
	break;
	case 0x56: // Clear 'V'
	ld_mode = LD_MODE_STILL;
	ld_repcnt = -1;
	ld_mark = -1;
	ld_mode_value = 0;
	ld_direction = 0;
	ld_video = true;
	ld_value = 0;
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif
	ack();
	if (log_ld)
		write_log(_T("LD: CL\n"));
	break;
	case 0x60: // ADDR INQ '`'
	{
#ifdef VIDEOGRAB
		if (!ld_save_restore && isvideograb() && ld_direction == 0) {
			ld_address = (uae_u32)getsetpositionvideograb(-1);
		}
#endif
		uae_u32 v = ld_address;
		uae_u32 m = 10000;
		for (int i = 0; i < 5; i++) {
			uae_u8 vv = ((v / m) % 10) + '0';
			sb(vv);
			m /= 10;
		}
		if (log_ld > 1)
			write_log(_T("LD: ADDR INQ %d\n"), ld_address);
	}
	break;
	case 0x67: // STATUS INQ 'g'
		sb(0x80);
		sb(0x00);
		sb(0x40);
		sb((ld_mode_value == LD_MODE_SEARCH ? 0x02 : 0x00) | (ld_mode_value == LD_MODE_REPEAT ? 0x04 : 0x00) | (ld_mode_value ? 0x01 : 0x00) | (ld_mode_value == LD_MODE_REPEAT2 ? 0x20 : 0x00));
		sb((ld_mode == LD_MODE_PLAY ? 0x01 : 0x00) | (ld_mode == LD_MODE_STILL ? 0x20 : 0x00) | (ld_mode == LD_MODE_STOP ? 0x40 : 0x00) | (ld_direction < 0 ? 0x80 : 0x00));
		if (log_ld > 1)
			write_log(_T("LD: STATUS INQ\n"));
		break;
	case 0x73: // MARK SET 's'
		ld_value = 0;
		ld_mode_value = LD_MODE_MARK;
		ack();
		if (log_ld)
			write_log(_T("LD: MARK SET\n"));
		break;
	case 0x80: // USER INDEX
		ld_ui = -1;
		ack();
		if (log_ld)
			write_log("LD: USER INDEX\n");
		break;
	case 0x81: // USER INDEX ON
		ld_user_index = 1;
		ack();
		if (log_ld)
			write_log("LD: USER INDEX ON\n");
		break;
	case 0x82: // USER INDEX OFF
		ld_user_index = 0;
		ack();
		if (log_ld)
			write_log(_T("LD: USER INDEX OFF\n"));
		break;
	default:
		write_log(_T("LD: Unemulated command %02x\n"), w);
		break;
	}
}

bool alg_ld_active(void)
{
	return ld_mode == LD_MODE_PLAY || ld_mode == LD_MODE_STILL;
}

static void alg_vsync(void)
{
	ld_vsync++;
	if (ld_save_restore) {
#ifdef VIDEOGRAB
		if (ld_address == 0 || getsetpositionvideograb(ld_address) > 0) {
			ld_save_restore = false;
			setchflagsvideograb(ld_audio, false);
		}
		if (ld_save_restore) {
			return;
		}
#endif
	}

	if (ld_mode == LD_MODE_PLAY) {
#ifdef VIDEOGRAB
		if (log_ld && (ld_vsync & 63) == 0) {
			uae_s64 pos = getsetpositionvideograb(-1);
			write_log(_T("LD: frame %lld\n"), pos);
		}
		pausevideograb(0);
#endif
		if (ld_direction < 0) {
			if (ld_address > 0) {
				ld_address -= (-ld_direction);
				if ((ld_vsync & 15) == 0) {
#ifdef VIDEOGRAB
					if (isvideograb()) {
						getsetpositionvideograb(ld_address);
					}
#endif
				}
			}
		} else {
			ld_address += 1 + ld_direction;
			if (ld_direction > 0) {
				if ((ld_vsync & 15) == 0) {
#ifdef VIDEOGRAB
					if (isvideograb()) {
						getsetpositionvideograb(ld_address);
					}
#endif
				}
			}
		}
#ifdef VIDEOGRAB
		if (ld_repcnt >= 0 || ld_mark >= 0) {
			uae_s64 f = getsetpositionvideograb(-1);
			if (ld_repcnt >= 0) {
				if ((ld_direction >= 0 && f >= ld_endaddress) || (ld_direction < 0 && f <= ld_endaddress)) {
					if (ld_repcnt > 0) {
						ld_repcnt--;
						if (ld_repcnt == 0) {
							sb(0x01); // Repeat complete
							if (log_ld) {
								write_log(_T("LD: Repeat complete\n"));
							}
							ld_repcnt = -1;
							ld_mode = LD_MODE_STILL;
							ld_direction = 0;
							pausevideograb(1);
						} else {
							ld_address = ld_startaddress;
							getsetpositionvideograb(ld_address);
							if (log_ld) {
								write_log(_T("LD: Repeat %d from %d\n"), ld_repcnt, ld_startaddress);
							}
						}
					} else {
						ld_address = ld_startaddress;
						getsetpositionvideograb(ld_address);
						if (log_ld) {
							write_log(_T("LD: Repeat %d from %d\n"), ld_repcnt, ld_startaddress);
						}
					}
				}
			}
			if (ld_mark == f || ld_mark == f + 1 || ld_mark == f + 2) {
				sb(0x07); // MARK RETURN
				if (log_ld) {
					write_log(_T("LD: Mark return %d\n"), ld_mark);
				}
				ld_mark = -1;
			}
		}
	} else {
		pausevideograb(1);
#endif
	}
	if (algmemory_modified > 0) {
		algmemory_modified--;
		if (algmemory_modified == 0) {
			alg_nvram_write();
		}
	}
}

static int sony_serial_write(void)
{
	if (ld_wait_seek_status && arcadia_hsync_cnt >= ld_wait_seek) {
		sb(ld_wait_seek_status);
		if (log_ld) {
			write_log("LD: seek status %d\n", ld_wait_seek_status);
		}
		ld_wait_seek_status = 0;
	}
	if (ser_buf_offset > 0) {
		uae_u16 v = alg_ser_buf[0];
		if (v == 0x0a) {
			if (arcadia_hsync_cnt < ld_wait_ack) {
				return -1;
			}
		}
		ser_buf_offset--;
		memmove(alg_ser_buf, alg_ser_buf + 1, ser_buf_offset);
		return v;
	}
	return -1;
}

static void pioneer_serial_read(uae_u16 w)
{
	w &= 0xff;
}
static int pioneer_serial_write(void)
{
	return -1;
}

void ld_serial_read(uae_u16 w)
{
	if (alg_flag || currprefs.genlock_image == 7) {
		sony_serial_read(w);
	} else if (currprefs.genlock_image == 8) {
		pioneer_serial_read(w);
	}
}

int ld_serial_write(void)
{
	int v = -1;
	if (arcadia_hsync_cnt < alg_hsync_delay) {
		return -1;
	}
	if (alg_flag || currprefs.genlock_image == 7) {
		v = sony_serial_write();
	} else if (currprefs.genlock_image == 8) {
		v = pioneer_serial_write();
	}
	if (v >= 0) {
		alg_hsync_delay = arcadia_hsync_cnt + 2;
	}
	return v;
}

/*

Port 1:

- Joy Up = Service mode
- Joy Down = Left Start
- 3rd button = Right Start
- Joy Left = Left Coin
- Joy Right = Right Coin

Port 2:

- Light gun (player 0/1)
- 2nd button = Player select (output)
- 3rd button = Trigger (player 0/1)
- Joy Up = Holster (player 0/1)

*/

static uae_u16 alg_potgo;
static uae_u8 picmatic_parallel;

uae_u8 alg_parallel_port(uae_u8 drb, uae_u8 v)
{
	picmatic_parallel = v;
	//write_log("%02x\n", v);
	return v;
}

int alg_get_player(uae_u16 potgo)
{
	if (alg_picmatic_nova == 1) {
		// Picmatic: parallel port bit 0 set = player 2
		return (picmatic_io & 1) ? 0 : 1;
	} else {
		// ALG: 2nd button output and high = player 2.
		return (potgo & 0xc000) == 0xc000 ? 1 : 0;
	}
}

uae_u16 alg_potgor(uae_u16 potgo)
{
	alg_potgo = potgo;

	int ply = alg_get_player(alg_potgo);

	if (alg_picmatic_nova == 1) {
		potgo |= (0x1000 | 0x0100 | 0x4000 | 0x0400);
		// right trigger
		if (alg_flag & 128)
			potgo &= ~0x1000;

	} else {
		potgo |= (0x1000 | 0x0100);
		// trigger
		if (((alg_flag & 128) && ply == 1) || ((alg_flag & 64) && ply == 0))
			potgo &= ~0x1000;
		// right start
		if (alg_flag & 8)
			potgo &= ~0x0100;
	}

	return potgo;
}
uae_u16 alg_joydat(int joy, uae_u16 v)
{
	if (!alg_flag)
		return v;
	int ply = alg_get_player(alg_potgo);
	v = 0;

	if (alg_picmatic_nova == 1) {

		if (joy == 0) {

			// coin
			if (alg_flag & (16 | 32))
				v |= 0x0002;

			// left start
			if (alg_flag & 4)
				v |= 0x0200;

			// service
			if (alg_flag & 2)
				v |= (v & 0x0200) ? 0x0000 : 0x0100;
			else
				v |= (v & 0x0200) ? 0x0100 : 0x0000;

			// right start
			if (alg_flag & 8)
				v |= (v & 0x0002) ? 0x0000 : 0x0001;
			else
				v |= (v & 0x0002) ? 0x0001 : 0x0000;

		} else if (joy == 1) {

			// right holster
			if (alg_flag & 512)
				v |= 0x0200;

			// left trigger
			if (alg_flag & 64)
				v |= 0x0002;

			// left holster
			if (alg_flag & 256)
				v |= (v & 0x0002) ? 0x0000 : 0x0001;
			else
				v |= (v & 0x0002) ? 0x0001 : 0x0000;
		}

	} else if (alg_picmatic_nova == 2) {

		if (joy == 0) {

			// left coin
			if (alg_flag & 16)
				v |= 0x0200;
			// right coin
			if (alg_flag & 32)
				v |= 0x0002;

			// service
			if (alg_flag & 2)
				v |= (v & 0x0200) ? 0x0000 : 0x0100;
			else
				v |= (v & 0x0200) ? 0x0100 : 0x0000;
			// left start
			if (alg_flag & 4)
				v |= (v & 0x0002) ? 0x0000 : 0x0001;
			else
				v |= (v & 0x0002) ? 0x0001 : 0x0000;

		}

	} else {

		if (joy == 0) {

			// left coin
			if (alg_flag & 16)
				v |= 0x0200;
			// right coin
			if (alg_flag & 32)
				v |= 0x0002;

			// service
			if (alg_flag & 2)
				v |= (v & 0x0200) ? 0x0000 : 0x0100;
			else
				v |= (v & 0x0200) ? 0x0100 : 0x0000;
			// left start
			if (alg_flag & 4)
				v |= (v & 0x0002) ? 0x0000: 0x0001;
			else
				v |= (v & 0x0002) ? 0x0001: 0x0000;

		} else if (joy == 1) {

			// holster
			if (((alg_flag & 512) && ply == 0) || ((alg_flag & 256) && ply == 1))
				v |= (v & 0x0200) ? 0x0000 : 0x0100;
			else
				v |= (v & 0x0200) ? 0x0100 : 0x0000;

		}
	}
	return v;
}
uae_u8 alg_joystick_buttons(uae_u8 pra, uae_u8 dra, uae_u8 v)
{
	if (alg_flag) {
		int ply = alg_get_player(alg_potgo);
	
		if (alg_game_id == 175) {
			// Mad Dog McCree v1C Holster is fire button
			if ((alg_flag & 256) && ply == 0) {
				v &= ~0x40;
			}
			if ((alg_flag & 512) && ply == 1) {
				v &= ~0x80;
			}
		}
	}
	return v;
}

struct romdata *get_alg_rom(const TCHAR *name)
{
	struct romdata *rd = getromdatabypath(name);
	if (!rd) {
		return NULL;
	}
	if (!(rd->type & ROMTYPE_ALG)) {
		return NULL;
	}
	/* find parent node */
	while (rd[-1].id == rd->id) {
		rd--;
	}
	return rd;
}

void alg_map_banks(void)
{
	if (!savestate_state) {
		alg_flag = 1;
	}
	if (!algmemory_initialized) {
		alg_nvram_read();
		algmemory_initialized = 1;
	}
	struct romdata *rd = get_alg_rom(currprefs.romextfile);
	if (rd->id == 198 || rd->id == 301 || rd->id == 302 || rd->id == 315 || rd->id == 314) {
		map_banks(&alg_ram_bank, 0xf4, 1, 0);
	} else if (rd->id == 182 || rd->id == 273 || rd->size < 0x40000) {
		map_banks(&alg_ram_bank, 0xf5, 1, 0);
	} else {
		map_banks(&alg_ram_bank, 0xf7, 1, 0);
	}
	alg_game_id = rd->id;
	alg_picmatic_nova = rd->id == 198 || rd->id == 301 || rd->id == 302 || rd->id == 315 || rd->id == 314 ? 1 : (rd->id == 197 ? 2 : 0);
	if (alg_picmatic_nova == 1) {
		map_banks(&alg_ram_bank, 0xf6, 1, 0);
	}
#ifdef VIDEOGRAB
	pausevideograb(1);
#endif

	currprefs.cs_floppydatapullup = changed_prefs.cs_floppydatapullup = true;
	device_add_vsync_pre(arcadia_vsync);
	device_add_hsync(arcadia_hsync);
	if (!currprefs.genlock) {
		currprefs.genlock = changed_prefs.genlock = 1;
	}

	ld_save_restore = false;
	if (savestate_state == STATE_RESTORE) {
		if (ld_mode == LD_MODE_PLAY || ld_mode == LD_MODE_STILL) {
			ld_save_restore = true;
		}
	} else {
		picmatic_io = 0;
		picmatic_ply = 0;
		ld_repcnt = -1;
		ld_mark = -1;
		ld_audio = 0;
		ld_audio_mute = false;
		ld_video = true;
		ld_mode = 0;
		ld_wait_ack = 0;
		ld_wait_seek = 0;
		ld_direction = 0;
		ser_buf_offset = 0;
		alg_hsync_delay = 0;
		arcadia_hsync_cnt = 0;
		ld_ui = -1;
		ld_user_index = 0;
		ld_uidx_config[0] = 0;
		ld_uidx_config[1] = 0;
		ld_uidx_config[2] = 0;
		ld_uidx_offset = 0;
		ld_uidx_offsetd = 0;
		memset(ld_uidx_data, 0, sizeof(ld_uidx_data));
	}
}

uae_u8 *restore_alg(uae_u8 *src)
{
	uae_u32 flags = restore_u32();
	uae_u8 v;
	alg_flag = restore_u32();
	ld_value = restore_u32();
	ld_address = restore_u32();
	ld_startaddress = restore_u32();
	ld_endaddress = restore_u32();
	ld_repcnt = restore_u32();
	ld_mark = restore_u32();
	ld_wait_ack = restore_u32();
	alg_hsync_delay = restore_u32();
	arcadia_hsync_cnt = restore_u32();
	v = restore_u8();
	ld_audio = v & 3;
	ld_audio_mute = (v & 0x80) != 0;
	ld_video = restore_u8();
	ld_mode = restore_u8();
	ld_mode_value = restore_u8();
	ld_direction = restore_u8();
	picmatic_io = restore_u8();
	picmatic_parallel = restore_u8();
	ser_buf_offset  = restore_u8();
	for (int i = 0; i < ser_buf_offset; i++) {
		alg_ser_buf[i] = restore_u8();
	}
	ld_vsync = restore_u32();
	ld_user_index = restore_u8();
	ld_uidx_config[0] = restore_u8();
	ld_uidx_config[1] = restore_u8();
	ld_uidx_config[2] = restore_u8();
	ld_uidx_offset = restore_u8();
	ld_uidx_offsetd = restore_u8();
	for (int i = 0; i < 32; i++) {
		ld_uidx_data[i] = restore_u8();
	}
	picmatic_ply = restore_u8();

	return src;
}

uae_u8 *save_alg(size_t *len)
{
	uae_u8 *dstbak, *dst;

	if (!alg_flag)
		return NULL;

	dstbak = dst = xmalloc(uae_u8, 1000);
	save_u32(1);
#ifdef VIDEOGRAB
	uae_u32 addr = (uae_u32)getsetpositionvideograb(-1);
#else
	uae_u32 addr = 0;
#endif

	save_u32(alg_flag);
	save_u32(ld_value);
	save_u32(addr);
	save_u32(ld_startaddress);
	save_u32(ld_endaddress);
	save_u32(ld_repcnt);
	save_u32(ld_mark);
	save_u32(ld_wait_ack);
	save_u32(alg_hsync_delay);
	save_u32(arcadia_hsync_cnt);
	save_u8(ld_audio | (ld_audio_mute ? 0x80 : 0x00));
	save_u8(ld_video);
	save_u8(ld_mode);
	save_u8(ld_mode_value);
	save_u8(ld_direction);
	save_u8(picmatic_io);
	save_u8(picmatic_parallel);
	save_u8(ser_buf_offset);
	for (int i = 0; i < ser_buf_offset; i++) {
		save_u8(alg_ser_buf[i]);
	}
	save_u32(ld_vsync);
	save_u8(ld_user_index);
	save_u8(ld_uidx_config[0]);
	save_u8(ld_uidx_config[1]);
	save_u8(ld_uidx_config[2]);
	save_u8(ld_uidx_offset);
	save_u8(ld_uidx_offsetd);
	for (int i = 0; i < 32; i++) {
		save_u8(ld_uidx_data[i]);
	}
	save_u8(picmatic_ply);
	*len = dst - dstbak;
	return dstbak;
}

static TCHAR cubo_pic_settings[ROMCONFIG_CONFIGTEXT_LEN];
static uae_u32 cubo_settings;

#define TOUCH_BUFFER_SIZE 10
static int touch_buf_offset, touch_write_buf_offset;
static int touch_cmd_active;
static uae_u8 touch_data[TOUCH_BUFFER_SIZE + 1];
static uae_u8 touch_data_w[TOUCH_BUFFER_SIZE];
static int touch_active;
extern uae_u32 cubo_flag;

static int ts_width_mult = 853;
static int ts_width_offset = -73;
static int ts_height_mult = 895;
static int ts_height_offset = -8;

int touch_serial_write(void)
{
	struct vidbuf_description *avidinfo = &adisplays[0].gfxvidinfo;

	if (!(cubo_settings & 0x40000))
		return -1;

	if (!touch_write_buf_offset && touch_active) {
		if ((cubo_flag & 0x80000000) && !(cubo_flag & 0x40000000)) {
			uae_u8 *p = touch_data_w;
			int sw = avidinfo->drawbuffer.inwidth * ts_width_mult / 1000;
			int sh = avidinfo->drawbuffer.inheight * ts_height_mult / 1000;

			int x = ((lightpen_x[0] + ts_width_offset) * 999) / sw;
			int y = ((lightpen_y[0] + ts_height_offset) * 999) / sh;

			if (x < 0)
				x = 0;
			if (x > 999)
				x = 999;
			if (y < 0)
				y = 0;
			if (y > 999)
				y = 999;

			write_log(_T("%d*%d\n"), x, y);

			// y-coordinate is inverted
			y = 999 - y;

			*p++ = 0x01;
			_sntprintf((char*)p, sizeof p, "%03d", x);
			p += 3;
			*p++ = ',';
			_sntprintf((char*)p, sizeof p, "%03d", y);
			p += 3;
			*p++ = 0x0d;
			touch_write_buf_offset = addrdiff(p, touch_data_w);

			cubo_flag |= 0x40000000;
		}

	}
	if (touch_write_buf_offset > 0) {
		uae_u16 v = touch_data_w[0];
		touch_write_buf_offset--;
		memmove(touch_data_w, touch_data_w + 1, touch_write_buf_offset);
		return v;
	}
	return -1;
}

static void touch_command_do(void)
{
	TCHAR *s = au((char*)touch_data);
	write_log(_T("Cubo touch screen command '%s'\n"), s);
	xfree(s);

	if (!memcmp(touch_data, "MDU", 3))
		touch_active = 1; // mode up/down
	if (!memcmp(touch_data, "MP", 2))
		touch_active = 2; // mode point (single per touchdown)
	if (!memcmp(touch_data, "MS", 2))
		touch_active = 3; // mode stream (continuous)
	if (!memcmp(touch_data, "MI", 2))
		touch_active = 0; // mode inactive

	// just respond "ok" to all commands..
	touch_data_w[0] = 0x01;
	touch_data_w[1] = '0';
	touch_data_w[2] = 0x0d;
	touch_write_buf_offset = 3;
}

void touch_serial_read(uae_u16 w)
{
	if (!(cubo_settings & 0x40000))
		return;

	w &= 0xff;
	if (!touch_cmd_active) {
		if (w == 0x01) {
			touch_cmd_active = 1;
			touch_buf_offset = 0;
		}
	}
	else {
		if (w == 0x0d) {
			touch_command_do();
			touch_cmd_active = 0;
		}
		else {
			if (touch_buf_offset < TOUCH_BUFFER_SIZE) {
				touch_data[touch_buf_offset++] = (uae_u8)w;
				touch_data[touch_buf_offset] = 0;
			}
		}
	}
}


static const uae_u8 cubo_pic_secret_base[] = {
	0x7c, 0xf9, 0x56, 0xf7, 0x58, 0x18, 0x22, 0x54, 0x38, 0xcd, 0x3d, 0x94, 0x09, 0xe6, 0x8e, 0x0d,
	0x9a, 0x86, 0xfc, 0x1c, 0xa0, 0x19, 0x8f, 0xbc, 0xfd, 0x8d, 0xd1, 0x57, 0x56, 0xf2, 0xb6, 0x4f
};
static uae_u8 cubo_pic_byte, cubo_pic_bit_cnt, cubo_io_pic;
static uae_u8 cubo_pic_key[2 + 8];
static uae_u32 cubo_key;
static bool cubo_pic_bit_cnt_changed;
static uae_u8 cubo_pic_secret[32];

static void calculate_key(uae_u8 key, int *idxp, uae_u8 *out)
{
	int idx = *idxp;
	out[idx] ^= key;
	uae_u8 b = cubo_pic_secret[out[idx] >> 4];
	uae_u8 c = cubo_pic_secret[(out[idx] & 15) + 16];
	c = ~(c + b);
	c = (c << 1) | (c >> 7);
	c += key;
	c = (c << 1) | (c >> 7);
	c = (c >> 4) | (c << 4);
	idx++;
	idx &= 3;
	out[idx] ^= c;
	*idxp = idx;
}

static char *get_bytes_from_string(const TCHAR *s)
{
	char *ds = xcalloc(char, _tcslen(s) + 1);
	char *ss = ua(s);
	char *sss = ss;
	char *sp = ds;
	while (*sss) {
		char c = *sss++;
		if (c == ':')
			break;
		if (c == ' ')
			continue;
		if (c == '\\') {
			if (*sss) {
				char *endptr = sss;
				*sp++ = (uae_u8)strtol(sss, &endptr, 16);
				sss = endptr;
			}
			continue;
		}
		*sp++ = c;
	}
	*sp = 0;
	xfree(ss);
	return ds;
}

static uae_u32 cubo_calculate_secret(uae_u8 *key)
{
	uae_u8 out[4] = { 0, 0, 0, 0 };
	memcpy(cubo_pic_secret, cubo_pic_secret_base, sizeof(cubo_pic_secret));
	TCHAR *s = cubo_pic_settings;
	TCHAR *s2 = _tcschr(s, ':');
	if (s2) {
		s2++;
		char *cs = get_bytes_from_string(s2);
		if (cs) {
			memcpy(cubo_pic_secret, cs, strlen(cs));
			xfree(cs);
		}
	}
	int idx = 0;
	for (int i = 0; i < 8; i++) {
		calculate_key(key[i], &idx, out);
		calculate_key(key[i], &idx, out);
	}
	uae_u32 v = (out[0] << 24) | (out[1] << 16) | (out[2] << 8) | (out[3] << 0);
	return v;
}

static uae_u8 cubo_read_pic(void)
{
	if (!(cubo_io_pic & 0x40))
		return 0;
	uae_u8 c = 0;
	if (cubo_pic_bit_cnt >= 10 * 8) {
		// return calculated 32-bit key
		int offset = (cubo_pic_bit_cnt - (10 * 8)) / 8;
		c = (cubo_key >> ((3 - offset) * 8)) & 0xff;
	} else if (cubo_pic_bit_cnt < 8) {
		struct boardromconfig *brc = get_device_rom(&currprefs, ROMTYPE_CUBO, 0, NULL);
		if (brc && brc->roms[0].configtext[0]) {
			char *cs = get_bytes_from_string(brc->roms[0].configtext);
			if (cs && strlen(cs) >= 2) {
				c = cs[0];
			}
			xfree(cs);
		}
	} else {
		struct boardromconfig *brc = get_device_rom(&currprefs, ROMTYPE_CUBO, 0, NULL);
		if (brc && brc->roms[0].configtext[0]) {
			char *cs = get_bytes_from_string(brc->roms[0].configtext);
			if (cs && strlen(cs) >= 2) {
				c = cs[1];
			}
			xfree(cs);
		}
	}
	uae_u8 v = 0;
	if ((1 << (7 - (cubo_pic_bit_cnt & 7))) & c) {
		v |= 4;
	}
	if (cubo_pic_bit_cnt_changed && (cubo_pic_bit_cnt & 7) == 7) {
		write_log(_T("PIC sent %02x (%c)\n"), c, c);
	}
	cubo_pic_bit_cnt_changed = false;
	return v;
}

static int cubo_message;

static void cubo_write_pic(uae_u8 v)
{
	uae_u8 old = cubo_io_pic;
	if ((v & 0x40) && !(cubo_io_pic & 0x40)) {
		write_log(_T("Cubo PIC reset.\n"));
		cubo_pic_bit_cnt = -1;
	}
	cubo_io_pic = v;

	if (!(v & 0x40)) {
		return;
	}

	if (!(old & 0x02) && (v & 0x02)) {
		cubo_pic_bit_cnt++;
		cubo_pic_bit_cnt_changed = true;
		cubo_pic_byte <<= 1;
		if (v & 0x08)
			cubo_pic_byte |= 1;
		if ((cubo_pic_bit_cnt & 7) == 7) {
			int offset = cubo_pic_bit_cnt / 8;
			if (offset < sizeof(cubo_pic_key)) {
				cubo_pic_key[offset] = cubo_pic_byte;
				write_log(_T("Cubo PIC received %02x (%d/%zu)\n"), cubo_pic_byte, offset, sizeof(cubo_pic_key));
			}
			if (offset == sizeof(cubo_pic_key) - 1) {
				write_log(_T("Cubo PIC key in: "));
				for (int i = 0; i < 8; i++) {
					write_log(_T("%02x "), cubo_pic_key[i + 2]);
				}
				write_log(_T("\n"));
				cubo_key = cubo_calculate_secret(cubo_pic_key + 2);
				write_log(_T("Cubo PIC key out: %02x.%02x.%02x.%02x\n"),
					(cubo_key >> 24) & 0xff, (cubo_key >> 16) & 0xff,
					(cubo_key >> 8) & 0xff, (cubo_key >> 0) & 0xff);
			}
		}
	}
}

static void *cubo_rtc;
uae_u8 *cubo_nvram;
static uae_u8 cubo_rtc_byte;
extern struct zfile *cd32_flashfile;

#define CUBO_NVRAM_SIZE 2048
#define CUBO_RTC_SIZE 16

static void cubo_read_rtc(void)
{
	if (!cubo_nvram)
		return;
	
	uae_u8 *r = &cubo_nvram[CUBO_NVRAM_SIZE];
	struct timeval tv;
	static int prevs100;
	
	gettimeofday(&tv, NULL);
	time_t t = tv.tv_sec;
	t += currprefs.cs_rtc_adjust;
	struct tm *ct = localtime(&t);

	bool h24 = (r[4] & 0x80) == 0;
	bool mask = (r[0] & 0x08) != 0;

	int h = ct->tm_hour;
	if (h24) {
		r[4] = (h % 10) | ((h / 10) << 4);
	} else {
		r[4] &= ~0x40;
		if (h > 12) {
			h -= 12;
			r[4] |= 0x40;
		}
		r[4] = (h % 10) | ((h / 10) << 4);
	}
	r[4] &= ~0x80;
	r[4] |= h24 ? 0x80 : 0x00;

	r[5] = (r[5] & 0xc0) | (ct->tm_mday % 10) | ((ct->tm_mday / 10) << 4);

	r[6] = (ct->tm_wday << 5) | ((ct->tm_mon + 1) % 10) | (((ct->tm_mon + 1) / 10) << 4);

	int s100 = tv.tv_usec / 10000;
	if (s100 == prevs100) {
		s100++;
		s100 %= 1000000;
	}
	prevs100 = s100;

	r[1] = (s100 % 10) | ((s100 / 10) << 4);

	r[2] = (ct->tm_sec % 10) | ((ct->tm_sec / 10) << 4);

	r[3] = (ct->tm_min % 10) | ((ct->tm_min / 10) << 4);

	r[4] = (ct->tm_hour % 10) | ((ct->tm_hour / 10) << 4);

	if (cd32_flashfile) {
		zfile_fseek(cd32_flashfile, currprefs.cs_cd32nvram_size + CUBO_NVRAM_SIZE, SEEK_SET);
		zfile_fwrite(r, 1, CUBO_RTC_SIZE, cd32_flashfile);
	}

}

static void cubo_rtc_write(uae_u8 addr, uae_u8 v)
{
	uae_u8 *r = &cubo_nvram[CUBO_NVRAM_SIZE];
	addr &= CUBO_RTC_SIZE - 1;
	if (addr == 0) {
		if ((v & 0x40) && !(r[0] & 0x40)) {
			cubo_read_rtc();
		}
		r[addr] = v;
	} else if (addr == 5) {
		// 2 year bits are writable
		r[addr] &= 0x3f;
		r[addr] |= v & 0xc0;
	} else if (addr >= 7) {
		// timer registers are writable
		r[addr] = v;
	}
	write_log(_T("CUBO RTC write %08x %02x\n"), addr, v);

	if (cd32_flashfile) {
		zfile_fseek(cd32_flashfile, currprefs.cs_cd32nvram_size + CUBO_NVRAM_SIZE + addr, SEEK_SET);
		zfile_fwrite(&v, 1, 1, cd32_flashfile);
	}
}
static uae_u8 cubo_rtc_read(uae_u8 addr)
{
	uae_u8 *r = &cubo_nvram[CUBO_NVRAM_SIZE];
	bool mask = (r[0] & 0x08) != 0;
	addr &= CUBO_RTC_SIZE - 1;
	uae_u8 v = r[addr];
	if (mask) {
		if (addr == 5)
			v &= 0x3f;
		if (addr == 6)
			v &= 0x1f;
	}
	write_log(_T("CUBO RTC read %08x %02x\n"), addr, v);
	return v;
}

static void cubo_write_rtc(uae_u8 v)
{
	// bit 5 = data
	// bit 6 = enable?
	// bit 7 = clock

	cubo_rtc_byte = v;

	if (!(v & 0x40)) {
		i2c_reset(cubo_rtc);
		return;
	}

	int sda = (v & 0x20) ? 1 : 0;
	int scl = (v & 0x80) ? 1 : 0;

	i2c_set(cubo_rtc, BITBANG_I2C_SDA, sda);
	i2c_set(cubo_rtc, BITBANG_I2C_SCL, scl);
}

static uae_u8 cubo_vars[4];


static uae_u32 REGPARAM2 cubo_get(uaecptr addr)
{
	uae_u8 v = 0;

	addr -= 0x00600000;
	addr &= 0x003fffff;
	addr += 0x00600000;

	if (addr >= 0x600000 && addr < 0x600000 + 0x2000 && (addr & 3) == 0) {
		if (cubo_settings & 0x10000) {
			int offset = (addr - 0x600000) / 4;
			if (cubo_nvram)
				v = cubo_nvram[offset];
		}
#if CUBO_DEBUG > 1
		write_log(("CUBO_GET NVRAM %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
	} else if (addr == 0x00800003) {
		// DIP switch #1
		// #1 = coin 3 (0->1)
		// #8 = coin 1 (0->1)
		v = (uae_u8)cubo_settings;
		// Test button clears DIP 8 (but lets just toggle here)
		if (cubo_flag & 0x00800000) {
			v ^= 0x0080;
		}

		v ^= (cubo_flag & 0x00ff) >> 0;

#if CUBO_DEBUG > 2
		write_log(("CUBO_GET D1 %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
	} else if (addr == 0x00800013) {
		// DIP switch #2
		// #1 = coin 4 (0->1)
		// #8 = coin 2 (0->1)
		v = (uae_u8)(cubo_settings >> 8);

		// bits 4 and 5 have something to do with hopper

		if (cubo_pic_settings[0]) {
			// bit 2 (0x04) is PIC data
			if (cubo_io_pic & 0x40) {
				v &= ~0x04;
				v |= cubo_read_pic();
			}
		}
		// bit 6 (0x40) is I2C SDA
		if ((cubo_settings & 0x20000) && cubo_rtc && (cubo_rtc_byte & 0x40)) {
			v &= ~0x40;
			if (eeprom_i2c_set(cubo_rtc, BITBANG_I2C_SDA, -1)) {
				v |= 0x40;
			}
		}

		v ^= (cubo_flag & 0xff00) >> 8;

#if CUBO_DEBUG > 2
		write_log(("CUBO_GET D2/PIC %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif

	} else {
#if CUBO_DEBUG
		write_log(("CUBO_GET %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
	}
	return v;
}

static void REGPARAM2 cubo_put(uaecptr addr, uae_u32 v)
{
	addr -= 0x00600000;
	addr &= 0x003fffff;
	addr += 0x00600000;

	v &= 0xff;

	if (addr >= 0x600000 && addr < 0x600000 + 0x2000 && (addr & 3) == 0) {
		if (cubo_settings & 0x10000) {
			int offset = (addr - 0x600000) / 4;
#if CUBO_DEBUG > 1
			write_log(("CUBO_PUT NVRAM %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			if (cubo_nvram) {
				cubo_nvram[offset] = v;
				if (cd32_flashfile) {
					zfile_fseek(cd32_flashfile, currprefs.cs_cd32nvram_size + offset, SEEK_SET);
					zfile_fwrite(&v, 1, 1, cd32_flashfile);
				}
			}
		}
	} else if (addr == 0x0080000b) {
		if (cubo_pic_settings[0]) {
			// PIC communication uses bits 1 (0x02 "clock"), 3 (0x08 "data"), 6 (0x40 enable)
#if CUBO_DEBUG > 2
			write_log(("CUBO_PUT PIC %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			cubo_write_pic(v);
		}
		if ((v & (0x80 | 0x04)) && !(cubo_message & 1)) {
			// bits 2 and 7 are used by hopper
			cubo_message |= 1;
			statusline_add_message(STATUSTYPE_OTHER, _T("Unemulated Hopper bits set."));
		}
	} else if (addr == 0x0080001b) {
#if CUBO_DEBUG > 2
		write_log(("CUBO_PUT RTC %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
		if (cubo_settings & 0x20000) {
			cubo_write_rtc(v);
		}
		// bit 4 = suzo
		if ((v & (0x10)) && !(cubo_message & 2)) {
			cubo_message |= 2;
			statusline_add_message(STATUSTYPE_OTHER, _T("Unemulated Suzo bits set."));
		}
	} else {
#if CUBO_DEBUG
		write_log(("CUBO_PUT %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
	}
}

static uae_u8 dip_delay[16];

void cubo_function(int v)
{
	int c = -1;
	switch (v)
	{
	case 0:
		c = 7;
		break;
	case 1:
		c = 15;
		break;
	case 2:
		c = 0;
		break;
	case 3:
		c = 8;
		break;
	}
	if (c < 0 || dip_delay[c])
		return;
	dip_delay[c] = 5;
}

static void cubo_vsync(void)
{
	for (int i = 0; i < 16; i++) {
		if (dip_delay[i] >= 3) {
			cubo_flag |= 1 << i;
		} else if (dip_delay[i] > 0) {
			cubo_flag &= ~(1 << i);
		}
		if (dip_delay[i] > 0)
			dip_delay[i]--;
	}
}


static addrbank cubo_bank = {
	cubo_get, cubo_get, cubo_get,
	cubo_put, cubo_put, cubo_put,
	default_xlate, default_check, NULL, NULL, _T("CUBO"),
	dummy_lgeti, dummy_wgeti, ABFLAG_RAM, S_READ, S_WRITE,
	NULL, 0x3fffff, 0x600000, 0x600000
};

static void arcadia_reset(int hardreset)
{
	cubo_enabled = is_board_enabled(&currprefs, ROMTYPE_CUBO, 0);
	i2c_free(cubo_rtc);
	cubo_rtc = i2c_new(0xa2, CUBO_RTC_SIZE, cubo_rtc_read, cubo_rtc_write);
	cubo_read_rtc();
	cubo_message = 0;
	cubo_rtc_byte = 0;
	cubo_settings = 0;
	cubo_pic_settings[0] = 0;
}

static void check_arcadia_prefs_changed(void)
{
	if (!config_changed)
		return;
	board_prefs_changed(ROMTYPE_CUBO, 0);
	cubo_enabled = is_board_enabled(&currprefs, ROMTYPE_CUBO, 0);
	struct boardromconfig *brc = get_device_rom(&currprefs, ROMTYPE_CUBO, 0, NULL);
	if (!brc)
		return;
	cubo_settings = brc->roms[0].device_settings;
	_tcscpy(cubo_pic_settings, brc->roms[0].configtext);
}

bool cubo_init(struct autoconfig_info *aci)
{
	aci->start = 0x00600000;
	aci->size  = 0x00400000;
	device_add_reset(arcadia_reset);
	if (!aci->doinit)
		return true;
	map_banks(&cubo_bank, aci->start >> 16, aci->size >> 16, 0);
	aci->addrbank = &cubo_bank;
	device_add_check_config(check_arcadia_prefs_changed);
	device_add_vsync_pre(arcadia_vsync);
	return true;
}

