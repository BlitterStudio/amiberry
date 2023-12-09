#pragma once

#include "uae/types.h"

extern int midi_emu;
extern void midi_emu_close(void);
extern int midi_emu_open(const TCHAR *);
extern void midi_emu_parse(uae_u8 *midi, int len);
extern bool midi_emu_available(const TCHAR *);
extern void midi_emu_reopen(void);
extern void midi_update_sound(float);
