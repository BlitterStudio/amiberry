#ifndef AMIBERRY_GUI_AUTOMATION_H
#define AMIBERRY_GUI_AUTOMATION_H

#include <charconv>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <system_error>

#include "amiberry_gui_geometry.h"

constexpr int AMIBERRY_GUI_SUPPORTED_BUTTON_MASK = 7;

using AmiberryGuiInputMutation =
	std::function<bool(int monitor_id, int x, int y, int button_mask)>;

static inline bool amiberry_gui_parse_int(const std::string& value, int& result)
{
	if (value.empty())
		return false;
	const char* begin = value.data();
	const char* end = begin + value.size();
	const auto parsed = std::from_chars(begin, end, result, 10);
	return parsed.ec == std::errc{} && parsed.ptr == end;
}

static inline bool amiberry_gui_parse_u64(
	const std::string& value, std::uint64_t& result)
{
	if (value.empty())
		return false;
	const char* begin = value.data();
	const char* end = begin + value.size();
	const auto parsed = std::from_chars(begin, end, result, 10);
	return parsed.ec == std::errc{} && parsed.ptr == end;
}

static inline bool amiberry_gui_button_mask_supported(const int button_mask)
{
	return button_mask >= 0
		&& (button_mask & ~AMIBERRY_GUI_SUPPORTED_BUTTON_MASK) == 0;
}

enum class AmiberryGuiGuardRejectReason
{
	none,
	geometry_invalid,
	runtime_mismatch,
	geometry_revision_mismatch,
	monitor_mismatch,
	input_config_revision_mismatch,
	focus_not_ready,
	settings_incompatible,
	coordinate_out_of_bounds,
	unsupported_button_mask,
	input_rejected
};

static inline const char* amiberry_gui_guard_reason_name(
	const AmiberryGuiGuardRejectReason reason)
{
	switch (reason) {
	case AmiberryGuiGuardRejectReason::none: return "none";
	case AmiberryGuiGuardRejectReason::geometry_invalid: return "geometry_invalid";
	case AmiberryGuiGuardRejectReason::runtime_mismatch: return "runtime_mismatch";
	case AmiberryGuiGuardRejectReason::geometry_revision_mismatch:
		return "geometry_revision_mismatch";
	case AmiberryGuiGuardRejectReason::monitor_mismatch: return "monitor_mismatch";
	case AmiberryGuiGuardRejectReason::input_config_revision_mismatch:
		return "input_config_revision_mismatch";
	case AmiberryGuiGuardRejectReason::focus_not_ready: return "focus_not_ready";
	case AmiberryGuiGuardRejectReason::settings_incompatible:
		return "settings_incompatible";
	case AmiberryGuiGuardRejectReason::coordinate_out_of_bounds:
		return "coordinate_out_of_bounds";
	case AmiberryGuiGuardRejectReason::unsupported_button_mask:
		return "unsupported_button_mask";
	case AmiberryGuiGuardRejectReason::input_rejected: return "input_rejected";
	}
	return "input_rejected";
}

struct AmiberryGuiGuardedInputRequest
{
	int x = 0;
	int y = 0;
	int button_mask = 0;
	std::string runtime_id;
	std::uint64_t geometry_revision = 0;
	int monitor_id = 0;
	std::uint64_t input_config_revision = 0;
};

struct AmiberryGuiGuardedInputEnvironment
{
	bool focus_ready = false;
	bool settings_compatible = false;
	int active_monitor_id = -1;
	std::uint64_t input_config_revision = 0;
};

struct AmiberryGuiGuardedInputResult
{
	bool applied = false;
	AmiberryGuiGuardRejectReason reason = AmiberryGuiGuardRejectReason::none;
	AmiberryGuiGeometrySnapshot geometry;
	int button_mask = 0;
};

template<typename Mutation>
static AmiberryGuiGuardedInputResult amiberry_gui_guarded_input(
	AmiberryGuiGeometryState& state,
	const AmiberryGuiGuardedInputRequest& request,
	const AmiberryGuiGuardedInputEnvironment& environment,
	Mutation&& mutation)
{
	return state.with_synchronized_snapshot(
		[&](const AmiberryGuiGeometrySnapshot& geometry) {
			AmiberryGuiGuardedInputResult result;
			result.geometry = geometry;
			if (!geometry.valid)
				result.reason = AmiberryGuiGuardRejectReason::geometry_invalid;
			else if (geometry.runtime_id != request.runtime_id)
				result.reason = AmiberryGuiGuardRejectReason::runtime_mismatch;
			else if (geometry.geometry_revision != request.geometry_revision)
				result.reason = AmiberryGuiGuardRejectReason::geometry_revision_mismatch;
			else if (geometry.monitor_id != request.monitor_id)
				result.reason = AmiberryGuiGuardRejectReason::monitor_mismatch;
			else if (environment.active_monitor_id != geometry.monitor_id)
				result.reason = AmiberryGuiGuardRejectReason::monitor_mismatch;
			else if (environment.input_config_revision
				!= request.input_config_revision)
				result.reason = AmiberryGuiGuardRejectReason::input_config_revision_mismatch;
			else if (!environment.settings_compatible)
				result.reason = AmiberryGuiGuardRejectReason::settings_incompatible;
			else if (!environment.focus_ready)
				result.reason = AmiberryGuiGuardRejectReason::focus_not_ready;
			else if (!amiberry_gui_button_mask_supported(request.button_mask))
				result.reason = AmiberryGuiGuardRejectReason::unsupported_button_mask;
			else if (request.x < 0 || request.x >= geometry.window_width
				|| request.y < 0 || request.y >= geometry.window_height)
				result.reason = AmiberryGuiGuardRejectReason::coordinate_out_of_bounds;
			else if (!mutation(geometry.monitor_id, request.x, request.y,
				request.button_mask))
				result.reason = AmiberryGuiGuardRejectReason::input_rejected;
			else {
				result.applied = true;
				result.button_mask = request.button_mask;
			}
			return result;
		});
}

template<typename Release>
static int amiberry_gui_release_mouse_buttons(Release&& release)
{
	for (int button = 0; button < 3; ++button)
		release(button);
	return 0;
}

struct AmiberryGuiInputConfigSnapshot
{
	int pending_tablet_mode = 0;
	int pending_mouse_untrap = 0;
	int effective_tablet_mode = 0;
	int effective_mouse_untrap = 0;
	std::uint64_t input_config_revision = 0;
};

struct AmiberryGuiPendingInputConfig
{
	int tablet_mode = 0;
	int mouse_untrap = 0;
};

struct AmiberryGuiInputConfigValues
{
	AmiberryGuiPendingInputConfig pending;
	AmiberryGuiPendingInputConfig effective;
};

enum class AmiberryGuiConfigRejectReason
{
	none,
	input_config_conflict
};

struct AmiberryGuiConfigResult
{
	bool applied = false;
	AmiberryGuiConfigRejectReason reason =
		AmiberryGuiConfigRejectReason::input_config_conflict;
	AmiberryGuiInputConfigSnapshot snapshot;
};

class AmiberryGuiInputConfigState
{
public:
	AmiberryGuiInputConfigSnapshot observe(const int pending_tablet_mode,
		const int pending_mouse_untrap, const int effective_tablet_mode,
		const int effective_mouse_untrap)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		synchronize(pending_tablet_mode, pending_mouse_untrap,
			effective_tablet_mode, effective_mouse_untrap);
		return snapshot_;
	}

	template<typename Mutation>
	AmiberryGuiConfigResult compare_exchange(
		const std::uint64_t expected_revision,
		const AmiberryGuiPendingInputConfig& expected,
		const AmiberryGuiInputConfigValues& current,
		const AmiberryGuiPendingInputConfig& desired,
		Mutation&& mutation)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		synchronize(current.pending.tablet_mode,
			current.pending.mouse_untrap, current.effective.tablet_mode,
			current.effective.mouse_untrap);
		AmiberryGuiConfigResult result;
		result.snapshot = snapshot_;
		if (snapshot_.input_config_revision != expected_revision
			|| snapshot_.pending_tablet_mode != expected.tablet_mode
			|| snapshot_.pending_mouse_untrap != expected.mouse_untrap) {
			return result;
		}
		mutation(desired.tablet_mode, desired.mouse_untrap);
		if (snapshot_.pending_tablet_mode != desired.tablet_mode
			|| snapshot_.pending_mouse_untrap != desired.mouse_untrap) {
			++revision_;
			snapshot_.pending_tablet_mode = desired.tablet_mode;
			snapshot_.pending_mouse_untrap = desired.mouse_untrap;
			snapshot_.input_config_revision = revision_;
		}
		result.applied = true;
		result.reason = AmiberryGuiConfigRejectReason::none;
		result.snapshot = snapshot_;
		return result;
	}

private:
	void synchronize(const int pending_tablet_mode,
		const int pending_mouse_untrap, const int effective_tablet_mode,
		const int effective_mouse_untrap)
	{
		const bool changed = !initialized_
			|| snapshot_.pending_tablet_mode != pending_tablet_mode
			|| snapshot_.pending_mouse_untrap != pending_mouse_untrap
			|| snapshot_.effective_tablet_mode != effective_tablet_mode
			|| snapshot_.effective_mouse_untrap != effective_mouse_untrap;
		if (changed)
			++revision_;
		initialized_ = true;
		snapshot_.pending_tablet_mode = pending_tablet_mode;
		snapshot_.pending_mouse_untrap = pending_mouse_untrap;
		snapshot_.effective_tablet_mode = effective_tablet_mode;
		snapshot_.effective_mouse_untrap = effective_mouse_untrap;
		snapshot_.input_config_revision = revision_;
	}

	std::mutex mutex_;
	bool initialized_ = false;
	std::uint64_t revision_ = 0;
	AmiberryGuiInputConfigSnapshot snapshot_;
};

#endif // AMIBERRY_GUI_AUTOMATION_H
