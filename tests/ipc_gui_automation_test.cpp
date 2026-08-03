#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "amiberry_gui_geometry.h"

static int failures;

static void expect_true(const bool actual, const char* message)
{
	if (!actual) {
		std::cerr << message << '\n';
		failures++;
	}
}

static void expect_int_eq(const int actual, const int expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

static AmiberryGuiGeometrySnapshot make_snapshot(const int monitor_id = 0)
{
	AmiberryGuiGeometrySnapshot snapshot;
	snapshot.monitor_id = monitor_id;
	snapshot.display_id = 7;
	snapshot.display_mode = "native";
	snapshot.renderer = "opengl";
	snapshot.image_width = 800;
	snapshot.image_height = 600;
	snapshot.source = {40, 20, 720, 540};
	snapshot.viewport = {160, 90, 1440, 1080};
	snapshot.window_width = 1920;
	snapshot.window_height = 1080;
	snapshot.valid = true;
	return snapshot;
}

static void test_drawable_geometry_converts_to_logical_once()
{
	const AmiberryGfxRect logical = amiberry_gui_drawable_to_logical_rect(
		{200, 100, 1200, 800}, 1600, 1000, 800, 500);
	expect_int_eq(logical.x, 100, "HiDPI conversion must scale the left edge once");
	expect_int_eq(logical.y, 50, "HiDPI conversion must scale the top edge once");
	expect_int_eq(logical.w, 600, "HiDPI conversion must scale the width once");
	expect_int_eq(logical.h, 400, "HiDPI conversion must scale the height once");

	const AmiberryGfxRect unchanged = amiberry_gui_drawable_to_logical_rect(
		{10, 20, 300, 200}, 800, 500, 800, 500);
	expect_int_eq(unchanged.x, 10, "1x density must not move the viewport");
	expect_int_eq(unchanged.w, 300, "1x density must not rescale the viewport");
}

static void test_bounded_viewport_matches_renderer_clamping()
{
	const AmiberryGfxRect bounded = amiberry_gui_clamp_viewport(
		{-100, 20, 1000, 700}, 800, 600);
	expect_int_eq(bounded.x, 0, "negative viewport origins must be bounded");
	expect_int_eq(bounded.y, 20, "valid viewport origins must be preserved");
	expect_int_eq(bounded.w, 800, "viewport width must fit the drawable");
	expect_int_eq(bounded.h, 580, "viewport height must fit below its origin");
}

static void test_revision_is_stable_until_mapping_changes()
{
	AmiberryGuiGeometryState state("runtime-a");
	auto snapshot = make_snapshot();
	state.publish(snapshot);
	const auto first = state.snapshot();
	expect_true(first.valid, "published geometry must be readable");

	state.publish(snapshot);
	const auto stable = state.snapshot();
	expect_true(stable.geometry_revision == first.geometry_revision,
		"ordinary frame publication must not invalidate unchanged geometry");

	snapshot.viewport.x = 161;
	state.publish(snapshot);
	const auto moved = state.snapshot();
	expect_true(moved.geometry_revision > stable.geometry_revision,
		"viewport movement must increment geometry revision");

	snapshot.viewport.x = 160;
	state.publish(snapshot);
	expect_true(state.snapshot().geometry_revision > moved.geometry_revision,
		"away-and-back geometry must still create a new revision");

	const auto before_display_move = state.snapshot();
	snapshot.display_id = 8;
	state.publish(snapshot);
	expect_true(state.snapshot().geometry_revision
		> before_display_move.geometry_revision,
		"window display migration must increment geometry revision");
}

static void test_monitor_change_and_explicit_invalidation_advance_revision()
{
	AmiberryGuiGeometryState state("runtime-a");
	state.publish(make_snapshot(0));
	const auto initial = state.snapshot();

	state.set_active_monitor(1);
	const auto invalid = state.snapshot();
	expect_true(!invalid.valid, "active-monitor changes must invalidate old geometry");
	expect_true(invalid.geometry_revision > initial.geometry_revision,
		"active-monitor changes must increment revision");

	state.publish(make_snapshot(1));
	const auto republished = state.snapshot();
	expect_int_eq(republished.monitor_id, 1,
		"new geometry must be bound to the selected input monitor");
	const auto before_invalidate = republished.geometry_revision;
	state.invalidate();
	expect_true(state.snapshot().geometry_revision > before_invalidate,
		"explicit resize/scaling invalidation must increment revision");

	state.publish(make_snapshot(1));
	const auto before_inactive_change = state.snapshot();
	state.invalidate(0);
	expect_true(state.snapshot().geometry_revision
		== before_inactive_change.geometry_revision,
		"inactive-monitor changes must not invalidate active geometry");
}

static void test_runtime_identity_is_process_scoped()
{
	AmiberryGuiGeometryState first("runtime-a");
	AmiberryGuiGeometryState restarted("runtime-b");
	first.publish(make_snapshot());
	restarted.publish(make_snapshot());
	expect_true(first.snapshot().runtime_id != restarted.snapshot().runtime_id,
		"a restarted process must expose a different runtime identity");
}

static void test_snapshot_reads_never_mix_generations()
{
	AmiberryGuiGeometryState state("runtime-a");
	auto even = make_snapshot();
	auto odd = make_snapshot();
	odd.image_width = 801;
	odd.source.w = 721;
	odd.viewport.w = 1441;
	state.publish(even);

	std::atomic<bool> done{false};
	std::thread writer([&] {
		for (int i = 0; i < 10000; i++)
			state.publish((i & 1) ? odd : even);
		done = true;
	});

	while (!done) {
		const auto current = state.snapshot();
		const bool is_even = current.image_width == 800
			&& current.source.w == 720 && current.viewport.w == 1440;
		const bool is_odd = current.image_width == 801
			&& current.source.w == 721 && current.viewport.w == 1441;
		expect_true(is_even || is_odd,
			"a snapshot must contain fields from one complete generation");
	}
	writer.join();
}

static void test_actionable_response_has_fixed_fields()
{
	auto snapshot = make_snapshot(1);
	snapshot.runtime_id = "runtime-a";
	snapshot.capture_nonce = "capture-7";
	snapshot.geometry_revision = 42;
	const std::vector<std::string> fields =
		amiberry_gui_actionable_fields("/tmp/frame.png", snapshot);

	const std::vector<std::string> expected{
		"schema_version=1", "path=/tmp/frame.png", "runtime_id=runtime-a",
		"capture_nonce=capture-7", "geometry_revision=42", "monitor_id=1",
		"display_mode=native", "renderer=opengl", "image_width=800",
		"image_height=600", "source_x=40", "source_y=20", "source_width=720",
		"source_height=540", "viewport_x=160", "viewport_y=90",
		"viewport_width=1440", "viewport_height=1080", "window_width=1920",
		"window_height=1080"
	};
	expect_true(fields == expected,
		"actionable screenshot metadata must use the fixed v1 field set and order");
}

static void test_saved_png_dimensions_are_read_from_ihdr()
{
	const unsigned char png_header[24]{
		137, 80, 78, 71, 13, 10, 26, 10,
		0, 0, 0, 13, 'I', 'H', 'D', 'R',
		0, 0, 3, 32, 0, 0, 2, 88
	};
	int width = 0;
	int height = 0;
	expect_true(amiberry_gui_png_dimensions(
		png_header, sizeof png_header, width, height),
		"a valid PNG IHDR must expose saved dimensions");
	expect_int_eq(width, 800, "PNG width must use network byte order");
	expect_int_eq(height, 600, "PNG height must use network byte order");

	unsigned char invalid[24]{};
	expect_true(!amiberry_gui_png_dimensions(
		invalid, sizeof invalid, width, height),
		"invalid or truncated PNG metadata must reject actionable success");
}

int main()
{
	test_drawable_geometry_converts_to_logical_once();
	test_bounded_viewport_matches_renderer_clamping();
	test_revision_is_stable_until_mapping_changes();
	test_monitor_change_and_explicit_invalidation_advance_revision();
	test_runtime_identity_is_process_scoped();
	test_snapshot_reads_never_mix_generations();
	test_actionable_response_has_fixed_fields();
	test_saved_png_dimensions_are_read_from_ihdr();
	return failures == 0 ? 0 : 1;
}
