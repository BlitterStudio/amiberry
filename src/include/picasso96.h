/*
 * UAE - The U*nix Amiga Emulator
 *
 * Picasso96 Support Module Header
 *
 * Copyright 1997 Brian King <Brian_King@Mitel.com, Brian_King@Cloanto.com>
 */
#ifndef __PICASSO96_H__
#define __PICASSO96_H__

#include "rtgmodes.h"

struct ScreenResolution
{
    uae_u32 width;  /* in pixels */
    uae_u32 height; /* in pixels */
};

#define MAX_PICASSO_MODES 100
#define MAX_REFRESH_RATES 10
struct PicassoResolution
{
    struct ScreenResolution res;
    int depth;   /* depth in bytes-per-pixel */
    int residx;
    int refresh[MAX_REFRESH_RATES]; /* refresh-rates in Hz */
    char name[25];
    /* Bit mask of RGBFF_xxx values.  */
    uae_u32 colormodes;
};
extern struct PicassoResolution *DisplayModes;

typedef struct _RECT
{
  uae_s32 left;
  uae_s32 top;
  uae_s32 right;
  uae_s32 bottom;
} RECT;

#define MAX_DISPLAYS 4
struct MultiDisplay {
    int primary, disabled, gdi;
    char *name;
    char *name2;
    struct PicassoResolution *DisplayModes;
    RECT rect;
};
extern struct MultiDisplay Displays[MAX_DISPLAYS];

extern int GetSurfacePixelFormat(void);
extern void picasso_InitResolutions (void);

#ifdef PICASSO96

#define NOSIGNAL 0xFFFFFFFF

/************************************************************************/
/* Types for BoardType Identification
 */
typedef enum {
	BT_NoBoard,
	BT_oMniBus,
	BT_Graffity,
	BT_CyberVision,
	BT_Domino,
	BT_Merlin,
	BT_PicassoII,
	BT_Piccolo,
	BT_RetinaBLT,
	BT_Spectrum,
	BT_PicassoIV,
	BT_PiccoloSD64,
	BT_A2410,
	BT_Pixel64,
	BT_uaegfx,
	BT_CVision3D,
	BT_Altais,
	BT_Prototype1,
	BT_Prototype2,
	BT_Prototype3,
	BT_Prototype4,
	BT_Prototype5,
	BT_MaxBoardTypes
} BTYPE;

/************************************************************************/
/* Types for PaletteChipType Identification
 */
typedef enum {
	PCT_Unknown,
	PCT_S11483,			// Sierra S11483: HiColor 15 bit, oMniBus, Domino
	PCT_S15025,			// Sierra S15025: TrueColor 32 bit, oMniBus
	PCT_CirrusGD542x,		// Cirrus GD542x internal: TrueColor 24 bit
	PCT_Domino,			// is in fact a Sierra S11483
	PCT_BT482,			// BrookTree BT482: TrueColor 32 bit, Merlin
	PCT_Music,			// Music MU9C4910: TrueColor 24 bit, oMniBus
	PCT_ICS5300,			// ICS 5300: ...., Retina BLT Z3
	PCT_CirrusGD5446,		// Cirrus GD5446 internal: TrueColor 24 bit
	PCT_CirrusGD5434,		// Cirrus GD5434 internal: TrueColor 32 bit
	PCT_S3Trio64,			// S3 Trio64 internal: TrueColor 32 bit
	PCT_A2410_xxx,			// A2410 DAC, *type unknown*
	PCT_S3ViRGE,			// S3 ViRGE internal: TrueColor 32 bit
	PCT_MaxPaletteChipTypes
} PCTYPE;

/************************************************************************/
/* Types for GraphicsControllerType Identification
 */
typedef enum {
	GCT_Unknown,
	GCT_ET4000,
	GCT_ETW32,	
	GCT_CirrusGD542x,
	GCT_NCR77C32BLT,	
	GCT_CirrusGD5446,
	GCT_CirrusGD5434,
	GCT_S3Trio64,
	GCT_TI34010,
	GCT_S3ViRGE,
	GCT_MaxGraphicsControllerTypes
} GCTYPE;

#define JAM1 0
#define JAM2 1
#define COMP 2
#define INVERS 4

/************************************************************************/

enum {
    PLANAR,
    CHUNKY,
    HICOLOR,
    TRUECOLOR,
    TRUEALPHA,
    MAXMODES
};

/************************************************************************/
struct MyCLUTEntry {
    uae_u8 Red;
    uae_u8 Green;
    uae_u8 Blue;
    uae_u8 Pad;
};

struct ColorIndexMapping {
    uae_u32 ColorMask;
    uae_u32 Colors[256];
};

struct CLUTEntry {
    uae_u8 Red;
    uae_u8 Green;
    uae_u8 Blue;
};

#define PSSO_BitMap_BytesPerRow 0
#define PSSO_BitMap_Rows 2
#define PSSO_BitMap_Flags 4
#define PSSO_BitMap_Depth 5
#define PSSO_BitMap_pad 6
#define PSSO_BitMap_Planes 8
#define PSSO_BitMap_sizeof 40

struct BitMap
{
    uae_u16 BytesPerRow;
    uae_u16 Rows;
    uae_u8 Flags;
    uae_u8 Depth;
    uae_u16 pad;
    uae_u8 *Planes[8];
};

/************************************************************************/

#define SETTINGSNAMEMAXCHARS		30
#define BOARDNAMEMAXCHARS		30

struct Settings {
    uae_u32					BoardType;
    /* a value discribing assignment to nth board local to boardtype
     * to be used for reassignment when boards are added or removed.  */
    uae_u16					LocalOrdering;
    uae_s16					LastSelected;
    char					NameField[SETTINGSNAMEMAXCHARS];
    /* neu! */
    char					*BoardName;
};

#define MAXRESOLUTIONNAMELENGTH 22

/********************************
 * only used within rtg.library *
 ********************************/

#define PSSO_LibResolution_P96ID 14
#define PSSO_LibResolution_Name 20
#define PSSO_LibResolution_DisplayID 42 /* Name + MAXRESOLUTIONNAMELENGTH */
#define PSSO_LibResolution_Width 46
#define PSSO_LibResolution_Height 48
#define PSSO_LibResolution_Flags 50
#define PSSO_LibResolution_Modes 52
#define PSSO_LibResolution_BoardInfo (52 + MAXMODES*4)
#define PSSO_LibResolution_sizeof (60 + MAXMODES*4)

struct LibResolution {
    char P96ID[6];
    char Name[MAXRESOLUTIONNAMELENGTH];
    uae_u32 DisplayID;
    uae_u16 Width;
    uae_u16 Height;
    uae_u16 Flags;
    uaecptr Modes[MAXMODES];
    uaecptr BoardInfo;
};

#define P96B_FAMILY	0			/* obsolete (Resolution is an entire family) */
#define P96B_PUBLIC	1			/* Resolution should be added to the public */
#define P96B_MONITOOL	2

#define P96F_FAMILY	(1<<P96B_FAMILY)	/* obsolete */
#define P96F_PUBLIC	(1<<P96B_PUBLIC)
#define P96F_MONITOOL	(1<<P96B_MONITOOL)

#define PSSO_ModeInfo_OpenCount 14
#define PSSO_ModeInfo_Active 16
#define PSSO_ModeInfo_Width 18
#define PSSO_ModeInfo_Height 20
#define PSSO_ModeInfo_Depth 22
#define PSSO_ModeInfo_Flags 23
#define PSSO_ModeInfo_HorTotal 24
#define PSSO_ModeInfo_HorBlankSize 26
#define PSSO_ModeInfo_HorSyncStart 28
#define PSSO_ModeInfo_HorSyncSize 30
#define PSSO_ModeInfo_HorSyncSkew 32
#define PSSO_ModeInfo_HorEnableSkew 33
#define PSSO_ModeInfo_VerTotal 34
#define PSSO_ModeInfo_VerBlankSize 36
#define PSSO_ModeInfo_VerSyncStart 38
#define PSSO_ModeInfo_VerSyncSize 40
#define PSSO_ModeInfo_first_union 42
#define PSSO_ModeInfo_second_union 43
#define PSSO_ModeInfo_PixelClock 44
#define PSSO_ModeInfo_sizeof 48

#define PSSO_RenderInfo_Memory 0
#define PSSO_RenderInfo_BytesPerRow 4
#define PSSO_RenderInfo_pad 6
#define PSSO_RenderInfo_RGBFormat 8
#define PSSO_RenderInfo_sizeof 12

struct RenderInfo {
    uae_u8 *Memory;
    uae_s16 BytesPerRow;
    uae_s16 pad;
    RGBFTYPE RGBFormat;
    uaecptr AMemory;
};

#define PSSO_Pattern_Memory 0
#define PSSO_Pattern_XOffset 4
#define PSSO_Pattern_YOffset 6
#define PSSO_Pattern_FgPen 8
#define PSSO_Pattern_BgPen 12
#define PSSO_Pattern_Size 16
#define PSSO_Pattern_DrawMode 17
#define PSSO_Pattern_sizeof 18
struct Pattern {
    char *Memory;
    uae_u16 XOffset, YOffset;
    uae_u32 FgPen, BgPen;
    uae_u8 Size;					/* Width: 16, Height: (1<<pat_Size) */
    uae_u8 DrawMode;
};

#define PSSO_Template_Memory 0
#define PSSO_Template_BytesPerRow 4
#define PSSO_Template_XOffset 6
#define PSSO_Template_DrawMode 7
#define PSSO_Template_FgPen 8
#define PSSO_Template_BgPen 12
#define PSSO_Template_sizeof 16

struct Template {
    char *Memory;
    uae_s16 BytesPerRow;
    uae_u8 XOffset;
    uae_u8 DrawMode;
    uae_u32 FgPen;
    uae_u32 BgPen;
};

#define PSSO_Line_X 0
#define PSSO_Line_Y 2
#define PSSO_Line_Length 4
#define PSSO_Line_dX 6
#define PSSO_Line_dY 8
#define PSSO_Line_sDelta 10
#define PSSO_Line_lDelta 12
#define PSSO_Line_twoSDminusLD 14
#define PSSO_Line_LinePtrn 16
#define PSSO_Line_PatternShift 18
#define PSSO_Line_FgPen 20
#define PSSO_Line_BgPen 24
#define PSSO_Line_Horizontal 28
#define PSSO_Line_DrawMode 30
#define PSSO_Line_pad 31
#define PSSO_Line_Xorigin 32
#define PSSO_Line_Yorigin 34

struct Line {
	uae_u16			X, Y;
	uae_u16			Length;
	uae_s16			dX, dY;
	uae_s16			sDelta, lDelta, twoSDminusLD;
	uae_u16			LinePtrn;
	uae_u16			PatternShift;
	uae_u32			FgPen, BgPen;
	uae_u16			Horizontal;
	uae_u8			DrawMode;
	uae_s8			pad;
	uae_u16			Xorigin, Yorigin;
};

#define PSSO_BitMapExtra_BoardNode	  0
#define PSSO_BitMapExtra_HashChain	  8 /* BoardNode is 8-bytes */
#define PSSO_BitMapExtra_Match		 12
#define PSSO_BitMapExtra_BitMap		 16
#define PSSO_BitMapExtra_BoardInfo	 20
#define PSSO_BitMapExtra_MemChunk	 24
#define PSSO_BitMapExtra_RenderInfo	 28
#define PSSO_BitMapExtra_Width		 40 /* RenderInfo is 12-bytes */
#define PSSO_BitMapExtra_Height		 42
#define PSSO_BitMapExtra_Flags		 44
#define PSSO_BitMapExtra_BaseLevel	 46
#define PSSO_BitMapExtra_CurrentLevel	 48
#define PSSO_BitMapExtra_CompanionMaster 50
#define PSSO_BitMapExtra_Last		 54

#define PSSO_BoardInfo_RegisterBase		   0
#define PSSO_BoardInfo_MemoryBase		   PSSO_BoardInfo_RegisterBase + 4
#define PSSO_BoardInfo_MemoryIOBase		   PSSO_BoardInfo_MemoryBase + 4
#define PSSO_BoardInfo_MemorySize		   PSSO_BoardInfo_MemoryIOBase + 4
#define PSSO_BoardInfo_BoardName		   PSSO_BoardInfo_MemorySize + 4
#define PSSO_BoardInfo_VBIName			   PSSO_BoardInfo_BoardName + 4
#define PSSO_BoardInfo_CardBase			   PSSO_BoardInfo_VBIName + 32
#define PSSO_BoardInfo_ChipBase			   PSSO_BoardInfo_CardBase + 4
#define PSSO_BoardInfo_ExecBase			   PSSO_BoardInfo_ChipBase + 4
#define PSSO_BoardInfo_UtilBase			   PSSO_BoardInfo_ExecBase + 4
#define PSSO_BoardInfo_HardInterrupt		   PSSO_BoardInfo_UtilBase + 4
#define PSSO_BoardInfo_SoftInterrupt		   PSSO_BoardInfo_HardInterrupt + 22 /* The HardInterrupt is 22-bytes */
#define PSSO_BoardInfo_BoardLock		   PSSO_BoardInfo_SoftInterrupt + 22 /* The SoftInterrupt is 22-bytes */
#define PSSO_BoardInfo_ResolutionsList		   PSSO_BoardInfo_BoardLock + 46 /* On the BoardLock, we were having some fun... */
#define PSSO_BoardInfo_BoardType		   PSSO_BoardInfo_ResolutionsList + 12 /* The ResolutionsList is 12-bytes */
#define PSSO_BoardInfo_PaletteChipType		   PSSO_BoardInfo_BoardType + 4
#define PSSO_BoardInfo_GraphicsControllerType	   PSSO_BoardInfo_PaletteChipType + 4
#define PSSO_BoardInfo_MoniSwitch		   PSSO_BoardInfo_GraphicsControllerType + 4
#define PSSO_BoardInfo_BitsPerCannon		   PSSO_BoardInfo_MoniSwitch + 2
#define PSSO_BoardInfo_Flags			   PSSO_BoardInfo_BitsPerCannon + 2
#define PSSO_BoardInfo_SoftSpriteFlags		   PSSO_BoardInfo_Flags + 4
#define PSSO_BoardInfo_ChipFlags		   PSSO_BoardInfo_SoftSpriteFlags + 2
#define PSSO_BoardInfo_CardFlags		   PSSO_BoardInfo_ChipFlags + 2
#define PSSO_BoardInfo_BoardNum			   PSSO_BoardInfo_CardFlags + 4
#define PSSO_BoardInfo_RGBFormats		   PSSO_BoardInfo_BoardNum + 2
#define PSSO_BoardInfo_MaxHorValue		   PSSO_BoardInfo_RGBFormats + 2
#define PSSO_BoardInfo_MaxVerValue		   PSSO_BoardInfo_MaxHorValue + MAXMODES*2
#define PSSO_BoardInfo_MaxHorResolution		   PSSO_BoardInfo_MaxVerValue + MAXMODES*2
#define PSSO_BoardInfo_MaxVerResolution		   PSSO_BoardInfo_MaxHorResolution + MAXMODES*2
#define PSSO_BoardInfo_MaxMemorySize		   PSSO_BoardInfo_MaxVerResolution + MAXMODES*2
#define PSSO_BoardInfo_MaxChunkSize		   PSSO_BoardInfo_MaxMemorySize + 4
#define PSSO_BoardInfo_MemoryClock		   PSSO_BoardInfo_MaxChunkSize + 4
#define PSSO_BoardInfo_PixelClockCount		   PSSO_BoardInfo_MemoryClock + 4

#define PSSO_BoardInfo_AllocCardMem		   PSSO_BoardInfo_PixelClockCount + MAXMODES*4
#define PSSO_BoardInfo_FreeCardMem		    PSSO_BoardInfo_AllocCardMem + 4

#define PSSO_BoardInfo_SetSwitch		    PSSO_BoardInfo_FreeCardMem + 4

#define PSSO_BoardInfo_SetColorArray		    PSSO_BoardInfo_SetSwitch + 4

#define PSSO_BoardInfo_SetDAC			    PSSO_BoardInfo_SetColorArray + 4
#define PSSO_BoardInfo_SetGC			    PSSO_BoardInfo_SetDAC + 4
#define PSSO_BoardInfo_SetPanning		    PSSO_BoardInfo_SetGC + 4
#define PSSO_BoardInfo_CalculateBytesPerRow	    PSSO_BoardInfo_SetPanning + 4
#define PSSO_BoardInfo_CalculateMemory		    PSSO_BoardInfo_CalculateBytesPerRow + 4
#define PSSO_BoardInfo_GetCompatibleFormats	    PSSO_BoardInfo_CalculateMemory + 4
#define PSSO_BoardInfo_SetDisplay		    PSSO_BoardInfo_GetCompatibleFormats + 4

#define PSSO_BoardInfo_ResolvePixelClock	    PSSO_BoardInfo_SetDisplay + 4
#define PSSO_BoardInfo_GetPixelClock		    PSSO_BoardInfo_ResolvePixelClock + 4
#define PSSO_BoardInfo_SetClock			    PSSO_BoardInfo_GetPixelClock + 4

#define PSSO_BoardInfo_SetMemoryMode		    PSSO_BoardInfo_SetClock + 4
#define PSSO_BoardInfo_SetWriteMask		    PSSO_BoardInfo_SetMemoryMode + 4
#define PSSO_BoardInfo_SetClearMask		    PSSO_BoardInfo_SetWriteMask + 4
#define PSSO_BoardInfo_SetReadPlane		    PSSO_BoardInfo_SetClearMask + 4

#define PSSO_BoardInfo_WaitVerticalSync		    PSSO_BoardInfo_SetReadPlane + 4
#define PSSO_BoardInfo_SetInterrupt		    PSSO_BoardInfo_WaitVerticalSync + 4

#define PSSO_BoardInfo_WaitBlitter		    PSSO_BoardInfo_SetInterrupt + 4

#define PSSO_BoardInfo_ScrollPlanar		    PSSO_BoardInfo_WaitBlitter + 4
#define PSSO_BoardInfo_ScrollPlanarDefault	    PSSO_BoardInfo_ScrollPlanar + 4
#define PSSO_BoardInfo_UpdatePlanar		    PSSO_BoardInfo_ScrollPlanarDefault + 4
#define PSSO_BoardInfo_UpdatePlanarDefault	    PSSO_BoardInfo_UpdatePlanar + 4
#define PSSO_BoardInfo_BlitPlanar2Chunky	    PSSO_BoardInfo_UpdatePlanarDefault + 4
#define PSSO_BoardInfo_BlitPlanar2ChunkyDefault	    PSSO_BoardInfo_BlitPlanar2Chunky + 4

#define PSSO_BoardInfo_FillRect			    PSSO_BoardInfo_BlitPlanar2ChunkyDefault + 4
#define PSSO_BoardInfo_FillRectDefault		    PSSO_BoardInfo_FillRect + 4
#define PSSO_BoardInfo_InvertRect		    PSSO_BoardInfo_FillRectDefault + 4
#define PSSO_BoardInfo_InvertRectDefault	    PSSO_BoardInfo_InvertRect + 4
#define PSSO_BoardInfo_BlitRect			    PSSO_BoardInfo_InvertRectDefault + 4
#define PSSO_BoardInfo_BlitRectDefault		    PSSO_BoardInfo_BlitRect + 4
#define PSSO_BoardInfo_BlitTemplate		    PSSO_BoardInfo_BlitRectDefault + 4
#define PSSO_BoardInfo_BlitTemplateDefault	    PSSO_BoardInfo_BlitTemplate + 4
#define PSSO_BoardInfo_BlitPattern		    PSSO_BoardInfo_BlitTemplateDefault + 4
#define PSSO_BoardInfo_BlitPatternDefault	    PSSO_BoardInfo_BlitPattern + 4
#define PSSO_BoardInfo_DrawLine			    PSSO_BoardInfo_BlitPatternDefault + 4
#define PSSO_BoardInfo_DrawLineDefault		    PSSO_BoardInfo_DrawLine + 4
#define PSSO_BoardInfo_BlitRectNoMaskComplete	    PSSO_BoardInfo_DrawLineDefault + 4
#define PSSO_BoardInfo_BlitRectNoMaskCompleteDefault PSSO_BoardInfo_BlitRectNoMaskComplete + 4
#define PSSO_BoardInfo_BlitPlanar2Direct	    PSSO_BoardInfo_BlitRectNoMaskCompleteDefault + 4
#define PSSO_BoardInfo_BlitPlanar2DirectDefault	    PSSO_BoardInfo_BlitPlanar2Direct + 4

#define PSSO_BoardInfo_Reserved0		    PSSO_BoardInfo_BlitPlanar2DirectDefault + 4
#define PSSO_BoardInfo_Reserved0Default		    PSSO_BoardInfo_Reserved0 + 4
#define PSSO_BoardInfo_Reserved1		    PSSO_BoardInfo_Reserved0Default + 4
#define PSSO_BoardInfo_Reserved1Default		    PSSO_BoardInfo_Reserved1 + 4
#define PSSO_BoardInfo_Reserved2		    PSSO_BoardInfo_Reserved1Default + 4
#define PSSO_BoardInfo_Reserved2Default		    PSSO_BoardInfo_Reserved2 + 4
#define PSSO_BoardInfo_Reserved3		    PSSO_BoardInfo_Reserved2Default + 4
#define PSSO_BoardInfo_Reserved3Default		    PSSO_BoardInfo_Reserved3 + 4
#define PSSO_BoardInfo_Reserved4		    PSSO_BoardInfo_Reserved3Default + 4
#define PSSO_BoardInfo_Reserved4Default		    PSSO_BoardInfo_Reserved4 + 4
#define PSSO_BoardInfo_Reserved5		    PSSO_BoardInfo_Reserved4Default + 4
#define PSSO_BoardInfo_Reserved5Default		    PSSO_BoardInfo_Reserved5 + 4

#define PSSO_BoardInfo_SetDPMSLevel		    PSSO_BoardInfo_Reserved5Default + 4
#define PSSO_BoardInfo_ResetChip		    PSSO_BoardInfo_SetDPMSLevel + 4

#define PSSO_BoardInfo_GetFeatureAttrs		    PSSO_BoardInfo_ResetChip + 4

#define PSSO_BoardInfo_AllocBitMap		    PSSO_BoardInfo_GetFeatureAttrs + 4
#define PSSO_BoardInfo_FreeBitMap		    PSSO_BoardInfo_AllocBitMap + 4
#define PSSO_BoardInfo_GetBitMapAttr		    PSSO_BoardInfo_FreeBitMap + 4

#define PSSO_BoardInfo_SetSprite		    PSSO_BoardInfo_GetBitMapAttr + 4
#define PSSO_BoardInfo_SetSpritePosition	    PSSO_BoardInfo_SetSprite + 4
#define PSSO_BoardInfo_SetSpriteImage		    PSSO_BoardInfo_SetSpritePosition + 4
#define PSSO_BoardInfo_SetSpriteColor		    PSSO_BoardInfo_SetSpriteImage + 4

#define PSSO_BoardInfo_CreateFeature		    PSSO_BoardInfo_SetSpriteColor + 4
#define PSSO_BoardInfo_SetFeatureAttrs		    PSSO_BoardInfo_CreateFeature + 4
#define PSSO_BoardInfo_DeleteFeature		    PSSO_BoardInfo_SetFeatureAttrs + 4
#define PSSO_BoardInfo_SpecialFeatures		    PSSO_BoardInfo_DeleteFeature + 4
#define PSSO_BoardInfo_ModeInfo			   PSSO_BoardInfo_SpecialFeatures + 12 /* SpecialFeatures is 12-bytes */
#define PSSO_BoardInfo_RGBFormat		   PSSO_BoardInfo_ModeInfo + 4
#define PSSO_BoardInfo_XOffset			   PSSO_BoardInfo_RGBFormat + 4
#define PSSO_BoardInfo_YOffset			   PSSO_BoardInfo_XOffset + 2
#define PSSO_BoardInfo_Depth			   PSSO_BoardInfo_YOffset + 2
#define PSSO_BoardInfo_ClearMask		   PSSO_BoardInfo_Depth + 1
#define PSSO_BoardInfo_Border			   PSSO_BoardInfo_ClearMask + 1
#define PSSO_BoardInfo_Mask			   PSSO_BoardInfo_Border + 2 /* BOOL type is only 2-bytes! */
#define PSSO_BoardInfo_CLUT			   PSSO_BoardInfo_Mask + 4
#define PSSO_BoardInfo_ViewPort			   PSSO_BoardInfo_CLUT + 3*256
#define PSSO_BoardInfo_VisibleBitMap		   PSSO_BoardInfo_ViewPort + 4
#define PSSO_BoardInfo_BitMapExtra		   PSSO_BoardInfo_VisibleBitMap + 4
#define PSSO_BoardInfo_BitMapList		   PSSO_BoardInfo_BitMapExtra + 4
#define PSSO_BoardInfo_MemList			   PSSO_BoardInfo_BitMapList + 12 /* BitMapList is 12-bytes */
#define PSSO_BoardInfo_MouseX			   PSSO_BoardInfo_MemList + 12 /* MemList is 12-bytes */
#define PSSO_BoardInfo_MouseY			   PSSO_BoardInfo_MouseX + 2
#define PSSO_BoardInfo_MouseWidth		   PSSO_BoardInfo_MouseY + 2
#define PSSO_BoardInfo_MouseHeight		   PSSO_BoardInfo_MouseWidth + 1
#define PSSO_BoardInfo_MouseXOffset		   PSSO_BoardInfo_MouseHeight + 1
#define PSSO_BoardInfo_MouseYOffset		   PSSO_BoardInfo_MouseXOffset + 1
#define PSSO_BoardInfo_MouseImage		   PSSO_BoardInfo_MouseYOffset + 1
#define PSSO_BoardInfo_MousePens		   PSSO_BoardInfo_MouseImage + 4
#define PSSO_BoardInfo_MouseRect		   PSSO_BoardInfo_MousePens + 4
#define PSSO_BoardInfo_MouseChunky		   PSSO_BoardInfo_MouseRect + 8 /* MouseRect is 8-bytes */
#define PSSO_BoardInfo_MouseRendered		   PSSO_BoardInfo_MouseChunky + 4
#define PSSO_BoardInfo_MouseSaveBuffer		   PSSO_BoardInfo_MouseRendered + 4

#define PSSO_BoardInfo_ChipData			    PSSO_BoardInfo_MouseSaveBuffer + 4
#define PSSO_BoardInfo_CardData			    PSSO_BoardInfo_ChipData + 16 * 4
#define PSSO_BoardInfo_MemorySpaceBase		    PSSO_BoardInfo_CardData + 16 * 4
#define PSSO_BoardInfo_MemorySpaceSize		    PSSO_BoardInfo_MemorySpaceBase + 4
#define PSSO_BoardInfo_DoubleBufferList		    PSSO_BoardInfo_MemorySpaceSize + 4
#define PSSO_BoardInfo_SyncTime			    PSSO_BoardInfo_DoubleBufferList + 4
#define PSSO_BoardInfo_SyncPeriod		    PSSO_BoardInfo_SyncTime + 4
#define PSSO_BoardInfo_SoftVBlankPort		    PSSO_BoardInfo_SyncPeriod + 8
#define PSSO_BoardInfo_SizeOf			    PSSO_BoardInfo_SoftVBlankPort + 34

/* BoardInfo flags */
/*  0-15: hardware flags */
/* 16-31: user flags */
#define BIB_HARDWARESPRITE	0	/* board has hardware sprite */
#define BIB_NOMEMORYMODEMIX	1	/* board does not support modifying planar bitmaps while displaying chunky and vice versa */
#define BIB_NEEDSALIGNMENT	2	/* bitmaps have to be aligned (not yet supported!) */
#define BIB_CACHEMODECHANGE	 3	/* board memory may be set to Imprecise (060) or Nonserialised (040) */
#define BIB_VBLANKINTERRUPT	 4	/* board can cause a hardware interrupt on a vertical retrace */
#define BIB_DBLSCANDBLSPRITEY	8	/* hardware sprite y position is doubled on doublescan display modes */
#define BIB_ILACEHALFSPRITEY	9	/* hardware sprite y position is halved on interlace display modes */
#define BIB_ILACEDBLROWOFFSET	10	/* doubled row offset in interlaced display modes needs additional horizontal bit */

#define BIB_FLICKERFIXER	12	/* board can flicker fix Amiga RGB signal */
#define BIB_VIDEOCAPTURE	13	/* board can capture video data to a memory area */
#define BIB_VIDEOWINDOW		14	/* board can display a second mem area as a pip */
#define BIB_BLITTER		15	/* board has blitter */

#define BIB_HIRESSPRITE		16	/* mouse sprite has double resolution */
#define BIB_BIGSPRITE		17	/* user wants big mouse sprite */
#define BIB_BORDEROVERRIDE	18	/* user wants to override system overscan border prefs */
#define BIB_BORDERBLANK		19	/* user wants border blanking */
#define BIB_INDISPLAYCHAIN	20	/* board switches Amiga signal */
#define BIB_QUIET		21	/* not yet implemented */
#define BIB_NOMASKBLITS		22	/* perform blits without taking care of mask */
#define BIB_NOC2PBLITS		23	/* use CPU for planar to chunky conversions */
#define BIB_NOBLITTER		24	/* disable all blitter functions */
#define BIB_OVERCLOCK		31	/* enable overclocking for some boards */

#define BIB_IGNOREMASK	BIB_NOMASKBLITS

#define BIF_HARDWARESPRITE	(1<<BIB_HARDWARESPRITE)
#define BIF_NOMEMORYMODEMIX	(1<<BIB_NOMEMORYMODEMIX)
#define BIF_NEEDSALIGNMENT	(1<<BIB_NEEDSALIGNMENT)
#define BIF_CACHEMODECHANGE	(1 << BIB_CACHEMODECHANGE)
#define BIF_VBLANKINTERRUPT	(1 << BIB_VBLANKINTERRUPT)
#define BIF_DBLSCANDBLSPRITEY	(1<<BIB_DBLSCANDBLSPRITEY)
#define BIF_ILACEHALFSPRITEY	(1<<BIB_ILACEHALFSPRITEY)
#define BIF_ILACEDBLROWOFFSET	(1<<BIB_ILACEDBLROWOFFSET)
#define BIF_FLICKERFIXER	(1<<BIB_FLICKERFIXER)
#define BIF_VIDEOCAPTURE	(1<<BIB_VIDEOCAPTURE)
#define BIF_VIDEOWINDOW		(1<<BIB_VIDEOWINDOW)
#define BIF_BLITTER		(1<<BIB_BLITTER)
#define BIF_HIRESSPRITE		(1<<BIB_HIRESSPRITE)
#define BIF_BIGSPRITE		(1<<BIB_BIGSPRITE)
#define BIF_BORDEROVERRIDE	(1<<BIB_BORDEROVERRIDE)
#define BIF_BORDERBLANK		(1<<BIB_BORDERBLANK)
#define BIF_INDISPLAYCHAIN	(1<<BIB_INDISPLAYCHAIN)
#define BIF_QUIET		(1<<BIB_QUIET)
#define BIF_NOMASKBLITS		(1<<BIB_NOMASKBLITS)
#define BIF_NOC2PBLITS		(1<<BIB_NOC2PBLITS)
#define BIF_NOBLITTER		(1<<BIB_NOBLITTER)
#define BIF_OVERCLOCK		(1 << BIB_OVERCLOCK)

#define BIF_IGNOREMASK	BIF_NOMASKBLITS

/************************************************************************/
struct picasso96_state_struct
{
    uae_u32		RGBFormat;   /* true-colour, CLUT, hi-colour, etc. */
    struct MyCLUTEntry	CLUT[256];   /* Duh! */
    uaecptr		Address;     /* Active screen address (Amiga-side) */
    uaecptr		Extent;	     /* End address of screen (Amiga-side) */
    uae_u16		Width;	     /* Active display width  (From SetGC) */
    uae_u16		VirtualWidth;/* Total screen width (From SetPanning) */
    uae_u16		BytesPerRow; /* Total screen width in bytes (From SetGC) */
    uae_u16		Height;	     /* Active display height (From SetGC) */
    uae_u16		VirtualHeight; /* Total screen height */
    uae_u8		GC_Depth;    /* From SetGC() */
    uae_u8		GC_Flags;    /* From SetGC() */
    long		XOffset;     /* From SetPanning() */
    long		YOffset;     /* From SetPanning() */
    uae_u8		SwitchState; /* From SetSwitch() - 0 is Amiga, 1 is Picasso */
    uae_u8		BytesPerPixel;
    uae_u8		CardFound;
    //here follow winuae additional entrys
    uae_u8    *HostAddress; /* Active screen address (PC-side) */
    // host address is need because Windows
    // support NO direct access all the time to gfx Card
    // everytime windows can remove your surface from card so the mainrender place
    // must be in memory
    long		XYOffset;
};

extern void InitPicasso96 (void);

extern uae_u32 gfxmem_mask;
extern uae_u8 *gfxmemory;

extern int uaegfx_card_found;

extern struct picasso96_state_struct picasso96_state;

extern int  DX_InvertRect (int X, int Y, int Width, int Height);
extern void DX_SetPalette (int start, int count);
extern void DX_Invalidate (int, int, int, int);
extern int  DX_Flip (void);
extern void picasso_enablescreen (int on);
extern void picasso_refresh (void);
extern void picasso_handle_vsync (void);
extern void init_hz_p96 (void);
static __inline__ void picasso_handle_hsync (void)
{
}
extern int picasso_palette (void);
extern void picasso_reset (void);

/* This structure describes the UAE-side framebuffer for the Picasso
 * screen.  */
struct picasso_vidbuf_description {
    int width, height, depth;
    int rowbytes, pixbytes;
    int extra_mem; /* nonzero if there's a second buffer that must be updated */
    uae_u32 rgbformat;
    uae_u32 selected_rgbformat;
};

extern struct picasso_vidbuf_description picasso_vidinfo;

extern void gfx_set_picasso_modeinfo (uae_u32 w, uae_u32 h, uae_u32 d, RGBFTYPE rgbfmt);
extern void gfx_set_picasso_baseaddr (uaecptr);
extern void gfx_set_picasso_state (int on);
extern uae_u8 *gfx_lock_picasso (void);
extern void gfx_unlock_picasso (void);
extern void picasso_clip_mouse (int *, int *);
extern void picasso_putcursor (int,int,int,int);
extern void picasso_clearcursor (void);

extern int p96refresh_active;
extern int p96hsync_counter;

#define LIB_SIZE 34
#define CARD_FLAGS LIB_SIZE
#define CARD_EXECBASE (CARD_FLAGS + 2)
#define CARD_EXPANSIONBASE (CARD_EXECBASE + 4)
#define CARD_SEGMENTLIST (CARD_EXPANSIONBASE + 4)
#define CARD_NAME (CARD_SEGMENTLIST + 4)
/* uae specific stuff */
#define CARD_RESLIST (CARD_NAME + 4)
#define CARD_RESLISTSIZE (CARD_RESLIST + 4)
#define CARD_BOARDINFO (CARD_RESLISTSIZE + 4)
#define CARD_VBLANKIRQ (CARD_BOARDINFO + 4)
#define CARD_VBLANKFLAG (CARD_VBLANKIRQ + 22)
#define CARD_VBLANKCODE (CARD_VBLANKFLAG + 4)
#define CARD_END (CARD_VBLANKCODE + 16 * 2)
#define CARD_SIZEOF CARD_END

#ifdef __cplusplus
  extern "C" {
#endif
  void copy_screen_16bit_swap(uae_u8 *dst, uae_u8 *src, int bytes);
  void copy_screen_32bit_to_16bit_neon(uae_u8 *dst, uae_u8 *src, int bytes);
#ifdef __cplusplus
  }
#endif


#endif

#endif
