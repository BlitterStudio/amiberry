#ifndef UAE_NCR_SCSI_H
#define UAE_NCR_SCSI_H

#include "uae/types.h"

void ncr710_io_bput_a4000t(uaecptr, uae_u32);
uae_u32 ncr710_io_bget_a4000t(uaecptr);

void ncr815_io_bput_wildfire(uaecptr addr, uae_u32 v);
uae_u32 ncr815_io_bget_wildfire(uaecptr addr);
void wildfire_ncr815_irq(int id, int v);

extern addrbank ncr_bank_cyberstorm;
extern addrbank ncr_bank_generic;

extern void ncr_reset(int);

extern bool ncr710_a4091_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr710_warpengine_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr710_zeus040_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr710_magnum40_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr710_draco_init(struct autoconfig_info *aci);

void cpuboard_ncr710_io_bput(uaecptr addr, uae_u32 v);
uae_u32 cpuboard_ncr710_io_bget(uaecptr addr);
void cpuboard_ncr720_io_bput(uaecptr addr, uae_u32 v);
uae_u32 cpuboard_ncr720_io_bget(uaecptr addr);

extern void a4000t_add_scsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern bool is_a4000t_scsi(void);
extern bool a4000t_scsi_init(struct autoconfig_info *aci);

extern void warpengine_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void tekmagic_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void quikpak_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void cyberstorm_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void blizzardppc_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void a4091_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void wildfire_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void zeus040_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void magnum40_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void draco_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

#endif /* UAE_NCR_SCSI_H */
