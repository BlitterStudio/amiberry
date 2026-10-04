/*
 * gl_context_ladder.h - pure GL context-ladder step selection
 *
 * Maps a ladder attempt index to the GL API request and attribute policy
 * set_opengl_attributes() applies. Pure function — no SDL calls — so the
 * table is unit-testable without a window system.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#pragma once

enum class GlLadderApi { Core, Legacy, Es3 };

// Which ladder shape to walk:
//   Desktop      - x11/wayland/windows/cocoa: GL first, ES only as a
//                  build-opt-in fallback tier (AMIBERRY_GLES_FALLBACK).
//   GlesPreferred- KMSDRM: GLES 3.0 first (v3d and GLES-only boards), then
//                  the desktop tiers so drivers without ES 3.0 (RPi3 vc4)
//                  keep hardware GL 2.1 instead of demoting to software.
//   GlesOnly     - Android/iOS/USE_GLES3 builds: ES is all that exists;
//                  desktop requests can never succeed, so the ladder stops.
enum class GlLadderProfile { Desktop, GlesPreferred, GlesOnly };

struct GlLadderStep {
	bool valid = false;          // false: mode unsupported, caller stops/demotes
	GlLadderApi api = GlLadderApi::Core;
	bool minimal_attrs = false;  // only DOUBLEBUFFER set (pixel-format-picky drivers)
};

// `es_fallback_tier`: AMIBERRY_GLES_FALLBACK build (Desktop profile only).
// `mode`: ladder attempt index, 0..5.
GlLadderStep select_gl_ladder_step(GlLadderProfile profile, bool es_fallback_tier, int mode);
