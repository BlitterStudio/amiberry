#define IOERR_OPENFAIL	 -1
#define IOERR_ABORTED	 -2
#define IOERR_NOCMD	 -3
#define IOERR_BADLENGTH	 -4
#define IOERR_BADADDRESS -5
#define IOERR_UNITBUSY	 -6
#define IOERR_SELFTEST	 -7

#define CMD_INVALID	0
#define CMD_RESET	1
#define CMD_READ	2
#define CMD_WRITE	3
#define CMD_UPDATE	4
#define CMD_CLEAR	5
#define CMD_STOP	6
#define CMD_START	7
#define CMD_FLUSH	8

#define IOSTDREQ_SIZE 48

#define DRIVE_NEWSTYLE 0x4E535459L   /* 'NSTY' */
#define NSCMD_DEVICEQUERY 0x4000

#define     NSDEVTYPE_UNKNOWN       0
#define     NSDEVTYPE_GAMEPORT      1
#define     NSDEVTYPE_TIMER         2
#define     NSDEVTYPE_KEYBOARD      3
#define     NSDEVTYPE_INPUT         4
#define     NSDEVTYPE_TRACKDISK     5
#define     NSDEVTYPE_CONSOLE       6
#define     NSDEVTYPE_SANA2         7
#define     NSDEVTYPE_AUDIO         8
#define     NSDEVTYPE_CLIPBOARD     9
#define     NSDEVTYPE_PRINTER       10
#define     NSDEVTYPE_SERIAL        11
#define     NSDEVTYPE_PARALLEL      12

#define CMD_MOTOR	9
#define CMD_SEEK	10
#define CMD_FORMAT	11
#define CMD_REMOVE	12
#define CMD_CHANGENUM	13
#define CMD_CHANGESTATE	14
#define CMD_PROTSTATUS	15
#define CMD_GETDRIVETYPE 18
#define CMD_GETNUMTRACKS 19
#define CMD_ADDCHANGEINT 20
#define CMD_REMCHANGEINT 21
#define CMD_GETGEOMETRY	22
#define HD_SCSICMD 28

/* Trackdisk64 support */
#define TD_READ64 24
#define TD_WRITE64 25
#define TD_SEEK64 26
#define TD_FORMAT64 27

/* New Style Devices (NSD) support */
#define NSCMD_TD_READ64 0xc000
#define NSCMD_TD_WRITE64 0xc001
#define NSCMD_TD_SEEK64 0xc002
#define NSCMD_TD_FORMAT64 0xc003

