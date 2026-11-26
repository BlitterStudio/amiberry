/*
 * Shared-buffer output of a media session. See zz9000_sdk_media_output.h.
 *
 * The conversion is the card's, documented in the SDK's
 * zz9k-media-output.md: the scaler works per axis on the three planes
 * (vertical pass into a row, horizontal pass into the output width), the
 * colour matrix is BT.601 limited range with the constants the overlay
 * compositor uses, and the quantiser adds a 4x4 Bayer threshold per
 * channel.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "zz9000_sdk_media_output.h"
#include "zz9000_sdk_internal.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace zz9000_sdk {
namespace media_output {
namespace {

constexpr uint32_t max_w = max_width;
constexpr uint32_t max_chroma_w = (max_source_width + 1) / 2;

constexpr uint8_t bayer4[4][4] = {
	{ 0, 8, 2, 10 },
	{ 12, 4, 14, 6 },
	{ 3, 11, 1, 9 },
	{ 15, 7, 13, 5 },
};

// Cube geometry of the fixed palettes. Colour i of a cube palette is
// (r * lg + g) * lb + b with r < lr, g < lg, b < lb; a gray palette has
// 1 << depth levels.
struct FixedDims {
	uint8_t lr, lg, lb, gray;
};

constexpr FixedDims fixed_dims[9] = {
	{ 0, 0, 0, 0 },
	{ 2, 2, 2, 1 },
	{ 4, 4, 4, 1 },
	{ 2, 2, 2, 0 },
	{ 2, 4, 2, 0 },
	{ 4, 4, 2, 0 },
	{ 4, 4, 4, 0 },
	{ 5, 5, 5, 0 },
	{ 6, 7, 6, 0 },
};

// Bayer amplitude of a host palette, one cube step of an equivalent cube.
constexpr uint8_t host_step[9] = { 0, 255, 255, 255, 170, 128, 85, 64, 48 };

uint32_t level_value(uint32_t i, uint32_t levels)
{
	if (levels <= 1)
		return 0;
	return (i * 255 + (levels - 1) / 2) / (levels - 1);
}

uint32_t row_bytes_for(uint32_t format, uint32_t width)
{
	if (format == surface_index8)
		return width;
	if (format == surface_planar)
		return (width + 7) / 8;
	return width * 2;
}

// Two taps per output pixel (nearest and bilinear:
// out = lerp(src[i0], src[i1], w / 256)), or an area average over
// count[x] source pixels starting at first[x].
struct Tap2 {
	uint16_t i0, i1, w;
};

struct HPlan {
	Tap2 tap[max_w];
	uint16_t first[max_w];
	uint16_t count[max_w];
	uint16_t weight[2 * max_w];
	uint32_t r24 = 0;
	bool area = false;
	bool identity = false;
};

uint32_t plane_size(uint32_t luma, bool chroma)
{
	return chroma ? (luma + 1) / 2 : luma;
}

bool use_area(uint32_t luma_src, uint32_t dst, bool nearest)
{
	return !nearest && luma_src >= 2 * dst;
}

// Two-tap position of output index i on an axis whose luma size is `sl`,
// scaled to `d` output pixels. `chroma` selects the half-size chroma plane,
// whose samples sit between pairs of luma pixels.
void tap_at(Tap2 &t, uint32_t i, uint32_t sl, uint32_t d, bool chroma,
            bool nearest)
{
	const uint32_t size = plane_size(sl, chroma);
	if (nearest) {
		uint32_t at = ((2 * i + 1) * sl) / (2 * d);
		if (chroma)
			at >>= 1;
		if (at >= size)
			at = size - 1;
		t.i0 = t.i1 = static_cast<uint16_t>(at);
		t.w = 0;
		return;
	}
	const int64_t pos = static_cast<int64_t>(
		((static_cast<uint64_t>(2 * i + 1) * sl) << (chroma ? 14 : 15)) / d) -
		32768;
	if (pos < 0 || size == 1) {
		t.i0 = t.i1 = 0;
		t.w = 0;
		return;
	}
	if (static_cast<uint64_t>(pos >> 16) >= size - 1) {
		t.i0 = t.i1 = static_cast<uint16_t>(size - 1);
		t.w = 0;
		return;
	}
	t.i0 = static_cast<uint16_t>(pos >> 16);
	t.i1 = static_cast<uint16_t>(t.i0 + 1);
	t.w = static_cast<uint16_t>((pos >> 8) & 255);
}

uint32_t reciprocal24(uint32_t size)
{
	return static_cast<uint32_t>(((1ull << 24) + size / 2) / size);
}

uint8_t area_round(uint32_t sum, uint32_t r24)
{
	return static_cast<uint8_t>(
		(static_cast<uint64_t>(sum) * r24 + (1ull << 23)) >> 24);
}

void build_hplan(HPlan &p, uint32_t sl, uint32_t d, bool chroma, bool nearest)
{
	const uint32_t size = plane_size(sl, chroma);
	p.area = use_area(sl, d, nearest);
	p.identity = false;
	if (p.area) {
		uint32_t total = 0;
		p.r24 = reciprocal24(size);
		for (uint32_t x = 0; x < d; ++x) {
			const uint32_t s0 = x * size, s1 = s0 + size;
			const uint32_t j0 = s0 / d;
			uint32_t j1 = (s1 + d - 1) / d;
			if (j1 > size)
				j1 = size;
			p.first[x] = static_cast<uint16_t>(j0);
			p.count[x] = static_cast<uint16_t>(j1 - j0);
			for (uint32_t j = j0; j < j1; ++j) {
				const uint32_t lo = j * d > s0 ? j * d : s0;
				const uint32_t hi = (j + 1) * d < s1 ? (j + 1) * d : s1;
				p.weight[total++] = static_cast<uint16_t>(hi - lo);
			}
		}
		return;
	}
	for (uint32_t x = 0; x < d; ++x)
		tap_at(p.tap[x], x, sl, d, chroma, nearest);
	if (!chroma) {
		p.identity = true;
		for (uint32_t x = 0; x < d; ++x) {
			if (p.tap[x].i0 != x || p.tap[x].w != 0) {
				p.identity = false;
				break;
			}
		}
	}
}

// Level of value v on a channel of `levels` levels with Bayer threshold
// (2 * t + 1) / 32: floor(v * (levels - 1) / 255 + (2t + 1) / 32), capped.
// Row 16 is the plain rounding threshold of one half.
uint32_t level_for(uint32_t v, uint32_t levels, uint32_t row)
{
	const uint32_t thr = row < 16 ? 2 * row + 1 : 16;
	const uint32_t q = (v * (levels - 1) * 32 + thr * 255) / (255 * 32);
	return q > levels - 1 ? levels - 1 : q;
}

int32_t clamp8(int32_t v)
{
	return v < 0 ? 0 : v > 255 ? 255 : v;
}

void yuv_to_rgb(int32_t y, int32_t cb, int32_t cr, int32_t &r, int32_t &g,
                int32_t &b)
{
	const int32_t c = 298 * (y - 16);
	const int32_t d = cb - 128;
	const int32_t e = cr - 128;
	r = clamp8((c + 409 * e + 128) >> 8);
	g = clamp8((c - 100 * d - 208 * e + 128) >> 8);
	b = clamp8((c + 516 * d + 128) >> 8);
}

enum class RowKind { cube, gray, host, rgb };

} // namespace

uint16_t check(const Config &cfg, Layout &layout)
{
	const uint32_t flags = cfg.flags;
	if ((flags & ~flags_known) != 0 || cfg.width == 0 || cfg.width > max_width ||
	    cfg.height == 0 || cfg.height > max_height ||
	    (cfg.buffer_offset & 3) != 0 || (cfg.slot_stride & 3) != 0)
		return bad_request;
	if ((flags & flag_dither_ordered) != 0 && (flags & flag_dither_diffusion) != 0)
		return bad_request;

	Layout out;
	switch (cfg.format) {
	case surface_index8:
	case surface_planar:
		out.depth = flags & flag_depth_mask;
		if (out.depth == 0)
			out.depth = 8;
		if (out.depth > 8)
			return bad_request;
		if (cfg.format == surface_index8 && (flags & flag_planar_interleaved) != 0)
			return bad_request;
		break;
	case surface_rgb565:
	case surface_rgb555:
		if ((flags & (flag_depth_mask | flag_planar_interleaved |
		              flag_palette_host)) != 0)
			return bad_request;
		break;
	default:
		return unsupported;
	}
	if ((flags & flag_dither_diffusion) != 0)
		return unsupported;

	out.row_bytes = row_bytes_for(cfg.format, cfg.width);
	if (cfg.pitch < out.row_bytes)
		return bad_request;
	uint64_t picture = static_cast<uint64_t>(cfg.pitch) * cfg.height;
	if (cfg.format == surface_planar)
		picture *= out.depth;
	out.slots = cfg.slot_stride != 0 ? 2 : 1;
	uint64_t span = picture;
	if (out.slots == 2) {
		if (cfg.slot_stride < picture)
			return bad_request;
		span += cfg.slot_stride;
	}
	if (span > 0xffffffffull)
		return bad_request;
	out.picture_bytes = static_cast<uint32_t>(picture);
	out.span_bytes = static_cast<uint32_t>(span);
	layout = out;
	return ok;
}

uint32_t job_bytes(const Job &job)
{
	uint32_t bytes = job.pitch * job.height;
	if (job.format == surface_planar)
		bytes *= job.depth;
	return bytes;
}

uint32_t fixed_palette(uint32_t depth, uint32_t out[256])
{
	std::memset(out, 0, 256 * sizeof(out[0]));
	if (depth < 1 || depth > 8)
		return 0;
	const FixedDims &d = fixed_dims[depth];
	if (d.gray) {
		const uint32_t used = 1u << depth;
		for (uint32_t i = 0; i < used; ++i) {
			const uint32_t v = level_value(i, used);
			out[i] = (v << 16) | (v << 8) | v;
		}
		return used;
	}
	const uint32_t used = static_cast<uint32_t>(d.lr) * d.lg * d.lb;
	for (uint32_t i = 0; i < used; ++i) {
		const uint32_t b = i % d.lb;
		const uint32_t g = (i / d.lb) % d.lg;
		const uint32_t r = i / (static_cast<uint32_t>(d.lb) * d.lg);
		out[i] = (level_value(r, d.lr) << 16) | (level_value(g, d.lg) << 8) |
			level_value(b, d.lb);
	}
	return used;
}

struct Converter::State {
	struct {
		uint32_t sw = 0, sh = 0, dw = 0, dh = 0;
		bool nearest = false, valid = false;
		HPlan luma, chroma;
	} plan;
	struct {
		uint32_t format = 0, depth = 0;
		bool dither = false, host = false, valid = false;
		uint16_t table[3][17][256];
	} quant;
	struct {
		uint8_t pal[palette_bytes];
		uint32_t depth = 0;
		bool valid = false;
		uint8_t lut[32768];
	} inverse;
	struct {
		uint8_t vy[max_w];
		uint8_t vcb[max_chroma_w];
		uint8_t vcr[max_chroma_w];
		uint8_t hy[max_w];
		uint8_t hcb[max_w];
		uint8_t hcr[max_w];
		uint8_t index[max_w + 8];
		uint32_t acc[max_w];
	} row;

	void ensure_plan(uint32_t sw, uint32_t sh, uint32_t dw, uint32_t dh,
	                 bool nearest);
	const uint8_t *vertical_row(const uint8_t *src, uint32_t pitch,
	                            uint32_t width, uint32_t sl_height, uint32_t dh,
	                            uint32_t y, bool chroma, bool nearest,
	                            uint8_t *tmp);
	void build_quant(uint32_t format, uint32_t depth, bool dither, bool host);
	void build_inverse(const uint8_t *palette, uint32_t depth);
	void convert_row(RowKind kind, bool dither, uint32_t y, uint32_t w,
	                 const uint8_t *yl, const uint8_t *cbl, const uint8_t *crl,
	                 uint8_t *out);
};

void Converter::State::ensure_plan(uint32_t sw, uint32_t sh, uint32_t dw,
                                   uint32_t dh, bool nearest)
{
	if (plan.valid && plan.sw == sw && plan.sh == sh && plan.dw == dw &&
	    plan.dh == dh && plan.nearest == nearest)
		return;
	build_hplan(plan.luma, sw, dw, false, nearest);
	build_hplan(plan.chroma, sw, dw, true, nearest);
	plan.sw = sw;
	plan.sh = sh;
	plan.dw = dw;
	plan.dh = dh;
	plan.nearest = nearest;
	plan.valid = true;
}

// The row of a plane that output row y reads: a source row when it needs no
// blending, else `tmp` holding the vertical pass.
const uint8_t *Converter::State::vertical_row(
	const uint8_t *src, uint32_t pitch, uint32_t width, uint32_t sl_height,
	uint32_t dh, uint32_t y, bool chroma, bool nearest, uint8_t *tmp)
{
	const uint32_t ph = plane_size(sl_height, chroma);
	if (use_area(sl_height, dh, nearest)) {
		const uint32_t s0 = y * ph, s1 = s0 + ph;
		const uint32_t j0 = s0 / dh;
		uint32_t j1 = (s1 + dh - 1) / dh;
		const uint32_t r24 = reciprocal24(ph);
		if (j1 > ph)
			j1 = ph;
		std::memset(row.acc, 0, width * sizeof(row.acc[0]));
		for (uint32_t j = j0; j < j1; ++j) {
			const uint32_t lo = j * dh > s0 ? j * dh : s0;
			const uint32_t hi = (j + 1) * dh < s1 ? (j + 1) * dh : s1;
			const uint32_t w = hi - lo;
			const uint8_t *r = src + static_cast<size_t>(j) * pitch;
			for (uint32_t x = 0; x < width; ++x)
				row.acc[x] += w * r[x];
		}
		for (uint32_t x = 0; x < width; ++x)
			tmp[x] = area_round(row.acc[x], r24);
		return tmp;
	}
	Tap2 t;
	tap_at(t, y, sl_height, dh, chroma, nearest);
	const uint8_t *a = src + static_cast<size_t>(t.i0) * pitch;
	if (t.w == 0)
		return a;
	const uint8_t *b = src + static_cast<size_t>(t.i1) * pitch;
	for (uint32_t x = 0; x < width; ++x)
		tmp[x] = static_cast<uint8_t>((a[x] * (256u - t.w) + b[x] * t.w + 128u) >> 8);
	return tmp;
}

namespace {
void horizontal_row(const HPlan &p, const uint8_t *src, uint8_t *dst, uint32_t dw)
{
	if (p.area) {
		const uint16_t *weight = p.weight;
		for (uint32_t x = 0; x < dw; ++x) {
			const uint8_t *s = src + p.first[x];
			const uint32_t n = p.count[x];
			uint32_t sum = 0;
			for (uint32_t k = 0; k < n; ++k)
				sum += static_cast<uint32_t>(weight[k]) * s[k];
			weight += n;
			dst[x] = area_round(sum, p.r24);
		}
		return;
	}
	for (uint32_t x = 0; x < dw; ++x) {
		const Tap2 &t = p.tap[x];
		const uint32_t a = src[t.i0], b = src[t.i1];
		dst[x] = static_cast<uint8_t>((a * (256u - t.w) + b * t.w + 128u) >> 8);
	}
}

// Splits `w` indices (already padded with zeros to a multiple of 8) into
// `depth` plane bytes per eight pixels, leftmost pixel in the top bit.
void pack_planes(const uint8_t *index, uint32_t w, uint32_t depth,
                 uint8_t *const plane[8])
{
	const uint32_t groups = (w + 7) / 8;
	for (uint32_t g = 0; g < groups; ++g) {
		const uint8_t *s = index + 8 * g;
		uint32_t x = (static_cast<uint32_t>(s[0]) << 24) |
			(static_cast<uint32_t>(s[1]) << 16) |
			(static_cast<uint32_t>(s[2]) << 8) | s[3];
		uint32_t y = (static_cast<uint32_t>(s[4]) << 24) |
			(static_cast<uint32_t>(s[5]) << 16) |
			(static_cast<uint32_t>(s[6]) << 8) | s[7];
		uint32_t t;
		// 8x8 bit-matrix transpose (Hacker's Delight 7-3): afterwards
		// byte 7 - p of the pair holds bit p of every index.
		t = (x ^ (x >> 7)) & 0x00aa00aau;
		x = x ^ t ^ (t << 7);
		t = (y ^ (y >> 7)) & 0x00aa00aau;
		y = y ^ t ^ (t << 7);
		t = (x ^ (x >> 14)) & 0x0000ccccu;
		x = x ^ t ^ (t << 14);
		t = (y ^ (y >> 14)) & 0x0000ccccu;
		y = y ^ t ^ (t << 14);
		t = (x & 0xf0f0f0f0u) | ((y >> 4) & 0x0f0f0f0fu);
		y = ((x << 4) & 0xf0f0f0f0u) | (y & 0x0f0f0f0fu);
		x = t;
		for (uint32_t p = 0; p < depth; ++p) {
			const uint32_t b = 7 - p;
			const uint32_t word = b < 4 ? x : y;
			plane[p][g] = static_cast<uint8_t>(word >> (8 * (3 - (b & 3))));
		}
	}
}
} // namespace

void Converter::State::build_quant(uint32_t format, uint32_t depth, bool dither,
                                   bool host)
{
	static const uint32_t shift565[3] = { 11, 5, 0 };
	static const uint32_t shift555[3] = { 10, 5, 0 };
	static const uint32_t shift_cell[3] = { 10, 5, 0 };
	if (quant.valid && quant.format == format && quant.depth == depth &&
	    quant.dither == dither && quant.host == host)
		return;
	for (uint32_t ch = 0; ch < 3; ++ch) {
		for (uint32_t r = 0; r < 17; ++r) {
			for (uint32_t v = 0; v < 256; ++v) {
				uint32_t entry;
				if (format == surface_rgb565) {
					entry = level_for(v, ch == 1 ? 64 : 32, r) << shift565[ch];
				} else if (format == surface_rgb555) {
					entry = level_for(v, 32, r) << shift555[ch];
				} else if (host) {
					const int32_t off = r < 16 ?
						((2 * static_cast<int32_t>(r) - 15) *
						 static_cast<int32_t>(host_step[depth])) / 32 : 0;
					int32_t w = static_cast<int32_t>(v) + off;
					w = w < 0 ? 0 : w > 255 ? 255 : w;
					const uint32_t cell =
						(static_cast<uint32_t>(w) * 31 + 127) / 255;
					entry = cell << shift_cell[ch];
				} else {
					const FixedDims &d = fixed_dims[depth];
					if (d.gray) {
						entry = ch == 1 ? level_for(v, 1u << depth, r) : 0;
					} else {
						const uint32_t levels = ch == 0 ? d.lr :
							ch == 1 ? d.lg : d.lb;
						const uint32_t mult = ch == 0 ?
							static_cast<uint32_t>(d.lg) * d.lb :
							ch == 1 ? d.lb : 1;
						entry = level_for(v, levels, r) * mult;
					}
				}
				quant.table[ch][r][v] = static_cast<uint16_t>(entry);
			}
		}
	}
	quant.format = format;
	quant.depth = depth;
	quant.dither = dither;
	quant.host = host;
	quant.valid = true;
}

// Nearest palette entry (squared RGB distance, lowest index on a tie) for
// every 5-5-5 cell, measured at the cell's replicated centre colour.
void Converter::State::build_inverse(const uint8_t *palette, uint32_t depth)
{
	const uint32_t entries = depth >= 8 ? 256 : 1u << depth;
	if (inverse.valid && inverse.depth == depth &&
	    std::memcmp(inverse.pal, palette, entries * 4) == 0)
		return;
	uint8_t cc[32];
	for (uint32_t i = 0; i < 32; ++i)
		cc[i] = static_cast<uint8_t>((i << 3) | (i >> 2));
	for (uint32_t rg = 0; rg < 1024; ++rg) {
		uint32_t best_d[32];
		uint8_t best[32];
		for (uint32_t b = 0; b < 32; ++b) {
			best_d[b] = 0xffffffffu;
			best[b] = 0;
		}
		for (uint32_t p = 0; p < entries; ++p) {
			const int32_t dr = static_cast<int32_t>(cc[rg >> 5]) - palette[p * 4 + 1];
			const int32_t dg = static_cast<int32_t>(cc[rg & 31]) - palette[p * 4 + 2];
			const int32_t pb = palette[p * 4 + 3];
			const uint32_t base = static_cast<uint32_t>(dr * dr + dg * dg);
			for (uint32_t b = 0; b < 32; ++b) {
				const int32_t db = static_cast<int32_t>(cc[b]) - pb;
				const uint32_t d = base + static_cast<uint32_t>(db * db);
				if (d < best_d[b]) {
					best_d[b] = d;
					best[b] = static_cast<uint8_t>(p);
				}
			}
		}
		std::memcpy(&inverse.lut[rg * 32], best, 32);
	}
	std::memcpy(inverse.pal, palette, entries * 4);
	inverse.depth = depth;
	inverse.valid = true;
}

void Converter::State::convert_row(RowKind kind, bool dither, uint32_t y,
                                   uint32_t w, const uint8_t *yl,
                                   const uint8_t *cbl, const uint8_t *crl,
                                   uint8_t *out)
{
	const uint16_t *tr[4], *tg[4], *tb[4];
	for (uint32_t k = 0; k < 4; ++k) {
		const uint32_t t = dither ? bayer4[y & 3][k] : 16;
		tr[k] = quant.table[0][t];
		tg[k] = quant.table[1][t];
		tb[k] = quant.table[2][t];
	}
	for (uint32_t x = 0; x < w; ++x) {
		const uint32_t k = x & 3;
		int32_t r, g, b;
		yuv_to_rgb(yl[x], cbl[x], crl[x], r, g, b);
		switch (kind) {
		case RowKind::cube:
			out[x] = static_cast<uint8_t>(tr[k][r] + tg[k][g] + tb[k][b]);
			break;
		case RowKind::gray:
			out[x] = static_cast<uint8_t>(tg[k][(77 * r + 150 * g + 29 * b + 128) >> 8]);
			break;
		case RowKind::host:
			out[x] = inverse.lut[tr[k][r] + tg[k][g] + tb[k][b]];
			break;
		case RowKind::rgb: {
			const uint32_t v = static_cast<uint32_t>(tr[k][r] + tg[k][g] + tb[k][b]);
			out[2 * x] = static_cast<uint8_t>(v >> 8);
			out[2 * x + 1] = static_cast<uint8_t>(v);
			break;
		}
		}
	}
}

Converter::Converter() : state_(new State) {}
Converter::~Converter() = default;

uint16_t Converter::convert(const Job &job, const Frame &frame, uint8_t *dst,
                            const uint8_t *palette)
{
	if (!dst)
		return bad_request;
	const uint32_t sw = frame.width, sh = frame.height;
	const uint32_t dw = job.width, dh = job.height;
	const uint32_t cw = (sw + 1) / 2;
	if (sw == 0 || sh == 0 || sw > max_source_width || dw == 0 || dh == 0 ||
	    dw > max_width || dh > max_height || !frame.y || !frame.cb || !frame.cr ||
	    frame.y_pitch < sw || frame.chroma_pitch < cw)
		return bad_request;

	const bool nearest = (job.flags & flag_scale_nearest) != 0;
	const bool dither = (job.flags & flag_dither_ordered) != 0;
	bool host = (job.flags & flag_palette_host) != 0;
	uint32_t depth = job.depth;
	const bool planar = job.format == surface_planar;
	const bool interleaved = (job.flags & flag_planar_interleaved) != 0;
	RowKind kind;
	if (job.format == surface_index8 || planar) {
		if (depth < 1 || depth > 8 || (host && !palette))
			return bad_request;
		kind = host ? RowKind::host :
			fixed_dims[depth].gray ? RowKind::gray : RowKind::cube;
	} else if (job.format == surface_rgb565 || job.format == surface_rgb555) {
		host = false;
		depth = 0;
		kind = RowKind::rgb;
	} else {
		return bad_request;
	}

	State &s = *state_;
	s.ensure_plan(sw, sh, dw, dh, nearest);
	s.build_quant(job.format, depth, dither, host);
	if (host)
		s.build_inverse(palette, depth);

	std::memset(&s.row.index[dw], 0, 8);
	for (uint32_t y = 0; y < dh; ++y) {
		const uint8_t *vy = s.vertical_row(frame.y, frame.y_pitch, sw, sh, dh, y,
			false, nearest, s.row.vy);
		const uint8_t *vcb = s.vertical_row(frame.cb, frame.chroma_pitch, cw, sh,
			dh, y, true, nearest, s.row.vcb);
		const uint8_t *vcr = s.vertical_row(frame.cr, frame.chroma_pitch, cw, sh,
			dh, y, true, nearest, s.row.vcr);
		const uint8_t *yl = vy;
		if (!s.plan.luma.identity) {
			horizontal_row(s.plan.luma, vy, s.row.hy, dw);
			yl = s.row.hy;
		}
		horizontal_row(s.plan.chroma, vcb, s.row.hcb, dw);
		horizontal_row(s.plan.chroma, vcr, s.row.hcr, dw);
		if (planar) {
			uint8_t *plane[8];
			s.convert_row(kind, dither, y, dw, yl, s.row.hcb, s.row.hcr,
				s.row.index);
			for (uint32_t p = 0; p < depth; ++p)
				plane[p] = dst + (interleaved ?
					static_cast<size_t>(y) * depth + p :
					static_cast<size_t>(p) * job.height + y) * job.pitch;
			pack_planes(s.row.index, dw, depth, plane);
		} else {
			s.convert_row(kind, dither, y, dw, yl, s.row.hcb, s.row.hcr,
				dst + static_cast<size_t>(y) * job.pitch);
		}
	}
	return ok;
}

} // namespace media_output
} // namespace zz9000_sdk
