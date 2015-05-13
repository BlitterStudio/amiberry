 /*
  * UAE - The Un*x Amiga Emulator
  *
  * a SCSI device
  *
  * (c) 1995 Bernd Schmidt (hardfile.c)
  * (c) 1999 Patrick Ohly
  * (c) 2001-2005 Toni Wilen
  */

uaecptr scsidev_startup (uaecptr resaddr);
void scsidev_install (void);
void scsidev_reset (void);
void scsidev_start_threads (void);
