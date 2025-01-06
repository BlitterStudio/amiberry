 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Save/restore emulator state
  *
  * (c) 1999-2001 Toni Wilen
  */

#ifndef UAE_SAVESTATE_H
#define UAE_SAVESTATE_H

#include "uae/types.h"

/* functions to save byte,word or long word
 * independent of CPU's endianness */

extern void save_store_pos_func (uae_u8 **);
extern void save_store_size_func (uae_u8 **);
extern void restore_store_pos_func (uae_u8 **);
extern void restore_store_size_func (uae_u8 **);

#define save_store_pos() save_store_pos_func (&dst)
#define save_store_size() save_store_size_func (&dst)
#define restore_store_pos() restore_store_pos_func (&src)
#define restore_store_size() restore_store_size_func (&src)

extern void save_u64_func(uae_u8 **, uae_u64);
extern void save_u32t_func(uae_u8 **, size_t);
extern void save_u32_func(uae_u8 **, uae_u32);
extern void save_u16_func(uae_u8 **, uae_u16);
extern void save_u8_func(uae_u8 **, uae_u8);

extern uae_u64 restore_u64_func(uae_u8 **);
extern uae_u32 restore_u32_func(uae_u8 **);
extern uae_u16 restore_u16_func(uae_u8 **);
extern uae_u8 restore_u8_func(uae_u8 **);

extern void save_string_func(uae_u8 **, const TCHAR*);
extern TCHAR *restore_string_func(uae_u8 **);

#define SAVESTATE_PATH 0
#define SAVESTATE_PATH_FLOPPY 1
#define SAVESTATE_PATH_VDIR 2
#define SAVESTATE_PATH_HDF 3
#define SAVESTATE_PATH_HD 4
#define SAVESTATE_PATH_CD 5

extern void save_path_func (uae_u8 **, const TCHAR*, int type);
extern void save_path_full_func(uae_u8 **, const TCHAR*, int type);
extern TCHAR *restore_path_func(uae_u8 **, int type);
extern TCHAR *restore_path_full_func(uae_u8 **);

#define save_u64(x) save_u64_func(&dst, (x))
#define save_u32(x) save_u32_func(&dst, (x))
#define save_u32t(x) save_u32t_func(&dst, (x))
#define save_u16(x) save_u16_func(&dst, (x))
#define save_u8(x) save_u8_func(&dst, (x))

#define restore_u64() restore_u64_func(&src)
#define restore_u64to32() (uae_u32)restore_u64_func(&src)
#define restore_u32() restore_u32_func(&src)
#define restore_u16() restore_u16_func(&src)
#define restore_u8() restore_u8_func(&src)

#define save_string(x) save_string_func(&dst, (x))
#define restore_string() restore_string_func(&src)

#define save_path(x, p) save_path_func(&dst, (x), p)
#define save_path_full(x, p) save_path_full_func(&dst, (x), p)
#define restore_path(p) restore_path_func(&src, p)
#define restore_path_full() restore_path_full_func(&src)

/* save, restore and initialize routines for Amiga's subsystems */

extern uae_u8 *restore_cpu(uae_u8 *);
extern void restore_cpu_finish(void);
extern uae_u8 *save_cpu(size_t *, uae_u8 *);
extern uae_u8 *restore_cpu_extra(uae_u8 *);
extern uae_u8 *save_cpu_extra(size_t *, uae_u8 *);
extern uae_u8 *save_cpu_trace(size_t *, uae_u8 *);
extern uae_u8 *restore_cpu_trace(uae_u8 *);

extern uae_u8 *restore_mmu(uae_u8 *);
extern uae_u8 *save_mmu(size_t *, uae_u8 *);

extern uae_u8 *restore_fpu(uae_u8 *);
extern uae_u8 *save_fpu(size_t *, uae_u8 *);

extern uae_u8 *restore_disk(int, uae_u8 *);
extern uae_u8 *save_disk(int, size_t *, uae_u8 *, bool);
extern uae_u8 *restore_floppy(uae_u8 *src);
extern uae_u8 *save_floppy(size_t *len, uae_u8 *);
extern uae_u8 *save_disk2(int num, size_t *len, uae_u8 *dstptr);
extern uae_u8 *restore_disk2(int num, uae_u8 *src);
extern void DISK_save_custom (uae_u32 *pdskpt, uae_u16 *pdsklen, uae_u16 *pdsksync, uae_u16 *pdskbytr);
extern void DISK_restore_custom (uae_u32 pdskpt, uae_u16 pdsklength, uae_u16 pdskbytr);
extern void restore_disk_finish(void);

extern uae_u8 *restore_custom(uae_u8 *);
extern uae_u8 *save_custom(size_t *, uae_u8 *, int);
extern uae_u8 *restore_custom_extra(uae_u8 *);
extern uae_u8 *save_custom_extra(size_t *, uae_u8 *);
extern void restore_custom_finish(void);
extern void restore_custom_start(void);

extern uae_u8 *restore_custom_sprite(int num, uae_u8 *src);
extern uae_u8 *save_custom_sprite(int num, size_t *len, uae_u8 *);

extern uae_u8 *restore_custom_agacolors (uae_u8 *src);
extern uae_u8 *save_custom_agacolors(size_t *len, uae_u8 *);

extern uae_u8 *restore_custom_event_delay (uae_u8 *src);
extern uae_u8 *save_custom_event_delay(size_t *len, uae_u8 *dstptr);

extern uae_u8 *restore_custom_slots(uae_u8 *src);
extern uae_u8 *save_custom_slots(size_t *len, uae_u8 *dstptr);

extern uae_u8 *restore_blitter (uae_u8 *src);
extern uae_u8 *save_blitter (size_t *len, uae_u8 *, bool);
extern uae_u8 *restore_blitter_new (uae_u8 *src);
extern uae_u8 *save_blitter_new (size_t *len, uae_u8 *);
extern void restore_blitter_finish (void);

extern uae_u8 *restore_audio(int, uae_u8 *);
extern uae_u8 *save_audio(int, size_t *, uae_u8 *);
extern void restore_audio_finish(void);
extern void restore_audio_start(void);

extern uae_u8 *restore_cia(int, uae_u8 *);
extern uae_u8 *save_cia(int, size_t *, uae_u8 *);
extern void restore_cia_finish(void);
extern void restore_cia_start(void);

extern uae_u8 *restore_expansion(uae_u8 *);
extern uae_u8 *save_expansion(size_t *, uae_u8 *);

extern uae_u8 *restore_p96(uae_u8 *);
extern uae_u8 *save_p96(size_t *, uae_u8 *);
extern void restore_p96_finish(void);

extern uae_u8 *restore_keyboard(uae_u8 *);
extern uae_u8 *save_keyboard(size_t *,uae_u8*);

extern uae_u8 *restore_kbmcu(uae_u8 *);
extern uae_u8 *save_kbmcu(size_t *,uae_u8*);
extern uae_u8 *restore_kbmcu2(uae_u8 *);
extern uae_u8 *save_kbmcu2(size_t *,uae_u8*);
extern uae_u8 *restore_kbmcu3(uae_u8 *);
extern uae_u8 *save_kbmcu3(size_t *,uae_u8*);

extern uae_u8 *restore_akiko(uae_u8 *src);
extern uae_u8 *save_akiko(size_t *len, uae_u8*);
extern void restore_akiko_finish(void);
extern void restore_akiko_final(void);

extern uae_u8 *restore_cdtv(uae_u8 *src);
extern uae_u8 *save_cdtv(size_t *len, uae_u8*);
extern void restore_cdtv_finish(void);
extern void restore_cdtv_final(void);

extern uae_u8 *restore_cdtv_dmac(uae_u8 *src);
extern uae_u8 *save_cdtv_dmac(size_t *len, uae_u8*);
extern uae_u8 *restore_scsi_dmac(int wdtype, uae_u8 *src);
extern uae_u8 *save_scsi_dmac(int wdtype, int *len, uae_u8*);

extern uae_u8 *save_scsi_device(int wdtype, int num, size_t *len, uae_u8 *dstptr);
extern uae_u8 *restore_scsi_device(int wdtype, uae_u8 *src);

extern uae_u8 *save_scsidev(int num, size_t *len, uae_u8 *dstptr);
extern uae_u8 *restore_scsidev(uae_u8 *src);

extern uae_u8 *restore_filesys(uae_u8 *src);
extern uae_u8 *save_filesys(int num, size_t *len);
extern uae_u8 *restore_filesys_common(uae_u8 *src);
extern uae_u8 *save_filesys_common(size_t *len);
extern uae_u8 *restore_filesys_paths(uae_u8 *src);
extern uae_u8 *save_filesys_paths(int num, size_t *len);
extern int save_filesys_cando(void);

extern uae_u8 *restore_gayle(uae_u8 *src);
extern uae_u8 *save_gayle(size_t *len, uae_u8*);
extern uae_u8 *restore_gayle_ide(uae_u8 *src);
extern uae_u8 *save_gayle_ide(int num, size_t *len, uae_u8*);

extern uae_u8 *save_cd(int num, size_t *len);
extern uae_u8 *restore_cd(int, uae_u8 *src);
extern void restore_cd_finish(void);

extern uae_u8 *save_configuration(size_t *len, bool fullconfig);
extern uae_u8 *restore_configuration(uae_u8 *src);
extern uae_u8 *save_log(int, size_t *len);
//extern uae_u8 *restore_log (uae_u8 *src);

extern uae_u8 *restore_input(uae_u8 *src);
extern uae_u8 *save_input(size_t *len, uae_u8 *dstptr);

extern uae_u8 *restore_inputstate(uae_u8 *src);
extern uae_u8 *save_inputstate(size_t *len, uae_u8 *dstptr);
extern void clear_inputstate(void);

extern uae_u8 *save_a2065(size_t *len, uae_u8 *dstptr);
extern uae_u8 *restore_a2065(uae_u8 *src);
extern void restore_a2065_finish(void);

extern uae_u8 *restore_debug_memwatch(uae_u8 *src);
extern uae_u8 *save_debug_memwatch(size_t *len, uae_u8 *dstptr);
extern void restore_debug_memwatch_finish(void);

extern uae_u8 *save_screenshot(int monid, size_t *len);

extern uae_u8 *save_cycles(size_t *len, uae_u8 *dstptr);
extern uae_u8 *restore_cycles(uae_u8 *src);

extern uae_u8 *save_alg(size_t *len);
extern uae_u8 *restore_alg(uae_u8 *src);

extern void restore_cram(int, size_t);
extern void restore_bram(int, size_t);
extern void restore_fram(int, size_t, int);
extern void restore_zram(int, size_t, int);
extern void restore_bootrom(int, size_t);
extern void restore_pram(int, size_t);
extern void restore_a3000lram(int, size_t);
extern void restore_a3000hram(int, size_t);

extern void restore_ram (size_t, uae_u8*);

extern uae_u8 *save_cram(size_t *);
extern uae_u8 *save_bram(size_t *);
extern uae_u8 *save_fram(size_t *, int);
extern uae_u8 *save_zram(size_t *, int);
extern uae_u8 *save_bootrom(size_t *);
extern uae_u8 *save_pram(size_t *);
extern uae_u8 *save_a3000lram (size_t *);
extern uae_u8 *save_a3000hram (size_t *);

extern uae_u8 *restore_rom(uae_u8 *);
extern uae_u8 *save_rom(int, size_t *, uae_u8 *);

extern uae_u8 *save_expansion_boards(size_t *, uae_u8*, int);
extern uae_u8 *restore_expansion_boards(uae_u8*);
#if 0
extern uae_u8 *save_expansion_info_old(int*, uae_u8*);
extern uae_u8 *restore_expansion_info_old(uae_u8*);
#endif
extern void restore_expansion_finish(void);

extern uae_u8 *restore_action_replay(uae_u8 *);
extern uae_u8 *save_action_replay(size_t *, uae_u8 *);
extern uae_u8 *restore_hrtmon(uae_u8 *);
extern uae_u8 *save_hrtmon(size_t *, uae_u8 *);
extern void restore_ar_finish(void);

extern void savestate_initsave(const TCHAR *filename, int docompress, int nodialogs, bool save);
extern int save_state(const TCHAR *filename, const TCHAR *description);
extern void restore_state(const TCHAR *filename);
extern bool savestate_restore_finish(void);
extern void savestate_restore_final(void);
extern void savestate_memorysave(void);
extern bool is_savestate_incompatible(void);

extern void custom_prepare_savestate(void);

extern bool savestate_check(void);

#define STATE_SAVE 1
#define STATE_RESTORE 2
#define STATE_DOSAVE 4
#define STATE_DORESTORE 8
#define STATE_REWIND 16
#define STATE_DOREWIND 32

#define STATE_SAVE_DESCRIPTION _T("Description!")

extern int savestate_state;
extern TCHAR savestate_fname[MAX_DPATH];
extern TCHAR path_statefile[MAX_DPATH];
extern struct zfile *savestate_file;

STATIC_INLINE bool isrestore(void)
{
	return savestate_state == STATE_RESTORE || savestate_state == STATE_REWIND;
}

extern void savestate_quick(int slot, int save);

extern void savestate_capture(int);
extern void savestate_free(void);
extern void savestate_init(void);
extern void savestate_rewind(void);
extern int savestate_dorewind(int);
extern void savestate_listrewind(void);
extern void statefile_save_recording(const TCHAR*);
extern void savestate_capture_request(void);

#endif /* UAE_SAVESTATE_H */
