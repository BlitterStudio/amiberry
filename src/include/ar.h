#ifndef UAE_AR_H
#define UAE_AR_H

#include "uae/types.h"

/* disable HRTMon support by commenting this out */
#define ACTION_REPLAY_HRTMON

#ifdef ACTION_REPLAY
#define ACTION_REPLAY_COMMON
#endif

#ifdef ACTION_REPLAY_HRTMON
#define ACTION_REPLAY_COMMON
#endif

#ifdef ACTION_REPLAY
/* disable Action Replay ROM/RAM hide by commenting this out */
/* Better not disable this unless you do plenty of testing first. -Mark */
#define ACTION_REPLAY_HIDE_CARTRIDGE
#endif

#define ACTION_REPLAY_WAIT_PC -3 	/* Wait for a specified Program counter */
#define ACTION_REPLAY_INACTIVE -2
#define ACTION_REPLAY_WAITRESET -1
#define ACTION_REPLAY_IDLE 1
#define ACTION_REPLAY_ACTIVATE 2
#define ACTION_REPLAY_ACTIVE 3
#define ACTION_REPLAY_DORESET 4
#define ACTION_REPLAY_HIDE 5

extern int action_replay_freeze (void);

extern uaecptr wait_for_pc;
extern int action_replay_flag;
extern int armodel;

extern int is_ar_pc_in_rom(void);
extern int is_ar_pc_in_ram(void);
extern void action_replay_enter (void);
extern void action_replay_cia_access (bool write);
extern void action_replay_hide (void);
extern void action_replay_reset (bool hardreset, bool keyboardreset);
extern int action_replay_load (void);
extern int action_replay_unload (int in_memory_reset);

extern void action_replay_memory_reset (void);
extern void action_replay_init (int);
extern void action_replay_cleanup (void);
extern void REGPARAM3 chipmem_lput_actionreplay23 (uaecptr addr, uae_u32 l) REGPARAM;
extern void REGPARAM3 chipmem_wput_actionreplay23 (uaecptr addr, uae_u32 w) REGPARAM;
extern void REGPARAM3 chipmem_bput_actionreplay1 (uaecptr addr, uae_u32 b) REGPARAM;
extern void REGPARAM3 chipmem_wput_actionreplay1 (uaecptr addr, uae_u32 w) REGPARAM;
extern void REGPARAM3 chipmem_lput_actionreplay1 (uaecptr addr, uae_u32 l) REGPARAM;

extern void action_replay_version (void);

extern int hrtmon_flag;

extern void hrtmon_enter (void);
extern void hrtmon_breakenter (void);
extern void hrtmon_ciaread (void);
extern void hrtmon_hide (void);
extern void hrtmon_reset (void);
extern int hrtmon_load (void);
extern void hrtmon_map_banks (void);

/*extern uae_u8 *hrtmemory;*/
extern uae_u32 hrtmem_start, hrtmem_size, hrtmem2_start, hrtmem3_start;

extern uae_u8 ar_custom[2*256], ar_ciaa[16], ar_ciab[16];

extern int hrtmon_lang;

#endif /* UAE_AR_H */
