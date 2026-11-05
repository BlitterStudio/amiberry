#ifndef UAE_CLIPBOARD_COMMON_H
#define UAE_CLIPBOARD_COMMON_H

#include "uae/types.h"
#include <string>
#include <vector>

struct TrapContext;

const size_t CLIP_SIZE_LIMIT = 10000000;
const size_t CLIP_SIZE_LIMIT_INIT = 30000;

// Pixels are top-down 0x00RRGGBB, independent of the native bitmap format.
struct clipboard_bitmap {
	int width = 0;
	int height = 0;
	std::vector<uae_u32> pixels;
};

std::vector<uae_u8> clipboard_encode_text(const std::string &text);
// CRLF mode retains the Windows ANSI, NUL and legacy chunk-padding rules.
bool clipboard_decode_text(const uae_u8 *data, uae_u32 size,
	std::string *text, bool crlf);
bool clipboard_encode_bitmap(const clipboard_bitmap &bitmap, bool initial,
	std::vector<uae_u8> *iff);
// Windows retains low HAM8 bits and accepts reduced-depth RGB planes.
bool clipboard_decode_bitmap(const uae_u8 *data, uae_u32 size,
	clipboard_bitmap *bitmap, bool preserve_ham_low_bits = false,
	bool allow_reduced_rgb = false);

extern int clipboard_debug;
extern int clipboard_log;
void clipboard_debug_dump(TrapContext *ctx, const TCHAR *name,
	uaecptr data, int size);

// Host adapters own notification, polling and clipboard access. The shared
// transfer owns the clipboard.device handshake and its guest-facing buffer.
struct clipboard_transfer {
	uaecptr guest_data = 0;
	int delay = 0;
	int unsafe_delay = 0;
	bool signaling = false;
	bool initialized = false;
	std::vector<uae_u8> pending;
	int phase = 0;

	void start();
	int want_data(TrapContext *ctx);
	// Windows defers data transfer until the periodic signal is due.
	void vsync(bool defer_while_signaling);
	void unsafe_period();
	void reset();
private:
	static uae_u32 announce(TrapContext *ctx, void *user);
	static uae_u32 signal(TrapContext *ctx, void *user);
};

#endif
