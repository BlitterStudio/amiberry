#pragma once
/*
 * display_modes.h - Display mode enumeration and query functions
 *
 * Extracted from amiberry_gfx.cpp to reduce translation unit size.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#include <SDL3/SDL.h>
#include "uae/types.h"

// Forward declarations
struct AmigaMonitor;
struct MultiDisplay;
struct uae_prefs;

// Display query
int gfx_IsPicassoScreen(const struct AmigaMonitor* mon);
int isfullscreen_2(const struct uae_prefs* p);
int isfullscreen();
int gfx_GetWidth(const struct AmigaMonitor* mon);
int gfx_GetHeight(const struct AmigaMonitor* mon);

// Display lookup
struct MultiDisplay* getdisplay2(const struct uae_prefs* p, int index);
struct MultiDisplay* getdisplay(const struct uae_prefs* p, int monid);

// Desktop / display geometry
void desktop_coords(int monid, int* dw, int* dh, int* ax, int* ay, int* aw, int* ah);
int target_get_display(const TCHAR* name);
const TCHAR* target_get_display_name(int num, bool friendlyname);
int target_get_display_scanline(int displayindex);
void centerdstrect(struct AmigaMonitor* mon, SDL_Rect* dr);

// Refresh / vblank
int getrefreshrate(int monid, int width, int height);
float target_getcurrentvblankrate(int monid);
void target_cpu_speed();
void display_param_init(struct AmigaMonitor* mon);

// Mode enumeration and sorting
void reenumeratemonitors();
void enumeratedisplays();
void sortdisplays();
int gfx_adjust_screenmode(const MultiDisplay* md, int* pwidth, int* pheight);
int getbestmode(struct AmigaMonitor* mon, int nextbest);

// Shared state (defined in display_modes.cpp)
extern int deskhz;
extern bool calculated_scanline;
