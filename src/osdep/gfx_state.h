#pragma once

/*
 * gfx_state.h - Grouped graphics state structs
 *
 * Groups the scattered global/static variables from amiberry_gfx.cpp into named
 * structs to make state ownership visible and enable extraction of functions into
 * separate translation units (each taking a struct reference parameter).
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "uae/types.h"
#include "uae/time.h"

// Forward declarations
struct MultiDisplay;

// VSync and vblank state
struct VSyncState {
	int current_interval = -1;
	float cached_refresh_rate = 0.0f;
	bool gl_initialized = false;
	volatile int waitvblankthread_mode = 0;
	frame_time_t wait_vblank_timestamp = 0;
	MultiDisplay* wait_vblank_display = nullptr;
	volatile bool active = false;
	bool scanlinecalibrating = false;
};

