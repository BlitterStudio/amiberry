/*
 *  uaeexe.h - launch executable in UAE
 *
 *  (c) 1997 by Samuel Devulder
 */

#ifndef UAE_UAEEXE_H
#define UAE_UAEEXE_H

#include "uae/types.h"

struct uae_xcmd {
    struct uae_xcmd *prev, *next;
    TCHAR *cmd;
};

#define UAEEXE_ORG         0xF0FF90 /* sam: I hope this slot is free */

#define UAEEXE_OK          0
#define UAEEXE_NOTRUNNING  1
#define UAEEXE_NOMEM       2

extern void uaeexe_install (void);
extern int uaeexe (const TCHAR *cmd);

#endif /* UAE_UAEEXE_H */
