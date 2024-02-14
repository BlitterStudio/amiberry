#ifndef UAE_ARCADIA_H
#define UAE_ARCADIA_H

#ifdef ARCADIA

extern int is_arcadia_rom (const TCHAR *path);
extern int arcadia_map_banks (void);
extern void arcadia_unmap (void);
extern uae_u8 arcadia_parport (int port, uae_u8 pra, uae_u8 dra);
extern struct romdata *scan_arcadia_rom (TCHAR*, int);

struct arcadiarom {
    int romid;
    const TCHAR *name, *romid1, *romid2;
    int type, extra;
    int b7, b6, b5, b4, b3, b2, b1, b0;
	const TCHAR *ext;
	const TCHAR *exts[24 + 1];
};

extern struct arcadiarom *arcadia_bios, *arcadia_game;
extern int arcadia_flag, arcadia_coin[2];

#define NO_ARCADIA_ROM 0
#define ARCADIA_BIOS 1
#define ARCADIA_GAME 2

extern int alg_flag;
extern void alg_map_banks(void);
extern bool alg_ld_active(void);

extern int alg_get_player(uae_u16);
extern uae_u16 alg_potgor(uae_u16);
extern uae_u16 alg_joydat(int, uae_u16);
extern uae_u8 alg_joystick_buttons(uae_u8, uae_u8, uae_u8);
extern uae_u8 alg_parallel_port(uae_u8, uae_u8);
extern struct romdata *get_alg_rom(const TCHAR *name);

extern void ld_serial_read(uae_u16 v);
extern int ld_serial_write(void);

extern int cubo_enabled;
extern void touch_serial_read(uae_u16 w);
extern int touch_serial_write(void);

extern bool cubo_init(struct autoconfig_info *aci);

extern void cubo_function(int);

#endif /* ARCADIA */

#endif /* UAE_ARCADIA_H */
