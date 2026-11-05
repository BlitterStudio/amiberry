/*
 * Clipboard sharing, emulation side.
 *
 * The guest half (the clipboard.device handshake with the boot ROM's
 * clipboard task, and the IFF FTXT and ILBM conversions) is WinUAE's shared
 * code in clipboard_common.cpp. The host clipboard is only touched on the
 * main thread, in clipboard_host.cpp; this file hands content between the two
 * under a mutex, because the trap handlers below can run on a trap thread.
 * It has no host dependencies, so the libretro core builds it too.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include <cstring>
#include <mutex>
#include <vector>

#include "traps.h"
#include "keybuf.h"
#include "clipboard.h"

extern bool filesys_heartbeat(void);

static clipboard_transfer transfer;
static bool clip_disabled;
// The next offer to the guest is the first one after its clipboard task
// started, which uses the smaller initial size limit.
static bool initial_offer;

static std::mutex content_mutex;
// What the host clipboard holds, kept for paste-to-keyboard.
static clipboard_content host_content;
// host_content still has to be offered to the guest.
static bool host_content_pending;
// What the guest's clipboard holds as far as this side knows: the last
// content offered to it or copied there. Host content equal to it is not
// offered, so the guest's own copies are not sent back to it.
static clipboard_content guest_clipboard;
// What the guest copied, waiting for the main thread.
static clipboard_content guest_content;
static bool guest_content_pending;
static bool host_snapshot_wanted;

static bool same_content(const clipboard_content &a, const clipboard_content &b)
{
	if (a.type != b.type)
		return false;
	if (a.type == clipboard_content::kind::text)
		return a.text == b.text;
	if (a.type == clipboard_content::kind::bitmap)
		return a.bitmap.width == b.bitmap.width && a.bitmap.height == b.bitmap.height
			&& a.bitmap.pixels == b.bitmap.pixels;
	return true;
}

void clipboard_set_host_content(clipboard_content &&content)
{
	std::lock_guard<std::mutex> lock(content_mutex);
	host_content = std::move(content);
	host_content_pending = host_content.type != clipboard_content::kind::none
		&& !same_content(host_content, guest_clipboard);
}

bool clipboard_take_guest_content(clipboard_content *content)
{
	std::lock_guard<std::mutex> lock(content_mutex);
	if (!guest_content_pending)
		return false;
	// This becomes the host clipboard; remember it as such so that it is not
	// sent back to the guest and paste-to-keyboard types it.
	host_content = guest_content;
	host_content_pending = false;
	*content = std::move(guest_content);
	guest_content = clipboard_content();
	guest_content_pending = false;
	return true;
}

bool clipboard_host_snapshot_wanted()
{
	std::lock_guard<std::mutex> lock(content_mutex);
	const bool wanted = host_snapshot_wanted;
	host_snapshot_wanted = false;
	return wanted;
}

static void queue_for_host(clipboard_content &&content)
{
	std::lock_guard<std::mutex> lock(content_mutex);
	// The guest's copy is newer than any host content still waiting.
	guest_clipboard = content;
	host_content_pending = false;
	guest_content = std::move(content);
	guest_content_pending = true;
}

static void from_iff(TrapContext *ctx, uaecptr data, uae_u32 len)
{
	if (len < 12 || !trap_valid_address(ctx, data, len))
		return;
	std::vector<uae_u8> buffer(len);
	trap_get_bytes(ctx, buffer.data(), data, len);
	if (clipboard_debug)
		clipboard_debug_dump(ctx, _T("clipboard_a2p"), data, len);
	if (memcmp(buffer.data(), "FORM", 4))
		return;

	clipboard_content content;
	if (!memcmp(buffer.data() + 8, "FTXT", 4)) {
		// SDL converts line endings for the host, so the text stays LF.
		if (!clipboard_decode_text(buffer.data(), len, &content.text, false))
			return;
		content.type = clipboard_content::kind::text;
	} else if (!memcmp(buffer.data() + 8, "ILBM", 4)) {
		// Keep HAM8's low colour bits and accept RGB images with fewer than
		// 24 planes, as Amiberry's earlier Windows-only decoder did.
		if (!clipboard_decode_bitmap(buffer.data(), len, &content.bitmap, true, true)) {
			write_log(_T("clipboard: unsupported ILBM from the guest\n"));
			return;
		}
		content.type = clipboard_content::kind::bitmap;
	} else {
		return;
	}
	queue_for_host(std::move(content));
}

static void offer_host_content()
{
	clipboard_content content;
	bool initial;
	{
		std::lock_guard<std::mutex> lock(content_mutex);
		if (!host_content_pending)
			return;
		host_content_pending = false;
		content = host_content;
		initial = initial_offer;
		initial_offer = false;
	}
	const size_t limit = initial ? CLIP_SIZE_LIMIT_INIT : CLIP_SIZE_LIMIT;
	if (content.type == clipboard_content::kind::text) {
		if (content.text.size() > limit)
			return;
		transfer.pending = clipboard_encode_text(content.text);
	} else if (!clipboard_encode_bitmap(content.bitmap, initial, &transfer.pending)) {
		write_log(_T("clipboard: host image %dx%d is too large for the guest\n"),
			content.bitmap.width, content.bitmap.height);
		return;
	}
	{
		std::lock_guard<std::mutex> lock(content_mutex);
		guest_clipboard = std::move(content);
	}
	transfer.start();
}

void clipboard_vsync()
{
	if (!filesys_heartbeat() || !transfer.guest_data)
		return;
	// Like WinUAE on Windows, nothing is sent while the periodic signal
	// that wakes the guest's clipboard task is counting down.
	transfer.vsync(true);
	if (transfer.initialized && !transfer.phase && !clip_disabled)
		offer_host_content();
}

void clipboard_unsafeperiod()
{
	transfer.unsafe_period();
}

void clipboard_disable(bool disabled)
{
	clip_disabled = disabled;
}

void clipboard_reset()
{
	write_log(_T("clipboard: reset (%08x)\n"), transfer.guest_data);
	transfer.reset();
	clip_disabled = false;
	std::lock_guard<std::mutex> lock(content_mutex);
	guest_content = clipboard_content();
	guest_content_pending = false;
	guest_clipboard = clipboard_content();
	host_content_pending = false;
	host_snapshot_wanted = false;
	initial_offer = false;
}

uaecptr amiga_clipboard_proc_start(TrapContext *ctx)
{
	write_log(_T("clipboard process init: %08x\n"), transfer.guest_data);
	transfer.signaling = true;
	return transfer.guest_data;
}

void amiga_clipboard_task_start(TrapContext *ctx, uaecptr data)
{
	transfer.guest_data = data;
	transfer.signaling = true;
	write_log(_T("clipboard task init: %08x\n"), transfer.guest_data);
}

void amiga_clipboard_die(TrapContext *ctx)
{
	transfer.signaling = false;
	write_log(_T("clipboard not initialized\n"));
}

void amiga_clipboard_init(TrapContext *ctx)
{
	transfer.signaling = false;
	transfer.initialized = true;
	write_log(_T("clipboard initialized\n"));
	// A new clipboard task starts with an empty clipboard. The main thread
	// reads the host clipboard on its next pass and the next vsync offers
	// it, even when it equals what an earlier task received.
	std::lock_guard<std::mutex> lock(content_mutex);
	guest_clipboard = clipboard_content();
	host_content_pending = false;
	host_snapshot_wanted = true;
	initial_offer = true;
}

void amiga_clipboard_got_data(TrapContext *ctx, uaecptr data, uae_u32 size, uae_u32 actual)
{
	if (!transfer.initialized) {
		write_log(_T("clipboard: got_data() before initialized!?\n"));
		return;
	}
	from_iff(ctx, data, actual);
}

int amiga_clipboard_want_data(TrapContext *ctx)
{
	return transfer.want_data(ctx);
}

void target_paste_to_keyboard()
{
	std::string text;
	{
		std::lock_guard<std::mutex> lock(content_mutex);
		if (host_content.type != clipboard_content::kind::text)
			return;
		text.reserve(host_content.text.size());
		for (size_t i = 0; i < host_content.text.size(); i++) {
			const char c = host_content.text[i];
			if (c != '\r') {
				text.push_back(c);
			} else if (i + 1 >= host_content.text.size() || host_content.text[i + 1] != '\n') {
				text.push_back('\n');
			}
		}
	}
	if (!text.empty())
		keybuf_inject(text.c_str());
}
