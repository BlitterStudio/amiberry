#ifndef UAE_CPUBOARD_H
#define UAE_CPUBOARD_H

#include "uae/types.h"

bool cpuboard_autoconfig_init(struct autoconfig_info*);
bool cpuboard_maprom(void);
void cpuboard_map(void);
void cpuboard_reset(int hardreset);
void cpuboard_rethink(void);
void cpuboard_cleanup(void);
void cpuboard_init(void);
void cpuboard_clear(void);
bool cpuboard_is_ppcboard_irq(void);
int cpuboard_memorytype(struct uae_prefs *p);
int cpuboard_maxmemory(struct uae_prefs *p);
bool cpuboard_32bit(struct uae_prefs *p);
bool cpuboard_jitdirectompatible(struct uae_prefs *p);
bool is_ppc_cpu(struct uae_prefs *);
bool cpuboard_io_special(int addr, uae_u32 *val, int size, bool write);
void cpuboard_overlay_override(void);
void cpuboard_setboard(struct uae_prefs *p, int type, int subtype);
uaecptr cpuboard_get_reset_pc(uaecptr *stack);
void cpuboard_set_flash_unlocked(bool unlocked);
void cpuboard_set_cpu(struct uae_prefs *p);
bool cpuboard_forced_hardreset(void);

bool ppc_interrupt(int new_m68k_ipl);

void cyberstorm_scsi_ram_put(uaecptr addr, uae_u32);
uae_u32 cyberstorm_scsi_ram_get(uaecptr addr);
int REGPARAM3 cyberstorm_scsi_ram_check(uaecptr addr, uae_u32 size) REGPARAM;
uae_u8 *REGPARAM3 cyberstorm_scsi_ram_xlate(uaecptr addr) REGPARAM;

void cyberstorm_mk3_ppc_irq(int id, int level);
void blizzardppc_irq(int id, int level);
void cyberstorm_mk3_ppc_irq_setonly(int id, int level);
void blizzardppc_irq_setonly(int id, int level);
void cpuboard_gvpmaprom(int);

#define BOARD_MEMORY_Z2 1
#define BOARD_MEMORY_Z3 2
#define BOARD_MEMORY_HIGHMEM 3
#define BOARD_MEMORY_BLIZZARD_12xx 4
#define BOARD_MEMORY_BLIZZARD_PPC 5
#define BOARD_MEMORY_25BITMEM 6
#define BOARD_MEMORY_CUSTOM_32 7

#define ISCPUBOARDP(p, type,subtype) (cpuboards[(p)->cpuboard_type].id == type && (type < 0 || (p)->cpuboard_subtype == subtype))
#define ISCPUBOARD(type,subtype) (cpuboards[currprefs.cpuboard_type].id == type && (type < 0 || currprefs.cpuboard_subtype == subtype))

#define BOARD_ACT 1
#define BOARD_ACT_SUB_APOLLO_12xx 0
#define BOARD_ACT_SUB_APOLLO_630 1

#define BOARD_COMMODORE 2
#define BOARD_COMMODORE_SUB_A26x0 0

#define BOARD_DCE 3
#define BOARD_DCE_SUB_SX32PRO 0
#define BOARD_DCE_SUB_TYPHOON2 1

#define BOARD_DKB 4
#define BOARD_DKB_SUB_12x0 0
#define BOARD_DKB_SUB_WILDFIRE 1

#define BOARD_GVP 5
#define BOARD_GVP_SUB_A3001SI 0
#define BOARD_GVP_SUB_A3001SII 1
#define BOARD_GVP_SUB_A530 2
#define BOARD_GVP_SUB_GFORCE030 3
#define BOARD_GVP_SUB_GFORCE040 4
#define BOARD_GVP_SUB_TEKMAGIC 5
#define BOARD_GVP_SUB_A1230SI 6
#define BOARD_GVP_SUB_A1230SII 7
#define BOARD_GVP_SUB_QUIKPAK 8

#define BOARD_KUPKE 6

#define BOARD_MACROSYSTEM 7
#define BOARD_MACROSYSTEM_SUB_WARPENGINE_A4000 0
#define BOARD_MACROSYSTEM_SUB_FALCON040 1
#define BOARD_MACROSYSTEM_SUB_DRACO 2

#define BOARD_MTEC 8
#define BOARD_MTEC_SUB_EMATRIX530 0

#define BOARD_BLIZZARD 9
#define BOARD_BLIZZARD_SUB_1230II 0
#define BOARD_BLIZZARD_SUB_1230III 1
#define BOARD_BLIZZARD_SUB_1230IV 2
#define BOARD_BLIZZARD_SUB_1260 3
#define BOARD_BLIZZARD_SUB_2060 4
#define BOARD_BLIZZARD_SUB_PPC 5

#define BOARD_CYBERSTORM 10
#define BOARD_CYBERSTORM_SUB_MK1 0
#define BOARD_CYBERSTORM_SUB_MK2 1
#define BOARD_CYBERSTORM_SUB_MK3 2
#define BOARD_CYBERSTORM_SUB_PPC 3

#define BOARD_RCS 11
#define BOARD_RCS_SUB_FUSIONFORTY 0

#define BOARD_IVS 12
#define BOARD_IVS_SUB_VECTOR 0

#define BOARD_PPS 13
#define BOARD_PPS_SUB_ZEUS040 0

#define BOARD_CSA 14
#define BOARD_CSA_SUB_MAGNUM40 0
#define BOARD_CSA_SUB_12GAUGE 1

#define BOARD_HARDITAL 15
#define BOARD_HARDITAL_SUB_TQM 0

#define BOARD_HARMS 16
#define BOARD_HARMS_SUB_3KPRO 0

#endif /* UAE_CPUBOARD_H */
