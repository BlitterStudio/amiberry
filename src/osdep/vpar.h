/*
 * UAE - The Un*x Amiga Emulator
 *
 * Virtual parallel port protocol (vpar) for Amiga Emulators
 *
 * Written by Christian Vogelgsang <chris@vogelgsang.org>
 */

#ifndef UAE_VPAR_H
#define UAE_VPAR_H

#ifdef _WIN32
/* Not compatible wih Win32 API right now */
#undef WITH_VPAR
#endif

#ifdef WITH_VPAR

#define PAR_MODE_OFF     0
#define PAR_MODE_PRT     1
#define PAR_MODE_RAW     -1

void vpar_open(void);
void vpar_close(void);
void vpar_update(void);
int vpar_direct_write_status(uae_u8 v, uae_u8 dir);
int vpar_direct_read_status(uae_u8 *vp);
int vpar_direct_write_data(uae_u8 v, uae_u8 dir);
int vpar_direct_read_data(uae_u8 *v);

extern int par_fd;
extern int par_mode;

static inline bool vpar_enabled(void)
{
    return par_fd >= 0;
}

#endif

#endif /* UAE_VPAR_H */
