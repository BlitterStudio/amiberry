/*
 * MNT ZZ9000 RTG, Ethernet, and ZZ9000AX audio emulation
 *
 * Implements the register and framebuffer ABI used by the current
 * ZZ9000.card driver and ZZ9000OS 2.8 firmware.  The optional surface
 * allocator and packed-YUV video overlay use the same GFXData command
 * interface as the hardware.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "picasso96.h"
#include "gfxboard.h"
#include "custom.h"
#include "ethernet.h"
#include "sana2.h"
#include "zz9000_net_protocol.h"
#ifdef AMIBERRY
#include "amiberry_cursor.h"
#endif
#include "xwin.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <mutex>
#include <new>
#if defined(AHI) && !defined(LIBRETRO)
#include <chrono>
#include <SDL3/SDL.h>
#include "sounddep/sound.h"
struct zz_ax_audio_engine;
#endif
struct zz9000_net_engine;


static constexpr uae_u32 ZZ9000_Z2_SIZE = 0x00400000;
static constexpr uae_u32 ZZ9000_Z3_SIZE = 0x08000000;
static constexpr uae_u32 ZZ9000_MEMORY_BASE = 0x00010000;
static constexpr uae_u32 ZZ9000_GFXDATA = 0x031f0000;
static constexpr uae_u32 ZZ9000_TEMPLATE = 0x03200000;
static constexpr uae_u32 ZZ9000_SURFACE_HEAP = 0x03300000;
static constexpr uae_u32 ZZ9000_SURFACE_HEAP_SIZE = 0x02b00000;
static constexpr uae_u32 ZZ9000_SURFACE_HEAP_END =
	ZZ9000_SURFACE_HEAP + ZZ9000_SURFACE_HEAP_SIZE;
static constexpr int ZZ9000_MAX_SURFACES = 1024;

enum zz9000_color_mode {
	ZZ_COLOR_CLUT,
	ZZ_COLOR_RGB565,
	ZZ_COLOR_BGRA32,
	ZZ_COLOR_RGB555
};

enum zz9000_register {
	ZZ_REG_HW_VERSION = 0x00,
	ZZ_REG_MODE = 0x02,
	ZZ_REG_CONFIG = 0x04,
	ZZ_REG_SPRITE_X = 0x06,
	ZZ_REG_SPRITE_Y = 0x08,
	ZZ_REG_PAN_HI = 0x0a,
	ZZ_REG_PAN_LO = 0x0c,
	ZZ_REG_X1 = 0x10,
	ZZ_REG_Y1 = 0x12,
	ZZ_REG_X2 = 0x14,
	ZZ_REG_Y2 = 0x16,
	ZZ_REG_ROW_PITCH = 0x18,
	ZZ_REG_X3 = 0x1a,
	ZZ_REG_Y3 = 0x1c,
	ZZ_REG_RGB_HI = 0x1e,
	ZZ_REG_RGB_LO = 0x20,
	ZZ_REG_FILLRECT = 0x22,
	ZZ_REG_COPYRECT = 0x24,
	ZZ_REG_FILLTEMPLATE = 0x26,
	ZZ_REG_SRC_HI = 0x28,
	ZZ_REG_SRC_LO = 0x2a,
	ZZ_REG_DST_HI = 0x2c,
	ZZ_REG_DST_LO = 0x2e,
	ZZ_REG_COLORMODE = 0x30,
	ZZ_REG_SRC_PITCH = 0x32,
	ZZ_REG_RGB2_HI = 0x34,
	ZZ_REG_RGB2_LO = 0x36,
	ZZ_REG_P2C = 0x38,
	ZZ_REG_DRAWLINE = 0x3a,
	ZZ_REG_P2D = 0x3c,
	ZZ_REG_INVERTRECT = 0x3e,
	ZZ_REG_USER1 = 0x40,
	ZZ_REG_USER2 = 0x42,
	ZZ_REG_USER3 = 0x44,
	ZZ_REG_USER4 = 0x46,
	ZZ_REG_SPRITE_BITMAP = 0x48,
	ZZ_REG_SPRITE_COLORS = 0x4a,
	ZZ_REG_VBLANK = 0x4c,
	ZZ_REG_DMA_OP = 0x5a,
	ZZ_REG_ACC_OP = 0x5c,
	ZZ_REG_SPLIT_POS = 0x5e,
	ZZ_REG_SET_FEATURE = 0x60,
	ZZ_REG_ETH_TX = 0x80,
	ZZ_REG_ETH_RX = 0x82,
	ZZ_REG_ETH_MAC_HI = 0x84,
	ZZ_REG_ETH_MAC_MID = 0x86,
	ZZ_REG_ETH_MAC_LO = 0x88,
	ZZ_REG_ETH_RX_STATUS = 0x8c,
	ZZ_REG_ETH_RX_STATS = 0x8e,
	ZZ_REG_FW_VERSION = 0xc0,
	ZZ_REG_FW_CAPABILITIES = 0xe6,
	ZZ_REG_CONFIG_KEY = 0xe8,
	ZZ_REG_CONFIG_PRESENT = 0xea,

	// ZZ9000AX audio (AHI card)
	ZZ_REG_AUDIO_SWAB = 0x70,
	ZZ_REG_AUDIO_SCALE = 0x74,
	ZZ_REG_AUDIO_PARAM = 0x76,
	ZZ_REG_AUDIO_VAL = 0x78,
	ZZ_REG_AUDIO_CONFIG = 0xf4,
	ZZ_REG_AUDIO_RX_STATUS = 0xf6,
	ZZ_REG_AUDIO_TX_STATUS = 0xf8
};

enum zz9000_dma_op {
	ZZ_OP_NONE,
	ZZ_OP_DRAWLINE,
	ZZ_OP_FILLRECT,
	ZZ_OP_COPYRECT,
	ZZ_OP_COPYRECT_NOMASK,
	ZZ_OP_RECT_TEMPLATE,
	ZZ_OP_RECT_PATTERN,
	ZZ_OP_P2C,
	ZZ_OP_P2D,
	ZZ_OP_INVERTRECT,
	ZZ_OP_PAN,
	ZZ_OP_SPRITE_XY,
	ZZ_OP_SPRITE_COLOR,
	ZZ_OP_SPRITE_BITMAP,
	ZZ_OP_SPRITE_CLUT_BITMAP,
	ZZ_OP_ETH_USB_OFFSETS,
	ZZ_OP_SET_SPLIT_POS,
	ZZ_OP_SET_PALETTE,
	ZZ_OP_WRITE_YUV,
	ZZ_OP_VIDEO_OVERLAY
};

enum zz9000_acc_op {
	ZZ_ACC_NONE,
	ZZ_ACC_BUFFER_FLIP,
	ZZ_ACC_BUFFER_CLEAR,
	ZZ_ACC_BLIT_RECT,
	ZZ_ACC_ALLOC_SURFACE,
	ZZ_ACC_FREE_SURFACE,
	ZZ_ACC_SET_BPP_CONVERSION_TABLE,
	ZZ_ACC_DRAW_LINE,
	ZZ_ACC_FILL_RECT
};

enum zz9000_minterm {
	ZZ_MINTERM_FALSE,
	ZZ_MINTERM_NOR,
	ZZ_MINTERM_ONLYDST,
	ZZ_MINTERM_NOTSRC,
	ZZ_MINTERM_ONLYSRC,
	ZZ_MINTERM_INVERT,
	ZZ_MINTERM_EOR,
	ZZ_MINTERM_NAND,
	ZZ_MINTERM_AND,
	ZZ_MINTERM_NEOR,
	ZZ_MINTERM_DST,
	ZZ_MINTERM_NOTONLYSRC,
	ZZ_MINTERM_SRC,
	ZZ_MINTERM_NOTONLYDST,
	ZZ_MINTERM_OR,
	ZZ_MINTERM_TRUE
};

struct zz9000_surface {
	uae_u32 offset;
	uae_u32 size;
};

struct zz9000_overlay {
	bool configured;
	bool active;
	bool mode_stale;
	bool key_enabled;
	uae_u8 variant;
	uae_u32 source_offset;
	uae_u16 source_pitch;
	uae_u16 source_width;
	uae_u16 source_height;
	int destination_x;
	int destination_y;
	int destination_width;
	int destination_height;
	uae_u32 color_key;
	int mode_width;
	int mode_height;
	int mode_color;
	int mode_scale;
	uae_u32 mode_pitch;
};

struct zz9000_state {
	int devnum;
	int monitor_id;
	bool z3;
	uae_u32 configured;
	uae_u32 card_size;
	uae_u8 *memory;
	uae_u16 registers[0x808];

	bool enabled;
	bool visible;
	bool modified;
	bool mode_changed;
	bool capture_mode;
	bool display_blank;
	bool overlay_feature;
	bool secondary_palette_feature;
	uae_u16 config_key;
	bool vblank_read;

	int mode_id;
	int color_mode;
	int scale_mode;
	int width;
	int height;
	uae_u32 pan_offset;
	uae_u32 pan_width;
	uae_u32 split_offset;
	int split_position;

	uae_u16 x[4];
	uae_u16 y[4];
	uae_u16 user[4];
	uae_u32 source_offset;
	uae_u32 destination_offset;
	uae_u16 source_pitch;
	uae_u16 destination_pitch;
	uae_u8 blit_color_mode;
	uae_u8 blit_high;
	uae_u32 foreground;
	uae_u32 background;

	uae_u32 palette_data;
	MyCLUTEntry primary_palette[256];
	MyCLUTEntry secondary_palette[256];
	zz9000_surface surfaces[ZZ9000_MAX_SURFACES];
	int surface_count;
	uae_u8 color_conversion_table[65536];
	zz9000_overlay overlay;

	bool sprite_visible;
	int sprite_x;
	int sprite_y;
	int sprite_x_offset;
	int sprite_y_offset;
	int sprite_width;
	int sprite_height;
	uae_u8 sprite_pixels[32 * 48];
	uae_u32 sprite_colors[4];

	// ZZ9000AX audio (AHI card): same physical MNT ZZ9000 board as the RTG
	// model (identical Zorro ConfigDev), so the audio registers and the
	// TX/RX rings live in this board's register bank and card memory.
	bool audio_play;
	bool audio_record;
	uae_u16 audio_scale;
	bool audio_collision;
	uae_u16 audio_tx_seq;
	uae_u8 audio_tx_read_period;
	uae_u16 audio_rx_seq;
	uae_u8 audio_rx_write_period;
	uae_u32 audio_tx_ring_off;
	uae_u32 audio_rx_ring_off;
	bool audio_intreq_pending;
	// E2E diagnostics: rate-limited logging of unmodified-driver traffic.
	// audio_log_flags bits: 0 = first 0xF4 probe read, 1 = first 0xF4
	// setup write, 2 = first INT6 assert, 3 = first 0x04 ISR ack.
	uae_u32 audio_log_kicks;
	uae_u32 audio_log_periods;
	uae_u32 audio_log_flags;
	uae_u32 audio_cfg_reads;
	zz9000_net_engine *net;
	bool int2;
#if defined(AHI) && !defined(LIBRETRO)
	zz_ax_audio_engine *ax_audio;
#endif
};

static zz9000_state *zz9000_boards[MAX_RTG_BOARDS];

static uae_u32 REGPARAM2 zz9000_lget(uaecptr addr);
static uae_u32 REGPARAM2 zz9000_wget(uaecptr addr);
static uae_u32 REGPARAM2 zz9000_bget(uaecptr addr);
static void REGPARAM2 zz9000_lput(uaecptr addr, uae_u32 value);
static void REGPARAM2 zz9000_wput(uaecptr addr, uae_u32 value);
static void REGPARAM2 zz9000_bput(uaecptr addr, uae_u32 value);
static int REGPARAM2 zz9000_check(uaecptr addr, uae_u32 size);
static uae_u8 *REGPARAM2 zz9000_xlate(uaecptr addr);

static addrbank zz9000_bank = {
	zz9000_lget, zz9000_wget, zz9000_bget,
	zz9000_lput, zz9000_wput, zz9000_bput,
	zz9000_xlate, zz9000_check, nullptr, nullptr, _T("ZZ9000"),
	zz9000_lget, zz9000_wget,
	/* ZZ9000 VRAM lives in the board's private allocation, not at its
	 * natmem address. Force JIT accesses through the bank translation and
	 * helpers instead of replaying faulting direct accesses. */
	ABFLAG_IO, S_READ | S_N_ADDR, S_WRITE | S_N_ADDR
};

static inline uae_u16 zz_bswap16(uae_u16 value)
{
	return static_cast<uae_u16>((value << 8) | (value >> 8));
}

static inline uae_u16 zz_read_be16(const uae_u8 *p)
{
	return static_cast<uae_u16>((p[0] << 8) | p[1]);
}

static inline uae_u32 zz_read_be32(const uae_u8 *p)
{
	return (static_cast<uae_u32>(p[0]) << 24) |
		(static_cast<uae_u32>(p[1]) << 16) |
		(static_cast<uae_u32>(p[2]) << 8) | p[3];
}

static inline uae_u32 zz_read_le32(const uae_u8 *p)
{
	return static_cast<uae_u32>(p[0]) |
		(static_cast<uae_u32>(p[1]) << 8) |
		(static_cast<uae_u32>(p[2]) << 16) |
		(static_cast<uae_u32>(p[3]) << 24);
}

static inline void zz_write_be32(uae_u8 *p, uae_u32 value)
{
	p[0] = static_cast<uae_u8>(value >> 24);
	p[1] = static_cast<uae_u8>(value >> 16);
	p[2] = static_cast<uae_u8>(value >> 8);
	p[3] = static_cast<uae_u8>(value);
}

static inline void zz_write_le32(uae_u8 *p, uae_u32 value)
{
	p[0] = static_cast<uae_u8>(value);
	p[1] = static_cast<uae_u8>(value >> 8);
	p[2] = static_cast<uae_u8>(value >> 16);
	p[3] = static_cast<uae_u8>(value >> 24);
}

static uae_u8 *zz_memory_ptr(zz9000_state *data, uae_u32 offset, uae_u32 size = 1)
{
	const uae_u64 card_offset = static_cast<uae_u64>(ZZ9000_MEMORY_BASE) + offset;
	if (card_offset + size > data->card_size)
		return nullptr;
	return data->memory + card_offset;
}

static const uae_u8 *zz_memory_ptr(const zz9000_state *data, uae_u32 offset, uae_u32 size = 1)
{
	return zz_memory_ptr(const_cast<zz9000_state *>(data), offset, size);
}

static uae_u8 *zz_gfxdata_ptr(zz9000_state *data, uae_u32 offset, uae_u32 size)
{
	return zz_memory_ptr(data, ZZ9000_GFXDATA + offset, size);
}

static uae_u16 zz_gd_u16(zz9000_state *data, uae_u32 offset)
{
	const uae_u8 *p = zz_gfxdata_ptr(data, offset, 2);
	return p ? zz_read_be16(p) : 0;
}

static uae_u32 zz_gd_u32(zz9000_state *data, uae_u32 offset)
{
	const uae_u8 *p = zz_gfxdata_ptr(data, offset, 4);
	return p ? zz_read_be32(p) : 0;
}

static uae_u32 zz_gd_pen(zz9000_state *data, uae_u32 offset)
{
	const uae_u8 *p = zz_gfxdata_ptr(data, offset, 4);
	return p ? zz_read_le32(p) : 0;
}

static uae_u8 zz_gd_u8(zz9000_state *data, uae_u32 offset)
{
	const uae_u8 *p = zz_gfxdata_ptr(data, offset, 1);
	return p ? *p : 0;
}

static void zz_gd_write_u32(zz9000_state *data, uae_u32 offset, uae_u32 value)
{
	uae_u8 *p = zz_gfxdata_ptr(data, offset, 4);
	if (p)
		zz_write_be32(p, value);
}

static int zz_bytes_per_pixel(int color_mode)
{
	switch (color_mode) {
		case ZZ_COLOR_CLUT:
			return 1;
		case ZZ_COLOR_RGB565:
		case ZZ_COLOR_RGB555:
			return 2;
		case ZZ_COLOR_BGRA32:
			return 4;
		default:
			return 0;
	}
}

static int zz_color_mode_from_bpp(int bpp)
{
	switch (bpp) {
		case 1: return ZZ_COLOR_CLUT;
		case 2: return ZZ_COLOR_RGB565;
		case 4: return ZZ_COLOR_BGRA32;
		default: return -1;
	}
}

static RGBFTYPE zz_rgb_format(int color_mode)
{
	switch (color_mode) {
		case ZZ_COLOR_CLUT:
			return RGBFB_CLUT;
		case ZZ_COLOR_RGB565:
			return RGBFB_R5G6B5;
		case ZZ_COLOR_BGRA32:
			return RGBFB_B8G8R8A8;
		case ZZ_COLOR_RGB555:
			return RGBFB_R5G5B5;
		default:
			return RGBFB_NONE;
	}
}

static bool zz_mode_dimensions(int mode, int *width, int *height)
{
	switch (mode) {
		case 0: *width = 1280; *height = 720; break;
		case 1: *width = 800; *height = 600; break;
		case 2: *width = 640; *height = 480; break;
		case 3: *width = 1024; *height = 768; break;
		case 4: *width = 1280; *height = 1024; break;
		case 5: *width = 1920; *height = 1080; break;
		case 6: *width = 720; *height = 576; break;
		case 8: *width = 720; *height = 480; break;
		case 9: *width = 640; *height = 512; break;
		case 10: *width = 1600; *height = 1200; break;
		case 11: *width = 2560; *height = 1440; break;
		case 16: *width = 640; *height = 400; break;
		case 17: *width = 1920; *height = 800; break;
		default: return false;
	}
	return true;
}

static zz9000_state *zz_find_board(uaecptr addr)
{
	for (auto *data : zz9000_boards) {
		if (data && data->configured && addr >= data->configured &&
			static_cast<uae_u64>(addr) < static_cast<uae_u64>(data->configured) + data->card_size)
			return data;
	}
	return nullptr;
}

static int REGPARAM2 zz9000_check(uaecptr addr, uae_u32 size)
{
	zz9000_state *data = zz_find_board(addr);
	if (!data)
		return 0;
	const uae_u32 offset = addr - data->configured;
	return offset >= ZZ9000_MEMORY_BASE && offset < data->card_size &&
		size <= data->card_size - offset;
}

static uae_u8 *REGPARAM2 zz9000_xlate(uaecptr addr)
{
	zz9000_state *data = zz_find_board(addr);
	if (!data)
		return default_xlate(addr);
	const uae_u32 offset = addr - data->configured;
	if (offset < ZZ9000_MEMORY_BASE || offset >= data->card_size)
		return default_xlate(addr);
	/* Callers using a translated pointer bypass the byte/word/long bank
	 * handlers, so conservatively flag VRAM as changed for display refresh. */
	data->modified = true;
	return data->memory + offset;
}

static uae_u32 zz_apply_minterm(uae_u32 source, uae_u32 destination, int minterm)
{
	switch (minterm & 15) {
		case ZZ_MINTERM_FALSE: return 0;
		case ZZ_MINTERM_NOR: return ~(source | destination);
		case ZZ_MINTERM_ONLYDST: return ~source & destination;
		case ZZ_MINTERM_NOTSRC: return ~source;
		case ZZ_MINTERM_ONLYSRC: return source & ~destination;
		case ZZ_MINTERM_INVERT: return ~destination;
		case ZZ_MINTERM_EOR: return source ^ destination;
		case ZZ_MINTERM_NAND: return ~(source & destination);
		case ZZ_MINTERM_AND: return source & destination;
		case ZZ_MINTERM_NEOR: return ~(source ^ destination);
		case ZZ_MINTERM_DST: return destination;
		case ZZ_MINTERM_NOTONLYSRC: return ~source | destination;
		case ZZ_MINTERM_SRC: return source;
		case ZZ_MINTERM_NOTONLYDST: return source | ~destination;
		case ZZ_MINTERM_OR: return source | destination;
		default: return ~0U;
	}
}

static bool zz_pixel_address(zz9000_state *data, uae_u32 base, uae_u32 pitch,
	int x, int y, int color_mode, uae_u8 **pixel)
{
	const int bpp = zz_bytes_per_pixel(color_mode);
	if (!bpp || x < 0 || y < 0)
		return false;
	const uae_u64 offset = static_cast<uae_u64>(base) +
		static_cast<uae_u64>(y) * pitch + static_cast<uae_u64>(x) * bpp;
	if (offset > 0xffffffffU)
		return false;
	*pixel = zz_memory_ptr(data, static_cast<uae_u32>(offset), bpp);
	return *pixel != nullptr;
}

static uae_u32 zz_read_pixel(zz9000_state *data, uae_u32 base, uae_u32 pitch,
	int x, int y, int color_mode)
{
	uae_u8 *p;
	if (!zz_pixel_address(data, base, pitch, x, y, color_mode, &p))
		return 0;
	switch (zz_bytes_per_pixel(color_mode)) {
		case 1: return p[0];
		case 2: return p[0] | (p[1] << 8);
		case 4: return zz_read_le32(p);
		default: return 0;
	}
}

static void zz_write_pixel(zz9000_state *data, uae_u32 base, uae_u32 pitch,
	int x, int y, int color_mode, uae_u32 color, uae_u8 mask = 0xff)
{
	uae_u8 *p;
	if (!zz_pixel_address(data, base, pitch, x, y, color_mode, &p))
		return;
	if (color_mode == ZZ_COLOR_CLUT) {
		const uae_u8 pen = static_cast<uae_u8>(color >> 24);
		p[0] = static_cast<uae_u8>((p[0] & ~mask) | (pen & mask));
	} else if (zz_bytes_per_pixel(color_mode) == 2) {
		p[0] = static_cast<uae_u8>(color);
		p[1] = static_cast<uae_u8>(color >> 8);
	} else {
		zz_write_le32(p, color);
	}
	data->modified = true;
}

static void zz_write_native_pixel(uae_u8 *p, int color_mode, int red, int green, int blue)
{
	red = std::max(0, std::min(255, red));
	green = std::max(0, std::min(255, green));
	blue = std::max(0, std::min(255, blue));
	if (color_mode == ZZ_COLOR_BGRA32) {
		p[0] = static_cast<uae_u8>(blue);
		p[1] = static_cast<uae_u8>(green);
		p[2] = static_cast<uae_u8>(red);
		p[3] = 0xff;
	} else {
		const uae_u16 value = color_mode == ZZ_COLOR_RGB565
			? static_cast<uae_u16>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3))
			: static_cast<uae_u16>(((red >> 3) << 10) | ((green >> 3) << 5) | (blue >> 3));
		p[0] = static_cast<uae_u8>(value >> 8);
		p[1] = static_cast<uae_u8>(value);
	}
}

static void zz_fill_rect(zz9000_state *data, uae_u32 destination, uae_u32 pitch,
	int x, int y, int width, int height, int color_mode, uae_u32 color, uae_u8 mask)
{
	if (width <= 0 || height <= 0)
		return;
	for (int row = 0; row < height; ++row) {
		for (int column = 0; column < width; ++column)
			zz_write_pixel(data, destination, pitch, x + column, y + row,
				color_mode, color, mask);
	}
}

static void zz_invert_rect(zz9000_state *data, uae_u32 destination, uae_u32 pitch,
	int x, int y, int width, int height, int color_mode, uae_u8 mask)
{
	if (width <= 0 || height <= 0)
		return;
	const int bpp = zz_bytes_per_pixel(color_mode);
	for (int row = 0; row < height; ++row) {
		for (int column = 0; column < width; ++column) {
			uae_u8 *p;
			if (!zz_pixel_address(data, destination, pitch, x + column, y + row,
				color_mode, &p))
				continue;
			if (bpp == 1)
				p[0] ^= mask;
			else
				for (int byte = 0; byte < bpp; ++byte)
					p[byte] ^= 0xff;
			data->modified = true;
		}
	}
}

static void zz_copy_rect(zz9000_state *data,
	uae_u32 source, uae_u32 source_pitch, int source_x, int source_y,
	uae_u32 destination, uae_u32 destination_pitch, int destination_x, int destination_y,
	int width, int height, int color_mode, uae_u8 mask, int minterm)
{
	const int bpp = zz_bytes_per_pixel(color_mode);
	if (!bpp || width <= 0 || height <= 0)
		return;
	const uae_u64 buffer_size64 = static_cast<uae_u64>(width) * height * bpp;
	if (!buffer_size64 || buffer_size64 > 0x10000000)
		return;
	const size_t buffer_size = static_cast<size_t>(buffer_size64);
	uae_u8 *temporary = xmalloc(uae_u8, buffer_size);
	if (!temporary)
		return;
	memset(temporary, 0, buffer_size);

	for (int row = 0; row < height; ++row) {
		for (int column = 0; column < width; ++column) {
			uae_u8 *p;
			if (zz_pixel_address(data, source, source_pitch, source_x + column,
				source_y + row, color_mode, &p))
				memcpy(temporary + (static_cast<size_t>(row) * width + column) * bpp, p, bpp);
		}
	}
	for (int row = 0; row < height; ++row) {
		for (int column = 0; column < width; ++column) {
			uae_u8 *destination_pixel;
			if (!zz_pixel_address(data, destination, destination_pitch,
				destination_x + column, destination_y + row, color_mode, &destination_pixel))
				continue;
			const uae_u8 *source_pixel = temporary +
				(static_cast<size_t>(row) * width + column) * bpp;
			if (bpp == 1 && minterm == ZZ_MINTERM_SRC) {
				destination_pixel[0] = static_cast<uae_u8>((destination_pixel[0] & ~mask) |
					(source_pixel[0] & mask));
			} else {
				for (int byte = 0; byte < bpp; ++byte)
					destination_pixel[byte] = static_cast<uae_u8>(zz_apply_minterm(
						source_pixel[byte], destination_pixel[byte], minterm));
			}
			data->modified = true;
		}
	}
	xfree(temporary);
}

static void zz_template_rect(zz9000_state *data, uae_u32 source, uae_u32 source_pitch,
	uae_u32 destination, uae_u32 destination_pitch, int x, int y, int width, int height,
	int source_x, int pattern_y, int loop_rows, int color_mode, int draw_mode,
	uae_u32 foreground, uae_u32 background, uae_u8 mask, bool pattern)
{
	// A zero source pitch is legal for a template: BytesPerRow is the offset
	// from one template line to the next, not a size, so 0 means the single
	// line repeats down the whole rectangle. source_row * source_pitch below
	// degenerates to row 0 on its own. Patterns still need loop_rows, which
	// the modulo divides by.
	if (width <= 0 || height <= 0 || (pattern && !loop_rows))
		return;
	for (int row = 0; row < height; ++row) {
		// Picasso96 templates have no Y offset. This field is only valid for
		// patterns and can contain stale pattern state during a template blit.
		const int source_row = pattern ? (row + pattern_y) % loop_rows : row;
		for (int column = 0; column < width; ++column) {
			const int bit_x = pattern ? (source_x + column) & 15 : source_x + column;
			const uae_u32 byte_offset = source + source_row * source_pitch + bit_x / 8;
			const uae_u8 *source_byte = zz_memory_ptr(data, byte_offset);
			if (!source_byte)
				continue;
			bool set = (*source_byte & (0x80 >> (bit_x & 7))) != 0;
			if (draw_mode & 4)
				set = !set;
			const int base_mode = draw_mode & 3;
			if (base_mode & COMP) {
				if (set) {
					uae_u8 *pixel;
					if (zz_pixel_address(data, destination, destination_pitch,
						x + column, y + row, color_mode, &pixel)) {
						const int bpp = zz_bytes_per_pixel(color_mode);
						if (bpp == 1)
							pixel[0] ^= mask;
						else
							for (int byte = 0; byte < bpp; ++byte)
								pixel[byte] ^= 0xff;
						data->modified = true;
					}
				}
			} else if (set) {
				zz_write_pixel(data, destination, destination_pitch, x + column,
					y + row, color_mode, foreground, mask);
			} else if (base_mode == 1) {
				zz_write_pixel(data, destination, destination_pitch, x + column,
					y + row, color_mode, background, mask);
			}
		}
	}
}

static void zz_draw_line(zz9000_state *data, uae_u32 destination, uae_u32 pitch,
	int x, int y, int delta_x, int delta_y, int length, int error_seed,
	uae_u16 pattern, int pattern_offset, int color_mode, int draw_mode,
	uae_u32 foreground, uae_u32 background, uae_u8 mask)
{
	if (length < 0)
		return;
	if (draw_mode & 4)
		pattern ^= 0xffff;
	const bool complement = (draw_mode & 2) != 0;
	const int jam_mode = draw_mode & 1;
	const int step_x = delta_x < 0 ? -1 : 1;
	const int step_y = delta_y < 0 ? -1 : 1;
	const int dx = std::abs(delta_x);
	const int dy = std::abs(delta_y);
	const int major = std::max(dx, dy);
	const int minor = std::min(dx, dy);
	int error = error_seed;
	uae_u16 pattern_bit = static_cast<uae_u16>(0x8000 >> (pattern_offset & 15));

	for (int pixel_number = 0; pixel_number <= length; ++pixel_number) {
		const bool set = (pattern & pattern_bit) != 0;
		if (complement && (set || jam_mode)) {
			uae_u8 *pixel;
			if (zz_pixel_address(data, destination, pitch, x, y, color_mode, &pixel)) {
				const int bpp = zz_bytes_per_pixel(color_mode);
				if (bpp == 1)
					pixel[0] ^= mask;
				else
					for (int byte = 0; byte < bpp; ++byte)
						pixel[byte] ^= 0xff;
				data->modified = true;
			}
		} else if (set) {
			zz_write_pixel(data, destination, pitch, x, y, color_mode, foreground, mask);
		} else if (jam_mode) {
			zz_write_pixel(data, destination, pitch, x, y, color_mode, background, mask);
		}
		pattern_bit >>= 1;
		if (!pattern_bit)
			pattern_bit = 0x8000;
		if (pixel_number == length || !major)
			continue;
		error += minor;
		const bool minor_step = error >= major;
		if (minor_step)
			error -= major;
		if (dx >= dy) {
			x += step_x;
			if (minor_step)
				y += step_y;
		} else {
			y += step_y;
			if (minor_step)
				x += step_x;
		}
	}
}

static void zz_planar_rect(zz9000_state *data, bool direct,
	uae_u32 source, uae_u32 source_pitch, uae_u32 destination, uae_u32 destination_pitch,
	int source_x, int destination_x, int destination_y, int width, int height,
	int planes, uae_u8 plane_mask, uae_u8 mask, int minterm,
	int color_mode, uae_u32 color_mask)
{
	if (!source_pitch || width <= 0 || height <= 0 || planes <= 0)
		return;
	planes = std::min(planes, 8);
	const uae_u32 palette_size = direct ? 256 * 4 : 0;
	const uae_u32 plane_size = source_pitch * height;
	for (int row = 0; row < height; ++row) {
		for (int column = 0; column < width; ++column) {
			uae_u8 pen = 0;
			for (int plane = 0; plane < planes; ++plane) {
				if (!(plane_mask & (1 << plane)))
					continue;
				const int bit_x = source_x + column;
				const uae_u32 byte_offset = source + palette_size + plane * plane_size +
					row * source_pitch + (bit_x / 8) % source_pitch;
				const uae_u8 *p = zz_memory_ptr(data, byte_offset);
				if (p && (*p & (0x80 >> (bit_x & 7))))
					pen |= static_cast<uae_u8>(1 << plane);
			}
			if (!direct) {
				uae_u8 *destination_pixel;
				if (zz_pixel_address(data, destination, destination_pitch,
					destination_x + column, destination_y + row, ZZ_COLOR_CLUT,
					&destination_pixel)) {
					const uae_u8 result = static_cast<uae_u8>(zz_apply_minterm(
						pen, destination_pixel[0], minterm));
					destination_pixel[0] = static_cast<uae_u8>((destination_pixel[0] & ~mask) |
						(result & mask));
					data->modified = true;
				}
			} else {
				const bool direct_copy = minterm == ZZ_MINTERM_SRC || minterm == ZZ_MINTERM_NOTSRC;
				const uae_u8 palette_index = minterm == ZZ_MINTERM_NOTSRC
					? static_cast<uae_u8>(~pen) : pen;
				const uae_u8 *palette = zz_memory_ptr(data, source + palette_index * 4, 4);
				if (!palette)
					continue;
				const uae_u32 color = zz_read_le32(palette);
				const uae_u32 old_color = zz_read_pixel(data, destination, destination_pitch,
					destination_x + column, destination_y + row, color_mode);
				uae_u32 result = direct_copy ? color : zz_apply_minterm(color, old_color, minterm);
				result = (old_color & ~color_mask) | (result & color_mask);
				zz_write_pixel(data, destination, destination_pitch,
					destination_x + column, destination_y + row, color_mode, result);
			}
		}
	}
}

struct zz_yuv_layout {
	uae_u8 y0, y1, u, v;
};

static constexpr zz_yuv_layout zz_yuv_layouts[5] = {
	{ 0, 2, 1, 3 },
	{ 2, 0, 3, 1 },
	{ 1, 3, 0, 2 },
	{ 0, 1, 2, 3 },
	{ 3, 2, 1, 0 }
};

static void zz_yuv_to_rgb(const uae_u8 *macropixel, int variant, bool second,
	int *red, int *green, int *blue)
{
	const auto &layout = zz_yuv_layouts[variant];
	const int y = macropixel[second ? layout.y1 : layout.y0] - 16;
	const int u = macropixel[layout.u] - 128;
	const int v = macropixel[layout.v] - 128;
	*red = (298 * y + 409 * v + 128) >> 8;
	*green = (298 * y - 100 * u - 208 * v + 128) >> 8;
	*blue = (298 * y + 516 * u + 128) >> 8;
}

static void zz_yuv_rect(zz9000_state *data, uae_u32 source, uae_u32 source_pitch,
	uae_u32 destination, uae_u32 destination_pitch, int source_phase,
	int destination_x, int destination_y, int width, int height, int variant, int color_mode)
{
	if (variant < 0 || variant >= 5 || destination_x < 0 || destination_y < 0 ||
		width <= 0 || height <= 0 ||
		color_mode == ZZ_COLOR_CLUT)
		return;
	for (int row = 0; row < height; ++row) {
		for (int column = 0; column < width; ++column) {
			const int source_pixel = source_phase + column;
			const uae_u8 *macropixel = zz_memory_ptr(data, source + row * source_pitch +
				(source_pixel / 2) * 4, 4);
			uae_u8 *destination_pixel;
			if (!macropixel || !zz_pixel_address(data, destination, destination_pitch,
				destination_x + column, destination_y + row, color_mode, &destination_pixel))
				continue;
			int red, green, blue;
			zz_yuv_to_rgb(macropixel, variant, (source_pixel & 1) != 0, &red, &green, &blue);
			zz_write_native_pixel(destination_pixel, color_mode, red, green, blue);
			data->modified = true;
		}
	}
}

static void zz_acc_copy_bytes(zz9000_state *data, uae_u32 source, uae_u32 source_pitch,
	uae_u32 destination, uae_u32 destination_pitch, int destination_x_bytes, int destination_y,
	int width_bytes, int height, int draw_mode, uae_u8 mask_color)
{
	if (width_bytes <= 0 || height <= 0 || destination_x_bytes < 0 || destination_y < 0)
		return;
	const uae_u64 temporary_size64 = static_cast<uae_u64>(width_bytes) * height;
	if (temporary_size64 > 0x10000000)
		return;
	const size_t temporary_size = static_cast<size_t>(temporary_size64);
	uae_u8 *temporary = xmalloc(uae_u8, temporary_size);
	if (!temporary)
		return;
	memset(temporary, 0, temporary_size);
	for (int row = 0; row < height; ++row) {
		const uae_u8 *source_row = zz_memory_ptr(data, source + row * source_pitch,
			static_cast<uae_u32>(width_bytes));
		if (source_row)
			memcpy(temporary + static_cast<size_t>(row) * width_bytes,
				source_row, width_bytes);
	}
	for (int row = 0; row < height; ++row) {
		uae_u8 *destination_row = zz_memory_ptr(data, destination +
			static_cast<uae_u32>(destination_y + row) * destination_pitch + destination_x_bytes,
			static_cast<uae_u32>(width_bytes));
		if (!destination_row)
			continue;
		const uae_u8 *source_row = temporary + static_cast<size_t>(row) * width_bytes;
		if (draw_mode == 2) {
			for (int column = 0; column < width_bytes; ++column) {
				if (source_row[column] != mask_color)
					destination_row[column] = source_row[column];
			}
		} else {
			memcpy(destination_row, source_row, width_bytes);
		}
		data->modified = true;
	}
	xfree(temporary);
}

static void zz_acc_draw_line(zz9000_state *data, uae_u32 destination, uae_u32 pitch,
	int x0, int y0, int x1, int y1, uae_u32 color, int bpp, int pen_width)
{
	const int color_mode = zz_color_mode_from_bpp(bpp);
	if (color_mode < 0 || pen_width <= 0)
		return;
	const int dx = std::abs(x1 - x0);
	const int sx = x0 < x1 ? 1 : -1;
	const int dy = -std::abs(y1 - y0);
	const int sy = y0 < y1 ? 1 : -1;
	int error = dx + dy;
	for (;;) {
		zz_fill_rect(data, destination, pitch, x0, y0, pen_width, pen_width,
			color_mode, color, 0xff);
		if (x0 == x1 && y0 == y1)
			break;
		const int twice_error = error * 2;
		if (twice_error >= dy) {
			error += dy;
			x0 += sx;
		}
		if (twice_error <= dx) {
			error += dx;
			y0 += sy;
		}
	}
}

static uae_u32 zz_surface_allocate(zz9000_state *data, uae_u32 requested_size)
{
	if (!requested_size || data->surface_count >= ZZ9000_MAX_SURFACES)
		return 0;
	const uae_u32 size = (requested_size + 255) & ~255U;
	if (!size || size > ZZ9000_SURFACE_HEAP_SIZE)
		return 0;
	uae_u32 candidate = ZZ9000_SURFACE_HEAP;
	int insert_at = data->surface_count;
	for (int i = 0; i < data->surface_count; ++i) {
		if (data->surfaces[i].offset - candidate >= size) {
			insert_at = i;
			break;
		}
		candidate = data->surfaces[i].offset + data->surfaces[i].size;
	}
	if (static_cast<uae_u64>(candidate) + size >
		static_cast<uae_u64>(ZZ9000_SURFACE_HEAP) + ZZ9000_SURFACE_HEAP_SIZE)
		return 0;
	uae_u8 *memory = zz_memory_ptr(data, candidate, size);
	if (!memory)
		return 0;
	memmove(&data->surfaces[insert_at + 1], &data->surfaces[insert_at],
		(data->surface_count - insert_at) * sizeof(data->surfaces[0]));
	data->surfaces[insert_at].offset = candidate;
	data->surfaces[insert_at].size = size;
	data->surface_count++;
	memset(memory, 0, size);
	return candidate;
}

static bool zz_surface_free(zz9000_state *data, uae_u32 offset)
{
	for (int i = 0; i < data->surface_count; ++i) {
		if (data->surfaces[i].offset != offset)
			continue;
		memmove(&data->surfaces[i], &data->surfaces[i + 1],
			(data->surface_count - i - 1) * sizeof(data->surfaces[0]));
		data->surface_count--;
		return true;
	}
	return false;
}

static void zz_refresh_host_palette(zz9000_state *data)
{
	auto &state = picasso96_state[data->monitor_id];
	picasso_palette(state.CLUT, picasso_vidinfo[data->monitor_id].clut);
}

static void zz_activate_palette(zz9000_state *data, const MyCLUTEntry *palette)
{
	memcpy(picasso96_state[data->monitor_id].CLUT, palette,
		sizeof data->primary_palette);
	zz_refresh_host_palette(data);
}

static void zz_update_palette(zz9000_state *data, int index, int red, int green, int blue,
	bool secondary = false)
{
	index &= 255;
	auto &entry = secondary ? data->secondary_palette[index] : data->primary_palette[index];
	entry.Red = static_cast<uae_u8>(red);
	entry.Green = static_cast<uae_u8>(green);
	entry.Blue = static_cast<uae_u8>(blue);
	entry.Pad = 0;
	if (!secondary) {
		picasso96_state[data->monitor_id].CLUT[index] = entry;
		zz_refresh_host_palette(data);
	}
	data->modified = true;
}

static void zz_set_pan(zz9000_state *data, uae_u32 base, int x, int y,
	uae_u32 width, int color_mode)
{
	const int shift = color_mode == ZZ_COLOR_RGB555 ? 1 : color_mode;
	const int64_t offset = static_cast<int64_t>(base) +
		(static_cast<int64_t>(x) << shift) +
		static_cast<int64_t>(y) * (static_cast<int64_t>(width) << shift);
	data->pan_offset = offset < 0 ? 0 : static_cast<uae_u32>(offset);
	data->pan_width = width;
	data->modified = true;
}

static void zz_decode_sprite(zz9000_state *data, uae_u32 source, int width_field,
	int height_field)
{
	const bool doubled = (width_field & 0x100) != 0;
	const bool hires = (height_field & 0x100) != 0;
	const int width = std::min(width_field & 0xff, 32);
	const int height = std::min(height_field & 0xff, 48);
	memset(data->sprite_pixels, 0, sizeof data->sprite_pixels);
	const int full_source_pitch = (std::max(8, width_field & 0xff) / 8) * 2;
	const int source_pitch = doubled && !hires ? full_source_pitch / 2 : full_source_pitch;
	const int output_scale = doubled ? 2 : 1;
	const int source_width = doubled ? width / 2 : width;
	const int source_height = doubled ? height / 2 : height;
	for (int row = 0; row < source_height; ++row) {
		for (int column = 0; column < source_width; ++column) {
			const uae_u8 *plane0 = zz_memory_ptr(data, source + row * source_pitch + column / 8);
			const uae_u8 *plane1 = zz_memory_ptr(data, source + row * source_pitch +
				source_pitch / 2 + column / 8);
			if (!plane0 || !plane1)
				continue;
			const int bit = 0x80 >> (column & 7);
			const uae_u8 pen = ((*plane0 & bit) ? 1 : 0) | ((*plane1 & bit) ? 2 : 0);
			for (int sy = 0; sy < output_scale; ++sy) {
				for (int sx = 0; sx < output_scale; ++sx) {
					const int output_x = column * output_scale + sx;
					const int output_y = row * output_scale + sy;
					if (output_x < 32 && output_y < 48)
						data->sprite_pixels[output_y * 32 + output_x] = pen;
				}
			}
		}
	}
	data->sprite_width = std::min(source_width * output_scale, 32);
	data->sprite_height = std::min(source_height * output_scale, 48);
	data->modified = true;
}

static void zz_handle_overlay(zz9000_state *data)
{
	auto &overlay = data->overlay;
	const int subcommand = zz_gd_u8(data, 48 + 5);
	if (subcommand == 1) {
		memset(&overlay, 0, sizeof overlay);
		zz_gd_write_u32(data, 60, 0);
		data->modified = true;
		return;
	}
	overlay.source_offset = zz_gd_u32(data, 4);
	overlay.source_pitch = zz_gd_u16(data, 42);
	overlay.destination_x = static_cast<int16_t>(zz_gd_u16(data, 16));
	overlay.destination_y = static_cast<int16_t>(zz_gd_u16(data, 24));
	overlay.destination_width = static_cast<int16_t>(zz_gd_u16(data, 18));
	overlay.destination_height = static_cast<int16_t>(zz_gd_u16(data, 26));
	overlay.source_width = zz_gd_u16(data, 20);
	overlay.source_height = zz_gd_u16(data, 28);
	const uae_u16 flags = zz_gd_u16(data, 32);
	overlay.variant = zz_gd_u8(data, 48 + 4);
	overlay.key_enabled = (flags & 1) != 0;
	overlay.active = (flags & 2) != 0;
	const uae_u32 raw_key = zz_gd_pen(data, 64);
	overlay.color_key = data->color_mode == ZZ_COLOR_BGRA32 ? raw_key : raw_key >> 16;
	overlay.mode_width = data->width;
	overlay.mode_height = data->height;
	overlay.mode_color = data->color_mode;
	overlay.mode_scale = data->scale_mode;
	overlay.mode_pitch = data->pan_width * zz_bytes_per_pixel(data->color_mode);

	uae_u32 status = 0;
	const uae_u64 source_size = static_cast<uae_u64>(overlay.source_pitch) * overlay.source_height;
	if (!overlay.source_width || !overlay.source_height ||
		(overlay.active && (overlay.destination_width < 1 || overlay.destination_height < 1)) ||
		overlay.variant >= 5 ||
		!data->width || !data->height || !data->pan_width ||
		data->scale_mode != 0 || data->color_mode == ZZ_COLOR_CLUT ||
		overlay.source_pitch < ((overlay.source_width + 1) / 2) * 4 ||
		overlay.source_offset >= ZZ9000_SURFACE_HEAP_END ||
		source_size > ZZ9000_SURFACE_HEAP_END - overlay.source_offset ||
		!zz_memory_ptr(data, overlay.source_offset,
			source_size > 0xffffffffU ? 0xffffffffU : static_cast<uae_u32>(source_size)))
		status = 1;
	if (!status) {
		overlay.configured = true;
		overlay.mode_stale = false;
		data->modified = true;
	} else {
		overlay.configured = false;
		overlay.active = false;
	}
	zz_gd_write_u32(data, 60, status);
}

static void zz_handle_dma(zz9000_state *data, int operation)
{
	const int color_mode = zz_gd_u8(data, 48);
	const uae_u8 draw_mode = zz_gd_u8(data, 49);
	const uae_u8 mask = zz_gd_u8(data, 57);
	const uae_u8 minterm = zz_gd_u8(data, 58);
	const uae_u32 destination = zz_gd_u32(data, 0);
	const uae_u32 source = zz_gd_u32(data, 4);
	const uae_u32 foreground = zz_gd_pen(data, 8);
	const uae_u32 background = zz_gd_pen(data, 12);
	const int x0 = static_cast<int16_t>(zz_gd_u16(data, 16));
	const int x1 = static_cast<int16_t>(zz_gd_u16(data, 18));
	const int x2 = static_cast<int16_t>(zz_gd_u16(data, 20));
	const int y0 = static_cast<int16_t>(zz_gd_u16(data, 24));
	const int y1 = static_cast<int16_t>(zz_gd_u16(data, 26));
	const int y2 = static_cast<int16_t>(zz_gd_u16(data, 28));
	const uae_u16 user0 = zz_gd_u16(data, 32);
	const uae_u16 user1 = zz_gd_u16(data, 34);
	const uae_u16 user3 = zz_gd_u16(data, 38);
	const uae_u32 destination_pitch = zz_gd_u16(data, 40);
	const uae_u32 source_pitch = zz_gd_u16(data, 42);

	// The driver uses longword pitches for normal framebuffer operations, matching
	// the firmware's set_fb() contract. Template/pattern framebuffer pitches and
	// planar/YUV source pitches are the exceptions: those are already in bytes.

	switch (operation) {
		case ZZ_OP_DRAWLINE:
			zz_draw_line(data, destination, destination_pitch * 4,
				x0, y0, x1, y1, user0, static_cast<int16_t>(user3), user1,
				zz_gd_u8(data, 50), color_mode, draw_mode, foreground, background, mask);
			break;
		case ZZ_OP_FILLRECT:
			zz_fill_rect(data, destination, destination_pitch * 4,
				x0, y0, x1, y1, color_mode, foreground, mask);
			break;
		case ZZ_OP_COPYRECT:
			zz_copy_rect(data, destination, destination_pitch * 4, x2, y2,
				destination, destination_pitch * 4, x0, y0, x1, y1,
				color_mode, mask, ZZ_MINTERM_SRC);
			break;
		case ZZ_OP_COPYRECT_NOMASK:
			zz_copy_rect(data, source, source_pitch * 4, x2, y2,
				destination, destination_pitch * 4, x0, y0, x1, y1,
				color_mode, 0xff, minterm);
			break;
		case ZZ_OP_RECT_TEMPLATE:
		case ZZ_OP_RECT_PATTERN:
			zz_template_rect(data, source,
				operation == ZZ_OP_RECT_PATTERN ? 2 : source_pitch, destination,
				destination_pitch, x0, y0, x1, y1, x2, y2,
				operation == ZZ_OP_RECT_PATTERN ? user0 : 0, color_mode,
				draw_mode, foreground, background, mask,
				operation == ZZ_OP_RECT_PATTERN);
			break;
		case ZZ_OP_P2C:
		case ZZ_OP_P2D:
			zz_planar_rect(data, operation == ZZ_OP_P2D, source, source_pitch,
				destination, destination_pitch * 4, x0, x1, y1, x2, y2,
				user1, static_cast<uae_u8>(user0), mask, minterm, color_mode,
				foreground);
			break;
		case ZZ_OP_INVERTRECT:
			zz_invert_rect(data, destination, destination_pitch * 4,
				x0, y0, x1, y1, color_mode, mask);
			break;
		case ZZ_OP_PAN:
			zz_set_pan(data, destination, x0, y0, x1, color_mode);
			data->sprite_x_offset = x0;
			data->sprite_y_offset = y0;
			break;
		case ZZ_OP_SPRITE_XY:
			data->sprite_x = x0;
			data->sprite_y = y0;
			data->modified = true;
			break;
		case ZZ_OP_SPRITE_COLOR: {
			const int index = zz_gd_u8(data, 59);
			if (index < 4) {
				data->sprite_colors[index] = foreground & 0x00ffffff;
				if (index && data->sprite_colors[index] == 0x00ff00ff)
					data->sprite_colors[index] = 0x00fe00fe;
			}
			data->modified = true;
			break;
		}
		case ZZ_OP_SPRITE_BITMAP:
			data->sprite_x_offset = x0;
			data->sprite_y_offset = y0;
			zz_decode_sprite(data, source, x1, y1);
			break;
		case ZZ_OP_SET_PALETTE: {
			const int start = user0;
			const int count = std::min<int>(user1, 256);
			const uae_u8 *palette = zz_gfxdata_ptr(data, 92, count * 3);
			if (palette) {
				const bool secondary = zz_gd_u8(data, 48) != 0;
				for (int i = 0; i < count; ++i)
					zz_update_palette(data, start + i, palette[i * 3], palette[i * 3 + 1],
						palette[i * 3 + 2], secondary);
			}
			break;
		}
		case ZZ_OP_SET_SPLIT_POS:
			data->split_offset = destination;
			data->split_position = y0;
			data->modified = true;
			break;
		case ZZ_OP_WRITE_YUV:
			zz_yuv_rect(data, source, source_pitch, destination,
				destination_pitch * 4, x0, x1, y1, x2, y2,
				zz_gd_u8(data, 52), color_mode);
			break;
		case ZZ_OP_VIDEO_OVERLAY:
			zz_handle_overlay(data);
			break;
		default:
			break;
	}
}

static void zz_handle_acc(zz9000_state *data, int operation)
{
	const uae_u32 destination = zz_gd_u32(data, 0);
	const uae_u32 source = zz_gd_u32(data, 4);
	const int source_bpp = zz_gd_u8(data, 48);
	const int destination_bpp = zz_gd_u8(data, 49);
	switch (operation) {
		case ZZ_ACC_ALLOC_SURFACE: {
			const uae_u32 size = zz_gd_u8(data, 49) == 1
				? source : static_cast<uae_u32>(zz_gd_u16(data, 16)) *
					zz_gd_u16(data, 24) * zz_gd_u8(data, 48);
			zz_gd_write_u32(data, 0, zz_surface_allocate(data, size));
			break;
		}
		case ZZ_ACC_FREE_SURFACE:
			zz_surface_free(data, destination);
			zz_gd_write_u32(data, 0, 0);
			break;
		case ZZ_ACC_BUFFER_CLEAR:
			if (const int color_mode = zz_color_mode_from_bpp(source_bpp); color_mode >= 0)
				zz_fill_rect(data, destination, zz_gd_u16(data, 40) * source_bpp, 0, 0,
					zz_gd_u16(data, 16), zz_gd_u16(data, 24), color_mode,
					zz_gd_pen(data, 8), 0xff);
			break;
		case ZZ_ACC_BUFFER_FLIP: {
			const uae_u32 line_size = static_cast<uae_u32>(zz_gd_u16(data, 40)) * source_bpp;
			zz_acc_copy_bytes(data, source, line_size, destination, line_size, 0, 0,
				line_size, zz_gd_u16(data, 24), 0, 0);
			break;
		}
		case ZZ_ACC_BLIT_RECT: {
			const int destination_x = zz_gd_u16(data, 16);
			const int destination_y = zz_gd_u16(data, 24);
			const int width = zz_gd_u16(data, 18);
			const int height = zz_gd_u16(data, 26);
			if (source_bpp == 2 && destination_bpp == 1) {
				for (int row = 0; row < height; ++row) {
					const uae_u8 *source_row = zz_memory_ptr(data,
						source + row * zz_gd_u16(data, 42) * 2, width * 2);
					uae_u8 *destination_row = zz_memory_ptr(data, destination +
						(destination_y + row) * zz_gd_u16(data, 40) + destination_x, width);
					if (!source_row || !destination_row)
						continue;
					for (int column = 0; column < width; ++column)
						destination_row[column] = data->color_conversion_table[
							zz_read_be16(source_row + column * 2)];
					data->modified = true;
				}
			} else {
				zz_acc_copy_bytes(data, source, zz_gd_u16(data, 42), destination,
					zz_gd_u16(data, 40), destination_x * destination_bpp, destination_y,
					width * source_bpp, height, zz_gd_u8(data, 50), zz_gd_u8(data, 59));
			}
			break;
		}
		case ZZ_ACC_SET_BPP_CONVERSION_TABLE: {
			const uae_u8 *table = zz_memory_ptr(data, destination,
				sizeof data->color_conversion_table);
			if (table)
				memcpy(data->color_conversion_table, table,
					sizeof data->color_conversion_table);
			break;
		}
		case ZZ_ACC_DRAW_LINE:
			zz_acc_draw_line(data, destination, zz_gd_u16(data, 40),
				static_cast<int16_t>(zz_gd_u16(data, 16)),
				static_cast<int16_t>(zz_gd_u16(data, 24)),
				static_cast<int16_t>(zz_gd_u16(data, 18)),
				static_cast<int16_t>(zz_gd_u16(data, 26)),
				zz_gd_pen(data, 8), source_bpp, destination_bpp);
			break;
		case ZZ_ACC_FILL_RECT:
			if (const int color_mode = zz_color_mode_from_bpp(source_bpp); color_mode >= 0)
				zz_fill_rect(data, destination, zz_gd_u16(data, 40),
					static_cast<int16_t>(zz_gd_u16(data, 16)),
					static_cast<int16_t>(zz_gd_u16(data, 24)),
					static_cast<int16_t>(zz_gd_u16(data, 18)),
					static_cast<int16_t>(zz_gd_u16(data, 26)), color_mode,
					zz_gd_pen(data, 8), 0xff);
			break;
		default:
			break;
	}
}

static void zz_set_mode(zz9000_state *data, uae_u16 value)
{
	int width, height;
	const int mode = value & 0xff;
	const int color_mode = (value >> 8) & 0x0f;
	const int scale_mode = (value >> 12) & 0x0f;
	if (!zz_mode_dimensions(mode, &width, &height) || !zz_bytes_per_pixel(color_mode))
		return;
	data->mode_id = mode;
	data->color_mode = color_mode;
	data->scale_mode = scale_mode;
	data->width = scale_mode & 1 ? width / 2 : width;
	data->height = scale_mode & 2 ? height / 2 : height;
	data->display_blank = false;
	// ZZ9000OS preserves the virtual framebuffer stride across SetGC calls.
	// SetPanning owns subsequent updates; only provide an initial fallback here.
	if (!data->pan_width)
		data->pan_width = data->width;
	data->mode_changed = true;
	data->modified = true;
	if (data->capture_mode) {
		data->capture_mode = false;
		gfxboard_set(data->monitor_id, true);
	}
}

static void zz_palette_write_z2(zz9000_state *data, int operation)
{
	if (operation != 3 && operation != 19)
		return;
	const int index = (data->palette_data >> 24) & 0xff;
	zz_update_palette(data, index, (data->palette_data >> 16) & 0xff,
		(data->palette_data >> 8) & 0xff, data->palette_data & 0xff, operation == 19);
}

static constexpr uae_u32 ZZ_NET_RX_WINDOW = 0x2000;
static constexpr uae_u32 ZZ_NET_TX_WINDOW = 0x8000;
static constexpr uae_u32 ZZ_NET_WINDOW_BYTES = 0x2000;
static constexpr uae_u8 ZZ_NET_DEFAULT_MAC[6] = {0x68, 0x82, 0xf2, 0x00, 0x01, 0x00};

struct zz9000_net_engine {
    struct packet {
        int length;
        std::array<uae_u8, zz9000_net::RxQueue::max_frame> bytes;
    };

    // Backend callbacks can arrive off the emulation thread. The card RX
    // window and interrupt state are updated only when hsync drains this queue.
    std::mutex incoming_mutex;
    std::deque<packet> incoming;
    uae_u32 incoming_drops = 0;
    zz9000_net::RxQueue rx;
    std::array<uae_u8, ZZ_NET_WINDOW_BYTES> tx{};
    uae_u8 mac[6] = {};
    netdriverdata *backend = nullptr;
    uae_u8 *backend_data = nullptr;
    int tx_length = 0;
    uae_u16 tx_status = 0;
    bool irq_enabled = false;
    bool irq_pending = false;
};

bool zz9000_net_board_present(void)
{
    for (const auto *board : zz9000_boards) {
        if (board && board->configured)
            return true;
    }
    return false;
}

bool zz9000_net_host_active(void)
{
    for (const auto *board : zz9000_boards) {
        if (board && board->configured && board->net && board->net->backend)
            return true;
    }
    return false;
}

static void zz_card_raise_interrupt(const zz9000_state *data)
{
    // PORTS is INT2 (bit 3); EXTER is INT6 (bit 13).
    INTREQ_0(0x8000 | (data->int2 ? 0x0008 : 0x2000));
}

static void zz_net_received(void *user, const uae_u8 *bytes, int length)
{
    auto *net = static_cast<zz9000_net_engine *>(user);
    if (!net || !bytes || length < 14 ||
        length > static_cast<int>(zz9000_net::RxQueue::max_frame))
        return;
    std::lock_guard<std::mutex> lock(net->incoming_mutex);
    if (net->incoming.size() >= zz9000_net::RxQueue::capacity) {
        ++net->incoming_drops;
        return;
    }
    zz9000_net_engine::packet packet{};
    packet.length = length;
    std::copy_n(bytes, length, packet.bytes.begin());
    net->incoming.push_back(packet);
}

static int zz_net_transmit(void *user, uae_u8 *bytes, int *length)
{
    auto *net = static_cast<zz9000_net_engine *>(user);
    if (!net || !bytes || !length || net->tx_length <= 0 ||
        net->tx_length > *length)
        return 0;
    std::copy_n(net->tx.begin(), net->tx_length, bytes);
    *length = net->tx_length;
    net->tx_length = 0;
    return 1;
}

static void zz_net_close(zz9000_net_engine *net)
{
    if (!net)
        return;
    if (net->backend && net->backend_data)
        ethernet_close(net->backend, net->backend_data);
    xfree(net->backend_data);
    net->backend_data = nullptr;
    net->backend = nullptr;
    net->tx_length = 0;
    std::lock_guard<std::mutex> lock(net->incoming_mutex);
    net->incoming.clear();
    net->incoming_drops = 0;
}

static void zz_net_open(zz9000_state *data, const TCHAR *name)
{
    auto *net = data->net;
    if (!net || !name || !name[0] || !_tcsicmp(name, _T("none")))
        return;
    netdriverdata *backend = ethernet_find_driver(name);
    if (backend) {
        const int data_length = ethernet_getdatalength(backend);
        if (data_length > 0) {
            net->backend_data = xcalloc(uae_u8, data_length);
            if (net->backend_data) {
                if (ethernet_open(backend, net->backend_data, net,
                                  zz_net_received, zz_net_transmit, 0, net->mac)) {
                    net->backend = backend;
                    write_log(_T("ZZ9000Net: host backend '%s' open\n"), name);
                    return;
                }
                xfree(net->backend_data);
                net->backend_data = nullptr;
            }
        }
    }
    write_log(_T("ZZ9000Net: host backend '%s' unavailable; card disconnected\n"), name);
}

static void zz_net_tick(zz9000_state *data)
{
    auto *net = data->net;
    if (!net)
        return;
    if (net->backend)
        ethernet_receive_poll(net->backend, net->backend_data);
    {
        std::lock_guard<std::mutex> lock(net->incoming_mutex);
        while (!net->incoming.empty()) {
            const auto &packet = net->incoming.front();
            net->rx.enqueue(packet.bytes.data(), packet.length);
            net->incoming.pop_front();
        }
        net->rx.record_drop(net->incoming_drops);
        net->incoming_drops = 0;
    }
    if (net->irq_enabled && net->rx.ready() && !net->irq_pending)
        net->irq_pending = true;
    // Exec can clear the shared Paula request after one interrupt server
    // accepts it. Reassert a second source still pending on the next hsync.
    if (net->irq_pending || data->audio_intreq_pending)
        zz_card_raise_interrupt(data);
}

static void zz_net_reset(zz9000_state *data)
{
    auto *net = data->net;
    if (!net)
        return;
    zz_net_close(net);
    net->rx.reset();
    net->tx.fill(0);
    std::copy_n(ZZ_NET_DEFAULT_MAC, 6, net->mac);
    net->tx_status = 0;
    net->irq_enabled = false;
    net->irq_pending = false;
    zz_net_open(data, currprefs.zz9000net_name);
}

// ZZ9000AX audio: 8-slot DMA ring in card memory (8 x 3840 bytes).
// Each slot is 20 ms of 16-bit stereo at 96 kHz. The host is the single
// producer (capture) / consumer (playback) of the ring; the Amiga-side
// driver only pokes the kick register and polls the packed status words.
static constexpr uae_u32 ZZ_AX_PERIOD_BYTES = 3840;
static constexpr uae_u32 ZZ_AX_PERIODS = 8;
static constexpr uae_u32 ZZ_AX_RING_BYTES = ZZ_AX_PERIODS * ZZ_AX_PERIOD_BYTES;

// Packed status word: bit 15 = formatter present, bits 12-14 = period
// index (0..7), bits 0-11 = completed sequence (0..4095).
static inline uae_u16 zz_ax_status(uae_u8 period, uae_u16 seq)
{
	return 0x8000u | static_cast<uae_u16>((period & 0x07u) << 12) | (seq & 0x0fffu);
}

// The SWAB kick value is the ring byte offset shifted right by 8 bits.
// A collision means the Amiga-side kick lands inside the host reader's
// active period, which the firmware reports as an overrun.
static inline bool zz_ax_collision(const zz9000_state *data, uae_u32 swab_value)
{
	const uae_u32 reader = static_cast<uae_u32>(data->audio_tx_read_period) * ZZ_AX_PERIOD_BYTES;
	const uae_u32 kick = ((swab_value & 0x7fffu) * 256u) % ZZ_AX_RING_BYTES;
	const uae_u32 d = (reader >= kick) ? (reader - kick) : (ZZ_AX_RING_BYTES - (kick - reader));
	return d < ZZ_AX_PERIOD_BYTES;
}

static void zz_ax_clear_interrupt(zz9000_state *data)
{
	// Exec clears the shared EXTER line after dispatching its server chain.
	// Card acknowledgement must only clear this device's pending state.
	data->audio_intreq_pending = false;
}

#if defined(AHI) && !defined(LIBRETRO)
// Card registers, ring memory, and the formatter advance on the emulation
// thread. This avoids racing guest writes and lets the physical EXTER
// interrupt number used by the stock driver work without driver changes.
struct zz_ax_audio_engine {
    SDL_AudioStream *play_stream = nullptr;
    SDL_AudioStream *rec_stream = nullptr;
    std::chrono::steady_clock::time_point next_period;
    std::chrono::steady_clock::time_point next_play_retry;
    std::chrono::steady_clock::time_point next_record_retry;
    uae_u32 rate = 48000;
    uae_u32 last_peak = 0;
    bool play_enabled = false;
    bool record_enabled = false;
    bool play_paused = false;
    float play_gain = -1.0f;
    int soundcard = -1;
    bool soundcard_default = true;
    int samplersoundcard = -1;
};

static constexpr uae_u32 ZZ_AX_MAX_FRAMES = ZZ_AX_PERIOD_BYTES / 4;

static uae_u32 zz_ax_frames(const zz9000_state *data)
{
    return data->audio_scale > 0 && data->audio_scale <= ZZ_AX_MAX_FRAMES
        ? data->audio_scale : ZZ_AX_MAX_FRAMES;
}

static void zz_ax_swap16(uae_u8 *samples, uae_u32 bytes)
{
    for (uae_u32 i = 0; i + 1 < bytes; i += 2)
        std::swap(samples[i], samples[i + 1]);
}

static void zz_ax_swap_stereo(uae_u8 *samples, uae_u32 bytes)
{
    for (uae_u32 i = 0; i + 3 < bytes; i += 4) {
        std::swap(samples[i], samples[i + 2]);
        std::swap(samples[i + 1], samples[i + 3]);
    }
}

static void zz_ax_period_pending(zz9000_state *data)
{
    if (data->audio_intreq_pending)
        return;
    data->audio_intreq_pending = true;
	// The installed AHI ISR uses the selected PORTS or EXTER line.
	zz_card_raise_interrupt(data);
    if (!(data->audio_log_flags & (1u << 2))) {
        data->audio_log_flags |= 1u << 2;
        write_log(_T("ZZ9000AX: audio interrupt pending (tx_seq=%u rx_seq=%u)\n"),
            data->audio_tx_seq, data->audio_rx_seq);
    }
}

static void zz_ax_audio_period(zz9000_state *data)
{
    zz_ax_audio_engine *eng = data->ax_audio;
    const uae_u32 bytes = zz_ax_frames(data) * 4;
    if (data->audio_play) {
        uae_u8 *slot = data->memory + data->audio_tx_ring_off +
            static_cast<uae_u32>(data->audio_tx_read_period) * ZZ_AX_PERIOD_BYTES;
        if (eng->play_stream && !eng->play_paused &&
            SDL_GetAudioStreamQueued(eng->play_stream) < static_cast<int>(bytes * ZZ_AX_PERIODS)) {
            if (currprefs.sound_stereo_swap_ahi) {
                uae_u8 samples[ZZ_AX_PERIOD_BYTES];
                memcpy(samples, slot, bytes);
                zz_ax_swap_stereo(samples, bytes);
                SDL_PutAudioStreamData(eng->play_stream, samples, static_cast<int>(bytes));
            } else {
                SDL_PutAudioStreamData(eng->play_stream, slot, static_cast<int>(bytes));
            }
        }
        uae_u32 peak = 0;
        for (uae_u32 i = 0; i + 1 < bytes; i += 2) {
            const uae_u32 sample = (slot[i + 1] << 8) | slot[i];
            peak = std::max(peak, (sample & 0x8000) ? 0x10000u - sample : sample);
        }
        eng->last_peak = peak;
        data->audio_tx_seq = (data->audio_tx_seq + 1) & 0x0fff;
        // TX status names the period that just finished, which the driver
        // can refill. The next read period is advanced only afterward.
        data->audio_tx_read_period = (data->audio_tx_read_period + 1) % ZZ_AX_PERIODS;
        zz_ax_period_pending(data);
    }
    if (data->audio_record) {
        uae_u8 samples[ZZ_AX_PERIOD_BYTES] = {};
        if (eng->rec_stream) {
            const int period_bytes = static_cast<int>(bytes);
            const int available = SDL_GetAudioStreamAvailable(eng->rec_stream);
            // A short scheduling delay is harmless. After a longer stall,
            // discard old input so the guest receives the latest period.
            if (available > period_bytes * 3) {
                int stale_bytes = (available - period_bytes) & ~3;
                while (stale_bytes > 0) {
                    const int count = std::min(stale_bytes, static_cast<int>(sizeof samples));
                    const int read = SDL_GetAudioStreamData(eng->rec_stream, samples, count);
                    if (read <= 0)
                        break;
                    stale_bytes -= read;
                }
                memset(samples, 0, bytes);
            }
            SDL_GetAudioStreamData(eng->rec_stream, samples, period_bytes);
        }
        if (currprefs.sound_stereo_swap_ahi)
            zz_ax_swap_stereo(samples, bytes);
        const uae_u32 filled =
            (static_cast<uae_u32>(data->audio_rx_write_period) + 1) % ZZ_AX_PERIODS;
        uae_u8 *slot = data->memory + data->audio_rx_ring_off + filled * ZZ_AX_PERIOD_BYTES;
        memcpy(slot, samples, bytes);
        zz_ax_swap16(slot, bytes);
        data->audio_rx_write_period = static_cast<uae_u8>(filled);
        data->audio_rx_seq = (data->audio_rx_seq + 1) & 0x0fff;
        zz_ax_period_pending(data);
    }
    if (data->audio_log_periods == 0 || data->audio_log_periods % 250 == 0)
        write_log(_T("ZZ9000AX: period %u tx_seq=%u rx_seq=%u peak=%u pending=%d\n"),
            data->audio_log_periods, data->audio_tx_seq, data->audio_rx_seq,
            eng->last_peak, data->audio_intreq_pending ? 1 : 0);
    data->audio_log_periods++;
}

static void zz_ax_audio_close(zz9000_state *data)
{
    auto *eng = data->ax_audio;
    if (!eng)
        return;
    if (eng->play_stream) {
        SDL_DestroyAudioStream(eng->play_stream);
        eng->play_stream = nullptr;
    }
    if (eng->rec_stream) {
        SDL_DestroyAudioStream(eng->rec_stream);
        eng->rec_stream = nullptr;
    }
    eng->play_paused = false;
    eng->play_gain = -1.0f;
}

static void zz_ax_audio_start(zz9000_state *data)
{
    if (!data->ax_audio)
        data->ax_audio = new zz_ax_audio_engine();
    auto *eng = data->ax_audio;
    const uae_u32 rate = zz_ax_frames(data) * 50;
    const auto now = std::chrono::steady_clock::now();
    const bool play_enabled = data->audio_play && currprefs.produce_sound >= 2;
    const bool starting = eng->next_period == std::chrono::steady_clock::time_point{};
    const bool rate_changed = eng->rate != rate;
    const bool play_changed = starting || rate_changed || eng->play_enabled != play_enabled ||
        (play_enabled && (eng->soundcard != currprefs.soundcard ||
            eng->soundcard_default != currprefs.soundcard_default ||
            (!eng->play_stream && now >= eng->next_play_retry)));
    const bool record_enabled = data->audio_record && currprefs.samplersoundcard >= 0;
    const bool record_changed = starting || rate_changed || eng->record_enabled != record_enabled ||
        (record_enabled && (eng->samplersoundcard != currprefs.samplersoundcard ||
            (!eng->rec_stream && now >= eng->next_record_retry)));
    if (!play_changed && !record_changed)
        return;
    if (play_changed && eng->play_stream) {
        SDL_DestroyAudioStream(eng->play_stream);
        eng->play_stream = nullptr;
        eng->play_paused = false;
    }
    if (record_changed && eng->rec_stream) {
        SDL_DestroyAudioStream(eng->rec_stream);
        eng->rec_stream = nullptr;
    }
    eng->rate = rate;
    eng->play_enabled = play_enabled;
    eng->record_enabled = record_enabled;
    eng->soundcard = currprefs.soundcard;
    eng->soundcard_default = currprefs.soundcard_default;
    eng->samplersoundcard = currprefs.samplersoundcard;
    SDL_AudioSpec spec = {};
    spec.format = SDL_AUDIO_S16LE;
    spec.channels = 2;
    spec.freq = static_cast<int>(rate);
    if ((play_changed && play_enabled) || (record_changed && record_enabled))
        enumerate_sound_devices();
    if (play_changed && play_enabled) {
        const int soundcard = currprefs.soundcard;
        const bool use_default_device = currprefs.soundcard_default || soundcard < 0 ||
            soundcard >= MAX_SOUND_DEVICES || sound_devices[soundcard] == nullptr;
        const SDL_AudioDeviceID device_id = use_default_device
            ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
            : static_cast<SDL_AudioDeviceID>(sound_devices[soundcard]->id);
        eng->play_stream = SDL_OpenAudioDeviceStream(
            device_id, &spec, nullptr, nullptr);
        if (!eng->play_stream && !use_default_device) {
            write_log(_T("ZZ9000AX: selected playback device open failed, retrying default: %s\n"), SDL_GetError());
            eng->play_stream = SDL_OpenAudioDeviceStream(
                SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        }
        if (eng->play_stream) {
            eng->next_play_retry = {};
            eng->play_paused = sound_paused();
            eng->play_gain = -1.0f;
            if (!eng->play_paused)
                SDL_ResumeAudioStreamDevice(eng->play_stream);
        } else {
            eng->next_play_retry = std::chrono::steady_clock::now() + std::chrono::seconds(1);
            write_log(_T("ZZ9000AX: playback device open failed: %s\n"), SDL_GetError());
        }
    }
    if (record_changed && record_enabled) {
        const int soundcard = currprefs.samplersoundcard;
        const bool use_default_device = soundcard >= MAX_SOUND_DEVICES ||
            record_devices[soundcard] == nullptr;
        const SDL_AudioDeviceID device_id = use_default_device
            ? SDL_AUDIO_DEVICE_DEFAULT_RECORDING
            : static_cast<SDL_AudioDeviceID>(record_devices[soundcard]->id);
        eng->rec_stream = SDL_OpenAudioDeviceStream(
            device_id, &spec, nullptr, nullptr);
        if (!eng->rec_stream && !use_default_device) {
            write_log(_T("ZZ9000AX: selected recording device open failed, retrying default: %s\n"), SDL_GetError());
            eng->rec_stream = SDL_OpenAudioDeviceStream(
                SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, nullptr, nullptr);
        }
        if (eng->rec_stream) {
            eng->next_record_retry = {};
            SDL_ResumeAudioStreamDevice(eng->rec_stream);
        } else {
            eng->next_record_retry = std::chrono::steady_clock::now() + std::chrono::seconds(1);
            write_log(_T("ZZ9000AX: recording device open failed: %s\n"), SDL_GetError());
        }
    }
    if (starting)
        eng->next_period = std::chrono::steady_clock::now() + std::chrono::milliseconds(20);
    write_log(_T("ZZ9000AX audio: %u Hz, play=%d record=%d\n"), rate,
        data->audio_play ? 1 : 0, data->audio_record ? 1 : 0);
}

static void zz_ax_audio_stop(zz9000_state *data)
{
    zz_ax_audio_close(data);
    if (data->ax_audio)
        data->ax_audio->next_period = {};
}

static void zz_ax_audio_tick(zz9000_state *data)
{
    auto *eng = data->ax_audio;
    if (!eng || (!data->audio_play && !data->audio_record))
        return;
    zz_ax_audio_start(data);
    if (eng->play_stream) {
        const float master_gain = (100 - std::clamp(currprefs.sound_volume_master, 0, 100)) / 100.0f;
        const float board_gain = (100 - std::clamp(currprefs.sound_volume_board, 0, 100)) / 100.0f;
        const float gain = sound_muted() ? 0.0f : master_gain * board_gain;
        if (gain != eng->play_gain && SDL_SetAudioStreamGain(eng->play_stream, gain))
            eng->play_gain = gain;
    }
    const bool paused = sound_paused();
    if (eng->play_stream && eng->play_paused != paused) {
        if (paused)
            SDL_PauseAudioStreamDevice(eng->play_stream);
        SDL_ClearAudioStream(eng->play_stream);
        eng->play_paused = paused;
        if (!paused)
            SDL_ResumeAudioStreamDevice(eng->play_stream);
    }
    const auto now = std::chrono::steady_clock::now();
    if (now < eng->next_period)
        return;
    eng->next_period += std::chrono::milliseconds(20);
    if (eng->next_period < now) {
        eng->next_period = now + std::chrono::milliseconds(20);
        // A full emulator pause stops hsync, so discard any host audio that
        // was queued before the pause when emulation resumes.
        if (eng->play_stream)
            SDL_ClearAudioStream(eng->play_stream);
    }
    zz_ax_audio_period(data);
}
#endif

static void zz_write_register(zz9000_state *data, uae_u32 offset, uae_u16 value)
{
	if (offset / 2 < sizeof data->registers / sizeof data->registers[0])
		data->registers[offset / 2] = value;

	switch (offset) {
		case ZZ_REG_MODE:
			zz_set_mode(data, value);
			break;
		case ZZ_REG_X1: data->x[0] = value; break;
		case ZZ_REG_Y1: data->y[0] = value; break;
		case ZZ_REG_X2: data->x[1] = value; break;
		case ZZ_REG_Y2: data->y[1] = value; break;
		case ZZ_REG_X3: data->x[2] = value; break;
		case ZZ_REG_Y3: data->y[2] = value; break;
		case ZZ_REG_USER1: data->user[0] = value; break;
		case ZZ_REG_USER2: data->user[1] = value; break;
		case ZZ_REG_USER3: data->user[2] = value; break;
		case ZZ_REG_USER4: data->user[3] = value; break;
		case ZZ_REG_ROW_PITCH: data->destination_pitch = value; break;
		case ZZ_REG_SRC_PITCH: data->source_pitch = value; break;
		case ZZ_REG_SRC_HI:
			data->source_offset = (data->source_offset & 0xffff) | (static_cast<uae_u32>(value) << 16);
			break;
		case ZZ_REG_SRC_LO:
			data->source_offset = (data->source_offset & 0xffff0000) | value;
			break;
		case ZZ_REG_DST_HI:
			data->destination_offset = (data->destination_offset & 0xffff) |
				(static_cast<uae_u32>(value) << 16);
			break;
		case ZZ_REG_DST_LO:
			data->destination_offset = (data->destination_offset & 0xffff0000) | value;
			break;
		case ZZ_REG_COLORMODE:
			data->blit_color_mode = value & 0x0f;
			data->blit_high = value >> 8;
			break;
		case ZZ_REG_RGB_HI:
			data->foreground = (data->foreground & 0xffff0000) | zz_bswap16(value);
			break;
		case ZZ_REG_RGB_LO:
			data->foreground = (data->foreground & 0x0000ffff) |
				(static_cast<uae_u32>(zz_bswap16(value)) << 16);
			break;
		case ZZ_REG_RGB2_HI:
			data->background = (data->background & 0xffff0000) | zz_bswap16(value);
			break;
		case ZZ_REG_RGB2_LO:
			data->background = (data->background & 0x0000ffff) |
				(static_cast<uae_u32>(zz_bswap16(value)) << 16);
			break;
		case ZZ_REG_PAN_HI:
			data->pan_offset = (data->pan_offset & 0xffff) | (static_cast<uae_u32>(value) << 16);
			break;
		case ZZ_REG_PAN_LO: {
			const uae_u32 base = (data->pan_offset & 0xffff0000) | value;
			zz_set_pan(data, base, static_cast<int16_t>(data->x[0]),
				static_cast<int16_t>(data->y[0]), data->x[1], data->blit_color_mode);
			data->sprite_x_offset = static_cast<int16_t>(data->x[0]);
			data->sprite_y_offset = static_cast<int16_t>(data->y[0]);
			break;
		}
		case ZZ_REG_FILLRECT:
			zz_fill_rect(data, data->destination_offset, data->destination_pitch * 4,
				data->x[0], data->y[0], data->x[1], data->y[1], data->blit_color_mode,
				data->foreground, value & 0xff);
			break;
		case ZZ_REG_COPYRECT:
			if (value == 1) {
				zz_copy_rect(data, data->destination_offset, data->destination_pitch * 4,
					data->x[2], data->y[2], data->destination_offset,
					data->destination_pitch * 4, data->x[0], data->y[0], data->x[1],
					data->y[1], data->blit_color_mode, data->blit_high, ZZ_MINTERM_SRC);
			} else if (value == 2) {
				zz_copy_rect(data, data->source_offset, data->source_pitch * 4,
					data->x[2], data->y[2], data->destination_offset,
					data->destination_pitch * 4, data->x[0], data->y[0], data->x[1],
					data->y[1], data->blit_color_mode, 0xff, data->blit_high);
			}
			break;
		case ZZ_REG_FILLTEMPLATE: {
			const bool pattern = (value & 0x8000) != 0;
			zz_template_rect(data, data->source_offset, pattern ? 2 : data->source_pitch,
				data->destination_offset, data->destination_pitch, data->x[0], data->y[0],
				data->x[1], data->y[1], data->x[2], data->y[2],
				pattern ? value & 0xff : 0, data->blit_color_mode, data->blit_high,
				data->foreground, data->background,
				pattern ? data->user[0] & 0xff : value & 0xff, pattern);
			break;
		}
		case ZZ_REG_P2C:
		case ZZ_REG_P2D:
			zz_planar_rect(data, offset == ZZ_REG_P2D, data->source_offset,
				data->source_pitch, data->destination_offset, data->destination_pitch * 4,
				data->x[0], data->x[1], data->y[1], data->x[2], data->y[2],
				value >> 8, data->user[1] & 0xff, value & 0xff, data->blit_high,
				data->blit_color_mode, data->foreground);
			break;
		case ZZ_REG_DRAWLINE:
			zz_draw_line(data, data->destination_offset, data->destination_pitch * 4,
				static_cast<int16_t>(data->x[0]), static_cast<int16_t>(data->y[0]),
				static_cast<int16_t>(data->x[1]), static_cast<int16_t>(data->y[1]),
				data->user[0], static_cast<int16_t>(data->user[2]), data->x[2],
				data->y[2] & 15, data->blit_color_mode, data->blit_high,
				data->foreground, data->background, value & 0xff);
			break;
		case ZZ_REG_INVERTRECT:
			zz_invert_rect(data, data->destination_offset, data->destination_pitch * 4,
				data->x[0], data->y[0], data->x[1], data->y[1], data->blit_color_mode,
				value & 0xff);
			break;
		case ZZ_REG_DMA_OP:
			zz_handle_dma(data, value);
			break;
		case ZZ_REG_ACC_OP:
			zz_handle_acc(data, value);
			break;
		case ZZ_REG_SPRITE_BITMAP:
			if (value == 1) {
				data->sprite_visible = true;
			} else if (value == 2) {
				data->sprite_visible = false;
			} else {
				data->sprite_x_offset = static_cast<int16_t>(data->x[0]);
				data->sprite_y_offset = static_cast<int16_t>(data->y[0]);
				zz_decode_sprite(data, data->source_offset, data->x[1], data->y[1]);
			}
			data->modified = true;
			break;
		case ZZ_REG_SPRITE_Y:
			if (data->sprite_visible) {
				data->sprite_x = static_cast<int16_t>(data->x[0]);
				data->sprite_y = static_cast<int16_t>(data->y[0]);
				data->modified = true;
			}
			break;
		case ZZ_REG_SPRITE_COLORS:
			if (value < 4) {
				data->sprite_colors[value] =
					(static_cast<uae_u32>(data->user[0]) << 16) | data->user[1];
				if (value && data->sprite_colors[value] == 0x00ff00ff)
					data->sprite_colors[value] = 0x00fe00fe;
			}
			data->modified = true;
			break;
		case ZZ_REG_SET_FEATURE:
			if (data->user[0] == 1)
				data->secondary_palette_feature = value != 0;
			else if (data->user[0] == 3)
				data->overlay_feature = value != 0;
			else if (data->user[0] == 4)
				data->display_blank = value != 0;
			data->modified = true;
			break;
		case ZZ_REG_CONFIG_KEY:
			data->config_key = value;
			break;
		case ZZ_REG_ETH_TX:
			if (data->net) {
				auto *net = data->net;
				net->tx_status = 1;
				if (value >= 14 && value <= zz9000_net::RxQueue::max_frame &&
				    net->backend) {
					net->tx_length = value;
					ethernet_trigger(net->backend, net->backend_data);
					net->tx_status = net->tx_length == 0 ? 0 : 2;
					net->tx_length = 0;
				}
			}
			break;
		case ZZ_REG_ETH_RX:
			if (data->net)
				data->net->rx.accept(value);
			break;
		case ZZ_REG_ETH_MAC_HI:
		case ZZ_REG_ETH_MAC_MID:
		case ZZ_REG_ETH_MAC_LO:
			if (data->net) {
				const int index = (offset - ZZ_REG_ETH_MAC_HI) / 2 * 2;
				data->net->mac[index] = static_cast<uae_u8>(value >> 8);
				data->net->mac[index + 1] = static_cast<uae_u8>(value);
				if (offset == ZZ_REG_ETH_MAC_LO && data->net->backend) {
					zz_net_close(data->net);
					zz_net_open(data, currprefs.zz9000net_name);
				}
			}
			break;
		case 0x1000:
			data->palette_data = (data->palette_data & 0xffff) |
				(static_cast<uae_u32>(value) << 16);
			break;
		case 0x1002:
			data->palette_data = (data->palette_data & 0xffff0000) | value;
			break;
		case 0x1004:
			zz_palette_write_z2(data, value);
			break;
		case 0x1006:
			data->capture_mode = value != 0;
			gfxboard_set(data->monitor_id, !data->capture_mode);
			break;
		case ZZ_REG_SPLIT_POS:
			data->split_offset = data->source_offset;
			data->split_position = static_cast<int16_t>(value);
			data->modified = true;
			break;
		case ZZ_REG_AUDIO_SWAB:
			data->audio_collision = zz_ax_collision(data, value);
			// Firmware converts the newly submitted guest period at the kick,
			// not when the formatter later reads it. A silent/stale slot must
			// never be byte-swapped a second time on the next ring revolution.
#if defined(AHI) && !defined(LIBRETRO)
			if (!(value & 0x8000)) {
				const uae_u32 slot_offset = ((value & 0x7fffu) * 256u) % ZZ_AX_RING_BYTES;
				if (slot_offset + zz_ax_frames(data) * 4 <= ZZ_AX_RING_BYTES)
					zz_ax_swap16(data->memory + data->audio_tx_ring_off + slot_offset,
						zz_ax_frames(data) * 4);
			}
#endif
			if (data->audio_log_kicks == 0)
				write_log(_T("ZZ9000AX: first SWAB kick 0x%04x\n"), value);
			else if (data->audio_log_kicks % 250 == 0)
				write_log(_T("ZZ9000AX: SWAB kick 0x%04x (tx_seq=%u)\n"),
					value, data->audio_tx_seq);
			data->audio_log_kicks++;
			break;
		case ZZ_REG_AUDIO_SCALE:
			data->audio_scale = value;
#if defined(AHI) && !defined(LIBRETRO)
			if (data->audio_play || data->audio_record)
				zz_ax_audio_start(data);
#endif
			break;
		case ZZ_REG_AUDIO_CONFIG:
			data->audio_play = (value & 1) != 0;
			data->audio_record = (value & 2) != 0;
			if (!(data->audio_log_flags & (1u << 1))) {
				data->audio_log_flags |= 1u << 1;
				write_log(_T("ZZ9000AX: intreq mask 0x%04x (play=%d rec=%d)\n"),
					value, data->audio_play ? 1 : 0, data->audio_record ? 1 : 0);
			}
#if defined(AHI) && !defined(LIBRETRO)
			if (data->audio_play || data->audio_record) {
				zz_ax_audio_start(data);
			} else {
				// Stop: clear a still-pending card latch (a period may
				// have gone pending without an 0x04 acknowledgement).
				zz_ax_clear_interrupt(data);
				zz_ax_audio_stop(data);
			}
#endif
			break;
		case ZZ_REG_CONFIG:
			// Bit 3 selects ack mode. Bits 4 and 5 independently
			// acknowledge Ethernet and audio on the shared IRQ line.
			if (value & 8) {
				if ((value & 16) && data->net)
					data->net->irq_pending = false;
				if (value & 32) {
					if (data->audio_intreq_pending &&
					    !(data->audio_log_flags & (1u << 3))) {
						data->audio_log_flags |= 1u << 3;
						write_log(_T("ZZ9000AX: interrupt acked via 0x04 write 0x%04x\n"), value);
					}
					zz_ax_clear_interrupt(data);
				}
			} else if (!(value & 128) && data->net) {
				data->net->irq_enabled = (value & 1) != 0;
				if (!data->net->irq_enabled)
					data->net->irq_pending = false;
			}
			break;
		default:
			break;
	}
}

static uae_u16 zz_read_register(zz9000_state *data, uae_u32 offset)
{
	switch (offset) {
		case ZZ_REG_FW_VERSION:
			return 0x0208;
		case ZZ_REG_FW_CAPABILITIES:
			/* No 2.8-only capability bits: custom P96 modelines, Z2
			 * aperture negotiation and the VCAP centered-1080p /
			 * live-calibration stack are not emulated, so a 2.8
			 * card driver stays on the legacy paths this
			 * emulation backs. */
			return 0;
		case ZZ_REG_ETH_TX:
			return data->net ? data->net->tx_status : 1;
		case ZZ_REG_ETH_MAC_HI:
		case ZZ_REG_ETH_MAC_MID:
		case ZZ_REG_ETH_MAC_LO:
			if (data->net) {
				const int index = (offset - ZZ_REG_ETH_MAC_HI) / 2 * 2;
				return (data->net->mac[index] << 8) | data->net->mac[index + 1];
			}
			return 0;
		case ZZ_REG_ETH_RX_STATUS:
			return data->net ? static_cast<uae_u16>(
				(data->net->rx.ready() >= zz9000_net::RxQueue::capacity ? 0x8000 : 0) |
				std::min<size_t>(data->net->rx.ready(), 255)) : 0;
		case ZZ_REG_ETH_RX_STATS:
			return data->net ? static_cast<uae_u16>(
				std::min<uae_u32>(data->net->rx.dropped(), 255) << 8) : 0;
		case ZZ_REG_VBLANK:
			data->vblank_read = !data->vblank_read;
			return data->vblank_read ? 1 : 0;
		case ZZ_REG_CONFIG_KEY:
			if ((data->config_key == 5 && data->int2) ||
			    data->config_key == 9 || data->config_key == 10 || data->config_key == 11)
				return 1;
			return 0;
		case ZZ_REG_CONFIG_PRESENT:
			return (data->config_key == 5 && data->int2) ||
				data->config_key == 9 || data->config_key == 10 ||
				data->config_key == 11 ? 1 : 0;
		case ZZ_REG_AUDIO_CONFIG:
			// Libretro's SDL audio stream stub does not deliver AX samples
			// to its frontend; do not advertise a silent card there.
#if defined(AHI) && !defined(LIBRETRO)
			// bit 0 = AX present, bit 1 = TX status capable
			if (!(data->audio_log_flags & 1u)) {
				data->audio_log_flags |= 1u;
				write_log(_T("ZZ9000AX: probed (0xF4 read)\n"));
			}
			return 0x3;
#else
			return 0;
#endif
		case ZZ_REG_AUDIO_RX_STATUS:
			return zz_ax_status(data->audio_rx_write_period, data->audio_rx_seq);
		case ZZ_REG_AUDIO_TX_STATUS:
			return zz_ax_status((data->audio_tx_read_period + ZZ_AX_PERIODS - 1) % ZZ_AX_PERIODS,
				data->audio_tx_seq);
		case ZZ_REG_AUDIO_SWAB:
			return data->audio_collision ? 1 : 0;
		case ZZ_REG_CONFIG:
			// Ethernet uses bit 0; AX audio uses bit 1. RTG does not
			// use this interrupt register.
			data->audio_cfg_reads++;
			if (data->audio_cfg_reads % 500 == 1)
				write_log(_T("ZZ9000: 0x04 read=%u (n=%u)\n"),
					(data->audio_intreq_pending ? 2 : 0) |
					(data->net && data->net->irq_pending ? 1 : 0),
					data->audio_cfg_reads);
			return (data->audio_intreq_pending ? 2 : 0) |
				(data->net && data->net->irq_pending ? 1 : 0);
		default:
			if (offset / 2 < sizeof data->registers / sizeof data->registers[0])
				return data->registers[offset / 2];
			return 0;
	}
}

static uae_u32 REGPARAM2 zz9000_wget(uaecptr addr)
{
	zz9000_state *data = zz_find_board(addr);
	if (!data)
		return 0;
	const uae_u32 offset = addr - data->configured;
	if (data->net && offset >= ZZ_NET_RX_WINDOW && offset + 1 < 0x6000)
		return (data->net->rx.read(offset - ZZ_NET_RX_WINDOW) << 8) |
			data->net->rx.read(offset + 1 - ZZ_NET_RX_WINDOW);
	if (data->net && offset >= ZZ_NET_TX_WINDOW && offset + 1 < ZZ_NET_TX_WINDOW + ZZ_NET_WINDOW_BYTES)
		return (data->net->tx[offset - ZZ_NET_TX_WINDOW] << 8) |
			data->net->tx[offset + 1 - ZZ_NET_TX_WINDOW];
	if (offset >= ZZ9000_MEMORY_BASE) {
		if (offset + 1 >= data->card_size)
			return 0;
		return (data->memory[offset] << 8) | data->memory[offset + 1];
	}
	return zz_read_register(data, offset & ~1U);
}

static uae_u32 REGPARAM2 zz9000_bget(uaecptr addr)
{
	zz9000_state *data = zz_find_board(addr);
	if (!data)
		return 0;
	const uae_u32 offset = addr - data->configured;
	if (data->net && offset >= ZZ_NET_RX_WINDOW && offset < 0x6000)
		return data->net->rx.read(offset - ZZ_NET_RX_WINDOW);
	if (data->net && offset >= ZZ_NET_TX_WINDOW && offset < ZZ_NET_TX_WINDOW + ZZ_NET_WINDOW_BYTES)
		return data->net->tx[offset - ZZ_NET_TX_WINDOW];
	if (offset >= ZZ9000_MEMORY_BASE)
		return offset < data->card_size ? data->memory[offset] : 0;
	const uae_u16 value = zz_read_register(data, offset & ~1U);
	return (offset & 1) ? value & 0xff : value >> 8;
}

static uae_u32 REGPARAM2 zz9000_lget(uaecptr addr)
{
	return (zz9000_wget(addr) << 16) | zz9000_wget(addr + 2);
}

static void REGPARAM2 zz9000_wput(uaecptr addr, uae_u32 value)
{
	zz9000_state *data = zz_find_board(addr);
	if (!data)
		return;
	const uae_u32 offset = addr - data->configured;
	if (offset >= ZZ_NET_RX_WINDOW && offset < 0x6000)
		return;
	if (data->net && offset >= ZZ_NET_TX_WINDOW && offset + 1 < ZZ_NET_TX_WINDOW + ZZ_NET_WINDOW_BYTES) {
		data->net->tx[offset - ZZ_NET_TX_WINDOW] = static_cast<uae_u8>(value >> 8);
		data->net->tx[offset + 1 - ZZ_NET_TX_WINDOW] = static_cast<uae_u8>(value);
		return;
	}
	if (offset >= ZZ9000_MEMORY_BASE) {
		if (offset + 1 >= data->card_size)
			return;
		data->memory[offset] = static_cast<uae_u8>(value >> 8);
		data->memory[offset + 1] = static_cast<uae_u8>(value);
		data->modified = true;
		return;
	}
	zz_write_register(data, offset & ~1U, static_cast<uae_u16>(value));
}

static void REGPARAM2 zz9000_bput(uaecptr addr, uae_u32 value)
{
	zz9000_state *data = zz_find_board(addr);
	if (!data)
		return;
	const uae_u32 offset = addr - data->configured;
	if (offset >= ZZ_NET_RX_WINDOW && offset < 0x6000)
		return;
	if (data->net && offset >= ZZ_NET_TX_WINDOW && offset < ZZ_NET_TX_WINDOW + ZZ_NET_WINDOW_BYTES) {
		data->net->tx[offset - ZZ_NET_TX_WINDOW] = static_cast<uae_u8>(value);
		return;
	}
	if (offset >= ZZ9000_MEMORY_BASE) {
		if (offset >= data->card_size)
			return;
		data->memory[offset] = static_cast<uae_u8>(value);
		data->modified = true;
		return;
	}
	const uae_u32 word_offset = offset & ~1U;
	uae_u16 word = zz_read_register(data, word_offset);
	if (offset & 1)
		word = static_cast<uae_u16>((word & 0xff00) | (value & 0xff));
	else
		word = static_cast<uae_u16>((word & 0x00ff) | ((value & 0xff) << 8));
	zz_write_register(data, word_offset, word);
}

static void REGPARAM2 zz9000_lput(uaecptr addr, uae_u32 value)
{
	zz9000_wput(addr, value >> 16);
	zz9000_wput(addr + 2, value & 0xffff);
}

static void zz_composite_overlay_row(zz9000_state *data, uae_u8 *row, int screen_y)
{
	auto &overlay = data->overlay;
	if (!overlay.configured || !overlay.active || !data->overlay_feature ||
		overlay.mode_stale || data->split_position != 0 ||
		data->scale_mode != 0 || data->color_mode == ZZ_COLOR_CLUT ||
		overlay.destination_width <= 0 || overlay.destination_height <= 0)
		return;
	if (overlay.mode_width != data->width || overlay.mode_height != data->height ||
		overlay.mode_color != data->color_mode || overlay.mode_scale != data->scale_mode ||
		overlay.mode_pitch != data->pan_width * zz_bytes_per_pixel(data->color_mode)) {
		overlay.mode_stale = true;
		return;
	}
	if (screen_y < std::max(0, overlay.destination_y) ||
		screen_y >= std::min(data->height,
			overlay.destination_y + overlay.destination_height))
		return;
	const int bpp = zz_bytes_per_pixel(data->color_mode);
	const uae_u32 source_step_y = (static_cast<uae_u32>(overlay.source_height) << 8) /
		overlay.destination_height;
	uae_u32 source_y = (static_cast<uae_u32>(screen_y - overlay.destination_y) *
		source_step_y) >> 8;
	if (source_y >= overlay.source_height)
		source_y = overlay.source_height - 1;
	const uae_u32 source_step_x = (static_cast<uae_u32>(overlay.source_width) << 8) /
		overlay.destination_width;
	for (int screen_x = std::max(0, overlay.destination_x);
		screen_x < std::min(data->width, overlay.destination_x + overlay.destination_width);
		++screen_x) {
		uae_u8 *destination = row + screen_x * bpp;
		if (overlay.key_enabled) {
			const uae_u32 current = bpp == 4 ? zz_read_le32(destination) :
				destination[0] | (destination[1] << 8);
			if ((bpp == 4 ? current & 0x00ffffff : current & 0xffff) !=
				(bpp == 4 ? overlay.color_key & 0x00ffffff : overlay.color_key & 0xffff))
				continue;
		}
		uae_u32 source_x = (static_cast<uae_u32>(screen_x - overlay.destination_x) *
			source_step_x) >> 8;
		if (source_x >= overlay.source_width)
			source_x = overlay.source_width - 1;
		const uae_u8 *macropixel = zz_memory_ptr(data, overlay.source_offset +
			source_y * overlay.source_pitch + (source_x / 2) * 4, 4);
		if (!macropixel)
			continue;
		int red, green, blue;
		zz_yuv_to_rgb(macropixel, overlay.variant, (source_x & 1) != 0,
			&red, &green, &blue);
		zz_write_native_pixel(destination, data->color_mode, red, green, blue);
	}
}

static uae_u8 zz_nearest_palette_color(zz9000_state *data, int red, int green, int blue)
{
	const auto &palette = picasso96_state[data->monitor_id].CLUT;
	int best = 0;
	int best_distance = 0x7fffffff;
	for (int i = 0; i < 256; ++i) {
		const int dr = red - palette[i].Red;
		const int dg = green - palette[i].Green;
		const int db = blue - palette[i].Blue;
		const int distance = dr * dr + dg * dg + db * db;
		if (distance < best_distance) {
			best = i;
			best_distance = distance;
		}
	}
	return static_cast<uae_u8>(best);
}

static void zz_composite_sprite_row(zz9000_state *data, uae_u8 *row, int screen_y)
{
	if (!data->sprite_visible || !data->sprite_width || !data->sprite_height)
		return;
	const int sprite_x = data->sprite_x - data->sprite_x_offset + 1;
	const int sprite_y = data->sprite_y - data->sprite_y_offset + 1;
	const int source_y = screen_y - sprite_y;
	if (source_y < 0 || source_y >= data->sprite_height)
		return;
	const int bpp = zz_bytes_per_pixel(data->color_mode);
	for (int source_x = 0; source_x < data->sprite_width; ++source_x) {
		const int screen_x = sprite_x + source_x;
		if (screen_x < 0 || screen_x >= data->width)
			continue;
		const int pen = data->sprite_pixels[source_y * 32 + source_x];
		if (!pen)
			continue;
		const uae_u32 color = data->sprite_colors[pen & 3];
		const int red = (color >> 16) & 0xff;
		const int green = (color >> 8) & 0xff;
		const int blue = color & 0xff;
		uae_u8 *destination = row + screen_x * bpp;
		if (data->color_mode == ZZ_COLOR_CLUT)
			destination[0] = zz_nearest_palette_color(data, red, green, blue);
		else
			zz_write_native_pixel(destination, data->color_mode, red, green, blue);
	}
}

static bool zz9000_vsync(void *userdata, gfxboard_mode *mode)
{
	auto *data = static_cast<zz9000_state *>(userdata);
	if (!data->visible || data->capture_mode || !data->width || !data->height)
		return false;
	const int bpp = zz_bytes_per_pixel(data->color_mode);
	const RGBFTYPE rgb_format = zz_rgb_format(data->color_mode);
	if (!bpp || rgb_format == RGBFB_NONE)
		return false;

	mode->width = data->width;
	mode->height = data->height;
	mode->mode = rgb_format;
	mode->hlinedbl = 1;
	mode->vlinedbl = 1;
#ifdef AMIBERRY
	const bool host_only_cursor = picasso_uses_host_cursor(data->monitor_id);
	if (host_only_cursor && data->sprite_visible) {
		picasso_update_external_host_cursor(data->monitor_id, data->sprite_pixels, 32,
			data->sprite_width, data->sprite_height, data->sprite_colors,
			amiberry_cursor_hotspot_from_zz9000_offset(
				data->sprite_x_offset, data->sprite_width),
			amiberry_cursor_hotspot_from_zz9000_offset(
				data->sprite_y_offset, data->sprite_height));
	} else {
		picasso_clear_external_host_cursor(data->monitor_id);
	}
#endif
	const auto &state = picasso96_state[data->monitor_id];
	if (data->mode_changed && (state.Width != data->width ||
		state.Height != data->height || state.RGBFormat != rgb_format))
		return true;

	uae_u8 *surface = gfx_lock_picasso(data->monitor_id, false);
	if (!surface)
		return false;
	if (data->modified || data->mode_changed || mode->redraw_required) {
		const size_t line_size = static_cast<size_t>(data->width) * bpp;
		uae_u8 *line = xmalloc(uae_u8, line_size);
		if (line) {
			const uae_u32 source_pitch = data->pan_width * bpp;
			bool secondary_palette_active = false;
			for (int y = 0; y < data->height; ++y) {
				const bool split = data->split_position > 0 && y >= data->split_position;
				const uae_u32 source_base = split ? data->split_offset : data->pan_offset;
				const int source_y = split ? y - data->split_position : y;
				if (split && data->secondary_palette_feature && !secondary_palette_active &&
					data->color_mode == ZZ_COLOR_CLUT) {
					zz_activate_palette(data, data->secondary_palette);
					secondary_palette_active = true;
				}
				const uae_u8 *source = zz_memory_ptr(data, source_base + source_y * source_pitch,
					static_cast<uae_u32>(line_size));
				if (data->display_blank || !source)
					memset(line, 0, line_size);
				else
					memcpy(line, source, line_size);
				if (!data->display_blank) {
					zz_composite_overlay_row(data, line, y);
#ifdef AMIBERRY
					if (!host_only_cursor) {
						zz_composite_sprite_row(data, line, y);
					}
#else
					zz_composite_sprite_row(data, line, y);
#endif
				}
				fb_copyrow(data->monitor_id, line, surface, 0, 0, data->width, bpp, y);
			}
			if (secondary_palette_active)
				zz_activate_palette(data, data->primary_palette);
			xfree(line);
			data->modified = false;
			data->mode_changed = false;
		}
	}
	gfx_unlock_picasso(data->monitor_id, true);
	return true;
}

static bool zz9000_toggle(void *userdata, int mode)
{
	auto *data = static_cast<zz9000_state *>(userdata);
	if (!data->configured)
		return false;
	if (!mode) {
		if (!data->enabled)
			return false;
		data->enabled = false;
		data->visible = false;
#ifdef AMIBERRY
		picasso_clear_external_host_cursor(data->monitor_id);
#endif
		return true;
	}
	if (data->enabled)
		return false;
	data->enabled = true;
	data->visible = true;
	data->mode_changed = true;
	return true;
}

static void zz9000_configured(void *userdata, uae_u32 address)
{
	auto *data = static_cast<zz9000_state *>(userdata);
	data->configured = address;
	write_log(_T("ZZ9000 %s configured at %08x (%u MB)\n"),
		data->z3 ? _T("Z3") : _T("Z2"), address, data->card_size >> 20);
}

static void zz9000_refresh(void *userdata)
{
	auto *data = static_cast<zz9000_state *>(userdata);
	data->modified = true;
}

static void zz9000_hsync(void *userdata)
{
	zz_net_tick(static_cast<zz9000_state *>(userdata));
#if defined(AHI) && !defined(LIBRETRO)
	zz_ax_audio_tick(static_cast<zz9000_state *>(userdata));
#endif
}

static void zz9000_reset(void *userdata)
{
	auto *data = static_cast<zz9000_state *>(userdata);
#if defined(AHI) && !defined(LIBRETRO)
	zz_ax_audio_stop(data);
#endif
#ifdef AMIBERRY
	picasso_clear_external_host_cursor(data->monitor_id);
#endif
	data->surface_count = 0;
	memset(&data->overlay, 0, sizeof data->overlay);
	data->overlay_feature = false;
	data->secondary_palette_feature = false;
	data->display_blank = false;
	data->sprite_visible = false;
	data->split_position = 0;
	data->audio_play = false;
	data->audio_record = false;
	data->audio_scale = 0;
	data->audio_collision = false;
	data->audio_tx_seq = 0;
	data->audio_tx_read_period = 0;
	data->audio_rx_seq = 0;
	data->audio_rx_write_period = 0;
	zz_ax_clear_interrupt(data);
	data->audio_log_kicks = 0;
	data->audio_log_periods = 0;
	data->audio_log_flags = 0;
	data->audio_cfg_reads = 0;
	zz_net_reset(data);
	data->modified = true;
}

static void zz9000_free(void *userdata)
{
	auto *data = static_cast<zz9000_state *>(userdata);
	if (!data)
		return;
#ifdef AMIBERRY
	picasso_clear_external_host_cursor(data->monitor_id);
#endif
	if (data->devnum >= 0 && data->devnum < MAX_RTG_BOARDS)
		zz9000_boards[data->devnum] = nullptr;
#if defined(AHI) && !defined(LIBRETRO)
	zz_ax_audio_stop(data);
	delete data->ax_audio;
	data->ax_audio = nullptr;
#endif
	zz_ax_clear_interrupt(data);
	zz_net_close(data->net);
	delete data->net;
	data->net = nullptr;
	xfree(data->memory);
	data->memory = nullptr;
	xfree(data);
}

static bool zz9000_init(autoconfig_info *aci)
{
	const bool z3 = aci->prefs->rtgboards[aci->devnum].rtgmem_type == GFXBOARD_ID_ZZ9000_Z3;
	aci->label = z3 ? _T("ZZ9000 Zorro III") : _T("ZZ9000 Zorro II");
	if (z3)
		aci->autoconfig_bytes[2] = 0x30;
	if (!aci->doinit)
		return true;

	auto *data = xcalloc(zz9000_state, 1);
	if (!data)
		return false;
	data->devnum = aci->devnum;
	data->monitor_id = aci->prefs->rtgboards[aci->devnum].monitor_id;
	data->z3 = z3;
	data->int2 = aci->prefs->zz9000_int2;
	data->card_size = z3 ? ZZ9000_Z3_SIZE : ZZ9000_Z2_SIZE;
	// AX audio rings live in the top 64 KB of card memory (index into
	// data->memory): TX base = size - 0x10000, RX base = size - 0x8000.
	// The 8-slot ring is 8 x 3840 B; the legacy aperture (FW_CAPS = 0)
	// places them here for both Z2 and Z3.
	data->audio_tx_ring_off = data->card_size - 0x10000;
	data->audio_rx_ring_off = data->card_size - 0x8000;
	data->memory = xcalloc(uae_u8, data->card_size);
	if (!data->memory) {
		xfree(data);
		return false;
	}
	data->net = new (std::nothrow) zz9000_net_engine;
	if (!data->net) {
		xfree(data->memory);
		xfree(data);
		return false;
	}
	zz_net_reset(data);
	data->sprite_colors[0] = 0x00ff00ff;
	data->sprite_visible = false;
	data->modified = true;
	zz9000_boards[data->devnum] = data;
	aci->addrbank = &zz9000_bank;
	aci->userdata = data;
	return true;
}

gfxboard_func zz9000_func = {
	zz9000_init,
	zz9000_free,
	zz9000_reset,
	zz9000_hsync,
	zz9000_vsync,
	zz9000_refresh,
	zz9000_toggle,
	zz9000_configured
};
