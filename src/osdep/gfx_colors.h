#pragma once

/*
 * gfx_colors.h - Pixel format, color tables, and palette management
 *
 * Extracted from amiberry_gfx.cpp for single-responsibility decomposition.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#include "uae/types.h"

struct MyCLUTEntry;

// Update pixel format based on current RTG and board settings
void update_pixel_format();

// Initialize color tables for the given monitor
void init_colors(int monid);

// Update RTG palette (returns 1 if any entry changed)
int picasso_palette(struct MyCLUTEntry* CLUT, uae_u32* clut);

// Set Picasso96 color format
void gfx_set_picasso_colors(int monid, int rgbfmt);
