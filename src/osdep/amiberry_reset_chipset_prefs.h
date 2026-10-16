/*
 * amiberry_reset_chipset_prefs.h - Chipset settings applied only by a hard reset
 *
 * The emulator core never copies these fields from changed_prefs into
 * currprefs while it runs: check_prefs_changed_custom() and memory_reset()
 * skip them. WinUAE applies them by copying every setting into currprefs when
 * its GUI requests a hard reset. Amiberry dropped that full copy (#1068), so:
 *
 *  - target_reset() copies them into currprefs on every hard reset, whichever
 *    path requested it (GUI, input event, IPC);
 *  - prefs_to_gui() keeps their pending changed_prefs values when it refreshes
 *    the GUI from currprefs, so the GUI shows what the next hard reset applies
 *    (#2459).
 *
 * Copyright 2026 Dimitris Panokostas
 */

#ifndef AMIBERRY_RESET_CHIPSET_PREFS_H
#define AMIBERRY_RESET_CHIPSET_PREFS_H

#include "options.h"

struct reset_only_chipset_prefs {
	int compatible;
	int rtc_adjust;
	bool cdtvcr;
	bool resetwarning;
	bool ciatodbug;
	bool chip1mjumper;
	bool bkpthang;
};

inline reset_only_chipset_prefs get_reset_only_chipset_prefs(const uae_prefs& p)
{
	return { p.cs_compatible, p.cs_rtc_adjust, p.cs_cdtvcr, p.cs_resetwarning,
		p.cs_ciatodbug, p.cs_1mchipjumper, p.cs_bkpthang };
}

inline void set_reset_only_chipset_prefs(uae_prefs& p, const reset_only_chipset_prefs& v)
{
	p.cs_compatible = v.compatible;
	p.cs_rtc_adjust = v.rtc_adjust;
	p.cs_cdtvcr = v.cdtvcr;
	p.cs_resetwarning = v.resetwarning;
	p.cs_ciatodbug = v.ciatodbug;
	p.cs_1mchipjumper = v.chip1mjumper;
	p.cs_bkpthang = v.bkpthang;
}

#endif // AMIBERRY_RESET_CHIPSET_PREFS_H
