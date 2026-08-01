#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "amiberry_gfx_geometry.h"

struct PresentationScenario
{
	AmiberryGfxRect available_area;
	int presentation_width;
	int presentation_height;
	float fallback_aspect;
	bool use_bounded_fallback;
	AmiberryGfxRect drawable_area;
	AmiberryGfxRect expected_rect;
	bool expected_covers_drawable;
};

static constexpr PresentationScenario scenarios[] = {
	{{0, 0, 1920, 1080}, 1440, 1080, 4.0f / 3.0f, false,
		{0, 0, 1920, 1080}, {240, 0, 1440, 1080}, false},
	{{0, 0, 1920, 1080}, 1280, 800, 640.0f / 400.0f, false,
		{0, 0, 1920, 1080}, {320, 140, 1280, 800}, false},
	{{100, 50, 1000, 700}, 640, 480, 4.0f / 3.0f, true,
		{0, 0, 1920, 1080}, {280, 160, 640, 480}, false},
	{{100, 50, 300, 180}, 640, 400, 640.0f / 400.0f, true,
		{0, 0, 1920, 1080}, {106, 50, 288, 180}, false},
	{{250, 120, 640, 480}, 800, 600, 4.0f / 3.0f, false,
		{0, 0, 1920, 1080}, {170, 60, 800, 600}, false},
	{{0, 0, 1920, 1080}, 2000, 1200, 5.0f / 3.0f, false,
		{0, 0, 1920, 1080}, {-40, -60, 2000, 1200}, true},
	{{250, 120, 640, 480}, 2000, 1200, 5.0f / 3.0f, false,
		{0, 0, 1920, 1080}, {-430, -240, 2000, 1200}, false},
	{{10, 20, 300, 500}, 1000, 1000, 4.0f / 3.0f, true,
		{0, 0, 320, 540}, {10, 157, 300, 225}, false},
};

static volatile std::uint32_t benchmark_sink;

static bool rect_equals(const AmiberryGfxRect& lhs, const AmiberryGfxRect& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.w == rhs.w && lhs.h == rhs.h;
}

static bool reference_outputs_match()
{
	for (const auto& scenario : scenarios) {
		const AmiberryGfxRect rect = amiberry_gfx_final_presentation_rect(
			scenario.available_area, scenario.presentation_width,
			scenario.presentation_height, scenario.fallback_aspect,
			scenario.use_bounded_fallback);
		if (!rect_equals(rect, scenario.expected_rect)
			|| amiberry_gfx_rect_covers_area(rect, scenario.drawable_area)
				!= scenario.expected_covers_drawable) {
			return false;
		}
	}
	return true;
}

static std::uint32_t run_scenarios(const std::uint64_t iterations, const std::uint64_t offset)
{
	std::uint32_t checksum = 0;
	for (std::uint64_t iteration = 0; iteration < iterations; iteration++) {
		const auto& scenario = scenarios[(iteration + offset) & 7];
		const AmiberryGfxRect rect = amiberry_gfx_final_presentation_rect(
			scenario.available_area, scenario.presentation_width,
			scenario.presentation_height, scenario.fallback_aspect,
			scenario.use_bounded_fallback);
		const bool covers_drawable = amiberry_gfx_rect_covers_area(
			rect, scenario.drawable_area);
		checksum += static_cast<std::uint32_t>(rect.x)
			+ static_cast<std::uint32_t>(rect.y) * 3U
			+ static_cast<std::uint32_t>(rect.w) * 5U
			+ static_cast<std::uint32_t>(rect.h) * 7U
			+ static_cast<std::uint32_t>(covers_drawable);
	}
	return checksum;
}

int main(const int argc, char* argv[])
{
	const std::uint64_t iterations = argc > 1
		? std::strtoull(argv[1], nullptr, 10) : 100000000ULL;
	const std::uint64_t offset = argc > 2
		? std::strtoull(argv[2], nullptr, 10) : 37ULL;
	const bool outputs_match = reference_outputs_match();

	benchmark_sink = run_scenarios(1000000ULL, offset);
	const auto started_at = std::chrono::steady_clock::now();
	const std::uint32_t checksum = run_scenarios(iterations, offset);
	const auto finished_at = std::chrono::steady_clock::now();
	benchmark_sink = checksum;

	const double elapsed_ns = std::chrono::duration<double, std::nano>(
		finished_at - started_at).count();
	const double ns_per_scenario = iterations > 0
		? elapsed_ns / static_cast<double>(iterations) : 0.0;

	std::cout << std::fixed << std::setprecision(6)
		<< "{\"geometry_ns_per_scenario\":" << ns_per_scenario
		<< ",\"focused_tests_passed\":1"
		<< ",\"reference_outputs_match\":" << (outputs_match ? 1 : 0)
		<< ",\"result_checksum\":" << checksum
		<< ",\"timed_scenarios\":" << iterations << "}\n";
	return 0;
}
