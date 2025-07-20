#ifndef UAE_GAYLE_H
#define UAE_GAYLE_H

#include "uae/types.h"

void gayle_add_ide_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc);
bool gayle_ide_init(struct autoconfig_info*);
void gayle_free_units (void);
bool gayle_init_pcmcia(struct autoconfig_info *aci);
bool gayle_init_board_io_pcmcia(struct autoconfig_info *aci);
bool gayle_init_board_common_pcmcia(struct autoconfig_info *aci);
void pcmcia_eject(struct uae_prefs *p);
void pcmcia_reinsert(struct uae_prefs*);
bool pcmcia_disk_reinsert(struct uae_prefs *p, struct uaedev_config_info *uci, bool ejectonly);

extern int gary_toenb; // non-existing memory access = bus error.
extern int gary_timeout; // non-existing memory access = delay

#define PCMCIA_COMMON_START 0x600000
#define PCMCIA_COMMON_SIZE 0x400000
#define PCMCIA_ATTRIBUTE_START 0xa00000
#define PCMCIA_ATTRIBUTE_SIZE 0x80000

void gayle_dataflyer_enable(bool);

bool isideint(void);

#endif /* UAE_GAYLE_H */
