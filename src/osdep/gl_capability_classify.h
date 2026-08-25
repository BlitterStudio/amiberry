#pragma once

/*
 * gl_capability_classify.h - pure GL capability classification
 *
 * Maps GL_VERSION / GL_EXTENSIONS strings to the feature bits the renderer
 * gates on (border-color wrap, sRGB write toggle, RGBA16F renderability).
 * No GL or SDL dependencies so it compiles standalone for unit tests.
 *
 * Copyright 2026 Dimitris Panokostas
 */

struct GlCapabilities {
	bool is_gles = false;
	int major = 0;
	int minor = 0;
	// GL_CLAMP_TO_BORDER is usable (desktop: core since GL 1.3; ES >= 3.2 or
	// the border-clamp extension).
	bool clamp_to_border = false;
	// The desktop GL_FRAMEBUFFER_SRGB enable/disable toggle exists
	// (desktop GL >= 3.0). ES converts automatically with no toggle.
	bool framebuffer_srgb = false;
	// RGBA16F is usable as a color-renderable FBO attachment (desktop
	// >= 3.0, ES >= 3.2, or EXT_color_buffer_half_float/float on ES 3.x).
	bool rgba16f_renderable = false;
};

// Classifies capability bits from the GL_VERSION string and (ES only) the
// GL_EXTENSIONS string. Both may be nullptr; a null version yields a zeroed
// snapshot. Pure function — no GL calls.
GlCapabilities classify_gl_capabilities(const char* gl_version, const char* gl_extensions);
