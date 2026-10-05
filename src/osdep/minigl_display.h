/* SPDX-FileCopyrightText: 2026 Dimitris Panokostas
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef AMIBERRY_MINIGL_DISPLAY_H
#define AMIBERRY_MINIGL_DISPLAY_H
#include <cstdint>

// Zero-copy handoff from the MiniGL plugin: when enabled, the plugin skips
// encoding GLQuake-style frames into the Picasso96 span and instead exports
// the presented attachment as a dma-buf. The renderer imports it on its own
// EGLDisplay (EGLImages cannot cross displays; the fd is the token) and
// composites it directly. A fd of -1 means the span is authoritative again.

struct MiniglDisplayImage {
	int fd = -1;
	uint32_t offset = 0;
	uint32_t width = 0, height = 0, stride = 0, fourcc = 0;
	uint64_t modifier = 0;
	uint64_t seq = 0;  // 0 = no image; bumped on every change
};

// True when the sink is registered and at least one image or withdrawal
// arrived: the renderer can then consult minigl_display_current().
bool minigl_display_active();
MiniglDisplayImage minigl_display_current();

// Registers the sink through the plugin symbol (may be null on old plugin
// builds; the call is then a no-op). Called once after plugin load.
void minigl_display_install(void* set_image_sink_sym, void* plugin_lib);

#endif
