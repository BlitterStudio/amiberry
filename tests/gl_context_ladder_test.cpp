#include <iostream>
#include <string>

#include "gl_context_ladder.h"

namespace {

int failures;

void expect(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

std::string step_label(const GlLadderStep& step)
{
	if (!step.valid)
		return "invalid";
	const char* api = step.api == GlLadderApi::Es3 ? "Es3"
		: step.api == GlLadderApi::Legacy ? "Legacy" : "Core";
	return std::string(api) + (step.minimal_attrs ? "/minimal" : "/full");
}

void expect_step(GlLadderProfile profile, bool es_fallback, int mode,
	const std::string& wanted, const std::string& label)
{
	const std::string got = step_label(
		select_gl_ladder_step(profile, es_fallback, mode));
	expect(got == wanted, label + ": expected " + wanted + ", got " + got);
}

} // namespace

int main()
{
	// Desktop ladder (x11/wayland/windows/cocoa, standard build).
	expect_step(GlLadderProfile::Desktop, false, 0, "Core/full", "desktop mode 0");
	expect_step(GlLadderProfile::Desktop, false, 1, "Legacy/full", "desktop mode 1");
	expect_step(GlLadderProfile::Desktop, false, 2, "Core/minimal", "desktop mode 2");
	expect_step(GlLadderProfile::Desktop, false, 3, "Legacy/minimal", "desktop mode 3");
	expect_step(GlLadderProfile::Desktop, false, 4, "invalid", "desktop mode 4 without fallback build");
	expect_step(GlLadderProfile::Desktop, false, 5, "invalid", "desktop mode 5 without fallback build");

	// Desktop ladder with the AMIBERRY_GLES_FALLBACK tier enabled.
	expect_step(GlLadderProfile::Desktop, true, 0, "Core/full", "fallback build mode 0");
	expect_step(GlLadderProfile::Desktop, true, 3, "Legacy/minimal", "fallback build mode 3");
	expect_step(GlLadderProfile::Desktop, true, 4, "Es3/full", "fallback build mode 4");
	expect_step(GlLadderProfile::Desktop, true, 5, "Es3/minimal", "fallback build mode 5");

	// KMSDRM ladder (GLES preferred): ES 3.0 first, then desktop tiers so
	// drivers without ES 3.0 (RPi3 vc4) keep hardware GL 2.1 instead of
	// demoting to the software renderer.
	expect_step(GlLadderProfile::GlesPreferred, false, 0, "Es3/full", "kmsdrm mode 0");
	expect_step(GlLadderProfile::GlesPreferred, false, 1, "Es3/minimal", "kmsdrm mode 1");
	expect_step(GlLadderProfile::GlesPreferred, false, 2, "Core/full", "kmsdrm mode 2");
	expect_step(GlLadderProfile::GlesPreferred, false, 3, "Legacy/full", "kmsdrm mode 3 (vc4 keeps GL 2.1)");
	expect_step(GlLadderProfile::GlesPreferred, false, 4, "Core/minimal", "kmsdrm mode 4");
	expect_step(GlLadderProfile::GlesPreferred, false, 5, "Legacy/minimal", "kmsdrm mode 5");
	expect_step(GlLadderProfile::GlesPreferred, false, 6, "invalid", "kmsdrm mode 6");
	expect_step(GlLadderProfile::GlesPreferred, true, 3, "Legacy/full", "kmsdrm fallback build mode 3 unchanged");

	// GLES-only builds (Android/iOS/USE_GLES3): strict as before — desktop
	// requests can never succeed there.
	expect_step(GlLadderProfile::GlesOnly, false, 0, "Es3/full", "gles-only mode 0");
	expect_step(GlLadderProfile::GlesOnly, false, 1, "Es3/minimal", "gles-only mode 1");
	expect_step(GlLadderProfile::GlesOnly, false, 2, "invalid", "gles-only mode 2 stays strict");
	expect_step(GlLadderProfile::GlesOnly, false, 5, "invalid", "gles-only mode 5 stays strict");

	// Out-of-range modes are invalid on every ladder.
	expect_step(GlLadderProfile::Desktop, false, -1, "invalid", "negative mode");
	expect_step(GlLadderProfile::Desktop, true, 7, "invalid", "mode past the table");

	if (failures > 0) {
		std::cerr << failures << " gl_context_ladder failure(s)\n";
		return 1;
	}
	std::cout << "gl_context_ladder: all checks passed\n";
	return 0;
}
