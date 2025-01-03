#include <stdio.h>
#include <stdbool.h>
#include "sysdeps.h"

#ifdef CAPS

#ifdef _WIN32
#include <shlobj.h>
#endif

#include "uae/caps.h"
#include "zfile.h"
#include "gui.h"
#include "uae.h"
#include "uae/dlopen.h"

#include "Comtype.h"
#include "CapsAPI.h"

#define CAPS_TRACKTIMING 1
#define LOG_REVOLUTION 0

static SDWORD caps_cont[4]= {-1, -1, -1, -1};
static bool caps_revolution_hack[4];
static int caps_locked[4];
static int caps_flags = DI_LOCK_DENVAR|DI_LOCK_DENNOISE|DI_LOCK_NOISE|DI_LOCK_UPDATEFD|DI_LOCK_TYPE|DI_LOCK_OVLBIT;
static struct CapsVersionInfo cvi;
static bool oldlib, canseed;

#ifdef _WIN32
#define CAPSCALL __cdecl
#else
#define CAPSCALL
#endif

typedef SDWORD (CAPSCALL * CAPSINIT)(void);
static CAPSINIT pCAPSInit;
typedef SDWORD (CAPSCALL * CAPSADDIMAGE)(void);
static CAPSADDIMAGE pCAPSAddImage;
typedef SDWORD (CAPSCALL * CAPSLOCKIMAGEMEMORY)(SDWORD,PUBYTE,UDWORD,UDWORD);
static CAPSLOCKIMAGEMEMORY pCAPSLockImageMemory;
typedef SDWORD (CAPSCALL * CAPSUNLOCKIMAGE)(SDWORD);
static CAPSUNLOCKIMAGE pCAPSUnlockImage;
typedef SDWORD (CAPSCALL * CAPSLOADIMAGE)(SDWORD,UDWORD);
static CAPSLOADIMAGE pCAPSLoadImage;
typedef SDWORD (CAPSCALL * CAPSGETIMAGEINFO)(PCAPSIMAGEINFO,SDWORD);
static CAPSGETIMAGEINFO pCAPSGetImageInfo;
typedef SDWORD (CAPSCALL * CAPSLOCKTRACK)(PCAPSTRACKINFO,SDWORD,UDWORD,UDWORD,UDWORD);
static CAPSLOCKTRACK pCAPSLockTrack;
typedef SDWORD (CAPSCALL * CAPSUNLOCKTRACK)(SDWORD,UDWORD);
static CAPSUNLOCKTRACK pCAPSUnlockTrack;
typedef SDWORD (CAPSCALL * CAPSUNLOCKALLTRACKS)(SDWORD);
static CAPSUNLOCKALLTRACKS pCAPSUnlockAllTracks;
typedef SDWORD (CAPSCALL * CAPSGETVERSIONINFO)(PCAPSVERSIONINFO,UDWORD);
static CAPSGETVERSIONINFO pCAPSGetVersionInfo;
typedef SDWORD (CAPSCALL * CAPSGETINFO)(PVOID pinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD inftype, UDWORD infid);
static CAPSGETINFO pCAPSGetInfo;
typedef SDWORD (CAPSCALL * CAPSSETREVOLUTION)(SDWORD id, UDWORD value);
static CAPSSETREVOLUTION pCAPSSetRevolution;
typedef SDWORD (CAPSCALL * CAPSGETIMAGETYPEMEMORY)(PUBYTE buffer, UDWORD length);
static CAPSGETIMAGETYPEMEMORY pCAPSGetImageTypeMemory;

int caps_init (void)
{
	static int init, noticed;
	int i;

	if (init)
		return 1;
	UAE_DLHANDLE h = uae_dlopen_plugin(_T("libcapsimage"));
	if (!h) {
		if (noticed)
			return 0;
		notify_user (NUMSG_NOCAPS);
		noticed = 1;
		return 0;
	}

	if (uae_dlsym(h, "CAPSLockImageMemory") == 0 || uae_dlsym(h, "CAPSGetVersionInfo") == 0) {
		if (noticed)
			return 0;
		notify_user (NUMSG_OLDCAPS);
		noticed = 1;
		return 0;
	}

	pCAPSInit = (CAPSINIT) uae_dlsym(h, "CAPSInit");
	pCAPSAddImage = (CAPSADDIMAGE) uae_dlsym(h, "CAPSAddImage");
	pCAPSLockImageMemory = (CAPSLOCKIMAGEMEMORY) uae_dlsym(h, "CAPSLockImageMemory");
	pCAPSUnlockImage = (CAPSUNLOCKIMAGE) uae_dlsym(h, "CAPSUnlockImage");
	pCAPSLoadImage = (CAPSLOADIMAGE) uae_dlsym(h, "CAPSLoadImage");
	pCAPSGetImageInfo = (CAPSGETIMAGEINFO) uae_dlsym(h, "CAPSGetImageInfo");
	pCAPSLockTrack = (CAPSLOCKTRACK) uae_dlsym(h, "CAPSLockTrack");
	pCAPSUnlockTrack = (CAPSUNLOCKTRACK) uae_dlsym(h, "CAPSUnlockTrack");
	pCAPSUnlockAllTracks = (CAPSUNLOCKALLTRACKS) uae_dlsym(h, "CAPSUnlockAllTracks");
	pCAPSGetVersionInfo = (CAPSGETVERSIONINFO) uae_dlsym(h, "CAPSGetVersionInfo");
	pCAPSGetInfo = (CAPSGETINFO) uae_dlsym(h, "CAPSGetInfo");
	pCAPSSetRevolution = (CAPSSETREVOLUTION) uae_dlsym(h, "CAPSSetRevolution");
	pCAPSGetImageTypeMemory = (CAPSGETIMAGETYPEMEMORY) uae_dlsym(h, "CAPSGetImageTypeMemory");

	init = 1;
	cvi.type = 1;
	pCAPSGetVersionInfo (&cvi, 0);
	write_log (_T("CAPS: library version %d.%d (flags=%08X)\n"), cvi.release, cvi.revision, cvi.flag);
	oldlib = (cvi.flag & (DI_LOCK_TRKBIT | DI_LOCK_OVLBIT)) != (DI_LOCK_TRKBIT | DI_LOCK_OVLBIT);
	if (!oldlib)
		caps_flags |= DI_LOCK_TRKBIT | DI_LOCK_OVLBIT;
	canseed = (cvi.flag & DI_LOCK_SETWSEED) != 0;
	for (i = 0; i < 4; i++)
		caps_cont[i] = pCAPSAddImage ();
	return 1;
}

void caps_unloadimage (int drv)
{
	if (!caps_locked[drv])
		return;
	pCAPSUnlockAllTracks (caps_cont[drv]);
	pCAPSUnlockImage (caps_cont[drv]);
	caps_locked[drv] = 0;
}

int caps_loadimage (struct zfile *zf, int drv, int *num_tracks)
{
	static int notified;
	struct CapsImageInfo ci;
	int len, ret;
	uae_u8 *buf;
	TCHAR s1[100];
	struct CapsDateTimeExt *cdt;
	int type;

	if (!caps_init ())
		return 0;
	caps_unloadimage (drv);
	zfile_fseek (zf, 0, SEEK_END);
	len = zfile_ftell (zf);
	zfile_fseek (zf, 0, SEEK_SET);
	if (len <= 0)
		return 0;
	buf = xmalloc (uae_u8, len);
	if (!buf)
		return 0;
	if (zfile_fread (buf, len, 1, zf) == 0)
		return 0;
	type = -1;
	if (pCAPSGetImageTypeMemory) {
		type = pCAPSGetImageTypeMemory(buf, len);
		if (type == citError || type == citUnknown) {
			write_log(_T("caps: CAPSGetImageTypeMemory() returned %d\n"), type);
			return 0;
		}
		if (type == citKFStream || type == citDraft) {
			write_log(_T("caps: CAPSGetImageTypeMemory() returned unsupported image type %d\n"), type);
			return 0;
		}
	}
	ret = pCAPSLockImageMemory (caps_cont[drv], buf, len, 0);
	xfree (buf);
	if (ret != imgeOk) {
		if (ret == imgeIncompatible || ret == imgeUnsupported) {
			if (!notified)
				notify_user (NUMSG_OLDCAPS);
			notified = 1;
		}
		write_log (_T("caps: CAPSLockImageMemory() returned %d\n"), ret);
		return 0;
	}
	caps_locked[drv] = 1;
	ret = pCAPSGetImageInfo(&ci, caps_cont[drv]);
	*num_tracks = (ci.maxcylinder - ci.mincylinder + 1) * (ci.maxhead - ci.minhead + 1);

	if (cvi.release < 4) { // pre-4.x bug workaround
		struct CapsTrackInfoT1 cit;
		cit.type = 1;
		if (pCAPSLockTrack ((PCAPSTRACKINFO)&cit, caps_cont[drv], 0, 0, caps_flags) == imgeIncompatible) {
			if (!notified)
				notify_user (NUMSG_OLDCAPS);
			notified = 1;
			caps_unloadimage (drv);
			return 0;
		}
		pCAPSUnlockAllTracks (caps_cont[drv]);
	}

	ret = pCAPSLoadImage(caps_cont[drv], caps_flags);
	caps_revolution_hack[drv] = type == citCTRaw;
	cdt = &ci.crdt;
	_sntprintf (s1, sizeof s1, _T("%d.%d.%d %d:%d:%d"), cdt->day, cdt->month, cdt->year, cdt->hour, cdt->min, cdt->sec);
	write_log (_T("caps: type:%d imagetype:%d date:%s rel:%d rev:%d\n"), ci.type, type, s1, ci.release, ci.revision);
	return 1;
}

#if 0
static void outdisk (void)
{
	struct CapsTrackInfo ci;
	int tr;
	FILE *f;
	static int done;

	if (done)
		return;
	done = 1;
	f = fopen("c:\\out3.dat", "wb");
	if (!f)
		return;
	for (tr = 0; tr < 160; tr++) {
		pCAPSLockTrack(&ci, caps_cont[0], tr / 2, tr & 1, caps_flags);
		fwrite (ci.trackdata[0], ci.tracksize[0], 1, f);
		fwrite ("XXXX", 4, 1, f);
	}
	fclose (f);
}
#endif

static void mfmcopy (uae_u16 *mfm, uae_u8 *data, int len)
{
	int memlen = (len + 7) / 8;
	for (int i = 0; i < memlen; i+=2) {
		if (i + 1 < memlen)
			*mfm++ = (data[i] << 8) + data[i + 1];
		else
			*mfm++ = (data[i] << 8);
	}
}

static int load (struct CapsTrackInfoT2 *ci, int drv, int track, bool seed, bool newtrack)
{
	int flags = caps_flags;
	if (canseed) {
		ci->type = 2;
		if (seed) {
			flags |= DI_LOCK_SETWSEED;
			ci->wseed = uaerand ();
		}
	} else {
		ci->type = 1;
	}
#if 0
	if (newtrack)
		flags |= DI_LOCK_NOUPDATE;
#endif
	if (pCAPSLockTrack ((PCAPSTRACKINFO)ci, caps_cont[drv], track / 2, track & 1, flags) != imgeOk)
		return 0;
	return 1;
}

int caps_loadrevolution (uae_u16 *mfmbuf, uae_u16 *tracktiming, int drv, int track, int *tracklength, int *nextrev, bool track_access_done)
{
	int len;
	struct CapsTrackInfoT2 ci;

	if (!track_access_done && caps_revolution_hack[drv]) {
#if LOG_REVOLUTION
		CapsRevolutionInfo  pinfo;
		pCAPSGetInfo(&pinfo, caps_cont[drv], track / 2, track & 1, cgiitRevolution, 0);
		write_log(_T("%03d skipped revolution increase. Next = %d\n"), track, pinfo.next);
#endif
		return 1;
	}
	if (!load (&ci, drv, track, false, false))
		return 0;
	if (oldlib)
		len = ci.tracklen * 8;
	else
		len = ci.tracklen;
	*tracklength = len;
	mfmcopy (mfmbuf, ci.trackbuf, len);
#if CAPS_TRACKTIMING
	if (ci.timelen > 0 && tracktiming) {
		for (int i = 0; i < ci.timelen; i++)
			tracktiming[i] = (uae_u16)ci.timebuf[i];
	}
#endif
	if (nextrev && pCAPSGetInfo) {
		CapsRevolutionInfo  pinfo;
		*nextrev = 0;
		pCAPSGetInfo(&pinfo, caps_cont[drv], track / 2, track & 1, cgiitRevolution, 0);
#if LOG_REVOLUTION
		write_log (_T("%03d load next rev = %d\n"), track, pinfo.next);
#endif
		if (pinfo.max > 0)
			*nextrev = pinfo.next;
	}
	return 1;
}

int caps_loadtrack (uae_u16 *mfmbuf, uae_u16 *tracktiming, int drv, int track, int *tracklength, int *multirev, int *gapoffset, int *nextrev, bool sametrack)
{
	int len;
	struct CapsTrackInfoT2 ci;
	CapsRevolutionInfo  pinfo;

	if (tracktiming)
		*tracktiming = 0;

	if (nextrev && pCAPSSetRevolution) {
		if (sametrack) {
			pCAPSSetRevolution(caps_cont[drv], *nextrev);
#if LOG_REVOLUTION
			write_log(_T("%03d set rev = %d\n"), track, *nextrev);
#endif
		} else {
			pCAPSSetRevolution(caps_cont[drv], 0);
#if LOG_REVOLUTION
			write_log(_T("%03d clear rev\n"), track, *nextrev);
#endif
		}
	}

	if (!load (&ci, drv, track, true, sametrack != true))
		return 0;

	if (pCAPSGetInfo) {
		if (nextrev)
			*nextrev = 0;
		pCAPSGetInfo(&pinfo, caps_cont[drv], track / 2, track & 1, cgiitRevolution, 0);
#if LOG_REVOLUTION
		write_log(_T("%03d get next rev = %d\n"), track, pinfo.next);
#endif
		if (nextrev && sametrack && pinfo.max > 0)
			*nextrev = pinfo.next;
	}

	*multirev = (ci.type & CTIT_FLAG_FLAKEY) ? 1 : 0;
	if (oldlib) {
		len = ci.tracklen * 8;
		*gapoffset = ci.overlap >= 0 ? ci.overlap * 8 : -1;
	} else {
		len = ci.tracklen;
		*gapoffset = ci.overlap >= 0 ? ci.overlap : -1;
	}
	//write_log (_T("%d %d %d %d\n"), track, len, ci.tracklen, ci.overlap);
	*tracklength = len;
	mfmcopy (mfmbuf, ci.trackbuf, len);
#if 0
	{
		FILE *f=fopen("c:\\1.txt","wb");
		fwrite (ci.trackbuf, len, 1, f);
		fclose (f);
	}
#endif
#if CAPS_TRACKTIMING
	if (ci.timelen > 0 && tracktiming) {
		for (int i = 0; i < ci.timelen; i++)
			tracktiming[i] = (uae_u16)ci.timebuf[i];
	}
#endif
#if 0
	write_log (_T("caps: drive:%d track:%d len:%d multi:%d timing:%d type:%d overlap:%d\n"),
		drv, track, len, *multirev, ci.timelen, type, ci.overlap);
#endif
	return 1;
}

#endif /* CAPS */
