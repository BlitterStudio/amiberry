 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff, Amiberry version
  *
  * Copyright 1997 Bernd Schmidt
  */

#pragma once
#include "SDL.h"

#define TARGET_NAME "amiberry"

#define NO_MAIN_IN_MAIN_C

#define OPTIONSFILENAME "uaeconfig"

extern int emulating;

extern int z3_start_adr;
extern int rtg_start_adr;

void run_gui();
void InGameMessage(const char *msg);
void wait_for_vsync();

void saveAdfDir();
void update_display(struct uae_prefs *);
void graphics_subshutdown();

void amiberry_stop_sound();

void keyboard_settrans();
void translate_amiberry_keys(int, int, int);
void SimulateMouseOrJoy(int code, int keypressed);

#define REMAP_MOUSEBUTTON_LEFT    -1
#define REMAP_MOUSEBUTTON_RIGHT   -2
#define REMAP_JOYBUTTON_ONE       -3
#define REMAP_JOYBUTTON_TWO       -4
#define REMAP_JOY_UP              -5
#define REMAP_JOY_DOWN            -6
#define REMAP_JOY_LEFT            -7
#define REMAP_JOY_RIGHT           -8
#define REMAP_CD32_GREEN          -9
#define REMAP_CD32_YELLOW         -10
#define REMAP_CD32_PLAY           -11
#define REMAP_CD32_FFW            -12
#define REMAP_CD32_RWD            -13

extern void free_AmigaMem();
extern void alloc_AmigaMem();

void reinit_amiga();
int count_HDs(struct uae_prefs *p);
extern void gui_force_rtarea_hdchange();
extern bool hardfile_testrdb(const TCHAR *filename);

#ifdef __cplusplus
extern "C" {
#endif
	void trace_begin(void);
	void trace_end(void);
#ifdef __cplusplus
}
#endif


STATIC_INLINE size_t uae_tcslcpy(TCHAR *dst, const TCHAR *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	size_t src_len = _tcslen(src);
	size_t cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len * sizeof(TCHAR));
	dst[cpy_len] = _T('\0');
	return src_len;
}

STATIC_INLINE size_t uae_strlcpy(char *dst, const char *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	size_t src_len = strlen(src);
	size_t cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len);
	dst[cpy_len] = '\0';
	return src_len;
}

STATIC_INLINE int max(int x, int y)
{
	return x > y ? x : y;
}
