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
}

static void test_smooth_scaling_defaults()
{
	const PlayDisplayDefaults defaults{
		PlayScreenMode::Fullscreen,
		PlayScalingMode::Smooth,
		PlayShaderChoice::Monitor1084
	};

	const auto prefs = play_apply_display_defaults(defaults);

	expect_eq(prefs.scaling_method, 1, "Smooth scaling must set scaling method to 1");
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
	test_smooth_scaling_defaults();
	test_no_dependency_state_helpers_stay_false();

	return failures == 0 ? 0 : 1;
}
