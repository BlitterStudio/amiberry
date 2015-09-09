#ifndef LIBRARIES_XFDMASTER_H
#define LIBRARIES_XFDMASTER_H

/*
**	$VER: xfdmaster.h 39.5 (31.08.2002)
**
**	Copyright © 1994-2002 by Georg Hörmann, Dirk Stöcker
**	All Rights Reserved.
*/

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

/*********************
*                    *
*    Library Base    *
*                    *
*********************/

struct xfdMasterBase {
  struct Library LibNode;
  ULONG			xfdm_SegList;      /* PRIVATE! */
  struct DosLibrary *	xfdm_DosBase;      /* May be used for I/O etc. */
  struct xfdSlave *	xfdm_FirstSlave;   /* List of available slaves */
  struct xfdForeMan *	xfdm_FirstForeMan; /* PRIVATE! */
  ULONG			xfdm_MinBufferSize;/* (V36) Min. BufSize for xfdRecogBuffer() */
  ULONG			xfdm_MinLinkerSize;/* (V36) Min. BufSize for xfdRecogLinker() */
  struct ExecBase *	xfdm_ExecBase;     /* (V38.2) Cached for fast access */
};

#define XFDM_VERSION	39		/* for OpenLibrary() */
#define XFDM_NAME	"xfdmaster.library"

/***************************
*                          *
*    Object Types (V36)    *
*                          *
***************************/

#define XFDOBJ_BUFFERINFO	1	/* xfdBufferInfo structure */
#define XFDOBJ_SEGMENTINFO	2	/* xfdSegmentInfo structure */
#define XFDOBJ_LINKERINFO	3	/* xfdLinkerInfo structure */
#define XFDOBJ_SCANNODE		4	/* (V37) xfdScanNode structure */
#define XFDOBJ_SCANHOOK		5	/* (V37) xfdScanHook structure */
#define XFDOBJ_MAX		5	/* PRIVATE! */

/********************
*                   *
*    Buffer Info    *
*                   *
********************/

struct xfdBufferInfo {
  APTR		   xfdbi_SourceBuffer;	  /* Pointer to source buffer */
  ULONG		   xfdbi_SourceBufLen;	  /* Length of source buffer */
  struct xfdSlave *xfdbi_Slave;		  /* PRIVATE! */
  STRPTR	   xfdbi_PackerName;	  /* Name of recognized packer */
  UWORD		   xfdbi_PackerFlags;	  /* Flags for recognized packer */
  UWORD		   xfdbi_Error;		  /* Error return code */
  APTR		   xfdbi_TargetBuffer;	  /* Pointer to target buffer */
  ULONG		   xfdbi_TargetBufMemType;/* Memtype of target buffer */
  ULONG		   xfdbi_TargetBufLen;	  /* Full length of buffer */
  ULONG		   xfdbi_TargetBufSaveLen;/* Used length of buffer */
  ULONG		   xfdbi_DecrAddress;	  /* Address to load decrunched file */
  ULONG		   xfdbi_JmpAddress;	  /* Address to jump in file */
  APTR		   xfdbi_Special;	  /* Special decrunch info (eg. password) */
  UWORD		   xfdbi_Flags;		  /* (V37) Flags to influence recog/decr */
  UWORD		   xfdbi_Reserved0;	  /* (V38) PRIVATE! */
  ULONG		   xfdbi_MinTargetLen;	  /* (V38) Required length of target buffer */
  ULONG		   xfdbi_FinalTargetLen;  /* (V38) Final length of decrunched file */
  APTR		   xfdbi_UserTargetBuf;	  /* (V38) Target buffer allocated by user */
  ULONG		   xfdbi_UserTargetBufLen;/* (V38) Target buffer length */
  ULONG		   xfdbi_MinSourceLen;	  /* (V39) minimum source length (tested by
  					     master library */
};

#define xfdbi_MaxSpecialLen xfdbi_Error	/* Max. length of special info */

/*********************
*                    *
*    Segment Info    *
*                    *
*********************/

struct xfdSegmentInfo {
  ULONG		   xfdsi_SegList;	/* BPTR to segment list */
  struct xfdSlave *xfdsi_Slave;		/* PRIVATE! */
  STRPTR	   xfdsi_PackerName;	/* Name of recognized packer */
  UWORD		   xfdsi_PackerFlags;	/* Flags for recognized packer */
  UWORD		   xfdsi_Error;		/* Error return code */
  APTR		   xfdsi_Special;	/* Special decrunch info (eg. password) */
  UWORD		   xfdsi_RelMode;	/* (V34) Relocation mode */
  UWORD		   xfdsi_Flags;		/* (V37) Flags to influence recog/decr */
};

#define xfdsi_MaxSpecialLen xfdsi_Error	/* Max. length of special info */

/**************************
*                         *
*    Linker Info (V36)    *
*                         *
**************************/

struct xfdLinkerInfo {
	APTR	xfdli_Buffer;		/* Pointer to buffer */
	ULONG	xfdli_BufLen;		/* Length of buffer */
	STRPTR	xfdli_LinkerName;	/* Name of recognized linker */
	APTR	xfdli_Unlink;		/* PRIVATE! */
	UWORD	xfdli_Reserved;		/* Set to NULL */
	UWORD	xfdli_Error;		/* Error return code */
	ULONG	xfdli_Hunk1;		/* PRIVATE! */
	ULONG	xfdli_Hunk2;		/* PRIVATE! */
	ULONG	xfdli_Amount1;		/* PRIVATE! */
	ULONG	xfdli_Amount2;		/* PRIVATE! */
	APTR	xfdli_Save1;		/* Pointer to first unlinked file */
	APTR	xfdli_Save2;		/* Pointer to second unlinked file */
	ULONG	xfdli_SaveLen1;		/* Length of first unlinked file */
	ULONG	xfdli_SaveLen2;		/* Length of second unlinked file */
};

/************************
*                       *
*    Scan Node (V37)    *
*                       *
************************/

struct xfdScanNode {
  struct xfdScanNode *xfdsn_Next;	/* Pointer to next xfdScanNode or NULL */
  APTR		      xfdsn_Save;	/* Pointer to data */
  ULONG		      xfdsn_SaveLen;	/* Length of data */
  STRPTR	      xfdsn_PackerName;	/* Name of recognized packer */
  UWORD		      xfdsn_PackerFlags;/* Flags for recognized packer */
};

/************************
*                       *
*    Scan Hook (V37)    *
*                       *
************************/

struct xfdScanHook {
  BOOL	(* xfdsh_Entry)();	/* Entrypoint of hook code */
  APTR	   xfdsh_Data;		/* Private data of hook */
  ULONG	   xfdsh_ToDo;		/* Bytes still to scan (READ ONLY) */
  ULONG	   xfdsh_ScanNode;	/* Found data right now (or NULL) (READ ONLY) */
};

/********************
*                   *
*    Error Codes    *
*                   *
********************/

#define XFDERR_OK		0x0000	/* No errors */

#define XFDERR_NOMEMORY		0x0001	/* Error allocating memory */
#define XFDERR_NOSLAVE		0x0002	/* No slave entry in info structure */
#define XFDERR_NOTSUPPORTED	0x0003	/* Slave doesn't support called function */
#define XFDERR_UNKNOWN		0x0004	/* Unknown file */
#define XFDERR_NOSOURCE		0x0005	/* No sourcebuffer/seglist specified */
#define XFDERR_WRONGPASSWORD	0x0006	/* Wrong password for decrunching */
#define XFDERR_BADHUNK		0x0007	/* Bad hunk structure */
#define XFDERR_CORRUPTEDDATA	0x0008	/* Crunched data is corrupted */
#define XFDERR_MISSINGRESOURCE	0x0009	/* (V34) Missing resource (eg. library) */
#define XFDERR_WRONGKEY		0x000a	/* (V35) Wrong 16/32 bit key */
#define XFDERR_BETTERCPU	0x000b	/* (V37) Better CPU required */
#define XFDERR_HOOKBREAK	0x000c	/* (V37) Hook caused break */
#define XFDERR_DOSERROR		0x000d	/* (V37) Dos error */
#define XFDERR_NOTARGET		0x000e	/* (V38) No user target given */
#define XFDERR_TARGETTOOSMALL	0x000f	/* (V38) User target is too small */
#define XFDERR_TARGETNOTSUPPORTED 0x0010 /* (V38) User target not supported */

#define XFDERR_UNDEFINEDHUNK	0x1000	/* (V34) Undefined hunk type */
#define XFDERR_NOHUNKHEADER	0x1001	/* (V34) File is not executable */
#define XFDERR_BADEXTTYPE	0x1002	/* (V34) Bad hunk_ext type */
#define XFDERR_BUFFERTRUNCATED	0x1003	/* (V34) Unexpected end of file */
#define XFDERR_WRONGHUNKAMOUNT	0x1004	/* (V34) Wrong amount of hunks */
#define XFDERR_NOOVERLAYS	0x1005	/* (V36) Overlays not allowed */

#define XFDERR_UNSUPPORTEDHUNK	0x2000	/* (V34) Hunk type not supported */
#define XFDERR_BADRELMODE	0x2001	/* (V34) Unknown XFDREL_#? mode */

/*******************************
*                              *
*    Relocation Modes (V34)    *
*                              *
*******************************/

#define XFDREL_DEFAULT		0x0000	/* Use memory types given by hunk_header */
#define XFDREL_FORCECHIP	0x0001	/* Force all hunks to chip ram */
#define XFDREL_FORCEFAST	0x0002	/* Force all hunks to fast ram */

/*************************************
*                                    *
*    Values for xfd??_PackerFlags    *
*                                    *
*************************************/

/* Bit numbers */
#define XFDPFB_RELOC	0	/* Relocatible file packer */
#define XFDPFB_ADDR	1	/* Absolute address file packer */
#define XFDPFB_DATA	2	/* Data file packer */

#define XFDPFB_PASSWORD	4	/* Packer requires password */
#define XFDPFB_RELMODE	5	/* (V34) Decruncher supports xfdsi_RelMode */
#define XFDPFB_KEY16	6	/* (V35) Packer requires 16 bit key */
#define XFDPFB_KEY32	7	/* (V35) Packer requires 32 bit key */

#define	XFDPFB_RECOGLEN	8	/* (V38) slave recognizes target lengths */
#define	XFDPFB_USERTARGET 9	/* (V38) slave supports user target buffer */

#define XFDPFB_EXTERN	15	/* (V37) PRIVATE */

/* Bit masks */
#define XFDPFF_RELOC	(1<<XFDPFB_RELOC)
#define XFDPFF_ADDR	(1<<XFDPFB_ADDR)
#define XFDPFF_DATA	(1<<XFDPFB_DATA)

#define XFDPFF_PASSWORD	(1<<XFDPFB_PASSWORD)
#define XFDPFF_RELMODE	(1<<XFDPFB_RELMODE)
#define XFDPFF_KEY16	(1<<XFDPFB_KEY16)
#define XFDPFF_KEY32	(1<<XFDPFB_KEY32)

#define XFDPFF_RECOGLEN	(1<<XFDPFB_RECOGLEN)
#define XFDPFF_USERTARGET (1<<XFDPFB_USERTARGET)

#define XFDPFF_EXTERN	(1<<XFDPFB_EXTERN)

/************************************
*                                   *
*    Values for xfd??_Flags (V37)   *
*                                   *
************************************/

/* Bit numbers */
#define XFDFB_RECOGEXTERN	0	/* xfdRecog#?() uses external slaves */
#define	XFDFB_RECOGTARGETLEN	1	/* (V38) xfdRecogBuffer() uses only slaves
					   that recognize target lengths */
#define	XFDFB_RECOGUSERTARGET	2	/* (V38) xfdRecogBuffer() uses only slaves
					   that support user targets */
#define	XFDFB_USERTARGET	3	/* (V38) xfdbi_DecrunchBuffer() decrunchs
					   to given xfdbi_UserTarget */
#define XFDFB_MASTERALLOC	4	/* (V39) master allocated decrunch buffer */ 

/* Bit masks */
#define XFDFF_RECOGEXTERN	(1<<XFDFB_RECOGEXTERN)
#define XFDFF_RECOGTARGETLEN	(1<<XFDFB_RECOGTARGETLEN)
#define XFDFF_RECOGUSERTARGET	(1<<XFDFB_RECOGUSERTARGET)
#define XFDFF_USERTARGET	(1<<XFDFB_USERTARGET)
#define XFDFF_MASTERALLOC	(1<<XFDFB_MASTERALLOC)

/****************************************************
*                                                   *
*    Flags for xfdTestHunkStructureFlags() (V36)    *
*                                                   *
****************************************************/

/* Bit numbers */
#define XFDTHB_NOOVERLAYS	0	/* Abort on hunk_overlay */

/* Bit masks */
#define XFDTHF_NOOVERLAYS	(1<<XFDTHB_NOOVERLAYS)

/****************************************
*                                       *
*    Flags for xfdStripHunks() (V36)    *
*                                       *
****************************************/

/* Bit numbers */
#define XFDSHB_NAME	0	/* Strip hunk_name */
#define XFDSHB_SYMBOL	1	/* Strip hunk_symbol */
#define XFDSHB_DEBUG	2	/* Strip hunk_debug */

/* Bit masks */
#define XFDSHF_NAME	(1<<XFDSHB_NAME)
#define XFDSHF_SYMBOL	(1<<XFDSHB_SYMBOL)
#define XFDSHF_DEBUG	(1<<XFDSHB_DEBUG)

/**************************************
*                                     *
*    Flags for xfdScanData() (V37)    *
*                                     *
**************************************/

/* Bit numbers */
#define XFDSDB_USEEXTERN 0	/* Use external slaves for scanning */
#define XFDSDB_SCANODD	1	/* Scan at odd addresses too */

/* Bit masks */
#define XFDSDF_USEEXTERN (1<<XFDSDB_USEEXTERN)
#define XFDSDF_SCANODD	(1<<XFDSDB_SCANODD)

/****************
*               *
*    Foreman    *
*               *
****************/

struct xfdForeMan {
  ULONG 	   xfdf_Security;	/* moveq #-1,d0 ; rts */
  ULONG 	   xfdf_ID;		/* Set to XFDF_ID */
  UWORD	           xfdf_Version;	/* Set to XFDF_VERSION */
  UWORD 	   xfdf_Reserved;	/* Not used by now, set to NULL */
  ULONG		   xfdf_Next;		/* PRIVATE! */
  ULONG		   xfdf_SegList;	/* PRIVATE! */
  struct xfdSlave *xfdf_FirstSlave;	/* First slave (see below) */
};

#define XFDF_ID		(('X'<<24)|('F'<<16)|('D'<<8)|('F'))
#define XFDF_VERSION	1

/**************
*             *
*    Slave    *
*             *
**************/

struct xfdSlave {
  struct xfdSlave *xfds_Next;		/* Next slave (or NULL) */
  UWORD		   xfds_Version;	/* Set to XFDS_VERSION */
  UWORD 	   xfds_MasterVersion;	/* Minimum XFDM_VERSION required */
  STRPTR	   xfds_PackerName;	/* Name of packer ('\0' terminated) */
  UWORD 	   xfds_PackerFlags;	/* Flags for packer */
  UWORD 	   xfds_MaxSpecialLen;	/* Max. length of special info (eg. password) */
  BOOL		(* xfds_RecogBuffer)();	   /* buffer recognition code (or NULL) */
  BOOL		(* xfds_DecrunchBuffer)(); /* buffer decrunch code (or NULL) */
  BOOL		(* xfds_RecogSegment)();   /* segment recognition code (or NULL) */
  BOOL		(* xfds_DecrunchSegment)();/* segment decrunch code (or NULL) */
  UWORD		   xfds_SlaveID;	/* (V36) Slave ID (only internal slaves) */
  UWORD		   xfds_ReplaceID;	/* (V36) ID of slave to be replaced */
  ULONG		   xfds_MinBufferSize;	/* (V36) Min. BufSize for RecogBufferXYZ() */
};

#define xfds_ScanData xfds_RecogSegment		/* (V37) XFDPFB_DATA: Scan code (or NULL) */
#define xfds_VerifyData xfds_DecrunchSegment	/* (V37) XFDPFB_DATA: Verify code (or NULL) */

#define XFDS_VERSION	2

/*********************************************
*                                            *
*    Additional Recognition Results (V38)    *
*                                            *
*********************************************/

struct xfdRecogResult {
	ULONG	xfdrr_MinTargetLen;	/* Min. required length of target buffer */
	ULONG	xfdrr_FinalTargetLen;	/* Final length of decrunched file */
	ULONG	xfdrr_MinSourceLen;	/* (V39) minimum size of source file */
};

/*********************************
*                                *
*    Internal Slave IDs (V36)    *
*                                *
*********************************/

#define XFDID_BASE	0x8000

#define XFDID_PowerPacker23		(XFDID_BASE+0x0001)
#define XFDID_PowerPacker30		(XFDID_BASE+0x0003)
#define XFDID_PowerPacker30Enc		(XFDID_BASE+0x0005)
#define XFDID_PowerPacker30Ovl		(XFDID_BASE+0x0007)
#define XFDID_PowerPacker40		(XFDID_BASE+0x0009)
#define XFDID_PowerPacker40Lib		(XFDID_BASE+0x000a)
#define XFDID_PowerPacker40Enc		(XFDID_BASE+0x000b)
#define XFDID_PowerPacker40LibEnc	(XFDID_BASE+0x000c)
#define XFDID_PowerPacker40Ovl		(XFDID_BASE+0x000d)
#define XFDID_PowerPacker40LibOvl	(XFDID_BASE+0x000e)
#define XFDID_PowerPackerData		(XFDID_BASE+0x000f)
#define XFDID_PowerPackerDataEnc	(XFDID_BASE+0x0010)
#define XFDID_ByteKiller13		(XFDID_BASE+0x0011)
#define XFDID_ByteKiller20		(XFDID_BASE+0x0012)
#define XFDID_ByteKiller30		(XFDID_BASE+0x0013)
#define XFDID_ByteKillerPro10		(XFDID_BASE+0x0014)
#define XFDID_ByteKillerPro10Pro	(XFDID_BASE+0x0015)
#define XFDID_DragPack10		(XFDID_BASE+0x0016)
#define XFDID_TNMCruncher11		(XFDID_BASE+0x0017)
#define XFDID_HQCCruncher20		(XFDID_BASE+0x0018)
#define XFDID_RSICruncher14		(XFDID_BASE+0x0019)
#define XFDID_ANCCruncher		(XFDID_BASE+0x001a)
#define XFDID_ReloKit10			(XFDID_BASE+0x001b)
#define XFDID_HighPressureCruncher	(XFDID_BASE+0x001c)
#define XFDID_STPackedSong		(XFDID_BASE+0x001d)
#define XFDID_TSKCruncher		(XFDID_BASE+0x001e)
#define XFDID_LightPack15		(XFDID_BASE+0x001f)
#define XFDID_CrunchMaster10		(XFDID_BASE+0x0020)
#define XFDID_HQCCompressor100		(XFDID_BASE+0x0021)
#define XFDID_FlashSpeed10		(XFDID_BASE+0x0022)
#define XFDID_CrunchManiaData		(XFDID_BASE+0x0023)
#define XFDID_CrunchManiaDataEnc	(XFDID_BASE+0x0024)
#define XFDID_CrunchManiaLib		(XFDID_BASE+0x0025)
#define XFDID_CrunchManiaNormal		(XFDID_BASE+0x0026)
#define XFDID_CrunchManiaSimple		(XFDID_BASE+0x0027)
#define XFDID_CrunchManiaAddr		(XFDID_BASE+0x0028)
#define XFDID_DefJamCruncher32		(XFDID_BASE+0x0029)
#define XFDID_DefJamCruncher32Pro	(XFDID_BASE+0x002a)
#define XFDID_TetraPack102		(XFDID_BASE+0x002b)
#define XFDID_TetraPack11		(XFDID_BASE+0x002c)
#define XFDID_TetraPack21		(XFDID_BASE+0x002d)
#define XFDID_TetraPack21Pro		(XFDID_BASE+0x002e)
#define XFDID_TetraPack22		(XFDID_BASE+0x002f)
#define XFDID_TetraPack22Pro		(XFDID_BASE+0x0030)
#define XFDID_DoubleAction10		(XFDID_BASE+0x0031)
#define XFDID_DragPack252Data		(XFDID_BASE+0x0032)
#define XFDID_DragPack252		(XFDID_BASE+0x0033)
#define XFDID_FCG10			(XFDID_BASE+0x0034)
#define XFDID_Freeway07			(XFDID_BASE+0x0035)
#define XFDID_IAMPacker10ATM5Data	(XFDID_BASE+0x0036)
#define XFDID_IAMPacker10ATM5		(XFDID_BASE+0x0037)
#define XFDID_IAMPacker10ICEData	(XFDID_BASE+0x0038)
#define XFDID_IAMPacker10ICE		(XFDID_BASE+0x0039)
#define XFDID_Imploder			(XFDID_BASE+0x003a)
#define XFDID_ImploderLib		(XFDID_BASE+0x003b)
#define XFDID_ImploderOvl		(XFDID_BASE+0x003c)
#define XFDID_FileImploder		(XFDID_BASE+0x003d)
#define XFDID_MasterCruncher30Addr	(XFDID_BASE+0x003f)
#define XFDID_MasterCruncher30		(XFDID_BASE+0x0040)
#define XFDID_MaxPacker12		(XFDID_BASE+0x0041)
#define XFDID_PackIt10Data		(XFDID_BASE+0x0042)
#define XFDID_PackIt10			(XFDID_BASE+0x0043)
#define XFDID_PMCNormal			(XFDID_BASE+0x0044)
#define XFDID_PMCSample			(XFDID_BASE+0x0045)
#define XFDID_XPKPacked			(XFDID_BASE+0x0046)
#define XFDID_XPKCrypted		(XFDID_BASE+0x0047)
#define XFDID_TimeCruncher17		(XFDID_BASE+0x0048)
#define XFDID_TFACruncher154		(XFDID_BASE+0x0049)
#define XFDID_TurtleSmasher13		(XFDID_BASE+0x004a)
#define XFDID_MegaCruncher10		(XFDID_BASE+0x004b)
#define XFDID_MegaCruncher12		(XFDID_BASE+0x004c)
#define XFDID_ProPack			(XFDID_BASE+0x004d)
#define XFDID_ProPackData		(XFDID_BASE+0x004e)
#define XFDID_ProPackDataKey		(XFDID_BASE+0x004f)
#define XFDID_STCruncher10		(XFDID_BASE+0x0050)
#define XFDID_STCruncher10Data		(XFDID_BASE+0x0051)
#define XFDID_SpikeCruncher		(XFDID_BASE+0x0052)
#define XFDID_SyncroPacker46		(XFDID_BASE+0x0053)
#define XFDID_SyncroPacker46Pro		(XFDID_BASE+0x0054)
#define XFDID_TitanicsCruncher11	(XFDID_BASE+0x0055)
#define XFDID_TitanicsCruncher12	(XFDID_BASE+0x0056)
#define XFDID_TryItCruncher101		(XFDID_BASE+0x0057)
#define XFDID_TurboSqueezer61		(XFDID_BASE+0x0058)
#define XFDID_TurboSqueezer80		(XFDID_BASE+0x0059)
#define XFDID_TurtleSmasher200		(XFDID_BASE+0x005a)
#define XFDID_TurtleSmasher200Data	(XFDID_BASE+0x005b)
#define XFDID_StoneCracker270		(XFDID_BASE+0x005c)
#define XFDID_StoneCracker270Pro	(XFDID_BASE+0x005d)
#define XFDID_StoneCracker292		(XFDID_BASE+0x005e)
#define XFDID_StoneCracker299		(XFDID_BASE+0x005f)
#define XFDID_StoneCracker299d		(XFDID_BASE+0x0060)
#define XFDID_StoneCracker300		(XFDID_BASE+0x0061)
#define XFDID_StoneCracker300Data	(XFDID_BASE+0x0062)
#define XFDID_StoneCracker310		(XFDID_BASE+0x0063)
#define XFDID_StoneCracker310Data	(XFDID_BASE+0x0064)
#define XFDID_StoneCracker311		(XFDID_BASE+0x0065)
#define XFDID_StoneCracker400		(XFDID_BASE+0x0066)
#define XFDID_StoneCracker400Data	(XFDID_BASE+0x0067)
#define XFDID_StoneCracker401		(XFDID_BASE+0x0068)
#define XFDID_StoneCracker401Data	(XFDID_BASE+0x0069)
#define XFDID_StoneCracker401Addr	(XFDID_BASE+0x006a)
#define XFDID_StoneCracker401BetaAddr	(XFDID_BASE+0x006b)
#define XFDID_StoneCracker403Data	(XFDID_BASE+0x006c)
#define XFDID_StoneCracker404		(XFDID_BASE+0x006d)
#define XFDID_StoneCracker404Data	(XFDID_BASE+0x006e)
#define XFDID_StoneCracker404Addr	(XFDID_BASE+0x006f)
#define XFDID_ChryseisCruncher09	(XFDID_BASE+0x0070)
#define XFDID_QuickPowerPacker10	(XFDID_BASE+0x0071)
#define XFDID_GNUPacker12		(XFDID_BASE+0x0072)
#define XFDID_GNUPacker12Seg		(XFDID_BASE+0x0073)
#define XFDID_GNUPacker12Data		(XFDID_BASE+0x0074)
#define XFDID_TrashEliminator10		(XFDID_BASE+0x0075)
#define XFDID_MasterCruncher30Data	(XFDID_BASE+0x0076)
#define XFDID_SuperCruncher27		(XFDID_BASE+0x0077)
#define XFDID_UltimatePacker11		(XFDID_BASE+0x0078)
#define XFDID_ProPackOld		(XFDID_BASE+0x0079)
#define XFDID_SACFPQCruncher		(XFDID_BASE+0x007a) /* disabled */
#define XFDID_PowerPackerPatch10	(XFDID_BASE+0x007b)
#define XFDID_CFP135			(XFDID_BASE+0x007c)
#define XFDID_BOND			(XFDID_BASE+0x007d)
#define XFDID_PowerPackerLoadSeg	(XFDID_BASE+0x007e)
#define XFDID_StoneCracker299b		(XFDID_BASE+0x007f)
#define XFDID_CrunchyDat10		(XFDID_BASE+0x0080)
#define XFDID_PowerPacker20		(XFDID_BASE+0x0081)
#define XFDID_StoneCracker403		(XFDID_BASE+0x0082)
#define XFDID_PKProtector200		(XFDID_BASE+0x0083)
#define XFDID_PPbk			(XFDID_BASE+0x0084)
#define XFDID_StoneCracker292Data	(XFDID_BASE+0x0085)
#define XFDID_MegaCruncherObj		(XFDID_BASE+0x0086)
#define XFDID_DeluxeCruncher1		(XFDID_BASE+0x0087)
#define XFDID_DeluxeCruncher3		(XFDID_BASE+0x0088)
#define XFDID_ByteKiller97		(XFDID_BASE+0x0089)
#define XFDID_TurboSqueezer51		(XFDID_BASE+0x008A)
#define XFDID_SubPacker10		(XFDID_BASE+0x008B)
#define XFDID_StoneCracker404Lib	(XFDID_BASE+0x008C)
#define XFDID_ISC_Pass1			(XFDID_BASE+0x008D)
#define XFDID_ISC_Pass2			(XFDID_BASE+0x008E)
#define XFDID_ISC_Pass3			(XFDID_BASE+0x008F)
#define XFDID_PCompressFALH		(XFDID_BASE+0x0090)
#define XFDID_PCompressHILH		(XFDID_BASE+0x0091)
#define XFDID_SMF			(XFDID_BASE+0x0092)
#define XFDID_DefJamCruncher32T		(XFDID_BASE+0x0093)

#endif /* LIBRARIES_XFDMASTER_H */
