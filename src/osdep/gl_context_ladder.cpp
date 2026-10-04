/*
 * gl_context_ladder.cpp - pure GL context-ladder step selection
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "gl_context_ladder.h"

GlLadderStep select_gl_ladder_step(GlLadderProfile profile, bool es_fallback_tier, int mode)
{
	switch (profile) {
	case GlLadderProfile::GlesPreferred:
		// KMSDRM: 0/1 = GLES 3.0 full/minimal (preferred), then the
		// desktop Core/Legacy tiers (2/3 full, 4/5 minimal) so drivers
		// without ES 3.0 (RPi3 vc4) keep hardware GL 2.1 instead of
		// exhausting the ladder and demoting to the software renderer.
		switch (mode) {
		case 0: return { true, GlLadderApi::Es3, false };
		case 1: return { true, GlLadderApi::Es3, true };
		case 2: return { true, GlLadderApi::Core, false };
		case 3: return { true, GlLadderApi::Legacy, false };
		case 4: return { true, GlLadderApi::Core, true };
		case 5: return { true, GlLadderApi::Legacy, true };
		}
		return {};
	case GlLadderProfile::GlesOnly:
		// Android/iOS/USE_GLES3: desktop requests can never succeed.
		switch (mode) {
		case 0: return { true, GlLadderApi::Es3, false };
		case 1: return { true, GlLadderApi::Es3, true };
		}
		return {};
	case GlLadderProfile::Desktop:
		break;
	}
	if (es_fallback_tier && (mode == 4 || mode == 5))
		return { true, GlLadderApi::Es3, mode == 5 };
	switch (mode) {
	case 0: return { true, GlLadderApi::Core, false };
	case 1: return { true, GlLadderApi::Legacy, false };
	case 2: return { true, GlLadderApi::Core, true };
	case 3: return { true, GlLadderApi::Legacy, true };
	}
	return {};
}
