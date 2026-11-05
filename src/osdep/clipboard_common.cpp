#include "sysconfig.h"
#include "sysdeps.h"

#include "clipboard_common.h"
#include "traps.h"
#include "native2amiga_api.h"
#include <algorithm>
#include <stdint.h>
#include <limits.h>

int clipboard_debug;
int clipboard_log;

void clipboard_debug_dump (TrapContext *ctx, const TCHAR *name, uaecptr p, int size)
{
	FILE *f;
	int cnt;

	if (!p || !size)
		return;
	cnt = 0;
	for (;;) {
		TCHAR tmp[MAX_DPATH];
		_stprintf (tmp, _T("%s.%03d.dat"), name, cnt);
		f = uae_tfopen (tmp, _T("rb"));
		if (f) {
			fclose (f);
			cnt++;
			continue;
		}
		f = uae_tfopen (tmp, _T("wb"));
		if (f) {
			uae_u8 *pd = xmalloc(uae_u8, size);
			trap_get_bytes(ctx, pd, p, size);
			fwrite (pd, size, 1, f);
			fclose (f);
			xfree(pd);
		}
		return;
	}
}

static std::string host_text_to_amiga_text(const std::string &text)
{
	std::string converted;
	converted.reserve(text.size());
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] != '\r') {
			converted.push_back(text[i]);
		}
	}
	return converted;
}

static void append_be32(std::vector<uae_u8> *out, uae_u32 value)
{
	out->push_back(value >> 24);
	out->push_back(value >> 16);
	out->push_back(value >> 8);
	out->push_back(value);
}

static void append_be16(std::vector<uae_u8> *out, uae_u16 value)
{
	out->push_back(value >> 8);
	out->push_back(value);
}

static uae_u16 read_be16(const uae_u8 *p)
{
	return ((uae_u16)p[0] << 8) | p[1];
}

static uae_u32 read_be32(const uae_u8 *p)
{
	return ((uae_u32)p[0] << 24) | ((uae_u32)p[1] << 16) | ((uae_u32)p[2] << 8) | p[3];
}

static void append_literal(std::vector<uae_u8> *out, const char *literal)
{
	for (int i = 0; i < 4; i++) {
		out->push_back((uae_u8)literal[i]);
	}
}

std::vector<uae_u8> clipboard_encode_text(const std::string &host_text)
{
	const std::string amiga_text = host_text_to_amiga_text(host_text);
	std::vector<uae_u8> iff;
	const uae_u32 text_size = (uae_u32)amiga_text.size();
	const uae_u32 form_size = 4 + 8 + text_size + (text_size & 1);

	iff.reserve(form_size + 8);
	append_literal(&iff, "FORM");
	append_be32(&iff, form_size);
	append_literal(&iff, "FTXT");
	append_literal(&iff, "CHRS");
	append_be32(&iff, text_size);
	iff.insert(iff.end(), amiga_text.begin(), amiga_text.end());
	if (text_size & 1) {
		iff.push_back(0);
	}
	return iff;
}

bool clipboard_encode_bitmap(const clipboard_bitmap &bitmap, bool initial, std::vector<uae_u8> *iff)
{
	if (!iff || bitmap.width <= 0 || bitmap.height <= 0 ||
		bitmap.pixels.size() < (size_t)bitmap.width * (size_t)bitmap.height) {
		return false;
	}

	const size_t pixel_bytes = (size_t)bitmap.width * (size_t)bitmap.height * 4;
	if (pixel_bytes > (initial ? CLIP_SIZE_LIMIT_INIT : CLIP_SIZE_LIMIT)) {
		return false;
	}

	std::vector<uae_u32> colors;
	colors.reserve(256);
	for (uae_u32 pixel : bitmap.pixels) {
		size_t i = 0;
		for (; i < colors.size(); i++) {
			if (colors[i] == pixel) {
				break;
			}
		}
		if (i == colors.size()) {
			if (colors.size() >= 256) {
				colors.clear();
				break;
			}
			colors.push_back(pixel);
		}
	}

	int planes = 24;
	if (colors.size() >= 256) {
		colors.clear();
	}
	if (!colors.empty()) {
		planes = 0;
		for (size_t count = colors.size(); count > 0; count >>= 1) {
			planes++;
		}
		if (planes < 1) {
			planes = 1;
		}
	}

	const int width = bitmap.width;
	const int height = bitmap.height;
	const int plane_rowbytes = ((width + 15) & ~15) / 8;
	const uae_u32 body_size = (uae_u32)(plane_rowbytes * height * planes);

	iff->clear();
	append_literal(iff, "FORM");
	append_be32(iff, 0);
	append_literal(iff, "ILBM");
	append_literal(iff, "BMHD");
	append_be32(iff, 20);
	append_be16(iff, (uae_u16)width);
	append_be16(iff, (uae_u16)height);
	append_be16(iff, 0);
	append_be16(iff, 0);
	iff->push_back((uae_u8)planes);
	iff->push_back(0);
	iff->push_back(0);
	iff->push_back(0);
	append_be16(iff, 0);
	iff->push_back(1);
	iff->push_back(1);
	append_be16(iff, (uae_u16)width);
	append_be16(iff, (uae_u16)height);
	append_literal(iff, "CAMG");
	append_be32(iff, 4);
	uae_u32 camg = 0;
	if (width > 400) {
		camg |= 0x8000;
	}
	if (height > 300) {
		camg |= 0x0004;
	}
	append_be32(iff, camg);

	if (planes <= 8) {
		const int color_count = 1 << planes;
		append_literal(iff, "CMAP");
		append_be32(iff, color_count * 3);
		for (int i = 0; i < color_count; i++) {
			const uae_u32 color = i < (int)colors.size() ? colors[i] : 0;
			iff->push_back(color >> 16);
			iff->push_back(color >> 8);
			iff->push_back(color);
		}
		if ((color_count * 3) & 1) {
			iff->push_back(0);
		}
	}

	append_literal(iff, "BODY");
	append_be32(iff, body_size);
	const size_t body_offset = iff->size();
	iff->resize(body_offset + body_size, 0);
	uae_u8 *body = iff->data() + body_offset;

	for (int y = 0; y < height; y++) {
		if (planes <= 8) {
			for (int b = 0; b < planes; b++) {
				for (int x = 0; x < width; x++) {
					const uae_u32 pixel = bitmap.pixels[(size_t)y * (size_t)width + x];
					int color_index = 0;
					for (int i = 0; i < (int)colors.size(); i++) {
						if (colors[i] == pixel) {
							color_index = i;
							break;
						}
					}
					if (color_index & (1 << b)) {
						body[x / 8] |= 1 << (7 - (x & 7));
					}
				}
				body += plane_rowbytes;
			}
		} else {
			for (int component = 0; component < 3; component++) {
				for (int b = 0; b < 8; b++) {
					const int mask = 1 << (((2 - component) * 8) + b);
					for (int x = 0; x < width; x++) {
						const uae_u32 pixel = bitmap.pixels[(size_t)y * (size_t)width + x];
						if (pixel & mask) {
							body[x / 8] |= 1 << (7 - (x & 7));
						}
					}
					body += plane_rowbytes;
				}
			}
		}
	}

	const uae_u32 form_size = (uae_u32)iff->size() - 8;
	(*iff)[4] = form_size >> 24;
	(*iff)[5] = form_size >> 16;
	(*iff)[6] = form_size >> 8;
	(*iff)[7] = form_size;
	if (form_size & 1) {
		iff->push_back(0);
	}
	return true;
}

static int parse_csi(const std::string &text, size_t offset, bool crlf)
{
	while (offset < text.size()) {
		if ((crlf ? (int)(char)text[offset] : (int)(uae_u8)text[offset]) >= 0x40) {
			break;
		}
		offset++;
	}
	return (int)offset;
}

static std::string amiga_text_to_host_text(const std::string &text, bool crlf)
{
	if (crlf && text.find('\r') != std::string::npos) {
		return std::string(text.c_str());
	}
	std::string converted;
	converted.reserve(text.size());
	for (size_t i = 0; i < text.size(); i++) {
		if (crlf && text[i] == 0) {
			break;
		}
		uae_u8 c = (uae_u8)text[i];
		if (c == 0 && i + 1 < text.size()) {
			continue;
		}
		// Preserve the Windows ANSI conversion's signed-char CSI handling.
		if (c == 0x9b && (!crlf || CHAR_MIN == 0)) {
			i = parse_csi(text, i + 1, crlf);
			continue;
		}
		if (c == 0x1b && i + 1 < text.size() && text[i + 1] == '[') {
			i = parse_csi(text, i + 2, crlf);
			continue;
		}
		if (c == '\r') {
			converted.push_back('\n');
			if (i + 1 < text.size() && text[i + 1] == '\n') {
				i++;
			}
			continue;
		}
		if (crlf && c == '\n') {
			converted.push_back('\r');
		}
		converted.push_back((char)c);
	}
	return converted;
}

bool clipboard_decode_text(const uae_u8 *addr, uae_u32 len, std::string *out, bool crlf)
{
	if (len < 12 || memcmp(addr, "FORM", 4) || memcmp(addr + 8, "FTXT", 4)) {
		return false;
	}

	std::string text;
	uae_u32 offset = 12;
	while (offset + 8 <= len) {
		const uae_u8 *chunk = addr + offset;
		uae_u32 csize = ((uae_u32)chunk[4] << 24) | ((uae_u32)chunk[5] << 16) | ((uae_u32)chunk[6] << 8) | chunk[7];
		offset += 8;
		if (csize > len - offset) {
			break;
		}
		if (!memcmp(chunk, "CHRS", 4)) {
			text.append((const char *)(addr + offset), csize);
		}
		offset += csize + (csize & 1);
		// Some Windows clipboard producers put CRLF outside the CHRS size.
		if (crlf && csize && offset < len) {
			if ((addr[offset - 2] == 13 && addr[offset - 1] == 10 && addr[offset] == 0) ||
				(addr[offset - 1] == 13 && addr[offset] == 10)) {
				offset++;
			}
		}
	}

	if (crlf) {
		text.resize(strlen(text.c_str()));
	}
	*out = amiga_text_to_host_text(text, crlf);
	return true;
}

static bool decompress_byterun1(const uae_u8 *src, const uae_u8 *end, std::vector<uae_u8> *out, size_t expected)
{
	out->clear();
	out->reserve(expected);
	while (src < end && out->size() < expected) {
		const int8_t control = (int8_t)*src++;
		if (control >= 0) {
			const int count = control + 1;
			if ((size_t)(end - src) < (size_t)count) {
				return false;
			}
			const size_t writable = std::min((size_t)count, expected - out->size());
			out->insert(out->end(), src, src + writable);
			src += count;
		} else if (control >= -127) {
			if (src >= end) {
				return false;
			}
			const int count = -control + 1;
			const uae_u8 value = *src++;
			const size_t writable = std::min((size_t)count, expected - out->size());
			out->insert(out->end(), writable, value);
		}
	}
	return out->size() == expected;
}

bool clipboard_decode_bitmap(const uae_u8 *addr, uae_u32 len, clipboard_bitmap *bitmap, bool preserve_ham_low_bits, bool allow_reduced_rgb)
{
	if (!bitmap || len < 12 || memcmp(addr, "FORM", 4) || memcmp(addr + 8, "ILBM", 4)) {
		return false;
	}
	if (read_be32(addr + 4) > 0xffffff) {
		return false;
	}

	int width = 0;
	int height = 0;
	int planes = 0;
	int masking = 0;
	int compression = 0;
	uae_u32 camg = 0;
	uae_u32 palette[256];
	for (int i = 0; i < 256; i++) {
		palette[i] = ((uae_u32)i << 16) | ((uae_u32)i << 8) | i;
	}
	bool have_bmhd = false;
	const uae_u8 *body = NULL;
	uae_u32 body_size = 0;

	uae_u32 offset = 12;
	while (offset + 8 <= len) {
		const uae_u8 *chunk = addr + offset;
		const uae_u32 csize = read_be32(chunk + 4);
		offset += 8;
		if (csize > len - offset) {
			return false;
		}
		const uae_u8 *data = addr + offset;
		if (!memcmp(chunk, "BMHD", 4) && csize >= 20) {
			width = read_be16(data);
			height = read_be16(data + 2);
			planes = data[8];
			masking = data[9];
			compression = data[10];
			have_bmhd = true;
		} else if (!memcmp(chunk, "CAMG", 4) && csize >= 4) {
			camg = read_be32(data);
			if ((camg & 0xffff0000) && !(camg & 0x1000)) {
				camg = 0;
			}
		} else if (!memcmp(chunk, "CMAP", 4)) {
			const int colors = std::min<int>(256, csize / 3);
			bool zero4 = true;
			for (int i = 0; i < colors; i++) {
				const uae_u8 r = data[(size_t)i * 3 + 0];
				const uae_u8 g = data[(size_t)i * 3 + 1];
				const uae_u8 b = data[(size_t)i * 3 + 2];
				if ((r & 0x0f) || (g & 0x0f) || (b & 0x0f)) {
					zero4 = false;
				}
				palette[i] = ((uae_u32)r << 16) | ((uae_u32)g << 8) | b;
			}
			if (zero4) {
				for (int i = 0; i < colors; i++) {
					const uae_u8 r = ((palette[i] >> 16) & 0xff) | ((palette[i] >> 20) & 0x0f);
					const uae_u8 g = ((palette[i] >> 8) & 0xff) | ((palette[i] >> 12) & 0x0f);
					const uae_u8 b = (palette[i] & 0xff) | ((palette[i] >> 4) & 0x0f);
					palette[i] = ((uae_u32)r << 16) | ((uae_u32)g << 8) | b;
				}
			}
		} else if (!memcmp(chunk, "BODY", 4)) {
			body = data;
			body_size = csize;
		}
		offset += csize + (csize & 1);
	}

	if (!have_bmhd || !body || width <= 0 || height <= 0 || planes <= 0 || planes > 24) {
		return false;
	}
	if (!allow_reduced_rgb && planes > 8 && planes != 24 && !((camg & 0x0800) && planes > 4)) {
		return false;
	}
	// BMHD dimensions come from the guest: refuse anything the encoder would
	// refuse too, before allocating the decode buffers.
	if ((uae_u64)width * (uae_u64)height * 4 > CLIP_SIZE_LIMIT) {
		return false;
	}

	const int plane_rowbytes = ((width + 15) & ~15) / 8;
	const int stored_planes = planes + (masking == 1 ? 1 : 0);
	const size_t decoded_size = (size_t)plane_rowbytes * (size_t)height * (size_t)stored_planes;
	std::vector<uae_u8> decoded;
	if (compression) {
		if (!decompress_byterun1(body, body + body_size, &decoded, decoded_size)) {
			return false;
		}
		body = decoded.data();
		body_size = (uae_u32)decoded.size();
	} else if (body_size < decoded_size) {
		return false;
	}

	const bool ham = (camg & 0x0800) && planes > 4;
	const bool ehb = !(camg & (0x0800 | 0x0400 | 0x8000 | 0x0040)) && (camg & 0x0080) && planes == 6;
	bitmap->width = width;
	bitmap->height = height;
	bitmap->pixels.assign((size_t)width * (size_t)height, 0);

	for (int y = 0; y < height; y++) {
		const uae_u8 *row = body + (size_t)y * (size_t)stored_planes * (size_t)plane_rowbytes;
		uae_u32 ham_lastcolor = 0;
		for (int x = 0; x < width; x++) {
			int value = 0;
			for (int b = 0; b < planes; b++) {
				const int off = x / 8;
				const int mask = 1 << (7 - (x & 7));
				if (row[(size_t)b * plane_rowbytes + off] & mask) {
					value |= 1 << b;
				}
			}
			uae_u32 pixel = 0;
			if (ham) {
				if (planes >= 7) {
					const uae_u32 c = value & 0x3f;
					switch (value >> 6)
					{
						case 0: ham_lastcolor = palette[value & 0x3f]; break;
						case 1: ham_lastcolor = (ham_lastcolor & (preserve_ham_low_bits ? 0xffff03 : 0xffff00)) | (c << 2); break;
						case 2: ham_lastcolor = (ham_lastcolor & (preserve_ham_low_bits ? 0x03ffff : 0x00ffff)) | (c << 18); break;
						case 3: ham_lastcolor = (ham_lastcolor & (preserve_ham_low_bits ? 0xff03ff : 0xff00ff)) | (c << 10); break;
					}
				} else {
					const uae_u32 c = value & 0x0f;
					switch (value >> 4)
					{
						case 0: ham_lastcolor = palette[value & 0x0f]; break;
						case 1: ham_lastcolor = (ham_lastcolor & 0xffff00) | (c << 4); break;
						case 2: ham_lastcolor = (ham_lastcolor & 0x00ffff) | (c << 20); break;
						case 3: ham_lastcolor = (ham_lastcolor & 0xff00ff) | (c << 12); break;
					}
				}
				pixel = ham_lastcolor;
			} else if (planes <= 8) {
				if (ehb && value >= 32) {
					const uae_u32 base = palette[value - 32];
					pixel = (((base >> 17) & 0x7f) << 16) | (((base >> 9) & 0x7f) << 8) | ((base >> 1) & 0x7f);
				} else {
					pixel = palette[value & 0xff];
				}
			} else {
				uae_u8 r = 0, g = 0, b = 0;
				for (int bit = 0; bit < planes / 3; bit++) {
					const int off = x / 8;
					const int mask = 1 << (7 - (x & 7));
					if (row[(size_t)bit * plane_rowbytes + off] & mask) {
						r |= 1 << bit;
					}
					if (row[(size_t)(planes / 3 + bit) * plane_rowbytes + off] & mask) {
						g |= 1 << bit;
					}
					if (row[(size_t)(2 * (planes / 3) + bit) * plane_rowbytes + off] & mask) {
						b |= 1 << bit;
					}
				}
				pixel = ((uae_u32)r << 16) | ((uae_u32)g << 8) | b;
			}
			bitmap->pixels[(size_t)y * (size_t)width + x] = pixel;
		}
	}
	return true;
}

void clipboard_transfer::start()
{
	phase = 0;
	if (initialized && guest_data && !pending.empty()) {
		phase = 1;
	}
}

uae_u32 clipboard_transfer::announce(TrapContext *ctx, void *user)
{
	clipboard_transfer *t = (clipboard_transfer*)user;
	if (!t->guest_data || t->pending.empty() || trap_get_long(ctx, t->guest_data)) {
		return 0;
	}
	if (clipboard_debug) {
		clipboard_debug_dump(ctx, _T("clipboard_p2a"), t->guest_data, (int)t->pending.size());
	}
	if (clipboard_log) {
		write_log(_T("clipboard: to_amiga %08x %d\n"), t->guest_data, (int)t->pending.size());
	}
	trap_put_long(ctx, t->guest_data, (uae_u32)t->pending.size());
	uae_Signal(trap_get_long(ctx, t->guest_data + 8), 1 << 13);
	t->phase = 2;
	return 1;
}

uae_u32 clipboard_transfer::signal(TrapContext *ctx, void *user)
{
	clipboard_transfer *t = (clipboard_transfer*)user;
	if (t->guest_data) {
		uaecptr task = trap_get_long(ctx, t->guest_data + 8);
		if (task && native2amiga_isfree()) {
			uae_Signal(task, 1 << 13);
			if (clipboard_log) {
				write_log(_T("clipboard: signal %08x\n"), t->guest_data);
			}
		}
	}
	return 0;
}

void clipboard_transfer::vsync(bool defer_while_signaling)
{
	if (!guest_data) {
		return;
	}
	if (signaling) {
		delay--;
		if (delay <= 0) {
			trap_callback(signal, this);
			delay = 50;
		} else if (defer_while_signaling) {
			return;
		}
	}
	if (unsafe_delay > 0) {
		unsafe_delay--;
	}
	if (phase == 1 && unsafe_delay <= 0) {
		trap_callback(announce, this);
	}
}

int clipboard_transfer::want_data(TrapContext *ctx)
{
	if (!guest_data) {
		return 0;
	}
	uaecptr addr = trap_get_long(ctx, guest_data + 4);
	uae_u32 size = trap_get_long(ctx, guest_data);
	int result = 0;
	if (!initialized || pending.empty()) {
		write_log(_T("clipboard: want_data() without pending data\n"));
	} else if (size != pending.size()) {
		write_log(_T("clipboard: size %d <> %d mismatch\n"), size, (int)pending.size());
	} else {
		if (addr && size) {
			trap_put_bytes(ctx, pending.data(), addr, size);
		}
		if (clipboard_log) {
			write_log(_T("clipboard: ->amiga, %08x, %08x %d bytes\n"), guest_data, addr, size);
		}
		result = 1;
	}
	pending.clear();
	phase = 0;
	return result;
}

void clipboard_transfer::unsafe_period()
{
	unsafe_delay = 100;
	if (delay < 60) {
		delay = 60;
	}
}

void clipboard_transfer::reset()
{
	unsafe_period();
	guest_data = 0;
	signaling = false;
	initialized = false;
	pending.clear();
	phase = 0;
}
