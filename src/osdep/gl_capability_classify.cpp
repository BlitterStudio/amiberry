/*
 * gl_capability_classify.cpp - pure GL capability classification
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "gl_capability_classify.h"

#include <cstdlib>
#include <cstring>

GlCapabilities classify_gl_capabilities(const char* gl_version, const char* gl_extensions)
{
	GlCapabilities caps = {};

	if (gl_version == nullptr)
		return caps;

	caps.is_gles = (strstr(gl_version, "OpenGL ES") != nullptr);
	{
		const char* v = gl_version;
		while (*v && (*v < '0' || *v > '9')) v++;
		if (*v) {
			caps.major = atoi(v);
			while (*v && *v != '.') v++;
			if (*v == '.') {
				v++;
				caps.minor = atoi(v);
			}
		}
	}

	const auto version_at_least = [&caps](int major, int minor) {
		return caps.major > major || (caps.major == major && caps.minor >= minor);
	};

	if (caps.is_gles) {
		const auto has_ext = [gl_extensions](const char* name) {
			return gl_extensions && strstr(gl_extensions, name) != nullptr;
		};
		caps.clamp_to_border = version_at_least(3, 2)
			|| has_ext("GL_EXT_texture_border_clamp")
			|| has_ext("GL_OES_texture_border_clamp");
		caps.rgba16f_renderable = version_at_least(3, 2)
			|| has_ext("GL_EXT_color_buffer_half_float")
			|| has_ext("GL_EXT_color_buffer_float");
		// ES has no GL_FRAMEBUFFER_SRGB toggle; sRGB write conversion is
		// automatic for sRGB-format attachments.
		caps.framebuffer_srgb = false;
	} else {
		// Desktop: border-color wrap has been core since GL 1.3 and float
		// renderability since 3.0; the context ladder only obtains GL >= 2.1.
		caps.clamp_to_border = true;
		caps.rgba16f_renderable = version_at_least(3, 0);
		caps.framebuffer_srgb = version_at_least(3, 0);
	}

	return caps;
}
