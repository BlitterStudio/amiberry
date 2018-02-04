#ifndef UAE_FDI2RAW_H
#define UAE_FDI2RAW_H

#include "uae/types.h"

typedef struct fdi FDI;

#ifdef __cplusplus
extern "C" {
#endif

extern int fdi2raw_loadtrack (FDI*, uae_u16 *mfmbuf, uae_u16 *tracktiming, int track, int *tracklength, int *indexoffset, int *multirev, int mfm);
extern int fdi2raw_loadrevolution (FDI*, uae_u16 *mfmbuf, uae_u16 *tracktiming, int track, int *tracklength, int mfm);
extern FDI *fdi2raw_header(struct zfile *f);
extern void fdi2raw_header_free (FDI *);
extern int fdi2raw_get_last_track(FDI *);
extern int fdi2raw_get_num_sector (FDI *);
extern int fdi2raw_get_last_head(FDI *);
extern int fdi2raw_get_type (FDI *);
extern int fdi2raw_get_bit_rate (FDI *);
extern int fdi2raw_get_rotation (FDI *);
extern int fdi2raw_get_write_protect (FDI *);

#ifdef __cplusplus
}
#endif

#endif /* UAE_FDI2RAW_H */
