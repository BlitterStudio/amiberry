
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "zfile.h"
#include "amax.h"
#include "custom.h"
#include "memory.h"
#include "newcpu.h"
#include "rommgr.h"

static const int data_scramble[8] = { 3, 2, 4, 5, 7, 6, 0, 1 };
static const int addr_scramble[16] = { 14, 12, 2, 10, 15, 13, 1, 0, 7, 6, 5, 4, 8, 9, 11, 3 };

static int romptr;
static uae_u8 *rom;
static int amax_rom_size, rom_oddeven;
static uae_u8 amax_data;
static uae_u8 bfd100, bfe001;
static uae_u8 dselect;
static bool amax_is_active;

#define AMAX_LOG 0

static void load_byte (void)
{
	int addr, i;
	uae_u8 val, v;

	v = 0xff;
	addr = 0;
	for (i = 0; i < 16; i++) {
		if (romptr & (1 << i))
			addr |= 1 << addr_scramble[i];
	}
	if (rom_oddeven < 0) {
		val = v;
	} else {
		v = rom[addr * 2 + rom_oddeven];
		val = 0;
		for (i = 0; i < 8; i++) {
			if (v & (1 << data_scramble[i]))
				val |= 1 << i;
		}
	}
	amax_data = val;
	if (AMAX_LOG > 0)
		write_log (_T("AMAX: load byte, rom=%d addr=%06x (%06x) data=%02x (%02x) PC=%08X\n"), rom_oddeven, romptr, addr, v, val, M68K_GETPC);
}

static void amax_check (void)
{
	/* DIR low = reset address counter */
	if ((bfd100 & 2)) {
		if (romptr && AMAX_LOG > 0)
			write_log (_T("AMAX: counter reset PC=%08X\n"), M68K_GETPC);
		romptr = 0;
		amax_is_active = false;
	}
}

static int dwlastbit;

void amax_diskwrite (uae_u16 w)
{
	int i;

	/* this is weird, 1->0 transition in disk write line increases address pointer.. */
	for (i = 0; i < 16; i++) {
		if (dwlastbit && !(w & 0x8000)) {
			romptr++;
			if (AMAX_LOG > 0)
				write_log (_T("AMAX: counter increase %d PC=%08X\n"), romptr, M68K_GETPC);
		}
		dwlastbit = (w & 0x8000) ? 1 : 0;
		w <<= 1;
	}
	romptr &= amax_rom_size - 1;
	amax_check ();
}

static uae_u8 bfe001_ov;

void amax_bfe001_write (uae_u8 pra, uae_u8 dra)
{
	uae_u8 v = dra & pra;

	bfe001 = v;
	/* CHNG low -> high: shift data register */
	if ((v & 4) && !(bfe001_ov & 4)) {
		amax_data <<= 1;
		amax_data |= 1;
		if (AMAX_LOG > 0)
			write_log (_T("AMAX: data shifted\n"));
	}
	/* TK0 = even, WPRO = odd */
	rom_oddeven = -1;
	if ((v & (8 | 16)) != (8 | 16)) {
		rom_oddeven = 0;
		if (!(v & 16))
			rom_oddeven = 1;
	}
	bfe001_ov = v;
	amax_check ();
}

void amax_disk_select (uae_u8 v, uae_u8 ov, int num)
{
	bfd100 = v;

	dselect = 1 << (num + 3);
	if (!(bfd100 & dselect) && (ov & dselect)) {
		amax_is_active = true;
		load_byte ();
	}
	amax_check ();
}

uae_u8 amax_disk_status (uae_u8 st)
{
	if (!(amax_data & 0x80))
		st &= ~0x20;
	return st;
}

bool amax_active(void)
{
	return amax_is_active;
}

void amax_reset (void)
{
	romptr = 0;
	rom_oddeven = 0;
	bfe001_ov = 0;
	dwlastbit = 0;
	amax_data = 0xff;
	xfree (rom);
	rom = NULL;
	dselect = 0;
}

void amax_init (void)
{
	struct zfile *z = NULL;

	if (is_device_rom(&currprefs, ROMTYPE_AMAX, 0) < 0)
		return;
	amax_reset ();
	if (is_device_rom(&currprefs, ROMTYPE_AMAX, 0) > 0)
		z = read_device_rom(&currprefs, ROMTYPE_AMAX, 0, NULL);
	if (z) {
		zfile_fseek (z, 0, SEEK_END);
		amax_rom_size = zfile_ftell32(z);
		zfile_fseek (z, 0, SEEK_SET);
	} else {
		write_log (_T("AMAX: failed to load rom\n"));
		amax_rom_size = 262144;
	}
	rom = xcalloc (uae_u8, amax_rom_size);
	if (z) {
		zfile_fread (rom, amax_rom_size, 1, z);
		zfile_fclose (z);
	}
	write_log (_T("AMAX: loaded, %d bytes\n"), amax_rom_size);
}

