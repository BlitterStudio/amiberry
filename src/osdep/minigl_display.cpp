/* SPDX-FileCopyrightText: 2026 Dimitris Panokostas
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
// Host side of the MiniGL zero-copy handoff. The sink is called on the
// emulation thread inside the plugin's serialized request path; renderers
// poll the snapshot from their own thread under the mutex below. The fd
// stays owned by the plugin (valid until the next handover), so it is never
// closed here; importers dup it internally.
#include "minigl_display.h"
#include <mutex>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace {
std::mutex g_mutex;
MiniglDisplayImage g_image;
bool g_installed = false;
int g_importer_refs = 0;  // live renderer instances that verified a capable context
void (*g_set_image_sink)(void (*)(void*, int, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t), void*) = nullptr;

void sink_callback(void* /*user*/, int fd, uint32_t offset, uint32_t width, uint32_t height,
	uint32_t stride, uint32_t fourcc, uint64_t modifier)
{
	// The plugin keeps its fd valid only until the next handover. It cannot
	// issue one while this callback runs (both sit inside the plugin's
	// serialized request path), so duplicating under the registry lock gives
	// the renderer an owned descriptor that outlives the plugin's.
	const std::lock_guard<std::mutex> lock(g_mutex);
#if defined(__linux__)
	if (g_image.fd >= 0) {
		close(g_image.fd);
		g_image.fd = -1;
	}
#endif
	if (fd >= 0) {
#if defined(__linux__)
		g_image.fd = dup(fd);
		if (g_image.fd < 0)
			fd = -1;  // import cannot work; treat as withdrawn
#else
		g_image.fd = fd;
#endif
	}
	g_image.offset = offset;
	g_image.width = width;
	g_image.height = height;
	g_image.stride = stride;
	g_image.fourcc = fourcc;
	g_image.modifier = modifier;
	++g_image.seq;  // 0 reserved for "no image yet"; overflow is not a concern
}
}

void minigl_display_note_importer(bool capable)
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	if (capable)
		++g_importer_refs;  // one reference per renderer instance, once
}

bool minigl_display_importer_ready()
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	return g_importer_refs > 0;
}

void minigl_display_release_importer()
{
	bool last = false;
	{
		const std::lock_guard<std::mutex> lock(g_mutex);
		if (g_importer_refs > 0)
			--g_importer_refs;
		last = g_importer_refs == 0;
		if (last)
			g_installed = false;  // a later importer may install the sink again
	}
	if (!last)
		return;  // another live context still imports
	{
		const std::lock_guard<std::mutex> lock(g_mutex);
#if defined(__linux__)
		if (g_image.fd >= 0) {
			close(g_image.fd);
			g_image.fd = -1;
		}
#endif
	}
	// No importer remains: removing the sink makes the plugin resume span
	// updates instead of exporting frames nobody displays.
	if (g_set_image_sink)
		g_set_image_sink(nullptr, nullptr);
	g_set_image_sink = nullptr;
}

bool minigl_display_active()
{
	return g_installed;
}

// The returned fd is a fresh duplicate owned by the caller: the next sink
// callback may replace and close the registry's descriptor at any moment,
// so the snapshot must stay alive through the caller's EGL import.
MiniglDisplayImage minigl_display_current()
{
#if defined(__linux__)
	const std::lock_guard<std::mutex> lock(g_mutex);
	MiniglDisplayImage snapshot = g_image;
	if (snapshot.fd >= 0) {
		snapshot.fd = dup(g_image.fd);
		if (snapshot.fd < 0)
			snapshot.seq = 0;  // treat as "nothing to show"
	}
	return snapshot;
#else
	const std::lock_guard<std::mutex> lock(g_mutex);
	return g_image;
#endif
}

void minigl_display_install(void* set_image_sink_sym, void* /*plugin_lib*/)
{
	if (!set_image_sink_sym || g_installed)
		return;
	using SetImageSink = void (*)(void (*)(void*, int, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t), void*);
	reinterpret_cast<SetImageSink>(set_image_sink_sym)(&sink_callback, nullptr);
	g_set_image_sink = reinterpret_cast<SetImageSink>(set_image_sink_sym);
	g_installed = true;
}
