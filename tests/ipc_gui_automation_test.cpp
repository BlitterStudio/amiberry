#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "amiberry_gui_automation.h"
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

static void test_guarded_input_is_atomic_and_rejects_stale_state()
{
	AmiberryGuiGeometryState state("runtime-a");
	state.publish(make_snapshot(1));
	const auto current = state.snapshot();
	AmiberryGuiGuardedInputRequest request{
		100, 200, 5, "runtime-a", current.geometry_revision, 1, 7
	};
	AmiberryGuiGuardedInputEnvironment environment{true, true, 1, 7};
	int mutations = 0;
	int applied_mask = -1;
	auto apply = [&](const int monitor_id, const int x, const int y,
		const int button_mask) {
		expect_int_eq(monitor_id, 1, "guard must retain the expected monitor");
		expect_int_eq(x, 100, "guard must retain x");
		expect_int_eq(y, 200, "guard must retain y");
		applied_mask = button_mask;
		++mutations;
		return true;
	};

	const auto applied = amiberry_gui_guarded_input(state, request, environment, apply);
	expect_true(applied.applied, "matching guarded input must apply");
	expect_int_eq(applied_mask, 5, "guard must assign the complete button mask");

	auto expect_rejection = [&](const AmiberryGuiGuardedInputRequest& rejected,
		const AmiberryGuiGuardRejectReason reason) {
		const int before = mutations;
		const auto result = amiberry_gui_guarded_input(
			state, rejected, environment, apply);
		expect_true(!result.applied, "mismatched guarded input must reject");
		expect_true(result.reason == reason, "guard must return a stable reason");
		expect_int_eq(mutations, before, "rejection must not mutate input");
	};

	auto rejected = request;
	rejected.runtime_id = "runtime-b";
	expect_rejection(rejected, AmiberryGuiGuardRejectReason::runtime_mismatch);
	rejected = request;
	++rejected.geometry_revision;
	expect_rejection(rejected, AmiberryGuiGuardRejectReason::geometry_revision_mismatch);
	rejected = request;
	rejected.monitor_id = 0;
	expect_rejection(rejected, AmiberryGuiGuardRejectReason::monitor_mismatch);
	auto wrong_active_monitor = environment;
	wrong_active_monitor.active_monitor_id = 0;
	expect_true(amiberry_gui_guarded_input(
		state, request, wrong_active_monitor, apply).reason
		== AmiberryGuiGuardRejectReason::monitor_mismatch,
		"an active-monitor change must reject the captured monitor");
	rejected = request;
	rejected.input_config_revision = 8;
	expect_rejection(rejected, AmiberryGuiGuardRejectReason::input_config_revision_mismatch);
	rejected = request;
	rejected.x = current.window_width;
	expect_rejection(rejected, AmiberryGuiGuardRejectReason::coordinate_out_of_bounds);
	rejected = request;
	rejected.button_mask = 8;
	expect_rejection(rejected, AmiberryGuiGuardRejectReason::unsupported_button_mask);

	auto unready = environment;
	unready.focus_ready = false;
	expect_true(amiberry_gui_guarded_input(state, request, unready, apply).reason
		== AmiberryGuiGuardRejectReason::focus_not_ready,
		"lost focus must reject before mutation");
	unready = environment;
	unready.settings_compatible = false;
	expect_true(amiberry_gui_guarded_input(state, request, unready, apply).reason
		== AmiberryGuiGuardRejectReason::settings_incompatible,
		"incompatible effective settings must reject before mutation");

	for (int mask = 0; mask <= AMIBERRY_GUI_SUPPORTED_BUTTON_MASK; ++mask) {
		request.button_mask = mask;
		expect_true(amiberry_gui_guarded_input(state, request, environment, apply).applied,
			"every supported complete button mask must apply");
		expect_int_eq(applied_mask, mask, "complete mask assignment must include releases");
	}

	request.button_mask = 1;
	expect_true(amiberry_gui_guarded_input(state, request, environment, apply).applied,
		"guarded press must apply before geometry becomes stale");
	auto changed = make_snapshot(1);
	changed.viewport.x++;
	state.publish(changed);
	request.button_mask = 0;
	expect_true(!amiberry_gui_guarded_input(state, request, environment, apply).applied,
		"stale guarded release must reject without changing the pressed mask");
	expect_int_eq(applied_mask, 1, "stale guarded release must leave input unchanged");
	amiberry_gui_release_mouse_buttons([&](const int button) {
		applied_mask &= ~(1 << button);
	});
	expect_int_eq(applied_mask, 0,
		"coordinate-free cleanup must release a press after geometry changes");
}

static void test_release_is_unconditional_and_idempotent()
{
	int buttons = 7;
	auto release = [&](const int button) { buttons &= ~(1 << button); };
	expect_int_eq(amiberry_gui_release_mouse_buttons(release), 0,
		"release must report an effective zero mask");
	expect_int_eq(buttons, 0, "release must clear left, right, and middle");
	expect_int_eq(amiberry_gui_release_mouse_buttons(release), 0,
		"repeated release must remain successful");
	expect_int_eq(buttons, 0, "repeated release must remain neutral");
}

static void test_input_config_revision_and_compare_exchange()
{
	AmiberryGuiInputConfigState state;
	auto snapshot = state.observe(0, 0, 0, 0);
	expect_true(snapshot.input_config_revision > 0,
		"first input snapshot must have a usable revision");
	const auto original_revision = snapshot.input_config_revision;

	snapshot = state.observe(1, 2, 0, 0);
	expect_true(snapshot.input_config_revision > original_revision,
		"pending input changes must advance the revision");
	expect_true(snapshot.pending_tablet_mode != snapshot.effective_tablet_mode,
		"pending/effective divergence must remain visible");

	int pending_tablet = 1;
	int pending_untrap = 2;
	auto mutate = [&](const int tablet, const int untrap) {
		pending_tablet = tablet;
		pending_untrap = untrap;
	};
	const AmiberryGuiPendingInputConfig expected{1, 2};
	const AmiberryGuiInputConfigValues current{{1, 2}, {0, 0}};
	const AmiberryGuiPendingInputConfig desired{2, 3};
	const auto applied = state.compare_exchange(
		snapshot.input_config_revision, expected, current, desired, mutate);
	expect_true(applied.applied, "matching compare-and-set must apply");
	expect_int_eq(pending_tablet, 2, "CAS must set the requested tablet mode");
	expect_int_eq(pending_untrap, 3, "CAS must set the requested untrap mode");

	const auto conflict = state.compare_exchange(snapshot.input_config_revision,
		expected, {{2, 3}, {0, 0}}, {0, 0}, mutate);
	expect_true(!conflict.applied, "stale compare-and-set must reject");
	expect_true(conflict.reason == AmiberryGuiConfigRejectReason::input_config_conflict,
		"CAS conflict must have a stable reason");
	expect_int_eq(pending_tablet, 2, "conflicting restore must preserve external state");

	const auto current_revision = applied.snapshot.input_config_revision;
	const auto external_change = state.compare_exchange(current_revision,
		{2, 3}, {{0, 3}, {0, 0}}, {1, 1}, mutate);
	expect_true(!external_change.applied,
		"live pending values must participate in the ownership check");
}

static void test_checked_protocol_integer_parsing()
{
	int signed_value = 0;
	std::uint64_t unsigned_value = 0;
	expect_true(amiberry_gui_parse_int("-42", signed_value) && signed_value == -42,
		"checked signed parser must accept canonical integers");
	expect_true(!amiberry_gui_parse_int("2147483648", signed_value),
		"signed overflow must reject");
	expect_true(!amiberry_gui_parse_int("12x", signed_value),
		"malformed signed fields must reject");
	expect_true(amiberry_gui_parse_u64("18446744073709551615", unsigned_value),
		"checked revision parser must accept uint64 max");
	expect_true(!amiberry_gui_parse_u64("18446744073709551616", unsigned_value),
		"revision overflow must reject");
	expect_true(!amiberry_gui_button_mask_supported(8),
		"unsupported button bits must reject");
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
	test_guarded_input_is_atomic_and_rejects_stale_state();
	test_release_is_unconditional_and_idempotent();
	test_input_config_revision_and_compare_exchange();
	test_checked_protocol_integer_parsing();
	return failures == 0 ? 0 : 1;
}
