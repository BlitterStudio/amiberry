#include "play_setup.h"

#ifndef PLAY_SETUP_NO_AMIBERRY_DEPS
#include "sysdeps.h"
#include "registry.h"
#endif

static int screen_mode_to_fullscreen(const PlayScreenMode mode)
{
	switch (mode) {
	case PlayScreenMode::Windowed:
		return GFX_WINDOW;
	case PlayScreenMode::FullWindow:
		return GFX_FULLWINDOW;
	}

	return GFX_WINDOW;
}

static int shader_choice_to_prefs(const PlayShaderChoice choice)
{
	switch (choice) {
	case PlayShaderChoice::None:
		return 0;
	case PlayShaderChoice::Crt:
		return 1;
	case PlayShaderChoice::Monitor1084:
		return 2;
	case PlayShaderChoice::Custom:
		return -1;
	}

	return 0;
}

void play_apply_display_defaults(const PlayDisplayDefaults& defaults, PlayDisplayPrefs& prefs)
{
	const auto fullscreen = screen_mode_to_fullscreen(defaults.screen_mode);
	prefs.native_fullscreen = fullscreen;
	prefs.rtg_fullscreen = fullscreen;

	switch (defaults.scaling) {
	case PlayScalingMode::Auto:
		prefs.scaling_method = -1;
		prefs.gfx_autoresolution = 0;
		break;
	case PlayScalingMode::Smooth:
		prefs.scaling_method = 1;
		prefs.gfx_autoresolution = 0;
		break;
	case PlayScalingMode::Integer:
		prefs.scaling_method = 2;
		prefs.gfx_autoresolution = 1;
		break;
	}

	prefs.shader_choice = shader_choice_to_prefs(defaults.shader);
	prefs.preserve_shader = defaults.shader == PlayShaderChoice::Custom;
	prefs.gfx_auto_crop = defaults.auto_crop;
}

PlayDisplayPrefs play_apply_display_defaults(const PlayDisplayDefaults& defaults)
{
	PlayDisplayPrefs prefs;
	play_apply_display_defaults(defaults, prefs);
	return prefs;
}

#ifdef PLAY_SETUP_NO_AMIBERRY_DEPS

bool play_expert_settings_enabled()
{
	return false;
}

void play_set_expert_settings_enabled(bool)
{
}

bool play_setup_dismissed()
{
	return false;
}

void play_set_setup_dismissed(bool)
{
}

#else

static bool query_bool_setting(const TCHAR* name)
{
	int value = 0;
	regqueryint(nullptr, name, &value);
	return value != 0;
}

static void set_bool_setting(const TCHAR* name, const bool enabled)
{
	regsetint(nullptr, name, enabled ? 1 : 0);
}

bool play_expert_settings_enabled()
{
	return query_bool_setting(_T("PlayExpertSettings"));
}

void play_set_expert_settings_enabled(const bool enabled)
{
	set_bool_setting(_T("PlayExpertSettings"), enabled);
}

bool play_setup_dismissed()
{
	return query_bool_setting(_T("PlaySetupDismissed"));
}

void play_set_setup_dismissed(const bool dismissed)
{
	set_bool_setting(_T("PlaySetupDismissed"), dismissed);
}

#endif
