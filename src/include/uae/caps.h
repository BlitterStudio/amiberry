int caps_init (void);
void caps_unloadimage (int drv);
int caps_loadimage (struct zfile *zf, int drv, int *num_tracks);
int caps_loadtrack (uae_u16 *mfmbuf, uae_u16 *tracktiming, int drv, int track, int *tracklength, int *multirev, int *gapoffset, int *nextrev, bool setrev);
int caps_loadrevolution (uae_u16 *mfmbuf, uae_u16 *tracktiming, int drv, int track, int *tracklength, int *nextrev, bool track_access_done);
