#ifndef UAE_CD32_FMV_H
#define UAE_CD32_FMV_H

#include "uae/types.h"

extern addrbank *cd32_fmv_init (struct autoconfig_info *aci);
extern void cd32_fmv_reset(void);
extern void cd32_fmv_free(void);
extern void rethink_cd32fmv(void);
extern void cd32_fmv_hsync_handler(void);
extern void cd32_fmv_vsync_handler(void);

extern void cd32_fmv_state(int state);
extern void cd32_fmv_new_image(int, int, int, uae_u8*);
extern void cd32_fmv_genlock(struct vidbuffer*, struct vidbuffer*);
extern void cd32_fmv_new_border_color(uae_u32);
extern void cd32_fmv_set_sync(float svpos, float adjust);

extern int cd32_fmv_active;

#endif /* UAE_CD32_FMV_H */
