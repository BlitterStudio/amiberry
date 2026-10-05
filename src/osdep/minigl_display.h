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

// The OpenGL renderer calls this once it has verified, under its own
// context, that frames can actually be imported (an EGL display with
// dma-buf image support). Sink registration is gated on it so builds or
// window systems without a working importer never stop the span updates.
void minigl_display_note_importer(bool capable);
bool minigl_display_importer_ready();
MiniglDisplayImage minigl_display_current();

// Registers the sink through the plugin symbol (may be null on old plugin
// builds; the call is then a no-op). The materialize symbol (also optional)
// refreshes the span from the live export for host-side readers.
void minigl_display_install(void* set_image_sink_sym, void* materialize_sym);

// Publishes every live export into its span; call before reading Picasso96
// memory (screenshot paths) so they capture current pixels.
void minigl_display_materialize();

// First capable RTG renderer wins: only that monitor composites the
// plugin's image; others keep their own span content.
bool minigl_display_claim_composite();

// Releases one renderer instance's importer reference. When the last one
// goes, the image is dropped and the sink is unregistered with the plugin
// so span updates resume.
void minigl_display_release_importer();

#endif
