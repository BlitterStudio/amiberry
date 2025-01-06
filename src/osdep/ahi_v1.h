#pragma once
extern void ahi_updatesound(int force);
extern uae_u32 REGPARAM2 ahi_demux (TrapContext*);
extern int ahi_open_sound ();
extern void ahi_close_sound ();
extern void ahi_finish_sound_buffer ();
extern void init_ahi();
extern void ahi_hsync();

extern int ahi_on;
extern int ahi_pollrate;
