#pragma once

/*
 * gfx_window.h - Window/context management: creation, destruction, and lifecycle
 *
 * Extracted from amiberry_gfx.cpp for single-responsibility decomposition.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#include <SDL.h>

struct AmigaMonitor;

// Window lifecycle
void close_hwnds(struct AmigaMonitor* mon);
void close_windows(struct AmigaMonitor* mon);
int open_windows(AmigaMonitor* mon, bool mousecapture, bool started);
void open_screen(struct AmigaMonitor* mon);
void reopen_gfx(struct AmigaMonitor* mon);
int reopen(struct AmigaMonitor* mon, int full, bool unacquire);

// Display mode and parameter updates
void updatemodes(struct AmigaMonitor* mon);
void update_gfxparams(struct AmigaMonitor* mon);

// Initialization
bool doInit(AmigaMonitor* mon);

// Texture tracking arrays (shared with target_graphics_buffer_update)
extern int oldtex_w[], oldtex_h[], oldtex_rtg[];

#ifdef USE_OPENGL
bool set_opengl_attributes(int mode);
#endif
