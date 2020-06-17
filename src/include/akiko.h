#ifndef UAE_AKIKO_H
#define UAE_AKIKO_H

#define AKIKO_BASE 0xb80000
#define AKIKO_BASE_END 0xb80040

extern int akiko_init (void);
extern void akiko_mute (int);
extern bool akiko_ntscmode(void);

#endif /* UAE_AKIKO_H */
