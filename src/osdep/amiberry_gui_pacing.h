/*
 * amiberry_gui_pacing.h - Software frame pacing for the GUI loop
 *
 * The GUI redraws itself on every loop iteration and used to rely entirely on
 * presentation to throttle that loop. That assumption does not hold:
 *
 *  - with VSync off (the default) the OpenGL swap interval is 0, so
 *    SDL_GL_SwapWindow() returns immediately;
 *  - the Vulkan swapchain prefers MAILBOX, whose pacing lives in
 *    present_frame() - a function the GUI never calls;
 *  - a shared SDL renderer (KMSDRM, no-WM, Android) keeps whatever VSync
 *    state emulation configured;
 *  - a VRR panel with driver-side sync disabled never blocks either.
 *
 * In those cases the loop spins as fast as the CPU allows. Even when
 * presentation does block, it blocks at the panel's refresh rate, which means
 * 240 redraws a second of a static UI on a high-refresh monitor.
 *
 * So pace the loop in software, independently of the host's presentation
 * policy. The delay computation is kept here, free of SDL, so it can be tested
 * on its own (tests/gui_pacing_test.cpp).
 *
 * Copyright 2026 Dimitris Panokostas
 */

#ifndef AMIBERRY_GUI_PACING_H
#define AMIBERRY_GUI_PACING_H

#include <algorithm>
#include <cstdint>

// Slowest and fastest rates the GUI is paced at, whatever the host reports.
// The lower bound keeps the UI responsive on 30 Hz-ish panels; the upper bound
// is what stops a 144/240 Hz or VRR display from burning a core on a UI that
// only changes when the user touches it.
constexpr float AMIBERRY_GUI_MIN_FPS = 50.0f;
constexpr float AMIBERRY_GUI_MAX_FPS = 60.0f;

// Milliseconds the GUI loop should sleep after a frame that took elapsed_ms,
// on a display reporting refresh_hz (<= 0 when the host does not know).
inline uint32_t amiberry_gui_frame_delay_ms(const float elapsed_ms, const float refresh_hz)
{
	const float target_fps = refresh_hz > 0.0f
		? std::clamp(refresh_hz, AMIBERRY_GUI_MIN_FPS, AMIBERRY_GUI_MAX_FPS)
		: AMIBERRY_GUI_MAX_FPS;

	const float frame_time_ms = 1000.0f / target_fps;
	const float delay_ms = frame_time_ms - elapsed_ms;

	// A frame that overran its budget - or a bogus elapsed time - must never
	// turn into a delay, and never into a wrapped-around unsigned one.
	if (delay_ms <= 0.0f)
		return 0;

	return static_cast<uint32_t>(delay_ms);
}

#endif // AMIBERRY_GUI_PACING_H
