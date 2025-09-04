 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Blitter emulation
  *
  * (c) 1995 Bernd Schmidt
  */

#ifndef UAE_BLITTER_H
#define UAE_BLITTER_H

#include "uae/types.h"

struct bltinfo {
    int blitzero;
    uae_u16 bltadat, bltbdat, bltcdat, bltddat;
    uae_u16 bltaold, bltahold, bltbold, bltbhold, bltafwm, bltalwm;
    uae_u16 bltahold2, bltbhold2, bltchold2;
    int vblitsize, hblitsize;
    uae_s16 bltamod, bltbmod, bltcmod, bltdmod;
    uae_s16 bltamod_next, bltbmod_next, bltcmod_next, bltdmod_next;
    uae_u32 bltapt, bltbpt, bltcpt, bltdpt;
    uae_u32 bltapt_prev, bltbpt_prev, bltcpt_prev, bltdpt_prev;
    uae_u16 bltsize;
    uae_u16 bltcon0, bltcon1;
    int got_cycle;
    uae_u32 nasty_cnt, wait_nasty;
    int blitter_nasty, blit_interrupt;
    int blit_main, blit_pending, blit_count_done;
    int blit_queued;
    evt_t finishcycle_dmacon, finishcycle_copper;
    evt_t blt_ch_cycles[4];
    evt_t blt_mod_cycles[4], blt_change_cycles;
    uae_u16 bltddatl;
    uae_u32 bltdptl;
};

extern struct bltinfo blt_info;

extern bool blitter_delayed_update;

extern void maybe_blit(int);
extern void reset_blit(int);
extern int blitnasty(void);
extern void blitter_handler(uae_u32);
extern void build_blitfilltable(void);
extern void do_blitter(int, uaecptr);
extern void decide_blitter(int hpos);
extern void blitter_done_notify(void);
extern void blitter_slowdown(int, int, int, int);
extern void blitter_check_start(void);
extern void blitter_reset(void);
extern void blitter_debugdump(void);
extern void restore_blitter_start(void);

void blitter_loadbdat(uae_u16 v);
void blitter_loadcdat(uae_u16 v);

void process_blitter(struct rgabuf *rga);
void generate_blitter(void);

typedef void blitter_func(uaecptr, uaecptr, uaecptr, uaecptr, struct bltinfo *);

#define BLITTER_MAX_WORDS 2048

extern blitter_func *const blitfunc_dofast[256];
extern blitter_func *const blitfunc_dofast_desc[256];
extern uae_u32 blit_masktable[BLITTER_MAX_WORDS];

#endif /* UAE_BLITTER_H */
