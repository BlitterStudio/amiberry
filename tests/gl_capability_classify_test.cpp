#include <iostream>
#include <string>

#include "gl_capability_classify.h"

namespace {

int failures;

void expect(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

GlCapabilities check(const char* version, const char* extensions, const std::string& label)
{
	const GlCapabilities caps = classify_gl_capabilities(version, extensions);
	expect(caps.major >= 0 && caps.minor >= 0, label + ": negative version fields");
	return caps;
}

} // namespace

int main()
{
	// Null / empty inputs yield a zeroed snapshot.
	const GlCapabilities none = check(nullptr, nullptr, "null version");
	expect(!none.is_gles && none.major == 0 && none.minor == 0, "null version: zeroed");
	expect(!none.clamp_to_border && !none.framebuffer_srgb && !none.rgba16f_renderable,
		"null version: all features off");

	// Desktop GL 4.6 core (typical Windows/Linux driver).
	const GlCapabilities desk46 = check("4.6.0 NVIDIA 566.36", nullptr, "desktop 4.6");
	expect(!desk46.is_gles && desk46.major == 4 && desk46.minor == 6, "desktop 4.6: parsed");
	expect(desk46.clamp_to_border && desk46.framebuffer_srgb && desk46.rgba16f_renderable,
		"desktop 4.6: all features on");

	// Desktop GL 3.3 core (macOS ceiling, common fallback tier).
	const GlCapabilities desk33 = check("3.3 (Core Profile) Mesa 24.0.9", nullptr, "desktop 3.3");
	expect(!desk33.is_gles && desk33.major == 3 && desk33.minor == 3, "desktop 3.3: parsed");
	expect(desk33.clamp_to_border && desk33.framebuffer_srgb && desk33.rgba16f_renderable,
		"desktop 3.3: all features on");

	// Desktop GL 3.0/3.1: border clamp is core since GL 1.3, so it stays on.
	const GlCapabilities desk30 = check("3.0 Mesa 21.3.9", nullptr, "desktop 3.0");
	expect(desk30.clamp_to_border, "desktop 3.0: clamp_to_border on (core since 1.3)");
	expect(desk30.rgba16f_renderable && desk30.framebuffer_srgb, "desktop 3.0: 3.0 features on");

	// Desktop GL 2.1 (legacy fallback tier): no float render, no sRGB toggle.
	const GlCapabilities desk21 = check("2.1.2 NVIDIA-550", nullptr, "desktop 2.1");
	expect(!desk21.rgba16f_renderable && !desk21.framebuffer_srgb, "desktop 2.1: 3.0 features off");
	expect(desk21.clamp_to_border, "desktop 2.1: clamp_to_border on (core since 1.3)");

	// ES 3.2: everything core except the sRGB toggle.
	const GlCapabilities es32 = check("OpenGL ES 3.2 v1.r14p0-01eac0", nullptr, "ES 3.2");
	expect(es32.is_gles && es32.major == 3 && es32.minor == 2, "ES 3.2: parsed");
	expect(es32.clamp_to_border && es32.rgba16f_renderable, "ES 3.2: border + float on");
	expect(!es32.framebuffer_srgb, "ES 3.2: no GL_FRAMEBUFFER_SRGB toggle");

	// ES 3.1 + EXT_color_buffer_half_float: float on, border off without ext.
	const GlCapabilities es31hf = check("OpenGL ES 3.1 Mesa 24.0.9",
		"GL_EXT_color_buffer_half_float GL_EXT_texture_norm16", "ES 3.1 + half float");
	expect(es31hf.rgba16f_renderable, "ES 3.1 + ext: float renderable");
	expect(!es31hf.clamp_to_border, "ES 3.1 without border ext: clamp_to_border off");

	// ES 3.1 + both border and float extensions.
	const GlCapabilities es31both = check("OpenGL ES 3.1 Mesa 24.0.9",
		"GL_EXT_texture_border_clamp GL_EXT_color_buffer_half_float", "ES 3.1 + both");
	expect(es31both.clamp_to_border && es31both.rgba16f_renderable, "ES 3.1 + both: on");

	// ES 3.0 (RPi4 VideoCore): nothing optional available.
	const GlCapabilities es30 = check("OpenGL ES 3.1 Mesa v3d-drm-fk-v6.4.14", nullptr, "ES 3.1 bare");
	expect(!es30.clamp_to_border && !es30.rgba16f_renderable && !es30.framebuffer_srgb,
		"ES 3.1 bare: optional features off");

	// ES 1.x-style string parses without crashing and stays minimal.
	const GlCapabilities es11 = check("OpenGL ES-CM 1.1", nullptr, "ES 1.1");
	expect(es11.is_gles && !es11.clamp_to_border && !es11.rgba16f_renderable, "ES 1.1: minimal");

	// Extensions must be ignored on desktop paths (no false-positive matches).
	const GlCapabilities desk33ext = check("3.3 (Core Profile) Mesa 24.0.9",
		"GL_EXT_texture_border_clamp GL_EXT_color_buffer_half_float", "desktop 3.3 + ext noise");
	expect(desk33ext.clamp_to_border && desk33ext.rgba16f_renderable, "desktop 3.3: unchanged by exts");

	if (failures > 0) {
		std::cerr << failures << " gl_capability_classify failure(s)\n";
		return 1;
	}
	std::cout << "gl_capability_classify: all checks passed\n";
	return 0;
}
