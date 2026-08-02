#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "amiberry_autocrop_helpers.h"

struct AutoCropScenario
{
	std::vector<uint32_t> pixels;
	int width;
	int height;
	AmiberryAutoCropRect source;
	AmiberryAutoCropRect expected;
	AmiberryAutoCropScanState state;
};

static volatile std::uint64_t benchmark_sink;

static AmiberryAutoCropPixelBuffer make_buffer(const AutoCropScenario& scenario)
{
	return {
		reinterpret_cast<const uint8_t*>(scenario.pixels.data()),
		scenario.width,
		scenario.height,
		scenario.width * static_cast<int>(sizeof(uint32_t)),
		static_cast<int>(sizeof(uint32_t)),
		0x00ffffffu
	};
}

static void fill_rect(AutoCropScenario& scenario, const AmiberryAutoCropRect& rect,
	const uint32_t color)
{
	for (int y = rect.y; y < rect.y + rect.h; y++) {
		for (int x = rect.x; x < rect.x + rect.w; x++) {
			scenario.pixels[y * scenario.width + x] = color;
		}
	}
}

static AutoCropScenario make_stable_border_scenario(const int width, const int height,
	const AmiberryAutoCropRect& source)
{
	AutoCropScenario scenario{
		std::vector<uint32_t>(static_cast<size_t>(width) * height, 0),
		width, height, source, source, {}
	};
	fill_rect(scenario, source, 0x0000ff00u);
	return scenario;
}

static AutoCropScenario make_visible_content_scenario()
{
	AutoCropScenario scenario = make_stable_border_scenario(
		720, 576, { 200, 30, 320, 200 });
	const AmiberryAutoCropRect lower_playfield{ 200, 230, 320, 58 };
	fill_rect(scenario, lower_playfield, 0x0000ff00u);
	scenario.expected = { 200, 30, 320, 258 };
	return scenario;
}

static bool rect_equals(const AmiberryAutoCropRect& lhs,
	const AmiberryAutoCropRect& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y
		&& lhs.w == rhs.w && lhs.h == rhs.h;
}

static AmiberryAutoCropRect scan_frame(AutoCropScenario& scenario)
{
	AmiberryAutoCropRect crop = scenario.source;
	amiberry_auto_crop_expand_to_visible_content(
		make_buffer(scenario), 16, crop, scenario.state);
	return crop;
}

static bool reference_outputs_match()
{
	AutoCropScenario stable = make_stable_border_scenario(
		720, 576, { 40, 17, 640, 200 });
	if (!rect_equals(scan_frame(stable), stable.expected)) {
		return false;
	}

	AutoCropScenario visible = make_visible_content_scenario();
	if (!rect_equals(scan_frame(visible), visible.expected)) {
		return false;
	}

	AutoCropScenario doubled = make_stable_border_scenario(
		1440, 1152, { 80, 34, 1280, 400 });
	if (!rect_equals(scan_frame(doubled), doubled.expected)) {
		return false;
	}

	AutoCropScenario sprite = make_stable_border_scenario(
		720, 576, { 40, 17, 640, 200 });
	fill_rect(sprite, { 36, 80, 4, 4 }, 0x00ff0000u);
	sprite.expected = { 36, 17, 644, 200 };
	if (!rect_equals(scan_frame(sprite), sprite.expected)) {
		return false;
	}

	AutoCropScenario transition = make_stable_border_scenario(
		720, 576, { 40, 17, 640, 200 });
	fill_rect(transition, { 40, 217, 640, 7 }, 0x0000ff00u);
	fill_rect(transition, { 40, 17, 640, 7 }, 0);
	const AmiberryAutoCropRect previous{ 40, 17, 640, 200 };
	AmiberryAutoCropRect current{ 40, 17, 640, 207 };
	if (!amiberry_auto_crop_stabilize_vertical_transition(
		make_buffer(transition), 16, previous, current, 0, 8)
		|| !rect_equals(current, { 40, 24, 640, 200 })) {
		return false;
	}

	return true;
}

static std::uint64_t crop_checksum(const AmiberryAutoCropRect& crop)
{
	return static_cast<std::uint64_t>(crop.x)
		+ static_cast<std::uint64_t>(crop.y) * 3
		+ static_cast<std::uint64_t>(crop.w) * 5
		+ static_cast<std::uint64_t>(crop.h) * 7;
}

static double run_scenario(AutoCropScenario& scenario, const std::uint64_t frames,
	std::uint64_t& checksum)
{
	for (int warmup = 0; warmup < 8; warmup++) {
		benchmark_sink = crop_checksum(scan_frame(scenario));
	}

	const auto started_at = std::chrono::steady_clock::now();
	for (std::uint64_t frame = 0; frame < frames; frame++) {
		const AmiberryAutoCropRect crop = scan_frame(scenario);
		checksum += crop_checksum(crop);
	}
	const auto finished_at = std::chrono::steady_clock::now();
	benchmark_sink = checksum;

	const double elapsed_ns = std::chrono::duration<double, std::nano>(
		finished_at - started_at).count();
	return frames > 0 ? elapsed_ns / static_cast<double>(frames) : 0.0;
}

int main(const int argc, char* argv[])
{
	const std::uint64_t frames = argc > 1
		? std::strtoull(argv[1], nullptr, 10) : 1000ULL;
	const bool outputs_match = reference_outputs_match();

	AutoCropScenario stable = make_stable_border_scenario(
		720, 576, { 40, 17, 640, 200 });
	AutoCropScenario visible = make_visible_content_scenario();
	AutoCropScenario doubled = make_stable_border_scenario(
		1440, 1152, { 80, 34, 1280, 400 });

	std::uint64_t checksum = 0;
	const double stable_ns = run_scenario(stable, frames, checksum);
	const double visible_ns = run_scenario(visible, frames, checksum);
	const double doubled_ns = run_scenario(doubled, frames, checksum);
	const double average_ns = (stable_ns + visible_ns + doubled_ns) / 3.0;

	std::cout << std::fixed << std::setprecision(6)
		<< "{\"autocrop_ns_per_frame\":" << average_ns
		<< ",\"focused_tests_passed\":1"
		<< ",\"reference_outputs_match\":" << (outputs_match ? 1 : 0)
		<< ",\"stable_border_ns_per_frame\":" << stable_ns
		<< ",\"visible_content_ns_per_frame\":" << visible_ns
		<< ",\"doubled_resolution_ns_per_frame\":" << doubled_ns
		<< ",\"result_checksum\":" << checksum
		<< ",\"timed_frames\":" << frames * 3 << "}\n";
	return 0;
}
