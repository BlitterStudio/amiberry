/*
 * UAE - The Un*x Amiga Emulator
 *
 * Not a parser, but parallel and serial emulation for Win32
 *
 * Copyright 1997 Mathias Ortmann
 * Copyright 1998-1999 Brian King
 */

#define PRTBUFSIZE 65536

int setbaud (long baud );
void getserstat(int *status);
void setserstat (int mask, int onoff);
int readser (int *buffer);
int readseravail (void);
void writeser(int c);
void writeser_flush(void);
int openser (const TCHAR *sername);
void closeser (void);
void doserout (void);
void closeprinter (void);
void flushprinter (void);
int checkserwrite (int spaceneeded);
void serialuartbreak (int);

void hsyncstuff(void);

void shmem_serial_delete(void);
bool shmem_serial_create(void);
int shmem_serial_state(void);

#define SERIAL_INTERNAL _T("INTERNAL_SERIAL")

#define TIOCM_CAR 1
#define TIOCM_DSR 2
#define TIOCM_RI 4
#define TIOCM_DTR 8
#define TIOCM_RTS 16
#define TIOCM_CTS 32

extern void unload_ghostscript (void);
extern int load_ghostscript (void);

#define MAX_MIDI_PORTS 100
struct midiportinfo
{
	TCHAR *name;
	unsigned int devid;
};
extern struct midiportinfo *midiinportinfo[MAX_MIDI_PORTS];
extern struct midiportinfo *midioutportinfo[MAX_MIDI_PORTS];

#define MAX_SERPAR_PORTS 100
struct serparportinfo
{
    TCHAR *dev;
    TCHAR *cfgname;
    TCHAR *name;
};
extern struct serparportinfo *comports[MAX_SERPAR_PORTS];
extern struct serparportinfo *parports[MAX_SERPAR_PORTS];

extern int enumserialports (void);
extern int enummidiports (void);
extern void sernametodev (TCHAR*);
extern void serdevtoname (TCHAR*);

extern void epson_printchar (uae_u8 c);
extern int epson_init (const TCHAR *printername, int pins);
extern void epson_close (void);

#define PARALLEL_MATRIX_TEXT 1
#define PARALLEL_MATRIX_EPSON 2
#define PARALLEL_MATRIX_EPSON9 2
#define PARALLEL_MATRIX_EPSON24 3
#define PARALLEL_MATRIX_EPSON48 4

void finishjob (void);
