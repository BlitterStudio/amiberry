/* SPDX-FileCopyrightText: 2026 Dimitris Panokostas
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
// Host side of the MiniGL zero-copy handoff. The sink is called on the
// emulation thread inside the plugin's serialized request path; renderers
// poll the snapshot from their own thread under the mutex below. The fd
// stays owned by the plugin (valid until the next handover), so it is never
// closed here; importers dup it internally.
#include "minigl_display.h"
#include <mutex>

namespace {
std::mutex g_mutex;
MiniglDisplayImage g_image;
bool g_installed = false;

void sink_callback(void* /*user*/, int fd, uint32_t width, uint32_t height,
	uint32_t stride, uint32_t fourcc, uint64_t modifier)
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	g_image.fd = fd;
	g_image.width = width;
	g_image.height = height;
	g_image.stride = stride;
	g_image.fourcc = fourcc;
	g_image.modifier = modifier;
	++g_image.seq;  // 0 reserved for "no image yet"; overflow is not a concern
}
}

bool minigl_display_active()
{
	return g_installed;
}

MiniglDisplayImage minigl_display_current()
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	return g_image;
}

void minigl_display_install(void* set_image_sink_sym, void* /*plugin_lib*/)
{
	if (!set_image_sink_sym || g_installed)
		return;
	using SetImageSink = void (*)(void (*)(void*, int, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t), void*);
	reinterpret_cast<SetImageSink>(set_image_sink_sym)(&sink_callback, nullptr);
	g_installed = true;
}
