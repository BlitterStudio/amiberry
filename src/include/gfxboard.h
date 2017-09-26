#ifndef UAE_GFXBOARD_H
#define UAE_GFXBOARD_H

extern int gfxboard_get_configtype (struct rtgboardconfig*);
extern const TCHAR *gfxboard_get_name(int);
extern const TCHAR *gfxboard_get_configname(int);

#define GFXBOARD_UAE_Z2 0
#define GFXBOARD_UAE_Z3 1
#define GFXBOARD_HARDWARE 2

#endif /* UAE_GFXBOARD_H */
