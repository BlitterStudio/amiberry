#pragma once

#include "amiberry_gfx_mode.h"

enum class PlayScreenMode
{
	Windowed,
	FullWindow
};

enum class PlayScalingMode
{
	Auto,
	Integer,
	Smooth
};

enum class PlayShaderChoice
{
	None,
	Crt,
	Monitor1084,
	Custom
};

struct PlayDisplayDefaults
{
	PlayScreenMode screen_mode = PlayScreenMode::Windowed;
	PlayScalingMode scaling = PlayScalingMode::Auto;
	PlayShaderChoice shader = PlayShaderChoice::None;
	bool auto_crop = false;
};

struct PlayDisplayPrefs
{
	int native_fullscreen = 0;
	int rtg_fullscreen = 0;
	int scaling_method = -1;
	int gfx_autoresolution = 0;
	int shader_choice = static_cast<int>(PlayShaderChoice::None);
	bool preserve_shader = false;
	bool gfx_auto_crop = false;
};

PlayDisplayPrefs play_apply_display_defaults(const PlayDisplayDefaults& defaults);
void play_apply_display_defaults(const PlayDisplayDefaults& defaults, PlayDisplayPrefs& prefs);

bool play_expert_settings_enabled();
void play_set_expert_settings_enabled(bool enabled);
bool play_setup_dismissed();
void play_set_setup_dismissed(bool dismissed);
