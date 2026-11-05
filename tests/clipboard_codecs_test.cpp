// Clipboard sharing: the IFF codecs and clipboard.device handshake shared
// with WinUAE (src/osdep/clipboard_common.cpp, tests ported from WinUAE's
// od-generic/clipboard_test.cpp), the host image formats
// (src/osdep/clipboard_image.cpp) and the hand-over between host and guest
// (src/osdep/clipboard.cpp).

#include "sysconfig.h"
#include "sysdeps.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "clipboard.h"
#include "clipboard_common.h"
#include "clipboard_image.h"
#include "traps.h"
#include "native2amiga_api.h"

void target_paste_to_keyboard(void);

static int failures;
static uae_u8 guest[4096];
static int signals;
static bool signal_free = true;
static std::string injected;

static void check(bool ok, const char *name)
{
	if (!ok) {
		fprintf(stderr, "FAIL: %s\n", name);
		failures++;
	}
}

void write_log(const TCHAR *, ...) {}
int native2amiga_isfree(void) { return signal_free; }
void uae_Signal(uaecptr task, uae_u32 mask)
{
	check(task == 0x1234 && mask == (1 << 13), "clipboard task signal");
	signals++;
}
void trap_get_bytes(TrapContext *, void *p, uaecptr addr, int size)
{
	if (addr > sizeof guest || size < 0 || (size_t)size > sizeof guest - addr) {
		check(false, "guest read bounds");
		return;
	}
	memcpy(p, guest + addr, size);
}
void trap_put_bytes(TrapContext *, const void *p, uaecptr addr, int size)
{
	if (addr > sizeof guest || size < 0 || (size_t)size > sizeof guest - addr) {
		check(false, "guest write bounds");
		return;
	}
	memcpy(guest + addr, p, size);
}
uae_u32 trap_get_long(TrapContext *ctx, uaecptr addr)
{
	uae_u8 b[4];
	trap_get_bytes(ctx, b, addr, 4);
	return ((uae_u32)b[0] << 24) | ((uae_u32)b[1] << 16) | (b[2] << 8) | b[3];
}
void trap_put_long(TrapContext *ctx, uaecptr addr, uae_u32 n)
{
	uae_u8 b[] = { (uae_u8)(n >> 24), (uae_u8)(n >> 16), (uae_u8)(n >> 8), (uae_u8)n };
	trap_put_bytes(ctx, b, addr, 4);
}
void trap_callback(TRAP_CALLBACK callback, void *p) { callback(NULL, p); }
bool trap_valid_address(TrapContext *, uaecptr addr, uae_u32 size)
{
	return addr <= sizeof guest && size <= sizeof guest - addr;
}
bool filesys_heartbeat(void) { return true; }
void keybuf_inject(const uae_char *text) { injected = text; }

static void be32(std::vector<uae_u8> &v, uae_u32 n)
{
	v.push_back(n >> 24); v.push_back(n >> 16);
	v.push_back(n >> 8); v.push_back(n);
}
static void chunk(std::vector<uae_u8> &v, const char *id, const std::vector<uae_u8> &data)
{
	v.insert(v.end(), id, id + 4);
	be32(v, (uae_u32)data.size());
	v.insert(v.end(), data.begin(), data.end());
	if (data.size() & 1) v.push_back(0);
}
static void form_size(std::vector<uae_u8> &v)
{
	uae_u32 n = (uae_u32)v.size() - 8;
	v[4] = n >> 24; v[5] = n >> 16; v[6] = n >> 8; v[7] = n;
}

static void text_tests()
{
	std::vector<uae_u8> v = clipboard_encode_text("a\r\nb");
	const uae_u8 expected[] = { 'F','O','R','M',0,0,0,16,'F','T','X','T',
		'C','H','R','S',0,0,0,3,'a','\n','b',0 };
	check(v.size() == sizeof expected && !memcmp(v.data(), expected, sizeof expected), "FTXT encoding and odd padding");
	std::string text;
	check(clipboard_decode_text(v.data(), (uae_u32)v.size(), &text, false) && text == "a\nb", "LF text");
	check(clipboard_decode_text(v.data(), (uae_u32)v.size(), &text, true) && text == "a\r\nb", "CRLF text");
	v.resize(12);
	chunk(v, "JUNK", { 1 });
	chunk(v, "CHRS", { 'a', 0x1b, '[', '3', '1', 'm' });
	chunk(v, "CHRS", { 'b', '\r', '\n', 'c' });
	form_size(v);
	check(clipboard_decode_text(v.data(), (uae_u32)v.size(), &text, false) && text == "ab\nc", "multiple chunks and CSI");
	check(clipboard_decode_text(v.data(), (uae_u32)v.size(), &text, true) && text == "a\033[31mb\r\nc", "existing CR preserves Windows text");
	v.resize(12);
	chunk(v, "CHRS", { 'a', 0, 'b' });
	form_size(v);
	check(clipboard_decode_text(v.data(), (uae_u32)v.size(), &text, false) && text == "ab", "embedded NUL");
	for (uae_u32 i = 0; i < 12; i++)
		check(!clipboard_decode_text(v.data(), i, &text, false), "short text header");
	// A chunk header that runs past the end of the data is not read.
	v.resize(12);
	chunk(v, "CHRS", { 'x' });
	form_size(v);
	v.insert(v.end(), { 'C', 'H', 'R' });
	check(clipboard_decode_text(v.data(), (uae_u32)v.size(), &text, false) && text == "x", "partial trailing chunk header");
	v = clipboard_encode_text("");
	check(clipboard_decode_text(v.data(), (uae_u32)v.size(), &text, false) && text.empty(), "empty text");
}

static std::vector<uae_u8> planar(int planes, uae_u32 mode, int value, bool compressed, bool mask)
{
	std::vector<uae_u8> v = { 'F','O','R','M',0,0,0,0,'I','L','B','M' };
	std::vector<uae_u8> bmhd(20);
	bmhd[1] = 1; bmhd[3] = 1; bmhd[8] = planes;
	bmhd[9] = mask ? 1 : 0; bmhd[10] = compressed ? 1 : 0;
	chunk(v, "BMHD", bmhd);
	std::vector<uae_u8> camg; be32(camg, mode); chunk(v, "CAMG", camg);
	std::vector<uae_u8> cmap(256 * 3);
	cmap[0] = 0x12; cmap[1] = 0x34; cmap[2] = 0x56;
	cmap[3] = 0xab; cmap[4] = 0xcd; cmap[5] = 0xef;
	chunk(v, "CMAP", cmap);
	std::vector<uae_u8> body;
	for (int p = 0; p < planes + (mask ? 1 : 0); p++) {
		if (compressed) { body.push_back(128); body.push_back(1); }
		body.push_back((value & (1 << p)) ? 128 : 0);
		body.push_back(0);
	}
	chunk(v, "BODY", body); form_size(v);
	return v;
}

static void ilbm_tests()
{
	clipboard_bitmap src, dst;
	std::vector<uae_u8> v;
	for (int colors : { 1, 2, 255, 256, 257 }) {
		src.width = 257; src.height = 2;
		src.pixels.clear();
		for (int i = 0; i < src.width * src.height; i++) src.pixels.push_back(0x123400 + i % colors);
		check(clipboard_encode_bitmap(src, false, &v), "encode indexed/truecolor bitmap");
		check(clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst) &&
			dst.width == src.width && dst.height == src.height && dst.pixels == src.pixels,
			"bitmap roundtrip across palette cutoff");
	}
	for (bool compressed : { false, true }) {
		for (bool mask : { false, true }) {
			v = planar(2, 0, 1, compressed, mask);
			check(clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst) && dst.pixels[0] == 0xabcdef,
				"indexed bitmap, mask and ByteRun1");
		}
	}
	v = planar(6, 0x80, 33, true, false);
	check(clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst) && dst.pixels[0] == 0x556677, "EHB color");
	v = planar(6, 0x800, 0x1f, false, false);
	check(clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst) && dst.pixels[0] == 0xf0, "HAM6 blue");
	v = planar(8, 0x800, 0x7f, true, false);
	check(clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst) && dst.pixels[0] == 0xfc, "HAM8 blue");
	v = planar(24, 0, 0x563412, false, false);
	check(clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst) && dst.pixels[0] == 0x123456, "24-plane component order");
	v = planar(12, 0, 0x321, false, false);
	check(clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst, true, true) && dst.pixels[0] == 0x010203, "reduced RGB depth");
	check(!clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst), "strict RGB depth policy");
	v = planar(2, 0, 1, true, false);
	for (uae_u32 i = 0; i < v.size(); i++)
		check(!clipboard_decode_bitmap(v.data(), i, &dst), "truncated image");
	// A compressed BODY that claims 65535x65535 pixels is refused before
	// the decoder allocates for it.
	v = planar(8, 0, 1, true, false);
	v[20] = v[21] = v[22] = v[23] = 0xff;
	check(!clipboard_decode_bitmap(v.data(), (uae_u32)v.size(), &dst), "oversized BMHD dimensions");
	src.width = 100; src.height = 100; src.pixels.assign(10000, 0);
	check(!clipboard_encode_bitmap(src, true, &v), "initial bitmap size limit");
	check(clipboard_encode_bitmap(src, false, &v), "later bitmap size limit");
}

static void le16(std::vector<uae_u8> &v, uae_u32 n) { v.push_back(n); v.push_back(n >> 8); }
static void le32(std::vector<uae_u8> &v, uae_u32 n) { le16(v, n); le16(v, n >> 16); }

// A BMP file: info header of header_size bytes, optional colour masks
// written after the first 40 bytes of it, a palette, then rows as given.
static std::vector<uae_u8> bmp(int width, int height, int bpp, uae_u32 compression, uae_u32 header_size,
	const std::vector<uae_u32> &masks, const std::vector<uae_u32> &palette, const std::vector<std::vector<uae_u8>> &rows)
{
	std::vector<uae_u8> v = { 'B', 'M' };
	const uae_u32 extra_masks = header_size == 40 ? (uae_u32)masks.size() * 4 : 0;
	const uae_u32 offset = 14 + header_size + extra_masks + (uae_u32)palette.size() * 4;
	le32(v, 0);
	le32(v, 0);
	le32(v, offset);
	le32(v, header_size);
	le32(v, (uae_u32)width);
	le32(v, (uae_u32)height);
	le16(v, 1);
	le16(v, bpp);
	le32(v, compression);
	le32(v, 0);
	le32(v, 2835);
	le32(v, 2835);
	le32(v, (uae_u32)palette.size());
	le32(v, 0);
	for (uae_u32 m : masks)
		le32(v, m);
	v.resize(14 + header_size + extra_masks, 0);
	for (uae_u32 c : palette)
		le32(v, c);
	for (const auto &row : rows)
		v.insert(v.end(), row.begin(), row.end());
	return v;
}

static void bmp_tests()
{
	clipboard_bitmap dst;
	std::vector<uae_u8> v;

	// What SDL's Windows backend returns for CF_DIBV5: BITMAPV5HEADER,
	// 32 bpp BI_BITFIELDS with an alpha mask, bottom-up. Alpha is ignored.
	const std::vector<uae_u32> argb = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
	const std::vector<std::vector<uae_u8>> rows32 = {
		{ 0x56, 0x34, 0x12, 0x00, 0xff, 0xff, 0xff, 0x80 }, // bottom: 0x123456, white
		{ 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff }, // top: red, green
	};
	v = bmp(2, 2, 32, 3, 124, argb, {}, rows32);
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) && dst.width == 2 && dst.height == 2 &&
		dst.pixels == std::vector<uae_u32>({ 0xff0000, 0x00ff00, 0x123456, 0xffffff }), "V5 32-bit BI_BITFIELDS bottom-up");
	v = bmp(2, -2, 32, 3, 124, argb, {}, rows32);
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) &&
		dst.pixels == std::vector<uae_u32>({ 0x123456, 0xffffff, 0xff0000, 0x00ff00 }), "V5 32-bit top-down");
	// The masks decide the channel order, not the byte position.
	v = bmp(1, 1, 32, 3, 40, { 0x000000ff, 0x0000ff00, 0x00ff0000 }, {}, { { 0x11, 0x22, 0x33, 0x00 } });
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) && dst.pixels[0] == 0x112233, "BITMAPINFOHEADER with RGB masks");
	// 16-bit 5-6-5 and the default 5-5-5.
	v = bmp(1, 1, 16, 3, 40, { 0xf800, 0x07e0, 0x001f }, {}, { { 0xe0, 0x07, 0x00, 0x00 } });
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) && dst.pixels[0] == 0x00ff00, "16-bit 5-6-5 green");
	v = bmp(1, 1, 16, 0, 40, {}, {}, { { 0x00, 0x7c, 0x00, 0x00 } });
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) && dst.pixels[0] == 0xff0000, "16-bit 5-5-5 red");
	// 24-bit with an odd width: each row is padded to four bytes.
	v = bmp(3, 2, 24, 0, 40, {}, {}, {
		{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0 },
		{ 0x30, 0x20, 0x10, 0x60, 0x50, 0x40, 0x90, 0x80, 0x70, 0, 0, 0 } });
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) &&
		dst.pixels == std::vector<uae_u32>({ 0x102030, 0x405060, 0x708090, 0x030201, 0x060504, 0x090807 }),
		"24-bit row padding");
	// Palettes: 8-bit, 4-bit with an odd width, 1-bit.
	v = bmp(2, 1, 8, 0, 40, {}, { 0x000000, 0xabcdef, 0x123456 }, { { 2, 1, 0, 0 } });
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) && dst.pixels == std::vector<uae_u32>({ 0x123456, 0xabcdef }), "8-bit palette");
	std::vector<uae_u32> pal16(16);
	pal16[3] = 0x333333; pal16[10] = 0xaaaaaa; pal16[15] = 0xffffff;
	v = bmp(3, 1, 4, 0, 40, {}, pal16, { { 0x3a, 0xf0, 0, 0 } });
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) &&
		dst.pixels == std::vector<uae_u32>({ 0x333333, 0xaaaaaa, 0xffffff }), "4-bit palette");
	v = bmp(10, 1, 1, 0, 40, {}, { 0x000000, 0xffffff }, { { 0x80, 0x40, 0, 0 } });
	check(clipboard_bmp_decode(v.data(), v.size(), &dst) && dst.pixels[0] == 0xffffff && dst.pixels[1] == 0 &&
		dst.pixels[9] == 0xffffff, "1-bit palette");

	// Malformed files are refused.
	v = bmp(2, 2, 32, 3, 124, argb, {}, rows32);
	for (size_t i = 0; i < v.size(); i++)
		check(!clipboard_bmp_decode(v.data(), i, &dst), "truncated BMP");
	v = bmp(1, 1, 24, 1, 40, {}, {}, { { 0, 0, 0, 0 } });
	check(!clipboard_bmp_decode(v.data(), v.size(), &dst), "RLE BMP refused");
	v = bmp(1, 1, 32, 3, 40, { 0x00ff00ff, 0x0000ff00, 0x000000ff }, {}, { { 0, 0, 0, 0 } });
	check(!clipboard_bmp_decode(v.data(), v.size(), &dst), "non-contiguous mask refused");
	v = bmp(100000, 100000, 24, 0, 40, {}, {}, {});
	check(!clipboard_bmp_decode(v.data(), v.size(), &dst), "oversized BMP refused");

	// Encoded BMP and PNG decode to the same pixels.
	clipboard_bitmap src;
	src.width = 5; src.height = 3;
	for (int i = 0; i < 15; i++)
		src.pixels.push_back((uae_u32)(i * 0x112233) & 0xffffff);
	check(clipboard_bmp_encode(src, &v) && clipboard_bmp_decode(v.data(), v.size(), &dst) &&
		dst.width == 5 && dst.height == 3 && dst.pixels == src.pixels, "BMP roundtrip");
	check(clipboard_png_encode(src, &v) && clipboard_png_decode(v.data(), v.size(), &dst) &&
		dst.width == 5 && dst.height == 3 && dst.pixels == src.pixels, "PNG roundtrip");
	check(!clipboard_png_decode(v.data(), v.size() / 2, &dst), "truncated PNG");
#ifdef AMIBERRY_MACOS
	// ImageIO decoding keeps the top row on top (src's rows all differ).
	check(clipboard_tiff_encode(src, &v) && clipboard_imageio_decode(v.data(), v.size(), &dst) &&
		dst.width == 5 && dst.height == 3 && dst.pixels == src.pixels, "TIFF roundtrip through ImageIO");
	check(clipboard_png_encode(src, &v) && clipboard_imageio_decode(v.data(), v.size(), &dst) &&
		dst.pixels == src.pixels, "PNG through ImageIO");
#endif
}

static void transfer_tests()
{
	for (bool deferred : { false, true }) {
		clipboard_transfer t;
		t.pending = clipboard_encode_text("transfer");
		t.start(); check(t.phase == 0, "wait for guest initialization");
		t.guest_data = 16;
		trap_put_long(NULL, 24, 0x1234);
		t.signaling = true;
		int initial_signals = signals;
		t.vsync(deferred);
		check(signals == initial_signals + 1, "signal guest before clipboard initialization");
		t.signaling = false;
		t.initialized = true;
		trap_put_long(NULL, 16, 0); trap_put_long(NULL, 20, 128); trap_put_long(NULL, 24, 0x1234);
		t.start(); check(t.phase == 1, "pending transfer");
		t.unsafe_period();
		for (int i = 0; i < 99; i++) t.vsync(deferred);
		check(t.phase == 1 && trap_get_long(NULL, 16) == 0, "unsafe period");
		t.vsync(deferred);
		check(t.phase == 2 && trap_get_long(NULL, 16) == t.pending.size(), "announce transfer");
		std::vector<uae_u8> expected = t.pending;
		check(t.want_data(NULL) == 1 && !memcmp(guest + 128, expected.data(), expected.size()), "guest receives IFF bytes");
		check(t.pending.empty() && t.phase == 0, "transfer released");
		t.pending = expected; t.start(); trap_put_long(NULL, 16, 999);
		check(t.want_data(NULL) == 0 && t.pending.empty() && t.phase == 0, "mismatched request released");
		t.signaling = true; t.delay = 2; t.unsafe_delay = 10;
		t.vsync(deferred);
		check(t.unsafe_delay == (deferred ? 10 : 9), "host signaling cadence");
		signal_free = false; int before = signals;
		t.vsync(deferred); check(signals == before, "busy native queue"); signal_free = true;
		t.pending = expected; t.start(); t.reset();
		check(!t.initialized && !t.signaling && !t.guest_data && t.pending.empty() && !t.phase, "reset clears transfer");
	}
}

// Guest memory layout of the boot ROM's clipboard data block: size, buffer
// address, task.
static constexpr uaecptr CLIP_DATA = 16;
static constexpr uaecptr CLIP_BUFFER = 1024;

// Runs vsyncs until the emulation side announces a transfer, then lets the
// guest fetch it.
static std::vector<uae_u8> guest_receive()
{
	trap_put_long(NULL, CLIP_DATA, 0);
	for (int i = 0; i < 200 && !trap_get_long(NULL, CLIP_DATA); i++)
		clipboard_vsync();
	const uae_u32 size = trap_get_long(NULL, CLIP_DATA);
	if (!size || size > sizeof guest - CLIP_BUFFER)
		return {};
	if (amiga_clipboard_want_data(NULL) != 1)
		return {};
	return std::vector<uae_u8>(guest + CLIP_BUFFER, guest + CLIP_BUFFER + size);
}

static void guest_copy(const std::vector<uae_u8> &iff)
{
	memcpy(guest + CLIP_BUFFER, iff.data(), iff.size());
	amiga_clipboard_got_data(NULL, CLIP_BUFFER, (uae_u32)iff.size(), (uae_u32)iff.size());
}

static void sharing_tests()
{
	clipboard_reset();
	trap_put_long(NULL, CLIP_DATA + 4, CLIP_BUFFER);
	trap_put_long(NULL, CLIP_DATA + 8, 0x1234);
	amiga_clipboard_task_start(NULL, CLIP_DATA);
	check(!clipboard_host_snapshot_wanted(), "no host snapshot before the guest asks");
	amiga_clipboard_init(NULL);
	check(clipboard_host_snapshot_wanted() && !clipboard_host_snapshot_wanted(), "guest init asks for one host snapshot");
	// The main thread answers with what the host clipboard holds.
	clipboard_content host;
	host.type = clipboard_content::kind::text;
	host.text = "before boot";
	clipboard_set_host_content(std::move(host));

	std::vector<uae_u8> got = guest_receive();
	std::string text;
	check(clipboard_decode_text(got.data(), (uae_u32)got.size(), &text, false) && text == "before boot",
		"host text reaches the guest");

	// The guest copies an image; the host gets it, and when the host reports
	// that same image back, the guest is not sent its own copy again.
	clipboard_bitmap image;
	image.width = 3; image.height = 1; image.pixels = { 0xff0000, 0x00ff00, 0x0000ff };
	std::vector<uae_u8> ilbm;
	check(clipboard_encode_bitmap(image, false, &ilbm), "encode guest image");
	guest_copy(ilbm);
	clipboard_content copied;
	check(clipboard_take_guest_content(&copied) && copied.type == clipboard_content::kind::bitmap &&
		copied.bitmap.pixels == image.pixels, "guest image reaches the host");
	check(!clipboard_take_guest_content(&copied), "guest image handed over once");
	clipboard_content echo;
	echo.type = clipboard_content::kind::bitmap;
	echo.bitmap = image;
	clipboard_set_host_content(std::move(echo));
	check(guest_receive().empty(), "own copy not sent back to the guest");

	// A new host image is offered; paste-to-keyboard types the last text.
	clipboard_content host_image;
	host_image.type = clipboard_content::kind::bitmap;
	host_image.bitmap.width = 2; host_image.bitmap.height = 1; host_image.bitmap.pixels = { 0x123456, 0xabcdef };
	clipboard_set_host_content(std::move(host_image));
	got = guest_receive();
	clipboard_bitmap received;
	check(clipboard_decode_bitmap(got.data(), (uae_u32)got.size(), &received) &&
		received.pixels == std::vector<uae_u32>({ 0x123456, 0xabcdef }), "host image reaches the guest");
	injected.clear();
	target_paste_to_keyboard();
	check(injected.empty(), "nothing to type while the clipboard holds an image");
	guest_copy(clipboard_encode_text("line1\nline2"));
	check(clipboard_take_guest_content(&copied) && copied.text == "line1\nline2", "guest text reaches the host");
	clipboard_content host_text;
	host_text.type = clipboard_content::kind::text;
	host_text.text = "a\r\nb\rc";
	clipboard_set_host_content(std::move(host_text));
	target_paste_to_keyboard();
	check(injected == "a\nb\nc", "paste-to-keyboard normalizes line ends");

	// The host clipboard is emptied or holds something that cannot be
	// shared: the earlier text is not typed any more.
	clipboard_set_host_content(clipboard_content());
	injected.clear();
	target_paste_to_keyboard();
	check(injected.empty(), "emptied host clipboard is not typed");

	// The guest reboots while the host clipboard still holds the image the
	// guest received before; the new clipboard task gets it again.
	clipboard_content before_reboot;
	before_reboot.type = clipboard_content::kind::bitmap;
	before_reboot.bitmap.width = 2; before_reboot.bitmap.height = 1; before_reboot.bitmap.pixels = { 0x654321, 0x0a0b0c };
	clipboard_content after_reboot = before_reboot;
	clipboard_set_host_content(std::move(before_reboot));
	check(!guest_receive().empty(), "host image before the reboot");
	clipboard_reset();
	amiga_clipboard_task_start(NULL, CLIP_DATA);
	amiga_clipboard_init(NULL);
	check(clipboard_host_snapshot_wanted(), "rebooted guest asks for the host clipboard");
	clipboard_set_host_content(std::move(after_reboot));
	got = guest_receive();
	check(clipboard_decode_bitmap(got.data(), (uae_u32)got.size(), &received) &&
		received.pixels == std::vector<uae_u32>({ 0x654321, 0x0a0b0c }), "rebooted guest gets the unchanged host image");
	clipboard_reset();
}

int main()
{
	text_tests();
	ilbm_tests();
	bmp_tests();
	transfer_tests();
	sharing_tests();
	if (!failures)
		puts("Clipboard codec and sharing tests passed");
	return failures ? 1 : 0;
}
