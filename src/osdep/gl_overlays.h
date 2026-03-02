#pragma once

/*
 * gl_overlays.h - OpenGL overlay rendering: OSD, bezel, software cursor
 *
 * Extracted from amiberry_gfx.cpp.
 *
 * Copyright 2025 Dimitris Panokostas
 */

#include "gfx_state.h"

#ifdef USE_OPENGL
extern GLOverlayState g_overlay;

extern bool init_osd_shader();
extern void render_osd(int monid, int x, int y, int w, int h);
extern void render_bezel_overlay(int drawableWidth, int drawableHeight);
extern void render_software_cursor_gl(int monid, int x, int y, int w, int h);
extern void destroy_bezel_overlay();
#endif

extern void update_leds(int monid);
extern void update_custom_bezel();
