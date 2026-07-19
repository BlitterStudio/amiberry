#include <iostream>

#include "imgui/play_setup.h"

static int failures;

template<typename T>
static void expect_eq(T actual, T expected, const char *message)
{
	if (actual != expected) {
		std::cerr << message << '\n';
		failures++;
	}
}

static void test_fullwindow_integer_crt_defaults()
{
	const PlayDisplayDefaults defaults{
		PlayScreenMode::FullWindow,
		PlayScalingMode::Integer,
		PlayShaderChoice::Crt
	};

	const auto prefs = play_apply_display_defaults(defaults);

	expect_eq(prefs.native_fullscreen, 2, "FullWindow must set native fullscreen to 2");
	expect_eq(prefs.rtg_fullscreen, 2, "FullWindow must set RTG fullscreen to 2");
	expect_eq(prefs.scaling_method, 2, "Integer scaling must set scaling method to 2");
	expect_eq(prefs.gfx_autoresolution, 1, "Integer scaling must enable autoresolution");
	expect_eq(prefs.shader_choice, static_cast<int>(PlayShaderChoice::Crt),
		"CRT shader choice must be preserved");
	expect_eq(prefs.gfx_auto_crop, false, "AutoCrop must default to disabled");
}

static void test_windowed_auto_none_defaults()
{
	const PlayDisplayDefaults defaults{
		PlayScreenMode::Windowed,
		PlayScalingMode::Auto,
		PlayShaderChoice::None
	};

	const auto prefs = play_apply_display_defaults(defaults);

	expect_eq(prefs.native_fullscreen, 0, "Windowed must set native fullscreen to 0");
	expect_eq(prefs.rtg_fullscreen, 0, "Windowed must set RTG fullscreen to 0");
	expect_eq(prefs.scaling_method, -1, "Auto scaling must set scaling method to -1");
	expect_eq(prefs.gfx_autoresolution, 0, "Auto scaling must disable autoresolution");
	expect_eq(prefs.shader_choice, static_cast<int>(PlayShaderChoice::None),
		"No shader choice must be preserved");
	expect_eq(prefs.gfx_auto_crop, false, "Auto scaling must not enable AutoCrop");
}

static void test_fullwindow_smooth_scaling_defaults()
{
	const PlayDisplayDefaults defaults{
		PlayScreenMode::FullWindow,
		PlayScalingMode::Smooth,
		PlayShaderChoice::Monitor1084
	};

	const auto prefs = play_apply_display_defaults(defaults);

	expect_eq(prefs.native_fullscreen, 2, "Full-window must set native fullscreen to 2");
	expect_eq(prefs.rtg_fullscreen, 2, "Full-window must set RTG fullscreen to 2");
	expect_eq(prefs.scaling_method, 1, "Smooth scaling must set scaling method to 1");
	expect_eq(prefs.gfx_autoresolution, 0, "Smooth scaling must disable autoresolution");
	expect_eq(prefs.shader_choice, static_cast<int>(PlayShaderChoice::Monitor1084),
		"1084 shader choice must be preserved");
	expect_eq(prefs.gfx_auto_crop, false, "Smooth scaling must not enable AutoCrop");
}

static void test_autocrop_default_is_preserved()
{
	PlayDisplayDefaults defaults{
		PlayScreenMode::FullWindow,
		PlayScalingMode::Integer,
		PlayShaderChoice::None
	};
	defaults.auto_crop = true;

	const auto prefs = play_apply_display_defaults(defaults);

	expect_eq(prefs.gfx_auto_crop, true, "AutoCrop default must be preserved");
}

static void test_custom_shader_is_preserved()
{
	const PlayDisplayDefaults defaults{
		PlayScreenMode::Windowed,
		PlayScalingMode::Auto,
		PlayShaderChoice::Custom
	};

	const auto prefs = play_apply_display_defaults(defaults);

	expect_eq(prefs.preserve_shader, true, "Custom shader choices must preserve current shader names");
	expect_eq(prefs.shader_choice, -1, "Custom shader choices must not map to a preset shader");
}

static void test_no_dependency_state_helpers_stay_false()
{
	play_set_expert_settings_enabled(true);
	play_set_setup_dismissed(true);

	expect_eq(play_expert_settings_enabled(), false,
		"No-dependency expert settings helper must stay false");
	expect_eq(play_setup_dismissed(), false,
		"No-dependency setup dismissed helper must stay false");
}

int main()
{
	test_fullwindow_integer_crt_defaults();
	test_windowed_auto_none_defaults();
	test_fullwindow_smooth_scaling_defaults();
	test_autocrop_default_is_preserved();
	test_custom_shader_is_preserved();
	test_no_dependency_state_helpers_stay_false();

	return failures == 0 ? 0 : 1;
}
