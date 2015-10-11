 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Various stuff missing in some OSes.
  *
  * Copyright 1997 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"

#ifndef HAVE_STRDUP

char *my_strdup (const char *s)
{
    /* The casts to char * are there to shut up the compiler on HPUX */
    char *x = (char*)xmalloc(strlen((char *)s) + 1);
    strcpy(x, (char *)s);
    return x;
}

#endif


void *xmalloc (size_t n)
{
    void *a = malloc (n);
    return a;
}

void *xcalloc (size_t n, size_t size)
{
    void *a = calloc (n, size);
    return a;
}
 
void xfree (void *p)
{
    free (p);
}
