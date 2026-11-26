// The shared-buffer output converter against the card's sdk_media_output.c.
// The hashes below were produced by building the firmware's converter and
// running the same synthetic pictures through it, so a change here that moves
// a byte shows up as a mismatch with the card.
#include "zz9000_sdk_media_output.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace zz9000_sdk::media_output;

namespace {
constexpr uint16_t ok = 0, unsupported = 3, bad_request = 4;

struct Case {
	const char *name;
	unsigned sw, sh, dw, dh, format, flags, palette; // palette: 0 none, 1 256, 2 16 colours
	uint64_t hash;
};

// Formats: 1 RGB565, 4 INDEX8, 5 PLANAR, 6 RGB555. Flags: depth, 0x10
// interleaved, 0x20 ordered dither, 0x80 host palette, 0x100 nearest.
const Case cases[] = {
	{ "rgb565_same",      64, 48,  64, 48, 1, 0x00, 0, 0xf750d028f656812dull },
	{ "rgb565_dither_up", 64, 48, 100, 70, 1, 0x20, 0, 0x0d8eb39677ba7ba4ull },
	{ "rgb555_down",      64, 48,  30, 20, 6, 0x00, 0, 0x53dbaeab9ea50630ull },
	{ "rgb555_nearest",   64, 48,  97, 33, 6, 0x100, 0, 0xdde85839512884c6ull },
	{ "index8_d8",        64, 48,  64, 48, 4, 0x08, 0, 0x40ccfae312c3a2ffull },
	{ "index8_d8_dith",   161, 121, 80, 60, 4, 0x28, 0, 0xe2a1ea3eb0c4bfcaull },
	{ "index8_d5_up",     64, 48, 130, 90, 4, 0x25, 0, 0x95d7aa80e7685430ull },
	{ "index8_d3",        64, 48,  64, 48, 4, 0x03, 0, 0xaa03ef85bc60e3e3ull },
	{ "index8_d2_gray",   64, 48,  64, 48, 4, 0x22, 0, 0x16302fac1f2ffc23ull },
	{ "index8_d1_gray",   64, 48,  50, 40, 4, 0x01, 0, 0x90e41ee06eb37511ull },
	{ "index8_d6",        64, 48,  64, 48, 4, 0x26, 0, 0x5a1e389b5b1be898ull },
	{ "index8_d7",        64, 48,  64, 48, 4, 0x27, 0, 0x76c14716dcc68fd2ull },
	{ "index8_d4",        64, 48,  64, 48, 4, 0x24, 0, 0xc42bdeef5abac338ull },
	{ "planar_d5_sep",    64, 48,  72, 50, 5, 0x25, 0, 0x6b39713fc8da118bull },
	{ "planar_d5_il",     64, 48,  72, 50, 5, 0x35, 0, 0x1e3b941cca06a71full },
	{ "planar_d8_odd",    161, 121, 77, 51, 5, 0x28, 0, 0x2f2723938b647d33ull },
	{ "planar_d1",        64, 48,  64, 48, 5, 0x01, 0, 0x3d58f72cd11f0a05ull },
	{ "host256",          64, 48,  64, 48, 4, 0x88, 1, 0x1fb54976e39d0b2bull },
	{ "host256_dither",   64, 48,  80, 60, 4, 0xa8, 1, 0xd7c0169733cf7167ull },
	{ "host16_planar",    64, 48,  64, 48, 5, 0x84, 2, 0xb400bc536aef586eull },
	{ "host16_nearest",   161, 121, 64, 48, 4, 0x184, 2, 0x7b4344862aff2917ull },
	{ "area_both",        320, 240, 100, 60, 1, 0x00, 0, 0x150cc05f7e7b699bull },
	{ "area_chroma_odd",  161, 121, 80, 60, 6, 0x20, 0, 0x211e165737a3307dull },
	{ "tiny",             2, 2, 1, 1, 4, 0x08, 0, 0x4bec1917246ea3d0ull },
	{ "one_by_one_up",    1, 1, 9, 7, 1, 0x00, 0, 0x2d2026ccec8eb1c5ull },
};

uint64_t fnv(const uint8_t *p, size_t n)
{
	uint64_t h = 1469598103934665603ull;
	while (n--) {
		h ^= *p++;
		h *= 1099511628211ull;
	}
	return h;
}

Config config(unsigned width, unsigned height, unsigned format, unsigned flags,
              unsigned pitch)
{
	Config c;
	c.width = width; c.height = height; c.format = format; c.flags = flags;
	c.pitch = pitch;
	return c;
}

void check_vectors()
{
	Converter converter;
	for (const auto &c : cases) {
		const unsigned cw = (c.sw + 1) / 2, ch = (c.sh + 1) / 2;
		std::vector<uint8_t> y(c.sw * c.sh), cb(cw * ch), cr(cw * ch);
		for (unsigned r = 0; r < c.sh; ++r)
			for (unsigned x = 0; x < c.sw; ++x)
				y[r * c.sw + x] = static_cast<uint8_t>(16 + ((x * 3 + r * 5 + ((x * r) >> 3)) % 220));
		for (unsigned r = 0; r < ch; ++r)
			for (unsigned x = 0; x < cw; ++x) {
				cb[r * cw + x] = static_cast<uint8_t>(16 + ((x * 7 + r * 2) % 224));
				cr[r * cw + x] = static_cast<uint8_t>(16 + ((x * 2 + r * 11 + x * r) % 224));
			}
		Frame frame;
		frame.width = c.sw; frame.height = c.sh;
		frame.y = y.data(); frame.cb = cb.data(); frame.cr = cr.data();
		frame.y_pitch = c.sw; frame.chroma_pitch = cw;
		const unsigned row = c.format == surface_index8 ? c.dw : c.format == surface_planar ?
			(c.dw + 7) / 8 : c.dw * 2;
		const Config cfg = config(c.dw, c.dh, c.format, c.flags, (row + 3) & ~3u);
		Layout layout;
		assert(check(cfg, layout) == ok);
		uint8_t palette[palette_bytes] = {};
		if (c.palette) {
			const unsigned n = c.palette == 1 ? 256 : 16;
			for (unsigned k = 0; k < n; ++k) {
				palette[k * 4 + 1] = static_cast<uint8_t>(k * 37);
				palette[k * 4 + 2] = static_cast<uint8_t>(k * 91 + 5);
				palette[k * 4 + 3] = static_cast<uint8_t>(255 - k * 53);
			}
		}
		Job job;
		job.width = c.dw; job.height = c.dh; job.format = c.format;
		job.pitch = cfg.pitch; job.flags = c.flags; job.depth = layout.depth;
		assert(job_bytes(job) == layout.picture_bytes);
		std::vector<uint8_t> out(layout.picture_bytes + 16, 0xa5);
		const uint16_t status = converter.convert(job, frame, out.data(),
			(c.flags & flag_palette_host) ? palette : nullptr);
		if (status != ok || fnv(out.data(), layout.picture_bytes) != c.hash) {
			std::printf("%s: status %u hash %016llx\n", c.name, status,
				static_cast<unsigned long long>(fnv(out.data(), layout.picture_bytes)));
			assert(false);
		}
		// Nothing past the picture is written.
		for (size_t i = layout.picture_bytes; i < out.size(); ++i)
			assert(out[i] == 0xa5);
	}
}

void check_configuration()
{
	Layout layout;
	// A valid layout reports what it occupies.
	Config c = config(160, 100, surface_planar, 5, 20);
	assert(check(c, layout) == ok);
	assert(layout.depth == 5 && layout.row_bytes == 20 &&
	       layout.picture_bytes == 10000 && layout.slots == 1 &&
	       layout.span_bytes == 10000);
	c.slot_stride = 10000;
	assert(check(c, layout) == ok && layout.slots == 2 && layout.span_bytes == 20000);
	c.slot_stride = 9996; // shorter than a picture
	assert(check(c, layout) == bad_request);
	c.slot_stride = 10002; // not a multiple of 4
	assert(check(c, layout) == bad_request);
	c.slot_stride = 0;
	c.buffer_offset = 2;
	assert(check(c, layout) == bad_request);
	c.buffer_offset = 0;
	// Depth 0 means 8; above 8 is malformed.
	assert(check(config(8, 8, surface_index8, 0, 8), layout) == ok && layout.depth == 8);
	assert(check(config(8, 8, surface_index8, 9, 8), layout) == bad_request);
	// Size limits.
	assert(check(config(0, 8, surface_index8, 8, 8), layout) == bad_request);
	assert(check(config(8, 0, surface_index8, 8, 8), layout) == bad_request);
	assert(check(config(1921, 8, surface_rgb565, 0, 4000), layout) == bad_request);
	assert(check(config(8, 1089, surface_rgb565, 0, 16), layout) == bad_request);
	assert(check(config(1920, 1088, surface_rgb565, 0, 3840), layout) == ok);
	// A pitch shorter than the row.
	assert(check(config(100, 10, surface_index8, 8, 99), layout) == bad_request);
	assert(check(config(100, 10, surface_rgb555, 0, 199), layout) == bad_request);
	assert(check(config(100, 10, surface_planar, 3, 13), layout) == ok);
	assert(check(config(100, 10, surface_planar, 3, 12), layout) == bad_request);
	// Flags: unknown, interleaved INDEX8, host palette or depth on RGB.
	assert(check(config(8, 8, surface_index8, 8 | (1u << 9), 8), layout) == bad_request);
	assert(check(config(8, 8, surface_index8, 8 | flag_planar_interleaved, 8), layout) == bad_request);
	assert(check(config(8, 8, surface_rgb565, flag_palette_host, 16), layout) == bad_request);
	assert(check(config(8, 8, surface_rgb565, 3, 16), layout) == bad_request);
	assert(check(config(8, 8, surface_rgb565, flag_planar_interleaved, 16), layout) == bad_request);
	// Error diffusion is not implemented; with the ordered dither it is
	// malformed, and the status page's mask leaves it out.
	assert(check(config(8, 8, surface_rgb565, flag_dither_diffusion, 16), layout) == unsupported);
	assert(check(config(8, 8, surface_rgb565, flag_dither_diffusion | flag_dither_ordered, 16),
		layout) == bad_request);
	assert((flags_supported & flag_dither_diffusion) == 0);
	assert((flags_supported & flag_dither_ordered) != 0);
	// Other surface formats (ARGB8888) are unsupported.
	assert(check(config(8, 8, 2, 0, 32), layout) == unsupported);
}

void check_fixed_palettes()
{
	uint32_t palette[256];
	assert(fixed_palette(8, palette) == 252);
	assert(palette[0] == 0 && palette[251] == 0x00ffffff && palette[252] == 0);
	assert(fixed_palette(5, palette) == 32);
	assert(fixed_palette(3, palette) == 8 && palette[7] == 0x00ffffff);
	assert(fixed_palette(2, palette) == 4 && palette[1] == 0x00555555 &&
	       palette[2] == 0x00aaaaaa);
	assert(fixed_palette(1, palette) == 2 && palette[0] == 0 && palette[1] == 0x00ffffff);
	assert(fixed_palette(0, palette) == 0 && fixed_palette(9, palette) == 0);
}

void check_bad_jobs()
{
	Converter converter;
	std::vector<uint8_t> plane(64 * 64, 128), out(64 * 64 * 2);
	Frame frame;
	frame.width = 64; frame.height = 64;
	frame.y = plane.data(); frame.cb = plane.data(); frame.cr = plane.data();
	frame.y_pitch = 64; frame.chroma_pitch = 32;
	Job job;
	job.width = 64; job.height = 64; job.format = surface_rgb565; job.pitch = 128;
	assert(converter.convert(job, frame, out.data(), nullptr) == ok);
	assert(converter.convert(job, frame, nullptr, nullptr) == bad_request);
	Frame narrow = frame;
	narrow.y_pitch = 63;
	assert(converter.convert(job, narrow, out.data(), nullptr) == bad_request);
	Frame none = frame;
	none.cr = nullptr;
	assert(converter.convert(job, none, out.data(), nullptr) == bad_request);
	// A host palette job needs the palette.
	Job host;
	host.width = 64; host.height = 64; host.format = surface_index8; host.pitch = 64;
	host.flags = flag_palette_host | 8; host.depth = 8;
	assert(converter.convert(host, frame, out.data(), nullptr) == bad_request);
}
} // namespace

int main()
{
	check_vectors();
	check_configuration();
	check_fixed_palettes();
	check_bad_jobs();
	return 0;
}
