/*
* UAE - The Un*x Amiga Emulator
*
* Debugger
*
* (c) 1995 Bernd Schmidt
* (c) 2006 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <signal.h>

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cpu_prefetch.h"
#include "debug.h"
//#include "disasm.h"
#include "debugmem.h"
#include "cia.h"
#include "xwin.h"
//#include "identify.h"
#include "audio.h"
#include "sounddep/sound.h"
#include "disk.h"
#include "savestate.h"
#include "autoconf.h"
#include "akiko.h"
#include "inputdevice.h"
#include "crc32.h"
#include "cpummu.h"
#include "rommgr.h"
#include "inputrecord.h"
#include "calc.h"
#include "cpummu.h"
#include "cpummu030.h"
#include "ar.h"
//#include "pci.h"
//#include "ppc/ppcd.h"
//#include "uae/io.h"
//#include "uae/ppc.h"
#include "drawing.h"
#include "devices.h"
#include "blitter.h"
#include "ini.h"
#include "readcpu.h"
#include "cputbl.h"
#include "keybuf.h"

int debug_sprite_mask = 0xff;

static uae_u8 *dump_xlate (uae_u32 addr)
{
	if (!mem_banks[addr >> 16]->check (addr, 1))
		return NULL;
	return mem_banks[addr >> 16]->xlateaddr (addr);
}

static const TCHAR *bankmodes[] = { _T("F32"), _T("C16"), _T("C32"), _T("CIA"), _T("F16"), _T("F16X") };

static void memory_map_dump_3(UaeMemoryMap *map, int log)
{
	bool imold;
	int i, j, max;
	addrbank *a1 = mem_banks[0];
	TCHAR txt[256];

	map->num_regions = 0;
	imold = currprefs.illegal_mem;
	currprefs.illegal_mem = false;
	max = currprefs.address_space_24 ? 256 : 65536;
	j = 0;
	for (i = 0; i < max + 1; i++) {
		addrbank *a2 = NULL;
		if (i < max)
			a2 = mem_banks[i];
		if (a1 != a2) {
			int k, mirrored, mirrored2, size, size_out;
			TCHAR size_ext;
			uae_u8 *caddr;
			TCHAR tmp[MAX_DPATH];
			const TCHAR *name = a1->name;
			struct addrbank_sub *sb = a1->sub_banks;
			int bankoffset = 0;
			int region_size;

			k = j;
			caddr = dump_xlate (k << 16);
			mirrored = caddr ? 1 : 0;
			k++;
			while (k < i && caddr) {
				if (dump_xlate (k << 16) == caddr) {
					mirrored++;
				}
				k++;
			}
			mirrored2 = mirrored;
			if (mirrored2 == 0)
				mirrored2 = 1;

			while (bankoffset < 65536) {
				int bankoffset2 = bankoffset;
				if (sb) {
					uaecptr daddr;
					if (!sb->bank)
						break;
					daddr = (j << 16) | bankoffset;
					a1 = get_sub_bank(&daddr);
					name = a1->name;
					for (;;) {
						bankoffset2 += MEMORY_MIN_SUBBANK;
						if (bankoffset2 >= 65536)
							break;
						daddr = (j << 16) | bankoffset2;
						addrbank *dab = get_sub_bank(&daddr);
						if (dab != a1)
							break;
					}
					sb++;
					size = (bankoffset2 - bankoffset) / 1024;
					region_size = size * 1024;
				} else {
					size = (i - j) << (16 - 10);
					region_size = ((i - j) << 16) / mirrored2;
				}

				if (name == NULL)
					name = _T("<none>");

				size_out = size;
				size_ext = 'K';
				if (j >= 256 && (size_out / mirrored2 >= 1024) && !((size_out / mirrored2) & 1023)) {
					size_out /= 1024;
					size_ext = 'M';
				}
				_stprintf (txt, _T("%08X %7d%c/%d = %7d%c %s%s%c %s %s"), (j << 16) | bankoffset, size_out, size_ext,
					mirrored, mirrored ? size_out / mirrored : size_out, size_ext,
					(a1->flags & ABFLAG_CACHE_ENABLE_INS) ? _T("I") : _T("-"),
					(a1->flags & ABFLAG_CACHE_ENABLE_DATA) ? _T("D") : _T("-"),
					a1->baseaddr == NULL ? ' ' : '*',
					bankmodes[ce_banktype[j]],
					name);
				tmp[0] = 0;
				if ((a1->flags & ABFLAG_ROM) && mirrored) {
					TCHAR *p = txt + _tcslen (txt);
					uae_u32 crc = 0xffffffff;
					if (a1->check(((j << 16) | bankoffset), (size * 1024) / mirrored))
						crc = get_crc32 (a1->xlateaddr((j << 16) | bankoffset), (size * 1024) / mirrored);
					struct romdata *rd = getromdatabycrc (crc);
					_stprintf (p, _T(" (%08X)"), crc);
					if (rd) {
						tmp[0] = '=';
						getromname (rd, tmp + 1);
						_tcscat (tmp, _T("\n"));
					}
				}

				if (a1 != &dummy_bank) {
					for (int m = 0; m < mirrored2; m++) {
						if (map->num_regions >= UAE_MEMORY_REGIONS_MAX)
							break;
						UaeMemoryRegion *r = &map->regions[map->num_regions];
						r->start = (j << 16) + bankoffset + region_size * m;
						r->size = region_size;
						r->flags = 0;
						r->memory = NULL;
						if (!(a1->flags & ABFLAG_PPCIOSPACE)) {
							r->memory = dump_xlate((j << 16) | bankoffset);
							if (r->memory)
								r->flags |= UAE_MEMORY_REGION_RAM;
						}
						/* just to make it easier to spot in debugger */
						r->alias = 0xffffffff;
						if (m >= 0) {
							r->alias = j << 16;
							r->flags |= UAE_MEMORY_REGION_ALIAS | UAE_MEMORY_REGION_MIRROR;
						}
						_stprintf(r->name, _T("%s"), name);
						_stprintf(r->rom_name, _T("%s"), tmp);
						map->num_regions += 1;
					}
				}
				_tcscat (txt, _T("\n"));
				if (log > 0)
					write_log (_T("%s"), txt);
				else if (log == 0)
					write_log (txt);
				if (tmp[0]) {
					if (log > 0)
						write_log (_T("%s"), tmp);
					else if (log == 0)
						write_log (tmp);
				}
				if (!sb)
					break;
				bankoffset = bankoffset2;
			}
			j = i;
			a1 = a2;
		}
	}
	//pci_dump(log);
	currprefs.illegal_mem = imold;
}

static void memory_map_dump_2 (int log)
{
	UaeMemoryMap map;
	memory_map_dump_3(&map, log);
}

void memory_map_dump (void)
{
	memory_map_dump_2(1);
}