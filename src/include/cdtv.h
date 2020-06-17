#ifndef UAE_CDTV_H
#define UAE_CDTV_H

#include "uae/types.h"

#ifdef CDTV

extern bool cdtv_init (struct autoconfig_info *aci);

void cdtv_battram_write (int addr, int v);
uae_u8 cdtv_battram_read (int addr);

extern void cdtv_add_scsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern bool cdtvscsi_init(struct autoconfig_info *aci);
extern bool cdtvsram_init(struct autoconfig_info *aci);

extern void cdtv_getdmadata (uae_u32*);

extern void cdtv_scsi_int (void);
extern void cdtv_scsi_clear_int (void);

extern bool cdtv_front_panel (int);

#endif /* CDTV */

#endif /* UAE_CDTV_H */
