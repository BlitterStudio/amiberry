#pragma once

/*
 * crt_gpu_allowlist.h - CRT shader quality GPU classification
 *
 * Classifies a GL_RENDERER string as capable of the full-quality CRT shaders
 * (vs the simplified low-power path used by crtemu). Pure string logic with
 * no GL/SDL dependencies so it is unit-testable without a GL context.
 *
 * Copyright 2026 Dimitris Panokostas
 */

// Returns true when the GL_RENDERER string is a known-capable ES GPU family
// (RPi5 V3D 7.x, Mali-G and later, Apple A/M-series, Adreno 6xx+, Tegra).
// Unknown, empty, or low-power GPUs (VideoCore, Mali-4xx/T-xxx, Adreno
// 3xx-5xx, PowerVR) return false and keep the simplified shader path.
bool crt_gpu_supports_full_crt(const char* renderer);
