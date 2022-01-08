#ifndef UAE_SAMPLER_H
#define UAE_SAMPLER_H

#include "uae/types.h"

extern uae_u8 sampler_getsample (int);
extern int sampler_init (void);
extern void sampler_free (void);
extern void sampler_vsync (void);

#endif /* UAE_SAMPLER_H */
