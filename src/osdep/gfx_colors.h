#pragma once

/*
 * gfx_colors.h - Pixel format, color tables, and palette management
 *
 * Extracted from amiberry_gfx.cpp for single-responsibility decomposition.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#include <SDL3/SDL.h>

#include "uae/types.h"
#include "rtgmodes.h"

struct MyCLUTEntry;

// Resolve the host SDL surface format used to present an RTG RGBFormat.
SDL_PixelFormat p96_get_host_pixel_format(RGBFTYPE rgbfmt, bool hardware_rtg);

// Resolve the host SDL surface format for the current RTG monitor.
SDL_PixelFormat p96_get_host_pixel_format_for_monitor(int monid);

// Return true when the guest RTG pixels can be presented directly with this SDL format.
bool p96_rgbformat_matches_host_pixel_format(RGBFTYPE rgbfmt, SDL_PixelFormat format);

// Resolve the host RGBFormat metadata matching an SDL surface format.
RGBFTYPE p96_get_host_rgb_format(SDL_PixelFormat format, int pixbytes);

// Resolve the host RGBFormat metadata for the current RTG monitor.
RGBFTYPE p96_get_host_rgb_format_for_monitor(int monid, int pixbytes);

// Update pixel format based on current RTG and board settings
void update_pixel_format(int monid = 0);

// Initialize color tables for the given monitor
void init_colors(int monid);

// Update RTG palette (returns 1 if any entry changed)
int picasso_palette(struct MyCLUTEntry* CLUT, uae_u32* clut);

// Set Picasso96 color format
void gfx_set_picasso_colors(int monid, int rgbfmt);
