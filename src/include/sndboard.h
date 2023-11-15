#ifndef UAE_SNDBOARD_H
#define UAE_SNDBOARD_H

#include "uae/types.h"

bool toccata_init(struct autoconfig_info *aci);
bool prelude_init(struct autoconfig_info *aci);
bool prelude1200_init(struct autoconfig_info *aci);
void update_sndboard_sound(float);
void sndboard_ext_volume(void);

bool uaesndboard_init_z2(struct autoconfig_info *aci);
bool uaesndboard_init_z3(struct autoconfig_info *aci);

bool pmx_init(struct autoconfig_info *aci);

#endif /* UAE_SNDBOARD_H */
