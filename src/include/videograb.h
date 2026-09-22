#pragma once

bool initvideograb(const TCHAR *filename);
void uninitvideograb(void);
bool getvideograb(long **buffer, int *width, int *height);
void pausevideograb(int pause);
uae_s64 getsetpositionvideograb(uae_s64 framepos);
uae_s64 getdurationvideograb(void);
bool isvideograb(void);
bool getpausevideograb(void);
void setvolumevideograb(int volume);
void setchflagsvideograb(int chflags, bool mute);
void isvideograb_status(void);
