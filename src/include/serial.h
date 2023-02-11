 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Serial Line Emulation
  *
  * Copyright 1996, 1997 Stefan Reinauer <stepan@linux.de>
  * Copyright 1997 Christian Schmitt <schmitt@freiburg.linux.de>
  */

#ifndef UAE_SERIAL_H
#define UAE_SERIAL_H

#include "uae/types.h"

extern void serial_init (void);
extern void serial_exit (void);
extern void serial_dtr_off (void);
extern void serial_dtr_on (void);

extern uae_u16 SERDATR (void);
extern void  SERPER (uae_u16 w);
extern void  SERDAT (uae_u16 w);

extern uae_u8 serial_writestatus (uae_u8, uae_u8);
extern uae_u8 serial_readstatus (uae_u8, uae_u8);
extern void serial_uartbreak (int);
extern void serial_rbf_clear (void);
extern uae_u16 serdat;

extern int doreadser, serstat;

extern void serial_flush_buffer (void);

extern void serial_hsynchandler(void);
extern void serial_rethink(void);

extern int uaeser_getdatalength (void);
extern int uaeser_open (void*, void*, int);
extern void uaeser_close (void*);
extern int uaeser_read (void*, uae_u8 *data, uae_u32 len);
extern int uaeser_write (void*, uae_u8 *data, uae_u32 len);
extern int uaeser_query (void*, uae_u16 *status, uae_u32 *pending);
extern int uaeser_setparams (void*, int baud, int rbuffer, int bits, int sbits, int rtscts, int parity, uae_u32 xonxoff);
extern int uaeser_break (void*, int brklen);
extern void uaeser_signal (void*, int source);
extern void uaeser_trigger (void*);
extern void uaeser_clearbuffers (void*);

extern void enet_writeser (uae_u16);
extern int enet_readseravail (void);
extern int enet_readser (uae_u16 *buffer);
extern int enet_open (TCHAR *name);
extern void enet_close (void);

#endif /* UAE_SERIAL_H */
