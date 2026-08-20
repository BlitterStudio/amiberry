#pragma once

#include <algorithm>
#include <cmath>

// Pure computation for the user-adjustable GUI-scale preference, shared by
// DPIHandler::get_layout_scale() (the single choke point every GUI font,
// window-size, widget and drag-threshold consumer reads) and the "GUI scale"
// settings row. Header-only and free of SDL/ImGui includes so it compiles and
// unit-tests standalone (tests/gui_layout_scale_test.cpp).
//
// The preference is expressed as a percentage of the automatic, DPI-aware
// layout scale: 100 is stock behavior, Android defaults to 70 for large
// Chromebook panels. The minimum doubles as the lockout guarantee — even the
// smallest permitted value keeps the settings UI readable enough to raise the
// scale again.

inline constexpr float gui_layout_scale_min_percent = 50.0f;
inline constexpr float gui_layout_scale_max_percent = 200.0f;

// Clamp a user-entered percentage into the allowed range. Non-finite input
// (NaN or infinity, e.g. a hand-edited "nan" in amiberry.conf) falls back to
// the stock 100% default: std::clamp alone passes NaN through (every
// comparison involving NaN is false), which would poison every GUI metric
// downstream of get_layout_scale().
inline float clamp_gui_layout_scale_percent(const float percent)
{
	if (!std::isfinite(percent))
		return 100.0f;
	return std::clamp(percent, gui_layout_scale_min_percent, gui_layout_scale_max_percent);
}

// Effective layout scale: the stock DPI-aware scale multiplied by the clamped
// user preference. The percent is clamped first, then divided by 100, then
// multiplied into the stock scale — this ordering makes 100 an exact
// pass-through (100.0f / 100.0f == 1.0f exactly, and x * 1.0f == x for every
// float x), so the non-Android default is bit-identical to stock behavior.
inline float effective_layout_scale(const float stock_scale, const float percent)
{
	return stock_scale * (clamp_gui_layout_scale_percent(percent) / 100.0f);
}
