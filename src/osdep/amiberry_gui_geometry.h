#ifndef AMIBERRY_GUI_GEOMETRY_H
#define AMIBERRY_GUI_GEOMETRY_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "amiberry_gfx_geometry.h"

// Immutable geometry bound to one displayed frame mapping. Source coordinates
// are a half-open rectangle in the captured image. Viewport coordinates are a
// half-open rectangle in SDL logical window coordinates, never drawable pixels.
struct AmiberryGuiGeometrySnapshot
{
	std::string runtime_id;
	std::string capture_nonce;
	std::uint64_t geometry_revision = 0;
	int monitor_id = 0;
	std::uint32_t display_id = 0;
	std::string display_mode;
	std::string renderer;
	int image_width = 0;
	int image_height = 0;
	AmiberryGfxRect source{};
	AmiberryGfxRect viewport{};
	int window_width = 0;
	int window_height = 0;
	bool valid = false;
};

static inline bool amiberry_gui_rect_equal(
	const AmiberryGfxRect& lhs, const AmiberryGfxRect& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.w == rhs.w && lhs.h == rhs.h;
}

static inline bool amiberry_gui_mapping_equal(
	const AmiberryGuiGeometrySnapshot& lhs,
	const AmiberryGuiGeometrySnapshot& rhs)
{
	return lhs.monitor_id == rhs.monitor_id
		&& lhs.display_id == rhs.display_id
		&& lhs.display_mode == rhs.display_mode
		&& lhs.renderer == rhs.renderer
		&& lhs.image_width == rhs.image_width
		&& lhs.image_height == rhs.image_height
		&& amiberry_gui_rect_equal(lhs.source, rhs.source)
		&& amiberry_gui_rect_equal(lhs.viewport, rhs.viewport)
		&& lhs.window_width == rhs.window_width
		&& lhs.window_height == rhs.window_height;
}

static inline int amiberry_gui_scale_logical_edge(
	const int edge, const int drawable_size, const int logical_size)
{
	if (drawable_size <= 0 || logical_size <= 0)
		return 0;
	const std::int64_t numerator = static_cast<std::int64_t>(edge)
		* logical_size * 2 + drawable_size;
	return static_cast<int>(numerator / (static_cast<std::int64_t>(drawable_size) * 2));
}

// Convert drawable-pixel edges to SDL logical window edges exactly once.
// Both input and output rectangles use half-open [left,right) / [top,bottom).
static inline AmiberryGfxRect amiberry_gui_drawable_to_logical_rect(
	const AmiberryGfxRect& drawable_rect,
	const int drawable_width, const int drawable_height,
	const int logical_width, const int logical_height)
{
	if (drawable_width <= 0 || drawable_height <= 0
		|| logical_width <= 0 || logical_height <= 0) {
		return {};
	}
	const int left = amiberry_gui_scale_logical_edge(
		drawable_rect.x, drawable_width, logical_width);
	const int top = amiberry_gui_scale_logical_edge(
		drawable_rect.y, drawable_height, logical_height);
	const int right = amiberry_gui_scale_logical_edge(
		drawable_rect.x + drawable_rect.w, drawable_width, logical_width);
	const int bottom = amiberry_gui_scale_logical_edge(
		drawable_rect.y + drawable_rect.h, drawable_height, logical_height);
	return {left, top, right - left, bottom - top};
}

// Match renderer backends that bound a viewport's origin and extent separately.
// This is intentionally not a geometric intersection: it describes the actual
// viewport after the renderer clamps the values passed to its graphics API.
static inline AmiberryGfxRect amiberry_gui_clamp_viewport(
	const AmiberryGfxRect& viewport, const int width, const int height)
{
	if (width <= 0 || height <= 0)
		return {};
	const int x = std::clamp(viewport.x, 0, width);
	const int y = std::clamp(viewport.y, 0, height);
	return {
		x,
		y,
		std::clamp(viewport.w, 0, width - x),
		std::clamp(viewport.h, 0, height - y)
	};
}

// Lock-backed publication prevents readers from observing fields from two
// renderer generations. Equivalent per-frame publications retain the revision.
class AmiberryGuiGeometryState
{
public:
	explicit AmiberryGuiGeometryState(std::string runtime_id)
		: runtime_id_(std::move(runtime_id))
	{
		snapshot_.runtime_id = runtime_id_;
	}

	void publish(AmiberryGuiGeometrySnapshot candidate)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (snapshot_.valid
			&& amiberry_gui_mapping_equal(snapshot_, candidate)) {
			return;
		}
		++revision_;
		active_monitor_ = candidate.monitor_id;
		candidate.runtime_id = runtime_id_;
		candidate.capture_nonce.clear();
		candidate.geometry_revision = revision_;
		candidate.valid = true;
		snapshot_ = std::move(candidate);
	}

	void invalidate(const int monitor_id = -1)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (monitor_id >= 0 && active_monitor_ >= 0
			&& monitor_id != active_monitor_) {
			return;
		}
		++revision_;
		snapshot_.runtime_id = runtime_id_;
		snapshot_.capture_nonce.clear();
		snapshot_.geometry_revision = revision_;
		snapshot_.valid = false;
	}

	void set_active_monitor(const int monitor_id)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (active_monitor_ < 0) {
			active_monitor_ = monitor_id;
			return;
		}
		if (active_monitor_ == monitor_id)
			return;
		active_monitor_ = monitor_id;
		++revision_;
		snapshot_.runtime_id = runtime_id_;
		snapshot_.capture_nonce.clear();
		snapshot_.geometry_revision = revision_;
		snapshot_.valid = false;
	}

	AmiberryGuiGeometrySnapshot snapshot() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return snapshot_;
	}

	template<typename Callback>
	auto with_synchronized_snapshot(Callback&& callback)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return callback(static_cast<const AmiberryGuiGeometrySnapshot&>(snapshot_));
	}

private:
	std::string runtime_id_;
	mutable std::mutex mutex_;
	std::uint64_t revision_ = 0;
	int active_monitor_ = -1;
	AmiberryGuiGeometrySnapshot snapshot_;
};

static inline std::vector<std::string> amiberry_gui_actionable_fields(
	const std::string& path, const AmiberryGuiGeometrySnapshot& snapshot)
{
	return {
		"schema_version=1",
		"path=" + path,
		"runtime_id=" + snapshot.runtime_id,
		"capture_nonce=" + snapshot.capture_nonce,
		"geometry_revision=" + std::to_string(snapshot.geometry_revision),
		"monitor_id=" + std::to_string(snapshot.monitor_id),
		"display_mode=" + snapshot.display_mode,
		"renderer=" + snapshot.renderer,
		"image_width=" + std::to_string(snapshot.image_width),
		"image_height=" + std::to_string(snapshot.image_height),
		"source_x=" + std::to_string(snapshot.source.x),
		"source_y=" + std::to_string(snapshot.source.y),
		"source_width=" + std::to_string(snapshot.source.w),
		"source_height=" + std::to_string(snapshot.source.h),
		"viewport_x=" + std::to_string(snapshot.viewport.x),
		"viewport_y=" + std::to_string(snapshot.viewport.y),
		"viewport_width=" + std::to_string(snapshot.viewport.w),
		"viewport_height=" + std::to_string(snapshot.viewport.h),
		"window_width=" + std::to_string(snapshot.window_width),
		"window_height=" + std::to_string(snapshot.window_height)
	};
}

static inline bool amiberry_gui_png_dimensions(const unsigned char* data,
	const std::size_t size, int& width, int& height)
{
	static const unsigned char png_signature[8]{
		137, 80, 78, 71, 13, 10, 26, 10
	};
	if (!data || size < 24
		|| std::memcmp(data, png_signature, sizeof png_signature) != 0
		|| std::memcmp(data + 12, "IHDR", 4) != 0) {
		return false;
	}
	width = static_cast<int>((static_cast<std::uint32_t>(data[16]) << 24)
		| (static_cast<std::uint32_t>(data[17]) << 16)
		| (static_cast<std::uint32_t>(data[18]) << 8)
		| static_cast<std::uint32_t>(data[19]));
	height = static_cast<int>((static_cast<std::uint32_t>(data[20]) << 24)
		| (static_cast<std::uint32_t>(data[21]) << 16)
		| (static_cast<std::uint32_t>(data[22]) << 8)
		| static_cast<std::uint32_t>(data[23]));
	return width > 0 && height > 0;
}

#endif // AMIBERRY_GUI_GEOMETRY_H
