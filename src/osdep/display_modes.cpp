/*
 * display_modes.cpp - Display mode enumeration and query functions
 *
 * Extracted from amiberry_gfx.cpp to reduce translation unit size.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "gui.h"
#include "amiberry_gfx.h"
#include "display_modes.h"
#include "threaddep/thread.h"
#include "registry.h"
#include "fsdb_host.h"

#include "gfx_platform_internal.h"

#include "uae/time.h"

// ---------------------------------------------------------------------------
// Shared state (moved from amiberry_gfx.cpp)
// ---------------------------------------------------------------------------

int deskhz;
bool calculated_scanline = false;

// ---------------------------------------------------------------------------
// Externs from amiberry_gfx.cpp and other TUs
// ---------------------------------------------------------------------------

extern int vsync_activeheight, vsync_totalheight;
extern float vsync_vblank, vsync_hblank;
extern uae_s64 spincount;
extern void target_calibrate_spin();

// VSyncState is now a member of the IRenderer base class.
// Access it via g_renderer->vsync_state().
#include "irenderer.h"
extern std::unique_ptr<IRenderer> g_renderer;

// amiberry_getrefreshrate is defined in amiberry_gfx.cpp. We need it for
// target_getcurrentvblankrate. We'll declare it as an extern here.
extern float amiberry_getrefreshrate(int monid);

// OffsetRect is static in amiberry_gfx.cpp. centerdstrect uses it.
// We provide a local copy here.
static void OffsetRect(SDL_Rect* rect, const int dx, const int dy)
{
	rect->x += dx;
	rect->y += dy;
}

// ---------------------------------------------------------------------------
// Display query functions
// ---------------------------------------------------------------------------

int gfx_IsPicassoScreen(const struct AmigaMonitor* mon)
{
	return mon->screen_is_picasso ? 1 : 0;
}

int isfullscreen_2(const struct uae_prefs* p)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	const auto idx = mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE;
	return p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLSCREEN ? 1 : p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLWINDOW ? -1 : 0;
}
int isfullscreen()
{
	return isfullscreen_2(&currprefs);
}

int gfx_GetWidth(const struct AmigaMonitor* mon)
{
	return mon->currentmode.current_width;
}

int gfx_GetHeight(const struct AmigaMonitor* mon)
{
	return mon->currentmode.current_height;
}

// ---------------------------------------------------------------------------
// Display lookup
// ---------------------------------------------------------------------------

struct MultiDisplay* getdisplay2(const struct uae_prefs* p, const int index)
{
	static int max;
	int display;
	if (index < 0 && p) {
		const int apmode = p->gfx_apmode[APMODE_NATIVE].gfx_display;
		display = apmode - 1;
	} else {
		display = index;
	}

	if (!max || (max < MAX_DISPLAYS && max > 0 && Displays[max].monitorname != nullptr)) {
		max = 0;
		while (max < MAX_DISPLAYS && Displays[max].monitorname)
			max++;
		if (max == 0) {
			gui_message(_T("no display adapters! Exiting"));
			exit(0);
		}
	}
	if (index >= 0 && display >= max)
		return nullptr;
	if (display >= max)
		display = 0;
	display = std::max(display, 0);
	return &Displays[display];
}

struct MultiDisplay* getdisplay(const struct uae_prefs* p, const int monid)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	if (monid > 0 && mon->md)
		return mon->md;
	if (p) {
		int apmode_idx = mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE;
		return getdisplay2(p, p->gfx_apmode[apmode_idx].gfx_display - 1);
	}
	return getdisplay2(p, 0);
}

// ---------------------------------------------------------------------------
// Desktop / display geometry
// ---------------------------------------------------------------------------

void desktop_coords(const int monid, int* dw, int* dh, int* ax, int* ay, int* aw, int* ah)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	const struct MultiDisplay* md = getdisplay(&currprefs, monid);

	*dw = md->rect.w;
	*dh = md->rect.h;
	*ax = mon->amigawin_rect.x;
	*ay = mon->amigawin_rect.y;
	*aw = mon->amigawin_rect.w;
	*ah = mon->amigawin_rect.h;
}

static int target_get_display2(const TCHAR* name, const int mode)
{
	int found, found2;

	found = -1;
	found2 = -1;
	for (int i = 0; i < MAX_DISPLAYS && Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->monitorid[0] == '\\')
			continue;
		if (mode == 2 && md->monitorid[0] != '\\')
			continue;
		if (!_tcscmp(md->monitorid, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;

	found = -1;
	for (int i = 0; i < MAX_DISPLAYS && Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->adapterid[0] == '\\')
			continue;
		if (mode == 2 && md->adapterid[0] != '\\')
			continue;
		if (!_tcscmp(md->adapterid, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				if (found2 < 0)
					found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;

	for (int i = 0; i < MAX_DISPLAYS && Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->adaptername[0] == '\\')
			continue;
		if (mode == 2 && md->adaptername[0] != '\\')
			continue;
		if (!_tcscmp(md->adaptername, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				if (found2 < 0)
					found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;

	for (int i = 0; i < MAX_DISPLAYS && Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->monitorname[0] == '\\')
			continue;
		if (mode == 2 && md->monitorname[0] != '\\')
			continue;
		if (!_tcscmp(md->monitorname, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				if (found2 < 0)
					found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;
	if (mode == 3) {
		if (found2 >= 0)
			return found2;
	}

	return -1;
}

int target_get_display(const TCHAR* name)
{
	int disp;

	//write_log(_T("target_get_display '%s'\n"), name);
	disp = target_get_display2(name, 0);
	//write_log(_T("Scan 0: %d\n"), disp);
	if (disp >= 0)
		return disp;
	disp = target_get_display2(name, 1);
	//write_log(_T("Scan 1: %d\n"), disp);
	if (disp >= 0)
		return disp;
	disp = target_get_display2(name, 2);
	//write_log(_T("Scan 2: %d\n"), disp);
	if (disp >= 0)
		return disp;
	disp = target_get_display2(name, 3);
	//write_log(_T("Scan 3: %d\n"), disp);
	if (disp >= 0)
		return disp;
	return -1;
}

// ---------------------------------------------------------------------------
// Scanline / vblank
// ---------------------------------------------------------------------------

static int target_get_display_scanline2(int displayindex)
{
	float diff = static_cast<float>(read_processor_time() - g_renderer->vsync_state().wait_vblank_timestamp);
	if (diff < 0)
		return -1;
	int sl = static_cast<int>(diff * (vsync_activeheight + (vsync_totalheight - vsync_activeheight) / 10) * vsync_vblank / syncbase);
	if (sl < 0)
		sl = -1;
	return sl;
}

int target_get_display_scanline(const int displayindex)
{
	if (!g_renderer->vsync_state().scanlinecalibrating && calculated_scanline) {
		return target_get_display_scanline2(displayindex);
	} else {
		static uae_s64 lastrdtsc;
		static int lastvpos;
		if (spincount == 0 || currprefs.m68k_speed >= 0) {
			lastrdtsc = 0;
			lastvpos = target_get_display_scanline2(displayindex);
			return lastvpos;
		}
		const uae_s64 v = read_processor_time_rdtsc();
		if (lastrdtsc > v)
			return lastvpos;
		lastvpos = target_get_display_scanline2(displayindex);
		lastrdtsc = read_processor_time_rdtsc() + spincount * 4;
		return lastvpos;
	}
}

static bool get_display_vblank_params(int displayindex, int* activeheightp, int* totalheightp, float* vblankp, float* hblankp)
{
	bool ret = false;
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (!displays || displayindex >= count)
	{
		write_log("SDL_GetDisplays failed or displayindex out of range\n");
		SDL_free(displays);
		return ret;
	}
	const SDL_DisplayMode* dm = SDL_GetDesktopDisplayMode(displays[displayindex]);
	SDL_free(displays);
	if (!dm)
	{
		write_log("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
		return ret;
	}

	int active = dm->h;
	int total = active * 1125 / 1080; // Standard 1080p timing heuristic

	if (activeheightp)
		*activeheightp = active;
	if (totalheightp)
		*totalheightp = total;
	const auto vblank = dm->refresh_rate;
	// standard horizontal frequency is ~31.4 kHz for 60Hz 1080p
	const auto hblank = static_cast<float>(vblank * total);
	if (vblankp)
		*vblankp = vblank;
	if (hblankp)
		*hblankp = hblank;

	ret = true;
	return ret;
}

static void display_vblank_thread(struct AmigaMonitor* mon)
{
	const struct amigadisplay* ad = &adisplays[mon->monitor_id];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];

	if (g_renderer->vsync_state().waitvblankthread_mode > 0)
		return;
	// It seems some Windows 7 drivers stall if D3DKMTWaitForVerticalBlankEvent()
	// and D3DKMTGetScanLine() is used simultaneously.
	//if ((calculated_scanline || os_win8) && ap->gfx_vsyncmode && pD3DKMTWaitForVerticalBlankEvent && g_renderer->vsync_state().wait_vblank_display && g_renderer->vsync_state().wait_vblank_display->HasAdapterData) {
	//	waitvblankevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//	g_renderer->vsync_state().waitvblankthread_mode = 1;
	//	unsigned int th;
	//	_beginthreadex(NULL, 0, waitvblankthread, 0, 0, &th);
	//}
	// it is used when D3DKMTGetScanLine() is not available or not working.
	// calculated_scanline = false;
}

void target_cpu_speed()
{
	display_vblank_thread(&AMonitors[0]);
}

void display_param_init(struct AmigaMonitor* mon)
{
	const struct amigadisplay* ad = &adisplays[mon->monitor_id];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];

	vsync_activeheight = mon->currentmode.current_height;
	vsync_totalheight = vsync_activeheight * 1125 / 1080;
	vsync_vblank = 0;
	vsync_hblank = 0;
	get_display_vblank_params(0, &vsync_activeheight, &vsync_totalheight, &vsync_vblank, &vsync_hblank);
	if (vsync_vblank <= 0)
		vsync_vblank = static_cast<float>(mon->currentmode.freq);
	// GPU scaled mode?
	if (vsync_activeheight > mon->currentmode.current_height) {
		const float m = static_cast<float>(vsync_activeheight) / mon->currentmode.current_height;
		vsync_hblank = vsync_hblank / m + 0.5f;
		vsync_activeheight = mon->currentmode.current_height;
	}

	g_renderer->vsync_state().wait_vblank_display = getdisplay(&currprefs, mon->monitor_id);
	if (g_renderer->vsync_state().wait_vblank_display) {
		g_renderer->vsync_state().wait_vblank_display->HasAdapterData = true; // SDL displays always have adapter data
	}
	if (!g_renderer->vsync_state().wait_vblank_display || !g_renderer->vsync_state().wait_vblank_display->HasAdapterData) {
		write_log(_T("Selected display mode does not have adapter data!\n"));
	}
	g_renderer->vsync_state().scanlinecalibrating = true;
	target_calibrate_spin();
	g_renderer->vsync_state().scanlinecalibrating = false;
	display_vblank_thread(mon);
}

// ---------------------------------------------------------------------------
// Display name
// ---------------------------------------------------------------------------

const TCHAR* target_get_display_name(const int num, const bool friendlyname)
{
	if (num <= 0)
		return nullptr;
	const struct MultiDisplay* md = getdisplay2(nullptr, num - 1);
	if (!md)
		return nullptr;
	if (friendlyname)
		return md->monitorname;
	return md->monitorid;
}

// ---------------------------------------------------------------------------
// Center destination rect
// ---------------------------------------------------------------------------

void centerdstrect(struct AmigaMonitor* mon, SDL_Rect* dr)
{
	if (isfullscreen() == 0)
		OffsetRect(dr, mon->amigawin_rect.x, mon->amigawin_rect.y);
	if (isfullscreen() < 0) {
		if (mon->scalepicasso && mon->screen_is_picasso)
			return;
		if (mon->currentmode.fullfill && (mon->currentmode.current_width > mon->currentmode.native_width || mon->currentmode.current_height > mon->currentmode.native_height))
			return;
		OffsetRect(dr, (mon->currentmode.native_width - mon->currentmode.current_width) / 2,
			(mon->currentmode.native_height - mon->currentmode.current_height) / 2);
	}
}

// ---------------------------------------------------------------------------
// Refresh rate
// ---------------------------------------------------------------------------

int getrefreshrate(const int monid, const int width, const int height)
{
	const struct amigadisplay* ad = &adisplays[monid];
	const struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	int freq = 0;

	if (ap->gfx_refreshrate <= 0)
		return 0;

	const struct MultiDisplay* md = getdisplay(&currprefs, monid);
	for (int i = 0; md->DisplayModes[i].inuse; i++) {
		const struct PicassoResolution* pr = &md->DisplayModes[i];
		if (pr->res.width == width && pr->res.height == height) {
			for (int j = 0; pr->refresh[j] > 0; j++) {
				if (pr->refresh[j] == ap->gfx_refreshrate)
					return ap->gfx_refreshrate;
				if (pr->refresh[j] > freq && pr->refresh[j] < ap->gfx_refreshrate)
					freq = pr->refresh[j];
			}
		}
	}
	write_log(_T("Refresh rate %d not supported, using %d\n"), ap->gfx_refreshrate, freq);
	return freq;
}

// ---------------------------------------------------------------------------
// Mode management (add, sort, list)
// ---------------------------------------------------------------------------

static void addmode(const struct MultiDisplay* md, const SDL_DisplayMode* dm, const int rawmode)
{
	int i, j;
	int w = dm->w;
	int h = dm->h;
	int d = SDL_BITSPERPIXEL(dm->format);
	bool lace = false;
	int freq = 0;

	if (w > max_uae_width || h > max_uae_height) {
		write_log(_T("Ignored mode %d*%d\n"), w, h);
		return;
	}

	// SDL3 reports 24-bit here, but we can ignore it and use 32-bit modes
#ifndef AMIBERRY
	if (d != 32) {
		return;
	}
#endif

	if (dm->refresh_rate) {
		freq = dm->refresh_rate;
		if (freq < 10)
			freq = 0;
	}
	//if (dm->dmFields & DM_DISPLAYFLAGS) {
	//	lace = (dm->dmDisplayFlags & DM_INTERLACED) != 0;
	//}

	i = 0;
	while (md->DisplayModes[i].inuse) {
		if (md->DisplayModes[i].res.width == w && md->DisplayModes[i].res.height == h) {
			for (j = 0; j < MAX_REFRESH_RATES; j++) {
				if (md->DisplayModes[i].refresh[j] == 0 || md->DisplayModes[i].refresh[j] == freq)
					break;
			}
			if (j < MAX_REFRESH_RATES) {
				md->DisplayModes[i].refresh[j] = freq;
				md->DisplayModes[i].refreshtype[j] = (lace ? REFRESH_RATE_LACE : 0) | (rawmode ? REFRESH_RATE_RAW : 0);
				md->DisplayModes[i].refresh[j + 1] = 0;
				if (!lace)
					md->DisplayModes[i].lace = false;
				return;
			}
		}
		i++;
	}
	i = 0;
	while (md->DisplayModes[i].inuse)
		i++;
	if (i >= MAX_PICASSO_MODES - 1)
		return;
	md->DisplayModes[i].inuse = true;
	md->DisplayModes[i].rawmode = rawmode;
	md->DisplayModes[i].lace = lace;
	md->DisplayModes[i].res.width = w;
	md->DisplayModes[i].res.height = h;
	md->DisplayModes[i].refresh[0] = freq;
	md->DisplayModes[i].refreshtype[0] = (lace ? REFRESH_RATE_LACE : 0) | (rawmode ? REFRESH_RATE_RAW : 0);
	md->DisplayModes[i].refresh[1] = 0;
	_sntprintf(md->DisplayModes[i].name, sizeof md->DisplayModes[i].name, _T("%dx%d%s"),
		md->DisplayModes[i].res.width, md->DisplayModes[i].res.height,
		lace ? _T("i") : _T(""));
}

static int resolution_compare(const void* a, const void* b)
{
	const PicassoResolution* ma = (struct PicassoResolution*)a;
	const PicassoResolution* mb = (struct PicassoResolution*)b;
	if (ma->res.width < mb->res.width)
		return -1;
	if (ma->res.width > mb->res.width)
		return 1;
	if (ma->res.height < mb->res.height)
		return -1;
	if (ma->res.height > mb->res.height)
		return 1;
	return 0;
}

static void sortmodes(const struct MultiDisplay* md)
{
	int i = 0, idx = -1;
	unsigned int pw = -1, ph = -1;

	while (md->DisplayModes[i].inuse)
		i++;
	qsort(md->DisplayModes, i, sizeof(struct PicassoResolution), resolution_compare);
	for (i = 0; md->DisplayModes[i].inuse; i++)
	{
		int j, k;
		for (j = 0; md->DisplayModes[i].refresh[j]; j++) {
			for (k = j + 1; md->DisplayModes[i].refresh[k]; k++) {
				if (md->DisplayModes[i].refresh[j] > md->DisplayModes[i].refresh[k]) {
					int t = md->DisplayModes[i].refresh[j];
					md->DisplayModes[i].refresh[j] = md->DisplayModes[i].refresh[k];
					md->DisplayModes[i].refresh[k] = t;
					t = md->DisplayModes[i].refreshtype[j];
					md->DisplayModes[i].refreshtype[j] = md->DisplayModes[i].refreshtype[k];
					md->DisplayModes[i].refreshtype[k] = t;
				}
			}
		}
		if (md->DisplayModes[i].res.height != ph || md->DisplayModes[i].res.width != pw)
		{
			ph = md->DisplayModes[i].res.height;
			pw = md->DisplayModes[i].res.width;
			idx++;
		}
		md->DisplayModes[i].residx = idx;
	}
}

static void modesList(struct MultiDisplay* md)
{
	int i, j;

	i = 0;
	while (md->DisplayModes[i].inuse) {
		write_log(_T("%d: %s%s ("), i, md->DisplayModes[i].rawmode ? _T("!") : _T(""), md->DisplayModes[i].name);
		j = 0;
		while (md->DisplayModes[i].refresh[j] > 0) {
			if (j > 0)
				write_log(_T(","));
			if (md->DisplayModes[i].refreshtype[j] & REFRESH_RATE_RAW)
				write_log(_T("!"));
			write_log(_T("%d"), md->DisplayModes[i].refresh[j]);
			if (md->DisplayModes[i].refreshtype[j] & REFRESH_RATE_LACE)
				write_log(_T("i"));
			j++;
		}
		write_log(_T(")\n"));
		i++;
	}
}

// ---------------------------------------------------------------------------
// Monitor re-enumeration
// ---------------------------------------------------------------------------

void reenumeratemonitors()
{
	for (int i = 0; i < MAX_DISPLAYS; i++) {
		struct MultiDisplay* md = &Displays[i];
		memcpy(&md->workrect, &md->rect, sizeof(SDL_Rect));
	}
	enumeratedisplays();
}

static bool enumeratedisplays2(bool selectall)
{
	struct MultiDisplay *md = Displays;
	int num_displays = 0;
	SDL_DisplayID* display_ids = SDL_GetDisplays(&num_displays);
	if (!display_ids || num_displays < 1) {
		write_log("No video displays found\n");
		SDL_free(display_ids);
		return false;
	}

	SDL_DisplayID primary_id = SDL_GetPrimaryDisplay();

	for (int i = 0; i < num_displays; i++)
	{
		if (md - Displays >= MAX_DISPLAYS)
			break;
		const char *display_name = SDL_GetDisplayName(display_ids[i]);
		if (!display_name)
			continue;

		if (!SDL_GetDisplayBounds(display_ids[i], &md->rect))
			continue;
		SDL_GetDisplayBounds(display_ids[i], &md->workrect);

		md->adaptername = my_strdup_trim(display_name);
		md->adapterid = my_strdup(display_name);
		md->adapterkey = my_strdup(display_name);
		md->monitorname = my_strdup_trim(display_name);
		md->monitorid = my_strdup(display_name);
		md->display_id = display_ids[i];
		md->primary = (display_ids[i] == primary_id);
		md->monitor = i;

		int num_modes = 0;
		const SDL_DisplayMode* const* modes = SDL_GetFullscreenDisplayModes(display_ids[i], &num_modes);
		if (!modes || num_modes < 1)
			continue;

		md->DisplayModes = xcalloc(struct PicassoResolution, num_modes + 1);
		if (!md->DisplayModes)
			continue;

		for (int j = 0; j < num_modes; j++) {
			const SDL_DisplayMode* mode = modes[j];
			if (!mode)
				continue;

			md->DisplayModes[j].res.width = mode->w;
			md->DisplayModes[j].res.height = mode->h;
			md->DisplayModes[j].refresh[0] = static_cast<int>(mode->refresh_rate);
			md->DisplayModes[j].refresh[1] = 0;
		}

		md++;
	}
	SDL_free(display_ids);

	if (md == Displays)
		return false;

	for (auto& display : Displays) {
		if (display.monitorname == nullptr) {
			break;
		}
		if (display.fullname == nullptr) {
			display.fullname = my_strdup(display.adapterid);
		}
	}

	return true;
}

void enumeratedisplays()
{
	if (gfx_platform_enumeratedisplays())
		return;
	if (!enumeratedisplays2 (false))
		enumeratedisplays2(true);
}

// ---------------------------------------------------------------------------
// Sort displays
// ---------------------------------------------------------------------------

void sortdisplays()
{
	if (gfx_platform_skip_sortdisplays())
		return;
	struct MultiDisplay* md;
	int i, idx;

	const SDL_DisplayMode* desktop_dm = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
	if (!desktop_dm) {
		write_log("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
		return;
	}

	const int w = desktop_dm->w;
	const int h = desktop_dm->h;
	const int wv = w;
	const int hv = h;
	const int b = SDL_BITSPERPIXEL(desktop_dm->format);

	deskhz = 0;

	md = Displays;
	while (md->monitorname) {
		md->DisplayModes = xcalloc(struct PicassoResolution, MAX_PICASSO_MODES);

		write_log(_T("%s '%s' [%s]\n"), md->adaptername, md->adapterid, md->adapterkey);
		write_log(_T("-: %s [%s]\n"), md->fullname, md->monitorid);
		int num_disp_modes = 0;
		SDL_DisplayID display_id = md->display_id ? md->display_id : SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* const* modes = SDL_GetFullscreenDisplayModes(display_id, &num_disp_modes);
		for (int mode = 0; mode < 2; mode++)
		{
			for (idx = 0; idx < num_disp_modes; idx++)
			{
				const SDL_DisplayMode* dm = modes[idx];
				if (!dm)
					continue;
				int found = 0;
				int idx2 = 0;
				while (md->DisplayModes[idx2].inuse && !found)
				{
					struct PicassoResolution* pr = &md->DisplayModes[idx2];
					if (dm->w == w && dm->h == h && SDL_BITSPERPIXEL(dm->format) == b) {
						deskhz = std::max(static_cast<int>(dm->refresh_rate), deskhz);
					}
					if (pr->res.width == dm->w && pr->res.height == dm->h) {
						for (i = 0; pr->refresh[i]; i++) {
							if (pr->refresh[i] == static_cast<int>(dm->refresh_rate)) {
								found = 1;
								break;
							}
						}
					}
					idx2++;
				}
				if (!found && SDL_BITSPERPIXEL(dm->format) > 8) {
					addmode(md, dm, mode);
				}
			}

		}
		sortmodes(md);
		modesList(md);
		i = 0;
		while (md->DisplayModes[i].inuse)
			i++;
		write_log(_T("%d display modes.\n"), i);
		md++;
	}
	write_log(_T("Desktop: W=%d H=%d B=%d HZ=%d. CXVS=%d CYVS=%d\n"), w, h, b, deskhz, wv, hv);
}

// ---------------------------------------------------------------------------
// Adjust screen mode
// ---------------------------------------------------------------------------

int gfx_adjust_screenmode(const MultiDisplay *md, int *pwidth, int *pheight)
{
	struct PicassoResolution* best;
	int pass, i = 0, index = 0;

	for (pass = 0; pass < 2; pass++) {
		struct PicassoResolution* dm;
		i = 0;
		index = 0;

		best = &md->DisplayModes[0];
		dm = &md->DisplayModes[1];

		while (dm->inuse) {

			/* do we already have supported resolution? */
			if (dm->res.width == *pwidth && dm->res.height == *pheight)
				return i;

			if (dm->res.width <= best->res.width && dm->res.height <= best->res.height
				&& dm->res.width >= *pwidth && dm->res.height >= *pheight)
			{
				best = dm;
				index = i;
			}
			if (dm->res.width >= best->res.width && dm->res.height >= best->res.height
				&& dm->res.width <= *pwidth && dm->res.height <= *pheight)
			{
				best = dm;
				index = i;
			}
			dm++;
			i++;
		}
		if (best->res.width == *pwidth && best->res.height == *pheight) {
			break;
		}
	}
	*pwidth = best->res.width;
	*pheight = best->res.height;

	return index;
}

// ---------------------------------------------------------------------------
// Best mode (used during fullscreen init)
// ---------------------------------------------------------------------------

int getbestmode(struct AmigaMonitor* mon, int nextbest)
{
	int i, startidx;
	struct MultiDisplay* md;
	int ratio;
	int apmode_idx = mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE;
	int index = currprefs.gfx_apmode[apmode_idx].gfx_display - 1;

	for (;;) {
		md = getdisplay2(&currprefs, index);
		if (!md)
			return 0;
		ratio = mon->currentmode.native_width > mon->currentmode.native_height ? 1 : 0;
		for (i = 0; md->DisplayModes[i].inuse; i++) {
			struct PicassoResolution* pr = &md->DisplayModes[i];
			if (pr->res.width == mon->currentmode.native_width && pr->res.height == mon->currentmode.native_height)
				break;
		}
		if (md->DisplayModes[i].inuse) {
			if (!nextbest)
				break;
			while (md->DisplayModes[i].res.width == mon->currentmode.native_width && md->DisplayModes[i].res.height == mon->currentmode.native_height)
				i++;
		} else {
			i = 0;
		}
		// first iterate only modes that have similar aspect ratio
		startidx = i;
		for ( ; md->DisplayModes[i].inuse; i++) {
			struct PicassoResolution* pr = &md->DisplayModes[i];
			int r = pr->res.width > pr->res.height ? 1 : 0;
			if (pr->res.width >= mon->currentmode.native_width && pr->res.height >= mon->currentmode.native_height && r == ratio) {
				write_log(_T("FS: %dx%d -> %dx%d %d %d\n"), mon->currentmode.native_width, mon->currentmode.native_height,
					pr->res.width, pr->res.height, ratio, index);
				mon->currentmode.native_width = pr->res.width;
				mon->currentmode.native_height = pr->res.height;
				mon->currentmode.current_width = mon->currentmode.native_width;
				mon->currentmode.current_height = mon->currentmode.native_height;
				goto end;
			}
		}
		// still not match? check all modes
		i = startidx;
		for ( ; md->DisplayModes[i].inuse; i++) {
			struct PicassoResolution* pr = &md->DisplayModes[i];
			int r = pr->res.width > pr->res.height ? 1 : 0;
			if (pr->res.width >= mon->currentmode.native_width && pr->res.height >= mon->currentmode.native_height) {
				write_log(_T("FS: %dx%d -> %dx%d\n"), mon->currentmode.native_width, mon->currentmode.native_height,
					pr->res.width, pr->res.height);
				mon->currentmode.native_width = pr->res.width;
				mon->currentmode.native_height = pr->res.height;
				mon->currentmode.current_width = mon->currentmode.native_width;
				mon->currentmode.current_height = mon->currentmode.native_height;
				goto end;
			}
		}
		index++;
	}
end:
	if (index >= 0) {
		currprefs.gfx_apmode[mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display =
			changed_prefs.gfx_apmode[mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display = index;
		write_log(reinterpret_cast<const TCHAR*>(L"Can't find mode %dx%d ->\n"), mon->currentmode.native_width, mon->currentmode.native_height);
		write_log(reinterpret_cast<const TCHAR*>(L"Monitor switched to '%s'\n"), md->adaptername);
	}
	return 1;
}

// ---------------------------------------------------------------------------
// Current vblank rate
// ---------------------------------------------------------------------------

float target_getcurrentvblankrate(const int monid)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	float vb;
	if (currprefs.gfx_variable_sync)
		return static_cast<float>(mon->currentmode.freq);
	if (get_display_vblank_params(0, nullptr, nullptr, &vb, nullptr)) {
		return vb;
	}

	return amiberry_getrefreshrate(0);
}
