#ifndef UAE_EXECIO_H
#define UAE_EXECIO_H

#define IOERR_OPENFAIL	 -1
#define IOERR_ABORTED	 -2
#define IOERR_NOCMD	 -3
#define IOERR_BADLENGTH	 -4
#define IOERR_BADADDRESS -5
#define IOERR_UNITBUSY	 -6
#define IOERR_SELFTEST	 -7

#define IOERR_NotSpecified   20   /* general catchall			  */
#define IOERR_NoSecHdr	     21   /* couldn't even find a sector	  */
#define IOERR_BadSecPreamble 22   /* sector looked wrong		  */
#define IOERR_BadSecID	     23   /* ditto				  */
#define IOERR_BadHdrSum      24   /* header had incorrect checksum	  */
#define IOERR_BadSecSum      25   /* data had incorrect checksum	  */
#define IOERR_TooFewSecs     26   /* couldn't find enough sectors	  */
#define IOERR_BadSecHdr      27   /* another "sector looked wrong"	  */
#define IOERR_WriteProt      28   /* can't write to a protected disk	  */
#define IOERR_NoDisk	     29   /* no disk in the drive		  */
#define IOERR_SeekError      30   /* couldn't find track 0		  */
#define IOERR_NoMem          31   /* ran out of memory			  */
#define IOERR_BadUnitNum     32   /* asked for a unit > NUMUNITS	  */
#define IOERR_BadDriveType   33   /* not a drive cd.device understands	  */
#define IOERR_DriveInUse     34   /* someone else allocated the drive	  */
#define IOERR_PostReset      35   /* user hit reset; awaiting doom	  */
#define IOERR_BadDataType    36   /* data on disk is wrong type	  */
#define IOERR_InvalidState   37   /* invalid cmd under current conditions */
#define IOERR_BadStatus      45
#define IOERR_Phase	         42   /* illegal or unexpected SCSI phase	  */
#define IOERR_NoBoard        50   /* open failed for non-existant board   */


#define TDERR_DiskChanged 29

#define CMD_INVALID	0
#define CMD_RESET	1
#define CMD_READ	2
#define CMD_WRITE	3
#define CMD_UPDATE	4
#define CMD_CLEAR	5
#define CMD_STOP	6
#define CMD_START	7
#define CMD_FLUSH	8
#define CMD_NONSTD	9

#define IOB_QUICK	0
#define IOF_QUICK	(1<<0)

#define IOSTDREQ_SIZE 48

#define DRIVE_NEWSTYLE 0x4E535459L   /* 'NSTY' */
#define NSCMD_DEVICEQUERY 0x4000

#define TAG_DONE   0
#define TAG_IGNORE 1
#define TAG_MORE   2
#define TAG_SKIP   3
#define TAG_USER   (1 << 31)

#define NSDEVTYPE_UNKNOWN       0
#define NSDEVTYPE_GAMEPORT      1
#define NSDEVTYPE_TIMER         2
#define NSDEVTYPE_KEYBOARD      3
#define NSDEVTYPE_INPUT         4
#define NSDEVTYPE_TRACKDISK     5
#define NSDEVTYPE_CONSOLE       6
#define NSDEVTYPE_SANA2         7
#define NSDEVTYPE_AUDIO         8
#define NSDEVTYPE_CLIPBOARD     9
#define NSDEVTYPE_PRINTER       10
#define NSDEVTYPE_SERIAL        11
#define NSDEVTYPE_PARALLEL      12

#define CMD_MOTOR			 9
#define CMD_SEEK			10
#define CMD_FORMAT			11
#define CMD_REMOVE			12
#define CMD_CHANGENUM		13
#define CMD_CHANGESTATE		14
#define CMD_PROTSTATUS		15
#define CMD_GETDRIVETYPE	18
#define CMD_GETNUMTRACKS	19
#define CMD_ADDCHANGEINT	20
#define CMD_REMCHANGEINT	21
#define CMD_GETGEOMETRY		22
#define CMD_GETDRIVETYPE	18
#define CMD_GETNUMTRACKS	19
#define CMD_ADDCHANGEINT	20
#define CMD_REMCHANGEINT	21
#define CMD_GETGEOMETRY		22
#define CD_EJECT			23
#define TD_READ64			24
#define TD_WRITE64			25
#define TD_SEEK64			26
#define TD_FORMAT64			27
#define HD_SCSICMD			28
#define CD_INFO				32
#define CD_CONFIG			33
#define CD_TOCMSF			34
#define CD_TOCLSN			35
#define CD_READXL			36
#define CD_PLAYTRACK		37
#define CD_PLAYMSF			38
#define CD_PLAYLSN			39
#define CD_PAUSE			40
#define CD_SEARCH			41
#define CD_QCODEMSF			42
#define CD_QCODELSN			43
#define CD_ATTENUATE		44
#define CD_ADDFRAMEINT		45
#define CD_REMFRAMEINT		46

/* New Style Devices (NSD) support */
#define NSCMD_TD_READ64 0xc000
#define NSCMD_TD_WRITE64 0xc001
#define NSCMD_TD_SEEK64 0xc002
#define NSCMD_TD_FORMAT64 0xc003

#endif /* UAE_EXECIO_H */
