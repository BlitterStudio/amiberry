#pragma once
extern uae_u32 REGPARAM2 ahi_demux (TrapContext*);
extern int ahi_open_sound ();
extern void ahi_close_sound ();
extern void init_ahi();
extern void ahi_hsync();
// Apply the current AHI (board) and master volume prefs to the AHI stream.
extern void ahi_apply_volume();

extern int ahi_on;
extern int ahi_pollrate;
