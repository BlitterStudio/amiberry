#pragma once

#define GFX_WINDOW 0
#define GFX_FULLSCREEN 1 // Legacy exclusive-fullscreen config/API value
#define GFX_FULLWINDOW 2

static inline int amiberry_normalize_gfx_fullscreen_mode(const int mode)
{
	return mode == GFX_FULLSCREEN ? GFX_FULLWINDOW : mode;
}
