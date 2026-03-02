#include "sysdeps.h"

#ifdef AMIBERRY

#include <cstring>
#include <iostream>
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "events.h"
#include "xwin.h"
#include "drawing.h"
#include "gui.h"
#include "amiberry_gfx.h"
#include "irenderer.h"
#include "sounddep/sound.h"
#include "inputdevice.h"
#ifdef WITH_MIDI
#include "midi.h"
#endif
#include "serial.h"
#include "parallel.h"
#include "sampler.h"

#include "vkbd/vkbd.h"
#include "on_screen_joystick.h"

#ifdef WITH_MIDIEMU
#include "midiemu.h"
#endif

#include "target.h"
#include "gfx_colors.h"
#include "gfx_prefs_check.h"

static int display_change_requested;

void graphics_reset(const bool forced)
{
	if (forced) {
		display_change_requested = 2;
	} else {
		// full reset if display size can't be changed.
		if (currprefs.gfx_api) {
			display_change_requested = 3;
		} else {
			display_change_requested = 2;
		}
	}
}

void gfx_DisplayChangeRequested(const int mode)
{
	display_change_requested = mode;
}

int check_prefs_changed_gfx()
{
	int c = 0;
	bool monitors[MAX_AMIGAMONITORS]{};

	if (!config_changed && !display_change_requested)
		return 0;

	c |= config_changed_flags;
	config_changed_flags = 0;

	//c |= currprefs.win32_statusbar != changed_prefs.win32_statusbar ? 512 : 0;

	for (int i = 0; i < MAX_AMIGADISPLAYS; i++) {
		monitors[i] = false;
		int c2 = 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_fs.width != changed_prefs.gfx_monitor[i].gfx_size_fs.width ? 16 : 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_fs.height != changed_prefs.gfx_monitor[i].gfx_size_fs.height ? 16 : 0;
		c2 |= ((currprefs.gfx_monitor[i].gfx_size_win.width + 7) & ~7) != ((changed_prefs.gfx_monitor[i].gfx_size_win.width + 7) & ~7) ? 16 : 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_win.height != changed_prefs.gfx_monitor[i].gfx_size_win.height ? 16 : 0;
#ifdef AMIBERRY
		c2 |= currprefs.gfx_horizontal_offset != changed_prefs.gfx_horizontal_offset ? 16 : 0;
		c2 |= currprefs.gfx_vertical_offset != changed_prefs.gfx_vertical_offset ? 16 : 0;
		c2 |= currprefs.gfx_auto_crop != changed_prefs.gfx_auto_crop ? 16 : 0;
		c2 |= currprefs.gfx_manual_crop != changed_prefs.gfx_manual_crop ? 16 : 0;
		c2 |= currprefs.gfx_manual_crop_width != changed_prefs.gfx_manual_crop_width ? 16 : 0;
		c2 |= currprefs.gfx_manual_crop_height != changed_prefs.gfx_manual_crop_height ? 16 : 0;
		c2 |= currprefs.gfx_correct_aspect != changed_prefs.gfx_correct_aspect ? 16 : 0;
		c2 |= currprefs.scaling_method != changed_prefs.scaling_method ? 16 : 0;
#endif
		if (c2) {
			if (i > 0) {
				for (auto& rtgboard : changed_prefs.rtgboards) {
					struct rtgboardconfig* rbc = &rtgboard;
					if (rbc->monitor_id == i) {
						c |= c2;
						monitors[i] = true;
					}
				}
				if (!monitors[i]) {
					currprefs.gfx_monitor[i].gfx_size_fs.width = changed_prefs.gfx_monitor[i].gfx_size_fs.width;
					currprefs.gfx_monitor[i].gfx_size_fs.height = changed_prefs.gfx_monitor[i].gfx_size_fs.height;
					currprefs.gfx_monitor[i].gfx_size_win.width = changed_prefs.gfx_monitor[i].gfx_size_win.width;
					currprefs.gfx_monitor[i].gfx_size_win.height = changed_prefs.gfx_monitor[i].gfx_size_win.height;
				}
			} else {
				c |= c2;
				monitors[i] = true;
			}
		}
		if (AMonitors[i].screen_is_picasso) {
			struct gfx_filterdata* gfc = &changed_prefs.gf[1];
			if (gfc->changed) {
				gfc->changed = false;
				c |= 16;
			}
		} else {
			struct gfx_filterdata* gfc1 = &changed_prefs.gf[0];
			struct gfx_filterdata* gfc2 = &changed_prefs.gf[2];
			if (gfc1->changed || gfc2->changed) {
				gfc1->changed = false;
				gfc2->changed = false;
				c |= 16;
			}
		}
	}
	if (currprefs.gf[2].enable != changed_prefs.gf[2].enable) {
		currprefs.gf[2].enable = changed_prefs.gf[2].enable;
		c |= 512;
	}

	monitors[0] = true;

	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen ? 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen != changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen ? 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_vsync != changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsync ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode != changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate ? 2 | 16 : 0;
	c |= currprefs.gfx_autoresolution != changed_prefs.gfx_autoresolution ? (2 | 8 | 16) : 0;
	c |= currprefs.gfx_autoresolution_vga != changed_prefs.gfx_autoresolution_vga ? (2 | 8 | 16) : 0;
	c |= currprefs.gfx_api != changed_prefs.gfx_api ? (1 | 8 | 32) : 0;
	c |= currprefs.gfx_api_options != changed_prefs.gfx_api_options ? (1 | 8 | 32) : 0;
	c |= currprefs.lightboost_strobo != changed_prefs.lightboost_strobo ? (2 | 16) : 0;

	for (int j = 0; j < MAX_FILTERDATA; j++) {
		struct gfx_filterdata* gf = &currprefs.gf[j];
		struct gfx_filterdata* gfc = &changed_prefs.gf[j];

		c |= gf->gfx_filter != gfc->gfx_filter ? (2 | 8) : 0;
		c |= gf->gfx_filter != gfc->gfx_filter ? (2 | 8) : 0;

		for (int i = 0; i <= 2 * MAX_FILTERSHADERS; i++) {
			c |= _tcscmp(gf->gfx_filtershader[i], gfc->gfx_filtershader[i]) ? (2 | 8) : 0;
			c |= _tcscmp(gf->gfx_filtermask[i], gfc->gfx_filtermask[i]) ? (2 | 8) : 0;
		}
		c |= _tcscmp(gf->gfx_filteroverlay, gfc->gfx_filteroverlay) ? (2 | 8) : 0;

		c |= gf->gfx_filter_scanlines != gfc->gfx_filter_scanlines ? (1 | 8) : 0;
		c |= gf->gfx_filter_scanlinelevel != gfc->gfx_filter_scanlinelevel ? (1 | 8) : 0;
		c |= gf->gfx_filter_scanlineratio != gfc->gfx_filter_scanlineratio ? (1 | 8) : 0;
		c |= gf->gfx_filter_scanlineoffset != gfc->gfx_filter_scanlineoffset ? (1 | 8) : 0;

		c |= gf->gfx_filter_horiz_zoom_mult != gfc->gfx_filter_horiz_zoom_mult ? (1) : 0;
		c |= gf->gfx_filter_vert_zoom_mult != gfc->gfx_filter_vert_zoom_mult ? (1) : 0;

		c |= gf->gfx_filter_filtermodeh != gfc->gfx_filter_filtermodeh ? (2 | 8) : 0;
		c |= gf->gfx_filter_filtermodev != gfc->gfx_filter_filtermodev ? (2 | 8) : 0;
		c |= gf->gfx_filter_bilinear != gfc->gfx_filter_bilinear ? (2 | 8 | 16) : 0;
		c |= gf->gfx_filter_noise != gfc->gfx_filter_noise ? (1) : 0;
		c |= gf->gfx_filter_blur != gfc->gfx_filter_blur ? (1) : 0;

		c |= gf->gfx_filter_aspect != gfc->gfx_filter_aspect ? (1) : 0;
		c |= gf->gfx_filter_rotation != gfc->gfx_filter_rotation ? (1) : 0;
		c |= gf->gfx_filter_keep_aspect != gfc->gfx_filter_keep_aspect ? (1) : 0;
		c |= gf->gfx_filter_keep_autoscale_aspect != gfc->gfx_filter_keep_autoscale_aspect ? (1) : 0;
		c |= gf->gfx_filter_luminance != gfc->gfx_filter_luminance ? (1) : 0;
		c |= gf->gfx_filter_contrast != gfc->gfx_filter_contrast ? (1) : 0;
		c |= gf->gfx_filter_saturation != gfc->gfx_filter_saturation ? (1) : 0;
		c |= gf->gfx_filter_gamma != gfc->gfx_filter_gamma ? (1) : 0;
		c |= gf->gfx_filter_integerscalelimit != gfc->gfx_filter_integerscalelimit ? (1) : 0;
		if (j && gf->gfx_filter_autoscale != gfc->gfx_filter_autoscale)
			c |= 8 | 64;
	}

	c |= currprefs.rtg_horiz_zoom_mult != changed_prefs.rtg_horiz_zoom_mult ? 16 : 0;
	c |= currprefs.rtg_vert_zoom_mult != changed_prefs.rtg_vert_zoom_mult ? 16 : 0;

	c |= currprefs.gfx_luminance != changed_prefs.gfx_luminance ? (1 | 256) : 0;
	c |= currprefs.gfx_contrast != changed_prefs.gfx_contrast ? (1 | 256) : 0;
	c |= currprefs.gfx_gamma != changed_prefs.gfx_gamma ? (1 | 256) : 0;

	c |= currprefs.gfx_resolution != changed_prefs.gfx_resolution ? (128) : 0;
	c |= currprefs.gfx_vresolution != changed_prefs.gfx_vresolution ? (128) : 0;
	c |= currprefs.gfx_autoresolution_minh != changed_prefs.gfx_autoresolution_minh ? (128) : 0;
	c |= currprefs.gfx_autoresolution_minv != changed_prefs.gfx_autoresolution_minv ? (128) : 0;
	c |= currprefs.gfx_iscanlines != changed_prefs.gfx_iscanlines ? (2 | 8) : 0;
	c |= currprefs.gfx_pscanlines != changed_prefs.gfx_pscanlines ? (2 | 8) : 0;

	c |= currprefs.monitoremu != changed_prefs.monitoremu ? (2 | 8) : 0;
	c |= currprefs.genlock_image != changed_prefs.genlock_image ? (2 | 8) : 0;
	c |= currprefs.genlock != changed_prefs.genlock ? (2 | 8) : 0;
	c |= currprefs.genlock_alpha != changed_prefs.genlock_alpha ? (1 | 8) : 0;
	c |= currprefs.genlock_mix != changed_prefs.genlock_mix ? (1 | 256) : 0;
	c |= currprefs.genlock_aspect != changed_prefs.genlock_aspect ? (1 | 256) : 0;
	c |= currprefs.genlock_scale != changed_prefs.genlock_scale ? (1 | 256) : 0;
	c |= currprefs.genlock_offset_x != changed_prefs.genlock_offset_x ? (1 | 256) : 0;
	c |= currprefs.genlock_offset_y != changed_prefs.genlock_offset_y ? (1 | 256) : 0;
	c |= _tcsicmp(currprefs.genlock_image_file, changed_prefs.genlock_image_file) ? (2 | 8) : 0;
	c |= _tcsicmp(currprefs.genlock_video_file, changed_prefs.genlock_video_file) ? (2 | 8) : 0;

	c |= currprefs.gfx_lores_mode != changed_prefs.gfx_lores_mode ? (2 | 8) : 0;
	c |= currprefs.gfx_overscanmode != changed_prefs.gfx_overscanmode ? (2 | 8) : 0;
	c |= currprefs.gfx_scandoubler != changed_prefs.gfx_scandoubler ? (2 | 8) : 0;
	c |= currprefs.gfx_threebitcolors != changed_prefs.gfx_threebitcolors ? (256) : 0;
	c |= currprefs.gfx_grayscale != changed_prefs.gfx_grayscale ? (512) : 0;
	c |= currprefs.gfx_ntscpixels != changed_prefs.gfx_ntscpixels ? (512) : 0;
	c |= currprefs.gfx_monitorblankdelay != changed_prefs.gfx_monitorblankdelay ? (512) : 0;

	c |= currprefs.gfx_display_sections != changed_prefs.gfx_display_sections ? (512) : 0;
	c |= currprefs.gfx_variable_sync != changed_prefs.gfx_variable_sync ? 1 : 0;
	c |= currprefs.gfx_windowed_resize != changed_prefs.gfx_windowed_resize ? 1 : 0;

	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_display != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_display ? (2 | 4 | 8) : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_display != changed_prefs.gfx_apmode[APMODE_RTG].gfx_display ? (2 | 4 | 8) : 0;
	c |= currprefs.gfx_blackerthanblack != changed_prefs.gfx_blackerthanblack ? (2 | 8) : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers ? (2 | 16) : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced ? (2 | 8) : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_backbuffers != changed_prefs.gfx_apmode[APMODE_RTG].gfx_backbuffers ? (2 | 16) : 0;

	c |= currprefs.main_alwaysontop != changed_prefs.main_alwaysontop ? 32 : 0;
	c |= currprefs.gui_alwaysontop != changed_prefs.gui_alwaysontop ? 2 : 0;
	c |= currprefs.borderless != changed_prefs.borderless ? 32 : 0;
	c |= currprefs.blankmonitors != changed_prefs.blankmonitors ? 32 : 0;
	c |= currprefs.rtgallowscaling != changed_prefs.rtgallowscaling ? (2 | 8 | 64) : 0;
	c |= currprefs.rtgscaleaspectratio != changed_prefs.rtgscaleaspectratio ? (8 | 64) : 0;
	c |= currprefs.rtgvblankrate != changed_prefs.rtgvblankrate ? 8 : 0;

#ifdef AMIBERRY
	// These are missing from WinUAE
	c |= currprefs.rtg_hardwareinterrupt != changed_prefs.rtg_hardwareinterrupt ? 32 : 0;
	c |= currprefs.rtg_hardwaresprite != changed_prefs.rtg_hardwaresprite ? 32 : 0;
	c |= currprefs.rtg_multithread != changed_prefs.rtg_multithread ? 32 : 0;
	c |= currprefs.rtg_zerocopy != changed_prefs.rtg_zerocopy ? 32 : 0;
#endif

	if (display_change_requested || c)
	{
		bool setpause = false;
		bool dontcapture = false;
		int keepfsmode =
			currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen == changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen &&
			currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen == changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen;

		currprefs.gfx_autoresolution = changed_prefs.gfx_autoresolution;
		currprefs.gfx_autoresolution_vga = changed_prefs.gfx_autoresolution_vga;
		currprefs.lightboost_strobo = changed_prefs.lightboost_strobo;

		if (currprefs.gfx_api != changed_prefs.gfx_api) {
			display_change_requested = 1;
		}

#ifdef RETROPLATFORM
		if (c & 128) {
			// hres/vres change
			rp_screenmode_changed();
		}
#endif

		if (display_change_requested) {
			if (display_change_requested == 3) {
				c = 1024;
			} else if (display_change_requested == 2) {
				c = 512;
			} else if (display_change_requested == 4) {
				c = 32;
			} else {
				c = 2;
				keepfsmode = 0;
				if (display_change_requested <= -1) {
					dontcapture = true;
					if (display_change_requested == -2)
						setpause = true;
					if (pause_emulation)
						setpause = true;
				}
			}
			display_change_requested = 0;
		}

		for (int j = 0; j < MAX_FILTERDATA; j++) {
			struct gfx_filterdata* gf = &currprefs.gf[j];
			struct gfx_filterdata* gfc = &changed_prefs.gf[j];
			memcpy(gf, gfc, sizeof(struct gfx_filterdata));
		}

#ifdef AMIBERRY
		currprefs.gfx_horizontal_offset = changed_prefs.gfx_horizontal_offset;
		currprefs.gfx_vertical_offset = changed_prefs.gfx_vertical_offset;
		currprefs.gfx_manual_crop_width = changed_prefs.gfx_manual_crop_width;
		currprefs.gfx_manual_crop_height = changed_prefs.gfx_manual_crop_height;
		currprefs.gfx_auto_crop = changed_prefs.gfx_auto_crop;
		currprefs.gfx_manual_crop = changed_prefs.gfx_manual_crop;

		if (currprefs.gfx_auto_crop)
		{
			changed_prefs.gfx_xcenter = changed_prefs.gfx_ycenter = 0;
		}
		currprefs.gfx_correct_aspect = changed_prefs.gfx_correct_aspect;
		currprefs.scaling_method = changed_prefs.scaling_method;
#endif

		currprefs.rtg_horiz_zoom_mult = changed_prefs.rtg_horiz_zoom_mult;
		currprefs.rtg_vert_zoom_mult = changed_prefs.rtg_vert_zoom_mult;

		currprefs.gfx_luminance = changed_prefs.gfx_luminance;
		currprefs.gfx_contrast = changed_prefs.gfx_contrast;
		currprefs.gfx_gamma = changed_prefs.gfx_gamma;

		currprefs.gfx_resolution = changed_prefs.gfx_resolution;
		currprefs.gfx_vresolution = changed_prefs.gfx_vresolution;
		currprefs.gfx_autoresolution_minh = changed_prefs.gfx_autoresolution_minh;
		currprefs.gfx_autoresolution_minv = changed_prefs.gfx_autoresolution_minv;
		currprefs.gfx_iscanlines = changed_prefs.gfx_iscanlines;
		currprefs.gfx_pscanlines = changed_prefs.gfx_pscanlines;
		currprefs.monitoremu = changed_prefs.monitoremu;

		currprefs.genlock_image = changed_prefs.genlock_image;
		currprefs.genlock = changed_prefs.genlock;
		currprefs.genlock_mix = changed_prefs.genlock_mix;
		currprefs.genlock_alpha = changed_prefs.genlock_alpha;
		currprefs.genlock_aspect = changed_prefs.genlock_aspect;
		currprefs.genlock_scale = changed_prefs.genlock_scale;
		currprefs.genlock_offset_x = changed_prefs.genlock_offset_x;
		currprefs.genlock_offset_y = changed_prefs.genlock_offset_y;
		_tcscpy(currprefs.genlock_image_file, changed_prefs.genlock_image_file);
		_tcscpy(currprefs.genlock_video_file, changed_prefs.genlock_video_file);

		currprefs.gfx_lores_mode = changed_prefs.gfx_lores_mode;
		currprefs.gfx_overscanmode = changed_prefs.gfx_overscanmode;
		currprefs.gfx_scandoubler = changed_prefs.gfx_scandoubler;
		currprefs.gfx_threebitcolors = changed_prefs.gfx_threebitcolors;
		currprefs.gfx_grayscale = changed_prefs.gfx_grayscale;
		currprefs.gfx_ntscpixels = changed_prefs.gfx_ntscpixels;
		currprefs.gfx_monitorblankdelay = changed_prefs.gfx_monitorblankdelay;

		currprefs.gfx_display_sections = changed_prefs.gfx_display_sections;
		currprefs.gfx_variable_sync = changed_prefs.gfx_variable_sync;
		currprefs.gfx_windowed_resize = changed_prefs.gfx_windowed_resize;

		currprefs.gfx_apmode[APMODE_NATIVE].gfx_display = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_display;
		currprefs.gfx_apmode[APMODE_RTG].gfx_display = changed_prefs.gfx_apmode[APMODE_RTG].gfx_display;
		currprefs.gfx_blackerthanblack = changed_prefs.gfx_blackerthanblack;
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers;
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced;
		currprefs.gfx_apmode[APMODE_RTG].gfx_backbuffers = changed_prefs.gfx_apmode[APMODE_RTG].gfx_backbuffers;

		currprefs.main_alwaysontop = changed_prefs.main_alwaysontop;
		currprefs.gui_alwaysontop = changed_prefs.gui_alwaysontop;
		currprefs.borderless = changed_prefs.borderless;
		currprefs.blankmonitors = changed_prefs.blankmonitors;
		currprefs.rtgallowscaling = changed_prefs.rtgallowscaling;
		currprefs.rtgscaleaspectratio = changed_prefs.rtgscaleaspectratio;
		currprefs.rtgvblankrate = changed_prefs.rtgvblankrate;

#ifdef AMIBERRY
		// These are missing from WinUAE
		currprefs.rtg_hardwareinterrupt = changed_prefs.rtg_hardwareinterrupt;
		currprefs.rtg_hardwaresprite = changed_prefs.rtg_hardwaresprite;
		currprefs.rtg_multithread = changed_prefs.rtg_multithread;
		currprefs.rtg_zerocopy = changed_prefs.rtg_zerocopy;
#endif

		bool unacquired = false;
		for (int monid = MAX_AMIGAMONITORS - 1; monid >= 0; monid--) {
			if (!monitors[monid])
				continue;
			struct AmigaMonitor* mon = &AMonitors[monid];

			if (c & 64) {
				if (!unacquired) {
					inputdevice_unacquire();
					unacquired = true;
				}
			}
			if (c & 256) {
				init_colors(mon->monitor_id);
				reset_drawing();
			}
			if (c & 128) {
				if (currprefs.gfx_autoresolution) {
					c |= 2 | 8;
				} else {
					c |= 16;
					reset_drawing();
#ifdef AMIBERRY
					// Trigger auto-crop recalculations if needed
					force_auto_crop = true;
#endif
				}
			}
			if (c & 1024) {
				target_graphics_buffer_update(mon->monitor_id, true);
			}
			if (c & 512) {
				reopen_gfx(mon);
			}
			if ((c & 16) || ((c & 8) && keepfsmode)) {
				if (reopen(mon, c & 2, unacquired == false)) {
					c |= 2;
				} else {
					unacquired = true;
				}
			}
			if ((c & 32) || ((c & 2) && !keepfsmode)) {
				if (!unacquired) {
					inputdevice_unacquire();
					unacquired = true;
				}
				close_windows(mon);
				if (currprefs.gfx_api != changed_prefs.gfx_api || currprefs.gfx_api_options != changed_prefs.gfx_api_options) {
					currprefs.gfx_api = changed_prefs.gfx_api;
					currprefs.gfx_api_options = changed_prefs.gfx_api_options;
				}
				graphics_init(!dontcapture);
			}
		}
		init_custom();
		if (c & 4) {
			pause_sound();
			reset_sound();
			resume_sound();
		}

		if (setpause || dontcapture) {
			if (!unacquired)
				inputdevice_unacquire();
			unacquired = false;
		}

		if (unacquired)
			inputdevice_acquire(TRUE);

		if (setpause)
			setpaused(1);

		return 1;
	}

	bool changed = false;
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
		if (currprefs.cr[i].rate != changed_prefs.cr[i].rate ||
			currprefs.cr[i].locked != changed_prefs.cr[i].locked) {
			memcpy(&currprefs.cr[i], &changed_prefs.cr[i], sizeof(struct chipset_refresh));
			changed = true;
		}
	}
	if (changed) {
		init_hz();
	}
	if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate) {
		currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
		init_hz();
		return 1;
	}

	if (
		currprefs.gf[0].gfx_filter_autoscale != changed_prefs.gf[0].gfx_filter_autoscale ||
		currprefs.gf[2].gfx_filter_autoscale != changed_prefs.gf[2].gfx_filter_autoscale ||
		currprefs.gfx_xcenter_pos != changed_prefs.gfx_xcenter_pos ||
		currprefs.gfx_ycenter_pos != changed_prefs.gfx_ycenter_pos ||
		currprefs.gfx_xcenter_size != changed_prefs.gfx_xcenter_size ||
		currprefs.gfx_ycenter_size != changed_prefs.gfx_ycenter_size ||
		currprefs.gfx_xcenter != changed_prefs.gfx_xcenter ||
		currprefs.gfx_ycenter != changed_prefs.gfx_ycenter)
	{
		currprefs.gfx_xcenter_pos = changed_prefs.gfx_xcenter_pos;
		currprefs.gfx_ycenter_pos = changed_prefs.gfx_ycenter_pos;
		currprefs.gfx_xcenter_size = changed_prefs.gfx_xcenter_size;
		currprefs.gfx_ycenter_size = changed_prefs.gfx_ycenter_size;
		currprefs.gfx_xcenter = changed_prefs.gfx_xcenter;
		currprefs.gfx_ycenter = changed_prefs.gfx_ycenter;
		currprefs.gf[0].gfx_filter_autoscale = changed_prefs.gf[0].gfx_filter_autoscale;
		currprefs.gf[2].gfx_filter_autoscale = changed_prefs.gf[2].gfx_filter_autoscale;

		get_custom_limits(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		fixup_prefs_dimensions(&changed_prefs);

		return 1;
	}

	//currprefs.win32_norecyclebin = changed_prefs.win32_norecyclebin;
	currprefs.filesys_limit = changed_prefs.filesys_limit;
	currprefs.harddrive_read_only = changed_prefs.harddrive_read_only;

	//if (currprefs.win32_logfile != changed_prefs.win32_logfile) {
	//	currprefs.win32_logfile = changed_prefs.win32_logfile;
	//	if (currprefs.win32_logfile)
	//		logging_open (0, 1);
	//	else
	//		logging_cleanup ();
	//}

	if (currprefs.leds_on_screen != changed_prefs.leds_on_screen ||
		currprefs.keyboard_leds[0] != changed_prefs.keyboard_leds[0] ||
		currprefs.keyboard_leds[1] != changed_prefs.keyboard_leds[1] ||
		currprefs.keyboard_leds[2] != changed_prefs.keyboard_leds[2] ||
		currprefs.input_mouse_untrap != changed_prefs.input_mouse_untrap ||
		currprefs.input_magic_mouse_cursor != changed_prefs.input_magic_mouse_cursor ||
		currprefs.minimize_inactive != changed_prefs.minimize_inactive ||
		currprefs.active_capture_priority != changed_prefs.active_capture_priority ||
		currprefs.inactive_priority != changed_prefs.inactive_priority ||
		currprefs.minimized_priority != changed_prefs.minimized_priority ||
		currprefs.active_nocapture_nosound != changed_prefs.active_nocapture_nosound ||
		currprefs.active_nocapture_pause != changed_prefs.active_nocapture_pause ||
		currprefs.active_input != changed_prefs.active_input ||
		currprefs.inactive_nosound != changed_prefs.inactive_nosound ||
		currprefs.inactive_pause != changed_prefs.inactive_pause ||
		currprefs.inactive_input != changed_prefs.inactive_input ||
		currprefs.minimized_nosound != changed_prefs.minimized_nosound ||
		currprefs.minimized_pause != changed_prefs.minimized_pause ||
		currprefs.minimized_input != changed_prefs.minimized_input ||
		currprefs.capture_always != changed_prefs.capture_always ||
		currprefs.native_code != changed_prefs.native_code ||
		currprefs.alt_tab_release != changed_prefs.alt_tab_release ||
		currprefs.use_retroarch_quit != changed_prefs.use_retroarch_quit ||
		currprefs.use_retroarch_menu != changed_prefs.use_retroarch_menu ||
		currprefs.use_retroarch_reset != changed_prefs.use_retroarch_reset ||
		currprefs.use_retroarch_vkbd != changed_prefs.use_retroarch_vkbd ||
		currprefs.sound_pullmode != changed_prefs.sound_pullmode ||
		currprefs.kbd_led_num != changed_prefs.kbd_led_num ||
		currprefs.kbd_led_scr != changed_prefs.kbd_led_scr ||
		currprefs.kbd_led_cap != changed_prefs.kbd_led_cap ||
		currprefs.turbo_boot != changed_prefs.turbo_boot ||
		currprefs.right_control_is_right_win_key != changed_prefs.right_control_is_right_win_key)
	{
		currprefs.leds_on_screen = changed_prefs.leds_on_screen;
		currprefs.keyboard_leds[0] = changed_prefs.keyboard_leds[0];
		currprefs.keyboard_leds[1] = changed_prefs.keyboard_leds[1];
		currprefs.keyboard_leds[2] = changed_prefs.keyboard_leds[2];
		currprefs.input_mouse_untrap = changed_prefs.input_mouse_untrap;
		currprefs.input_magic_mouse_cursor = changed_prefs.input_magic_mouse_cursor;
		currprefs.minimize_inactive = changed_prefs.minimize_inactive;
		currprefs.active_capture_priority = changed_prefs.active_capture_priority;
		currprefs.inactive_priority = changed_prefs.inactive_priority;
		currprefs.minimized_priority = changed_prefs.minimized_priority;
		currprefs.active_nocapture_nosound = changed_prefs.active_nocapture_nosound;
		currprefs.active_input = changed_prefs.active_input;
		currprefs.active_nocapture_pause = changed_prefs.active_nocapture_pause;
		currprefs.inactive_nosound = changed_prefs.inactive_nosound;
		currprefs.inactive_pause = changed_prefs.inactive_pause;
		currprefs.inactive_input = changed_prefs.inactive_input;
		currprefs.minimized_nosound = changed_prefs.minimized_nosound;
		currprefs.minimized_pause = changed_prefs.minimized_pause;
		currprefs.minimized_input = changed_prefs.minimized_input;
		currprefs.capture_always = changed_prefs.capture_always;
		currprefs.native_code = changed_prefs.native_code;
		currprefs.alt_tab_release = changed_prefs.alt_tab_release;
		currprefs.use_retroarch_quit = changed_prefs.use_retroarch_quit;
		currprefs.use_retroarch_menu = changed_prefs.use_retroarch_menu;
		currprefs.use_retroarch_reset = changed_prefs.use_retroarch_reset;
		currprefs.use_retroarch_vkbd = changed_prefs.use_retroarch_vkbd;
		currprefs.sound_pullmode = changed_prefs.sound_pullmode;
		currprefs.kbd_led_num = changed_prefs.kbd_led_num;
		currprefs.kbd_led_scr = changed_prefs.kbd_led_scr;
		currprefs.kbd_led_cap = changed_prefs.kbd_led_cap;
		currprefs.turbo_boot = changed_prefs.turbo_boot;
		currprefs.right_control_is_right_win_key = changed_prefs.right_control_is_right_win_key;
		inputdevice_unacquire();
		currprefs.keyboard_leds_in_use = changed_prefs.keyboard_leds_in_use = (currprefs.keyboard_leds[0] | currprefs.keyboard_leds[1] | currprefs.keyboard_leds[2]) != 0;
		pause_sound();
		resume_sound();
		//refreshtitle();
		inputdevice_acquire(TRUE);
#ifndef	_DEBUG
		setpriority(currprefs.active_capture_priority);
#endif
		return 1;
	}

	if (currprefs.samplersoundcard != changed_prefs.samplersoundcard ||
		currprefs.sampler_stereo != changed_prefs.sampler_stereo) {
		currprefs.samplersoundcard = changed_prefs.samplersoundcard;
		currprefs.sampler_stereo = changed_prefs.sampler_stereo;
		sampler_free();
	}

	if (_tcscmp(currprefs.prtname, changed_prefs.prtname) ||
		currprefs.parallel_autoflush_time != changed_prefs.parallel_autoflush_time ||
		currprefs.parallel_matrix_emulation != changed_prefs.parallel_matrix_emulation ||
		currprefs.parallel_postscript_emulation != changed_prefs.parallel_postscript_emulation ||
		currprefs.parallel_postscript_detection != changed_prefs.parallel_postscript_detection ||
		_tcscmp(currprefs.ghostscript_parameters, changed_prefs.ghostscript_parameters)) {
		_tcscpy(currprefs.prtname, changed_prefs.prtname);
		currprefs.parallel_autoflush_time = changed_prefs.parallel_autoflush_time;
		currprefs.parallel_matrix_emulation = changed_prefs.parallel_matrix_emulation;
		currprefs.parallel_postscript_emulation = changed_prefs.parallel_postscript_emulation;
		currprefs.parallel_postscript_detection = changed_prefs.parallel_postscript_detection;
		_tcscpy(currprefs.ghostscript_parameters, changed_prefs.ghostscript_parameters);
#ifdef PARALLEL_PORT
		//closeprinter();
#endif
	}
	if (_tcscmp(currprefs.sername, changed_prefs.sername) ||
		currprefs.serial_hwctsrts != changed_prefs.serial_hwctsrts ||
		currprefs.serial_direct != changed_prefs.serial_direct ||
		currprefs.serial_demand != changed_prefs.serial_demand) {
		_tcscpy(currprefs.sername, changed_prefs.sername);
		currprefs.serial_hwctsrts = changed_prefs.serial_hwctsrts;
		currprefs.serial_demand = changed_prefs.serial_demand;
		currprefs.serial_direct = changed_prefs.serial_direct;
#ifdef SERIAL_PORT
		serial_exit();
		serial_init();
#endif
	}

	if (_tcscmp(currprefs.midiindev, changed_prefs.midiindev) ||
		_tcscmp(currprefs.midioutdev, changed_prefs.midioutdev) ||
		currprefs.midirouter != changed_prefs.midirouter)
	{
		_tcscpy(currprefs.midiindev, changed_prefs.midiindev);
		_tcscpy(currprefs.midioutdev, changed_prefs.midioutdev);
		currprefs.midirouter = changed_prefs.midirouter;

#ifdef SERIAL_PORT
		serial_exit();
		serial_init();
#ifdef WITH_MIDI
		Midi_Reopen();
#endif
#endif
#ifdef WITH_MIDIEMU
		midi_emu_reopen();
#endif
	}

#ifdef AMIBERRY
	// Virtual keyboard
	if (currprefs.vkbd_enabled != changed_prefs.vkbd_enabled ||
		currprefs.vkbd_hires != changed_prefs.vkbd_hires ||
		currprefs.vkbd_transparency != changed_prefs.vkbd_transparency ||
		currprefs.vkbd_exit != changed_prefs.vkbd_exit ||
		_tcscmp(currprefs.vkbd_language, changed_prefs.vkbd_language) ||
		_tcscmp(currprefs.vkbd_style, changed_prefs.vkbd_style) ||
		_tcscmp(currprefs.vkbd_toggle, changed_prefs.vkbd_toggle))
	{
		currprefs.vkbd_enabled = changed_prefs.vkbd_enabled;
		currprefs.vkbd_hires = changed_prefs.vkbd_hires;
		currprefs.vkbd_transparency = changed_prefs.vkbd_transparency;
		currprefs.vkbd_exit = changed_prefs.vkbd_exit;
		_tcscpy(currprefs.vkbd_language, changed_prefs.vkbd_language);
		_tcscpy(currprefs.vkbd_style, changed_prefs.vkbd_style);
		_tcscpy(currprefs.vkbd_toggle, changed_prefs.vkbd_toggle);

		if (currprefs.vkbd_enabled)
		{
			vkbd_set_transparency(static_cast<double>(currprefs.vkbd_transparency) / 100.0);
			vkbd_set_hires(currprefs.vkbd_hires);
			vkbd_set_keyboard_has_exit_button(currprefs.vkbd_exit);
			vkbd_set_language(string(currprefs.vkbd_language));
			vkbd_set_style(string(currprefs.vkbd_style));
			vkbd_key = get_hotkey_from_config(string(currprefs.vkbd_toggle));
			vkbd_button = SDL_GameControllerGetButtonFromString(currprefs.vkbd_toggle);
			if (vkbd_allowed(0))
				vkbd_init();
		}
		else
		{
			vkbd_key = {};
			vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;
			vkbd_quit();
		}
	}

	// On-screen joystick
	if (currprefs.onscreen_joystick != changed_prefs.onscreen_joystick)
	{
		currprefs.onscreen_joystick = changed_prefs.onscreen_joystick;
		if (currprefs.onscreen_joystick)
		{
			AmigaMonitor* mon = &AMonitors[0];
			on_screen_joystick_init(mon->amiga_renderer);
			int sw = 0, sh = 0;
			if (g_renderer)
				g_renderer->get_drawable_size(mon->amiga_window, &sw, &sh);
			on_screen_joystick_update_layout(sw, sh, render_quad);
			on_screen_joystick_set_enabled(true);
		}
		else
		{
			on_screen_joystick_set_enabled(false);
			on_screen_joystick_quit();
		}
	}
#endif

	if (changed_prefs.rtgboards[0].rtgmem_type != currprefs.rtgboards[0].rtgmem_type)
	{
		return 1;
	}

	return 0;
}

#endif // AMIBERRY
