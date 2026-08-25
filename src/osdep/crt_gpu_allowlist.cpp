/*
 * crt_gpu_allowlist.cpp - CRT shader quality GPU classification
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "crt_gpu_allowlist.h"

#include <cstring>

bool crt_gpu_supports_full_crt(const char* renderer)
{
	if (renderer == nullptr || renderer[0] == '\0')
		return false;
	if (strstr(renderer, "V3D 7"))
		return true; /* RPi5 */
	if (strstr(renderer, "Mali-G"))
		return true; /* Bifrost and later */
	if (strncmp(renderer, "Apple", 5) == 0)
		return true; /* Apple A/M-series (iOS) */
	if (strstr(renderer, "Adreno (TM) 6") || strstr(renderer, "Adreno (TM) 7")
		|| strstr(renderer, "Adreno (TM) 8"))
		return true;
	if (strstr(renderer, "Tegra"))
		return true; /* Shield-class Tegra */
	return false;
}
