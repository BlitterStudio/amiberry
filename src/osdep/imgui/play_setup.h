#pragma once

#include <string>

#include "amiberry_gfx_mode.h"

enum class PlayScreenMode
{
	Windowed,
	FullWindow
};

// Scaling is stored as uae_prefs::scaling_method (-1=auto, 0=nearest, 1=linear,
// 2=integer, 3=stretch) so Play offers the same choices as the Display panel
// rather than a parallel list of its own.
struct PlayDisplayDefaults
{
	PlayScreenMode screen_mode = PlayScreenMode::Windowed;
	int scaling_method = -1;
	std::string shader = "none";
	bool auto_crop = false;
};

struct PlayDisplayPrefs
{
	int native_fullscreen = 0;
	int rtg_fullscreen = 0;
	int scaling_method = -1;
	int gfx_autoresolution = 0;
	std::string shader = "none";
	bool gfx_auto_crop = false;
};

PlayDisplayPrefs play_apply_display_defaults(const PlayDisplayDefaults& defaults);
void play_apply_display_defaults(const PlayDisplayDefaults& defaults, PlayDisplayPrefs& prefs);

// Keep the Play panel's cached screen mode in sync when the Display panel
// changes it, so resuming later does not re-apply a stale value.
void play_sync_screen_mode_cache(int fullscreen);

bool play_expert_settings_enabled();
void play_set_expert_settings_enabled(bool enabled);
bool play_setup_dismissed();
void play_set_setup_dismissed(bool dismissed);
