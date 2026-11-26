/*
 * Shared-buffer output of a media session: scale a decoded planar 4:2:0
 * picture and convert it to INDEX8, bitplanes, RGB565 or RGB555 in memory
 * the guest can read. This is the card's sdk_media_output.c, so a picture
 * converts to the same bytes here as on the card (BT.601 limited range,
 * vertical then horizontal scale per plane, a 4x4 Bayer threshold per
 * channel, nearest-colour lookup for a host palette).
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_MEDIA_OUTPUT_H
#define AMIBERRY_ZZ9000_SDK_MEDIA_OUTPUT_H

#include <cstdint>
#include <memory>

namespace zz9000_sdk {
namespace media_output {

// Largest output and largest source width.
constexpr uint32_t max_width = 1920;
constexpr uint32_t max_height = 1088;
constexpr uint32_t max_source_width = 1920;
// A host palette is 256 big-endian 0x00RRGGBB words.
constexpr uint32_t palette_bytes = 1024;

constexpr uint32_t surface_rgb565 = 1;
constexpr uint32_t surface_index8 = 4;
constexpr uint32_t surface_planar = 5;
constexpr uint32_t surface_rgb555 = 6;

// ZZ9KMediaOutputFlags.
constexpr uint32_t flag_depth_mask = 0x0f;
constexpr uint32_t flag_planar_interleaved = 1u << 4;
constexpr uint32_t flag_dither_ordered = 1u << 5;
constexpr uint32_t flag_dither_diffusion = 1u << 6;
constexpr uint32_t flag_palette_host = 1u << 7;
constexpr uint32_t flag_scale_nearest = 1u << 8;
constexpr uint32_t flags_known = flag_depth_mask | flag_planar_interleaved |
	flag_dither_ordered | flag_dither_diffusion | flag_palette_host |
	flag_scale_nearest;
// Reported on status page 6: error-diffusion dithering is not implemented.
constexpr uint32_t flags_supported = flags_known & ~flag_dither_diffusion;

// A decoded OUTPUT_CONFIG request, as the client sent it.
struct Config {
	uint32_t width = 0, height = 0;
	uint32_t format = 0;       // Surface format.
	uint32_t pitch = 0;
	uint32_t flags = 0;
	uint32_t buffer_offset = 0; // Slot 0, bytes from the buffer start.
	uint32_t slot_stride = 0;   // 0 means one slot.
};

// What a valid configuration occupies.
struct Layout {
	uint32_t depth = 0;         // 1..8 for INDEX8 and PLANAR, 0 for RGB.
	uint32_t row_bytes = 0;     // Smallest legal pitch.
	uint32_t picture_bytes = 0; // One picture.
	uint32_t slots = 0;         // 1 or 2.
	uint32_t span_bytes = 0;    // From slot 0 to the end of the last slot.
};

// Validates a configuration: ok, bad_request for a malformed one, or
// unsupported for a format or flag that is not implemented. `layout` is
// filled only on success.
uint16_t check(const Config &config, Layout &layout);

// The fixed palette of an INDEX8 or PLANAR output of `depth` bits (1..8):
// 256 words, 0x00RRGGBB, entries beyond the colours in use are zero.
// Returns the number of colours in use.
uint32_t fixed_palette(uint32_t depth, uint32_t out[256]);

// A decoded picture: three planes, the chroma ones half size both ways.
struct Frame {
	uint32_t width = 0, height = 0;
	const uint8_t *y = nullptr, *cb = nullptr, *cr = nullptr;
	uint32_t y_pitch = 0, chroma_pitch = 0;
};

// One conversion: the configured output, minus where it goes.
struct Job {
	uint32_t width = 0, height = 0;
	uint32_t format = 0;
	uint32_t pitch = 0;
	uint32_t flags = 0;
	uint32_t depth = 0; // From Layout.
};

// Bytes one picture of the job occupies (pitch * height * planes).
uint32_t job_bytes(const Job &job);

// Scales and converts pictures. It keeps its scaling plans, quantiser
// tables and the nearest-colour table of a host palette between pictures,
// keyed by geometry and format, so a picture costs the per-pixel work only.
// About 110 KB: hold it by pointer.
class Converter {
public:
	Converter();
	~Converter();
	Converter(const Converter &) = delete;
	Converter &operator=(const Converter &) = delete;
	// Scales `frame` to the job's size and converts it into `dst`.
	// `palette` is the host palette (palette_bytes big-endian bytes) when
	// the job has flag_palette_host, else null. ok, or bad_request for a
	// frame or job it cannot convert.
	uint16_t convert(const Job &job, const Frame &frame, uint8_t *dst,
	                 const uint8_t *palette);

private:
	struct State;
	std::unique_ptr<State> state_;
};

} // namespace media_output
} // namespace zz9000_sdk

#endif
