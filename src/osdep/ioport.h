#ifndef UAE_IOPORT_H
#define UAE_IOPORT_H

#ifdef WINUAE
int ioport_init (void);
void ioport_free (void);
void ioport_write (int,uae_u8);
uae_u8 ioport_read (int);
#endif

#ifdef PARALLEL_DIRECT
int paraport_init (void);
int paraport_open (TCHAR*);
void paraport_free (void);
#endif

#endif /* UAE_IOPORT_H */
