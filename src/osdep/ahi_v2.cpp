/*
* UAE - The Un*x Amiga Emulator
*
* OpenAL AHI 7.1 "wrapper"
*
* Copyright 2008 Toni Wilen
*/

// Amiga-side driver does not exist, ahi_v2 never completed
// http://eab.abime.net/showthread.php?t=71953

#include "sysconfig.h"

#if defined(AHI)
#ifdef AHI_v2
#include <ctype.h>
#include <assert.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#include "sysdeps.h"
#include "options.h"
#include "audio.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "memory.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "threaddep/thread.h"
#include "native2amiga.h"
#include "sounddep/sound.h"
#include "ahi_v2.h"

#define AHI_STRUCT_VERSION 1

int ahi_debug = 1;

#define UAE_MAXCHANNELS 24
#define UAE_MAXSOUNDS 256
#define UAE_RECORDSAMPLES 2048
#define UAE_MAXPLAYSAMPLES 2048

#define ub_Flags 34
#define ub_Pad1 (ub_Flags + 1)
#define ub_Pad2 (ub_Pad1 + 1)
#define ub_SysLib (ub_Pad2 + 2)
#define ub_SegList (ub_SysLib + 4)
#define ub_DOSBase (ub_SegList + 4)
#define ub_AHIFunc (ub_DOSBase + 4)

#define pub_SizeOf 0
#define pub_Version (pub_SizeOf + 4)
#define pub_Index (pub_Version + 4)
#define pub_Base (pub_Index + 4)
#define pub_audioctrl (pub_Base + 4)
#define pub_FuncTask (pub_audioctrl + 4)
#define pub_WaitMask (pub_FuncTask + 4)
#define pub_WaitSigBit (pub_WaitMask + 4)
#define pub_FuncMode (pub_WaitSigBit +4)
#define pub_TaskMode (pub_FuncMode + 2)
#define pub_ChannelSignal (pub_TaskMode + 2)
#define pub_ChannelSignalAck (pub_ChannelSignal + 4)
#define pub_ahism_Channel (pub_ChannelSignalAck + 4)
#define pub_RecordHookDone (pub_ahism_Channel + 2)
#define pub_RecordSampleType (pub_RecordHookDone + 2)
#define pub_RecordBufferSize (pub_RecordSampleType + 4)
#define pub_RecordBufferSizeBytes (pub_RecordBufferSize + 4)
#define pub_RecordBuffer (pub_RecordBufferSizeBytes + 4)
#define pub_ChannelInfo (pub_RecordBuffer + 4 + 4 + 4 + 4)
#define pub_End (pub_ChannelInfo + 4)

#define FUNCMODE_PLAY 1
#define FUNCMODE_RECORD 2
#define FUNCMODE_RECORDALLOC 4

#define ahie_Effect 0
#define ahieci_Func 4
#define ahieci_Channels 8
#define ahieci_Pad 10
#define ahieci_Offset 12

#define ahisi_Type 0
#define ahisi_Address 4
#define ahisi_Length 8

#define ahiac_AudioCtrl 0
#define ahiac_Flags ahiac_AudioCtrl + 4
#define ahiac_SoundFunc ahiac_Flags + 4
#define ahiac_PlayerFunc ahiac_SoundFunc + 4
#define ahiac_PlayerFreq ahiac_PlayerFunc + 4
#define ahiac_MinPlayerFreq ahiac_PlayerFreq + 4
#define ahiac_MaxPlayerFreq ahiac_MinPlayerFreq + 4
#define ahiac_MixFreq ahiac_MaxPlayerFreq + 4
#define ahiac_Channels ahiac_MixFreq + 4
#define ahiac_Sounds ahiac_Channels + 2
#define ahiac_DriverData ahiac_Sounds + 2
#define ahiac_MixerFunc ahiac_DriverData + 4
#define ahiac_SamplerFunc ahiac_MixerFunc + 4 
#define ahiac_Obsolete ahiac_SamplerFunc + 4 
#define ahiac_BuffSamples ahiac_Obsolete + 4 
#define ahiac_MinBuffSamples ahiac_BuffSamples + 4 
#define ahiac_MaxBuffSamples ahiac_MinBuffSamples + 4 
#define ahiac_BuffSize ahiac_MaxBuffSamples + 4 
#define ahiac_BuffType ahiac_BuffSize + 4 
#define ahiac_PreTimer ahiac_BuffType + 4 
#define ahiac_PostTimer ahiac_PreTimer + 4 
#define ahiac_AntiClickSamples ahiac_PostTimer + 4 
#define ahiac_PreTimerFunc ahiac_AntiClickSamples + 4 
#define ahiac_PostTimerFunc ahiac_PreTimerFunc + 4 

/* AHIsub_AllocAudio return flags */
#define AHISF_ERROR		(1<<0)
#define AHISF_MIXING		(1<<1)
#define AHISF_TIMING		(1<<2)
#define AHISF_KNOWSTEREO	(1<<3)
#define AHISF_KNOWHIFI		(1<<4)
#define AHISF_CANRECORD 	(1<<5)
#define AHISF_CANPOSTPROCESS	(1<<6)
#define AHISF_KNOWMULTICHANNEL	(1<<7)

#define AHISB_ERROR		(0)
#define AHISB_MIXING		(1)
#define AHISB_TIMING		(2)
#define AHISB_KNOWSTEREO	(3)
#define AHISB_KNOWHIFI		(4)
#define AHISB_CANRECORD		(5)
#define AHISB_CANPOSTPROCESS	(6)
#define AHISB_KNOWMULTICHANNEL	(7)

/* AHIsub_Start() and AHIsub_Stop() flags */
#define	AHISF_PLAY		(1<<0)
#define	AHISF_RECORD		(1<<1)

#define	AHISB_PLAY		(0)
#define	AHISB_RECORD		(1)

/* ahiac_Flags */
#define	AHIACF_VOL		(1<<0)
#define	AHIACF_PAN		(1<<1)
#define	AHIACF_STEREO		(1<<2)
#define	AHIACF_HIFI		(1<<3)
#define	AHIACF_PINGPONG		(1<<4)
#define	AHIACF_RECORD		(1<<5)
#define AHIACF_MULTTAB  	(1<<6)
#define	AHIACF_MULTICHANNEL	(1<<7)

#define	AHIACB_VOL		(0)
#define	AHIACB_PAN		(1)
#define	AHIACB_STEREO		(2)
#define	AHIACB_HIFI		(3)
#define	AHIACB_PINGPONG		(4)
#define	AHIACB_RECORD		(5)
#define AHIACB_MULTTAB  	(6)
#define	AHIACB_MULTICHANNEL	(7)

#define AHISB_IMM		0
#define AHISB_NODELAY		1
#define AHISF_IMM		(1 << AHISB_IMM)
#define AHISF_NODELAY		(1 << SHISB_NODELAY)

#define AHIET_CANCEL		(1 << 31)
#define AHIET_MASTERVOLUME	1
#define AHIET_OUTPUTBUFFER	2
#define AHIET_DSPMASK		3
#define AHIET_DSPECHO		4
#define AHIET_CHANNELINFO	5

#define AHI_TagBase		(0x80000000)
#define AHI_TagBaseR		(AHI_TagBase|0x8000)

/* AHI_AllocAudioA tags */
#define AHIA_AudioID		(AHI_TagBase+1)		/* Desired audio mode */
#define AHIA_MixFreq		(AHI_TagBase+2)		/* Suggested mixing frequency */
#define AHIA_Channels		(AHI_TagBase+3)		/* Suggested number of channels */
#define AHIA_Sounds		(AHI_TagBase+4)		/* Number of sounds to use */
#define AHIA_SoundFunc		(AHI_TagBase+5)		/* End-of-Sound Hook */
#define AHIA_PlayerFunc		(AHI_TagBase+6)		/* Player Hook */
#define AHIA_PlayerFreq		(AHI_TagBase+7)		/* Frequency for player Hook (Fixed)*/
#define AHIA_MinPlayerFreq	(AHI_TagBase+8)		/* Minimum Frequency for player Hook */
#define AHIA_MaxPlayerFreq	(AHI_TagBase+9)		/* Maximum Frequency for player Hook */
#define AHIA_RecordFunc		(AHI_TagBase+10)	/* Sample recording Hook */
#define AHIA_UserData		(AHI_TagBase+11)	/* What to put in ahiac_UserData */
#define AHIA_AntiClickSamples	(AHI_TagBase+13)	/* # of samples to smooth (V6)	*/

/* AHI_ControlAudioA tags */
#define AHIC_Play		(AHI_TagBase+80)	/* Boolean */
#define AHIC_Record		(AHI_TagBase+81)	/* Boolean */
#define AHIC_MonitorVolume	(AHI_TagBase+82)
#define AHIC_MonitorVolume_Query (AHI_TagBase+83)	/* ti_Data is pointer to Fixed (LONG) */
#define AHIC_MixFreq_Query	(AHI_TagBase+84)	/* ti_Data is pointer to ULONG */
/* --- New for V2, they will be ignored by V1 --- */
#define AHIC_InputGain		(AHI_TagBase+85)
#define AHIC_InputGain_Query	(AHI_TagBase+86)	/* ti_Data is pointer to Fixed (LONG) */
#define AHIC_OutputVolume	(AHI_TagBase+87)
#define AHIC_OutputVolume_Query	(AHI_TagBase+88)	/* ti_Data is pointer to Fixed (LONG) */
#define AHIC_Input		(AHI_TagBase+89)
#define AHIC_Input_Query	(AHI_TagBase+90)	/* ti_Data is pointer to ULONG */
#define AHIC_Output		(AHI_TagBase+91)
#define AHIC_Output_Query	(AHI_TagBase+92)	/* ti_Data is pointer to ULONG */

/* AHI_GetAudioAttrsA tags */
#define AHIDB_AudioID		(AHI_TagBase+100)
#define AHIDB_Driver		(AHI_TagBaseR+101)	/* Pointer to name of driver */
#define AHIDB_Flags		(AHI_TagBase+102)	/* Private! */
#define AHIDB_Volume		(AHI_TagBase+103)	/* Boolean */
#define AHIDB_Panning		(AHI_TagBase+104)	/* Boolean */
#define AHIDB_Stereo		(AHI_TagBase+105)	/* Boolean */
#define AHIDB_HiFi		(AHI_TagBase+106)	/* Boolean */
#define AHIDB_PingPong		(AHI_TagBase+107)	/* Boolean */
#define AHIDB_MultTable		(AHI_TagBase+108)	/* Private! */
#define AHIDB_Name		(AHI_TagBaseR+109)	/* Pointer to name of this mode */
#define AHIDB_Bits		(AHI_TagBase+110)	/* Output bits */
#define AHIDB_MaxChannels	(AHI_TagBase+111)	/* Max supported channels */
#define AHIDB_MinMixFreq	(AHI_TagBase+112)	/* Min mixing freq. supported */
#define AHIDB_MaxMixFreq	(AHI_TagBase+113)	/* Max mixing freq. supported */
#define AHIDB_Record		(AHI_TagBase+114)	/* Boolean */
#define AHIDB_Frequencies	(AHI_TagBase+115)
#define AHIDB_FrequencyArg	(AHI_TagBase+116)	/* ti_Data is frequency index */
#define AHIDB_Frequency		(AHI_TagBase+117)
#define AHIDB_Author		(AHI_TagBase+118)	/* Pointer to driver author name */
#define AHIDB_Copyright		(AHI_TagBase+119)	/* Pointer to driver copyright notice */
#define AHIDB_Version		(AHI_TagBase+120)	/* Pointer to driver version string */
#define AHIDB_Annotation	(AHI_TagBase+121)	/* Pointer to driver annotation text */
#define AHIDB_BufferLen		(AHI_TagBase+122)	/* Specifies the string buffer size */
#define AHIDB_IndexArg		(AHI_TagBase+123)	/* ti_Data is frequency! */
#define AHIDB_Index		(AHI_TagBase+124)
#define AHIDB_Realtime		(AHI_TagBase+125)	/* Boolean */
#define AHIDB_MaxPlaySamples	(AHI_TagBase+126)	/* It's sample *frames* */
#define AHIDB_MaxRecordSamples	(AHI_TagBase+127)	/* It's sample *frames* */
#define AHIDB_FullDuplex	(AHI_TagBase+129)	/* Boolean */
/* --- New for V2, they will be ignored by V1 --- */
#define AHIDB_MinMonitorVolume	(AHI_TagBase+130)
#define AHIDB_MaxMonitorVolume	(AHI_TagBase+131)
#define AHIDB_MinInputGain	(AHI_TagBase+132)
#define AHIDB_MaxInputGain	(AHI_TagBase+133)
#define AHIDB_MinOutputVolume	(AHI_TagBase+134)
#define AHIDB_MaxOutputVolume	(AHI_TagBase+135)
#define AHIDB_Inputs		(AHI_TagBase+136)
#define AHIDB_InputArg		(AHI_TagBase+137)	/* ti_Data is input index */
#define AHIDB_Input		(AHI_TagBase+138)
#define AHIDB_Outputs		(AHI_TagBase+139)
#define AHIDB_OutputArg		(AHI_TagBase+140)	/* ti_Data is input index */
#define AHIDB_Output		(AHI_TagBase+141)
/* --- New for V4, they will be ignored by V2 and earlier --- */
#define AHIDB_Data		(AHI_TagBaseR+142)	/* Private! */
#define AHIDB_DriverBaseName	(AHI_TagBaseR+143)	/* Private! */
/* --- New for V6, they will be ignored by V4 and earlier --- */
#define AHIDB_MultiChannel	(AHI_TagBase+144)	/* Boolean */

/* Sound Types */
#define AHIST_NOTYPE		(~0UL)			/* Private */
#define AHIST_SAMPLE		(0UL)			/* 8 or 16 bit sample */
#define AHIST_DYNAMICSAMPLE	(1UL)			/* Dynamic sample */
#define AHIST_INPUT		(1UL<<29)		/* The input from your sampler */
#define AHIST_BW		(1UL<<30)		/* Private */

/* Sample types */
/* Note that only AHIST_M8S, AHIST_S8S, AHIST_M16S and AHIST_S16S
(plus AHIST_M32S, AHIST_S32S and AHIST_L7_1 in V6)
are supported by AHI_LoadSound(). */
#define AHIST_M8S		(0UL)			/* Mono, 8 bit signed (BYTE) */
#define AHIST_M16S		(1UL)			/* Mono, 16 bit signed (WORD) */
#define AHIST_S8S		(2UL)			/* Stereo, 8 bit signed (2xBYTE) */
#define AHIST_S16S		(3UL)			/* Stereo, 16 bit signed (2xWORD) */
#define AHIST_M32S		(8UL)			/* Mono, 32 bit signed (LONG) */
#define AHIST_S32S		(10UL)			/* Stereo, 32 bit signed (2xLONG) */
#define AHIST_M8U		(4UL)			/* OBSOLETE! */
#define AHIST_L7_1		(0x00c3000aUL)		/* 7.1, 32 bit signed (8xLONG) */

/* Error codes */
#define AHIE_OK			(0UL)			/* No error */
#define AHIE_NOMEM		(1UL)			/* Out of memory */
#define AHIE_BADSOUNDTYPE	(2UL)			/* Unknown sound type */
#define AHIE_BADSAMPLETYPE	(3UL)			/* Unknown/unsupported sample type */
#define AHIE_ABORTED		(4UL)			/* User-triggered abortion */
#define AHIE_UNKNOWN		(5UL)			/* Error, but unknown */
#define AHIE_HALFDUPLEX		(6UL)			/* CMD_WRITE/CMD_READ failure */

struct dssample {
	int num;
	int ch;
	int bitspersample;
	int bytespersample;
	int dynamic;
	uae_u32 addr;
	uae_u32 len;
	uae_u32 type;
	uae_u32 sampletype;
	uae_u32 al_buffer[2];
};

struct chsample {
	int frequency;
	int volume;
	int panning;
	int backwards;
	struct dssample* ds;
	int srcplayoffset;
	int srcplaylen;
};

struct dschannel {
	int num;
	struct chsample cs;
	struct chsample csnext;
	int channelsignal;
	int dsplaying;
	uae_u32 al_source;
	int samplecounter;
	int buffertoggle;
	int maxplaysamples;
	int totalsamples;
	int waitforack;
};

struct DSAHI {
	uae_u32 audioctrl;
	int chout;
	int bits24;
	int bitspersampleout;
	int bytespersampleout;
	int channellength;
	int mixlength;
	int input;
	int output;
	int channels;
	int sounds;
	int playerfreq;
	uae_u32 audioid;
	int enabledisable;
	struct dssample* sample;
	struct dschannel* channel;
	int playing, recording;
	evt evttime;
	uae_u32 signalchannelmask;

	SDL_AudioDeviceID al_dev, al_recorddev;
	ALCcontext* al_ctx;
	int al_bufferformat;
	uae_u8* tmpbuffer;
	int tmpbuffer_size;
	int dsrecording;
	int record_samples;
	int record_ch;
	int record_bytespersample;
	int record_wait;
	int maxplaysamples;
};

static struct DSAHI dsahi[1];

#define GETAHI (&dsahi[get_long(get_long(audioctrl + ahiac_DriverData) + pub_Index)])
#define GETSAMPLE (dsahip && sound >= 0 && sound < UAE_MAXSOUNDS ? &dsahip->sample[sound] : NULL)
#define GETCHANNEL (dsahip && channel >= 0 && channel < UAE_MAXCHANNELS ? &dsahip->channel[channel] : NULL)

static int cansurround;
static uae_u32 xahi_author, xahi_copyright, xahi_version;
static uae_u32 xahi_output[MAX_SOUND_DEVICES], xahi_output_num;
static uae_u32 xahi_input[MAX_SOUND_DEVICES], xahi_input_num;
static int ahi_paused;
static int ahi_active;

#define TAG_DONE   (0L)		/* terminates array of TagItems. ti_Data unused */
#define TAG_IGNORE (1L)		/* ignore this item, not end of array */
#define TAG_MORE   (2L)		/* ti_Data is pointer to another array of TagItems */
#define TAG_SKIP   (3L)		/* skip this and the next ti_Data items */
#define TAG_USER   ((uae_u32)(1L << 31))

static uae_u32 gettag(uae_u32* tagpp, uae_u32* datap)
{
	uae_u32 tagp = *tagpp;
	for (;;) {
		uae_u32 tag = get_long(tagp);
		uae_u32 data = get_long(tagp + 4);
		switch (tag)
		{
		case TAG_DONE:
			return 0;
		case TAG_IGNORE:
			tagp += 8;
			break;
		case TAG_MORE:
			tagp = data;
			break;
		case TAG_SKIP:
			tagp += data * 8;
			break;
		default:
			tagp += 8;
			*tagpp = tagp;
			*datap = data;
			return tag;
		}
	}
}


static int sendsignal(struct DSAHI* dsahip)
{
	uae_u32 audioctrl = dsahip->audioctrl;
	uae_u32 puaebase = get_long(audioctrl + ahiac_DriverData);
	uae_u32 channelinfo;
	uae_u32 task, signalmask;
	uae_s16 taskmode = get_word(puaebase + pub_TaskMode);
	uae_s16 funcmode = get_word(puaebase + pub_FuncMode);
	task = get_long(puaebase + pub_FuncTask);
	signalmask = get_long(puaebase + pub_WaitMask);

	if ((!dsahip->playing && !dsahip->recording) || ahi_paused)
		return 0;
	if (taskmode <= 0)
		return 0;
	if (dsahip->enabledisable) {
		// allocate Amiga-side recordingbuffer
		funcmode &= FUNCMODE_RECORDALLOC;
		put_word(puaebase + pub_FuncMode, funcmode);
	}

	channelinfo = get_long(puaebase + pub_ChannelInfo);
	if (channelinfo) {
		int i, ch;
		ch = get_word(channelinfo + ahieci_Channels);
		if (ch > UAE_MAXCHANNELS)
			ch = UAE_MAXCHANNELS;
		for (i = 0; i < ch; i++) {
			struct dschannel* dc = &dsahip->channel[i];
			int v;
			alGetSourcei(dc->al_source, AL_SAMPLE_OFFSET, &v);
			put_long(channelinfo + ahieci_Offset + i * 4, v + dc->samplecounter * dc->maxplaysamples);
		}
	}

	uae_Signal(task, signalmask);
	return 1;
}

static int setchannelevent(struct DSAHI* dsahip, struct dschannel* dc)
{
	uae_u32 audioctrl = dsahip->audioctrl;
	uae_u32 puaebase = get_long(audioctrl + ahiac_DriverData);
	int ch = dc - &dsahip->channel[0];
	uae_u32 mask;

	if (!dsahip->playing || ahi_paused || !dc->al_source || !get_long(audioctrl + ahiac_SoundFunc))
		return 0;
	mask = get_long(puaebase + pub_ChannelSignal);
	if (mask & (1 << ch))
		return 0;
	dc->channelsignal = 1;
	put_long(puaebase + pub_ChannelSignal, mask | (1 << ch));
	sendsignal(dsahip);
	return 1;
}

static void evtfunc(uae_u32 v)
{
	if (ahi_active) {
		struct DSAHI* dsahip = &dsahi[v];
		uae_u32 audioctrl = dsahip->audioctrl;
		uae_u32 puaebase = get_long(audioctrl + ahiac_DriverData);

		put_word(puaebase + pub_FuncMode, get_word(puaebase + pub_FuncMode) | FUNCMODE_PLAY);
		if (sendsignal(dsahip)) {
			event2_newevent2(dsahip->evttime, v, evtfunc);
		}
		else {
			dsahip->evttime = 0;
		}
	}
}

static void setevent(struct DSAHI* dsahip)
{
	uae_u32 audioctrl = dsahip->audioctrl;
	uae_u32 freq = get_long(audioctrl + ahiac_PlayerFreq);
	double f;
	uae_u32 cycles;
	evt t;

	f = ((double)(freq >> 16)) + ((double)(freq & 0xffff)) / 65536.0;
	if (f < 1)
		return;
	cycles = maxhpos * maxvpos_nom * vblank_hz;
	t = (evt)(cycles / f);
	if (dsahip->evttime == t)
		return;
	write_log(_T("AHI: playerfunc freq = %.2fHz\n"), f);
	dsahip->evttime = t;
	if (t < 10)
		return;
	event2_newevent2(t, dsahip - &dsahi[0], evtfunc);
}

static void alClear(void)
{
	//alGetError();
}
static int alError(const TCHAR* format, ...)
{
	TCHAR buffer[1000];
	va_list parms;
	int err;

	err = alGetError();
	if (err == AL_NO_ERROR)
		return 0;
	va_start(parms, format);
	_vsntprintf(buffer, sizeof buffer - 1, format, parms);
	_stprintf(buffer + _tcslen(buffer), _T(": ERR=%x\n"), err);
	write_log(_T("%s"), buffer);
	return err;
}

static void ds_freechannel(struct DSAHI* ahidsp, struct dschannel* dc)
{
	if (!dc)
		return;
	alDeleteSources(1, &dc->al_source);
	memset(dc, 0, sizeof(struct dschannel));
	dc->al_source = -1;
}

static void ds_freesample(struct DSAHI* ahidsp, struct dssample* ds)
{
	if (!ds)
		return;
	alDeleteBuffers(2, ds->al_buffer);
	memset(ds, 0, sizeof(struct dssample));
	ds->al_buffer[0] = -1;
	ds->al_buffer[1] = -1;
}

static void ds_free(struct DSAHI* dsahip)
{
	int i;

	if (!ahi_active)
		return;
	for (i = 0; i < dsahip->channels; i++) {
		struct dschannel* dc = &dsahip->channel[i];
		ds_freechannel(dsahip, dc);
	}
	for (i = 0; i < dsahip->sounds; i++) {
		struct dssample* ds = &dsahip->sample[i];
		ds_freesample(dsahip, ds);
	}
	alcMakeContextCurrent(NULL);
	alcDestroyContext(dsahip->al_ctx);
	dsahip->al_ctx = 0;
	SDL_CloseAudioDevice(dsahip->al_dev);
	dsahip->al_dev = 0;
	if (ahi_debug && ahi_active)
		write_log(_T("AHI: OpenAL freed\n"));
	ahi_active = 0;
}

static void ds_free_record(struct DSAHI* dsahip)
{
	if (dsahip->al_recorddev)
		SDL_CloseAudioDevice(dsahip->al_recorddev);
	dsahip->al_recorddev = NULL;
}

static int ds_init_record(struct DSAHI* dsahip)
{
//	uae_u32 pbase = get_long(dsahip->audioctrl + ahiac_DriverData);
//	int freq = get_long(dsahip->audioctrl + ahiac_MixFreq);
//	struct sound_device** sd;
//	int device, cnt;
//	char* s;
//
//	if (!freq)
//		return 0;
//	device = dsahip->input;
//	sd = record_devices;
//	cnt = 0;
//	for (;;) {
//		if (sd[cnt] && sd[cnt]->type == SOUND_DEVICE_AL) {
//			if (device <= 0)
//				break;
//			device--;
//		}
//		cnt++;
//		if (sd[cnt] == NULL)
//			return 0;
//	}
//	dsahip->record_samples = UAE_RECORDSAMPLES;
//	dsahip->record_ch = 2;
//	dsahip->record_bytespersample = 2;
//	alClear();
//	s = ua(sd[cnt]->alname);
//	dsahip->al_recorddev = alcCaptureOpenDevice(s, freq, AL_FORMAT_STEREO16, dsahip->record_samples);
//	xfree(s);
//	if (dsahip->al_recorddev == NULL)
//		goto error;
//	return 1;
//error:
	if (ahi_debug)
		write_log(_T("AHI: OPENAL recording initialization failed\n"));
	return 0;
}

static int ds_init(struct DSAHI* dsahip)
{
	int freq = 44100;
	int v;
	struct sound_device** sd;
	int device;
	char* s;
	int cnt;

	device = dsahip->output;
	sd = sound_devices;
	cnt = 0;
	for (;;) {
		if (sd[cnt] && sd[cnt]->type == SOUND_DEVICE_AL) {
			if (device <= 0)
				break;
			device--;
		}
		cnt++;
		if (sd[cnt] == NULL)
			return 0;
	}
	s = ua(sd[cnt]->alname);
	dsahip->al_dev = alcOpenDevice(s);
	xfree(s);
	if (!dsahip->al_dev)
		goto error;
	dsahip->al_ctx = alcCreateContext(dsahip->al_dev, NULL);
	if (!dsahip->al_ctx)
		goto error;
	alcMakeContextCurrent(dsahip->al_ctx);

	dsahip->chout = 2;
	dsahip->al_bufferformat = AL_FORMAT_STEREO16;
	cansurround = 0;
	if ((dsahip->audioid & 0xff) == 2) {
		if (v = alGetEnumValue("AL_FORMAT_QUAD16")) {
			dsahip->chout = 4;
			cansurround = 1;
			dsahip->al_bufferformat = v;
		}
		if (v = alGetEnumValue("AL_FORMAT_51CHN16")) {
			dsahip->chout = 6;
			cansurround = 1;
			dsahip->al_bufferformat = v;
		}
	}
	dsahip->bitspersampleout = dsahip->bits24 ? 24 : 16;
	dsahip->bytespersampleout = dsahip->bitspersampleout / 8;
	dsahip->channellength = 65536 * dsahip->chout * dsahip->bytespersampleout;
	if (ahi_debug)
		write_log(_T("AHI: CH=%d BLEN=%d\n"),
			dsahip->chout, dsahip->channellength);

	dsahip->tmpbuffer_size = 1000000;
	dsahip->tmpbuffer = xmalloc(uae_u8, dsahip->tmpbuffer_size);
	if (ahi_debug)
		write_log(_T("AHI: OpenAL initialized: %s\n"), sound_devices[dsahip->output]->name);

	return 1;
error:
	if (ahi_debug)
		write_log(_T("AHI: OpenAL initialization failed\n"));
	ds_free(dsahip);
	return 0;
}

static int ds_reinit(struct DSAHI* dsahip)
{
	ds_free(dsahip);
	return ds_init(dsahip);
}

static void ds_setvolume(struct DSAHI* dsahip, struct dschannel* dc)
{
	if (dc->al_source != -1) {
		if (abs(dc->cs.volume) != abs(dc->csnext.volume)) {
			float vol = ((float)(abs(dc->csnext.volume))) / 65536.0;
			alClear();
			alSourcef(dc->al_source, AL_GAIN, vol);
			alError(_T("AHI: SetVolume(%d,%d)"), dc->num, vol);
		}
		if (abs(dc->cs.panning) != abs(dc->csnext.panning)) {
			;//	    pan = (abs (dc->csnext.panning) - 0x8000) * DSBPAN_RIGHT / 32768;
		}
	}
	dc->cs.volume = dc->csnext.volume;
	dc->cs.panning = dc->csnext.panning;
}

static void ds_setfreq(struct DSAHI* dsahip, struct dschannel* dc)
{
	if (dc->dsplaying && dc->cs.frequency != dc->csnext.frequency && dc->csnext.frequency > 0 && dc->al_source != -1) {
		//alClear ();
		//alSourcei (dc->al_source, AL_FREQUENCY, dc->csnext.frequency);
		//alError (_T("AHI: SetFrequency(%d,%d)"), dc->num, dc->csnext.frequency);
	}
	dc->cs.frequency = dc->csnext.frequency;
}

static int ds_allocchannel(struct DSAHI* dsahip, struct dschannel* dc)
{
	if (dc->al_source != -1)
		return 1;
	alClear();
	alGenSources(1, &dc->al_source);
	if (alError(_T("alGenSources()")))
		goto error;
	dc->cs.frequency = -1;
	dc->cs.volume = -1;
	dc->cs.panning = -1;
	ds_setvolume(dsahip, dc);
	ds_setfreq(dsahip, dc);
	if (ahi_debug)
		write_log(_T("AHI: allocated OpenAL source for channel %d. vol=%d pan=%d freq=%d\n"),
			dc->num, dc->cs.volume, dc->cs.panning, dc->cs.frequency);
	return 1;
error:
	ds_freechannel(dsahip, dc);
	return 0;
}

#define MAKEXCH makexch (dsahip, dc, dst, i, och2, l, r)

STATIC_INLINE void makexch(struct DSAHI* dsahip, struct dschannel* dc, uae_u8* dst, int idx, int och2, uae_s32 l, uae_s32 r)
{
	if (dsahip->bits24) {
	}
	else {
		uae_s16* dst2 = (uae_s16*)(&dst[idx * och2]);
		l >>= 8;
		r >>= 8;
		if (dc->cs.volume < 0) {
			l = -l;
			r = -r;
		}
		dst2[0] = l;
		dst2[1] = r;
		if (dsahip->chout <= 2)
			return;
		dst2[4] = dst2[0];
		dst2[5] = dst2[1];
		if (dc->cs.panning < 0) {
			// surround only
			dst2[2] = 0; // center
			dst2[3] = (dst2[0] + dst2[1]) / 4; // lfe
			dst2[0] = dst2[1] = 0;
			return;
		}
		dst2[2] = dst2[3] = (dst2[0] + dst2[1]) / 4;
		if (dsahip->chout <= 6)
			return;
		dst2[6] = dst2[4];
		dst2[7] = dst2[5];
	}
}

/* sample conversion routines */
static int copysampledata(struct DSAHI* dsahip, struct dschannel* dc, struct dssample* ds, uae_u8** psrcp, uae_u8* srce, uae_u8* srcp, void* dstp, int dstlen)
{
	int i;
	uae_u8* src = *psrcp;
	uae_u8* dst = (uae_u8*)dstp;
	int och = dsahip->chout;
	int och2 = och * 2;
	int ich = ds->ch;
	int len;

	len = dstlen;
	switch (ds->sampletype)
	{
	case AHIST_M8S:
		for (i = 0; i < len; i++) {
			uae_u32 l = (src[0] << 16) | (src[0] << 8) | src[0];
			uae_u32 r = (src[0] << 16) | (src[0] << 8) | src[0];
			src += 1;
			if (src >= srce)
				src = srcp;
			MAKEXCH;
		}
		break;
	case AHIST_S8S:
		for (i = 0; i < len; i++) {
			uae_u32 l = (src[0] << 16) | (src[0] << 8) | src[0];
			uae_u32 r = (src[1] << 16) | (src[1] << 8) | src[1];
			src += 2;
			if (src >= srce)
				src = srcp;
			MAKEXCH;
		}
		break;
	case AHIST_M16S:
		for (i = 0; i < len; i++) {
			uae_u32 l = (src[0] << 16) | (src[1] << 8) | src[1];
			uae_u32 r = (src[0] << 16) | (src[1] << 8) | src[1];
			src += 2;
			if (src >= srce)
				src = srcp;
			MAKEXCH;
		}
		break;
	case AHIST_S16S:
		for (i = 0; i < len; i++) {
			uae_u32 l = (src[0] << 16) | (src[1] << 8) | src[1];
			uae_u32 r = (src[2] << 16) | (src[3] << 8) | src[3];
			src += 4;
			if (src >= srce)
				src = srcp;
			MAKEXCH;
		}
		break;
	case AHIST_M32S:
		for (i = 0; i < len; i++) {
			uae_u32 l = (src[3] << 16) | (src[2] << 8) | src[1];
			uae_u32 r = (src[3] << 16) | (src[2] << 8) | src[1];
			src += 4;
			if (src >= srce)
				src = srcp;
			MAKEXCH;
		}
		break;
	case AHIST_S32S:
		for (i = 0; i < len; i++) {
			uae_u32 l = (src[3] << 16) | (src[2] << 8) | src[1];
			uae_u32 r = (src[7] << 16) | (src[6] << 8) | src[5];
			src += 8;
			if (src >= srce)
				src = srcp;
			MAKEXCH;
		}
		break;
	case AHIST_L7_1:
		if (och == 8) {
			for (i = 0; i < len; i++) {
				if (dsahip->bits24) {
					uae_u32 fl = (src[0 * 4 + 3] << 16) | (src[0 * 4 + 2] << 8) | src[0 * 4 + 1];
					uae_u32 fr = (src[1 * 4 + 3] << 16) | (src[1 * 4 + 2] << 8) | src[1 * 4 + 1];
					uae_u32 cc = (src[6 * 4 + 3] << 16) | (src[6 * 4 + 2] << 8) | src[6 * 4 + 1];
					uae_u32 lf = (src[7 * 4 + 3] << 16) | (src[7 * 4 + 2] << 8) | src[7 * 4 + 1];
					uae_u32 bl = (src[2 * 4 + 3] << 16) | (src[2 * 4 + 2] << 8) | src[2 * 4 + 1];
					uae_u32 br = (src[3 * 4 + 3] << 16) | (src[3 * 4 + 2] << 8) | src[3 * 4 + 1];
					uae_u32 sl = (src[4 * 4 + 3] << 16) | (src[4 * 4 + 2] << 8) | src[4 * 4 + 1];
					uae_u32 sr = (src[5 * 4 + 3] << 16) | (src[5 * 4 + 2] << 8) | src[5 * 4 + 1];
					uae_s32* dst2 = (uae_s32*)(&dst[i * och2]);
					dst2[0] = fl;
					dst2[1] = fr;
					dst2[2] = cc;
					dst2[3] = lf;
					dst2[4] = bl;
					dst2[5] = br;
					dst2[6] = sl;
					dst2[7] = sr;
				}
				else {
					uae_u16 fl = (src[0 * 4 + 3] << 8) | src[0 * 4 + 2];
					uae_u16 fr = (src[1 * 4 + 3] << 8) | src[1 * 4 + 2];
					uae_u16 cc = (src[6 * 4 + 3] << 8) | src[6 * 4 + 2];
					uae_u16 lf = (src[7 * 4 + 3] << 8) | src[7 * 4 + 2];
					uae_u16 bl = (src[2 * 4 + 3] << 8) | src[2 * 4 + 2];
					uae_u16 br = (src[3 * 4 + 3] << 8) | src[3 * 4 + 2];
					uae_u16 sl = (src[4 * 4 + 3] << 8) | src[4 * 4 + 2];
					uae_u16 sr = (src[5 * 4 + 3] << 8) | src[5 * 4 + 2];
					uae_s16* dst2 = (uae_s16*)(&dst[i * och2]);
					dst2[0] = fl;
					dst2[1] = fr;
					dst2[2] = cc;
					dst2[3] = lf;
					dst2[4] = bl;
					dst2[5] = br;
					dst2[6] = sl;
					dst2[7] = sr;
				}
				dst += och2;
				src += 8 * 4;
				if (src >= srce)
					src = srcp;
			}
		}
		else if (och == 6) { /* 7.1 -> 5.1 */
			for (i = 0; i < len; i++) {
				if (dsahip->bits24) {
					printf("WARNING: dsahip->bits24 code does not do anything\n");
				}
				else {
					uae_s16* dst2 = (uae_s16*)(&dst[i * och2]);
					uae_u16 fl = (src[0 * 4 + 3] << 8) | src[0 * 4 + 2];
					uae_u16 fr = (src[1 * 4 + 3] << 8) | src[1 * 4 + 2];
					uae_u16 cc = (src[6 * 4 + 3] << 8) | src[6 * 4 + 2];
					uae_u16 lf = (src[7 * 4 + 3] << 8) | src[7 * 4 + 2];
					uae_u16 bl = (src[2 * 4 + 3] << 8) | src[2 * 4 + 2];
					uae_u16 br = (src[3 * 4 + 3] << 8) | src[3 * 4 + 2];
					uae_u16 sl = (src[4 * 4 + 3] << 8) | src[4 * 4 + 2];
					uae_u16 sr = (src[5 * 4 + 3] << 8) | src[5 * 4 + 2];
					dst2[0] = fl;
					dst2[1] = fr;
					dst2[2] = cc;
					dst2[3] = lf;
					dst2[4] = (bl + sl) / 2;
					dst2[5] = (br + sr) / 2;
				}
				dst += och2;
				src += 8 * 4;
				if (src >= srce)
					src = srcp;
			}
		}
		break;
	}
	*psrcp = src;
	return dstlen * och2;
}

static void dorecord(struct DSAHI* dsahip)
{
	uae_u32 pbase = get_long(dsahip->audioctrl + ahiac_DriverData);
	uae_u32 recordbuf;
	int bytes;

	if (dsahip->al_recorddev == NULL)
		return;
	if (dsahip->record_wait && !get_word(pbase + pub_RecordHookDone))
		return;
	dsahip->record_wait = 0;
	bytes = dsahip->record_samples * dsahip->record_ch * dsahip->record_bytespersample;
	recordbuf = get_long(pbase + pub_RecordBuffer);
	if (recordbuf == 0 || !valid_address(recordbuf, bytes))
		return;
	alClear();
	alcCaptureSamples(dsahip->al_recorddev, (void*)recordbuf, dsahip->record_samples);
	if (alGetError() != AL_NO_ERROR)
		return;
	put_word(pbase + pub_RecordHookDone, 0);
	dsahip->record_wait = 1;
	put_word(pbase + pub_FuncMode, get_word(pbase + pub_FuncMode) | FUNCMODE_RECORD);
	sendsignal(dsahip);
}


static void al_setloop(struct dschannel* dc, int state)
{
	alClear();
	alSourcei(dc->al_source, AL_LOOPING, state ? AL_TRUE : AL_FALSE);
	alError(_T("AHI: ds_play() alSourcei(AL_LOOPING)"));
}

static void al_startplay(struct dschannel* dc)
{
	alClear();
	alSourcePlay(dc->al_source);
	alError(_T("AHI: ds_play() alSourcePlay"));
}

static void preparesample_single(struct DSAHI* dsahip, struct dschannel* dc)
{
	uae_u8* p, * ps, * pe;
	struct dssample* ds;
	int slen, dlen;

	dc->samplecounter = -1;
	dc->buffertoggle = 0;

	ds = dc->cs.ds;
	ps = p = get_real_address(ds->addr);
	pe = ps + ds->len * ds->bytespersample * ds->ch;

	slen = ds->len;
	p += dc->cs.srcplayoffset * ds->bytespersample * ds->ch;
	dlen = copysampledata(dsahip, dc, ds, &p, pe, ps, dsahip->tmpbuffer, slen);
	alClear();
	alBufferData(ds->al_buffer[dc->buffertoggle], dsahip->al_bufferformat, dsahip->tmpbuffer, dlen, dc->cs.frequency);
	alError(_T("AHI: preparesample_single:alBufferData(len=%d,freq=%d)"), dlen, dc->cs.frequency);
	alClear();
	alSourceQueueBuffers(dc->al_source, 1, &ds->al_buffer[dc->buffertoggle]);
	alError(_T("AHI: al_initsample_single:alSourceQueueBuffers(freq=%d)"), dc->cs.frequency);
	if (ahi_debug > 2)
		write_log(_T("AHI: sample queued %d: %d/%d\n"),
			dc->num, dc->samplecounter, dc->totalsamples);
}


static void preparesample_multi(struct DSAHI* dsahip, struct dschannel* dc)
{
	uae_u8* p, * ps, * pe;
	struct dssample* ds;
	int slen, dlen;

	ds = dc->cs.ds;
	ps = p = get_real_address(ds->addr);
	pe = ps + ds->len * ds->bytespersample * ds->ch;

	slen = dc->maxplaysamples;
	if (dc->samplecounter == dc->totalsamples - 1)
		slen = ds->len - dc->maxplaysamples * (dc->totalsamples - 1);

	p += (dc->maxplaysamples * dc->samplecounter + dc->cs.srcplayoffset) * ds->bytespersample * ds->ch;
	dlen = copysampledata(dsahip, dc, ds, &p, pe, ps, dsahip->tmpbuffer, slen);
	alClear();
	alBufferData(ds->al_buffer[dc->buffertoggle], dsahip->al_bufferformat, dsahip->tmpbuffer, dlen, dc->cs.frequency);
	alError(_T("AHI: preparesample:alBufferData(len=%d,freq=%d)"), dlen, dc->cs.frequency);
	alClear();
	alSourceQueueBuffers(dc->al_source, 1, &ds->al_buffer[dc->buffertoggle]);
	alError(_T("AHI: al_initsample:alSourceQueueBuffers(freq=%d)"), dc->cs.frequency);
	if (ahi_debug > 2)
		write_log(_T("AHI: sample queued %d: %d/%d\n"),
			dc->num, dc->samplecounter, dc->totalsamples);
	dc->samplecounter++;
	dc->buffertoggle ^= 1;
}

/* called when sample is started for the first time */
static void al_initsample(struct DSAHI* dsahip, struct dschannel* dc)
{
	uae_u32 audioctrl = dsahip->audioctrl;
	struct dssample* ds;
	int single = 0;

	alSourceStop(dc->al_source);
	alClear();
	alSourcei(dc->al_source, AL_BUFFER, AL_NONE);
	alError(_T("AHI: al_initsample:AL_BUFFER=AL_NONE"));

	memcpy(&dc->cs, &dc->csnext, sizeof(struct chsample));
	dc->csnext.ds = NULL;
	dc->waitforack = 0;
	ds = dc->cs.ds;
	if (ds == NULL)
		return;

	if (get_long(audioctrl + ahiac_SoundFunc)) {
		dc->samplecounter = 0;
		if (ds->dynamic) {
			dc->maxplaysamples = dsahip->maxplaysamples / 2;
			if (dc->maxplaysamples > ds->len / 2)
				dc->maxplaysamples = ds->len / 2;
		}
		else {
			dc->maxplaysamples = ds->len / 2;
		}
		if (dc->maxplaysamples > dsahip->tmpbuffer_size)
			dc->maxplaysamples = dsahip->tmpbuffer_size;

		dc->totalsamples = ds->len / dc->maxplaysamples;
		if (dc->totalsamples <= 1)
			dc->totalsamples = 2;
		/* queue first half */
		preparesample_multi(dsahip, dc);
		/* queue second half */
		preparesample_multi(dsahip, dc);
	}
	else {
		single = 1;
		preparesample_single(dsahip, dc);
	}
	al_setloop(dc, single);

	if (dc->dsplaying) {
		dc->dsplaying = 1;
		al_startplay(dc);
		setchannelevent(dsahip, dc);
	}
}

/* called when previous sample is still playing */
static void al_queuesample(struct DSAHI* dsahip, struct dschannel* dc)
{
	int v, restart;

	if (!dc->cs.ds)
		return;
	if (dc->cs.ds->num < 0) {
		dc->cs.ds = NULL;
		return;
	}
	restart = 0;
	if (dc->dsplaying) {
		alClear();
		alGetSourcei(dc->al_source, AL_SOURCE_STATE, &v);
		alError(_T("AHI: queuesample AL_SOURCE_STATE"));
		if (v != AL_PLAYING) {
			alClear();
			alSourceRewind(dc->al_source);
			alError(_T("AHI: queuesample:restart"));
			restart = 1;
			if (ahi_debug > 2)
				write_log(_T("AHI: queuesample, play restart\n"));
			preparesample_multi(dsahip, dc);
		}
	}
	preparesample_multi(dsahip, dc);
	if (dc->dsplaying)
		dc->dsplaying = 1;
	if (restart)
		al_startplay(dc);
	if (ahi_debug > 2)
		write_log(_T("AHI: sample %d queued to channel %d\n"), dc->cs.ds->num, dc->num);
}

static int unqueuebuffers(struct dschannel* dc)
{
	int v, cnt = 0;
	for (;;) {
		uae_u32 tmp;
		alClear();
		alGetSourcei(dc->al_source, AL_BUFFERS_PROCESSED, &v);
		if (alError(_T("AHI: hsync AL_BUFFERS_PROCESSED %d"), dc->num))
			return cnt;
		if (v == 0)
			return cnt;
		alSourceUnqueueBuffers(dc->al_source, 1, &tmp);
		cnt++;
	}
}

void ahi_hsync(void)
{
	struct DSAHI* dsahip = &dsahi[0];
	static int cnt;
	uae_u32 pbase;
	int i, flags;

	if (ahi_paused || !ahi_active)
		return;
	pbase = get_long(dsahip->audioctrl + ahiac_DriverData);
	if (cnt >= 0)
		cnt--;
	if (cnt < 0) {
		if (dsahip->dsrecording && dsahip->enabledisable == 0) {
			dorecord(dsahip);
			cnt = 100;
		}
	}
	if (!dsahip->playing)
		return;
	flags = get_long(pbase + pub_ChannelSignalAck);
	for (i = 0; i < UAE_MAXCHANNELS; i++) {
		int v, removed;
		struct dschannel* dc = &dsahip->channel[i];
		uae_u32 mask = 1 << (dc - &dsahip->channel[0]);

		if (dc->dsplaying != 1 || dc->al_source == -1)
			continue;

		removed = unqueuebuffers(dc);
		v = 0;
		alClear();
		alGetSourcei(dc->al_source, AL_SOURCE_STATE, &v);
		alError(_T("AHI: hsync AL_SOURCE_STATE"));
		if (v != AL_PLAYING) {
			if (dc->cs.ds) {
				setchannelevent(dsahip, dc);
				if (ahi_debug)
					write_log(_T("AHI: ********* channel %d stopped state=%d!\n"), dc->num, v);
				removed = 1;
				dc->dsplaying = 2;
				dc->waitforack = 0;
			}
		}
		if (!dc->waitforack && dc->samplecounter >= 0 && removed) {
			int evt = 0;
			if (ahi_debug > 2)
				write_log(_T("sample end channel %d: %d/%d\n"), dc->num, dc->samplecounter, dc->totalsamples);
			if (dc->samplecounter >= dc->totalsamples) {
				evt = 1;
				if (ahi_debug > 2)
					write_log(_T("sample finished channel %d: %d\n"), dc->num, dc->totalsamples);
				dc->samplecounter = 0;
				if (dc->csnext.ds) {
					memcpy(&dc->cs, &dc->csnext, sizeof(struct chsample));
					dc->csnext.ds = NULL;
				}
			}
			if (evt) {
				flags &= ~mask;
				if (setchannelevent(dsahip, dc))
					dc->waitforack = 1;
			}
			if (!dc->waitforack)
				al_queuesample(dsahip, dc);
		}
		if (dc->waitforack && (flags & mask)) {
			al_queuesample(dsahip, dc);
			dc->waitforack = 0;
			flags &= ~mask;
		}
	}
	put_long(pbase + pub_ChannelSignalAck, flags);
}

static void ds_record(struct DSAHI* dsahip, int start)
{
	alClear();
	if (start) {
		if (!dsahip->dsrecording)
			alcCaptureStart(dsahip->al_recorddev);
		dsahip->dsrecording = 1;
	}
	else {
		alcCaptureStop(dsahip->al_recorddev);
		dsahip->dsrecording = 0;
	}
	alError(_T("AHI: alcCapture%s failed"), start ? "Start" : "Stop");
}

static void ds_stop(struct DSAHI* dsahip, struct dschannel* dc)
{
	dc->dsplaying = 0;
	if (dc->al_source == -1)
		return;
	if (ahi_debug)
		write_log(_T("AHI: ds_stop(%d)\n"), dc->num);
	alClear();
	alSourceStop(dc->al_source);
	alError(_T("AHI: alSourceStop"));
	unqueuebuffers(dc);
}

static void ds_play(struct DSAHI* dsahip, struct dschannel* dc)
{
	if (dc->dsplaying) {
		dc->dsplaying = 1;
		return;
	}
	dc->dsplaying = 1;
	if (dc->cs.frequency == 0)
		return;
	if (dc->al_source == -1)
		return;
	if (ahi_debug)
		write_log(_T("AHI: ds_play(%d)\n"), dc->num);
	al_startplay(dc);
}

void ahi2_pause_sound(int paused)
{
	int i;
	struct DSAHI* dsahip = &dsahi[0];

	ahi_paused = paused;
	if (!dsahip->playing && !dsahip->recording)
		return;
	for (i = 0; i < UAE_MAXCHANNELS; i++) {
		struct dschannel* dc = &dsahip->channel[i];
		if (dc->al_source == -1)
			continue;
		if (paused) {
			ds_stop(dsahip, dc);
		}
		else {
			ds_play(dsahip, dc);
			setchannelevent(dsahip, dc);
		}
	}
}

static uae_u32 init(TrapContext* ctx)
{
	int j;

	enumerate_sound_devices();
	xahi_author = ds(_T("Toni Wilen"));
	xahi_copyright = ds(_T("GPL"));
	xahi_version = ds(_T("uae2 0.2 (xx.xx.2008)\r\n"));
	j = 0;
	for (i = 0; i < MAX_SOUND_DEVICES && sound_devices[i]; i++) {
		if (sound_devices[i]->type == SOUND_DEVICE_AL)
			xahi_output[j++] = ds(sound_devices[i]->name);
	}
	xahi_output_num = j;
	j = 0;
	for (i = 0; i < MAX_SOUND_DEVICES && record_devices[i]; i++) {
		if (record_devices[i]->type == SOUND_DEVICE_AL)
			xahi_input[j++] = ds(record_devices[i]->name);
	}
	xahi_input_num = j;
	return 1;
}

static uae_u32 AHIsub_AllocAudio(TrapContext* ctx)
{
	int i;
	uae_u32 tags = m68k_areg(regs, 1);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	uae_u32 pbase = get_long(audioctrl + ahiac_DriverData);
	uae_u32 tag, data, v, ver, size;
	uae_u32 ret = AHISF_KNOWSTEREO | AHISF_KNOWHIFI;
	struct DSAHI* dsahip = &dsahi[0];

	if (ahi_debug)
		write_log(_T("AHI: AllocAudio(%08x,%08x)\n"), tags, audioctrl);

	ver = get_long(pbase + pub_Version);
	size = get_long(pbase + pub_SizeOf);
	if (ver != AHI_STRUCT_VERSION) {
		gui_message(_T("AHI: Incompatible DEVS:AHI/uae2.audio\nVersion mismatch %d<>%d."), ver, AHI_STRUCT_VERSION);
		return AHISF_ERROR;
	}
	if (size < pub_End) {
		gui_message(_T("AHI: Incompatible DEVS:AHI/uae2.audio.\nInternal structure size %d<>%d."), size, pub_End);
		return AHISF_ERROR;
	}

	v = get_long(pbase + pub_Index);
	if (v != -1) {
		write_log(_T("AHI: corrupted memory\n"));
		return AHISF_ERROR;
	}
	put_long(pbase + pub_Index, dsahip - dsahi);
	dsahip->audioctrl = audioctrl;

	dsahip->maxplaysamples = UAE_MAXPLAYSAMPLES;
	dsahip->sounds = UAE_MAXSOUNDS;
	dsahip->channels = UAE_MAXCHANNELS;

	dsahip->audioid = 0x003b0001;
	while ((tag = gettag(&tags, &data))) {
		if (ahi_debug)
			write_log(_T("- TAG %08x=%d: %08x=%u\n"), tag, tag & 0x7fff, data, data);
		switch (tag)
		{
		case AHIA_AudioID:
			dsahip->audioid = data;
			break;
		}
	}

	if ((dsahip->audioid >> 16) != 0x3b)
		return AHISF_ERROR;

	if (dsahip->sounds < 0 || dsahip->sounds > 1000)
		return AHISF_ERROR;

	if (!ds_init(dsahip))
		return AHISF_ERROR;

	if (xahi_input_num)
		ret |= AHISF_CANRECORD;
	if (cansurround)
		ret |= AHISF_KNOWMULTICHANNEL;

	dsahip->sample = xcalloc(struct dssample, dsahip->sounds);
	dsahip->channel = xcalloc(struct dschannel, dsahip->channels);
	for (i = 0; i < dsahip->channels; i++) {
		struct dschannel* dc = &dsahip->channel[i];
		dc->num = i;
		dc->al_source = -1;
	}
	for (i = 0; i < dsahip->sounds; i++) {
		struct dssample* ds = &dsahip->sample[i];
		ds->num = -1;
		ds->al_buffer[0] = -1;
		ds->al_buffer[1] = -1;
	}
	ahi_active = 1;
	return ret;
}

static void AHIsub_Disable(TrapContext* ctx)
{
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	if (ahi_debug > 1)
		write_log(_T("AHI: Disable(%08x)\n"), audioctrl);
	dsahip->enabledisable++;
}

static void AHIsub_Enable(TrapContext* ctx)
{
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	if (ahi_debug > 1)
		write_log(_T("AHI: Enable(%08x)\n"), audioctrl);
	dsahip->enabledisable--;
	if (dsahip->enabledisable == 0 && dsahip->playing)
		setevent(dsahip);
}

static void AHIsub_FreeAudio(TrapContext* ctx)
{
	uae_u32 audioctrl = m68k_areg(regs, 2);
	uae_u32 pbase = get_long(audioctrl + ahiac_DriverData);
	struct DSAHI* dsahip = GETAHI;
	if (ahi_debug)
		write_log(_T("AHI: FreeAudio(%08x)\n"), audioctrl);
	if (ahi_active == 0)
		return;
	ahi_active = 0;
	put_long(pbase + pub_Index, -1);
	if (dsahip) {
		ds_free(dsahip);
		ds_free_record(dsahip);
		xfree(dsahip->channel);
		xfree(dsahip->sample);
		memset(dsahip, 0, sizeof(struct DSAHI));
	}
}

static uae_u32 frequencies[] = { 48000, 44100 };
#define MAX_FREQUENCIES (sizeof (frequencies) / sizeof (uae_u32))

static uae_u32 getattr2(struct DSAHI* dsahip, uae_u32 attribute, uae_u32 argument, uae_u32 def)
{
	int i;

	switch (attribute)
	{
	case AHIDB_Bits:
		return 32;
	case AHIDB_Frequencies:
		return MAX_FREQUENCIES;
	case AHIDB_Frequency:
		if (argument < 0 || argument >= MAX_FREQUENCIES)
			argument = 0;
		return frequencies[argument];
	case AHIDB_Index:
		if (argument <= frequencies[0])
			return 0;
		if (argument >= frequencies[MAX_FREQUENCIES - 1])
			return MAX_FREQUENCIES - 1;
		for (i = 1; i < MAX_FREQUENCIES; i++) {
			if (frequencies[i] > argument) {
				if (argument - frequencies[i - 1] < frequencies[i] - argument)
					return i - 1;
				else
					return i;
			}
		}
		return 0;
	case AHIDB_Author:
		return xahi_author;
	case AHIDB_Copyright:
		return xahi_copyright;
	case AHIDB_Version:
		return xahi_version;
	case AHIDB_Record:
		return -1;
	case AHIDB_Realtime:
		return -1;
	case AHIDB_MinOutputVolume:
		return 0x00000;
	case AHIDB_MaxOutputVolume:
		return 0x10000;
	case AHIDB_Outputs:
		return xahi_output_num;
	case AHIDB_Output:
		if (argument >= 0 && argument < xahi_output_num)
			return xahi_output[argument];
		return 0;
	case AHIDB_Inputs:
		return xahi_input_num;
	case AHIDB_Input:
		if (argument >= 0 && argument < xahi_input_num)
			return xahi_input[argument];
		return 0;
	case AHIDB_Volume:
		return -1;
	case AHIDB_Panning:
		return -1;
	case AHIDB_HiFi:
		return -1;
	case AHIDB_MultiChannel:
		return cansurround ? -1 : 0;
	case AHIDB_MaxChannels:
		return UAE_MAXCHANNELS;
	case AHIDB_FullDuplex:
		return -1;
	case AHIDB_MaxRecordSamples:
		return UAE_RECORDSAMPLES;
	case AHIDB_MaxPlaySamples:
		if (def < dsahip->maxplaysamples)
			def = dsahip->maxplaysamples;
		return def;
	default:
		return def;
	}
}

static uae_u32 AHIsub_GetAttr(TrapContext* ctx)
{
	uae_u32 attribute = m68k_dreg(regs, 0);
	uae_u32 argument = m68k_dreg(regs, 1);
	uae_u32 def = m68k_dreg(regs, 2);
	// uae_u32 taglist = m68k_areg (regs, 1);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	uae_u32 v;

	v = getattr2(dsahip, attribute, argument, def);
	if (ahi_debug)
		write_log(_T("AHI: GetAttr(%08x=%d,%08x,%08x)=%08x\n"), attribute, attribute & 0x7fff, argument, def, v);

	return v;
}

static uae_u32 AHIsub_HardwareControl(TrapContext* ctx)
{
	uae_u32 attribute = m68k_dreg(regs, 0);
	uae_u32 argument = m68k_dreg(regs, 1);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	if (ahi_debug)
		write_log(_T("AHI: HardwareControl(%08x=%d,%08x,%08x)\n"), attribute, attribute & 0x7fff, argument, audioctrl);
	switch (attribute)
	{
	case AHIC_Input:
		if (dsahip->input != argument) {
			dsahip->input = argument;
			if (dsahip->al_dev) {
				ds_free_record(dsahip);
				ds_init_record(dsahip);
				if (dsahip->recording)
					ds_record(dsahip, 1);
			}
		}
		break;
	case AHIC_Input_Query:
		return dsahip->input;
	case AHIC_Output:
		if (dsahip->output != argument) {
			dsahip->output = argument;
			if (dsahip->al_dev)
				ds_reinit(dsahip);
		}
		break;
	case AHIC_Output_Query:
		return dsahip->output;
	}
	return 0;
}

static uae_u32 AHIsub_Start(TrapContext* ctx)
{
	uae_u32 flags = m68k_dreg(regs, 0);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	int i;

	if (ahi_debug)
		write_log(_T("AHI: Play(%08x,%08x)\n"),
			flags, audioctrl);
	if ((flags & AHISF_PLAY) && !dsahip->playing) {
		dsahip->playing = 1;
		setevent(dsahip);
		for (i = 0; i < dsahip->channels; i++) {
			struct dschannel* dc = &dsahip->channel[i];
			ds_play(dsahip, dc);
		}
	}
	if ((flags & AHISF_RECORD) && !dsahip->recording) {
		dsahip->recording = 1;
		ds_init_record(dsahip);
		ds_record(dsahip, 1);
	}
	return 0;
}

static uae_u32 AHIsub_Stop(TrapContext* ctx)
{
	uae_u32 flags = m68k_dreg(regs, 0);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	int i;

	if (ahi_debug)
		write_log(_T("AHI: Stop(%08x,%08x)\n"),
			flags, audioctrl);
	if ((flags & AHISF_PLAY) && dsahip->playing) {
		dsahip->playing = 0;
		for (i = 0; i < dsahip->channels; i++) {
			struct dschannel* dc = &dsahip->channel[i];
			ds_stop(dsahip, dc);
		}
	}
	if ((flags & AHISF_RECORD) && dsahip->recording) {
		dsahip->recording = 0;
		ds_record(dsahip, 0);
		ds_free_record(dsahip);
	}
	return 0;
}

static uae_u32 AHIsub_Update(TrapContext* ctx)
{
	uae_u32 flags = m68k_dreg(regs, 0);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	if (ahi_debug)
		write_log(_T("AHI: Update(%08x,%08x)\n"), flags, audioctrl);
	setevent(dsahip);
	return 0;
}

static uae_u32 AHIsub_SetVol(TrapContext* ctx)
{
	uae_u16 channel = m68k_dreg(regs, 0);
	uae_s32 volume = m68k_dreg(regs, 1);
	uae_s32 pan = m68k_dreg(regs, 2);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	uae_u32 flags = m68k_dreg(regs, 3);
	struct DSAHI* dsahip = GETAHI;
	struct dschannel* dc = GETCHANNEL;

	if (ahi_debug > 1)
		write_log(_T("AHI: SetVol(%d,%d,%d,%08x,%08x)\n"),
			channel, volume, pan, audioctrl, flags);
	if (dc) {
		if (volume < -65535)
			volume = -65535;
		if (volume > 65535)
			volume = 65535;
		if (pan < -65535)
			pan = -65535;
		if (pan > 65535)
			pan = 65535;
		dc->csnext.volume = volume;
		dc->csnext.panning = pan;
		if (flags & AHISF_IMM) {
			ds_setvolume(dsahip, dc);
		}
	}
	return 0;
}

static uae_u32 AHIsub_SetFreq(TrapContext* ctx)
{
	uae_u16 channel = m68k_dreg(regs, 0);
	uae_u32 frequency = m68k_dreg(regs, 1);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	uae_u32 flags = m68k_dreg(regs, 3);
	struct DSAHI* dsahip = GETAHI;
	struct dschannel* dc = GETCHANNEL;

	if (ahi_debug > 1)
		write_log(_T("AHI: SetFreq(%d,%d,%08x,%08x)\n"),
			channel, frequency, audioctrl, flags);
	if (dc) {
		dc->csnext.frequency = frequency;
		if (flags & AHISF_IMM) {
			ds_setfreq(dsahip, dc);
			ds_play(dsahip, dc);
		}
	}
	return 0;
}

static uae_u32 AHIsub_SetSound(TrapContext* ctx)
{
	uae_u16 channel = m68k_dreg(regs, 0);
	uae_u16 sound = m68k_dreg(regs, 1);
	uae_u32 offset = m68k_dreg(regs, 2);
	int length = m68k_dreg(regs, 3);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	uae_u32 flags = m68k_dreg(regs, 4);
	struct DSAHI* dsahip = GETAHI;
	struct dssample* ds = GETSAMPLE;
	struct dschannel* dc = GETCHANNEL;

	if (ahi_debug > 1)
		write_log(_T("AHI: SetSound(%d,%d,%08x,%d,%08x,%08x)\n"),
			channel, sound, offset, length, audioctrl, flags);
	if (dc == NULL)
		return AHIE_UNKNOWN;
	if (sound == 0xffff) {
		if (flags & AHISF_IMM) {
			dc->cs.ds = NULL;
			dc->csnext.ds = NULL;
		}
		return 0;
	}
	if (ds == NULL || ds->num < 0)
		return AHIE_UNKNOWN;
	ds_allocchannel(dsahip, dc);
	dc->cs.backwards = length < 0;
	length = abs(length);
	if (length == 0)
		length = ds->len;
	if (length > ds->len)
		length = ds->len;
	dc->csnext.ds = ds;
	dc->csnext.srcplaylen = length;
	dc->csnext.srcplayoffset = offset;
	if (flags & AHISF_IMM)
		dc->cs.ds = NULL;
	ds_setfreq(dsahip, dc);
	ds_setvolume(dsahip, dc);
	if (dc->cs.ds == NULL)
		al_initsample(dsahip, dc);
	return 0;
}

static uae_u32 AHIsub_SetEffect(TrapContext* ctx)
{
	uae_u32 effect = m68k_areg(regs, 0);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	uae_u32 effectype = get_long(effect);
	uae_u32 puaebase = get_long(audioctrl + ahiac_DriverData);

	if (ahi_debug)
		write_log(_T("AHI: SetEffect(%08x (%08x),%08x)\n"), effect, effectype, audioctrl);
	switch (effectype)
	{
	case AHIET_CHANNELINFO:
		put_long(puaebase + pub_ChannelInfo, effect);
		break;
	case static_cast<uae_u32>(AHIET_CHANNELINFO) | AHIET_CANCEL:
		put_long(puaebase + pub_ChannelInfo, 0);
		break;
	case AHIET_MASTERVOLUME:
		write_log(_T("AHI: SetEffect(MasterVolume=%08x)\n"), get_long(effect + 4));
	case static_cast<uae_u32>(AHIET_MASTERVOLUME) | AHIET_CANCEL:
		break;
	default:
		return AHIE_UNKNOWN;
	}
	return AHIE_OK;
}

static uae_u32 AHIsub_LoadSound(TrapContext* ctx)
{
	uae_u16 sound = m68k_dreg(regs, 0);
	uae_u32 type = m68k_dreg(regs, 1);
	uae_u32 info = m68k_areg(regs, 0);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	int sampletype = get_long(info + ahisi_Type);
	uae_u32 addr = get_long(info + ahisi_Address);
	uae_u32 len = get_long(info + ahisi_Length);
	struct dssample* ds = GETSAMPLE;
	int ch;
	int bps;

	if (ahi_debug > 1)
		write_log(_T("AHI: LoadSound(%d,%d,%08x,%08x,SMP=%d,ADDR=%08x,LEN=%d)\n"),
			sound, type, info, audioctrl, sampletype, addr, len);

	if (!ds)
		return AHIE_BADSOUNDTYPE;

	ds->num = sound;
	if (!cansurround && sampletype == AHIST_L7_1)
		return AHIE_BADSOUNDTYPE;
	ds->addr = addr;
	ds->sampletype = sampletype;
	ds->type = type;
	ds->len = len;
	ds->dynamic = type == AHIST_DYNAMICSAMPLE;

	switch (sampletype)
	{
	case AHIST_M8S:
	case AHIST_M16S:
	case AHIST_M32S:
		ch = 1;
		break;
	case AHIST_S8S:
	case AHIST_S16S:
	case AHIST_S32S:
		ch = 2;
		break;
	case AHIST_L7_1:
		ch = 8;
		break;
	default:
		return 0;
	}
	switch (sampletype)
	{
	case AHIST_M8S:
	case AHIST_S8S:
		bps = 8;
		break;
	case AHIST_M16S:
	case AHIST_S16S:
		bps = 16;
		break;
	case AHIST_M32S:
	case AHIST_S32S:
	case AHIST_L7_1:
		bps = 24;
		break;
	default:
		return 0;
	}
	ds->bitspersample = bps;
	ds->ch = ch;
	ds->bytespersample = bps / 8;
	if (ds->al_buffer[0] == -1) {
		alClear();
		alGenBuffers(2, ds->al_buffer);
		if (alError(_T("AHI: alGenBuffers")))
			return AHIE_NOMEM;
		if (ahi_debug > 1)
			write_log(_T("AHI:LoadSound:allocated OpenAL buffer\n"));
	}
	return AHIE_OK;
}

static uae_u32 AHIsub_UnloadSound(TrapContext* ctx)
{
	uae_u16 sound = m68k_dreg(regs, 0);
	uae_u32 audioctrl = m68k_areg(regs, 2);
	struct DSAHI* dsahip = GETAHI;
	struct dssample* ds = GETSAMPLE;

	if (ahi_debug > 1)
		write_log(_T("AHI: UnloadSound(%d,%08x)\n"),
			sound, audioctrl);
	ds->num = -1;
	return AHIE_OK;
}

static uae_u32 REGPARAM2 ahi_demux(TrapContext* ctx)
{
	uae_u32 ret = 0;
	uae_u32 sp = m68k_areg(regs, 7);
	uae_u32 offset = get_long(sp + 4);

	if (0 && ahi_debug)
		write_log(_T("AHI: %d\n"), offset);

	switch (offset)
	{
	case 0xffffffff:
		ret = init(ctx);
		break;
	case 0:
		ret = AHIsub_AllocAudio(ctx);
		break;
	case 1:
		AHIsub_FreeAudio(ctx);
		break;
	case 2:
		AHIsub_Disable(ctx);
		break;
	case 3:
		AHIsub_Enable(ctx);
		break;
	case 4:
		ret = AHIsub_Start(ctx);
		break;
	case 5:
		ret = AHIsub_Update(ctx);
		break;
	case 6:
		ret = AHIsub_Stop(ctx);
		break;
	case 7:
		ret = AHIsub_SetVol(ctx);
		break;
	case 8:
		ret = AHIsub_SetFreq(ctx);
		break;
	case 9:
		ret = AHIsub_SetSound(ctx);
		break;
	case 10:
		ret = AHIsub_SetEffect(ctx);
		break;
	case 11:
		ret = AHIsub_LoadSound(ctx);
		break;
	case 12:
		ret = AHIsub_UnloadSound(ctx);
		break;
	case 13:
		ret = AHIsub_GetAttr(ctx);
		break;
	case 14:
		ret = AHIsub_HardwareControl(ctx);
		break;
	}
	return ret;
}

void init_ahi_v2(void)
{
	write_log("installing ahi_avi\n");
	uaecptr a = here();
	org(rtarea_base + 0xFFC8);
	calltrap(deftrapres(ahi_demux, 0, _T("ahi_v2")));
	dw(RTS);
	org(a);
}

void free_ahi_v2(void)
{
	ds_free_record(&dsahi[0]);
	ds_free(&dsahi[0]);
}

#endif
#endif