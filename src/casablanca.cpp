#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "newcpu.h"
#include "casablanca.h"

/*

	53C710 (SCSI)
	Base: 0x04000000
	IRQ: 4
	INTREQ: 7 (AUD0)

	DracoMotion
	Base: 0x20000000
	IRQ: 3
	INTREQ: 4 (COPER)
	Size: 128k
	Autoconfig: 83 17 30 00 47 54 00 00 00 00 00 00 00 00 00 00 (18260/23)

	Mouse
	Base: 0x02400BE3
	IRQ: 4
	INTREQ: 9 (AUD2)

	Serial
	Base: 0x02400FE3
	IRQ: 4
	INTREQ: 10 (AUD3)

	Floppy:
	Base: 0x02400003
	IRQ: 5
	INTREQ: 11 (RBF)
*/

static uae_u32 REGPARAM2 casa_lget(uaecptr addr)
{
	static int max = 100;
	if (max < 0) {
		write_log(_T("casa_lget %08x %08x\n"), addr, M68K_GETPC);
		max--;
	}

	return 0;
}
static uae_u32 REGPARAM2 casa_wget(uaecptr addr)
{
	static int max = 100;
	if (max >= 0) {
		write_log(_T("casa_wget %08x %08x\n"), addr, M68K_GETPC);
		max--;
	}

	return 0;
}

static uae_u32 REGPARAM2 casa_bget(uaecptr addr)
{
	uae_u8 v = 0;

	static int max = 100;
	if (max >= 0) {
		write_log(_T("casa_bget %08x %08x\n"), addr, M68K_GETPC);
		max--;
	}


	// casa
	if (addr == 0x020007c3)
		v = 4;

	// draco
	if (addr == 0x02000009)
		v = 4;

	return v;
}

static void REGPARAM2 casa_lput(uaecptr addr, uae_u32 l)
{
	static int max = 100;
	if (max < 0)
		return;
	max--;

	write_log(_T("casa_lput %08x %08x %08x\n"), addr, l, M68K_GETPC);
}
static void REGPARAM2 casa_wput(uaecptr addr, uae_u32 w)
{
	static int max = 100;
	if (max < 0)
		return;
	max--;

	write_log(_T("casa_wput %08x %04x %08x\n"), addr, w & 0xffff, M68K_GETPC);
}

static void REGPARAM2 casa_bput(uaecptr addr, uae_u32 b)
{
	static int max = 100;
	if (max < 0)
		return;
	max--;

	write_log(_T("casa_bput %08x %02x %08x\n"), addr, b & 0xff, M68K_GETPC);
}

static addrbank casa_ram_bank = {
	casa_lget, casa_wget, casa_bget,
	casa_lput, casa_wput, casa_bput,
	default_xlate, default_check, NULL, NULL, _T("Casablanca IO"),
	dummy_lgeti, dummy_wgeti, ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE,
};


void casablanca_map_overlay(void)
{
	// Casablanca has ROM at address zero, no chip ram, no overlay.
	map_banks(&kickmem_bank, 524288 >> 16, 524288 >> 16, 0);
	map_banks(&extendedkickmem_bank, 0 >> 16, 524288 >> 16, 0);
	map_banks(&casa_ram_bank, 0x02000000 >> 16, 0x01000000 >> 16, 0);
	// at least draco has rom here
	map_banks(&kickmem_bank, 0x02c00000 >> 16, 524288 >> 16, 0);
}
