// Display panel options: host display, screen modes, sync, color
// adjustments, centering, scaling and the chipset refresh entries.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "amiberry_gfx_mode.h"
#include "amiberry_gfx.h"
#include "gfxboard.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

// KMSDRM owns the whole display, Android the device screen: both always
// run full-window and gfx_window.cpp enforces it on the running side, so
// the mode and size options are unavailable instead of fighting the
// enforcement the panel used to perform every frame.
bool fullscreen_only()
{
#ifdef __ANDROID__
	return true;
#else
	return kmsdrm_detected;
#endif
}

// WinUAE: RTG output exists with 24-bit addressing only for a Z2 UAEGFX
// board or a hardware board, and needs RTG memory. Without Picasso96
// support compiled in no RTG option is offered, but the descriptors (and
// so the configuration keys) exist in every build and configurations
// round-trip.
bool rtg_available(const uae_prefs& p)
{
#ifdef PICASSO96
	const bool addr_ok = !p.address_space_24
		|| gfxboard_get_configtype_of(p.rtgboards[0].rtgmem_type) == 2;
	return (addr_ok && p.rtgboards[0].rtgmem_size)
		|| p.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE;
#else
	(void)p;
	return false;
#endif
}

// ------------------------------------------------------------ host display

int host_display_count()
{
	int count = 0;
	while (count < MAX_DISPLAYS && Displays[count].monitorname)
		count++;
	return count;
}

bool gfx_display_available(const uae_prefs&, const context&)
{
	return host_display_count() > 1;
}

void gfx_display_range(const uae_prefs&, double& min, double& max)
{
	min = 0;
	max = host_display_count();
}

// The .uae value is the monitor index, 1-based, with 0 meaning the
// primary display. The choices are the monitors connected now.
std::vector<std::string> display_choice_storage;
std::vector<std::string> display_choice_values;
std::vector<amiberry_setting_choice> display_choice_items;
// What the list was built from, so it is rebuilt only when the host's
// displays change: callers hold the returned items while they check
// choices, which calls the hook again.
std::vector<std::string> display_names;
std::vector<bool> display_primary;

bool display_list_current()
{
	const int count = host_display_count();
	if (display_choice_items.empty() || static_cast<size_t>(count) != display_names.size())
		return false;
	for (int i = 0; i < count; i++) {
		if (display_names[i] != Displays[i].monitorname || display_primary[i] != Displays[i].primary)
			return false;
	}
	return true;
}

void gfx_display_dynamic_choices(const uae_prefs&, choice_list& out)
{
	if (!display_list_current()) {
		display_choice_storage.clear();
		display_choice_values.clear();
		display_names.clear();
		display_primary.clear();
		display_choice_values.emplace_back("0");
		display_choice_storage.emplace_back("Primary");
		for (int i = 0; i < host_display_count(); i++) {
			display_names.emplace_back(Displays[i].monitorname);
			display_primary.push_back(Displays[i].primary);
			std::string label = Displays[i].monitorname;
			if (Displays[i].primary)
				label += " (Primary)";
			display_choice_values.push_back(std::to_string(i + 1));
			display_choice_storage.push_back(std::move(label));
		}
		// Build the items only after every value and label has its final
		// storage, so the pointers stay valid while the vectors grow.
		display_choice_items.clear();
		for (size_t i = 0; i < display_choice_values.size(); i++) {
			display_choice_items.push_back({display_choice_values[i].c_str(),
				display_choice_storage[i].c_str()});
		}
	}
	out.items = display_choice_items.data();
	out.count = display_choice_items.size();
}

void get_gfx_display(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_apmode[APMODE_NATIVE].gfx_display);
}

void get_gfx_display_rtg(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_apmode[APMODE_RTG].gfx_display);
}

// The RTG screen's own monitor (gfx_display_rtg, read after gfx_display,
// which sets both).
void set_gfx_display_rtg(uae_prefs& p, const value& v)
{
	p.gfx_apmode[APMODE_RTG].gfx_display = static_cast<int>(v.number);
}

// The panel used to write only the native index while the parser applies
// gfx_display to both native and RTG, which left the RTG monitor silently
// divergent and persisted the divergence. One key, one value.
void set_gfx_display(uae_prefs& p, const value& v)
{
	const int display = static_cast<int>(v.number);
	p.gfx_apmode[APMODE_NATIVE].gfx_display = display;
	p.gfx_apmode[APMODE_RTG].gfx_display = display;
}

// ------------------------------------------------------ windowed resolution

bool window_sizes_available(const uae_prefs&, const context&)
{
	return !fullscreen_only();
}

void get_gfx_width_windowed(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_monitor[0].gfx_size_win.width);
}

void set_gfx_width_windowed(uae_prefs& p, const value& v)
{
	p.gfx_monitor[0].gfx_size_win.width = static_cast<int>(v.number);
}

void get_gfx_height_windowed(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_monitor[0].gfx_size_win.height);
}

void set_gfx_height_windowed(uae_prefs& p, const value& v)
{
	p.gfx_monitor[0].gfx_size_win.height = static_cast<int>(v.number);
}

void get_gfx_resize_windowed(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_windowed_resize);
}

void set_gfx_resize_windowed(uae_prefs& p, const value& v)
{
	p.gfx_windowed_resize = v.number != 0;
}

void get_borderless(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.borderless);
}

void set_borderless(uae_prefs& p, const value& v)
{
	p.borderless = v.number != 0;
}

bool borderless_available(const uae_prefs& p, const context&)
{
	return !fullscreen_only()
		&& amiberry_normalize_gfx_fullscreen_mode(p.gfx_apmode[APMODE_NATIVE].gfx_fullscreen)
			== GFX_WINDOW;
}

// ------------------------------------------------------------- screen mode

const amiberry_setting_choice fullscreen_choices[] = {
	{"false", "Windowed"},
	{"true", "Full-window (legacy exclusive)"},
	{"fullwindow", "Full-window"},
};

// The legacy exclusive value ("true") migrates to full-window, the way
// the parser does.
int fullscreen_value(const uae_prefs& p, int mode)
{
	if (p.gfx_apmode[mode].gfx_fullscreen == GFX_FULLSCREEN)
		return 1; // legacy "true"
	return p.gfx_apmode[mode].gfx_fullscreen == GFX_FULLWINDOW ? 2 : 0;
}

void get_gfx_fullscreen_amiga(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(fullscreen_choices), fullscreen_value(p, APMODE_NATIVE));
}

void set_gfx_fullscreen_amiga(uae_prefs& p, const value& v)
{
	const int mode = choice_index(CHOICES(fullscreen_choices), v.text);
	p.gfx_apmode[APMODE_NATIVE].gfx_fullscreen =
		amiberry_normalize_gfx_fullscreen_mode(mode);
}

bool fullscreen_amiga_available(const uae_prefs&, const context&)
{
	return !fullscreen_only();
}

void get_gfx_fullscreen_picasso(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(fullscreen_choices), fullscreen_value(p, APMODE_RTG));
}

// Windowed RTG cannot vsync: fixup_prefs_dimensions() clears the RTG
// vsync flags in a window, so switching RTG to a window clears what the
// core would clear instead of leaving a value the apply path drops.
void set_gfx_fullscreen_picasso(uae_prefs& p, const value& v)
{
	const int mode = choice_index(CHOICES(fullscreen_choices), v.text);
	p.gfx_apmode[APMODE_RTG].gfx_fullscreen =
		amiberry_normalize_gfx_fullscreen_mode(mode);
	if (p.gfx_apmode[APMODE_RTG].gfx_fullscreen == GFX_WINDOW) {
		p.gfx_apmode[APMODE_RTG].gfx_vsync = 0;
		p.gfx_apmode[APMODE_RTG].gfx_vsyncmode = 0;
	}
	clear_soundcard_for_vsync(p);
}

// "true" (legacy exclusive) is a configuration spelling the parser and the
// setters turn into full-window; it is not offered as a separate mode.
bool fullscreen_choice_available(const uae_prefs&, std::string_view choice)
{
	return choice != "true";
}

bool fullscreen_picasso_available(const uae_prefs& p, const context&)
{
	return rtg_available(p) && !fullscreen_only();
}

// ---------------------------------------------------------------- vsync

const amiberry_setting_choice vsync_choices[] = {
	{"false", "-"},
	{"true", "Standard"},
	{"autoswitch", "Standard 50/60Hz"},
};

void get_gfx_vsync(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(vsync_choices), p.gfx_apmode[APMODE_NATIVE].gfx_vsync);
}

// Lagless vsync (gfx_vsyncmode "busywait") waits for D3DKMT scanline
// events the Amiberry back end does not have; the panel always cleared
// it, so a vsync change normalizes the mode the same way.
void set_gfx_vsync(uae_prefs& p, const value& v)
{
	p.gfx_apmode[APMODE_NATIVE].gfx_vsync = choice_index(CHOICES(vsync_choices), v.text);
	p.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode = 0;
}

bool vsync_available(const uae_prefs&, const context&)
{
	return !kmsdrm_detected;
}

void get_gfx_vsync_picasso(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(vsync_choices), p.gfx_apmode[APMODE_RTG].gfx_vsync);
}

// The panel always cleared the sync mode: fixup_prefs_dimensions() sets
// the lagless mode again while RTG vsync is on, and a mode left from an
// earlier vsync would still read as lagless after it is turned off.
void set_gfx_vsync_picasso(uae_prefs& p, const value& v)
{
	p.gfx_apmode[APMODE_RTG].gfx_vsync = choice_index(CHOICES(vsync_choices), v.text);
	p.gfx_apmode[APMODE_RTG].gfx_vsyncmode = 0;
	clear_soundcard_for_vsync(p);
}

bool rtg_vsync_available(const uae_prefs& p, const context&)
{
	return rtg_available(p) && !kmsdrm_detected;
}

bool rtg_vsync_choice_available(const uae_prefs& p, std::string_view choice)
{
	// isvsync_rtg() treats 50/60 Hz switching like standard vsync and the
	// fixup turns either into the lagless mode, so it is reported, never
	// offered; fixup_prefs_dimensions() turns RTG vsync off in a window.
	if (choice == "autoswitch")
		return false;
	return choice == "false" || p.gfx_apmode[APMODE_RTG].gfx_fullscreen != GFX_WINDOW;
}

void get_gfx_vrr_monitor(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_variable_sync != 0);
}

// Adaptive runs standard vsync under VRR, the way the panel's combo set
// it.
void set_gfx_vrr_monitor(uae_prefs& p, const value& v)
{
	p.gfx_variable_sync = v.number != 0;
	if (p.gfx_variable_sync) {
		p.gfx_apmode[APMODE_NATIVE].gfx_vsync = 1;
		p.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode = 0;
	}
}

// ---------------------------------------------------------- simple switches

void get_gfx_blacker_than_black(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_blackerthanblack);
}

void set_gfx_blacker_than_black(uae_prefs& p, const value& v)
{
	p.gfx_blackerthanblack = v.number != 0;
}

void get_gfx_flickerfixer(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_scandoubler);
}

void set_gfx_flickerfixer(uae_prefs& p, const value& v)
{
	p.gfx_scandoubler = v.number != 0;
}

void get_gfx_monochrome(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_grayscale);
}

void set_gfx_monochrome(uae_prefs& p, const value& v)
{
	p.gfx_grayscale = v.number != 0;
}

const amiberry_setting_choice lores_mode_choices[] = {
	{"normal", "Normal"},
	{"filtered", "Filtered"},
};

void get_gfx_lores_mode(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(lores_mode_choices), p.gfx_lores_mode);
}

void set_gfx_lores_mode(uae_prefs& p, const value& v)
{
	p.gfx_lores_mode = choice_index(CHOICES(lores_mode_choices), v.text);
}

// VGA autoswitching can only trigger in a hires, line-doubled mode.
bool vga_autoswitch_available(const uae_prefs& p, const context&)
{
	return p.gfx_resolution >= 1 && p.gfx_vresolution >= 1;
}

void get_gfx_autoresolution_vga(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_autoresolution_vga);
}

void set_gfx_autoresolution_vga(uae_prefs& p, const value& v)
{
	p.gfx_autoresolution_vga = v.number != 0;
}

void get_gfx_monitorblankdelay(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_monitorblankdelay);
}

void set_gfx_monitorblankdelay(uae_prefs& p, const value& v)
{
	p.gfx_monitorblankdelay = static_cast<int>(v.number);
}

// ---------------------------------------------------- resolution and overscan

const amiberry_setting_choice resolution_choices[] = {
	{"lores", "LowRes"},
	{"hires", "HighRes (normal)"},
	{"superhires", "SuperHighRes"},
};

bool autoresol_disabled(const uae_prefs& p)
{
	return p.gfx_autoresolution == 0;
}

void get_gfx_resolution(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(resolution_choices), p.gfx_resolution);
}

// Below hires, or together with single line mode, the VGA autoswitch can
// never fire and the panel cleared it; a dependent change keeps that
// without writing during render.
void set_gfx_resolution(uae_prefs& p, const value& v)
{
	p.gfx_resolution = choice_index(CHOICES(resolution_choices), v.text);
	if (p.gfx_resolution < 1 || p.gfx_vresolution < 1)
		p.gfx_autoresolution_vga = false;
	fit_manual_crop(p);
}

bool resolution_available(const uae_prefs& p, const context&)
{
	return autoresol_disabled(p);
}

const amiberry_setting_choice overscan_choices[] = {
	{"tv_narrow", "TV Narrow"},
	{"tv_standard", "TV Standard"},
	{"tv_wide", "TV Wide"},
	{"overscan", "Overscan"},
	{"broadcast", "Broadcast"},
	{"extreme", "Extreme"},
	{"ultra", "Ultra"},
	{"ultra_hv", "Ultra H/V"},
	{"ultra_csync", "Ultra CSync"},
};

void get_gfx_overscanmode(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(overscan_choices), p.gfx_overscanmode);
}

void set_gfx_overscanmode(uae_prefs& p, const value& v)
{
	p.gfx_overscanmode = choice_index(CHOICES(overscan_choices), v.text);
}

const amiberry_setting_choice autoresolution_choices[] = {
	{"0", "Disabled"},
	{"1", "Always On"},
	{"10", "10%"},
	{"33", "33%"},
	{"66", "66%"},
};

void get_gfx_autoresolution(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_autoresolution);
}

void set_gfx_autoresolution(uae_prefs& p, const value& v)
{
	p.gfx_autoresolution = static_cast<int>(v.number);
}

// ------------------------------------------------ chipset refresh (FPS adj.)

// The .uae value of a displaydata_pal/_ntsc entry is the CSV the writer
// builds (write_chipset_refresh_entry): the rate, every field that differs
// from a plain entry, and the lock. The writer saves an entry only while it
// is locked, so the option's value is that CSV for a locked entry and "0"
// otherwise. A bare rate (the FPS adjustment) sets the rate and locks the
// entry, keeping its other fields (the PAL/NTSC binding the default entries
// carry); a full CSV replaces the entry the way the reader does
// (parse_chipset_refresh_entry) and is locked too, since an unlocked entry
// would not be saved. "0" unlocks it and follows the standard rate again.
float standard_rate(int index)
{
	return index == CHIPSET_REFRESH_NTSC ? 60.0f : 50.0f;
}

bool cr_plain(const struct chipset_refresh& cr)
{
	return !cr.commands[0] && !cr.filterprofile[0] && cr.resolution == 7
		&& cr.horiz < 0 && cr.vert < 0 && cr.lace < 0 && cr.vsync < 0 && cr.framelength < 0;
}

// The rate at the front of the value (the reader's rule: the first field,
// read as a number), or a negative number when there is none.
double displaydata_rate_of(std::string_view text)
{
	const std::string head(text.substr(0, text.find(',')));
	char* end = nullptr;
	const double rate = std::strtod(head.c_str(), &end);
	if (head.empty() || end == head.c_str() || *end != 0)
		return -1.0;
	return rate;
}

void get_displaydata(const uae_prefs& p, char* out, size_t size, int index)
{
	const struct chipset_refresh& cr = p.cr[index];
	if (!cr.locked || !cr.inuse) {
		write_int(out, size, 0); // the writer omits an unlocked entry
		return;
	}
	write_chipset_refresh_entry(&cr, index, out, size); // TCHAR is char in Amiberry builds
}

void set_displaydata(uae_prefs& p, const value& v, int index)
{
	struct chipset_refresh& cr = p.cr[index];
	// A CSV is parsed whatever its rate (a locked entry the reader loaded
	// can hold rate 0, and the getter returns it as a CSV); only a bare 0
	// resets the entry.
	const bool csv = v.text.find(',') != std::string_view::npos;
	const double rate = displaydata_rate_of(v.text);
	if (csv || rate > 0) {
		if (!csv) {
			cr.rate = static_cast<float>(rate);
		} else {
			const std::string text(v.text);
			parse_chipset_refresh_entry(text.c_str(), &cr);
		}
		// The reader names the two standard entries the same way.
		_tcscpy(cr.label, index == CHIPSET_REFRESH_NTSC ? _T("NTSC") : _T("PAL"));
		cr.locked = true;
		cr.inuse = true;
	} else {
		cr.rate = standard_rate(index);
		cr.locked = false;
		if (cr_plain(cr))
			cr.inuse = false; // WinUAE deactivates plain uncustomized entries
	}
	p.cr_selected = index; // the entry WinUAE's refresh editor selects
}

// The rate (the first field): 0 follows the standard rate; otherwise the
// 1-300 Hz the configuration reader keeps (it resets others to the
// standard rate).
bool displaydata_accepts(const uae_prefs&, const value& v)
{
	const double rate = displaydata_rate_of(v.text);
	return rate == 0 || (rate >= 1 && rate <= 300);
}

void get_displaydata_pal(const uae_prefs& p, char* out, size_t size)
{
	get_displaydata(p, out, size, CHIPSET_REFRESH_PAL);
}

void set_displaydata_pal(uae_prefs& p, const value& v)
{
	set_displaydata(p, v, CHIPSET_REFRESH_PAL);
}

void get_displaydata_ntsc(const uae_prefs& p, char* out, size_t size)
{
	get_displaydata(p, out, size, CHIPSET_REFRESH_NTSC);
}

void set_displaydata_ntsc(uae_prefs& p, const value& v)
{
	set_displaydata(p, v, CHIPSET_REFRESH_NTSC);
}

// ------------------------------------------------------ display attributes

void get_gfx_luminance(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_luminance);
}

void set_gfx_luminance(uae_prefs& p, const value& v)
{
	p.gfx_luminance = static_cast<int>(v.number);
}

void get_gfx_contrast(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_contrast);
}

void set_gfx_contrast(uae_prefs& p, const value& v)
{
	p.gfx_contrast = static_cast<int>(v.number);
}

void get_gfx_gamma(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_gamma);
}

void set_gfx_gamma(uae_prefs& p, const value& v)
{
	p.gfx_gamma = static_cast<int>(v.number);
}

void get_gfx_gamma_r(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_gamma_ch[0]);
}

void set_gfx_gamma_r(uae_prefs& p, const value& v)
{
	p.gfx_gamma_ch[0] = static_cast<int>(v.number);
}

void get_gfx_gamma_g(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_gamma_ch[1]);
}

void set_gfx_gamma_g(uae_prefs& p, const value& v)
{
	p.gfx_gamma_ch[1] = static_cast<int>(v.number);
}

void get_gfx_gamma_b(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_gamma_ch[2]);
}

void set_gfx_gamma_b(uae_prefs& p, const value& v)
{
	p.gfx_gamma_ch[2] = static_cast<int>(v.number);
}

const amiberry_setting_choice threebitcolors_choices[] = {
	{"disabled", "Disabled"},
	{"3to4to8bit", "3 to 4/8 bit"},
	{"3to4bit", "3 to 4 bit"},
	{"3to8bit", "3 to 8 bit"},
};

void get_gfx_atari_palette_fix(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(threebitcolors_choices), p.gfx_threebitcolors);
}

void set_gfx_atari_palette_fix(uae_prefs& p, const value& v)
{
	p.gfx_threebitcolors = choice_index(CHOICES(threebitcolors_choices), v.text);
}

// ------------------------------------------------------------ centering

const amiberry_setting_choice center_choices[] = {
	{"none", "None"},
	{"simple", "Simple"},
	{"smart", "Smart"},
};

// target_fixup_options() clears both centering modes under auto-crop.
bool centering_available(const uae_prefs& p, const context&)
{
	return p.gfx_overscanmode <= OVERSCANMODE_OVERSCAN && !p.gfx_auto_crop;
}

void get_gfx_center_horizontal(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(center_choices), p.gfx_xcenter);
}

void set_gfx_center_horizontal(uae_prefs& p, const value& v)
{
	p.gfx_xcenter = choice_index(CHOICES(center_choices), v.text);
}

void get_gfx_center_vertical(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(center_choices), p.gfx_ycenter);
}

void set_gfx_center_vertical(uae_prefs& p, const value& v)
{
	p.gfx_ycenter = choice_index(CHOICES(center_choices), v.text);
}

// ------------------------------------------------------- aspect and scaling

void get_gfx_correct_aspect(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_correct_aspect);
}

void set_gfx_correct_aspect(uae_prefs& p, const value& v)
{
	p.gfx_correct_aspect = static_cast<int>(v.number);
}

// Stretch fills the window, so aspect correction cannot apply.
bool correct_aspect_available(const uae_prefs& p, const context&)
{
	return p.scaling_method != 3;
}

void get_gfx_ntscpixelspal(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_ntscpixelspal);
}

void set_gfx_ntscpixelspal(uae_prefs& p, const value& v)
{
	p.gfx_ntscpixelspal = v.number != 0;
}

const amiberry_setting_choice scaling_method_choices[] = {
	{"-1", "Auto"},
	{"0", "Nearest"},
	{"1", "Linear"},
	{"2", "Integer"},
	{"3", "Stretch"},
};

void get_scaling_method(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.scaling_method);
}

void set_scaling_method(uae_prefs& p, const value& v)
{
	p.scaling_method = static_cast<int>(v.number);
}

// ------------------------------------------------------------- line mode

// One composite .uae key for the (line doubling, progressive scanlines,
// interlace scanlines) tuple, indexed exactly as cfgfile writes it:
// interlace mode * 4 + scanline mode + 1, and 0 for single line.
const amiberry_setting_choice linemode_choices[] = {
	{"none", "Single"},
	{"double", "Double"},
	{"scanlines", "Scanlines"},
	{"scanlines2p", "Scanlines 2+"},
	{"scanlines3p", "Scanlines 3+"},
	{"double2", "Double, frames"},
	{"scanlines2", "Double, fields"},
	{"scanlines2p2", "Double, fields+"},
	{"scanlines2p3", "Double, fields++"},
	{"double3", "Double, frames (3x)"},
	{"scanlines3", "Double, fields (3x)"},
	{"scanlines3p2", "Double, fields+ (3x)"},
	{"scanlines3p3", "Double, fields++ (3x)"},
};

// The canonical tuple the panel enforced every frame: single line mode
// has no scanline variants, and out-of-range values reset to none.
int linemode_index(const uae_prefs& p)
{
	if (p.gfx_vresolution == 0)
		return 0;
	int scanlines = p.gfx_pscanlines;
	if (scanlines < 0 || scanlines > 3)
		scanlines = 0;
	int interlaced = p.gfx_iscanlines;
	if (interlaced < 0 || interlaced > 2)
		interlaced = 0;
	return interlaced * 4 + scanlines + 1;
}

void get_gfx_linemode(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(linemode_choices), linemode_index(p));
}

void set_gfx_linemode(uae_prefs& p, const value& v)
{
	const int index = choice_index(CHOICES(linemode_choices), v.text);
	if (index <= 0) {
		p.gfx_vresolution = 0;
		p.gfx_pscanlines = 0;
		p.gfx_iscanlines = 0;
	} else {
		p.gfx_vresolution = 1;
		p.gfx_pscanlines = (index - 1) % 4;
		p.gfx_iscanlines = (index - 1) / 4;
	}
	if (p.gfx_vresolution < 1)
		p.gfx_autoresolution_vga = false;
	fit_manual_crop(p);
}

bool linemode_available(const uae_prefs& p, const context&)
{
	return autoresol_disabled(p);
}

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
#define DYNAMIC_CHOICE_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define NUMBER_OPTION(id, group, label, help, type, apply, min, max, step, def, constraint) \
	{id, group, label, help, type, apply, min, max, step, def, nullptr, 0, constraint}
#define STRING_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_STRING, apply, 0, 0, 0, def, nullptr, 0, constraint}

const definition definitions[] = {
	{DYNAMIC_CHOICE_OPTION("gfx_display", "display", "Host display",
			"Which connected monitor shows the Amiga screen; the primary display when 0.",
			AMIBERRY_APPLY_LIVE, "0",
			"Needs more than one connected monitor. Applies to the native and the RTG screen."),
		get_gfx_display, set_gfx_display, gfx_display_available, nullptr, gfx_display_range,
		nullptr, gfx_display_dynamic_choices},
	{DYNAMIC_CHOICE_OPTION("gfx_display_rtg", "rtg", "RTG host display",
			"Which connected monitor shows the RTG screen; the primary display when 0.",
			AMIBERRY_APPLY_LIVE, "0", "Needs more than one connected monitor."),
		get_gfx_display_rtg, set_gfx_display_rtg, gfx_display_available, nullptr, gfx_display_range,
		nullptr, gfx_display_dynamic_choices},
	{NUMBER_OPTION("gfx_width_windowed", "display", "Windowed width",
			"Width of the emulator window in windowed mode.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 160, 8192, 4, "720",
			"Multiples of 4 between 160 and 8192, the range the core's dimension fixup keeps."),
		get_gfx_width_windowed, set_gfx_width_windowed, window_sizes_available, nullptr, nullptr,
		nullptr},
	{NUMBER_OPTION("gfx_height_windowed", "display", "Windowed height",
			"Height of the emulator window in windowed mode.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 128, 8192, 1, "568",
			"Between 128 and 8192, the range the core's dimension fixup keeps."),
		get_gfx_height_windowed, set_gfx_height_windowed, window_sizes_available, nullptr, nullptr,
		nullptr},
	{BOOL_OPTION("gfx_resize_windowed", "display", "Window resize",
			"Allow resizing the emulator window by dragging its edges.",
			AMIBERRY_APPLY_LIVE, "true", "Not available while the display mode is forced full-window."),
		get_gfx_resize_windowed, set_gfx_resize_windowed, window_sizes_available, nullptr, nullptr,
		nullptr},
	{BOOL_OPTION("amiberry.borderless", "display", "Borderless",
			"Remove the window title bar and borders in windowed mode.",
			AMIBERRY_APPLY_LIVE, "false", "Only in windowed native mode."),
		get_borderless, set_borderless, borderless_available, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_fullscreen_amiga", "display", "Native screen mode",
			"Show the native chipset screen in a window or full-window (borderless desktop).",
			AMIBERRY_APPLY_LIVE, "false", fullscreen_choices,
			"KMSDRM and Android always use the display full-window."),
		get_gfx_fullscreen_amiga, set_gfx_fullscreen_amiga, fullscreen_amiga_available,
		fullscreen_choice_available, nullptr, nullptr},
	{CHOICE_OPTION("gfx_vsync", "display", "Native vertical sync",
			"Standard waits for vertical blank, 50/60Hz enforces the emulated refresh rate.",
			AMIBERRY_APPLY_LIVE, "false", vsync_choices,
			"Not available under KMSDRM, which paces with the console driver."),
		get_gfx_vsync, set_gfx_vsync, vsync_available, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfx_vrr_monitor", "display", "Adaptive sync",
			"Use VRR (FreeSync/G-SYNC) with a late-swap tearing fallback; needs Standard vsync.",
			AMIBERRY_APPLY_LIVE, "false", "Not available under KMSDRM."),
		get_gfx_vrr_monitor, set_gfx_vrr_monitor, vsync_available, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_fullscreen_picasso", "display", "RTG screen mode",
			"Show the Picasso96 (graphics card) screen in a window or full-window.",
			AMIBERRY_APPLY_LIVE, "false", fullscreen_choices,
			"Needs an RTG board with memory. KMSDRM and Android always use the display full-window."),
		get_gfx_fullscreen_picasso, set_gfx_fullscreen_picasso, fullscreen_picasso_available,
		fullscreen_choice_available, nullptr, nullptr},
	{CHOICE_OPTION("gfx_vsync_picasso", "display", "RTG vertical sync",
			"Vertical sync for the Picasso96 screen.",
			AMIBERRY_APPLY_LIVE, "false", vsync_choices,
			"Needs an RTG board, is not available under KMSDRM, and a windowed RTG screen "
			"cannot vsync because the core turns it off."),
		get_gfx_vsync_picasso, set_gfx_vsync_picasso, rtg_vsync_available,
		rtg_vsync_choice_available, nullptr, nullptr},
	{BOOL_OPTION("gfx_blacker_than_black", "display", "Blacker than black",
			"Enable proper black levels for displays that support it.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_gfx_blacker_than_black, set_gfx_blacker_than_black, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfx_flickerfixer", "display", "Remove interlace artifacts",
			"Apply scanline doubling to reduce the flicker of interlaced video modes.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_gfx_flickerfixer, set_gfx_flickerfixer, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfx_monochrome", "display", "Monochrome video out",
			"Render the display in grayscale, simulating a monochrome monitor.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_gfx_monochrome, set_gfx_monochrome, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_lores_mode", "display", "Filtered low resolution",
			"Apply filtering to low resolution modes for a smoother appearance.",
			AMIBERRY_APPLY_LIVE, "normal", lores_mode_choices, nullptr),
		get_gfx_lores_mode, set_gfx_lores_mode, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfx_autoresolution_vga", "display", "VGA mode resolution autoswitch",
			"Switch the render resolution for VGA modes when they are detected.",
			AMIBERRY_APPLY_LIVE, "true",
			"Needs HighRes and Double line mode, the only modes where it can trigger."),
		get_gfx_autoresolution_vga, set_gfx_autoresolution_vga, vga_autoswitch_available, nullptr,
		nullptr, nullptr},
	{NUMBER_OPTION("gfx_monitorblankdelay", "display", "Display resync blanking",
			"Blank the screen for this many milliseconds after a display mode change.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 0, 60000, 1, "0", nullptr),
		get_gfx_monitorblankdelay, set_gfx_monitorblankdelay, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_resolution", "display", "Amiga resolution",
			"Internal render resolution relative to the Amiga's video output.",
			AMIBERRY_APPLY_LIVE, "hires", resolution_choices,
			"Not available while resolution autoswitch follows the Amiga's screen mode."),
		get_gfx_resolution, set_gfx_resolution, resolution_available, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_overscanmode", "display", "Overscan",
			"Amount of border area to display around the Amiga picture.",
			AMIBERRY_APPLY_LIVE, "overscan", overscan_choices, nullptr),
		get_gfx_overscanmode, set_gfx_overscanmode, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_autoresolution", "display", "Resolution autoswitch",
			"Follow the Amiga's current screen mode; the percentages are the share of the frame "
			"that must switch before the render resolution changes.",
			AMIBERRY_APPLY_LIVE, "0", autoresolution_choices, nullptr),
		get_gfx_autoresolution, set_gfx_autoresolution, nullptr, nullptr, nullptr, nullptr},
	{STRING_OPTION("displaydata_pal", "display", "PAL refresh entry",
			"The custom PAL refresh entry: the rate in Hz, then optional fields (,locked,pal,lace,...). "
			"0 follows the PAL standard of 50 Hz.",
			AMIBERRY_APPLY_LIVE, "0",
			"The writer saves the entry only while it is locked, so a rate locks it."),
		get_displaydata_pal, set_displaydata_pal, nullptr, nullptr, nullptr, displaydata_accepts},
	{STRING_OPTION("displaydata_ntsc", "display", "NTSC refresh entry",
			"The custom NTSC refresh entry: the rate in Hz, then optional fields (,locked,ntsc,lace,...). "
			"0 follows the NTSC standard of 60 Hz.",
			AMIBERRY_APPLY_LIVE, "0",
			"The writer saves the entry only while it is locked, so a rate locks it."),
		get_displaydata_ntsc, set_displaydata_ntsc, nullptr, nullptr, nullptr, displaydata_accepts},
	{NUMBER_OPTION("gfx_luminance", "display", "Brightness",
			"Brightness adjustment; the panel works in tenths, so 1000 is +100.0.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -2000, 2000, 1, "0", nullptr),
		get_gfx_luminance, set_gfx_luminance, nullptr, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("gfx_contrast", "display", "Contrast",
			"Contrast adjustment; the panel works in tenths, so 1000 is +100.0.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -2000, 2000, 1, "0", nullptr),
		get_gfx_contrast, set_gfx_contrast, nullptr, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("gfx_gamma", "display", "Gamma",
			"Gamma adjustment; the panel works in tenths, so 1000 is +100.0.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -2000, 2000, 1, "0", nullptr),
		get_gfx_gamma, set_gfx_gamma, nullptr, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("gfx_gamma_r", "display", "Gamma red",
			"Red channel gamma adjustment in tenths.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -2000, 2000, 1, "0", nullptr),
		get_gfx_gamma_r, set_gfx_gamma_r, nullptr, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("gfx_gamma_g", "display", "Gamma green",
			"Green channel gamma adjustment in tenths.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -2000, 2000, 1, "0", nullptr),
		get_gfx_gamma_g, set_gfx_gamma_g, nullptr, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("gfx_gamma_b", "display", "Gamma blue",
			"Blue channel gamma adjustment in tenths.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -2000, 2000, 1, "0", nullptr),
		get_gfx_gamma_b, set_gfx_gamma_b, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_atari_palette_fix", "display", "Dark palette fix",
			"Convert 3-bit color palettes so dark shades stay distinguishable.",
			AMIBERRY_APPLY_LIVE, "disabled", threebitcolors_choices, nullptr),
		get_gfx_atari_palette_fix, set_gfx_atari_palette_fix, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_center_horizontal", "display", "Horizontal centering",
			"Center the display horizontally; Smart follows the picture.",
			AMIBERRY_APPLY_LIVE, "none", center_choices,
			"Not available above the Overscan border setting, which uses the whole area."),
		get_gfx_center_horizontal, set_gfx_center_horizontal, centering_available, nullptr,
		nullptr, nullptr},
	{CHOICE_OPTION("gfx_center_vertical", "display", "Vertical centering",
			"Center the display vertically; Smart follows the picture.",
			AMIBERRY_APPLY_LIVE, "none", center_choices,
			"Not available above the Overscan border setting, which uses the whole area."),
		get_gfx_center_vertical, set_gfx_center_vertical, centering_available, nullptr, nullptr,
		nullptr},
	{NUMBER_OPTION("amiberry.gfx_correct_aspect", "display", "Correct aspect ratio",
			"Apply Amiga display aspect correction to native modes.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 0, 1, 1, "1", "Not available with the Stretch scaling method."),
		get_gfx_correct_aspect, set_gfx_correct_aspect, correct_aspect_available, nullptr, nullptr,
		nullptr},
	{BOOL_OPTION("gfx_ntscpixelspal", "display", "Always stretch PAL as NTSC",
			"Show PAL modes with NTSC vertical proportions. The bottom of the PAL display is cut.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_gfx_ntscpixelspal, set_gfx_ntscpixelspal, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.scaling_method", "display", "Scaling method",
			"How the display is scaled to fit the window.",
			AMIBERRY_APPLY_LIVE, "-1", scaling_method_choices, nullptr),
		get_scaling_method, set_scaling_method, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_linemode", "display", "Line mode",
			"Line doubling and scanline effects for progressive and interlaced modes.",
			AMIBERRY_APPLY_LIVE, "double2", linemode_choices,
			"Not available while resolution autoswitch follows the Amiga's screen mode."),
		get_gfx_linemode, set_gfx_linemode, linemode_available, nullptr, nullptr, nullptr},
};

bool commit_display(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(gfx_apmode[APMODE_NATIVE].gfx_display);
	FIELD(gfx_apmode[APMODE_RTG].gfx_display);
	FIELD(gfx_monitor[0].gfx_size_win.width);
	FIELD(gfx_monitor[0].gfx_size_win.height);
	FIELD(gfx_windowed_resize);
	FIELD(borderless);
	FIELD(gfx_apmode[APMODE_NATIVE].gfx_fullscreen);
	FIELD(gfx_apmode[APMODE_RTG].gfx_fullscreen);
	FIELD(gfx_apmode[APMODE_NATIVE].gfx_vsync);
	FIELD(gfx_apmode[APMODE_NATIVE].gfx_vsyncmode);
	FIELD(gfx_apmode[APMODE_RTG].gfx_vsync);
	FIELD(gfx_apmode[APMODE_RTG].gfx_vsyncmode);
	FIELD(gfx_variable_sync);
	FIELD(gfx_blackerthanblack);
	FIELD(gfx_scandoubler);
	FIELD(gfx_grayscale);
	FIELD(gfx_lores_mode);
	FIELD(gfx_autoresolution_vga);
	FIELD(gfx_monitorblankdelay);
	FIELD(gfx_resolution);
	FIELD(gfx_overscanmode);
	FIELD(gfx_autoresolution);
	// Each entry is its own option (displaydata_pal, displaydata_ntsc), so
	// they are merged one by one.
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++)
		FIELD(cr[i]);
	FIELD(cr_selected);
	FIELD(gfx_luminance);
	FIELD(gfx_contrast);
	FIELD(gfx_gamma);
	FIELD(gfx_gamma_ch);
	FIELD(gfx_threebitcolors);
	FIELD(gfx_xcenter);
	FIELD(gfx_ycenter);
	FIELD(gfx_correct_aspect);
	FIELD(gfx_ntscpixelspal);
	FIELD(scaling_method);
	FIELD(gfx_vresolution);
	FIELD(gfx_pscanlines);
	FIELD(gfx_iscanlines);
#undef FIELD
	return wrote;
}

const domain display = {
	definitions, sizeof definitions / sizeof definitions[0], commit_display, nullptr,
};

} // namespace

const domain& display_domain()
{
	return display;
}

} // namespace amiberry_settings
