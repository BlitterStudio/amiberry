#ifndef UAE_NCR9X_SCSI_H
#define UAE_NCR9X_SCSI_H

#include "uae/types.h"

extern void cpuboard_ncr9x_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void cpuboard_dkb_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void fastlane_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void oktagon_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void masoboshi_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void trifecta_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void ematrix_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void multievolution_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void golemfast_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void scram5394_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void rapidfire_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void alf3_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void typhoon2scsi_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void squirrel_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void mtecmastercard_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

extern bool ncr_fastlane_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr_oktagon_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr_dkb_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr_ematrix_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr_multievolution_init(struct autoconfig_info *aci);
extern bool ncr_scram5394_init(struct autoconfig_info *aci);
extern bool ncr_rapidfire_init(struct autoconfig_info *aci);
extern bool ncr_alf3_autoconfig_init(struct autoconfig_info *aci);
extern bool ncr_mtecmastercard_init(struct autoconfig_info *aci);
extern bool typhoon2scsi_init(struct autoconfig_info *aci);

extern void cpuboard_ncr9x_scsi_put(uaecptr, uae_u32);
extern uae_u32 cpuboard_ncr9x_scsi_get(uaecptr);

uae_u32 masoboshi_ncr9x_scsi_get(uaecptr addr, int devnum);
void masoboshi_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum);
void ncr_masoboshi_autoconfig_init(struct romconfig*, uaecptr);

uae_u32 trifecta_ncr9x_scsi_get(uaecptr addr, int devnum);
void trifecta_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum);
void ncr_trifecta_autoconfig_init(struct romconfig*, uaecptr);

uae_u32 golemfast_ncr9x_scsi_get(uaecptr addr, int devnum);
void golemfast_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum);
void ncr_golemfast_autoconfig_init(struct romconfig*, uaecptr);

uae_u32 squirrel_ncr9x_scsi_get(uaecptr addr, int devnum);
void squirrel_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum);
void ncr_squirrel_init(struct romconfig *, uaecptr);
void pcmcia_interrupt_set(int);

#define BLIZZARD_2060_SCSI_OFFSET 0x1ff00
#define BLIZZARD_2060_DMA_OFFSET 0x1fff0
#define BLIZZARD_2060_LED_OFFSET 0x1ffe0

#define BLIZZARD_SCSI_KIT4_SCSI_OFFSET 0x8000
#define BLIZZARD_SCSI_KIT4_DMA_OFFSET 0x10000
#define BLIZZARD_SCSI_KIT3_SCSI_OFFSET 0x10000

#define CYBERSTORM_MK2_SCSI_OFFSET 0x1ff03
#define CYBERSTORM_MK2_LED_OFFSET 0x1ff43
#define CYBERSTORM_MK2_DMA_OFFSET 0x1ff83

#define CYBERSTORM_MK1_SCSI_OFFSET 0xf400
#define CYBERSTORM_MK1_LED_OFFSET 0xf4e0
#define CYBERSTORM_MK1_DMA_OFFSET 0xf800
#define CYBERSTORM_MK1_JUMPER_OFFSET 0xfc02

#endif /* UAE_NCR9X_SCSI_H */
