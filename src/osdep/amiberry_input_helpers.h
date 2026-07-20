#ifndef AMIBERRY_INPUT_HELPERS_H
#define AMIBERRY_INPUT_HELPERS_H

static inline int amiberry_input_clamp_native_axis(int value, int extent, bool* out_of_bounds)
{
	if (extent <= 0) {
		if (out_of_bounds) {
			*out_of_bounds = true;
		}
		return 0;
	}
	if (value < 0) {
		if (out_of_bounds) {
			*out_of_bounds = true;
		}
		return 0;
	}
	if (value >= extent) {
		if (out_of_bounds) {
			*out_of_bounds = true;
		}
		return extent - 1;
	}
	if (out_of_bounds) {
		*out_of_bounds = false;
	}
	return value;
}

static inline int amiberry_input_mousehack_hotspot_axis(int pointer_offset, int extent, int bias)
{
	if (extent <= 0) {
		return 0;
	}

	const int hotspot = -(pointer_offset + bias);
	if (hotspot < 0) {
		return 0;
	}
	if (hotspot >= extent) {
		return extent - 1;
	}
	return hotspot;
}

static inline int amiberry_input_mousehack_hotspot_residual_axis(int pointer_offset, int extent, int bias)
{
	if (extent <= 0) {
		return 0;
	}

	const int hotspot = amiberry_input_mousehack_hotspot_axis(pointer_offset, extent, bias);
	return pointer_offset + bias + hotspot;
}

static inline void amiberry_input_mousehack_cursor_hotspot(int pointer_x_offset, int pointer_y_offset,
	int cursor_width, int cursor_height, int* hotspot_x, int* hotspot_y)
{
	if (hotspot_x) {
		*hotspot_x = amiberry_input_mousehack_hotspot_axis(pointer_x_offset, cursor_width, 1);
	}
	if (hotspot_y) {
		*hotspot_y = amiberry_input_mousehack_hotspot_axis(pointer_y_offset, cursor_height, 2);
	}
}

static inline int amiberry_input_native_sprite_x(uae_u16 pos, uae_u16 ctl, bool extended_x)
{
	// Mousehack uses hires-granularity coordinates, so AGA's half-step bit cannot be represented here.
	int x = ((pos & 0xff) << 2) | ((ctl & 0x01) << 1);
	if (extended_x && (ctl & 0x10)) {
		x |= 1;
	}
	return x;
}

static inline int amiberry_input_native_sprite_y(uae_u16 pos, uae_u16 ctl, bool extended_y)
{
	int y = pos >> 8;
	if (ctl & 0x04) {
		y |= 0x0100;
	}
	if (extended_y && (ctl & 0x40)) {
		y |= 0x0200;
	}
	return y << 1;
}

static inline int amiberry_input_native_cursor_height(int sprite_height,
	bool double_scan_position_bit, bool sscan2_enabled)
{
	return double_scan_position_bit && sscan2_enabled ? sprite_height / 2 : sprite_height;
}

static inline int amiberry_input_native_mousehack_origin_compensation(
	bool source_origin_valid, int guest_screen_offset, int transformed_native_origin)
{
	return source_origin_valid ? guest_screen_offset - transformed_native_origin : 0;
}

static inline int amiberry_input_native_mousehack_uncropped_lowres_x_compensation(
	bool source_origin_valid, bool low_resolution)
{
	// Native LowRes mousehack coordinates use hires units. Without Auto Crop,
	// the input conversion retains one LowRes pixel of horizontal bias, so move
	// the delivered position left by two hires units.
	return !source_origin_valid && low_resolution ? -2 : 0;
}

static inline int amiberry_input_native_interlace_origin(
	int source_origin, bool interlaced, bool long_field, int field_line_height)
{
	if (interlaced && long_field && field_line_height > 0) {
		return source_origin - field_line_height;
	}
	return source_origin;
}

struct amiberry_input_cursor_hotspot_tracker
{
	bool have_sample = false;
	bool learned = false;
	bool confirmed_x = false;
	bool confirmed_y = false;
	bool candidate_x_moved = false;
	bool candidate_y_moved = false;
	int last_pointer_x = 0;
	int last_pointer_y = 0;
	int last_sprite_x = 0;
	int last_sprite_y = 0;
	int candidate_x = 0;
	int candidate_y = 0;
	int stable_samples = 0;
	int hotspot_x = 0;
	int hotspot_y = 0;
};

constexpr int AMIBERRY_INPUT_CURSOR_HOTSPOT_TRACKER_CACHE_SIZE = 4;

struct amiberry_input_cursor_hotspot_tracker_cache_entry
{
	bool valid = false;
	int cursor_width = 0;
	int cursor_height = 0;
	uae_u64 cursor_signature = 0;
	unsigned int last_used = 0;
	amiberry_input_cursor_hotspot_tracker tracker;
};

struct amiberry_input_cursor_hotspot_tracker_cache
{
	unsigned int use_sequence = 0;
	amiberry_input_cursor_hotspot_tracker_cache_entry
		entries[AMIBERRY_INPUT_CURSOR_HOTSPOT_TRACKER_CACHE_SIZE];
};

static inline void amiberry_input_cursor_hotspot_tracker_reset(
	amiberry_input_cursor_hotspot_tracker* tracker)
{
	if (tracker) {
		*tracker = {};
	}
}

static inline void amiberry_input_cursor_hotspot_tracker_cache_reset(
	amiberry_input_cursor_hotspot_tracker_cache* cache)
{
	if (cache) {
		*cache = {};
	}
}

static inline uae_u64 amiberry_input_cursor_bitmap_signature(
	const uae_u8* image, int image_size, const uae_u32* colors, int color_count)
{
	uae_u64 signature = 14695981039346656037ULL;
	for (int i = 0; image && i < image_size; i++) {
		signature ^= image[i];
		signature *= 1099511628211ULL;
	}
	for (int i = 0; colors && i < color_count; i++) {
		for (int shift = 0; shift < 32; shift += 8) {
			signature ^= (colors[i] >> shift) & 0xff;
			signature *= 1099511628211ULL;
		}
	}
	return signature;
}

static inline uae_u64 amiberry_input_cursor_hotspot_signature(
	uae_u64 cursor_signature, int hotspot_x, int hotspot_y)
{
	const uae_u32 offsets[] = {
		static_cast<uae_u32>(hotspot_x),
		static_cast<uae_u32>(hotspot_y)
	};
	for (const uae_u32 offset : offsets) {
		for (int shift = 0; shift < 32; shift += 8) {
			cursor_signature ^= (offset >> shift) & 0xff;
			cursor_signature *= 1099511628211ULL;
		}
	}
	return cursor_signature;
}

static inline amiberry_input_cursor_hotspot_tracker*
amiberry_input_cursor_hotspot_tracker_cache_acquire(
	amiberry_input_cursor_hotspot_tracker_cache* cache, int cursor_width, int cursor_height,
	uae_u64 cursor_signature)
{
	if (!cache || cursor_width <= 0 || cursor_height <= 0) {
		return nullptr;
	}

	cache->use_sequence++;
	amiberry_input_cursor_hotspot_tracker_cache_entry* available = nullptr;
	amiberry_input_cursor_hotspot_tracker_cache_entry* oldest = &cache->entries[0];
	for (auto& entry : cache->entries) {
		if (entry.valid && entry.cursor_width == cursor_width && entry.cursor_height == cursor_height
			&& entry.cursor_signature == cursor_signature) {
			entry.last_used = cache->use_sequence;
			return &entry.tracker;
		}
		if (!entry.valid && !available) {
			available = &entry;
		}
		if (entry.last_used < oldest->last_used) {
			oldest = &entry;
		}
	}

	auto* entry = available ? available : oldest;
	*entry = {};
	entry->valid = true;
	entry->cursor_width = cursor_width;
	entry->cursor_height = cursor_height;
	entry->cursor_signature = cursor_signature;
	entry->last_used = cache->use_sequence;
	return &entry->tracker;
}

static inline bool amiberry_input_cursor_hotspot_tracker_sample(
	amiberry_input_cursor_hotspot_tracker* tracker, bool position_valid,
	int pointer_x, int pointer_y, int sprite_x, int sprite_y,
	int cursor_width, int cursor_height, int required_stable_samples,
	int* hotspot_x, int* hotspot_y)
{
	if (!tracker) {
		return false;
	}
	if (tracker->confirmed_x && tracker->confirmed_y) {
		// The hotspot belongs to the cursor bitmap, not its current screen
		// position. Edge-clamped sprite coordinates must not relearn it.
		if (hotspot_x) {
			*hotspot_x = tracker->hotspot_x;
		}
		if (hotspot_y) {
			*hotspot_y = tracker->hotspot_y;
		}
		return true;
	}

	const int candidate_x = pointer_x - sprite_x;
	const int candidate_y = pointer_y - sprite_y;
	const bool candidate_valid = position_valid && cursor_width > 0 && cursor_height > 0
		&& candidate_x >= 0 && candidate_x < cursor_width
		&& candidate_y >= 0 && candidate_y < cursor_height;

	if (candidate_valid) {
		const bool stable = tracker->have_sample
			&& candidate_x == tracker->candidate_x && candidate_y == tracker->candidate_y;
		const bool moved_x = stable
			&& pointer_x != tracker->last_pointer_x && sprite_x != tracker->last_sprite_x;
		const bool moved_y = stable
			&& pointer_y != tracker->last_pointer_y && sprite_y != tracker->last_sprite_y;

		tracker->stable_samples = stable ? tracker->stable_samples + 1 : 1;
		tracker->candidate_x_moved = stable
			? tracker->candidate_x_moved || moved_x : false;
		tracker->candidate_y_moved = stable
			? tracker->candidate_y_moved || moved_y : false;
		tracker->have_sample = true;
		tracker->last_pointer_x = pointer_x;
		tracker->last_pointer_y = pointer_y;
		tracker->last_sprite_x = sprite_x;
		tracker->last_sprite_y = sprite_y;
		tracker->candidate_x = candidate_x;
		tracker->candidate_y = candidate_y;

		if (tracker->stable_samples >= required_stable_samples) {
			if (!tracker->learned) {
				tracker->learned = true;
				tracker->hotspot_x = candidate_x;
				tracker->hotspot_y = candidate_y;
			}
			// Stationary samples provide a provisional hotspot, but only
			// coordinated movement can replace and confirm an axis. A clamped
			// sprite cannot follow pointer movement along that axis.
			if (!tracker->confirmed_x && tracker->candidate_x_moved) {
				tracker->hotspot_x = candidate_x;
				tracker->confirmed_x = true;
			}
			if (!tracker->confirmed_y && tracker->candidate_y_moved) {
				tracker->hotspot_y = candidate_y;
				tracker->confirmed_y = true;
			}
		}
	} else {
		tracker->have_sample = false;
		tracker->stable_samples = 0;
		tracker->candidate_x_moved = false;
		tracker->candidate_y_moved = false;
	}

	if (tracker->learned) {
		if (hotspot_x) {
			*hotspot_x = tracker->hotspot_x;
		}
		if (hotspot_y) {
			*hotspot_y = tracker->hotspot_y;
		}
	}
	return tracker->learned;
}

static inline bool amiberry_input_cursor_hotspot_should_defer_swap(
	bool cursor_update_needed, bool compatible_previous_cursor,
	const amiberry_input_cursor_hotspot_tracker* tracker)
{
	return cursor_update_needed && compatible_previous_cursor && tracker
		&& tracker->have_sample && !tracker->learned;
}

static inline bool amiberry_input_cursor_snap_hotspot_to_dense_crossing(
	int candidate_x, int candidate_y, int dense_x, int dense_y, int maximum_distance,
	int* hotspot_x, int* hotspot_y)
{
	if (maximum_distance < 0
		|| candidate_x < dense_x - maximum_distance || candidate_x > dense_x + maximum_distance
		|| candidate_y < dense_y - maximum_distance || candidate_y > dense_y + maximum_distance) {
		return false;
	}
	if (hotspot_x) {
		*hotspot_x = dense_x;
	}
	if (hotspot_y) {
		*hotspot_y = dense_y;
	}
	return true;
}

#endif
