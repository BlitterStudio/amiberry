/*
* UAE - The Un*x Amiga Emulator
*
* joystick/mouse emulation
*
* Copyright 2001-2016 Toni Wilen
*
* new features:
* - very configurable (and very complex to configure :)
* - supports multiple native input devices (joysticks and mice)
* - supports mapping joystick/mouse buttons to keys and vice versa
* - joystick mouse emulation (supports both ports)
* - supports parallel port joystick adapter
* - full cd32 pad support (supports both ports)
* - fully backward compatible with old joystick/mouse configuration
*
*/

#define DONGLE_DEBUG 0
#define SWITCH_DEBUG 0
#define INPUT_DEBUG 0
#define OUTPUTDEBUG 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "inputrecord.h"
#include "keybuf.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "memory.h"
#include "events.h"
#include "newcpu.h"
#include "uae.h"
#include "picasso96.h"
#ifdef CATWEASEL
#include "catweasel.h"
#endif
#include "debug.h"
#include "ar.h"
#include "gui.h"
#include "disk.h"
#include "audio.h"
#include "sounddep/sound.h"
#include "savestate.h"
#ifdef ARCADIA
#include "arcadia.h"
#endif
#include "zfile.h"
#include "cia.h"
#include "autoconf.h"
#ifdef WITH_X86
#include "x86.h"
#endif
#ifdef WITH_DRACO
#include "draco.h"
#endif
#ifdef RETROPLATFORM
#include "rp.h"
#endif
#include "dongle.h"
#include "cdtv.h"
#ifdef AVIOUTPUT
#include "avioutput.h"
#endif
#include "tabletlibrary.h"
#include "statusline.h"
#include "native2amiga_api.h"
//#include "videograb.h"
#ifdef AMIBERRY
#include "amiberry_gfx.h"
#include "amiberry_input.h"
#include "vkbd/vkbd.h"
#endif

// 01 = host events
// 02 = joystick
// 04 = cia buttons
// 16 = potgo r/w
// 32 = vsync
// 128 = potgo write
// 256 = cia buttons write

int inputdevice_logging = 0;
extern int tablet_log;

#define COMPA_RESERVED_FLAGS (ID_FLAG_INVERT)

#define ID_FLAG_CANRELEASE 0x1000
#define ID_FLAG_TOGGLED 0x2000
#define ID_FLAG_CUSTOMEVENT_TOGGLED1 0x4000
#define ID_FLAG_CUSTOMEVENT_TOGGLED2 0x8000

#define IE_INVERT 0x80
#define IE_CDTV 0x100

#define INPUTEVENT_JOY1_CD32_FIRST INPUTEVENT_JOY1_CD32_PLAY
#define INPUTEVENT_JOY2_CD32_FIRST INPUTEVENT_JOY2_CD32_PLAY
#define INPUTEVENT_JOY1_CD32_LAST INPUTEVENT_JOY1_CD32_BLUE
#define INPUTEVENT_JOY2_CD32_LAST INPUTEVENT_JOY2_CD32_BLUE

#define JOYMOUSE_CDTV 8

#define DEFEVENT(A, B, C, D, E, F) {_T(#A), B, NULL, C, D, E, F, 0 },
#define DEFEVENT2(A, B, B2, C, D, E, F, G) {_T(#A), B, B2, C, D, E, F, G },
#define DEFEVENTKB(A, B, C, F, PC) {_T(#A), B, NULL, C, 0, 0, F, 0, PC },
static const struct inputevent events[] = {
	{0, 0, 0, AM_K, 0, 0, 0, 0, 0},
#include "inputevents.def"
	{0, 0, 0, 0, 0, 0, 0, 0, 0}
};
#undef DEFEVENT
#undef DEFEVENT2
struct aksevent
{
	int aks;
	const TCHAR *name;
};
#define AKS(A) { AKS_ ## A, _T("AKS_") _T(#A) },
static const struct aksevent akss[] = {
#include "aks.def"
	{ 0, NULL }
};

static int sublevdir[2][MAX_INPUT_SUB_EVENT];

static const int slotorder1[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static const int slotorder2[] = { 8, 1, 2, 3, 4, 5, 6, 7 };

struct uae_input_device2 {
	uae_u32 buttonmask;
	int states[MAX_INPUT_DEVICE_EVENTS / 2][MAX_INPUT_SUB_EVENT + 1];
};

#define INPUT_MATCH_CONFIG_NAME_ONLY 1
#define INPUT_MATCH_FRIENDLY_NAME_ONLY 2
#define INPUT_MATCH_BOTH 4
#define INPUT_MATCH_ALL 7

static struct uae_input_device2 joysticks2[MAX_INPUT_DEVICES];
static struct uae_input_device2 mice2[MAX_INPUT_DEVICES];
static uae_u8 scancodeused[MAX_INPUT_DEVICES][256];
static uae_u64 qualifiers, qualifiers_r;
static uae_s16 *qualifiers_evt[MAX_INPUT_QUALIFIERS];

// fire/left mouse button pullup resistors enabled?
static bool mouse_pullup = true;

static int joymodes[MAX_JPORTS], joysubmodes[MAX_JPORTS];
static const int *joyinputs[MAX_JPORTS];

static int input_acquired;
static int testmode;
struct teststore
{
	int testmode_type;
	int testmode_num;
	int testmode_wtype;
	int testmode_wnum;
	int testmode_state;
	int testmode_max;
};
#define TESTMODE_MAX 2
static int testmode_count;
static struct teststore testmode_data[TESTMODE_MAX];
static struct teststore testmode_wait[TESTMODE_MAX];

static int bouncy;
static frame_time_t bouncy_cycles;
static int autopause;
static int keyboard_reset_seq, keyboard_reset_seq_mode;

#define HANDLE_IE_FLAG_CANSTOPPLAYBACK 1
#define HANDLE_IE_FLAG_PLAYBACKEVENT 2
#define HANDLE_IE_FLAG_AUTOFIRE 4
#define HANDLE_IE_FLAG_ABSOLUTE 8
#define HANDLE_IE_FLAG_ALLOWOPPOSITE 16

static int handle_input_event (int nr, int state, int max, int flags);

static struct inputdevice_functions idev[IDTYPE_MAX];

struct temp_uids {
	TCHAR *name;
	TCHAR *configname;
	uae_s8 disabled;
	uae_s8 empty;
	uae_s8 custom;
	int joystick;
	int devtype;
	int idnum;
	int devnum;
	bool initialized;
	int kbreventcnt[MAX_INPUT_DEVICES];
	int lastdevtype;
	int matcheddevices[MAX_INPUT_DEVICES];
	int nonmatcheddevices[MAX_INPUT_DEVICES];
};
static struct temp_uids temp_uid;
static int temp_uid_index[MAX_INPUT_DEVICES][IDTYPE_MAX];
static int temp_uid_cnt[IDTYPE_MAX];
static int gp_swappeddevices[MAX_INPUT_DEVICES][IDTYPE_MAX];

#ifdef WITH_DRACO
extern int draco_keyboard_get_rate(void);
static int draco_keybord_repeat_cnt, draco_keybord_repeat_code;
#endif

static int isdevice (struct uae_input_device *id)
{
	int i, j;
	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (id->eventid[i][j] > 0)
				return 1;
		}
	}
	return 0;
}

static void check_enable(int ei);

int inputdevice_geteventid(const TCHAR *s)
{
	for (int i = 1; events[i].name; i++) {
		const struct inputevent *ie = &events[i];
		if (!_tcscmp(ie->confname, s))
			return i;
	}
	for (int i = 0; akss[i].name; i++) {
		if (!_tcscmp(s, akss[i].name))
			return i + AKS_FIRST;
	}
	return 0;
}

int inputdevice_uaelib (const TCHAR *s, const TCHAR *parm)
{
	//write_log(_T("%s: %s\n"), s, parm);

	if (!_tcsncmp(s, _T("KEY_RAW_"), 8)) {
		// KEY_RAW_UP <code>
		// KEY_RAW_DOWN <code>
		int v;
		const TCHAR *value = parm;
		TCHAR *endptr;
		int base = 10;
		int state = _tcscmp(s, _T("KEY_RAW_UP")) ? 1 : 0;
		if (value[0] == '0' && _totupper(value[1]) == 'X')
			value += 2, base = 16;
		v = _tcstol(value, &endptr, base);
		for (int i = 1; events[i].name; i++) {
			const struct inputevent *ie = &events[i];
			if (_tcsncmp(ie->confname, _T("KEY_"), 4))
				continue;
			if (ie->data == v) {
				handle_input_event(i, state, 1, 0);
				return 1;
			}
		}
		return 0;
	}

	if (!_tcsncmp(s, _T("AKS_"), 4)) {
		for (int i = 0; akss[i].name; i++) {
			if (!_tcscmp(s, akss[i].name)) {
				int v = _tstol(parm);
				if (!_tcscmp(parm, _T("0"))) {
					v = SET_ONOFF_OFF_VALUE;
					parm = NULL;
				} else if(!_tcscmp(parm, _T("1"))) {
					v = SET_ONOFF_ON_VALUE;
					parm = NULL;
				} else {
					v = SET_ONOFF_PRESS_VALUE;
				}
				inputdevice_add_inputcode(akss[i].aks, v, parm);
				return 1;
			}
		}
	}

	for (int i = 1; events[i].name; i++) {
		if (!_tcscmp (s, events[i].confname)) {
			check_enable(i);
			handle_input_event (i, parm ? _tstol (parm) : 0, 1, 0);
			return 1;
		}
	}
	return 0;
}

int inputdevice_uaelib(const TCHAR *s, int parm, int max, bool autofire)
{
	for (int i = 1; events[i].name; i++) {
		if (!_tcscmp(s, events[i].confname)) {
			check_enable(i);
			handle_input_event(i, parm, max, autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0);
			return 1;
		}
	}
	return 0;
}

static int getdevnum(int type, int devnum)
{
	int jcnt = idev[IDTYPE_JOYSTICK].get_num();
	int mcnt = idev[IDTYPE_MOUSE].get_num();
	int kcnt = idev[IDTYPE_KEYBOARD].get_num();

	if (type == IDTYPE_JOYSTICK)
		return devnum;
	else if (type == IDTYPE_MOUSE)
		return jcnt + devnum;
	else if (type == IDTYPE_KEYBOARD)
		return jcnt + mcnt + devnum;
	else if (type == IDTYPE_INTERNALEVENT)
		return jcnt + mcnt + kcnt + devnum;
	return -1;
}

static int gettype(int devnum)
{
	int jcnt = idev[IDTYPE_JOYSTICK].get_num();
	int mcnt = idev[IDTYPE_MOUSE].get_num();
	int kcnt = idev[IDTYPE_KEYBOARD].get_num();

	if (devnum < jcnt)
		return IDTYPE_JOYSTICK;
	else if (devnum < jcnt + mcnt)
		return IDTYPE_MOUSE;
	else if (devnum < jcnt + mcnt + kcnt)
		return IDTYPE_KEYBOARD;
	else if (devnum < jcnt + mcnt + kcnt + INTERNALEVENT_COUNT)
		return IDTYPE_INTERNALEVENT;
	return -1;
}

static struct inputdevice_functions *getidf(int devnum)
{
	int type = gettype(devnum);
	if (type < 0)
		return NULL;
	return &idev[type];
}

const struct inputevent *inputdevice_get_eventinfo(int evt)
{
	if (evt > 0 && !events[evt].name)
		return NULL;
	return &events[evt];
}

static uae_u64 inputdevice_flag_to_idev(uae_u64 flag)
{
	uae_u64 flags = 0;
	if (flag & ID_FLAG_AUTOFIRE)
		flags |= IDEV_MAPPED_AUTOFIRE_SET;
	if (flag & ID_FLAG_TOGGLE)
		flags |= IDEV_MAPPED_TOGGLE;
	if (flag & ID_FLAG_INVERTTOGGLE)
		flags |= IDEV_MAPPED_INVERTTOGGLE;
	if (flag & ID_FLAG_INVERT)
		flags |= IDEV_MAPPED_INVERT;
	if (flag & ID_FLAG_GAMEPORTSCUSTOM1)
		flags |= IDEV_MAPPED_GAMEPORTSCUSTOM1;
	if (flag & ID_FLAG_GAMEPORTSCUSTOM2)
		flags |= IDEV_MAPPED_GAMEPORTSCUSTOM2;
	if (flag & ID_FLAG_QUALIFIER_MASK)
		flags |= flag & ID_FLAG_QUALIFIER_MASK;
	if (flag & ID_FLAG_SET_ONOFF)
		flags |= IDEV_MAPPED_SET_ONOFF;
	if (flag & ID_FLAG_SET_ONOFF_VAL1)
		flags |= IDEV_MAPPED_SET_ONOFF_VAL1;
	if (flag & ID_FLAG_SET_ONOFF_VAL2)
		flags |= IDEV_MAPPED_SET_ONOFF_VAL2;
	return flags;
}

static struct uae_input_device *joysticks;
static struct uae_input_device *mice;
static struct uae_input_device *keyboards;
static struct uae_input_device *internalevents;
static struct uae_input_device_kbr_default *keyboard_default, **keyboard_default_table;
static int default_keyboard_layout[MAX_JPORTS];

#define KBR_DEFAULT_MAP_FIRST 0
#ifdef AMIBERRY
#define KBR_DEFAULT_MAP_LAST 15
#define KBR_DEFAULT_MAP_CD32_FIRST 16
#define KBR_DEFAULT_MAP_CD32_LAST 23
#else
#define KBR_DEFAULT_MAP_LAST 5
#define KBR_DEFAULT_MAP_CD32_FIRST 6
#define KBR_DEFAULT_MAP_CD32_LAST 8
#endif

#define KBR_DEFAULT_MAP_NP 0
#define KBR_DEFAULT_MAP_CK 1
#define KBR_DEFAULT_MAP_SE 2
#define KBR_DEFAULT_MAP_NP3 3
#define KBR_DEFAULT_MAP_CK3 4
#define KBR_DEFAULT_MAP_SE3 5
#ifdef AMIBERRY
#define KBR_DEFAULT_MAP_KEYRAH 6
#define KBR_DEFAULT_MAP_KEYRAH3 7
#define KBR_DEFAULT_MAP_RAPLAYER1 8
#define KBR_DEFAULT_MAP_RAPLAYER1_3 9
#define KBR_DEFAULT_MAP_RAPLAYER2 10
#define KBR_DEFAULT_MAP_RAPLAYER2_3 11
#define KBR_DEFAULT_MAP_RAPLAYER3 12
#define KBR_DEFAULT_MAP_RAPLAYER3_3 13
#define KBR_DEFAULT_MAP_RAPLAYER4 14
#define KBR_DEFAULT_MAP_RAPLAYER4_3 15
#define KBR_DEFAULT_MAP_CD32_NP 16
#define KBR_DEFAULT_MAP_CD32_CK 17
#define KBR_DEFAULT_MAP_CD32_SE 18
#define KBR_DEFAULT_MAP_CD32_KEYRAH 19
#define KBR_DEFAULT_MAP_CD32_RAPLAYER1 20
#define KBR_DEFAULT_MAP_CD32_RAPLAYER2 21
#define KBR_DEFAULT_MAP_CD32_RAPLAYER3 22
#define KBR_DEFAULT_MAP_CD32_RAPLAYER4 23
#define KBR_DEFAULT_MAP_ARCADIA 24
#define KBR_DEFAULT_MAP_CDTV 25
#else
#define KBR_DEFAULT_MAP_CD32_NP 6
#define KBR_DEFAULT_MAP_CD32_CK 7
#define KBR_DEFAULT_MAP_CD32_SE 8
#define KBR_DEFAULT_MAP_ARCADIA 9
#define KBR_DEFAULT_MAP_CDTV 10
#endif

static int **keyboard_default_kbmaps;

static int mouse_axis[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];
static int oldm_axis[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];

#define MOUSE_AXIS_TOTAL 4

static uae_s16 mouse_x[MAX_JPORTS], mouse_y[MAX_JPORTS];
static uae_s16 mouse_delta[MAX_JPORTS][MOUSE_AXIS_TOTAL];
static uae_s16 mouse_deltanoreset[MAX_JPORTS][MOUSE_AXIS_TOTAL];
static uae_s16 lightpen_delta[2][2];
static uae_s16 lightpen_deltanoreset[2][2];
static int lightpen_trigger2;
static int joybutton[MAX_JPORTS];
static int joydir[MAX_JPORTS];
static int joydirpot[MAX_JPORTS][2];
static uae_s16 mouse_frame_x[MAX_JPORTS], mouse_frame_y[MAX_JPORTS];

static int mouse_port[NORMAL_JPORTS];
static int cd32_shifter[NORMAL_JPORTS];
static int cd32_pad_enabled[NORMAL_JPORTS];
static int parport_joystick_enabled;
static int oleft[MAX_JPORTS], oright[MAX_JPORTS], otop[MAX_JPORTS], obot[MAX_JPORTS];
static int horizclear[MAX_JPORTS], vertclear[MAX_JPORTS];
static int relativecount[MAX_JPORTS][2];

uae_u16 potgo_value;
static int pot_cap[NORMAL_JPORTS][2];
static uae_u8 pot_dat[NORMAL_JPORTS][2];
static int pot_dat_act[NORMAL_JPORTS][2];
static int analog_port[NORMAL_JPORTS][2];
static int digital_port[NORMAL_JPORTS][2];
static int lightpen_port[NORMAL_JPORTS];
int cubo_enabled;
uae_u32 cubo_flag;
#define POTDAT_DELAY_PAL 8
#define POTDAT_DELAY_NTSC 7

static int use_joysticks[MAX_INPUT_DEVICES];
static int use_mice[MAX_INPUT_DEVICES];
static int use_keyboards[MAX_INPUT_DEVICES];

#define INPUT_QUEUE_SIZE 16
struct input_queue_struct {
	int evt, storedstate, state, max, linecnt, nextlinecnt;
	TCHAR *custom;
};
static struct input_queue_struct input_queue[INPUT_QUEUE_SIZE];

uae_u8 *restore_input (uae_u8 *src)
{
	restore_u32 ();
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			pot_cap[i][j] = restore_u16 ();
		}
	}
	return src;
}
uae_u8 *save_input (size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (0);
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			save_u16 (pot_cap[i][j]);
		}
	}
	*len = dst - dstbak;
	return dstbak;
}

static void freejport (struct uae_prefs *dst, int num)
{
	bool override = dst->jports[num].nokeyboardoverride;
	memset (&dst->jports[num], 0, sizeof (struct jport));
	dst->jports[num].id = -1;
	dst->jports[num].nokeyboardoverride = override;
}
static void copyjport (const struct uae_prefs *src, struct uae_prefs *dst, int num)
{
	if (!src)
		return;
	freejport (dst, num);
	memcpy(&dst->jports[num].idc, &src->jports[num].idc, sizeof(struct inputdevconfig));
	dst->jports[num].id = src->jports[num].id;
	dst->jports[num].mode = src->jports[num].mode;
	dst->jports[num].submode = src->jports[num].submode;
	dst->jports[num].autofire = src->jports[num].autofire;
	dst->jports[num].nokeyboardoverride = src->jports[num].nokeyboardoverride;
#ifdef AMIBERRY
	dst->jports[num].mousemap = src->jports[num].mousemap;
#endif
}

#define MAX_STORED_JPORTS 8
struct stored_jport
{
	struct jport jp;
	bool inuse;
	bool defaultports;
	uae_u32 age;
};
static struct stored_jport stored_jports[MAX_JPORTS][MAX_STORED_JPORTS];
static uae_u32 stored_jport_cnt;

// return port where idc was previously plugged if any and forgot it.
static int inputdevice_get_unplugged_device(struct inputdevconfig *idc, struct stored_jport **sjp)
{
	for (int portnum = 0; portnum < MAX_JPORTS; portnum++) {
		for (int i = 0; i < MAX_STORED_JPORTS; i++) {
			struct stored_jport *jp = &stored_jports[portnum][i];
			if (jp->inuse && jp->jp.id == JPORT_UNPLUGGED) {
				if (!_tcscmp(idc->name, jp->jp.idc.name) && !_tcscmp(idc->configname, jp->jp.idc.configname)) {
					jp->inuse = false;
					*sjp = jp;
					return portnum;
				}
			}
		}
	}
	return -1;
}

// forget port's unplugged device
void inputdevice_forget_unplugged_device(int portnum)
{
	for (int i = 0; i < MAX_STORED_JPORTS; i++) {
		struct stored_jport *jp = &stored_jports[portnum][i];
		if (jp->inuse && jp->jp.id == JPORT_UNPLUGGED) {
			jp->inuse = false;
		}
	}
}

static struct jport *inputdevice_get_used_device(int portnum, int ageindex)
{
	int idx = -1;
	int used[MAX_STORED_JPORTS] = { 0 };
	if (ageindex < 0)
		return NULL;
	while (ageindex >= 0) {
		uae_u32 age = 0;
		idx = -1;
		for (int i = 0; i < MAX_STORED_JPORTS; i++) {
			struct stored_jport *jp = &stored_jports[portnum][i];
			if (jp->inuse && !used[i] && jp->age > age) {
				age = jp->age;
				idx = i;
			}
		}
		if (idx < 0)
			return NULL;
		used[idx] = 1;
		ageindex--;
	}
	return &stored_jports[portnum][idx].jp;
}

static void inputdevice_store_clear(void)
{
	for (int j = 0; j < MAX_JPORTS; j++) {
		for (int i = 0; i < MAX_STORED_JPORTS; i++) {
			struct stored_jport *jp = &stored_jports[j][i];
			if (!jp->defaultports)
				memset(jp, 0, sizeof(struct stored_jport));
		}
	}
}

static void inputdevice_set_newest_used_device(int portnum, struct jport *jps)
{
	for (int i = 0; i < MAX_STORED_JPORTS; i++) {
		struct stored_jport *jp = &stored_jports[portnum][i];
		if (jp->inuse && &jp->jp == jps && !jp->defaultports) {
			stored_jport_cnt++;
			jp->age = stored_jport_cnt;
		}
	}
}

static void inputdevice_store_used_device(struct jport *jps, int portnum, bool defaultports)
{
	if (jps->id == JPORT_NONE)
		return;

	// already added? if custom or kbr layout: delete all old
	for (int i = 0; i < MAX_STORED_JPORTS; i++) {
		struct stored_jport *jp = &stored_jports[portnum][i];
		if (jp->inuse && ((jps->id == jp->jp.id) || (jps->idc.configname[0] != 0 && jp->jp.idc.configname[0] != 0 && !_tcscmp(jps->idc.configname, jp->jp.idc.configname)))) {
			if (!jp->defaultports) {
				jp->inuse = false;
			}
		}
	}

	// delete from other ports
	for (int j = 0; j < MAX_JPORTS; j++) {
		for (int i = 0; i < MAX_STORED_JPORTS; i++) {
			struct stored_jport *jp = &stored_jports[j][i];
			if (jp->inuse && ((jps->id == jp->jp.id) || (jps->idc.configname[0] != 0 && jp->jp.idc.configname[0] != 0 && !_tcscmp(jps->idc.configname, jp->jp.idc.configname)))) {
				if (!jp->defaultports) {
					jp->inuse = false;
				}
			}
		}
	}
	// delete oldest if full
	for (int i = 0; i < MAX_STORED_JPORTS; i++) {
		struct stored_jport *jp = &stored_jports[portnum][i];
		if (!jp->inuse)
			break;
		if (i == MAX_STORED_JPORTS - 1) {
			uae_u32 age = 0xffffffff;
			int idx = -1;
			for (int j = 0; j < MAX_STORED_JPORTS; j++) {
				struct stored_jport *jp = &stored_jports[portnum][j];
				if (jp->age < age && !jp->defaultports) {
					age = jp->age;
					idx = j;
				}
			}
			if (idx >= 0) {
				struct stored_jport *jp = &stored_jports[portnum][idx];
				jp->inuse = false;
			}
		}
	}
	// add new	
	for (int i = 0; i < MAX_STORED_JPORTS; i++) {
		struct stored_jport *jp = &stored_jports[portnum][i];
		if (!jp->inuse) {
			memcpy(&jp->jp, jps, sizeof(struct jport));
			write_log(_T("Stored port %d/%d d=%d: added %d %d %s %s\n"), portnum, i, defaultports, jp->jp.id, jp->jp.mode, jp->jp.idc.name, jp->jp.idc.configname);
			jp->inuse = true;
			jp->defaultports = defaultports;
			stored_jport_cnt++;
			jp->age = stored_jport_cnt;
			return;
		}
	}
}

static void inputdevice_store_unplugged_port(struct uae_prefs *p, struct inputdevconfig *idc)
{
	struct jport jpt = { 0 };
	_tcscpy(jpt.idc.configname, idc->configname);
	_tcscpy(jpt.idc.name, idc->name);
	jpt.id = JPORT_UNPLUGGED;
	for (int i = 0; i < MAX_JPORTS; i++) {
		struct jport *jp = &p->jports[i];
		if (!_tcscmp(jp->idc.name, idc->name) && !_tcscmp(jp->idc.configname, idc->configname)) {
			write_log(_T("On the fly unplugged stored, port %d '%s' (%s)\n"), i, jpt.idc.name, jpt.idc.configname);
			jpt.mode = jp->mode;
			jpt.submode = jp->submode;
			jpt.autofire = jp->autofire;
			inputdevice_store_used_device(&jpt, i, false);
		}
	}
}

static bool isemptykey(int keyboard, int scancode)
{
	int j = 0;
	struct uae_input_device *na = &keyboards[keyboard];
	while (j < MAX_INPUT_DEVICE_EVENTS && na->extra[j] >= 0) {
		if (na->extra[j] == scancode) {
			for (int k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
				if (na->eventid[j][k] > 0)
					return false;
				if (na->custom[j][k] != NULL)
					return false;
			}
			break;
		}
		j++;
	}
	return true;
}

static void out_config (struct zfile *f, int id, int num, const TCHAR *s1, const TCHAR *s2)
{
	TCHAR tmp[MAX_DPATH];
	_sntprintf (tmp, sizeof tmp, _T("input.%d.%s%d"), id, s1, num);
	cfgfile_write_str (f, tmp, s2);
}

static bool write_config_head (struct zfile *f, int idnum, int devnum, const TCHAR *name, struct uae_input_device *id,  struct inputdevice_functions *idf)
{
	const TCHAR* s = NULL;
	TCHAR tmp2[CONFIG_BLEN];

	if (idnum == GAMEPORT_INPUT_SETTINGS) {
		if (!isdevice (id))
			return false;
		if (!id->enabled)
			return false;
	}

	if (id->name)
		s = id->name;
	else if (devnum < idf->get_num ())
		s = idf->get_friendlyname (devnum);
	if (s) {
		_sntprintf (tmp2, sizeof tmp2, _T("input.%d.%s.%d.friendlyname"), idnum + 1, name, devnum);
		cfgfile_write_str (f, tmp2, s);
	}

	s = NULL;
	if (id->configname)
		s = id->configname;
	else if (devnum < idf->get_num ())
		s = idf->get_uniquename (devnum);
	if (s) {
		_sntprintf (tmp2, sizeof tmp2, _T("input.%d.%s.%d.name"), idnum + 1, name, devnum);
		cfgfile_write_str (f, tmp2, s);
	}

	if (!isdevice (id)) {
		_sntprintf (tmp2, sizeof tmp2, _T("input.%d.%s.%d.empty"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, true);
		if (id->enabled) {
			_sntprintf (tmp2, sizeof tmp2, _T("input.%d.%s.%d.disabled"), idnum + 1, name, devnum);
			cfgfile_write_bool (f, tmp2, id->enabled ? false : true);
		}
		return false;
	}

	if (idnum == GAMEPORT_INPUT_SETTINGS) {
		_sntprintf (tmp2, sizeof tmp2, _T("input.%d.%s.%d.custom"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, true);
	} else {
		_sntprintf (tmp2, sizeof tmp2, _T("input.%d.%s.%d.empty"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, false);
		_sntprintf (tmp2, sizeof tmp2, _T("input.%d.%s.%d.disabled"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, id->enabled ? false : true);
	}
	return true;
}

static bool write_slot (TCHAR *p, struct uae_input_device *uid, int i, int j)
{
	bool ok = false;
	if (i < 0 || j < 0) {
		_tcscpy (p, _T("NULL"));
		return false;
	}
	uae_u64 flags = uid->flags[i][j];
	if (uid->custom[i][j] && uid->custom[i][j][0] != '\0') {
		_sntprintf (p, sizeof p, _T("'%s'.%d"), uid->custom[i][j], (int)(flags & ID_FLAG_SAVE_MASK_CONFIG));
		ok = true;
	} else if (uid->eventid[i][j] > 0) {
		_sntprintf (p, sizeof p, _T("%s.%d"), events[uid->eventid[i][j]].confname, (int)(flags & ID_FLAG_SAVE_MASK_CONFIG));
		ok = true;
	} else {
		_tcscpy (p, _T("NULL"));
	}
	if (ok && (flags & ID_FLAG_SAVE_MASK_QUALIFIERS)) {
		TCHAR *p2 = p + _tcslen (p);
		*p2++ = '.';
		for (int k = 0; k < MAX_INPUT_QUALIFIERS * 2; k++) {
			if ((ID_FLAG_QUALIFIER1 << k) & flags) {
				if (k & 1)
					_sntprintf (p2, sizeof p2, _T("%c"), 'a' + k / 2);
				else
					_sntprintf (p2, sizeof p2, _T("%c"), 'A' + k / 2);
				p2++;
			}
		}
	}
	return ok;
}

static struct inputdevice_functions *getidf (int devnum);

static void kbrlabel (TCHAR *s)
{
	while (*s) {
		*s = _totupper (*s);
		if (*s == ' ')
			*s = '_';
		s++;
	}
}

static void write_config2 (struct zfile *f, int idnum, int i, int offset, const TCHAR *extra, struct uae_input_device *id)
{
	TCHAR tmp2[CONFIG_BLEN], tmp3[CONFIG_BLEN], *p;
	int evt, got, j, k;
	TCHAR *custom;
	const int *slotorder;
	int io = i + offset;

	tmp2[0] = 0;
	p = tmp2;
	got = 0;

	slotorder = slotorder1;
	// if gameports non-custom mapping in slot0 -> save slot8 as slot0
	if (id->port[io][0] && !(id->flags[io][0] & ID_FLAG_GAMEPORTSCUSTOM_MASK))
		slotorder = slotorder2;

	for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {

		evt = id->eventid[io][slotorder[j]];
		custom = id->custom[io][slotorder[j]];
		if (custom == NULL && evt <= 0) {
			for (k = j + 1; k < MAX_INPUT_SUB_EVENT; k++) {
				if ((id->port[io][k] == 0 || id->port[io][k] == MAX_JPORTS + 1) && (id->eventid[io][slotorder[k]] > 0 || id->custom[io][slotorder[k]] != NULL))
					break;
			}
			if (k == MAX_INPUT_SUB_EVENT)
				break;
		}

		if (p > tmp2) {
			*p++ = ',';
			*p = 0;
		}
		bool ok = write_slot (p, id, io, slotorder[j]);
		p += _tcslen (p);
		if (ok) {
			if (id->port[io][slotorder[j]] > 0 && id->port[io][slotorder[j]] < MAX_JPORTS + 1) {
				int pnum = id->port[io][slotorder[j]] - 1;
				_sntprintf (p, sizeof p, _T(".%d"), pnum);
				p += _tcslen (p);
				if (idnum != GAMEPORT_INPUT_SETTINGS && j == 0 && id->port[io][SPARE_SUB_EVENT] && slotorder == slotorder1) {
					*p++ = '.';
					write_slot (p, id, io, SPARE_SUB_EVENT);
					p += _tcslen (p);
				}
			}
		}
	}
	if (p > tmp2) {
		_sntprintf (tmp3, sizeof tmp3, _T("input.%d.%s%d"), idnum + 1, extra, i);
		cfgfile_write_str (f, tmp3, tmp2);
	}
}

static void write_kbr_config (struct zfile *f, int idnum, int devnum, struct uae_input_device *kbr, struct inputdevice_functions *idf)
{
	TCHAR tmp1[CONFIG_BLEN], tmp2[CONFIG_BLEN], tmp3[CONFIG_BLEN], tmp4[CONFIG_BLEN], tmp5[CONFIG_BLEN], *p;
	int i, j, k, evt, skip;
	const int *slotorder;

	if (!keyboard_default)
		return;

	if (!write_config_head (f, idnum, devnum, _T("keyboard"), kbr, idf))
		return;

	i = 0;
	while (i < MAX_INPUT_DEVICE_EVENTS && kbr->extra[i] >= 0) {

		slotorder = slotorder1;
		// if gameports non-custom mapping in slot0 -> save slot8 as slot0
		if (kbr->port[i][0] && !(kbr->flags[i][0] & ID_FLAG_GAMEPORTSCUSTOM_MASK))
			slotorder = slotorder2;

		skip = 0;
		k = 0;
		while (keyboard_default[k].scancode >= 0) {
			if (keyboard_default[k].scancode == kbr->extra[i]) {
				skip = 1;
				for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
					if (keyboard_default[k].node[j].evt != 0) {
						if (keyboard_default[k].node[j].evt != kbr->eventid[i][slotorder[j]] || keyboard_default[k].node[j].flags != (kbr->flags[i][slotorder[j]] & ID_FLAG_SAVE_MASK_FULL))
							skip = 0;
					} else if ((kbr->flags[i][slotorder[j]] & ID_FLAG_SAVE_MASK_FULL) != 0 || kbr->eventid[i][slotorder[j]] > 0) {
						skip = 0;
					}
				}
				break;
			}
			k++;
		}
		bool isdefaultspare =
			kbr->port[i][SPARE_SUB_EVENT] &&
			keyboard_default[k].node[0].evt == kbr->eventid[i][SPARE_SUB_EVENT] && keyboard_default[k].node[0].flags == (kbr->flags[i][SPARE_SUB_EVENT] & ID_FLAG_SAVE_MASK_FULL);

		if (kbr->port[i][0] > 0 && !(kbr->flags[i][0] & ID_FLAG_GAMEPORTSCUSTOM_MASK) && 
			(kbr->eventid[i][1] <= 0 && kbr->eventid[i][2] <= 0 && kbr->eventid[i][3] <= 0) &&
			(kbr->port[i][SPARE_SUB_EVENT] == 0 || isdefaultspare))
			skip = 1;
		if (kbr->eventid[i][0] == 0 && (kbr->flags[i][0] & ID_FLAG_SAVE_MASK_FULL) == 0 && keyboard_default[k].scancode < 0)
			skip = 1;
		if (skip) {
			i++;
			continue;
		}

		if (!input_get_default_keyboard(devnum)) {
			bool isempty = true;
			for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
				if (kbr->eventid[i][j] > 0) {
					isempty = false;
					break;
				}
				if (kbr->custom[i][j] != NULL) {
					isempty = false;
					break;
				}
			}
			if (isempty) {
				i++;
				continue;
			}
		}

		tmp2[0] = 0;
		p = tmp2;
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			TCHAR *custom = kbr->custom[i][slotorder[j]];
			evt = kbr->eventid[i][slotorder[j]];
			if (custom == NULL && evt <= 0) {
				for (k = j + 1; k < MAX_INPUT_SUB_EVENT; k++) {
					if (kbr->eventid[i][slotorder[k]] > 0 || kbr->custom[i][slotorder[k]] != NULL)
						break;
				}
				if (k == MAX_INPUT_SUB_EVENT)
					break;
			}
			if (p > tmp2) {
				*p++ = ',';
				*p = 0;
			}
			bool ok = write_slot (p, kbr, i, slotorder[j]);
			p += _tcslen (p);
			if (ok) {
				// save port number + SPARE SLOT if needed
				if (kbr->port[i][slotorder[j]] > 0 && (kbr->flags[i][slotorder[j]] & ID_FLAG_GAMEPORTSCUSTOM_MASK)) {
					_sntprintf (p, sizeof p, _T(".%d"), kbr->port[i][slotorder[j]] - 1);
					p += _tcslen (p);
					if (idnum != GAMEPORT_INPUT_SETTINGS && j == 0 && kbr->port[i][SPARE_SUB_EVENT] && !isdefaultspare && slotorder == slotorder1) {
						*p++ = '.';
						write_slot (p, kbr, i, SPARE_SUB_EVENT);
						p += _tcslen (p);
					}
				}
			}
		}
		idf->get_widget_type (devnum, i, tmp5, NULL);
		_sntprintf (tmp3, sizeof tmp3, _T("%d%s%s"), kbr->extra[i], tmp5[0] ? _T(".") : _T(""), tmp5[0] ? tmp5 : _T(""));
		kbrlabel (tmp3);
		_sntprintf (tmp1, sizeof tmp1, _T("keyboard.%d.button.%s"), devnum, tmp3);
		_sntprintf (tmp4, sizeof tmp4, _T("input.%d.%s"), idnum + 1, tmp1);
		cfgfile_write_str (f, tmp4, tmp2[0] ? tmp2 : _T("NULL"));
		i++;
	}
}

static void write_config (struct zfile *f, int idnum, int devnum, const TCHAR *name, struct uae_input_device *id, struct inputdevice_functions *idf)
{
	TCHAR tmp1[MAX_DPATH];
	int i;

	if (!write_config_head (f, idnum, devnum, name, id, idf))
		return;

	_sntprintf (tmp1, sizeof tmp1, _T("%s.%d.axis."), name, devnum);
	for (i = 0; i < ID_AXIS_TOTAL; i++)
		write_config2 (f, idnum, i, ID_AXIS_OFFSET, tmp1, id);
	_sntprintf (tmp1, sizeof tmp1, _T("%s.%d.button.") ,name, devnum);
	for (i = 0; i < ID_BUTTON_TOTAL; i++)
		write_config2 (f, idnum, i, ID_BUTTON_OFFSET, tmp1, id);
}

static const TCHAR *kbtypes[] = { _T("amiga"), _T("pc"), NULL };

void write_inputdevice_config (struct uae_prefs *p, struct zfile *f)
{
	int i, id;

	cfgfile_write(f, _T("input.config"), _T("%d"), p->input_selected_setting == GAMEPORT_INPUT_SETTINGS ? 0 : p->input_selected_setting + 1);
	cfgfile_write(f, _T("input.joymouse_speed_analog"), _T("%d"), p->input_joymouse_multiplier);
	cfgfile_write(f, _T("input.joymouse_speed_digital"), _T("%d"), p->input_joymouse_speed);
	cfgfile_write(f, _T("input.joymouse_deadzone"), _T("%d"), p->input_joymouse_deadzone);
	cfgfile_write(f, _T("input.joystick_deadzone"), _T("%d"), p->input_joystick_deadzone);
	cfgfile_write(f, _T("input.analog_joystick_multiplier"), _T("%d"), p->input_analog_joystick_mult);
	cfgfile_write(f, _T("input.analog_joystick_offset"), _T("%d"), p->input_analog_joystick_offset);
	cfgfile_write(f, _T("input.mouse_speed"), _T("%d"), p->input_mouse_speed);
	cfgfile_write(f, _T("input.autofire_speed"), _T("%d"), p->input_autofire_linecnt);
	cfgfile_write_bool(f, _T("input.autoswitch"), p->input_autoswitch);
	cfgfile_dwrite_str(f, _T("input.keyboard_type"), kbtypes[p->input_keyboard_type]);
	cfgfile_dwrite(f, _T("input.contact_bounce"), _T("%d"), p->input_contact_bounce);
	cfgfile_dwrite(f, _T("input.devicematchflags"), _T("%d"), p->input_device_match_mask);
	cfgfile_dwrite_bool(f, _T("input.autoswitchleftright"), p->input_autoswitchleftright);
	cfgfile_dwrite_bool(f, _T("input.advancedmultiinput"), p->input_advancedmultiinput);
	cfgfile_dwrite_bool(f, _T("input.default_osk"), p->input_default_onscreen_keyboard);
#ifndef AMIBERRY // not used in Amiberry
	for (id = 0; id < MAX_INPUT_SETTINGS; id++) {
		TCHAR tmp[MAX_DPATH];
		if (id < GAMEPORT_INPUT_SETTINGS) {
			_stprintf (tmp, _T("input.%d.name"), id + 1);
			cfgfile_dwrite_str (f, tmp, p->input_config_name[id]);
		}
		for (i = 0; i < MAX_INPUT_DEVICES; i++)
			write_config (f, id, i, _T("joystick"), &p->joystick_settings[id][i], &idev[IDTYPE_JOYSTICK]);
		for (i = 0; i < MAX_INPUT_DEVICES; i++)
			write_config (f, id, i, _T("mouse"), &p->mouse_settings[id][i], &idev[IDTYPE_MOUSE]);
		for (i = 0; i < MAX_INPUT_DEVICES; i++)
			write_kbr_config (f, id, i, &p->keyboard_settings[id][i], &idev[IDTYPE_KEYBOARD]);
		write_config (f, id, 0, _T("internal"), &p->internalevent_settings[id][0], &idev[IDTYPE_INTERNALEVENT]);
	}
#endif
}

static uae_u64 getqual (const TCHAR **pp)
{
	const TCHAR *p = *pp;
	uae_u64 mask = 0;

	while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) {
		bool press = (*p >= 'A' && *p <= 'Z');
		int shift, inc;

		if (press) {
			shift = *p - 'A';
			inc = 0;
		} else {
			shift = *p - 'a';
			inc = 1;
		}
		mask |= ID_FLAG_QUALIFIER1 << (shift * 2 + inc);
		p++;
	}
	while (*p != 0 && *p !='.' && *p != ',')
		p++;
	if (*p == '.' || *p == ',')
		p++;
	*pp = p;
	return mask;
}

static int getnum (const TCHAR **pp)
{
	const TCHAR *p = *pp;
	int v;

	if (!_tcsnicmp (p, _T("false"), 5))
		v = 0;
	else if (!_tcsnicmp (p, _T("true"), 4))
		v = 1;
	else
		v = _tstol (p);

	while (*p != 0 && *p !='.' && *p != ',' && *p != '=')
		p++;
	if (*p == '.' || *p == ',')
		p++;
	*pp = p;
	return v;
}
static TCHAR *getstring (const TCHAR **pp)
{
	int i;
	static TCHAR str[CONFIG_BLEN];
	const TCHAR *p = *pp;
	bool quoteds = false;
	bool quotedd = false;

	if (*p == 0)
		return 0;
	i = 0;
	while (*p != 0 && i < 1000 - 1) {
		if (*p == '\"')
			quotedd = quotedd ? false : true;
		if (*p == '\'')
			quoteds = quoteds ? false : true;
		if (!quotedd && !quoteds) {
			if (*p == '.' || *p == ',')
				break;
		}
		str[i++] = *p++;
	}
	if (*p == '.' || *p == ',')
		p++;
	str[i] = 0;
	*pp = p;
	return str;
}

static void reset_inputdevice_settings (struct uae_input_device *uid)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT_ALL; i++) {
			uid->eventid[l][i] = 0;
			uid->flags[l][i] = 0;
			if (uid->custom[l][i]) {
				xfree (uid->custom[l][i]);
				uid->custom[l][i] = NULL;
			}
		}
	}
}
static void reset_inputdevice_slot (struct uae_prefs *prefs, int slot)
{
	for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
		reset_inputdevice_settings (&prefs->joystick_settings[slot][m]);
		reset_inputdevice_settings (&prefs->mouse_settings[slot][m]);
		reset_inputdevice_settings (&prefs->keyboard_settings[slot][m]);
	}
}

static void reset_inputdevice_config_temp(void)
{
	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		for (int j = 0; j < IDTYPE_MAX; j++) {
			temp_uid_index[i][j] = -1;
		}
	}
	for (int i = 0; i < IDTYPE_MAX; i++) {
		temp_uid_cnt[i] = 0;
	}
	xfree(temp_uid.configname);
	xfree(temp_uid.name);
	memset(&temp_uid, 0, sizeof temp_uid);
	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		temp_uid.matcheddevices[i] = -1;
		temp_uid.nonmatcheddevices[i] = -1;
	}
	temp_uid.idnum = -1;
	temp_uid.lastdevtype = -1;
}

void reset_inputdevice_config (struct uae_prefs *prefs, bool reset)
{
	for (int i = 0; i < MAX_INPUT_SETTINGS; i++)
		reset_inputdevice_slot (prefs, i);
	reset_inputdevice_config_temp();

	if (reset) {
		inputdevice_store_clear();
	}
}

static void set_kbr_default_event (struct uae_input_device *kbr, struct uae_input_device_kbr_default *trans, int num)
{
	if (!kbr->enabled || !trans)
		return;
	for (int i = 0; trans[i].scancode >= 0; i++) {
		if (kbr->extra[num] == trans[i].scancode) {
			int k;
			for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
				if (kbr->eventid[num][k] == 0)
					break;
			}
			if (k == MAX_INPUT_SUB_EVENT) {
				write_log (_T("corrupt default keyboard mappings\n"));
				return;
			}
			int l = 0;
			while (k < MAX_INPUT_SUB_EVENT && trans[i].node[l].evt) {
				int evt = trans[i].node[l].evt;
				if (evt < 0 || evt >= INPUTEVENT_SPC_LAST)
					gui_message(_T("invalid event in default keyboard table!"));
				kbr->eventid[num][k] = evt;
				kbr->flags[num][k] = trans[i].node[l].flags;
				l++;
				k++;
			}
			break;
		}
	}
}

static void clear_id (struct uae_input_device *id)
{
#ifndef	_DEBUG
	int i, j;
	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT_ALL; j++) {
			xfree (id->custom[i][j]);
			id->custom[i][j] = NULL;
		}
	}
#endif
	TCHAR *cn = id->configname;
	TCHAR *n = id->name;
	memset (id, 0, sizeof (struct uae_input_device));
	id->configname = cn;
	id->name = n;
}

static void set_kbr_default (struct uae_prefs *p, int index, int devnum, struct uae_input_device_kbr_default *trans)
{
	int i, j;
	struct uae_input_device *kbr;
	struct inputdevice_functions *id = &idev[IDTYPE_KEYBOARD];
	uae_u32 scancode;

	for (j = 0; j < MAX_INPUT_DEVICES; j++) {
		if (devnum >= 0 && devnum != j)
			continue;
		kbr = &p->keyboard_settings[index][j];
		uae_s8 ena = kbr->enabled;
		clear_id (kbr);
		if (ena > 0)
			kbr->enabled = ena;
		for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++)
			kbr->extra[i] = -1;
		if (j < id->get_num ()) {
			if (input_get_default_keyboard (j))
				kbr->enabled = 1;
			for (i = 0; i < id->get_widget_num (j); i++) {
				id->get_widget_type (j, i, 0, &scancode);
				kbr->extra[i] = scancode;
				if (j == 0 || kbr->enabled)
					set_kbr_default_event (kbr, trans, i);
			}
		}
	}
}

static void inputdevice_default_kb (struct uae_prefs *p, int num)
{
	if (num == GAMEPORT_INPUT_SETTINGS) {
		reset_inputdevice_slot (p, num);
	}
	set_kbr_default (p, num, -1, keyboard_default);
}

static void inputdevice_default_kb_all (struct uae_prefs *p)
{
	for (int i = 0; i < MAX_INPUT_SETTINGS; i++)
		inputdevice_default_kb (p, i);
}

static const int af_port1[] = {
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_CD32_RED,
	-1
};
static const int af_port2[] = {
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_CD32_RED,
	-1
};
static const int af_port3[] = {
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON, INPUTEVENT_PAR_JOY1_2ND_BUTTON,
	-1
};
static const int af_port4[] = {
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON, INPUTEVENT_PAR_JOY2_2ND_BUTTON,
	-1
};
static const int *af_ports[] = { af_port1, af_port2, af_port3, af_port4 }; 

static void setautofireevent(struct uae_input_device *uid, int num, int sub, int af, int index)
{
	if (!af)
		return;
#ifdef RETROPLATFORM
	// don't override custom AF autofire mappings
	if (rp_isactive())
		return;
#endif
	const int *afp = af_ports[index];
	for (int k = 0; afp[k] >= 0; k++) {
		if (afp[k] == uid->eventid[num][sub]) {
			uid->flags[num][sub] &= ~ID_FLAG_AUTOFIRE_MASK;
			if (af >= JPORT_AF_NORMAL)
				uid->flags[num][sub] |= ID_FLAG_AUTOFIRE;
			if (af == JPORT_AF_TOGGLE)
				uid->flags[num][sub] |= ID_FLAG_TOGGLE;
			if (af == JPORT_AF_ALWAYS)
				uid->flags[num][sub] |= ID_FLAG_INVERTTOGGLE;
			if (af == JPORT_AF_TOGGLENOAF)
				uid->flags[num][sub] |= ID_FLAG_INVERT;
			return;
		}
	}
}

static void setcompakbevent(struct uae_prefs *p, struct uae_input_device *uid, int l, int evt, int port, int af, uae_u64 flags)
{
	inputdevice_sparecopy(uid, l, 0);
	if (p->jports[port].nokeyboardoverride && uid->port[l][0] == 0) {
		uid->eventid[l][MAX_INPUT_SUB_EVENT - 1] = uid->eventid[l][0];
		uid->flags[l][MAX_INPUT_SUB_EVENT - 1] = uid->flags[l][0] | ID_FLAG_RESERVEDGAMEPORTSCUSTOM;
		uid->custom[l][MAX_INPUT_SUB_EVENT - 1] = my_strdup(uid->custom[l][0]);
		uid->eventid[l][MAX_INPUT_SUB_EVENT - 1] = uid->eventid[l][0];
	}
	uid->eventid[l][0] = evt;
	uid->flags[l][0] &= COMPA_RESERVED_FLAGS;
	uid->flags[l][0] |= flags;
	uid->port[l][0] = port + 1;
	xfree(uid->custom[l][0]);
	uid->custom[l][0] = NULL;
	if (!JSEM_ISCUSTOM(port, p)) {
		setautofireevent(uid, l, 0, af, port);
	}
}

static int matchdevice(struct uae_prefs *p, struct inputdevice_functions *inf, const TCHAR *configname, const TCHAR *name)
{
	int match = -1;
	for (int j = 0; j < 4; j++) {
		bool fullmatch = j == 0;
		for (int i = 0; i < inf->get_num(); i++) {
			const TCHAR* aname1 = inf->get_friendlyname(i);
			const TCHAR* aname2 = inf->get_uniquename(i);
			if (fullmatch) {
				if (!aname1 || !name)
					continue;
				if (!(p->input_device_match_mask & INPUT_MATCH_BOTH))
					continue;
			} else {
				if (!(p->input_device_match_mask & INPUT_MATCH_CONFIG_NAME_ONLY))
					continue;
			}
			if (aname2 && configname && aname2[0] && configname[0]) {
				bool matched = false;
				TCHAR bname[MAX_DPATH];
				TCHAR bname2[MAX_DPATH];
				TCHAR *p1 = NULL, *p2 = NULL;
				_tcscpy(bname, configname);
				_tcscpy(bname2, aname2);
				// strip possible local guid part
				if (bname[_tcslen(bname) - 1] == '}') {
					p1 = _tcschr(bname, '{');
				}
				if (bname[_tcslen(bname2) - 1] == '}') {
					p2 = _tcschr(bname2, '{');
				}
				if (!p1 && !p2) {
					// check possible directinput names too
					p1 = _tcschr(bname, ' ');
					p2 = _tcschr(bname2, ' ');
				}
				if (!_tcscmp(bname, bname2)) {
					matched = true;
				} else if (p1 && p2 && p1 - bname == p2 - bname2) {
					*p1 = 0;
					*p2 = 0;
					if (bname[0] && !_tcscmp(bname2, bname))
						matched = true;
				}
				if (matched && fullmatch && _tcscmp(aname1, name) != 0) {
					matched = false;
				}
				if (matched) {
					if (match >= 0)
						match = -2;
					else
						match = i;
				}
				if (match == -2) {
					break;
				}
			}
		}
		if (match != -1) {
			break;
		}
	}
	// multiple matches -> use complete local-only id string for comparisons
	if (match == -2) {
		for (int j = 0; j < 2; j++) {
			bool fullmatch = j == 0;
			match = -1;
			for (int i = 0; i < inf->get_num(); i++) {
				const TCHAR* aname1 = inf->get_friendlyname(i);
				const TCHAR* aname2 = inf->get_uniquename(i);
				if (aname2 && configname && aname2[0] && configname[0]) {
					const TCHAR *bname2 = configname;
					bool matched = false;
					if (fullmatch) {
						if (!aname1 || !name)
							continue;
						if (!(p->input_device_match_mask & INPUT_MATCH_BOTH))
							continue;
					} else {
						if (!(p->input_device_match_mask & INPUT_MATCH_CONFIG_NAME_ONLY))
							continue;
					}
					if (aname2 && bname2 && !_tcscmp(aname2, bname2))
						matched = true;
					if (matched && fullmatch && _tcscmp(aname1, name) != 0)
						matched = false;
					if (matched) {
						if (match >= 0) {
							match = -2;
							break;
						} else {
							match = i;
						}
					}
				}
			}
			if (match != -1)
				break;
		}
	}
	if (match < 0) {
		// no match, try friendly names
		for (int i = 0; i < inf->get_num(); i++) {
			const TCHAR* aname1 = inf->get_friendlyname(i);
			if (!(p->input_device_match_mask & INPUT_MATCH_FRIENDLY_NAME_ONLY))
				continue;
			if (aname1 && name && aname1[0] && name[0]) {
				const TCHAR *bname1 = name;
				if (aname1 && bname1 && !_tcscmp(aname1, bname1)) {
					if (match >= 0) {
						match = -2;
						break;
					} else {
						match = i;
					}
				}
			}
		}
	}
	return match;
}

static bool read_slot (const TCHAR *parm, int num, int joystick, int button, struct uae_input_device *id, int keynum, int subnum, const struct inputevent *ie, uae_u64 flags, int port, TCHAR *custom)
{
	int mask;

	if (custom == NULL && ie->name == NULL) {
		if (!_tcscmp (parm, _T("NULL"))) {
			if (joystick < 0) {
				id->eventid[keynum][subnum] = 0;
				id->flags[keynum][subnum] = 0;
			} else if (button) {
				id->eventid[num + ID_BUTTON_OFFSET][subnum] = 0;
				id->flags[num + ID_BUTTON_OFFSET][subnum] = 0;
			} else {
				id->eventid[num + ID_AXIS_OFFSET][subnum] = 0;
				id->flags[num + ID_AXIS_OFFSET][subnum] = 0;
			}
		}
		return false;
	}
	if (custom)
		ie = &events[INPUTEVENT_SPC_CUSTOM_EVENT];

	if (joystick < 0) {
		if (!(ie->allow_mask & AM_K))
			return false;
		id->eventid[keynum][subnum] = (int)(ie - events);
		id->flags[keynum][subnum] = flags;
		id->port[keynum][subnum] = port;
		xfree (id->custom[keynum][subnum]);
		id->custom[keynum][subnum] = custom;
	} else  if (button) {
		if (joystick)
			mask = AM_JOY_BUT;
		else
			mask = AM_MOUSE_BUT;
		if (!(ie->allow_mask & mask))
			return false;
		id->eventid[num + ID_BUTTON_OFFSET][subnum] = (int)(ie - events);
		id->flags[num + ID_BUTTON_OFFSET][subnum] = flags;
		id->port[num + ID_BUTTON_OFFSET][subnum] = port;
		xfree (id->custom[num + ID_BUTTON_OFFSET][subnum]);
		id->custom[num + ID_BUTTON_OFFSET][subnum] = custom;
	} else {
		if (joystick)
			mask = AM_JOY_AXIS;
		else
			mask = AM_MOUSE_AXIS;
		if (!(ie->allow_mask & mask))
			return false;
		id->eventid[num + ID_AXIS_OFFSET][subnum] = (int)(ie - events);
		id->flags[num + ID_AXIS_OFFSET][subnum] = flags;
		id->port[num + ID_AXIS_OFFSET][subnum] = port;
		xfree (id->custom[num + ID_AXIS_OFFSET][subnum]);
		id->custom[num + ID_AXIS_OFFSET][subnum] = custom;
	}
	return true;
}

static const struct inputevent *readevent (const TCHAR *name, TCHAR **customp)
{
	int i = 1;
	while (events[i].name) {
		if (!_tcscmp (events[i].confname, name))
			return &events[i];
		i++;
	}
	if (_tcslen (name) > 2 && name[0] == '\'') {
		name++;
		const TCHAR *end = name;
		bool equote = false;
		while (*end) {
			if (*end == '\"') {
				if (!equote) {
					equote = 1;
				} else {
					equote = 0;
				}
			}
			if (!equote && *end == '\'') {
				break;
			}
			end++;
		}
		if (!customp || *end == 0)
			return NULL;
		TCHAR *custom = my_strdup (name);
		custom[end - name] = 0;
		*customp = custom;
	}
	return &events[0];
}

void read_inputdevice_config (struct uae_prefs *pr, const TCHAR *option, TCHAR *value)
{
	struct uae_input_device *id = NULL;
	const struct inputevent *ie;
	int devnum, num, button, joystick, subnum, idnum, keynum, devtype;
	const TCHAR *p;
	TCHAR *p2, *custom;
	struct temp_uids *tid = &temp_uid;
	struct inputdevice_functions *idf = NULL;
	bool directmode = option[5] == '_';

	option += 6; /* "input." */
	p = getstring (&option);
	if (!_tcsicmp (p, _T("config"))) {
		pr->input_selected_setting = _tstol (value) - 1;
		if (pr->input_selected_setting == -1)
			pr->input_selected_setting = GAMEPORT_INPUT_SETTINGS;
		if (pr->input_selected_setting < 0 || pr->input_selected_setting > MAX_INPUT_SETTINGS)
			pr->input_selected_setting = 0;
	}
	if (!_tcsicmp(p, _T("joymouse_speed_analog")))
		pr->input_joymouse_multiplier = _tstol(value);
	if (!_tcsicmp(p, _T("joymouse_speed_digital")))
		pr->input_joymouse_speed = _tstol(value);
	if (!_tcsicmp(p, _T("joystick_deadzone")))
		pr->input_joystick_deadzone = _tstol(value);
	if (!_tcsicmp(p, _T("joymouse_deadzone")))
		pr->input_joymouse_deadzone = _tstol(value);
	if (!_tcsicmp(p, _T("mouse_speed")))
		pr->input_mouse_speed = _tstol(value);
	if (!_tcsicmp(p, _T("autofire")))
		pr->input_autofire_linecnt = _tstol(value) * 312;
	if (!_tcsicmp(p, _T("autofire_speed")))
		pr->input_autofire_linecnt = _tstol (value);
	if (!_tcsicmp(p, _T("analog_joystick_multiplier")))
		pr->input_analog_joystick_mult = _tstol(value);
	if (!_tcsicmp(p, _T("analog_joystick_offset")))
		pr->input_analog_joystick_offset = _tstol(value);
	if (!_tcsicmp(p, _T("autoswitch")))
		pr->input_autoswitch = !_tcsicmp(value, _T("true")) || _tstol(value) != 0;
	if (!_tcsicmp(p, _T("autoswitchleftright")))
		pr->input_autoswitchleftright = !_tcsicmp(value, _T("true")) || _tstol(value) != 0;
	if (!_tcsicmp(p, _T("advancedmultiinput")))
		pr->input_advancedmultiinput = !_tcsicmp(value, _T("true")) || _tstol(value) != 0;
	if (!_tcsicmp(p, _T("default_osk")))
		pr->input_default_onscreen_keyboard = !_tcsicmp(value, _T("true")) || _tstol(value) != 0;
	if (!_tcsicmp(p, _T("keyboard_type"))) {
		cfgfile_strval(p, value, p, &pr->input_keyboard_type, kbtypes, 0);
		keyboard_default = keyboard_default_table[pr->input_keyboard_type];
		inputdevice_default_kb_all(pr);
	}
	if (!strcasecmp(p, _T("devicematchflags"))) {
		pr->input_device_match_mask = _tstol(value);
		write_log(_T("input_device_match_mask = %d\n"), pr->input_device_match_mask);
	}
	if (!strcasecmp(p, _T("contact_bounce")))
		pr->input_contact_bounce = _tstol(value);

	idnum = _tstol (p);
	if (idnum <= 0 || idnum > MAX_INPUT_SETTINGS)
		return;
	idnum--;

	if (idnum != tid->idnum || directmode) {
		reset_inputdevice_config_temp();
		tid->idnum = idnum;
		if (idnum == 0) {
			for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
				for (int j = 0; j < IDTYPE_MAX; j++) {
					gp_swappeddevices[i][j] = -1;
				}
			}
		}
	}

	if (!_tcscmp (option, _T("name"))) {
		if (idnum < GAMEPORT_INPUT_SETTINGS)
			_tcscpy (pr->input_config_name[idnum], value);
		return;
	} 

	if (_tcsncmp (option, _T("mouse."), 6) == 0) {
		p = option + 6;
	} else if (_tcsncmp (option, _T("joystick."), 9) == 0) {
		p = option + 9;
	} else if (_tcsncmp (option, _T("keyboard."), 9) == 0) {
		p = option + 9;
	} else if (_tcsncmp (option, _T("internal."), 9) == 0) {
		p = option + 9;
	} else
		return;

	devnum = getnum (&p);
	if (devnum < 0 || devnum >= MAX_INPUT_DEVICES)
		return;

	if (devnum != tid->devnum) {
		xfree(temp_uid.configname);
		xfree(temp_uid.name);
		temp_uid.name = NULL;
		temp_uid.configname = NULL;
		temp_uid.custom = false;
		temp_uid.empty = false;
		temp_uid.disabled = false;
		tid->devnum = devnum;
	}

	p2 = getstring (&p);
	if (!p2)
		return;

	if (_tcsncmp (option, _T("mouse."), 6) == 0) {
		id = &pr->mouse_settings[idnum][devnum];
		joystick = 0;
		devtype = IDTYPE_MOUSE;
	} else if (_tcsncmp (option, _T("joystick."), 9) == 0) {
		id = &pr->joystick_settings[idnum][devnum];
		joystick = 1;
		devtype = IDTYPE_JOYSTICK;
	} else if (_tcsncmp (option, _T("keyboard."), 9) == 0) {
		id = &pr->keyboard_settings[idnum][devnum];
		joystick = -1;
		devtype = IDTYPE_KEYBOARD;
	} else if (_tcsncmp (option, _T("internal."), 9) == 0) {
		if (devnum >= INTERNALEVENT_COUNT)
			return;
		id = &pr->internalevent_settings[idnum][devnum];
		joystick = 2;
		devtype = IDTYPE_INTERNALEVENT;
	}
	if (!id)
		return;
	idf = &idev[devtype];

	if (devtype != tid->lastdevtype) {
		tid->lastdevtype = devtype;
#if 0
		for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
			if (tid->nonmatcheddevices[i] >= 0) {
				write_log(_T("%d: %d %d\n"), i, tid->nonmatcheddevices[i], tid->matcheddevices[i]);
			}
		}
#endif
	}
	if (directmode) {
		tid->joystick = joystick;
		tid->devtype = devtype;
	}

	bool skipevent = false;

	if (!_tcscmp (p2, _T("name"))) {
		xfree(tid->configname);
		tid->configname = my_strdup (value);
		tid->custom = 0;
		tid->empty = false;
		tid->disabled = false;
		tid->joystick = joystick;
		tid->devtype = devtype;
		return;
	}
	if (!_tcscmp (p2, _T("friendlyname"))) {
		xfree (tid->name);
		tid->name = my_strdup (value);
		tid->custom = 0;
		tid->empty = false;
		tid->disabled = false;
		tid->joystick = joystick;
		tid->devtype = devtype;
		return;
	}
	if (!_tcscmp (p2, _T("custom"))) {
		p = value;
		tid->custom = getnum(&p);
		tid->empty = false;
		tid->joystick = joystick;
		tid->devtype = devtype;
		return;
	}
	if (!_tcscmp(p2, _T("empty"))) {
		p = value;
		tid->empty = getnum(&p);
		tid->joystick = joystick;
		tid->devtype = devtype;
		// don't wait for mapping data because it might not exist but device still might be
		// mapped in game ports panel. Don't lose device data if it isn't connected.
		if (tid->name && tid->configname && !tid->disabled && devtype != IDTYPE_KEYBOARD) {
			skipevent = true;
		}
		if (!skipevent) {
			return;
		}
	}
	if (!_tcscmp(p2, _T("disabled"))) {
		p = value;
		tid->disabled = getnum(&p);
		tid->joystick = joystick;
		tid->devtype = devtype;
		if (tid->devtype == IDTYPE_INTERNALEVENT && devnum < INTERNALEVENT_COUNT) {
			id = &pr->internalevent_settings[idnum][devnum];
			id->enabled = tid->disabled == 0;
		}
		return;
	}

	bool newdev = false;
	bool newmissingdev = false;
	if (temp_uid_index[devnum][tid->devtype] == -1) {
		int newdevnum = -1;
		if (tid->devtype == IDTYPE_KEYBOARD) {
			// keyboard devnum == 0: always select keyboard zero.
			if (devnum == 0) {
				newdevnum = 0;
				tid->disabled = false;
				tid->empty = false;
			} else if (tid->kbreventcnt[0] == 0) {
				write_log(_T("Previous keyboard skipped\n"));
				// if previously found keyboard had no events, next will be tried again
				newdevnum = 0;
				tid->disabled = false;
				tid->empty = false;
			} else {
				newdevnum = matchdevice(pr, idf, tid->configname, tid->name);
			}
		} else {
			// match devices with empty names to first free slot
			if (tid->configname && tid->configname[0] == 0 && tid->name && tid->name[0] == 0) {
				for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
					if (tid->matcheddevices[i] < 0) {
						newdevnum = i;
						break;
					}
				}
			} else {
				newdevnum = matchdevice(pr, idf, tid->configname, tid->name);
			}
		}
		newdev = true;
		if (newdevnum >= 0) {
			temp_uid_index[devnum][tid->devtype] = newdevnum;
			write_log(_T("%d %d: %d -> %d (%s)\n"), idnum, tid->devtype, devnum, temp_uid_index[devnum][tid->devtype], tid->name);
			tid->matcheddevices[devnum] = newdevnum;
			if (idnum == 0) {
				gp_swappeddevices[devnum][tid->devtype] = newdevnum;
				gp_swappeddevices[newdevnum][tid->devtype] = devnum;
			}
		} else {
			newdevnum = idf->get_num() + temp_uid_cnt[tid->devtype];
			if (newdevnum < MAX_INPUT_DEVICES) {
				temp_uid_index[devnum][tid->devtype] = newdevnum;
				temp_uid_cnt[tid->devtype]++;
				if (tid->name)
					write_log(_T("%d %d: %d -> %d (NO MATCH) (%s)\n"), idnum, tid->devtype, devnum, temp_uid_index[devnum][tid->devtype], tid->name);
				if (!tid->disabled) {
					newmissingdev = true;
					tid->nonmatcheddevices[devnum] = newdevnum;
					if (idnum == 0) {
						gp_swappeddevices[devnum][tid->devtype] = newdevnum;
						gp_swappeddevices[newdevnum][tid->devtype] = devnum;
					}
				}
			} else {
				temp_uid_index[devnum][tid->devtype] = -1;
			}
		}
	}
	devnum = temp_uid_index[devnum][tid->devtype];
	if (devnum < 0) {
		if (devnum == -1)
			write_log(_T("%s (%s) not found and no free slots\n"), tid->name, tid->configname);
		temp_uid_index[devnum][tid->devtype] = -2;
		return;
	}

	if (skipevent && !newmissingdev) {
		return;
	}

	if (tid->devtype == IDTYPE_MOUSE) {
		id = &pr->mouse_settings[idnum][devnum];
	} else if (tid->devtype == IDTYPE_JOYSTICK) {
		id = &pr->joystick_settings[idnum][devnum];
	} else if (tid->devtype == IDTYPE_KEYBOARD) {
		id = &pr->keyboard_settings[idnum][devnum];
	} else if (tid->devtype == IDTYPE_INTERNALEVENT) {
		if (devnum >= INTERNALEVENT_COUNT)
			return;
		id = &pr->internalevent_settings[idnum][devnum];
	} else {
		return;
	}

	if (newdev && !directmode) {
		if (!tid->initialized)
			clear_id(id);
		if (!tid->empty && tid->devtype == IDTYPE_KEYBOARD && !tid->initialized) {
			set_kbr_default(pr, idnum, devnum, keyboard_default);
		}
		tid->initialized = true;
		// device didn't exist but keep the data
		if (newmissingdev && tid->nonmatcheddevices[tid->devtype] >= 0) {
			id->configname = my_strdup(tid->configname);
			id->name = my_strdup(tid->name);
		}
		xfree(tid->configname);
		xfree(tid->name);
		tid->configname = NULL;
		tid->name = NULL;
	}

	id->enabled = tid->disabled == 0 ? 1 : 0;
	if (idnum == GAMEPORT_INPUT_SETTINGS) {
		id->enabled = 0;
	}
	if (tid->custom) {
		id->enabled = 1;
	}

	if (skipevent) {
		return;
	}

	if (idnum == GAMEPORT_INPUT_SETTINGS && id->enabled == 0)
		return;

	button = 0;
	keynum = 0;
	joystick = tid->joystick;
	if (joystick < 0) {
		num = getnum (&p);
		for (keynum = 0; keynum < MAX_INPUT_DEVICE_EVENTS; keynum++) {
			if (id->extra[keynum] == num)
				break;
		}
		if (keynum >= MAX_INPUT_DEVICE_EVENTS)
			return;
	} else {
		button = -1;
		if (!_tcscmp (p2, _T("axis")))
			button = 0;
		else if(!_tcscmp (p2, _T("button")))
			button = 1;
		if (button < 0)
			return;
		num = getnum (&p);
	}
	p = value;

	bool oldcustommapping = false;
	custom = NULL;
	for (subnum = 0; subnum < MAX_INPUT_SUB_EVENT; subnum++) {
		uae_u64 flags;
		int port;
		xfree (custom);
		custom = NULL;
		p2 = getstring (&p);
		if (!p2)
			break;
		ie = readevent (p2, &custom);
		flags = 0;
		port = 0;
		if (p[-1] == '.')
			flags = getnum (&p) & ID_FLAG_SAVE_MASK_CONFIG;
		if (p[-1] == '.') {
			if ((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z'))
				flags |= getqual (&p);
			if (p[-1] == '.')
				port = getnum (&p) + 1;
		}
		if (flags & ID_FLAG_RESERVEDGAMEPORTSCUSTOM)
			oldcustommapping = true;
		if (idnum == GAMEPORT_INPUT_SETTINGS && port == 0)
			continue;
		if (p[-1] == '.' && idnum != GAMEPORT_INPUT_SETTINGS) {
			p2 = getstring (&p);
			if (p2) {
				int flags2 = 0;
				if (p[-1] == '.')
					flags2 = getnum (&p) & ID_FLAG_SAVE_MASK_CONFIG;
				if (p[-1] == '.' && ((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')))
					flags |= getqual (&p);
				TCHAR *custom2 = NULL;
				const struct inputevent *ie2 = readevent (p2, &custom2);
				read_slot (p2, num, joystick, button, id, keynum, SPARE_SUB_EVENT, ie2, flags2, MAX_JPORTS + 1, custom2);
			}
		}

		while (*p != 0) {
			if (p[-1] == ',')
				break;
			p++;
		}
		if (!read_slot (p2, num, joystick, button, id, keynum, subnum, ie, flags, port, custom))
			continue;
		custom = NULL;
	}
	if (joystick < 0 && !oldcustommapping)
		tid->kbreventcnt[devnum]++;
	xfree (custom);
}

static void generate_jport_custom_item(struct uae_input_device *uid, int num, int port, int devtype, TCHAR *out)
{
	struct uae_input_device *uid2 = &uid[num];
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			int evt = uid2->eventid[i][j];
			uae_u64 flags = uid2->flags[i][j];
			if (flags & ID_FLAG_GAMEPORTSCUSTOM_MASK) {
				if (uid2->port[i][j] == port + 1 && evt > 0) {
					const struct inputevent *ie = &events[evt];
					TCHAR *p = out + _tcslen(out);
					if (out[0])
						*p++= ' ';
					if (devtype == IDTYPE_KEYBOARD) {
						_sntprintf(p, sizeof p, _T("k.%d.b.%d"), num, uid2->extra[i]);
					} else if (devtype == IDTYPE_JOYSTICK || devtype == IDTYPE_MOUSE) {
						TCHAR type = devtype == IDTYPE_JOYSTICK ? 'j' : 'm';
						if (i >= ID_BUTTON_OFFSET && i < ID_BUTTON_OFFSET + ID_BUTTON_TOTAL) {
							_sntprintf(p, sizeof p, _T("%c.%d.b.%d"), type, num, i - ID_BUTTON_OFFSET);
						} else if (i >= ID_AXIS_OFFSET && i < ID_AXIS_OFFSET + ID_AXIS_TOTAL) {
							_sntprintf(p, sizeof p, _T("%c.%d.a.%d"), type, num, i - ID_AXIS_OFFSET);
						}
					}
					TCHAR *p3 = p + _tcslen(p);
					_sntprintf(p3, sizeof p3, _T(".%d"), (int)(flags & (ID_FLAG_AUTOFIRE | ID_FLAG_TOGGLE | ID_FLAG_INVERTTOGGLE | ID_FLAG_INVERT)));
					if (flags & ID_FLAG_SAVE_MASK_QUALIFIERS) {
						TCHAR *p2 = p + _tcslen(p);
						*p2++ = '.';
						for (int k = 0; k < MAX_INPUT_QUALIFIERS * 2; k++) {
							if ((ID_FLAG_QUALIFIER1 << k) & flags) {
								if (k & 1)
									_sntprintf(p2, sizeof p2, _T("%c"), 'a' + k / 2);
								else
									_sntprintf(p2, sizeof p2, _T("%c"), 'A' + k / 2);
								p2++;
							}
						}
					}
					_tcscat(p, _T("="));
					_tcscat(p, ie->confname);
				}
			}
		}
	}
}

void inputdevice_generate_jport_custom(struct uae_prefs *pr, int port)
{
	if (!JSEM_ISCUSTOM(port, pr))
		return;
	struct jport_custom *jpc = &pr->jports_custom[JSEM_GETCUSTOMIDX(port, pr)];
	jpc->custom[0] = 0;
	for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
		generate_jport_custom_item(pr->joystick_settings[pr->input_selected_setting], l, port, IDTYPE_JOYSTICK, jpc->custom);
		generate_jport_custom_item(pr->mouse_settings[pr->input_selected_setting], l, port, IDTYPE_MOUSE, jpc->custom);
		generate_jport_custom_item(pr->keyboard_settings[pr->input_selected_setting], l, port, IDTYPE_KEYBOARD, jpc->custom);
	}
}

static int custom_autoswitch_joy[MAX_JPORTS_CUSTOM];
static int custom_autoswitch_mouse[MAX_JPORTS_CUSTOM];

void inputdevice_jportcustom_fixup(struct uae_prefs *p, TCHAR *data, int cnum)
{
	TCHAR olddata[MAX_DPATH];
	TCHAR olddata2[MAX_DPATH];
	TCHAR newdata[MAX_DPATH];
	bool modified = false;

	if (data[0] == 0) {
		return;
	}
	_tcscpy(olddata, data);
	_tcscpy(olddata2, data);
	newdata[0] = 0;

	TCHAR *bufp = olddata;
	_tcscat(bufp, _T(" "));

	for (;;) {
		TCHAR *next = bufp;
		TCHAR *startp = bufp;
		const TCHAR *bufp2 = bufp;
		TCHAR devtype;
		int devindex;
		int joystick = 0;
		int num = -1;
		int idtype = -1;
		int odevindex = -1;
		const TCHAR *pbufp = NULL;

		while (next != NULL && *next != ' ' && *next != 0)
			next++;
		if (!next || *next == 0)
			break;
		*next++ = 0;
		TCHAR *p = getstring(&bufp2);
		if (!p)
			goto skip;

		pbufp = bufp2;
		devindex = getnum(&bufp2);
		odevindex = devindex;
		if (*bufp == 0)
			goto skip;
		if (devindex < 0 || devindex >= MAX_INPUT_DEVICES)
			goto skip;

		devtype = _totupper(*p);
		if (devtype == 'M') {
			idtype = IDTYPE_MOUSE;
		} else if (devtype == 'J') {
			idtype = IDTYPE_JOYSTICK;
		} else if (devtype == 'K') {
			idtype = IDTYPE_KEYBOARD;
		}

		if (idtype < 0) {
			goto skip;
		}

		if (newdata[0]) {
			_tcscat(newdata, _T(" "));
		}

		if (gp_swappeddevices[devindex][idtype] >= 0) {
			devindex = gp_swappeddevices[devindex][idtype];
		}

		// index changed?
		if (devindex != odevindex) {
			int idx = addrdiff(pbufp, startp);
			TCHAR *p = olddata;
			TCHAR *d = newdata + _tcslen(newdata);
			if (devindex >= 10 && odevindex < 10) {
				for (int i = 0; i < idx; i++) {
					*d++ = *p++;
				}
				*d++ = '0' + (devindex / 10);
				*d++ = '0' + (devindex % 10);
				_tcscpy(d, &p[idx + 1]);
			} else if (devindex < 10 && odevindex >= 10) {
				for (int i = 0; i < idx; i++) {
					*d++ = *p++;
				}
				*d++ = '0' + devindex;
				_tcscpy(d, &p[idx + 1]);
			} else {
				startp[idx] = '0' + devindex;
				_tcscat(newdata, startp);
			}
			modified = true;
		} else {
			_tcscat(newdata, startp);
		}

	skip:
		bufp = next;
	}

	if (modified) {
		_tcscpy(data, newdata);
		write_log(_T("    '%s'\n -> '%s'\n"), olddata2, data);
	}
}

void inputdevice_parse_jport_custom(struct uae_prefs *pr, int index, int port, TCHAR *outname)
{
	const TCHAR *eventstr = pr->jports_custom[index].custom;
	TCHAR data[CONFIG_BLEN];
	TCHAR *bufp;
	int cnt = 0;

	custom_autoswitch_joy[index] = 0;
	custom_autoswitch_mouse[index] = 0;

	if (eventstr == NULL || eventstr[0] == 0)
		return;
	if (outname)
		outname[0] = 0;

	write_log(_T("parse_custom port %d, '%s'\n"), port, eventstr ? eventstr : _T("<NULL>"));

	_tcscpy(data, eventstr);
	_tcscat(data, _T(" "));
	bufp = data;
	for (;;) {
		const struct inputevent *ie;
		TCHAR *next = bufp;
		TCHAR devtype;
		int devindex;
		int flags = 0;
		const TCHAR *bufp2 = bufp;
		struct uae_input_device *id = 0;
		int joystick = 0;
		int devnum = 0;
		int num = -1;
		int keynum = 0;
		int idtype = -1;
		int odevindex = -1;
		const TCHAR *pbufp = NULL;
		bool skipped = false;

		while (next != NULL && *next != ' ' && *next != 0)
			next++;
		if (!next || *next == 0)
			break;
		*next++ = 0;
		TCHAR *p = getstring(&bufp2);
		if (!p)
			goto skip;

		pbufp = bufp2;
		devindex = getnum(&bufp2);
		odevindex = devindex;
		if (*bufp == 0)
			goto skip;
		if (devindex < 0 || devindex >= MAX_INPUT_DEVICES)
			goto skip;

		devtype = _totupper(*p);
		if (devtype == 'M') {
			idtype = IDTYPE_MOUSE;
		} else if (devtype == 'J') {
			idtype = IDTYPE_JOYSTICK;
		} else if (devtype == 'K') {
			idtype = IDTYPE_KEYBOARD;
		}

		if (idtype < 0) {
			goto skip;
		}

		if (idtype == IDTYPE_MOUSE) {
			id = &pr->mouse_settings[pr->input_selected_setting][devindex];
			joystick = 0;
			devnum = getdevnum(IDTYPE_MOUSE, devindex);
			if (gettype(devnum) != IDTYPE_MOUSE)
				skipped = true;
		} else if (idtype == IDTYPE_JOYSTICK) {
			id = &pr->joystick_settings[pr->input_selected_setting][devindex];
			joystick = 1;
			devnum = getdevnum(IDTYPE_JOYSTICK, devindex);
			if (gettype(devnum) != IDTYPE_JOYSTICK)
				skipped = true;
		} else if (idtype == IDTYPE_KEYBOARD) {
			// always use keyboard 0
			devindex = 0;
			id = &pr->keyboard_settings[pr->input_selected_setting][devindex];
			joystick = -1;
			devnum = getdevnum(IDTYPE_KEYBOARD, devindex);
			if (gettype(devnum) != IDTYPE_KEYBOARD) {
				write_log(_T("parse_custom keyboard missing!?\n"));
				skipped = true;
			}
		}
		if (!id) {
			goto skip;
		}

		p = getstring(&bufp2);
		if (!p)
			goto skip;

		if (joystick < 0) {
			num = getnum(&bufp2);
			if (*bufp == 0)
				goto skip;
			for (keynum = 0; keynum < MAX_INPUT_DEVICE_EVENTS; keynum++) {
				if (id->extra[keynum] == num)
					break;
			}
			if (keynum >= MAX_INPUT_DEVICE_EVENTS) {
				write_log(_T("parse_custom keyboard missing key %02x!\n"), num);
				goto skip;
			}
			num = keynum;
		} else {
			TCHAR dt = _totupper(*p);
			const struct inputdevice_functions *idf = getidf(devnum);
			num = getnum(&bufp2);
			if (dt == 'A') {
				num += idf->get_widget_first(devnum, IDEV_WIDGET_AXIS);
			} else if (dt == 'B') {
				num += idf->get_widget_first(devnum, IDEV_WIDGET_BUTTON);
			} else {
				goto skip;
			}
		}

		if (*bufp2 != '=') {
			flags = getnum(&bufp2);
		}

		while (*bufp2 != '=' && *bufp2 != 0)
			bufp2++;
		if (*bufp2 == 0)
			goto skip;
		bufp2++;

		p = getstring(&bufp2);
		if (!p)
			goto skip;

		ie = readevent(p, NULL);
		if (ie) {
			if (skipped) {
				if (outname) {
					if (outname[0] != 0)
						_tcscat(outname, _T(", "));
					const TCHAR *ps = ie->shortname ? ie->shortname : ie->name;
					_tcscat(outname, ps);
					_tcscat(outname, _T("=?"));
				}
				goto skip;
			}

			// Different port? Find matching request port event.
			if (port >= 0 && ie->unit > 0 && ie->unit != port + 1) {
				int pid = ie->portid;
				if (!pid)
					goto skip;
				for (int i = 1; events[i].name; i++) {
					const struct inputevent *ie2 = &events[i];
					if (ie2->portid == pid && ie2->unit == port + 1) {
						ie = ie2;
						break;
					}
				}
				if (ie->unit != port + 1)
					goto skip;
			}
			if (outname == NULL) {
				int evt = (int)(ie - &events[0]);
				if (joystick < 0) {
					if (port >= 0) {
						// all active keyboards
						for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
							id = &pr->keyboard_settings[pr->input_selected_setting][i];
							if (i == 0 || id->enabled) {
								setcompakbevent(pr, id, num, evt, port, 0, ID_FLAG_GAMEPORTSCUSTOM_MASK | flags);
							}
						}
					}
				} else {
					if (port >= 0) {
						uae_u64 iflags = IDEV_MAPPED_GAMEPORTSCUSTOM1 | IDEV_MAPPED_GAMEPORTSCUSTOM2 | inputdevice_flag_to_idev(flags);
						inputdevice_set_gameports_mapping(pr, devnum, num, evt, iflags, port, pr->input_selected_setting);
					}
					if (evt == INPUTEVENT_JOY1_FIRE_BUTTON || evt == INPUTEVENT_JOY2_FIRE_BUTTON) {
						if (joystick > 0)
							custom_autoswitch_joy[index] |= 1 << devindex;
						else
							custom_autoswitch_mouse[index] |= 1 << devindex;
					}
				}
			} else {
				TCHAR tmp[MAX_DPATH];
				if (outname[0] != 0)
					_tcscat(outname, _T(", "));
				const TCHAR *ps = ie->shortname ? ie->shortname : ie->name;
				if (inputdevice_get_widget_type(devnum, num, tmp, false)) {
					if (tmp[0]) {
						_tcscat(outname, tmp);
						_tcscat(outname, _T("="));
					}
				}
				_tcscat(outname, ps);
				if (flags & ID_FLAG_AUTOFIRE)
					_tcscat(outname, _T(" AF"));
			}
		} else {
			write_log(_T("parse_custom missing event %s\n"), p);
		}
skip:
		bufp = next;
	}

}

static int mouseedge_alive, mousehack_alive_cnt;
static int lastmx, lastmy;
static int lastmxy_abs[2][2];
static uaecptr magicmouse_ibase, magicmouse_gfxbase;
static int dimensioninfo_width, dimensioninfo_height, dimensioninfo_dbl;
static int vp_xoffset, vp_yoffset, mouseoffset_x, mouseoffset_y;
static int tablet_maxx, tablet_maxy, tablet_maxz;
static int tablet_resx, tablet_resy;
static int tablet_maxax, tablet_maxay, tablet_maxaz;
static int tablet_data;

int mousehack_alive (void)
{
	return mousehack_alive_cnt > 0 ? mousehack_alive_cnt : 0;
}

static uaecptr get_base (const uae_char *name)
{
	if (trap_is_indirect())
		return 0;

	uaecptr v = get_long (4);
	addrbank *b = &get_mem_bank(v);

	if (!b || !b->check (v, 400) || !(b->flags & ABFLAG_RAM))
		return 0;
	v += 378; // liblist
	while ((v = get_long (v))) {
		uae_u32 v2;
		uae_u8 *p;
		b = &get_mem_bank (v);
		if (!b || !b->check (v, 32) || !(b->flags & ABFLAG_RAM))
			goto fail;
		v2 = get_long (v + 10); // name
		b = &get_mem_bank (v2);
		if (!b || !b->check (v2, 20))
			goto fail;
		if (!(b->flags & ABFLAG_ROM) && !(b->flags & ABFLAG_RAM))
			return 0;
		p = b->xlateaddr (v2);
		if (!memcmp (p, name, strlen (name) + 1)) {
			TCHAR *s = au (name);
			write_log (_T("get_base('%s')=%08x\n"), s, v);
			xfree (s);
			return v;
		}
	}
	return 0;
fail:
	{
		TCHAR *s = au (name);
		write_log (_T("get_base('%s') failed, invalid library list\n"), s);
		xfree (s);
	}
	return 0xffffffff;
}

static uaecptr get_intuitionbase (void)
{
	if (magicmouse_ibase == 0xffffffff)
		return 0;
	if (magicmouse_ibase)
		return magicmouse_ibase;
	if (rtarea_bank.baseaddr)
		magicmouse_ibase = get_long_host(rtarea_bank.baseaddr + RTAREA_INTBASE);
	if (!magicmouse_ibase)
		magicmouse_ibase = get_base("intuition.library");
	return magicmouse_ibase;
}
static uaecptr get_gfxbase (void)
{
	if (magicmouse_gfxbase == 0xffffffff)
		return 0;
	if (magicmouse_gfxbase)
		return magicmouse_gfxbase;
	if (rtarea_bank.baseaddr)
		magicmouse_gfxbase = get_long_host(rtarea_bank.baseaddr + RTAREA_GFXBASE);
	if (!magicmouse_gfxbase)
		magicmouse_gfxbase = get_base("graphics.library");
	return magicmouse_gfxbase;
}

#define MH_E 0
#define MH_CNT 2
#define MH_MAXX 4
#define MH_MAXY 6
#define MH_MAXZ 8
#define MH_X 10
#define MH_Y 12
#define MH_Z 14
#define MH_RESX 16
#define MH_RESY 18
#define MH_MAXAX 20
#define MH_MAXAY 22
#define MH_MAXAZ 24
#define MH_AX 26
#define MH_AY 28
#define MH_AZ 30
#define MH_PRESSURE 32
#define MH_BUTTONBITS 34
#define MH_INPROXIMITY 38
#define MH_ABSX 40
#define MH_ABSY 42

#define MH_END 44
#define MH_START 4

int inputdevice_is_tablet (void)
{
	int v;
	if (uae_boot_rom_type <= 0)
		return 0;
	if (currprefs.input_tablet == TABLET_OFF)
		return 0;
	if (currprefs.input_tablet == TABLET_MOUSEHACK)
		return -1;
	v = is_tablet ();
	if (!v)
		return 0;
	if (kickstart_version < 37)
		return v ? -1 : 0;
	return v ? 1 : 0;
}

static uae_u8 *mousehack_address;
static bool mousehack_enabled;

static void mousehack_reset (void)
{
	dimensioninfo_width = dimensioninfo_height = 0;
	mouseoffset_x = mouseoffset_y = 0;
	dimensioninfo_dbl = 0;
	mousehack_alive_cnt = 0;
	vp_xoffset = vp_yoffset = 0;
	tablet_data = 0;
	if (rtarea_bank.baseaddr) {
		put_long_host(rtarea_bank.baseaddr + RTAREA_INTXY, 0xffffffff);
		if (mousehack_address && mousehack_address >= rtarea_bank.baseaddr && mousehack_address < rtarea_bank.baseaddr + 0x10000)
			put_byte_host(mousehack_address + MH_E, 0);
	}
	mousehack_address = 0;
	mousehack_enabled = false;
}

static bool mousehack_enable (void)
{
	int mode;

	if (uae_boot_rom_type <= 0 || currprefs.input_tablet == TABLET_OFF)
		return false;
	if (mousehack_address && mousehack_enabled)
		return true;
	mode = 0x80;
	if (currprefs.input_tablet == TABLET_MOUSEHACK)
		mode |= 1;
	if (inputdevice_is_tablet () > 0)
		mode |= 2;
	if (mousehack_address && rtarea_bank.baseaddr) {
		write_log (_T("Mouse driver enabled (%s)\n"), ((mode & 3) == 3 ? _T("tablet+mousehack") : ((mode & 3) == 2) ? _T("tablet") : _T("mousehack")));
		put_byte_host(mousehack_address + MH_E, mode);
		mousehack_enabled = true;
	}
	return true;
}

static void inputdevice_update_tablet_params(void)
{
	uae_u8 *p;
	if (inputdevice_is_tablet() <= 0 || !mousehack_address)
		return;
	p = mousehack_address;

	p[MH_MAXX] = tablet_maxx >> 8;
	p[MH_MAXX + 1] = tablet_maxx;
	p[MH_MAXY] = tablet_maxy >> 8;
	p[MH_MAXY + 1] = tablet_maxy;
	p[MH_MAXZ] = tablet_maxz >> 8;
	p[MH_MAXZ + 1] = tablet_maxz;

	p[MH_RESX] = tablet_resx >> 8;
	p[MH_RESX + 1] = tablet_resx;
	p[MH_RESY] = tablet_resy >> 8;
	p[MH_RESY + 1] = tablet_resy;

	p[MH_MAXAX] = tablet_maxax >> 8;
	p[MH_MAXAX + 1] = tablet_maxax;
	p[MH_MAXAY] = tablet_maxay >> 8;
	p[MH_MAXAY + 1] = tablet_maxay;
	p[MH_MAXAZ] = tablet_maxaz >> 8;
	p[MH_MAXAZ + 1] = tablet_maxaz;
}

void input_mousehack_mouseoffset(uaecptr pointerprefs)
{
	mouseoffset_x = (uae_s16)get_word (pointerprefs + 28);
	mouseoffset_y = (uae_s16)get_word (pointerprefs + 30);
}

static bool get_mouse_position(int *xp, int *yp, int inx, int iny)
{
	int monid = 0;
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	struct amigadisplay *ad = &adisplays[monid];
	struct picasso96_state_struct *state = &picasso96_state[monid];
	int x, y;
	float fdy, fdx, fmx, fmy;
	bool ob = false;

	x = inx;
	y = iny;

	getgfxoffset(monid, &fdx, &fdy, &fmx, &fmy);

	//write_log("%.2f*%.2f %.2f*%.2f\n", fdx, fdy, fmx, fmy);

#ifdef PICASSO96
	if (ad->picasso_on) {
		x -= state->XOffset;
		y -= state->YOffset;
		x = (int)(x * fmx);
		y = (int)(y * fmy);
		x += (int)(fdx * fmx);
		y += (int)(fdy * fmy);
	} else
#endif
	{
		if (vidinfo->outbuffer == NULL) {
			*xp = 0;
			*yp = 0;
			return false;
		}
		x = (int)(x * fmx);
		y = (int)(y * fmy);
		x -= (int)(fdx * fmx) - 1;
		y -= (int)(fdy * fmy) - 2;
		x = coord_native_to_amiga_x(x);
		if (y >= 0) {
			y = coord_native_to_amiga_y(y) * 2;
		}
		if (x < 0 || y < 0 || x >= vidinfo->outbuffer->outwidth || y >= vidinfo->outbuffer->outheight) {
			ob = true;
		}
	}
	*xp = x;
	*yp = y;
	return ob == false;
}

void mousehack_wakeup(void)
{
	if (mousehack_alive_cnt == 0)
		mousehack_alive_cnt = -100;
	else if (mousehack_alive_cnt > 0)
		mousehack_alive_cnt = 100;
	if (uaeboard_bank.baseaddr) {
		uaeboard_bank.baseaddr[0x201] &= ~3;
	}
}

int input_mousehack_status(TrapContext *ctx, int mode, uaecptr diminfo, uaecptr dispinfo, uaecptr vp, uae_u32 moffset)
{
	if (mode == 4) {
		return mousehack_enable () ? 1 : 0;
	} else if (mode == 5) {
		mousehack_address = (trap_get_dreg(ctx, 0) & 0xffff) + rtarea_bank.baseaddr;
		mousehack_enable ();
		inputdevice_update_tablet_params ();
	} else if (mode == 0) {
		if (mousehack_address) {
			uae_u8 v = get_byte_host(mousehack_address + MH_E);
			v |= 0x40;
			put_byte_host(mousehack_address + MH_E, v);
			write_log (_T("Tablet driver running (%p,%02x)\n"), mousehack_address, v);
		}
	} else if (mode == 1) {
		int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
		uae_u32 props = 0;
		dimensioninfo_width = -1;
		dimensioninfo_height = -1;
		vp_xoffset = 0;
		vp_yoffset = 0;
		if (diminfo) {
			x1 = trap_get_word(ctx, diminfo + 50);
			y1 = trap_get_word(ctx, diminfo + 52);
			x2 = trap_get_word(ctx, diminfo + 54);
			y2 = trap_get_word(ctx, diminfo + 56);
			dimensioninfo_width = x2 - x1 + 1;
			dimensioninfo_height = y2 - y1 + 1;
		}
		if (vp) {
			vp_xoffset = trap_get_word(ctx, vp + 28);
			vp_yoffset = trap_get_word(ctx, vp + 30);
		}
		if (dispinfo)
			props = trap_get_long(ctx, dispinfo + 18);
		dimensioninfo_dbl = (props & 0x00020000) ? 1 : 0;
		write_log (_T("%08x %08x %08x (%dx%d)-(%dx%d) d=%dx%d %s\n"),
			diminfo, props, vp, x1, y1, x2, y2, vp_xoffset, vp_yoffset,
			(props & 0x00020000) ? _T("dbl") : _T(""));
	}
	return 1;
}

void inputdevice_tablet_strobe (void)
{
	mousehack_enable ();
	if (uae_boot_rom_type <= 0)
		return;
	if (!tablet_data)
		return;
	if (mousehack_address)
		put_byte_host(mousehack_address + MH_CNT, get_byte_host(mousehack_address + MH_CNT) + 1);
}

int inputdevice_get_lightpen_id(void)
{
#ifdef ARCADIA
	if (!alg_flag) {
		if (lightpen_enabled2) 
			return alg_get_player(potgo_value);
		return -1;
	} else {
		return alg_get_player(potgo_value);
	}
#endif
}

void tablet_lightpen(int tx, int ty, int tmaxx, int tmaxy, int touch, int buttonmask, bool touchmode, int devid, int lpnum)
{
	int monid = 0;
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	struct amigadisplay *ad = &adisplays[monid];
	int dw, dh, ax, ay, aw, ah;
	float fx, fy;
	float xmult, ymult;
	float fdx, fdy, fmx, fmy;
	int x, y;

	if (ad->picasso_on)
		goto end;

	if (vidinfo->outbuffer == NULL)
		goto end;

	if (touch < 0)
		goto end;

	fx = (float)tx;
	fy = (float)ty;

	desktop_coords (0, &dw, &dh, &ax, &ay, &aw, &ah);

	if (tmaxx < 0 || tmaxy < 0) {
		tmaxx = dw;
		tmaxy = dh;
	} else if (tmaxx == 0 || tmaxy == 0) {
		tmaxx = aw;
		tmaxy = ah;
	}

	if (!touchmode) {
		dw = aw;
		dh = ah;
		ax = 0;
		ay = 0;
	}

	xmult = (float)tmaxx / dw;
	ymult = (float)tmaxy / dh;

	fx = fx / xmult;
	fy = fy / ymult;

	fx -= ax;
	fy -= ay;

	getgfxoffset(0, &fdx, &fdy, &fmx, &fmy);

	x = (int)(fx * fmx);
	y = (int)(fy * fmy);
	x -= (int)(fdx * fmx) - 1;
	y -= (int)(fdy * fmy) - 2;

	if (x < 0 || y < 0 || x >= aw || y >= ah)
		goto end;

	if (lpnum < 0) {
		lightpen_x[0] = x;
		lightpen_y[0] = y;
		lightpen_x[1] = x;
		lightpen_y[1] = y;
	} else {
		lightpen_x[lpnum] = x;
		lightpen_y[lpnum] = y;
	}

	if (touch >= 0)
		lightpen_active |= 1;

	if (touch > 0 && devid >= 0) {
		setmousebuttonstate (devid, 0, 1);
	}

	lightpen_enabled = true;
	return;

end:
	if (lightpen_active) {
		lightpen_active &= ~1;
		if (devid >= 0)
			setmousebuttonstate (devid, 0, 0);
	}

}

void inputdevice_tablet (int x, int y, int z, int pressure, uae_u32 buttonbits, int inproximity, int ax, int ay, int az, int devid)
{
	if (is_touch_lightpen()) {

		tablet_lightpen(x, y, tablet_maxx, tablet_maxy, inproximity ? 1 : -1, buttonbits, false, devid, -1);

	} else {
		uae_u8 *p;
		uae_u8 tmp[MH_END];

		mousehack_enable ();
		if (inputdevice_is_tablet () <= 0 || !mousehack_address)
			return;
		//write_log (_T("%d %d %d %d %08X %d %d %d %d\n"), x, y, z, pressure, buttonbits, inproximity, ax, ay, az);
		p = mousehack_address;

		memcpy (tmp, p + MH_START, MH_END - MH_START); 
#if 0
		if (currprefs.input_magic_mouse) {
			int maxx, maxy, diffx, diffy;
			int dw, dh, ax, ay, aw, ah;
			float xmult, ymult;
			float fx, fy;

			fx = (float)x;
			fy = (float)y;
			desktop_coords (&dw, &dh, &ax, &ay, &aw, &ah);
			xmult = (float)tablet_maxx / dw;
			ymult = (float)tablet_maxy / dh;

			diffx = 0;
			diffy = 0;
			if (picasso_on) {
				maxx = gfxvidinfo.width;
				maxy = gfxvidinfo.height;
			} else {
				get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy);
			}
			diffx += ax;
			diffy += ah;

			fx -= diffx * xmult;
			if (fx < 0)
				fx = 0;
			if (fx >= aw * xmult)
				fx = aw * xmult - 1;
			fy -= diffy * ymult;
			if (fy < 0)
				fy = 0;
			if (fy >= ah * ymult)
				fy = ah * ymult - 1;

			x = (int)(fx * (aw * xmult) / tablet_maxx + 0.5);
			y = (int)(fy * (ah * ymult) / tablet_maxy + 0.5);

		}
#endif
		p[MH_X] = x >> 8;
		p[MH_X + 1] = x;
		p[MH_Y] = y >> 8;
		p[MH_Y + 1] = y;
		p[MH_Z] = z >> 8;
		p[MH_Z + 1] = z;

		p[MH_AX] = ax >> 8;
		p[MH_AX + 1] = ax;
		p[MH_AY] = ay >> 8;
		p[MH_AY + 1] = ay;
		p[MH_AZ] = az >> 8;
		p[MH_AZ + 1] = az;

		p[MH_PRESSURE] = pressure >> 8;
		p[MH_PRESSURE + 1] = pressure;

		p[MH_BUTTONBITS + 0] = buttonbits >> 24;
		p[MH_BUTTONBITS + 1] = buttonbits >> 16;
		p[MH_BUTTONBITS + 2] = buttonbits >>  8;
		p[MH_BUTTONBITS + 3] = buttonbits >>  0;

		if (inproximity < 0) {
			p[MH_INPROXIMITY] = p[MH_INPROXIMITY + 1] = 0xff;
		} else {
			p[MH_INPROXIMITY] = 0;
			p[MH_INPROXIMITY + 1] = inproximity ? 1 : 0;
		}

		if (!memcmp (tmp, p + MH_START, MH_END - MH_START))
			return;

		//if (tablet_log & 1) {
		//	static int obuttonbits, oinproximity;
		//	if (inproximity != oinproximity || buttonbits != obuttonbits) {
		//		obuttonbits = buttonbits;
		//		oinproximity = inproximity;
		//		write_log (_T("TABLET: B=%08x P=%d\n"), buttonbits, inproximity);
		//	}
		//}
		//if (tablet_log & 2) {
		//	write_log (_T("TABLET: X=%d Y=%d Z=%d AX=%d AY=%d AZ=%d\n"), x, y, z, ax, ay, az);
		//}

		p[MH_E] = 0xc0 | 2;
		p[MH_CNT]++;
	}
}

void inputdevice_tablet_info (int maxx, int maxy, int maxz, int maxax, int maxay, int maxaz, int xres, int yres)
{
	tablet_maxx = maxx;
	tablet_maxy = maxy;
	tablet_maxz = maxz;

	tablet_resx = xres;
	tablet_resy = yres;
	tablet_maxax = maxax;
	tablet_maxay = maxay;
	tablet_maxaz = maxaz;
	inputdevice_update_tablet_params();
}

static void inputdevice_mh_abs (int x, int y, uae_u32 buttonbits)
{
	x -= mouseoffset_x + 1;
	y -= mouseoffset_y + 2;

	mousehack_enable ();
	if (mousehack_address) {
		uae_u8 tmp1[4], tmp2[4];
		uae_u8 *p = mousehack_address;

		memcpy (tmp1, p + MH_ABSX, sizeof tmp1);
		memcpy (tmp2, p + MH_BUTTONBITS, sizeof tmp2);

		//write_log (_T("%04dx%04d %08x\n"), x, y, buttonbits);

		p[MH_ABSX] = x >> 8;
		p[MH_ABSX + 1] = x;
		p[MH_ABSY] = y >> 8;
		p[MH_ABSY + 1] = y;

		p[MH_BUTTONBITS + 0] = buttonbits >> 24;
		p[MH_BUTTONBITS + 1] = buttonbits >> 16;
		p[MH_BUTTONBITS + 2] = buttonbits >>  8;
		p[MH_BUTTONBITS + 3] = buttonbits >>  0;

		if (!memcmp (tmp1, p + MH_ABSX, sizeof tmp1) && !memcmp (tmp2, p + MH_BUTTONBITS, sizeof tmp2))
			return;
		p[MH_E] = 0xc0 | 1;
		p[MH_CNT]++;
		tablet_data = 1;
#if 0
		if (inputdevice_is_tablet () <= 0) {
			tabletlib_tablet_info (1000, 1000, 0, 0, 0, 0, 1000, 1000);
			tabletlib_tablet (x, y, 0, 0, buttonbits, -1, 0, 0, 0);
		}
#endif
	}
#if 0
	if (uaeboard_bank.baseaddr) {
		uae_u8 tmp[16];

/*

	00 W Misc bit flags
		- Bit 0 = mouse is currently out of Amiga window bounds
		  (Below mouse X / Y value can be out of bounds, negative or too large)
		- Bit 1 = mouse coordinate / button state changed
		- Bit 2 = window size changed
		- Bit 15 = bit set = activated
	02 W PC Mouse absolute X position relative to emulation top / left corner
	04 W PC Mouse absolute Y position relative to emulation top / left corner
	06 W PC Mouse relative wheel (counter that counts up or down)
	08 W PC Mouse relative horizontal wheel (same as above)
	0A W PC Mouse button flags (bit 0 set: left button pressed, bit 1 = right, and so on)

	10 W PC emulation window width (Host OS pixels)
	12 W PC emulation window height (Host OS pixels)
	14 W RTG hardware emulation width (Amiga pixels)
	16 W RTG hardware emulation height (Amiga pixels)
	
	20 W Amiga Mouse X (write-only)
	22 W Amiga Mouse Y (write-only)

	Big endian. All read only except last two.
	Changed bit = bit automatically clears when Misc value is read.

*/

		uae_u8 *p = uaeboard_bank.baseaddr + 0x200;
		memcpy(tmp, p + 2, 2 * 5);

		// status
		p[0] = 0x00 | (currprefs.input_tablet != 0 ? 0x80 : 0x00);
		p[1] = 0x00;
		// host x
		p[2 * 1 + 0] = x >> 8;
		p[2 * 1 + 1] = (uae_u8)x;
		// host y
		p[2 * 2 + 0] = y >> 8;
		p[2 * 2 + 1] = (uae_u8)y;
		// host wheel
		p[2 * 3 + 0] = 0;
		p[2 * 3 + 1] = 0;
		// host hwheel
		p[2 * 4 + 0] = 0;
		p[2 * 4 + 1] = 0;
		// buttons
		p[2 * 5 + 0] = buttonbits >> 8;
		p[2 * 5 + 1] = (uae_u8)buttonbits;

		// mouse state changed?
		if (memcmp(tmp, p + 2, 2 * 5)) {
			p[1] |= 2;
		}

		p += 0x10;
		memcpy(tmp, p, 2 * 4);
		if (picasso_on) {
			// RTG host resolution width
			p[0 * 2 + 0] = picasso_vidinfo.width >> 8;
			p[0 * 2 + 1] = (uae_u8)picasso_vidinfo.width;
			// RTG host resolution height
			p[1 * 2 + 0] = picasso_vidinfo.height >> 8;
			p[1 * 2 + 1] = (uae_u8)picasso_vidinfo.height;
			// RTG resolution width
			p[2 * 2 + 0] = picasso96_state.Width >> 8;
			p[2 * 2 + 1] = (uae_u8)picasso96_state.Width;
			// RTG resolution height
			p[3 * 2 + 0] = picasso96_state.Height >> 8;
			p[3 * 2 + 1] = (uae_u8)picasso96_state.Height;
		} else {
			p[2 * 0 + 0] = 0;
			p[2 * 0 + 1] = 0;
			p[2 * 1 + 0] = 0;
			p[2 * 1 + 1] = 0;
			p[2 * 2 + 0] = 0;
			p[2 * 2 + 1] = 0;
			p[2 * 3 + 0] = 0;
			p[2 * 3 + 1] = 0;
		}

		// display size changed?
		if (memcmp(tmp, p, 2 * 4)) {
			p[1] |= 4;
		}
	}
#endif
}

#if 0
void mousehack_write(int reg, uae_u16 val)
{
	switch (reg)
	{
		case 0x20:
		write_log(_T("Mouse X = %d\n"), val);
		break;
		case 0x22:
		write_log(_T("Mouse Y = %d\n"), val);
		break;
	}
}
#endif

#if 0
static void inputdevice_mh_abs_v36 (int x, int y)
{
	uae_u8 *p;
	uae_u8 tmp[MH_END];
	uae_u32 off;
	int maxx, maxy, diffx, diffy;
	int fdy, fdx, fmx, fmy;

	mousehack_enable ();
	off = getmhoffset ();
	p = rtarea + off;

	memcpy (tmp, p + MH_START, MH_END - MH_START); 

	getgfxoffset (&fdx, &fdy, &fmx, &fmy);
	x -= fdx;
	y -= fdy;
	x += vp_xoffset;
	y += vp_yoffset;

	diffx = diffy = 0;
	maxx = maxy = 0;

	if (picasso_on) {
		maxx = picasso96_state.Width;
		maxy = picasso96_state.Height;
	} else if (dimensioninfo_width > 0 && dimensioninfo_height > 0) {
		maxx = dimensioninfo_width;
		maxy = dimensioninfo_height;
		get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy, dimensioninfo_dbl);
	} else {
		uaecptr gb = get_gfxbase ();
		maxx = 0; maxy = 0;
		if (gb) {
			maxy = get_word (gb + 216);
			maxx = get_word (gb + 218);
		}
		get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy, 0);
	}
#if 0
	{
		uaecptr gb = get_intuitionbase ();
		maxy = get_word (gb + 1344 + 2);
		maxx = get_word (gb + 1348 + 2);
		write_log (_T("%d %d\n"), maxx, maxy);
	}
#endif
#if 1
	{
		uaecptr gb = get_gfxbase ();
		uaecptr view = get_long (gb + 34);
		if (view) {
			uaecptr vp = get_long (view);
			if (vp) {
				int w, h, dw, dh;
				w = get_word (vp + 24);
				h = get_word (vp + 26) * 2;
				dw = get_word (vp + 28);
				dh = get_word (vp + 30);
				//write_log (_T("%d %d %d %d\n"), w, h, dw, dh);
				if (w < maxx)
					maxx = w;
				if (h < maxy)
					maxy = h;
				x -= dw;
				y -= dh;
			}
		}
		//write_log (_T("* %d %d\n"), get_word (gb + 218), get_word (gb + 216));
	}
	//write_log (_T("%d %d\n"), maxx, maxy);
#endif

	maxx = maxx * 1000 / fmx;
	maxy = maxy * 1000 / fmy;

	if (maxx <= 0)
		maxx = 1;
	if (maxy <= 0)
		maxy = 1;

	x -= diffx;
	if (x < 0)
		x = 0;
	if (x >= maxx)
		x = maxx - 1;

	y -= diffy;
	if (y < 0)
		y = 0;
	if (y >= maxy)
		y = maxy - 1;

	//write_log (_T("%d %d %d %d\n"), x, y, maxx, maxy);

	p[MH_X] = x >> 8;
	p[MH_X + 1] = x;
	p[MH_Y] = y >> 8;
	p[MH_Y + 1] = y;
	p[MH_MAXX] = maxx >> 8;
	p[MH_MAXX + 1] = maxx;
	p[MH_MAXY] = maxy >> 8;
	p[MH_MAXY + 1] = maxy;

	p[MH_Z] = p[MH_Z + 1] = 0;
	p[MH_MAXZ] = p[MH_MAXZ + 1] = 0;
	p[MH_AX] = p[MH_AX + 1] = 0;
	p[MH_AY] = p[MH_AY + 1] = 0;
	p[MH_AZ] = p[MH_AZ + 1] = 0;
	p[MH_PRESSURE] = p[MH_PRESSURE + 1] = 0;
	p[MH_INPROXIMITY] = p[MH_INPROXIMITY + 1] = 0xff;

	if (!memcmp (tmp, p + MH_START, MH_END - MH_START))
		return;
	p[MH_CNT]++;
	tablet_data = 1;
}
#endif

static void mousehack_helper (uae_u32 buttonmask)
{
	int x, y;
	//write_log (_T("mousehack_helper %08X\n"), buttonmask);

	if (quit_program) {
		return;
	}
	if (!(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) && currprefs.input_tablet < TABLET_MOUSEHACK) {
		return;
	}

	get_mouse_position(&x, &y, lastmx, lastmy);
	inputdevice_mh_abs(x, y, buttonmask);
}

static int mouseedge_x, mouseedge_y, mouseedge_time;
#define MOUSEEDGE_RANGE 100
#define MOUSEEDGE_TIME 2

static int mouseedge(int monid)
{
	struct amigadisplay *ad = &adisplays[monid];
	int x, y, dir;
	uaecptr ib;
	static int melast_x, melast_y;
	static int isnonzero;

	if (!(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) || currprefs.input_tablet > 0)
		return 0;
	if (magicmouse_ibase == 0xffffffff)
		return 0;
	if (joybutton[0] || joybutton[1])
		return 0;
	dir = 0;
	if (!mouseedge_time) {
		isnonzero = 0;
		goto end;
	}
	ib = get_intuitionbase ();
	if (!ib)
		return 0;
	if (trap_is_indirect()) {
		uae_u32 xy = get_long_host(rtarea_bank.baseaddr + RTAREA_INTXY);
		if (xy == 0xffffffff)
			return 0;
		y = xy >> 16;
		x = xy & 0xffff;
	} else {
		if (get_word(ib + 20) < 31) // version < 31
			return 0;
		x = get_word(ib + 70);
		y = get_word(ib + 68);
	}

	//write_log("%dx%d\n", x, y);

	if (x || y)
		isnonzero = 1;
	if (!isnonzero)
		return 0;
	if (melast_x == x) {
		if (mouseedge_x < -MOUSEEDGE_RANGE) {
			mouseedge_x = 0;
			dir |= 1;
			goto end;
		}
		if (mouseedge_x > MOUSEEDGE_RANGE) {
			mouseedge_x = 0;
			dir |= 2;
			goto end;
		}
	} else {
		mouseedge_x = 0;
		melast_x = x;
	}
	if (melast_y == y) {
		if (mouseedge_y < -MOUSEEDGE_RANGE) {
			mouseedge_y = 0;
			dir |= 4;
			goto end;
		}
		if (mouseedge_y > MOUSEEDGE_RANGE) {
			mouseedge_y = 0;
			dir |= 8;
			goto end;
		}
	} else {
		mouseedge_y = 0;
		melast_y = y;
	}
	return 1;

end:
	mouseedge_time = 0;
	if (dir) {
		if (!ad->picasso_on) {
			int aw = 0, ah = 0, dx, dy;
			get_custom_mouse_limits(&aw, &ah, &dx, &dy, dimensioninfo_dbl);
			x += dx;
			y += dy;
			float dx2, dy2, mx2, my2;
			getgfxoffset(monid, &dx2, &dy2, &mx2, &my2);
			if (mx2) {
				x = (int)(x / mx2);
			}
			if (my2) {
				y = (int)(y / my2);
			}
			x += (int)dx2;
			y += (int)dy2;
		} else {
			float dx, dy, mx, my;
			getgfxoffset(monid, &dx, &dy, &mx, &my);
			if (mx) {
				x = (int)(x / mx);
			}
			if (my) {
				y = (int)(y / my);
			}
			x -= (int)dx;
			y -= (int)dy;
		}
		if (!dmaen(DMA_SPRITE) && !ad->picasso_on) {
			setmouseactivexy(0, x, y, 0);
		} else {
			setmouseactivexy(0, x, y, dir);
		}
	}
	return 1;
}

int magicmouse_alive (void)
{
	return mouseedge_alive > 0;
}

static int getbuttonstate (int joy, int button)
{
	return (joybutton[joy] & (1 << button)) ? 1 : 0;
}

static int pc_mouse_buttons[MAX_JPORTS];

static int getvelocity (int num, int subnum, int pct)
{
	if (pct > 1000)
		pct = 1000;
	if (pct < 0) {
		pct = 0;
	}
	int val = mouse_delta[num][subnum];
	int v = val * pct / 1000;
	if (!v) {
		if (val < -maxvpos / 2)
			v = -2;
		else if (val < 0)
			v = -1;
		else if (val > maxvpos / 2)
			v = 2;
		else if (val > 0)
			v = 1;
	}
	if (!mouse_deltanoreset[num][subnum]) {
		mouse_delta[num][subnum] -= v;
		//gui_gameport_axis_change (num, subnum * 2 + 0, 0, -1);
		//gui_gameport_axis_change (num, subnum * 2 + 1, 0, -1);
	}
	return v;
}

#define MOUSEXY_MAX 16384

static void mouseupdate (int pct, bool vsync)
{
	int max = 120;
	bool pcmouse = false;
	static int mxd, myd;

	if (vsync) {
#ifdef WITH_DRACO
		if (
#ifdef WITH_X86
			x86_mouse(0, 0, 0, 0, -1) ||
#endif
			draco_mouse(0, 0, 0, 0, -1)) {
			pcmouse = true;
			pct = 1000;
		}
#endif

		if (mxd < 0) {
			if (mouseedge_x > 0)
				mouseedge_x = 0;
			else
				mouseedge_x += mxd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (mxd > 0) {
			if (mouseedge_x < 0)
				mouseedge_x = 0;
			else
				mouseedge_x += mxd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (myd < 0) {
			if (mouseedge_y > 0)
				mouseedge_y = 0;
			else
				mouseedge_y += myd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (myd > 0) {
			if (mouseedge_y < 0)
				mouseedge_y = 0;
			else
				mouseedge_y += myd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (mouseedge_time > 0) {
			mouseedge_time--;
			if (mouseedge_time == 0) {
				mouseedge_x = 0;
				mouseedge_y = 0;
			}
		}
		mxd = 0;
		myd = 0;
	}

	for (int i = 0; i < 2; i++) {

		if (mouse_port[i]) {

			int v1 = getvelocity (i, 0, pct);
			mxd += v1;
			mouse_x[i] += v1;
			if (mouse_x[i] < 0) {
				mouse_x[i] += MOUSEXY_MAX;
				mouse_frame_x[i] = mouse_x[i] - v1;
			}
			if (mouse_x[i] >= MOUSEXY_MAX) {
				mouse_x[i] -= MOUSEXY_MAX;
				mouse_frame_x[i] = mouse_x[i] - v1;
			}

			int v2 = getvelocity (i, 1, pct);
			myd += v2;
			mouse_y[i] += v2;
			if (mouse_y[i] < 0) {
				mouse_y[i] += MOUSEXY_MAX;
				mouse_frame_y[i] = mouse_y[i] - v2;
			}
			if (mouse_y[i] >= MOUSEXY_MAX) {
				mouse_y[i] -= MOUSEXY_MAX;
				mouse_frame_y[i] = mouse_y[i] - v2;
			}

			int v3 = getvelocity (i, 2, pct);
			/* if v != 0, record mouse wheel key presses
			 * according to the NewMouse standard */
			if (v3 > 0)
				record_key(0x7a << 1, true);
			else if (v3 < 0)
				record_key(0x7b << 1, true);
			if (!mouse_deltanoreset[i][2])
				mouse_delta[i][2] = 0;

		if (pcmouse) {
			if (getbuttonstate(i, JOYBUTTON_1))
				pc_mouse_buttons[i] |= 1;
			else
				pc_mouse_buttons[i] &= ~1;
			if (getbuttonstate(i, JOYBUTTON_2))
				pc_mouse_buttons[i] |= 2;
			else
				pc_mouse_buttons[i] &= ~2;
			if (getbuttonstate(i, JOYBUTTON_3))
				pc_mouse_buttons[i] |= 4;
			else
				pc_mouse_buttons[i] &= ~4;
#ifdef WITH_X86
				x86_mouse(0, v1, v2, v3, pc_mouse_buttons[i]);
#endif
#ifdef WITH_DRACO
				draco_mouse(0, v1, v2, v3, pc_mouse_buttons[i]);
#endif
			}


#if OUTPUTDEBUG
			if (v1 || v2) {
				TCHAR xx1[256];
				_stprintf(xx1, _T("%p %d VX=%d VY=%d X=%d Y=%d DX=%d DY=%d VS=%d\n"),
					GetCurrentProcess(), timeframes, v1, v2, mouse_x[i], mouse_y[i], mouse_frame_x[i] - mouse_x[i], mouse_frame_y[i] - mouse_y[i], vsync);
				OutputDebugString(xx1);
			}
#endif

			if (mouse_frame_x[i] - mouse_x[i] > max) {
				mouse_x[i] = mouse_frame_x[i] - max;
				mouse_x[i] &= MOUSEXY_MAX - 1;
			}
			if (mouse_frame_x[i] - mouse_x[i] < -max) {
				mouse_x[i] = mouse_frame_x[i] + max;
				mouse_x[i] &= MOUSEXY_MAX - 1;
			}

			if (mouse_frame_y[i] - mouse_y[i] > max) {
				mouse_y[i] = mouse_frame_y[i] - max;
				mouse_y[i] &= MOUSEXY_MAX - 1;
			}
			if (mouse_frame_y[i] - mouse_y[i] < -max) {
				mouse_y[i] = mouse_frame_y[i] + max;
				mouse_y[i] &= MOUSEXY_MAX - 1;
			}
		}

		if (!vsync) {
			mouse_frame_x[i] = mouse_x[i];
			mouse_frame_y[i] = mouse_y[i];
		}

	}

	for (int i = 0; i < 2; i++) {
		if (lightpen_delta[i][0]) {
			lightpen_x[i] += lightpen_delta[i][0];
			if (!lightpen_deltanoreset[i][0])
				lightpen_delta[i][0] = 0;
		}
		if (lightpen_delta[i][1]) {
			lightpen_y[i] += lightpen_delta[i][1];
			if (!lightpen_deltanoreset[i][1])
				lightpen_delta[i][1] = 0;
		}
	}

}

static uae_u32 prev_input_vpos, input_frame, prev_input_frame;
extern int vpos;
static void readinput (void)
{
	int max = current_maxvpos();
	int diff = (input_frame * max + vpos) - (prev_input_frame * max + prev_input_vpos);
	if (diff > 0) {
		if (diff < 10) {
			mouseupdate (0, false);
		} else {
			mouseupdate (diff * 1000 / current_maxvpos (), false);
		}
	}
	prev_input_frame = input_frame;
	prev_input_vpos = vpos;
}

static void joymousecounter (int joy)
{
	int left = 1, right = 1, top = 1, bot = 1;
	int b9, b8, b1, b0;
	int cntx, cnty, ocntx, ocnty;

	if (joydir[joy] & DIR_LEFT)
		left = 0;
	if (joydir[joy] & DIR_RIGHT)
		right = 0;
	if (joydir[joy] & DIR_UP)
		top = 0;
	if (joydir[joy] & DIR_DOWN)
		bot = 0;

	b0 = (bot ^ right) ? 1 : 0;
	b1 = (right ^ 1) ? 2 : 0;
	b8 = (top ^ left) ? 1 : 0;
	b9 = (left ^ 1) ? 2 : 0;

	cntx = b0 | b1;
	cnty = b8 | b9;
	ocntx = mouse_x[joy] & 3;
	ocnty = mouse_y[joy] & 3;

	if (cntx == 3 && ocntx == 0)
		mouse_x[joy] -= 4;
	else if (cntx == 0 && ocntx == 3)
		mouse_x[joy] += 4;
	mouse_x[joy] = (mouse_x[joy] & 0xfc) | cntx;

	if (cnty == 3 && ocnty == 0)
		mouse_y[joy] -= 4;
	else if (cnty == 0 && ocnty == 3)
		mouse_y[joy] += 4;
	mouse_y[joy] = (mouse_y[joy] & 0xfc) | cnty;

	if (!left || !right || !top || !bot) {
		mouse_frame_x[joy] = mouse_x[joy];
		mouse_frame_y[joy] = mouse_y[joy];
	}
}

static int inputread;

void inputdevice_read_msg(bool vblank)
{
	int got2 = 0;
	for (;;) {
		int got = handle_msgpump(vblank);
		if (!got)
			break;
		got2 = 1;
	}
}

static void inputdevice_read(void)
{
//	if ((inputdevice_logging & (2 | 4)))
//		write_log(_T("INPUTREAD\n"));
	inputdevice_read_msg(false);
#ifndef AMIBERRY // we handle input device reading based on SDL2 events in Amiberry
	if (inputread <= 0) {
		idev[IDTYPE_MOUSE].read();
		idev[IDTYPE_JOYSTICK].read();
		idev[IDTYPE_KEYBOARD].read();
	}
#endif
}

static void maybe_read_input(void)
{
	if (inputread >= 0 && (vpos - inputread) <= maxvpos_display / 3) {
		return;
	}
	if (input_record || savestate_state == STATE_DOSAVE) {
		return;
	}
	inputread = vpos;
	inputdevice_read();
}

static uae_u16 getjoystate (int joy)
{
	uae_u16 v;

	maybe_read_input();

	v = (uae_u8)mouse_x[joy] | (mouse_y[joy] << 8);
#if DONGLE_DEBUG
	if (notinrom ())
		write_log (_T("JOY%dDAT %04X %s\n"), joy, v, debuginfo (0));
#endif
	if (inputdevice_logging & 2)
		write_log (_T("JOY%dDAT=%04x %08x\n"), joy, v, M68K_GETPC);
	return v;
}

uae_u16 JOY0DAT (void)
{
	uae_u16 v;
	readinput ();
	v = getjoystate (0);
	v = dongle_joydat (0, v);
#ifdef ARCADIA
	v = alg_joydat(0, v);
#endif
	return v;
}

uae_u16 JOY1DAT (void)
{
	uae_u16 v;
	readinput ();
	v = getjoystate (1);
	v = dongle_joydat (1, v);
#ifdef ARCADIA
	v = alg_joydat(1, v);
#endif
	return v;
}

uae_u16 JOYGET (int num)
{
	uae_u16 v;
	v = getjoystate (num);
	v = dongle_joydat (num, v);
	return v;
}

void JOYSET (int num, uae_u16 dat)
{
	mouse_x[num] = dat & 0xff;
	mouse_y[num] = (dat >> 8) & 0xff;
	mouse_frame_x[num] = mouse_x[num];
	mouse_frame_y[num] = mouse_y[num];
}

void JOYTEST (uae_u16 v)
{
	mouse_x[0] &= 3;
	mouse_y[0] &= 3;
	mouse_x[1] &= 3;
	mouse_y[1] &= 3;
	mouse_x[0] |= v & 0xFC;
	mouse_x[1] |= v & 0xFC;
	mouse_y[0] |= (v >> 8) & 0xFC;
	mouse_y[1] |= (v >> 8) & 0xFC;
	mouse_frame_x[0] = mouse_x[0];
	mouse_frame_y[0] = mouse_y[0];
	mouse_frame_x[1] = mouse_x[1];
	mouse_frame_y[1] = mouse_y[1];
	dongle_joytest (v);
	if (inputdevice_logging & 2)
		write_log (_T("JOYTEST: %04X PC=%x\n"), v , M68K_GETPC);
}

static uae_u8 parconvert (uae_u8 v, int jd, int shift)
{
	if (jd & DIR_UP)
		v &= ~(1 << shift);
	if (jd & DIR_DOWN)
		v &= ~(2 << shift);
	if (jd & DIR_LEFT)
		v &= ~(4 << shift);
	if (jd & DIR_RIGHT)
		v &= ~(8 << shift);
	return v;
}

uae_u8 handle_parport_joystick (int port, uae_u8 data)
{
	uae_u8 v = data;
	maybe_read_input();
	switch (port)
	{
	case 0:
		if (parport_joystick_enabled) {
			v = parconvert (v, joydir[2], 0);
			v = parconvert (v, joydir[3], 4);
		}
		return v;
	case 1:
		if (parport_joystick_enabled) {
			if (getbuttonstate (2, 0))
				v &= ~4;
			if (getbuttonstate (3, 0))
				v &= ~1;
			if (getbuttonstate (2, 1) || getbuttonstate (3, 1))
				v &= ~2; /* spare */
		}
		return v;
	default:
		abort ();
		return 0;
	}
}

/* p5 (3rd button) is 1 or floating = cd32 2-button mode */
static bool cd32padmode(int joy)
{
	return pot_cap[joy][0] <= 100;
}

static bool is_joystick_pullup (int joy)
{
	return joymodes[joy] == JSEM_MODE_GAMEPAD;
}
static bool is_mouse_pullup (int joy)
{
	return mouse_pullup;
}

static int lightpen_port_number(void)
{
	return (currprefs.cs_agnusmodel == AGNUSMODEL_A1000 || currprefs.cs_agnusmodel == AGNUSMODEL_VELVET) ? 0 : 1;
}

static void charge_cap (int joy, int idx, int charge)
{
	if (charge < -1 || charge > 1)
		charge = charge * 80;
	pot_cap[joy][idx] += charge;
	if (pot_cap[joy][idx] < 0)
		pot_cap[joy][idx] = 0;
	if (pot_cap[joy][idx] > 511)
		pot_cap[joy][idx] = 511;
}

static void cap_check(bool hsync)
{
	int joy, i;

	for (joy = 0; joy < 2; joy++) {
		for (i = 0; i < 2; i++) {
			bool cancharge = true;
			int charge = 0, dong, joypot;
			uae_u16 pdir = 0x0200 << (joy * 4 + i * 2); /* output enable */
			uae_u16 pdat = 0x0100 << (joy * 4 + i * 2); /* data */
			uae_u16 p5dir = 0x0200 << (joy * 4);
			uae_u16 p5dat = 0x0100 << (joy * 4);
			int isbutton = getbuttonstate (joy, i == 0 ? JOYBUTTON_3 : JOYBUTTON_2);

			if (cd32_pad_enabled[joy] && !cd32padmode(joy)) {
				// only red and blue can be read if CD32 pad and only if it is in normal pad mode
				isbutton |= getbuttonstate (joy, JOYBUTTON_CD32_BLUE);
				// middle button line is floating
				if (i == 0)
					isbutton = 0;
				cd32_shifter[joy] = 8;
			}

			// two lightpens use multiplexer chip
			if (lightpen_enabled2 && lightpen_port_number() == joy)
				continue;

			dong = dongle_analogjoy (joy, i);
			if (dong >= 0) {
				isbutton = 0;
				joypot = dong;
				if (pot_cap[joy][i] < joypot)
					charge = 1; // slow charge via dongle resistor
			} else {
				joypot = joydirpot[joy][i];
				if (analog_port[joy][i] && pot_cap[joy][i] < joypot)
					charge = 1; // slow charge via pot variable resistor
				if ((is_joystick_pullup (joy) && digital_port[joy][i]) || (is_mouse_pullup (joy) && mouse_port[joy]))
					charge = 1; // slow charge via pull-up resistor
			}
			if (!(potgo_value & pdir)) { // input?
				if (pot_dat_act[joy][i] && hsync) {
					pot_dat[joy][i]++;
				}
				/* first 7 or 8 lines after potgo has been started = discharge cap */
				if (pot_dat_act[joy][i] == 1) {
					if (pot_dat[joy][i] < (currprefs.ntscmode ? POTDAT_DELAY_NTSC : POTDAT_DELAY_PAL)) {
						charge = -2; /* fast discharge delay */
						cancharge = hsync;
					} else {
						pot_dat_act[joy][i] = 2;
						pot_dat[joy][i] = 0;
					}
				}
				if (dong >= 0) {
					if (pot_dat_act[joy][i] == 2 && pot_cap[joy][i] >= joypot)
						pot_dat_act[joy][i] = 0;
				} else {
					if (analog_port[joy][i] && pot_dat_act[joy][i] == 2 && pot_cap[joy][i] >= joypot)
						pot_dat_act[joy][i] = 0;
					if ((digital_port[joy][i] || mouse_port[joy]) && pot_dat_act[joy][i] == 2) {
						if (pot_cap[joy][i] >= 10 && !isbutton)
							pot_dat_act[joy][i] = 0;
					}
				}
				// CD32 pad 3rd button line floating: 2-button mode
				if (cd32_pad_enabled[joy] && i == 0) {
					if (charge == 0)
						charge = 2;
				}
				// CD32 pad in 2-button mode: blue button is internally pulled up
				if (cd32_pad_enabled[joy] && !cd32padmode(joy) && i == 1) {
					if (charge == 0)
						charge = 2;
				}

			} else { // output?
				charge = (potgo_value & pdat) ? 2 : -2; /* fast (dis)charge if output */
				if (potgo_value & pdat)
					pot_dat_act[joy][i] = 0; // instant stop if output+high
				if (isbutton)
					pot_dat[joy][i]++; // "free running" if output+low
			}

			if (isbutton) {
				charge = -2; // button press overrides everything
			}

			if (currprefs.cs_cdtvcd) {
				/* CDTV P9 is not floating */
				if (!(potgo_value & pdir) && i == 1 && charge == 0)
					charge = 2;
			}

			if (dong < 0 && charge == 0) {

				if (lightpen_port[joy])
					charge = 2;

				/* official Commodore mouse has pull-up resistors in button lines
				 * NOTE: 3rd party mice may not have pullups! */
				if (is_mouse_pullup (joy) && mouse_port[joy] && digital_port[joy][i])
					charge = 2;

				/* emulate pullup resistor if button mapped because there too many broken
				 * programs that read second button in input-mode (and most 2+ button pads have
				 * pullups)
				 */
				if (is_joystick_pullup (joy) && digital_port[joy][i])
					charge = 2;
			}

			if (cancharge) {
				charge_cap(joy, i, charge);
			}
		}
	}
}


uae_u8 handle_joystick_buttons (uae_u8 pra, uae_u8 dra)
{
	uae_u8 but = 0;
	int i;

	maybe_read_input();
	cap_check(false);
	for (i = 0; i < 2; i++) {
		int mask = 0x40 << i;
		if (cd32_pad_enabled[i]) {
			but |= mask;
			if (!cd32padmode(i)) {
				if (getbuttonstate (i, JOYBUTTON_CD32_RED) || getbuttonstate (i, JOYBUTTON_1))
					but &= ~mask;
				// always zero if output=1 and data=0
				if ((dra & mask) && !(pra & mask)) {
					but &= ~mask;
				}
			}
		} else {
			if (!getbuttonstate (i, JOYBUTTON_1))
				but |= mask;
			if (bouncy && cycles_in_range (bouncy_cycles)) {
				but &= ~mask;
				if (uaerand () & 1)
					but |= mask;
			}
			// always zero if output=1 and data=0
			if ((dra & mask) && !(pra & mask)) {
				but &= ~mask;
			}
		}
	}

	if (inputdevice_logging & 4) {
//		static uae_u8 old;
//		if (but != old)
			write_log (_T("BFE001 R: %02X:%02X %x\n"), dra, but, M68K_GETPC);
//		old = but;
	}
	return but;
}

/* joystick 1 button 1 is used as a output for incrementing shift register */
void handle_cd32_joystick_cia (uae_u8 pra, uae_u8 dra)
{
	static int oldstate[2];
	int i;

	maybe_read_input();
	if (inputdevice_logging & (4 | 256)) {
		write_log (_T("BFE001 W: %02X:%02X %x\n"), dra, pra, M68K_GETPC);
	}
	cap_check(false);
	for (i = 0; i < 2; i++) {
		uae_u8 but = 0x40 << i;
		if (cd32padmode(i)) {
			if ((dra & but) && (pra & but) != oldstate[i]) {
				if (!(pra & but)) {
					cd32_shifter[i]--;
					if (cd32_shifter[i] < 0)
						cd32_shifter[i] = 0;
					if (inputdevice_logging & (4 | 256))
						write_log (_T("CD32 %d shift: %d %08x\n"), i, cd32_shifter[i], M68K_GETPC);
				}
			}
		}
		oldstate[i] = dra & pra & but;
	}
}

/* joystick port 1 button 2 is input for button state */
static uae_u16 handle_joystick_potgor (uae_u16 potgor)
{
	int i;

	cap_check(false);
	for (i = 0; i < 2; i++) {
		uae_u16 p9dir = 0x0800 << (i * 4); /* output enable P9 */
		uae_u16 p9dat = 0x0400 << (i * 4); /* data P9 */
		uae_u16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
		uae_u16 p5dat = 0x0100 << (i * 4); /* data P5 */

		if (cd32_pad_enabled[i] && cd32padmode(i)) {

			/* p5 is floating in input-mode */
			potgor &= ~p5dat;
			if (pot_cap[i][0] > 100)
				potgor |= p5dat;

			if (!(potgo_value & p9dir))
				potgor |= p9dat;

			/* shift at 1 == return one, >1 = return button states */
			if (cd32_shifter[i] == 0)
				potgor &= ~p9dat; /* shift at zero == return zero */
			if (cd32_shifter[i] >= 2 && (joybutton[i] & ((1 << JOYBUTTON_CD32_PLAY) << (cd32_shifter[i] - 2))))
				potgor &= ~p9dat;

			// normal second button pressed: always zero. Overrides CD32 mode.
			if (getbuttonstate(i, JOYBUTTON_2))
				potgor &= ~p9dat;
#ifdef ARCADIA
		} else if (alg_flag) {

			potgor = alg_potgor(potgo_value);
#endif
		} else if (lightpen_enabled2 && lightpen_port_number() == i) {

			int button;

			if (inputdevice_get_lightpen_id() == 1)
				button = lightpen_trigger2;
			else
				button = getbuttonstate(i, JOYBUTTON_3);

			potgor |= 0x1000;
			if (button)
				potgor &= ~0x1000;

		} else {

			potgor &= ~p5dat;
			if (pot_cap[i][0] > 100)
				potgor |= p5dat;

			if (!cd32_pad_enabled[i] || !cd32padmode(i)) {
				potgor &= ~p9dat;
				if (pot_cap[i][1] > 100)
					potgor |= p9dat;
			}

		}
	}
	return potgor;
}

static void inject_events (const TCHAR *str)
{
	bool quot = false;
	bool first = true;
	uae_u8 keys[300];
	int keycnt = 0;

	for (;;) {
		TCHAR ch = *str++;
		if (!ch)
			break;

		if (ch == '\'') {
			first = false;
			quot = !quot;
			continue;
		}

		if (!quot && (ch == ' ' || first)) {
			const TCHAR *s = str;
			if (first)
				s--;
			while (*s == ' ')
				s++;
			const TCHAR *s2 = s;
			while (*s && *s != ' ')
				s++;
			int s2len = (int)(s - s2);
			if (!s2len)
				break;
			for (int i = 1; events[i].name; i++) {
				const TCHAR *cf = events[i].confname;
				if (!_tcsnicmp (cf, _T("KEY_"), 4))
					cf += 4;
				if (events[i].allow_mask == AM_K && !_tcsnicmp (cf, s2, _tcslen (cf)) && s2len == _tcslen (cf)) {
					int j;
					uae_u8 kc = events[i].data << 1;
					TCHAR tch = _totupper (s2[0]);
					if (tch != s2[0]) {
						// release
						for (j = 0; j < keycnt; j++) {
							if (keys[j] == kc)
								keys[j] = 0xff;
						}
						kc |= 0x01;
					} else {
						for (j = 0; j < keycnt; j++) {
							if (keys[j] == kc) {
								kc = 0xff;
							}
						}
						if (kc != 0xff) {
							for (j = 0; j < keycnt; j++) {
								if (keys[j] == 0xff) {
									keys[j] = kc;
									break;
								}
							}
							if (j == keycnt) {
								if (keycnt < sizeof keys)
									keys[keycnt++] = kc;
							}
						}
					}
					if (kc != 0xff) {
						//write_log (_T("%s\n"), cf);
						record_key(kc, false);
					}
				}
			}
		} else if (quot) {
			ch = _totupper (ch);
			if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
				for (int i = 1; events[i].name; i++) {
					if (events[i].allow_mask == AM_K && events[i].name[1] == 0 && events[i].name[0] == ch) {
						record_key(events[i].data << 1, false);
						record_key((events[i].data << 1) | 0x01, false);
						//write_log (_T("%c\n"), ch);
					}
				}
			}
		}
		first = false;
	}
	while (--keycnt >= 0) {
		uae_u8 kc = keys[keycnt];
		if (kc != 0xff)
			record_key(kc | 0x01, false);
	}
}

struct delayed_event
{
	TCHAR *event_string;
	int delay;
	int append;
	struct delayed_event *next;
};
static struct delayed_event *delayed_events;

int handle_custom_event (const TCHAR *custom, int append)
{
	TCHAR *p, *buf, *nextp;
	bool noquot = false;
	bool first = true;
	int adddelay = 0;
	bool maybe_config_changed = false;

	if (custom == NULL) {
		return 0;
	}
	//write_log (_T("%s\n"), custom);

	if (append) {
		struct delayed_event *dee = delayed_events, *prev = NULL;
		while (dee) {
			if (dee->delay > 0 && dee->delay > adddelay && dee->append) {
				adddelay = dee->delay;
			}
			prev = dee;
			dee = dee->next;
		}
	}

	p = buf = my_strdup_trim (custom);
	if (p[0] != '\"')
		noquot = true;
	while (p && *p) {
		TCHAR *p2 = NULL;
		if (!noquot) {
			if (*p != '\"')
				break;
			p++;
			p2 = p;
			while (*p2 != '\"' && *p2 != 0)
				p2++;
			if (*p2 == '\"') {
				*p2++ = 0;
				nextp = p2 + 1;
				while (*nextp == ' ')
					nextp++;
			}
		}
		//write_log (L"-> '%s'\n", p);
		if (!_tcsnicmp (p, _T("delay "), 6) || !_tcsnicmp (p, _T("vdelay "), 7) || !_tcsnicmp (p, _T("hdelay "), 7) || adddelay) {
			TCHAR *next = NULL;
			int delay = -1;
			if (!_tcsnicmp (p, _T("delay "), 6)) {
				next = p + 7;
				delay = _tstol(p + 6) * maxvpos_nom;
				if (!delay)
					delay = maxvpos_nom;
			} else if (!_tcsnicmp (p, _T("vdelay "), 7)) {
				next = p + 8;
				delay = _tstol(p + 7) * maxvpos_nom;
				if (!delay)
					delay = maxvpos_nom;
			} else if (!_tcsnicmp (p, _T("hdelay "), 7)) {
				next = p + 8;
				delay = _tstol(p + 7);
			}
			if (adddelay && delay < 0) {
				delay = adddelay;
			} else if (adddelay > 0 && delay >= 0) {
				delay += adddelay;
			}
			if (delay >= 0) {
				if (!p2) {
					if (!next)
						p2 = p;
					else
						p2 = _tcschr(next, ' ');
				}
				struct delayed_event *de = delayed_events;
				while (de) {
					if (de->delay < 0) {
						de->delay = delay + 1;
						de->event_string = p2 ? my_strdup (p2) : my_strdup(_T(""));
						de->append = append;
						break;
					}
					de = de->next;
				}
				if (!de) {
					de = xcalloc (delayed_event, 1);
					de->next = delayed_events;
					delayed_events = de;
					de->delay = delay + 1;
					de->append = append;
					de->event_string = p2 ? my_strdup (p2) : my_strdup(_T(""));
				}
			}
			break;
		}
		if (first) {
			first = false;
			if (!append)
				maybe_config_changed = true;
		}
		if (!_tcsicmp (p, _T("no_config_check"))) {
			config_changed = 0;
			config_changed_flags = 0;
			maybe_config_changed = false;
		} else if (!_tcsicmp (p, _T("do_config_check"))) {
			set_config_changed ();
		} else if (!_tcsnicmp(p, _T("shellexec "), 10)) {
			uae_ShellExecute(p + 10);
#ifdef DEBUGGER
		} else if (!_tcsnicmp(p, _T("dbg "), 4)) {
			debug_parser(p + 4, NULL, -1);
#endif
		} else if (!_tcsnicmp (p, _T("kbr "), 4)) {
			inject_events (p + 4);
		} else if (!_tcsnicmp (p, _T("evt "), 4)) {
			TCHAR *pp = _tcschr (p + 4, ' ');
			p += 4;
			if (pp)
				*pp++ = 0;
			inputdevice_uaelib (p, pp);
		} else if (!_tcsnicmp(p, _T("key_raw_up "), 11)) {
			TCHAR *pp = _tcschr (p + 10, ' ');
			if (pp) {
				*pp++ = 0;
				inputdevice_uaelib (p, pp);
			}
		} else if (!_tcsnicmp(p, _T("key_raw_down "), 13)) {
			TCHAR *pp = _tcschr (p + 12, ' ');
			if (pp) {
				*pp++ = 0;
				inputdevice_uaelib (p, pp);
			}
		} else {
			cfgfile_parse_line (&changed_prefs, p, 0);
		}
		if (noquot)
			break;
		p = nextp;
	}
	if (maybe_config_changed)
		set_config_changed();
	xfree (buf);
	return 0;
}

void inputdevice_playevents(void)
{
	inprec_playdiskchange();
	int nr, state, max, autofire;
	while (inprec_playevent(&nr, &state, &max, &autofire))
		handle_input_event(nr, state, max, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_PLAYBACKEVENT);
}

void inputdevice_hsync (bool forceread)
{
	cap_check(true);

#ifdef CATWEASEL
	catweasel_hsync ();
#endif

	int cnt = 0;
	struct delayed_event *de = delayed_events, *prev = NULL;
	while (de) {
		if (de->delay < 0)
			cnt++;
		if (de->delay > 0)
			de->delay--;
		if (de->delay == 0) {
			de->delay = -1;
			if (de->event_string) {
				TCHAR *s = de->event_string;
				de->event_string = NULL;
				handle_custom_event (s, 0);
				xfree (s);
			}
		}
		prev = de;
		de = de->next;
	}
	if (cnt > 4) {
		// too many, delete some
		struct delayed_event *de_prev = NULL;
		de = delayed_events;
		while (de) {
			if (de->delay < 0 && de != delayed_events) {
				struct delayed_event *next = de->next;
				de_prev->next = next;
				xfree(de->event_string);
				xfree(de);
				de = next;
			} else {
				de_prev = de;
				de = de->next;
			}
		}
	}

	for (int i = 0; i < INPUT_QUEUE_SIZE; i++) {
		struct input_queue_struct *iq = &input_queue[i];
		if (iq->linecnt > 0) {
			iq->linecnt--;
			if (iq->linecnt == 0) {
				if (iq->state)
					iq->state = 0;
				else
					iq->state = iq->storedstate;
				if (iq->custom)
					handle_custom_event (iq->custom, 0);
				if (iq->evt)
					handle_input_event (iq->evt, iq->state, iq->max, HANDLE_IE_FLAG_PLAYBACKEVENT);
				iq->linecnt = iq->nextlinecnt;
			}
		}
	}

	if (bouncy && get_cycles () > bouncy_cycles)
		bouncy = 0;

	if (input_record && input_record != INPREC_RECORD_PLAYING) {
		if (vpos == 0)
			inputdevice_read();
	}
	if (input_play) {
		inputdevice_playevents();
		if (vpos == 0)
			handle_msgpump(true);
	}
	if (!input_record && !input_play) {
		if (forceread) {
			inputread = maxvpos + 1;
			inputdevice_read();
		} else {
			maybe_read_input();
		}
	}
#ifdef WITH_DRACO
	if (draco_keybord_repeat_cnt > 0) {
		draco_keybord_repeat_cnt--;
		if (draco_keybord_repeat_cnt == 0) {
			int rate = draco_keyboard_get_rate();
			int b = (rate >> 3) & 3;
			int d = (rate >> 0) & 7;
			float r = ((1 << b) * (d + 8) / 240.0f) * 1000.0f;
			draco_keybord_repeat_cnt = (int)(vblank_hz * maxvpos * r / 1000);
			draco_keycode(draco_keybord_repeat_code, 1);
		}
	}
#endif
}

static uae_u16 POTDAT (int joy)
{
	uae_u16 v = (pot_dat[joy][1] << 8) | pot_dat[joy][0];
	if (inputdevice_logging & (16 | 128))
		write_log (_T("POTDAT%d: %04X %08X\n"), joy, v, M68K_GETPC);
	return v;
}

uae_u16 POT0DAT (void)
{
	return POTDAT (0);
}
uae_u16 POT1DAT (void)
{
	return POTDAT (1);
}

/* direction=input, data pin floating, last connected logic level or previous status
*                  written when direction was ouput
*                  otherwise it is currently connected logic level.
* direction=output, data pin is current value, forced to zero if joystick button is pressed
* it takes some tens of microseconds before data pin changes state
*/

void POTGO (uae_u16 v)
{
	int i, j;

	if (inputdevice_logging & (16 | 128))
		write_log (_T("POTGO_W: %04X %08X\n"), v, M68K_GETPC);
#if DONGLE_DEBUG
	if (notinrom ())
		write_log (_T("POTGO %04X %s\n"), v, debuginfo(0));
#endif
	dongle_potgo (v);
	potgo_value = potgo_value & 0x5500; /* keep state of data bits */
	potgo_value |= v & 0xaa00; /* get new direction bits */
	for (i = 0; i < 8; i += 2) {
		uae_u16 dir = 0x0200 << i;
		if (v & dir) {
			uae_u16 data = 0x0100 << i;
			potgo_value &= ~data;
			potgo_value |= v & data;
		}
	}
	if (v & 1) {
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 2; j++) {
				pot_dat_act[i][j] = 1;
				pot_dat[i][j] = 0;
			}
		}
	}
}

uae_u16 POTGOR (void)
{
	uae_u16 v;

	v = handle_joystick_potgor (potgo_value) & 0x5500;
	v = dongle_potgor (v);
#if DONGLE_DEBUG
	if (notinrom ())
		write_log (_T("POTGOR %04X %s\n"), v, debuginfo(0));
#endif
	if (inputdevice_logging & 16)
		write_log (_T("POTGO_R: %04X %08X %d %d\n"), v, M68K_GETPC, cd32_shifter[0], cd32_shifter[1]);
	return v;
}

static int check_input_queue (int evt)
{
	struct input_queue_struct *iq;
	int i;
	for (i = 0; i < INPUT_QUEUE_SIZE; i++) {
		iq = &input_queue[i];
		if (iq->evt == evt && iq->linecnt >= 0)
			return i;
	}
	return -1;
}

static void queue_input_event (int evt, const TCHAR *custom, int state, int max, int linecnt, int autofire)
{
	struct input_queue_struct *iq;
	int idx;

	if (!evt)
		return;
	idx = check_input_queue (evt);
	if (state < 0 && idx >= 0) {
		iq = &input_queue[idx];
		iq->nextlinecnt = -1;
		iq->linecnt = -1;
		iq->evt = 0;
		if (iq->state == 0 && evt > 0)
			handle_input_event (evt, 0, 1, 0);
	} else if (state >= 0 && idx < 0) {
		if (evt == 0 && custom == NULL)
			return;
		for (idx = 0; idx < INPUT_QUEUE_SIZE; idx++) {
			iq = &input_queue[idx];
			if (iq->linecnt < 0)
				break;
		}
		if (idx == INPUT_QUEUE_SIZE) {
			write_log (_T("input queue overflow\n"));
			return;
		}
		xfree (iq->custom);
		iq->custom = NULL;
		if (custom)
			iq->custom = my_strdup (custom);
		iq->evt = evt;
		iq->state = iq->storedstate = state;
		iq->max = max;
		iq->linecnt = linecnt < 0 ? maxvpos + maxvpos / 2 : linecnt;
		iq->nextlinecnt = autofire > 0 ? linecnt : -1;
	}
}

static uae_u8 keybuf[256];
#define MAX_PENDING_EVENTS 20
struct inputcode
{
	int code;
	int state;
	TCHAR *s;
};
static struct inputcode inputcode_pending[MAX_PENDING_EVENTS];

static bool inputdevice_handle_inputcode_immediate(int code, int state)
{
	if (!state)
		return false;
	switch(code)
	{
		case AKS_ENTERDEBUGGER:
#ifdef DEBUGGER
			activate_debugger ();
#endif
			return true;
	}
	return false;
}


void inputdevice_add_inputcode (int code, int state, const TCHAR *s)
{
	for (int i = 0; i < MAX_PENDING_EVENTS; i++) {
		if (inputcode_pending[i].code == code && inputcode_pending[i].state == state)
			return;
	}
	for (int i = 0; i < MAX_PENDING_EVENTS; i++) {
		if (inputcode_pending[i].code == 0) {
			if (!inputdevice_handle_inputcode_immediate(code, state)) {
				inputcode_pending[i].code = code;
				inputcode_pending[i].state = state;
#ifdef AMIBERRY
				if (s == NULL)
					inputcode_pending[i].s = NULL;
				else
#endif
					inputcode_pending[i].s = my_strdup(s);
			}
			return;
		}
	}
}

static bool keyboardresetkeys(void)
{
	return (keybuf[AK_CTRL] || keybuf[AK_RCTRL]) && keybuf[AK_LAMI] && keybuf[AK_RAMI];
}

void inputdevice_do_keyboard(int code, int state)
{
#ifdef CDTV
	if (code >= 0x72 && code <= 0x77) { // CDTV keys
		if (cdtv_front_panel(-1)) {
			// front panel active
			if (!state)
				return;
			cdtv_front_panel(code - 0x72);
			return;
		}
	}
#endif
	if (code < 0x80) {
		uae_u8 key = code | (state ? 0x00 : 0x80);
		keybuf[key & 0x7f] = (key & 0x80) ? 0 : 1;
		if (keyboard_reset_seq_mode > 0) {
			if (keyboard_reset_seq_mode == 3) {
				if (keyboard_reset_seq >= 15 * 50) {
					if (!keyboardresetkeys()) {
						keyboard_reset_seq_mode = 0;
					} else {
						keyboard_reset_seq_mode = 4;
					}
				}
				return;
			}
			if (!keyboardresetkeys()) {
				memset(keybuf, 0, sizeof(keybuf));
				if (keyboard_reset_seq >= 5 * 50 || keyboard_reset_seq_mode > 0) {
					send_internalevent(INTERNALEVENT_KBRESET);
					if (keyboard_reset_seq >= 5 * 50 || keyboard_reset_seq_mode == 2) {
						custom_reset(true, true);
						uae_reset(1, 1);
					} else {
						custom_reset(false, true);
						uae_reset(0, 1);
					}
				}
				keyboard_reset_seq = 0;
				keyboard_reset_seq_mode = 0;
				release_keys();
			}
			return;
		}

		if (currprefs.keyboard_mode > 0) {
			if (!currprefs.cs_resetwarning) {
				if (keyboardresetkeys() || key == AK_RESETWARNING) {
					int r = keybuf[AK_LALT] | keybuf[AK_RALT];
					if (r) {
						keyboard_reset_seq_mode = 2;
						custom_reset(true, true);
						cpu_inreset();
					} else {
						custom_reset(false, true);
						cpu_inreset();
						keyboard_reset_seq_mode = 1;
					}
				}
			}
		} else if (key == AK_RESETWARNING) {
			if (resetwarning_do(0)) {
				keyboard_reset_seq_mode = 3;
			}
			return;
		} else if (keyboardresetkeys()) {
			keyboard_reset_seq = 0;
			keyboard_reset_seq_mode = 0;
			int r = keybuf[AK_LALT] | keybuf[AK_RALT];
			if (r) {
				keyboard_reset_seq_mode = 2;
				custom_reset(true, true);
				cpu_inreset();
			} else {
				if (currprefs.cs_resetwarning && resetwarning_do(0)) {
					keyboard_reset_seq_mode = 3;
				} else {
					custom_reset(false, true);
					cpu_inreset();
					keyboard_reset_seq_mode = 1;
				}
			}
		}
		if (!keyboard_reset_seq_mode) {
			if (record_key((uae_u8)((key << 1) | (key >> 7)), false)) {
				if (inputdevice_logging & 1)
					write_log(_T("Amiga key %02X %d\n"), key & 0x7f, key >> 7);
			}
		}
		return;
	}
	inputdevice_add_inputcode(code, state, NULL);
}

void inputdevice_do_kb_reset(void)
{
	custom_reset(false, true);
	if (keyboard_reset_seq_mode == 3) {
		if (!keyboardresetkeys()) {
			keyboard_reset_seq_mode = 0;
			uae_reset(0, 1);
		} else {
			keyboard_reset_seq_mode = 4;
		}
	} else {
		uae_reset(0, 1);
	}
}

#ifdef AMIBERRY
static const int mousespeed_values[] = {2, 5, 10, 20, 40};

int mousespeed;
int i;
int num_elements;
#endif

// these need cpu trace data
static bool needcputrace (int code)
{
	switch (code)
	{
	case AKS_ENTERGUI:
	case AKS_STATECAPTURE:
	case AKS_STATESAVEQUICK:
	case AKS_STATESAVEQUICK1:
	case AKS_STATESAVEQUICK2:
	case AKS_STATESAVEQUICK3:
	case AKS_STATESAVEQUICK4:
	case AKS_STATESAVEQUICK5:
	case AKS_STATESAVEQUICK6:
	case AKS_STATESAVEQUICK7:
	case AKS_STATESAVEQUICK8:
	case AKS_STATESAVEQUICK9:
	case AKS_STATESAVEDIALOG:
		return true;
	}
	return false;
}

void target_paste_to_keyboard(void);

static bool inputdevice_handle_inputcode2(int monid, int code, int state, const TCHAR *s)
{
	static int swapperslot;
	static int tracer_enable;
	int newstate;
	int onoffstate = state & ~SET_ONOFF_MASK_PRESS;

	if (s != NULL && s[0] == 0)
		s = NULL;

	if (code == 0)
		return false;
	if (state && needcputrace(code) && can_cpu_tracer() == true && is_cpu_tracer () == false && !input_play && !input_record && !debugging) {
		if (set_cpu_tracer (true)) {
			tracer_enable = 1;
			return true; // wait for next frame
		}
	}

#if 0
	if (vpos != 0)
		write_log (_T("inputcode=%d but vpos = %d\n"), code, vpos);
#endif
	if (onoffstate == SET_ONOFF_ON_VALUE)
		newstate = 1;
	else if (onoffstate == SET_ONOFF_OFF_VALUE)
		newstate = 0;
	else if (onoffstate == SET_ONOFF_PRESS_VALUE)
		newstate = -1;
	else if (onoffstate == SET_ONOFF_PRESSREL_VALUE)
		newstate = (state & SET_ONOFF_MASK_PRESS) ? 1 : -1;
	else if (state)
		newstate = -1;
	else
		newstate = 0;

#ifdef ARCADIA
	switch (code)
	{
	case AKS_ARCADIADIAGNOSTICS:
		arcadia_flag &= ~1;
		arcadia_flag |= state ? 1 : 0;
		break;
	case AKS_ARCADIAPLY1:
		arcadia_flag &= ~4;
		arcadia_flag |= state ? 4 : 0;
		break;
	case AKS_ARCADIAPLY2:
		arcadia_flag &= ~2;
		arcadia_flag |= state ? 2 : 0;
		break;
	case AKS_ARCADIACOIN1:
		if (state)
			arcadia_coin[0]++;
		break;
	case AKS_ARCADIACOIN2:
		if (state)
			arcadia_coin[1]++;
		break;

	case AKS_CUBOTOUCH:
		if (state)
			cubo_flag |= 0x80000000;
		else
			cubo_flag &= ~0x80000000;
		cubo_flag &= ~0x40000000;
		break;
	case AKS_CUBOTEST:
		if (state)
			cubo_flag |= 0x00800000;
		else
			cubo_flag &= ~0x00800000;
		break;
	case AKS_CUBOCOIN1:
		if (state)
			cubo_function(0);
		break;
	case AKS_CUBOCOIN2:
		if (state)
			cubo_function(1);
		break;
	case AKS_CUBOCOIN3:
		if (state)
			cubo_function(2);
		break;
	case AKS_CUBOCOIN4:
		if (state)
			cubo_function(3);
		break;

	case AKS_ALGSERVICE:
		alg_flag &= ~2;
		alg_flag |= state ? 2 : 0;
		break;
	case AKS_ALGLSTART:
		alg_flag &= ~4;
		alg_flag |= state ? 4 : 0;
		break;
	case AKS_ALGRSTART:
		alg_flag &= ~8;
		alg_flag |= state ? 8 : 0;
		break;
	case AKS_ALGLCOIN:
		alg_flag &= ~16;
		alg_flag |= state ? 16 : 0;
		break;
	case AKS_ALGRCOIN:
		alg_flag &= ~32;
		alg_flag |= state ? 32 : 0;
		break;
	case AKS_ALGLTRIGGER:
		alg_flag &= ~64;
		alg_flag |= state ? 64 : 0;
		break;
	case AKS_ALGRTRIGGER:
		alg_flag &= ~128;
		alg_flag |= state ? 128 : 0;
		break;
	case AKS_ALGLHOLSTER:
		alg_flag &= ~256;
		alg_flag |= state ? 256 : 0;
		break;
	case AKS_ALGRHOLSTER:
		alg_flag &= ~512;
		alg_flag |= state ? 512 : 0;
		break;
	}
#endif
	if (!state) {
		//switch(code)
		//{
		//	case AKS_SCREENSHOT_FILE:
			// stop multiscreenshot
		//	screenshot(-1, 4, 1);
		//	break;
		//}
		return false;
	}

	switch (code)
	{
	case AKS_ENTERGUI:
		gui_display (-1);
		setsystime ();
		break;
	case AKS_SCREENSHOT_FILE:
		if (state > 1) {
			screenshot(-1, 3, 1);
		} else {
			screenshot(-1, 1, 1);
		}
		break;
	case AKS_SCREENSHOT_CLIPBOARD:
		//screenshot(-1, 0, 1);
		break;
#ifdef AVIOUTPUT
	case AKS_VIDEORECORD:
		AVIOutput_Toggle(newstate, true);
		break;
	case AKS_VIDEORECORDFILE:
		if (s) {
			_tcsncpy(avioutput_filename_gui, s, MAX_DPATH);
			avioutput_filename_gui[MAX_DPATH - 1] = 0;
			set_config_changed();
		} else {
			gui_display (7);
			setsystime ();
		}
		break;
#endif
#ifdef ACTION_REPLAY
	case AKS_FREEZEBUTTON:
		action_replay_freeze ();
		break;
#endif
	case AKS_FLOPPY0:
		if (s) {
			_tcsncpy(changed_prefs.floppyslots[0].df, s, MAX_DPATH);
			changed_prefs.floppyslots[0].df[MAX_DPATH - 1] = 0;
			set_config_changed();
		} else {
			gui_display (0);
			setsystime ();
		}
		break;
	case AKS_FLOPPY1:
		if (s) {
			_tcsncpy(changed_prefs.floppyslots[1].df, s, MAX_DPATH);
			changed_prefs.floppyslots[1].df[MAX_DPATH - 1] = 0;
			set_config_changed();
		} else {
			gui_display (1);
			setsystime ();
		}
		break;
	case AKS_FLOPPY2:
		if (s) {
			_tcsncpy(changed_prefs.floppyslots[2].df, s, MAX_DPATH);
			changed_prefs.floppyslots[2].df[MAX_DPATH - 1] = 0;
			set_config_changed();
		} else {
			gui_display (2);
			setsystime ();
		}
		break;
	case AKS_FLOPPY3:
		if (s) {
			_tcsncpy(changed_prefs.floppyslots[3].df, s, MAX_DPATH);
			changed_prefs.floppyslots[3].df[MAX_DPATH - 1] = 0;
			set_config_changed();
		} else {
			gui_display (3);
			setsystime ();
		}
		break;
	case AKS_EFLOPPY0:
		disk_eject (0);
		break;
	case AKS_EFLOPPY1:
		disk_eject (1);
		break;
	case AKS_EFLOPPY2:
		disk_eject (2);
		break;
	case AKS_EFLOPPY3:
		disk_eject (3);
		break;
	case AKS_CD0:
		if (s) {
			_tcsncpy(changed_prefs.cdslots[0].name, s, MAX_DPATH);
			changed_prefs.cdslots[0].name[MAX_DPATH - 1] = 0;
			changed_prefs.cdslots[0].inuse = true;
			set_config_changed();
		}
		else {
			gui_display(6);
			setsystime();
		}
		break;
	case AKS_ECD0:
		changed_prefs.cdslots[0].name[0] = 0;
		changed_prefs.cdslots[0].inuse = false;
		break;
	case AKS_IRQ7:
		IRQ_forced(7, -1);
		break;
	case AKS_PAUSE:
		pausemode(newstate > 0 ? 1 : newstate);
		break;
	case AKS_SINGLESTEP:
		if (pause_emulation)
			pausemode(0);
		autopause = 1;
		break;
	case AKS_WARP:
		warpmode (newstate);
		break;
	case AKS_INHIBITSCREEN:
		toggle_inhibit_frame(monid, IHF_SCROLLLOCK);
		break;
	case AKS_STATEREWIND:
		savestate_dorewind (-2);
		break;
	case AKS_STATECURRENT:
		savestate_dorewind (-1);
		break;
	case AKS_STATECAPTURE:
		savestate_capture (1);
		break;
	case AKS_VOLDOWN:
		sound_volume (newstate <= 0 ? -1 : 1);
		break;
	case AKS_VOLUP:
		sound_volume (newstate <= 0 ? 1 : -1);
		break;
	case AKS_VOLMUTE:
		sound_mute (newstate);
		break;
	case AKS_MVOLDOWN:
		master_sound_volume (newstate <= 0 ? -1 : 1);
		break;
	case AKS_MVOLUP:
		master_sound_volume (newstate <= 0 ? 1 : -1);
		break;
	case AKS_MVOLMUTE:
		master_sound_volume (0);
		break;
	case AKS_QUIT:
		uae_quit ();
		break;
	case AKS_SOFTRESET:
		uae_reset (0, 0);
		break;
	case AKS_HARDRESET:
		uae_reset (1, 1);
		break;
	case AKS_RESTART:
		uae_restart(&currprefs, -1, NULL);
		break;
	case AKS_STATESAVEQUICK:
	case AKS_STATESAVEQUICK1:
	case AKS_STATESAVEQUICK2:
	case AKS_STATESAVEQUICK3:
	case AKS_STATESAVEQUICK4:
	case AKS_STATESAVEQUICK5:
	case AKS_STATESAVEQUICK6:
	case AKS_STATESAVEQUICK7:
	case AKS_STATESAVEQUICK8:
	case AKS_STATESAVEQUICK9:
		savestate_quick ((code - AKS_STATESAVEQUICK) / 2, 1);
		break;
	case AKS_STATERESTOREQUICK:
	case AKS_STATERESTOREQUICK1:
	case AKS_STATERESTOREQUICK2:
	case AKS_STATERESTOREQUICK3:
	case AKS_STATERESTOREQUICK4:
	case AKS_STATERESTOREQUICK5:
	case AKS_STATERESTOREQUICK6:
	case AKS_STATERESTOREQUICK7:
	case AKS_STATERESTOREQUICK8:
	case AKS_STATERESTOREQUICK9:
		savestate_quick ((code - AKS_STATERESTOREQUICK) / 2, 0);
		break;
	case AKS_TOGGLEDEFAULTSCREEN:
		toggle_fullscreen(0, -1);
		break;
	case AKS_TOGGLEWINDOWEDFULLSCREEN:
		toggle_fullscreen(0, 0);
		break;
	case AKS_TOGGLEFULLWINDOWFULLSCREEN:
		toggle_fullscreen(0, 1);
		break;
	case AKS_TOGGLEWINDOWFULLWINDOW:
		toggle_fullscreen(0, 2);
		break;
	case AKS_TOGGLEMOUSEGRAB:
		toggle_mousegrab();
		break;
	case AKS_SWAPJOYPORTS:
		if (state == 1)
			inputdevice_swap_compa_ports(&changed_prefs, 0);
		else if (state == 2)
			inputdevice_swap_compa_ports(&changed_prefs, 2);
		break;
	case AKS_PASTE:
		target_paste_to_keyboard();
		break;
	case AKS_SWITCHINTERPOL:
		changed_prefs.sound_interpol++;
		if (changed_prefs.sound_interpol > 4)
			changed_prefs.sound_interpol = 0;
		set_config_changed ();
		break;
	case AKS_ENTERDEBUGGER:
#ifdef DEBUGGER
		activate_debugger ();
#endif
		break;
	case AKS_STATESAVEDIALOG:
		if (s) {
			savestate_initsave (s, 1, true, true);
			save_state (savestate_fname, STATE_SAVE_DESCRIPTION);
		} else {
			gui_display (5);
		}
		break;
	case AKS_STATERESTOREDIALOG:
		if (s) {
			savestate_initsave (s, 1, true, false);
			savestate_state = STATE_DORESTORE;
		} else {
			gui_display (4);
		}
		break;
	case AKS_DECREASEREFRESHRATE:
	case AKS_INCREASEREFRESHRATE:
		{
			struct chipset_refresh *cr = get_chipset_refresh(&changed_prefs);
			if (cr) {
				int dir = code == AKS_INCREASEREFRESHRATE ? 5 : -5;
				if (cr->rate == 0)
					cr->rate = currprefs.ntscmode ? 60.0f : 50.0f;
				cr->locked = true;
				cr->rate += dir;
				if (cr->rate < 10.0f)
					cr->rate = 10.0f;
				if (cr->rate > 900.0f)
					cr->rate = 900.0f;
				set_config_changed();
			}
		}
		break;
	case AKS_DISKSWAPPER_NEXT:
		swapperslot++;
		if (swapperslot >= MAX_SPARE_DRIVES || currprefs.dfxlist[swapperslot][0] == 0)
			swapperslot = 0;
		break;
	case AKS_DISKSWAPPER_PREV:
		swapperslot--;
		if (swapperslot < 0)
			swapperslot = MAX_SPARE_DRIVES - 1;
		while (swapperslot > 0) {
			if (currprefs.dfxlist[swapperslot][0])
				break;
			swapperslot--;
		}
		break;
	case AKS_DISKSWAPPER_INSERT0:
	case AKS_DISKSWAPPER_INSERT1:
	case AKS_DISKSWAPPER_INSERT2:
	case AKS_DISKSWAPPER_INSERT3:
		_tcscpy (changed_prefs.floppyslots[code - AKS_DISKSWAPPER_INSERT0].df, currprefs.dfxlist[swapperslot]);
		set_config_changed ();
		break;
	case AKS_INPUT_CONFIG_1:
	case AKS_INPUT_CONFIG_2:
	case AKS_INPUT_CONFIG_3:
	case AKS_INPUT_CONFIG_4:
		changed_prefs.input_selected_setting = currprefs.input_selected_setting = code - AKS_INPUT_CONFIG_1;
		inputdevice_updateconfig (&changed_prefs, &currprefs);
		break;
	case AKS_DISK_PREV0:
	case AKS_DISK_PREV1:
	case AKS_DISK_PREV2:
	case AKS_DISK_PREV3:
		disk_prevnext (code - AKS_DISK_PREV0, -1);
		break;
	case AKS_DISK_NEXT0:
	case AKS_DISK_NEXT1:
	case AKS_DISK_NEXT2:
	case AKS_DISK_NEXT3:
		disk_prevnext (code - AKS_DISK_NEXT0, 1);
		break;
	case AKS_RTG_PREV:
		toggle_rtg(0, -1);
		break;
	case AKS_RTG_NEXT:
		toggle_rtg(0, MAX_RTG_BOARDS + 1);
		break;
	case AKS_RTG_C:
	case AKS_RTG_0:
	case AKS_RTG_1:
	case AKS_RTG_2:
	case AKS_RTG_3:
		toggle_rtg(0, code - AKS_RTG_C);
		break;
	case AKS_VIDEOGRAB_RESTART:
		//getsetpositionvideograb(0);
		//pausevideograb(0);
		break;
	case AKS_VIDEOGRAB_PAUSE:
		//pausevideograb(-1);
		break;
	case AKS_VIDEOGRAB_PREV:
	{
		//pausevideograb(1);
		//uae_s64 pos = getsetpositionvideograb(-1);
		//pos--;
		//if (pos >= 0)
		//	getsetpositionvideograb(pos);
		break;
	}
	case AKS_VIDEOGRAB_NEXT:
	{
		//pausevideograb(1);
		//uae_s64 pos = getsetpositionvideograb(-1);
		//pos++;
		//getsetpositionvideograb(pos);
		break;
	}

#ifdef CDTV
	case AKS_CDTV_FRONT_PANEL_STOP:
	case AKS_CDTV_FRONT_PANEL_PLAYPAUSE:
	case AKS_CDTV_FRONT_PANEL_PREV:
	case AKS_CDTV_FRONT_PANEL_NEXT:
	case AKS_CDTV_FRONT_PANEL_REW:
	case AKS_CDTV_FRONT_PANEL_FF:
		cdtv_front_panel (code - AKS_CDTV_FRONT_PANEL_STOP);
	break;
#endif
#ifdef AMIBERRY
	case AKS_MOUSEMAP_PORT0_LEFT:
		changed_prefs.jports[0].mousemap ^= 1 << 0;
		inputdevice_updateconfig(&changed_prefs, &currprefs);
		break;
	case AKS_MOUSEMAP_PORT0_RIGHT:
		changed_prefs.jports[0].mousemap ^= 1 << 1;
		inputdevice_updateconfig(&changed_prefs, &currprefs);
		break;
	case AKS_MOUSEMAP_PORT1_LEFT:
		changed_prefs.jports[1].mousemap ^= 1 << 0;
		inputdevice_updateconfig(&changed_prefs, &currprefs);
		break;
	case AKS_MOUSEMAP_PORT1_RIGHT:
		changed_prefs.jports[1].mousemap ^= 1 << 1;
		inputdevice_updateconfig(&changed_prefs, &currprefs);
		break;

	case AKS_MOUSE_SPEED_DOWN:
		num_elements = std::size(mousespeed_values);
		mousespeed = currprefs.input_joymouse_multiplier;

		i = find_in_array(mousespeed_values, num_elements, mousespeed);
		i = i - 1;
		if (i < 0) { i = num_elements - 1; }
		changed_prefs.input_joymouse_multiplier = mousespeed_values[i];
		inputdevice_updateconfig(&changed_prefs, &currprefs);
		break;

	case AKS_MOUSE_SPEED_UP:
		num_elements = std::size(mousespeed_values);
		mousespeed = currprefs.input_joymouse_multiplier;

		i = find_in_array(mousespeed_values, num_elements, mousespeed);
		i = i + 1;
		if (i >= num_elements) { i = 0; }
		changed_prefs.input_joymouse_multiplier = mousespeed_values[i];
		inputdevice_updateconfig(&changed_prefs, &currprefs);
		break;
	case AKS_SHUTDOWN:
		uae_quit();
		host_poweroff = true;
		break;
	case AKS_TOGGLE_JIT:
#ifdef JIT
		if (currprefs.cachesize == 0)
		{
			currprefs.cpu_compatible = changed_prefs.cpu_compatible = false;
			currprefs.cachesize = changed_prefs.cachesize = MAX_JIT_CACHE;
			currprefs.cachesize = changed_prefs.compfpu = true;
			currprefs.compfpu = changed_prefs.cpu_cycle_exact = false;
			currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact = false;
			currprefs.address_space_24 = changed_prefs.address_space_24 = false;
		}
		else
		{
			currprefs.cachesize = changed_prefs.cachesize = 0;
			currprefs.compfpu = changed_prefs.compfpu = false;
		}
		fixup_prefs(&changed_prefs, true);
#endif
		break;
	case AKS_TOGGLE_JIT_FPU:
#ifdef USE_JIT_FPU
		currprefs.compfpu = changed_prefs.compfpu = !currprefs.compfpu;
#endif
		break;
#endif
	case AKS_AUTO_CROP_IMAGE:
		currprefs.gfx_auto_crop = !currprefs.gfx_auto_crop;
		check_prefs_changed_gfx();
		break;
	case AKS_OSK:
		if (vkbd_allowed(0))
			vkbd_toggle();
		break;
	case AKS_DISKSWAPPER_NEXT_INSERT0:
		swapperslot++;
		if (swapperslot >= MAX_SPARE_DRIVES || currprefs.dfxlist[swapperslot][0] == 0)
			swapperslot = 0;
		_tcscpy(changed_prefs.floppyslots[0].df, currprefs.dfxlist[swapperslot]);
		set_config_changed();
		break;
	case AKS_DISKSWAPPER_PREVIOUS_INSERT0:
		swapperslot--;
		if (swapperslot < 0)
			swapperslot = MAX_SPARE_DRIVES - 1;
		while (swapperslot > 0) {
			if (currprefs.dfxlist[swapperslot][0])
				break;
			swapperslot--;
		}
		_tcscpy(changed_prefs.floppyslots[0].df, currprefs.dfxlist[swapperslot]);
		set_config_changed();
		break;
	}
end:
	if (tracer_enable) {
		set_cpu_tracer (false);
		tracer_enable = 0;
	}
	return false;
}

void inputdevice_handle_inputcode(void)
{
	int monid = 0;
	bool got = false;
	for (int i = 0; i < MAX_PENDING_EVENTS; i++) {
		int code = inputcode_pending[i].code;
		int state = inputcode_pending[i].state;
		const TCHAR *s = inputcode_pending[i].s;
		if (code) {
#ifdef AMIBERRY
			if (pause_emulation && state == 0)
			{
				got = false;
				xfree(inputcode_pending[i].s);
				inputcode_pending[i].code = 0;
				continue;
			}
#endif
			if (!inputdevice_handle_inputcode2(monid, code, state, s)) {
				xfree(inputcode_pending[i].s);
				inputcode_pending[i].code = 0;
			}
			got = true;
		}
	}
	if (!got)
		inputdevice_handle_inputcode2(monid, 0, 0, NULL);
}


static int getqualid (int evt)
{
	if (evt > INPUTEVENT_SPC_QUALIFIER_START && evt < INPUTEVENT_SPC_QUALIFIER_END)
		return evt - INPUTEVENT_SPC_QUALIFIER1;
	return -1;
}

static uae_u64 isqual (int evt)
{
	int num = getqualid (evt);
	if (num < 0)
		return 0;
	return ID_FLAG_QUALIFIER1 << (num * 2);
}

#ifdef AMIBERRY
// Pass the joystick state (joybutton and joydir) to vkbd subsystem and clear them for the emulator.
static void handle_vkbd()
{
	if (!vkbd_is_active()) return;

	int vkbd_state = 0;
	for (int joy = 0; joy < 2; ++joy)
	{
		oleft[joy] = 0;
		oright[joy] = 0;
		otop[joy] = 0;
		obot[joy] = 0;
		horizclear[joy] = 0;
		vertclear[joy] = 0;

		int mask_button;
		if (cd32_pad_enabled[joy])
			mask_button = 1 << JOYBUTTON_CD32_RED;
		else
			mask_button = 1 << JOYBUTTON_1;

		if (joybutton[joy] & mask_button)
		{
			vkbd_state |= VKBD_BUTTON;
			joybutton[joy] &= ~mask_button;
		}
		if (joydir[joy] & DIR_LEFT)
		{
			vkbd_state |= VKBD_LEFT;
			joydir[joy] &= ~DIR_LEFT;
		}
		if (joydir[joy] & DIR_RIGHT)
		{
			vkbd_state |= VKBD_RIGHT;
			joydir[joy] &= ~DIR_RIGHT;
		}
		if (joydir[joy] & DIR_UP)
		{
			vkbd_state |= VKBD_UP;
			joydir[joy] &= ~DIR_UP;
		}
		if (joydir[joy] & DIR_DOWN)
		{
			vkbd_state |= VKBD_DOWN;
			joydir[joy] &= ~DIR_DOWN;
		}
	}

	int code;
	int pressed;
	if (vkbd_process(vkbd_state, &code, &pressed))
	{
		inputdevice_do_keyboard(code, pressed);
	}
}
#endif

static int handle_input_event2(int nr, int state, int max, int flags, int extra)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	const struct inputevent *ie;
	int joy;
	bool isaks = false;
	bool allowoppositestick = false;
	int autofire = (flags & HANDLE_IE_FLAG_AUTOFIRE) ? 1 : 0;

	if (nr <= 0 || nr == INPUTEVENT_SPC_CUSTOM_EVENT)
		return 0;

#ifdef _WIN32
	// ignore normal GUI event if forced gui key is in use
	if (nr == INPUTEVENT_SPC_ENTERGUI) {
		if (currprefs.win32_guikey > 0)
			return 0;
	}
#endif

	ie = &events[nr];
	if (isqual (nr))
		return 0; // qualifiers do nothing
	if (ie->unit == 0 && ie->data >= AKS_FIRST) {
		isaks = true;
	}

#ifdef DEBUGGER
	if (isaks) {
		if (debug_trainer_event(ie->data, state))
		{
			return 0;
		}
	} else {
		if (debug_trainer_event(nr, state))
		{
			return 0;
		}
	}
#endif
	if (!isaks) {
		if (input_record && input_record != INPREC_RECORD_PLAYING)
			inprec_recordevent (nr, state, max, autofire);
		if (input_play && state && (flags & HANDLE_IE_FLAG_CANSTOPPLAYBACK)) {
			if (inprec_realtime ()) {
				if (input_record && input_record != INPREC_RECORD_PLAYING)
					inprec_recordevent (nr, state, max, autofire);
			}
		}
		if (!(flags & HANDLE_IE_FLAG_PLAYBACKEVENT) && input_play)
			return 0;
	}

	if (flags & HANDLE_IE_FLAG_ALLOWOPPOSITE) {
		if (ie->unit >= 1 && ie->unit <= 4) {
			if ((ie->data & (DIR_LEFT | DIR_RIGHT)) != (DIR_LEFT | DIR_RIGHT) && (ie->data & (DIR_UP | DIR_DOWN)) != (DIR_UP | DIR_DOWN))
			allowoppositestick = true;
		}
	}

	if ((inputdevice_logging & 1) || input_record || input_play)
		write_log (_T("STATE=%05d MAX=%05d AF=%d QUAL=%06x '%s' \n"), state, max, autofire, (uae_u32)(qualifiers >> 32), ie->name);
	if (autofire) {
		if (state)
			queue_input_event (nr, NULL, state, max, currprefs.input_autofire_linecnt, 1);
		else
			queue_input_event (nr, NULL, -1, 0, 0, 1);
	}
	switch (ie->unit)
	{
	case 5: /* lightpen/gun */
	case 6: /* lightpen/gun #2 */
		{
			int unit = (ie->data & 1) ? 1 : 0;
			int lpnum = ie->unit - 5;
			if (!(lightpen_active & (1 << lpnum))) {
				if (!state) {
					break;
				}
				lightpen_x[lpnum] = vidinfo->outbuffer->outwidth / 2;
				lightpen_y[lpnum] = vidinfo->outbuffer->outheight / 2;
				lightpen_active |= 1 << lpnum;
			}
			lightpen_enabled = true;
			if (flags & HANDLE_IE_FLAG_ABSOLUTE) {
				if (ie->data & IE_INVERT) {
					if (extra >= 0) {
						extra = 65535 - extra;
					}
				}
				lastmxy_abs[lpnum][unit] = extra;
				if (!unit)
					return 1;
				int x = lastmxy_abs[lpnum][0];
				int y = lastmxy_abs[lpnum][1];
				if (x <= 0 || x >= 65535 || y <= 0 || y >= 65535) {
					x = y = -1;
				}
				tablet_lightpen(x, y, 65535, 65535, 0, 0, false, -1, lpnum);
			} else if (ie->type == 0) {
				int delta = 0;
				if (max == 0) {
					delta = state * currprefs.input_mouse_speed / 100;
				} else {
					int deadzone = currprefs.input_joymouse_deadzone * max / 100;
					if (state <= deadzone && state >= -deadzone) {
						state = 0;
						lightpen_deltanoreset[lpnum][unit] = 0;
					} else if (state < 0) {
						state += deadzone;
						lightpen_deltanoreset[lpnum][unit] = 1;
					} else {
						state -= deadzone;
						lightpen_deltanoreset[lpnum][unit] = 1;
					}
					max -= deadzone;
					delta = state * currprefs.input_joymouse_multiplier / (10 * max);
				}
				if (ie->data & IE_INVERT)
					delta = -delta;
				if (unit)
					lightpen_y[lpnum] += delta;
				else
					lightpen_x[lpnum] += delta;
				if (max)
					lightpen_delta[lpnum][unit] = delta;
				else
					lightpen_delta[lpnum][unit] += delta;
			} else if (ie->type == 2) {
				lightpen_trigger2 = state;
			} else {
				if (state) {
					int delta = currprefs.input_joymouse_speed;
					if (ie->data & IE_INVERT)
						delta = -delta;
					if (ie->data & DIR_LEFT)
						lightpen_x[lpnum] -= delta;
					if (ie->data & DIR_RIGHT)
						lightpen_x[lpnum] += delta;
					if (ie->data & DIR_UP)
						lightpen_y[lpnum] -= delta;
					if (ie->data & DIR_DOWN)
						lightpen_y[lpnum] += delta;
				}
			}
			if (lightpen_x[lpnum] < -10)
				lightpen_x[lpnum] = -10;
			if (lightpen_x[lpnum] >= vidinfo->drawbuffer.inwidth + 10)
				lightpen_x[lpnum] = vidinfo->drawbuffer.inwidth + 10;
			if (lightpen_y[lpnum] < -10)
				lightpen_y[lpnum] = -10;
			if (lightpen_y[lpnum] >= vidinfo->drawbuffer.inheight + 10)
				lightpen_y[lpnum] = vidinfo->drawbuffer.inheight + 10;
#if 0
			write_log(_T("%d*%d\n"), lightpen_x[0], lightpen_y[0]);
#endif
		}
		break;
	case 1: /* ->JOY1 */
	case 2: /* ->JOY2 */
	case 3: /* ->Parallel port joystick adapter port #1 */
	case 4: /* ->Parallel port joystick adapter port #2 */
		joy = ie->unit - 1;
		if (ie->type & 4) {
			int old = joybutton[joy] & (1 << ie->data);

			if (state) {
				joybutton[joy] |= 1 << ie->data;
				//gui_gameport_button_change (joy, ie->data, 1);
			} else {
				joybutton[joy] &= ~(1 << ie->data);
				//gui_gameport_button_change (joy, ie->data, 0);
			}

			if (ie->data == 0 && old != (joybutton[joy] & (1 << ie->data)) && currprefs.cpu_cycle_exact) {
				if (!input_record && !input_play && currprefs.input_contact_bounce) {
					// emulate contact bounce, 1st button only, others have capacitors
					bouncy = 1;
					bouncy_cycles = get_cycles () + CYCLE_UNIT * currprefs.input_contact_bounce;
				}
			}

		} else if (ie->type & 8) {

			/* real mouse / analog stick mouse emulation */
			int delta;
			int deadzone = max < 0 ? 0 : currprefs.input_joymouse_deadzone * max / 100;
			int unit = ie->data & 0x7f;

			if (max) {
				if (state <= deadzone && state >= -deadzone) {
					state = 0;
					mouse_deltanoreset[joy][unit] = 0;
				} else if (state < 0) {
					state += deadzone;
					mouse_deltanoreset[joy][unit] = 1;
				} else {
					state -= deadzone;
					mouse_deltanoreset[joy][unit] = 1;
				}
				if (max > 0) {
					max -= deadzone;
					delta = state * currprefs.input_joymouse_multiplier / max;
				} else {
					delta = state;
				}
			} else {
				delta = state;
				mouse_deltanoreset[joy][unit] = 0;
			}
			if (ie->data & IE_CDTV) {
				delta = 0;
				if (state > 0)
					delta = JOYMOUSE_CDTV;
				else if (state < 0)
					delta = -JOYMOUSE_CDTV;
			}

			if (ie->data & IE_INVERT)
				delta = -delta;

			if (max)
				mouse_delta[joy][unit] = delta;
			else
				mouse_delta[joy][unit] += delta;

			max = 32;
			//if (unit) {
			//	if (delta < 0) {
			//		gui_gameport_axis_change (joy, DIR_UP_BIT, abs (delta), max);
			//		gui_gameport_axis_change (joy, DIR_DOWN_BIT, 0, max);
			//	}
			//	if (delta > 0) {
			//		gui_gameport_axis_change (joy, DIR_DOWN_BIT, abs (delta), max);
			//		gui_gameport_axis_change (joy, DIR_UP_BIT, 0, max);
			//	}
			//} else {
			//	if (delta < 0) {
			//		gui_gameport_axis_change (joy, DIR_LEFT_BIT, abs (delta), max);
			//		gui_gameport_axis_change (joy, DIR_RIGHT_BIT, 0, max);
			//	}
			//	if (delta > 0) {
			//		gui_gameport_axis_change (joy, DIR_RIGHT_BIT, abs (delta), max);
			//		gui_gameport_axis_change (joy, DIR_LEFT_BIT, 0, max);
			//	}
			//}

		} else if (ie->type & 32) { /* button mouse emulation vertical */

			int speed = (ie->data & IE_CDTV) ? JOYMOUSE_CDTV : currprefs.input_joymouse_speed;

			if (state && (ie->data & DIR_UP)) {
				mouse_delta[joy][1] = -speed;
				mouse_deltanoreset[joy][1] = 1;
			} else if (state && (ie->data & DIR_DOWN)) {
				mouse_delta[joy][1] = speed;
				mouse_deltanoreset[joy][1] = 1;
			} else
				mouse_deltanoreset[joy][1] = 0;

		} else if (ie->type & 64) { /* button mouse emulation horizontal */

			int speed = (ie->data & IE_CDTV) ? JOYMOUSE_CDTV : currprefs.input_joymouse_speed;

			if (state && (ie->data & DIR_LEFT)) {
				mouse_delta[joy][0] = -speed;
				mouse_deltanoreset[joy][0] = 1;
			} else if (state && (ie->data & DIR_RIGHT)) {
				mouse_delta[joy][0] = speed;
				mouse_deltanoreset[joy][0] = 1;
			} else
				mouse_deltanoreset[joy][0] = 0;

		} else if (ie->type & 128) { /* analog joystick / paddle */

			int deadzone = currprefs.input_joymouse_deadzone * max / 100;
			int unit = ie->data & 0x7f;
			if (max) {
				if (state <= deadzone && state >= -deadzone) {
					state = 0;
				} else if (state < 0) {
					state += deadzone;
				} else {
					state -= deadzone;
				}
				state = state * max / (max - deadzone);
			} else {
				max = 100;
				relativecount[joy][unit] += state;
				state = relativecount[joy][unit];
				if (state < -max)
					state = -max;
				if (state > max)
					state = max;
				relativecount[joy][unit] = state;
			}

			if (ie->data & IE_INVERT)
				state = -state;

			//if (!unit) {
			//	if (state <= 0)
			//		gui_gameport_axis_change (joy, DIR_UP_BIT, abs (state), max);
			//	if (state >= 0)
			//		gui_gameport_axis_change (joy, DIR_DOWN_BIT, abs (state), max);
			//} else {
			//	if (state <= 0)
			//		gui_gameport_axis_change (joy, DIR_LEFT_BIT, abs (state), max);
			//	if (state >= 0)
			//		gui_gameport_axis_change (joy, DIR_RIGHT_BIT, abs (state), max);
			//}

			state = state * currprefs.input_analog_joystick_mult / max;
			state += (128 * currprefs.input_analog_joystick_mult / 100) + currprefs.input_analog_joystick_offset;
			if (state < 0)
				state = 0;
			if (state > 255)
				state = 255;
			joydirpot[joy][unit] = state;
			mouse_deltanoreset[joy][0] = 1;
			mouse_deltanoreset[joy][1] = 1;

		} else {

			int left = oleft[joy], right = oright[joy], top = otop[joy], bot = obot[joy];
			if (ie->type & 16) {
				/* button to axis mapping */
				if (ie->data & DIR_LEFT) {
					left = oleft[joy] = state ? 1 : 0;
					if (horizclear[joy] && left) {
						horizclear[joy] = 0;
						right = oright[joy] = 0;
					}
				}
				if (ie->data & DIR_RIGHT) {
					right = oright[joy] = state ? 1 : 0;
					if (horizclear[joy] && right) {
						horizclear[joy] = 0;
						left = oleft[joy] = 0;
					}
				}
				if (ie->data & DIR_UP) {
					top = otop[joy] = state ? 1 : 0;
					if (vertclear[joy] && top) {
						vertclear[joy] = 0;
						bot = obot[joy] = 0;
					}
				}
				if (ie->data & DIR_DOWN) {
					bot = obot[joy] = state ? 1 : 0;
					if (vertclear[joy] && bot) {
						vertclear[joy] = 0;
						top = otop[joy] = 0;
					}
				}
			} else {
				/* "normal" joystick axis */
				int deadzone = currprefs.input_joystick_deadzone * max / 100;
				int neg, pos;
				if (max == 0) {
					int cnt;
					int mmax = 50, mextra = 10;
					int unit = (ie->data & (4 | 8)) ? 1 : 0;
					// relative events
					relativecount[joy][unit] += state;
					cnt = relativecount[joy][unit];
					neg = cnt < -mmax;	
					pos = cnt > mmax;
					if (cnt < -(mmax + mextra))
						cnt = -(mmax + mextra);
					if (cnt > (mmax + mextra))
						cnt = (mmax + mextra);
					relativecount[joy][unit] = cnt;
				} else {
					if (state < deadzone && state > -deadzone)
						state = 0;
					neg = state < 0 ? 1 : 0;
					pos = state > 0 ? 1 : 0;
				}
				if (ie->data & DIR_LEFT) {
					left = oleft[joy] = neg;
					if (horizclear[joy] && left) {
						horizclear[joy] = 0;
						right = oright[joy] = 0;
					}
				}
				if (ie->data & DIR_RIGHT) {
					right = oright[joy] = pos;
					if (horizclear[joy] && right) {
						horizclear[joy] = 0;
						left = oleft[joy] = 0;
					}
				}
				if (ie->data & DIR_UP) {
					top = otop[joy] = neg;
					if (vertclear[joy] && top) {
						vertclear[joy] = 0;
						bot = obot[joy] = 0;
					}
				}
				if (ie->data & DIR_DOWN) {
					bot = obot[joy] = pos;
					if (vertclear[joy] && bot) {
						vertclear[joy] = 0;
						top = otop[joy] = 0;
					}
				}
			}
			mouse_deltanoreset[joy][0] = 1;
			mouse_deltanoreset[joy][1] = 1;
			joydir[joy] = 0;
			if (left) {
				if (!allowoppositestick) {
					joydir[joy] &= ~DIR_RIGHT;
				}
				joydir[joy] |= DIR_LEFT;
			}
			if (right) {
				if (!allowoppositestick) {
					joydir[joy] &= ~DIR_LEFT;
				}
				joydir[joy] |= DIR_RIGHT;
			}
			if (top) {
				if (!allowoppositestick) {
					joydir[joy] &= ~DIR_DOWN;
				}
				joydir[joy] |= DIR_UP;
			}
			if (bot) {
				if (!allowoppositestick) {
					joydir[joy] &= ~DIR_UP;
				}
				joydir[joy] |= DIR_DOWN;
			}
			if (joy == 0 || joy == 1)
				joymousecounter (joy); 

			//gui_gameport_axis_change (joy, DIR_LEFT_BIT, left, 0);
			//gui_gameport_axis_change (joy, DIR_RIGHT_BIT, right, 0);
			//gui_gameport_axis_change (joy, DIR_UP_BIT, top, 0);
			//gui_gameport_axis_change (joy, DIR_DOWN_BIT, bot, 0);
		}
		break;
	case 0: /* ->KEY */
		inputdevice_do_keyboard (ie->data, state);
		break;
	}
#ifdef AMIBERRY
	if (vkbd_allowed(0) && vkbd_is_active())
		handle_vkbd();
#endif
	return 1;
}

static int handle_input_event_extra(int nr, int state, int max, int flags, int extra)
{
	return handle_input_event2(nr, state, max, flags, extra);
}

static int handle_input_event(int nr, int state, int max, int flags)
{
	return handle_input_event2(nr, state, max, flags, 0);
}

int send_input_event (int nr, int state, int max, int autofire)
{
	check_enable(nr);
	return handle_input_event(nr, state, max, autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0);
}

static void inputdevice_checkconfig (void)
{
	bool changed = false;
	for (int i = 0; i < MAX_JPORTS; i++) {
		if (currprefs.jports[i].id != changed_prefs.jports[i].id ||
			currprefs.jports[i].mode != changed_prefs.jports[i].mode ||
			currprefs.jports[i].submode != changed_prefs.jports[i].submode ||
			currprefs.jports[i].autofire != changed_prefs.jports[i].autofire ||
			_tcscmp(currprefs.jports_custom[i].custom, changed_prefs.jports_custom[i].custom))
				changed = true;
	}

	if (changed ||
		currprefs.input_selected_setting != changed_prefs.input_selected_setting ||
		currprefs.input_joymouse_multiplier != changed_prefs.input_joymouse_multiplier ||
		currprefs.input_joymouse_deadzone != changed_prefs.input_joymouse_deadzone ||
		currprefs.input_joystick_deadzone != changed_prefs.input_joystick_deadzone ||
		currprefs.input_joymouse_speed != changed_prefs.input_joymouse_speed ||
		currprefs.input_autofire_linecnt != changed_prefs.input_autofire_linecnt ||
		currprefs.input_autoswitch != changed_prefs.input_autoswitch ||
		currprefs.input_autoswitchleftright != changed_prefs.input_autoswitchleftright ||
		currprefs.input_device_match_mask != changed_prefs.input_device_match_mask ||
		currprefs.input_mouse_speed != changed_prefs.input_mouse_speed ||
		currprefs.input_tablet != changed_prefs.input_tablet) {

			currprefs.input_selected_setting = changed_prefs.input_selected_setting;
			currprefs.input_joymouse_multiplier = changed_prefs.input_joymouse_multiplier;
			currprefs.input_joymouse_deadzone = changed_prefs.input_joymouse_deadzone;
			currprefs.input_joystick_deadzone = changed_prefs.input_joystick_deadzone;
			currprefs.input_joymouse_speed = changed_prefs.input_joymouse_speed;
			currprefs.input_autofire_linecnt = changed_prefs.input_autofire_linecnt;
			currprefs.input_mouse_speed = changed_prefs.input_mouse_speed;
			currprefs.input_autoswitch = changed_prefs.input_autoswitch;
			currprefs.input_autoswitchleftright = changed_prefs.input_autoswitchleftright;
			currprefs.input_device_match_mask = changed_prefs.input_device_match_mask;
			currprefs.input_tablet = changed_prefs.input_tablet;

			inputdevice_updateconfig (&changed_prefs, &currprefs);
	}
	if (currprefs.dongle != changed_prefs.dongle) {
		currprefs.dongle = changed_prefs.dongle;
		dongle_reset ();
	}
}

void inputdevice_vsync (void)
{
	int monid = 0;
	if (inputdevice_logging & 32)
		write_log (_T("*\n"));

	if (autopause > 0 && pause_emulation == 0) {
		autopause--;
		if (!autopause) {
			pausemode(1);
		}
	}
	if (keyboard_reset_seq_mode && keyboard_reset_seq > 0) {
		keyboard_reset_seq++;
	}

	input_frame++;
	mouseupdate (0, true);
	inputread = -1;

	inputdevice_handle_inputcode ();
	if (mouseedge_alive > 0)
		mouseedge_alive--;
	if (mouseedge(monid))
		mouseedge_alive = 10;
	if (mousehack_alive_cnt > 0) {
		mousehack_alive_cnt--;
		if (mousehack_alive_cnt == 0)
			setmouseactive(0, -1);
	} else if (mousehack_alive_cnt < 0) {
		mousehack_alive_cnt++;
		if (mousehack_alive_cnt == 0) {
			mousehack_alive_cnt = 100;
			setmouseactive(0, -1);
		}
	}
	inputdevice_checkconfig ();

	if (currprefs.turbo_emulation > 2) {
		currprefs.turbo_emulation--;
		changed_prefs.turbo_emulation = currprefs.turbo_emulation;
		if (currprefs.turbo_emulation == 2) {
			warpmode(0);
		}
	}
}

void inputdevice_reset (void)
{
	magicmouse_ibase = 0;
	magicmouse_gfxbase = 0;
	mousehack_reset ();
	if (inputdevice_is_tablet ())
		mousehack_enable ();
	bouncy = 0;
	while (delayed_events) {
		struct delayed_event *de = delayed_events;
		delayed_events = de->next;
		xfree (de->event_string);
		xfree (de);
	}
	for (int i = 0; i < 2; i++) {
		lastmxy_abs[i][0] = 0;
		lastmxy_abs[i][1] = 0;
	}
	for (int i = 0; i < MAX_JPORTS; i++) {
		pc_mouse_buttons[i] = 0;
	}
	lightpen_trigger2 = 0;
	cubo_flag = 0;
#ifdef ARCADIA
	alg_flag &= 1;
#endif
#ifdef WITH_DRACO
	draco_keybord_repeat_cnt = 0;
#endif
}

static int getoldport (struct uae_input_device *id)
{
	int i, j;

	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			int evt = id->eventid[i][j];
			if (evt > 0) {
				int unit = events[evt].unit;
				if (unit >= 1 && unit <= 4)
					return unit;
			}
		}
	}
	return -1;
}

static int switchdevice (struct uae_input_device *id, int num, bool buttonmode)
{
	int ismouse = 0;
	int newport = 0;
	int newslot = -1;
	int devindex = -1;
	int flags = 0;
	const TCHAR *name = NULL, *fname = NULL;
	int otherbuttonpressed = 0;
	int acc = input_acquired;
	const int *customswitch = NULL;


#if SWITCH_DEBUG
	write_log (_T("switchdevice '%s' %d %d\n"), id->name, num, buttonmode);
#endif

	if (num >= 4)
		return 0;
	if (!currprefs.input_autoswitch)
		return false;
	if (!target_can_autoswitchdevice())
		return 0;

	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		if (id == &joysticks[i]) {
			name = idev[IDTYPE_JOYSTICK].get_uniquename (i);
			fname = idev[IDTYPE_JOYSTICK].get_friendlyname (i);
			newport = num == 0 ? 1 : 0;
			flags = idev[IDTYPE_JOYSTICK].get_flags (i);
			for (int j = 0; j < MAX_INPUT_DEVICES; j++) {
				if (j != i) {
					struct uae_input_device2 *id2 = &joysticks2[j];
					if (id2->buttonmask)
						otherbuttonpressed = 1;
				}
			}
			customswitch = custom_autoswitch_joy;
			devindex = i;
		} else if (id == &mice[i]) {
			ismouse = 1;
			name = idev[IDTYPE_MOUSE].get_uniquename (i);
			fname = idev[IDTYPE_MOUSE].get_friendlyname (i);
			newport = num == 0 ? 0 : 1;
			flags = idev[IDTYPE_MOUSE].get_flags (i);
			customswitch = custom_autoswitch_mouse;
			devindex = i;
		}
	}
	if (!name) {
#if SWITCH_DEBUG
		write_log(_T("device not found!?\n"));
#endif
		return 0;
	}
	if (buttonmode) {
		if (num == 0 && otherbuttonpressed)
			newport = newport ? 0 : 1;
	} else {
		newport = num ? 1 : 0;
	}
#if SWITCH_DEBUG
	write_log (_T("newport = %d ismouse=%d flags=%d name=%s\n"), newport, ismouse, flags, name);
#endif
	/* "GamePorts" switch if in GamePorts mode or Input mode and GamePorts port was not NONE */
	if (currprefs.input_selected_setting == GAMEPORT_INPUT_SETTINGS || currprefs.jports[newport].id != JPORT_NONE) {
#if SWITCH_DEBUG
		write_log (_T("GAMEPORTS MODE %d %d\n"), currprefs.input_selected_setting, currprefs.jports[newport].id);
#endif
		if ((num == 0 || num == 1) && !JSEM_ISCUSTOM(newport, &currprefs)) {
#if SWITCH_DEBUG
			write_log (_T("Port supported\n"));
#endif
			bool issupermouse = false;
			int om = jsem_ismouse (num, &currprefs);
			int om1 = jsem_ismouse (0, &currprefs);
			int om2 = jsem_ismouse (1, &currprefs);
			if ((om1 >= 0 || om2 >= 0) && ismouse) {
#if SWITCH_DEBUG
				write_log (_T("END3\n"));
#endif
				return 0;
			}
			if (flags) {
#if SWITCH_DEBUG
				write_log (_T("END2\n"));
#endif
				return 0;
			}

			for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
				if (devindex >= 0 && customswitch && (customswitch[i] & (1 << devindex))) {
					newslot = i;
					name = currprefs.jports_custom[i].custom;
					fname = name;
					break;
				}
			}

#if 1
			if (ismouse) {
				int nummouse = 0; // count number of non-supermouse mice
				int supermouse = -1;
				for (int i = 0; i < idev[IDTYPE_MOUSE].get_num (); i++) {
					if (!idev[IDTYPE_MOUSE].get_flags (i))
						nummouse++;
					else
						supermouse = i;
				}
#if SWITCH_DEBUG
				write_log (_T("inputdevice gameports change supermouse=%d num=%d\n"), supermouse, nummouse);
#endif
				if (supermouse >= 0 && nummouse == 1) {
					const TCHAR* oldname = name;
					name = idev[IDTYPE_MOUSE].get_uniquename (supermouse);
					fname = idev[IDTYPE_MOUSE].get_friendlyname(supermouse);
					issupermouse = true;
#if SWITCH_DEBUG
					write_log (_T("inputdevice gameports change '%s' -> '%s'\n"), oldname, name);
#endif
				}
			}
#endif
#if 1
			write_log (_T("inputdevice gameports change '%s':%d->%d %d,%d\n"), name, num, newport, currprefs.input_selected_setting, currprefs.jports[newport].id);
#endif
			inputdevice_unacquire ();
			if (fname) {
				if (newslot >= 0) {
					statusline_add_message(STATUSTYPE_INPUT, _T("Port %d: Custom %d"), newport, newslot + 1);
				} else {
					statusline_add_message(STATUSTYPE_INPUT, _T("Port %d: %s"), newport, fname);
				}
			}

			if (currprefs.input_selected_setting != GAMEPORT_INPUT_SETTINGS && currprefs.jports[newport].id > JPORT_NONE) {
				// disable old device
				int devnum;
				devnum = jsem_ismouse(newport, &currprefs);
#if SWITCH_DEBUG
				write_log(_T("ismouse num = %d supermouse=%d\n"), devnum, issupermouse);
#endif
				if (devnum >= 0) {
					if (changed_prefs.mouse_settings[currprefs.input_selected_setting][devnum].enabled) {
						changed_prefs.mouse_settings[currprefs.input_selected_setting][devnum].enabled = false;
#if SWITCH_DEBUG
						write_log(_T("input panel mouse device '%s' disabled\n"), changed_prefs.mouse_settings[currprefs.input_selected_setting][devnum].name);
#endif
					}
				}
				for (int l = 0; l < idev[IDTYPE_MOUSE].get_num(); l++) {
					if (changed_prefs.mouse_settings[currprefs.input_selected_setting][l].enabled) {
						if (idev[IDTYPE_MOUSE].get_flags(l)) {
#if SWITCH_DEBUG
							write_log (_T("enabled supermouse %d detected\n"), l);
#endif
							issupermouse = true;
						}
					}
				}
				if (issupermouse) {
					// new mouse is supermouse, disable all other mouse devices
					for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
						changed_prefs.mouse_settings[currprefs.input_selected_setting][l].enabled = false;
					}
				}

				devnum = jsem_isjoy(newport, &currprefs);
#if SWITCH_DEBUG
				write_log(_T("isjoy num = %d\n"), devnum);
#endif
				if (devnum >= 0) {
					if (changed_prefs.joystick_settings[currprefs.input_selected_setting][devnum].enabled) {
						changed_prefs.joystick_settings[currprefs.input_selected_setting][devnum].enabled = false;
#if SWITCH_DEBUG
						write_log(_T("input panel joystick device '%s' disabled\n"), changed_prefs.joystick_settings[currprefs.input_selected_setting][devnum].name);
#endif
					}
				}
			}

			if (newslot >= 0) {
				TCHAR cust[100];
				_sntprintf(cust, sizeof cust, _T("custom%d"), newslot);
				inputdevice_joyport_config(&changed_prefs, cust, cust, newport, -1, -1, 0, true);
			} else {
				inputdevice_joyport_config(&changed_prefs, name, name, newport, -1, -1, 1, true);
			}
			inputdevice_validate_jports (&changed_prefs, -1, NULL);
			inputdevice_copyconfig (&changed_prefs, &currprefs);
			if (acc)
				inputdevice_acquire (TRUE);
			return 1;
		}
#if SWITCH_DEBUG
		write_log (_T("END1\n"));
#endif
		return 0;

	} else {

#if SWITCH_DEBUG
		write_log (_T("INPUTPANEL MODE %d\n"), flags);
#endif
		int oldport = getoldport (id);
		int k, evt;

		const struct inputevent *ie, *ie2;
		if (flags)
			return 0;
		if (oldport <= 0) {
#if SWITCH_DEBUG
			write_log(_T("OLDPORT %d\n"), oldport);
#endif
			return 0;
		}
		newport++;
		/* do not switch if switching mouse and any "supermouse" mouse enabled */
		if (ismouse) {
			for (int i = 0; i < MAX_INPUT_SETTINGS; i++) {
				if (mice[i].enabled && idev[IDTYPE_MOUSE].get_flags (i)) {
#if SWITCH_DEBUG
					write_log(_T("SUPERMOUSE %d enabled\n"), i);
#endif
					return 0;
				}
			}
		}
		for (int i = 0; i < MAX_INPUT_SETTINGS; i++) {
			if (getoldport (&joysticks[i]) == newport) {
				joysticks[i].enabled = 0;
#if SWITCH_DEBUG
				write_log(_T("Joystick %d disabled\n"), i);
#endif
			}
			if (getoldport (&mice[i]) == newport) {
				mice[i].enabled = 0;
#if SWITCH_DEBUG
				write_log(_T("Mouse %d disabled\n"), i);
#endif
			}
		}
		id->enabled = 1;
		for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
			for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
				evt = id->eventid[i][j];
				if (evt <= 0)
					continue;
				ie = &events[evt];
				if (ie->unit == oldport) {
					k = 1;
					while (events[k].confname) {
						ie2 = &events[k];
						if (ie2->type == ie->type && ie2->data == ie->data && ie2->allow_mask == ie->allow_mask && ie2->unit == newport) {
							id->eventid[i][j] = k;
							break;
						}
						k++;
					}
				} else if (ie->unit == newport) {
					k = 1;
					while (events[k].confname) {
						ie2 = &events[k];
						if (ie2->type == ie->type && ie2->data == ie->data && ie2->allow_mask == ie->allow_mask && ie2->unit == oldport) {
							id->eventid[i][j] = k;
							break;
						}
						k++;
					}
				}
			}
		}
		write_log (_T("inputdevice input change '%s':%d->%d\n"), name, num, newport);
		inputdevice_unacquire ();
		if (fname)
			statusline_add_message(STATUSTYPE_INPUT, _T("Port %d: %s"), newport, fname);
		inputdevice_copyconfig (&currprefs, &changed_prefs);
		inputdevice_validate_jports (&changed_prefs, -1, NULL);
		inputdevice_copyconfig (&changed_prefs, &currprefs);
		if (acc)
			inputdevice_acquire (TRUE);
		return 1;
	}
	return 0;
}

uae_u64 input_getqualifiers (void)
{
	return qualifiers;
}

bool key_specialpressed(void)
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_SPECIAL) != 0;
}
bool key_shiftpressed(void)
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_SHIFT) != 0;
}
bool key_altpressed(void)
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_ALT) != 0;
}
bool key_ctrlpressed(void)
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_CONTROL) != 0;
}
#ifdef AMIBERRY // we also handle this key
bool key_winpressed(void)
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_WIN) != 0;
}
#endif

static bool checkqualifiers (int evt, uae_u64 flags, uae_u64 *qualmask, uae_s16 events[MAX_INPUT_SUB_EVENT_ALL])
{
	int i, j;
	int qualid = getqualid (evt);
	int nomatch = 0;
	bool isspecial = (qualifiers & (ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SPECIAL_R)) != 0;

	flags &= ID_FLAG_QUALIFIER_MASK;
	if (qualid >= 0 && events)
		qualifiers_evt[qualid] = events;
	/* special set and new qualifier pressed? do not sent it to Amiga-side */
	if ((qualifiers & (ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SPECIAL_R)) && qualid >= 0)
		return false;

	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		if (qualmask[i])
			break;
	}
	if (i == MAX_INPUT_SUB_EVENT) {
		 // no qualifiers in any slot and no special = always match
		return isspecial == false;
	}

	// do we have any subevents with qualifier set?
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		for (j = 0; j < MAX_INPUT_QUALIFIERS; j++) {
			uae_u64 mask = (ID_FLAG_QUALIFIER1 | ID_FLAG_QUALIFIER1_R) << (j * 2);
			bool isqualmask = (qualmask[i] & mask) != 0;
			bool isqual = (qualifiers & mask) != 0;
			if (isqualmask != isqual) {
				nomatch++;
				break;
			}
		}
	}
	if (nomatch == MAX_INPUT_SUB_EVENT) {
		// no matched qualifiers in any slot
		// allow all slots without qualifiers
		// special = never accept
		if (isspecial)
			return false;
		return flags ? false : true;
	}

	for (i = 0; i < MAX_INPUT_QUALIFIERS; i++) {
		uae_u64 mask = (ID_FLAG_QUALIFIER1 | ID_FLAG_QUALIFIER1_R) << (i * 2);
		bool isflags = (flags & mask) != 0;
		bool isqual = (qualifiers & mask) != 0;
		if (isflags != isqual)
			return false;
	}
	return true;
}

static void setqualifiers (int evt, int state)
{
	uae_u64 mask = isqual (evt);
	if (!mask)
		return;
	if (state)
		qualifiers |= mask;
	else
		qualifiers &= ~mask;
}

static uae_u64 getqualmask (uae_u64 *qualmask, struct uae_input_device *id, int num, bool *qualonly)
{
	uae_u64 mask = 0, mask2 = 0;
	for (int i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		int evt = id->eventid[num][i];
		mask |= id->flags[num][i];
		qualmask[i] = id->flags[num][i] & ID_FLAG_QUALIFIER_MASK;
		mask2 |= isqual (evt);
	}
	mask &= ID_FLAG_QUALIFIER_MASK;
	*qualonly = false;
	if (qualifiers & ID_FLAG_QUALIFIER_SPECIAL) {
		// ID_FLAG_QUALIFIER_SPECIAL already active and this event has one or more qualifiers configured
		*qualonly = mask2 != 0;
	}
	return mask;
}


static bool process_custom_event (struct uae_input_device *id, int offset, int state, uae_u64 *qualmask, int autofire, int sub)
{
	int idx, slotoffset, custompos;
	TCHAR *custom;
	uae_u64 flags, qual;

	if (!id)
		return false;
	
	slotoffset = sub & ~3;
	sub &= 3;
	flags = id->flags[offset][slotoffset];
	qual = flags & ID_FLAG_QUALIFIER_MASK;
	custom = id->custom[offset][slotoffset];

	for (idx = 1; idx < 4; idx++) {
		uae_u64 flags2 = id->flags[offset][slotoffset + idx];

		// all slots must have same qualifier
		if ((flags2 & ID_FLAG_QUALIFIER_MASK) != qual)
			break;
		// no slot must have autofire
		if ((flags2 & ID_FLAG_AUTOFIRE_MASK) || (flags & ID_FLAG_AUTOFIRE_MASK))
			break;
	}
	// at least slot 0 and 2 must have custom
	if (custom == NULL || id->custom[offset][slotoffset + 2] == NULL)
		idx = -1;

	if (idx < 4) {
		id->flags[offset][slotoffset] &= ~(ID_FLAG_CUSTOMEVENT_TOGGLED1 | ID_FLAG_CUSTOMEVENT_TOGGLED2);
		int evt2 = id->eventid[offset][slotoffset + sub];
		uae_u64 flags2 = id->flags[offset][slotoffset + sub];
		if (checkqualifiers (evt2, flags2, qualmask, NULL)) {
			custom = id->custom[offset][slotoffset + sub];
			if (state && custom) {
				if (autofire)
					queue_input_event (-1, custom, 1, 1, currprefs.input_autofire_linecnt, 1);
				handle_custom_event (custom, 0);
				return true;
			}
		}
		return false;
	}

	if (sub != 0)
		return false;

	slotoffset = 0;
	if (!checkqualifiers (id->eventid[offset][slotoffset], id->flags[offset][slotoffset], qualmask, NULL)) {
		slotoffset = 4;
		if (!checkqualifiers (id->eventid[offset][slotoffset], id->flags[offset][slotoffset], qualmask, NULL))
			return false;
	}

	flags = id->flags[offset][slotoffset];
	custompos = (flags & ID_FLAG_CUSTOMEVENT_TOGGLED1) ? 1 : 0;
	custompos |= (flags & ID_FLAG_CUSTOMEVENT_TOGGLED2) ? 2 : 0;
 
	if (state < 0) {
		idx = 0;
		custompos = 0;
	} else {
		if (state > 0) {
			if (custompos & 1)
				return false; // waiting for release
		} else {
			if (!(custompos & 1))
				return false; // waiting for press
		}
		idx = custompos;
		custompos++;
	}

	queue_input_event (-1, NULL, -1, 0, 0, 1);

	if ((id->flags[offset][slotoffset + idx] & ID_FLAG_QUALIFIER_MASK) == qual) {
		custom = id->custom[offset][slotoffset + idx];
		if (autofire)
			queue_input_event (-1, custom, 1, 1, currprefs.input_autofire_linecnt, 1);
		if (custom)
			handle_custom_event (custom, 0);
	}

	id->flags[offset][slotoffset] &= ~(ID_FLAG_CUSTOMEVENT_TOGGLED1 | ID_FLAG_CUSTOMEVENT_TOGGLED2);
	id->flags[offset][slotoffset] |= (custompos & 1) ? ID_FLAG_CUSTOMEVENT_TOGGLED1 : 0;
	id->flags[offset][slotoffset] |= (custompos & 2) ? ID_FLAG_CUSTOMEVENT_TOGGLED2 : 0;

	return true;
}

static void setbuttonstateall (struct uae_input_device *id, struct uae_input_device2 *id2, int button, int buttonstate)
{
	static frame_time_t switchdevice_timeout;
	int i;
	uae_u32 mask = 1 << button;
	uae_u32 omask = id2 ? id2->buttonmask & mask : 0;
	uae_u32 nmask = (buttonstate ? 1 : 0) << button;
	uae_u64 qualmask[MAX_INPUT_SUB_EVENT];
	bool qualonly;
	bool doit = true;

	if (input_play && buttonstate)
		inprec_realtime ();
	if (input_play)
		return;

#if INPUT_DEBUG
	write_log(_T("setbuttonstateall %d %d\n"), button, buttonstate);
#endif

	if (!id->enabled) {
		frame_time_t t = read_processor_time ();
		if (!t)
			t++;

		if (buttonstate) {
			switchdevice_timeout = t;
		} else {
			if (switchdevice_timeout) {
				int port = button;
				if (t - switchdevice_timeout >= syncbase) // 1s
					port ^= 1;
				switchdevice (id, port, true);
			}
			switchdevice_timeout = 0;
		}
		return;
	}
	if (button >= ID_BUTTON_TOTAL)
		return;

	if (currprefs.input_tablet == TABLET_REAL && mousehack_alive()) {
		// mouse driver injects all buttons when tablet mode
		if (id == &mice[0])
			doit = false;
	}

	if (doit) {
		getqualmask (qualmask, id, ID_BUTTON_OFFSET + button, &qualonly);

		bool didcustom = false;

		for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
			int sub = sublevdir[buttonstate == 0 ? 1 : 0][i];
			uae_u64 *flagsp = &id->flags[ID_BUTTON_OFFSET + button][sub];
			int evt = id->eventid[ID_BUTTON_OFFSET + button][sub];
			TCHAR *custom = id->custom[ID_BUTTON_OFFSET + button][sub];
			uae_u64 flags = flagsp[0];
#ifdef AMIBERRY
			int autofire = (flags & ID_FLAG_AUTOFIRE)
			? 1
			: evt > INPUTEVENT_AUTOFIRE_BEGIN && evt < INPUTEVENT_AUTOFIRE_END
			? 1
			: 0;
#endif
			int toggle = (flags & ID_FLAG_TOGGLE) ? 1 : 0;
			int inverttoggle = (flags & ID_FLAG_INVERTTOGGLE) ? 1 : 0;
			int invert = (flags & ID_FLAG_INVERT) ? 1 : 0;
			int setmode = (flags & ID_FLAG_SET_ONOFF) ? 1: 0;
			int setvalval = (flags & (ID_FLAG_SET_ONOFF_VAL1 | ID_FLAG_SET_ONOFF_VAL2));
			int setval = setvalval == (ID_FLAG_SET_ONOFF_VAL1 | ID_FLAG_SET_ONOFF_VAL2) ? SET_ONOFF_PRESSREL_VALUE :
				(setvalval == ID_FLAG_SET_ONOFF_VAL2 ? SET_ONOFF_PRESS_VALUE : (setvalval == ID_FLAG_SET_ONOFF_VAL1 ? SET_ONOFF_ON_VALUE : SET_ONOFF_OFF_VALUE));
			int state;
			int ie_flags = id->port[ID_BUTTON_OFFSET + button][sub] == 0 ? HANDLE_IE_FLAG_ALLOWOPPOSITE : 0;
			int afinvert = 0;

			// Game ports toggle "autofire" mode
			if (autofire && invert) {
				invert = 0;
				toggle = 1;
				autofire = 0;
			}

			if (buttonstate < 0) {
				state = buttonstate;
			} else if (invert) {
				state = buttonstate ? 0 : 1;
			} else {
				state = buttonstate;
			}
			if (setmode) {
				if (state || setval == SET_ONOFF_PRESS_VALUE || setval == SET_ONOFF_PRESSREL_VALUE)
					state = setval | (buttonstate ? 1 : 0);
			}

			if (!state) {
				didcustom |= process_custom_event (id, ID_BUTTON_OFFSET + button, state, qualmask, autofire, i);
			}

			setqualifiers (evt, state > 0);

			if (qualonly)
				continue;

			if (state < 0) {
				if (!checkqualifiers (evt, flags, qualmask, NULL))
					continue;
				handle_input_event (evt, 1, 1, HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
				didcustom |= process_custom_event (id, ID_BUTTON_OFFSET + button, state, qualmask, 0, i);
			} else if (inverttoggle) {
				/* pressed = firebutton, not pressed = autofire */
				if (state) {
					queue_input_event (evt, NULL, -1, 0, 0, 1);
					handle_input_event (evt, 2, 1, HANDLE_IE_FLAG_CANSTOPPLAYBACK);
				} else {
					handle_input_event (evt, 2, 1, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
				}
				didcustom |= process_custom_event (id, ID_BUTTON_OFFSET + button, state, qualmask, autofire, i);
			} else if (toggle) {
				if (!state)
					continue;
				if (omask & mask)
					continue;
				if (!checkqualifiers (evt, flags, qualmask, NULL))
					continue;
				*flagsp ^= ID_FLAG_TOGGLED;
				int toggled = (*flagsp & ID_FLAG_TOGGLED) ? 2 : 0;
				handle_input_event (evt, toggled, 1, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
				didcustom |= process_custom_event (id, ID_BUTTON_OFFSET + button, toggled, qualmask, autofire, i);
			} else {
				if (!checkqualifiers (evt, flags, qualmask, NULL)) {
					if (!state && !(flags & ID_FLAG_CANRELEASE)) {
						if (!invert)
							continue;
					} else if (state) {
						continue;
					}
				}
				if (!state)
					*flagsp &= ~ID_FLAG_CANRELEASE;
				else
					*flagsp |= ID_FLAG_CANRELEASE;
				if ((omask ^ nmask) & mask) {
					handle_input_event (evt, state, 1, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
					if (state)
						didcustom |= process_custom_event (id, ID_BUTTON_OFFSET + button, state, qualmask, autofire, i);
				}
			}
		}

		if (!didcustom)
			queue_input_event (-1, NULL, -1, 0, 0, 1);
	}

	if (id2 && ((omask ^ nmask) & mask)) {
		if (buttonstate)
			id2->buttonmask |= mask;
		else
			id2->buttonmask &= ~mask;
	}
}


/* - detect required number of joysticks and mice from configuration data
* - detect if CD32 pad emulation is needed
* - detect device type in ports (mouse or joystick)
*/

static int iscd32 (int ei)
{
	if (ei >= INPUTEVENT_JOY1_CD32_FIRST && ei <= INPUTEVENT_JOY1_CD32_LAST) {
		cd32_pad_enabled[0] = 1;
		return 1;
	}
	if (ei >= INPUTEVENT_JOY2_CD32_FIRST && ei <= INPUTEVENT_JOY2_CD32_LAST) {
		cd32_pad_enabled[1] = 1;
		return 2;
	}
	return 0;
}

static int isparport (int ei)
{
	if (ei > INPUTEVENT_PAR_JOY1_START && ei < INPUTEVENT_PAR_JOY_END) {
		parport_joystick_enabled = 1;
		return 1;
	}
	return 0;
}

static int ismouse (int ei)
{
	if (ei >= INPUTEVENT_MOUSE1_FIRST && ei <= INPUTEVENT_MOUSE1_LAST) {
		mouse_port[0] = 1;
		return 1;
	}
	if (ei >= INPUTEVENT_MOUSE2_FIRST && ei <= INPUTEVENT_MOUSE2_LAST) {
		mouse_port[1] = 1;
		return 2;
	}
	return 0;
}

static int isanalog (int ei)
{
	if (ei == INPUTEVENT_JOY1_HORIZ_POT || ei == INPUTEVENT_JOY1_HORIZ_POT_INV) {
		analog_port[0][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY1_VERT_POT || ei == INPUTEVENT_JOY1_VERT_POT_INV) {
		analog_port[0][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_HORIZ_POT || ei == INPUTEVENT_JOY2_HORIZ_POT_INV) {
		analog_port[1][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_VERT_POT || ei == INPUTEVENT_JOY2_VERT_POT_INV) {
		analog_port[1][1] = 1;
		return 1;
	}
	return 0;
}

static int isdigitalbutton (int ei)
{
	if (ei == INPUTEVENT_JOY1_2ND_BUTTON) {
		digital_port[0][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY1_3RD_BUTTON) {
		digital_port[0][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_2ND_BUTTON) {
		digital_port[1][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_3RD_BUTTON) {
		digital_port[1][0] = 1;
		return 1;
	}
	return 0;
}

static int islightpen (int ei)
{
	if (ei >= INPUTEVENT_LIGHTPEN_HORIZ2 && ei < INPUTEVENT_LIGHTPEN_DOWN2) {
		lightpen_enabled2 = true;
	}
	if (ei >= INPUTEVENT_LIGHTPEN_FIRST && ei < INPUTEVENT_LIGHTPEN_LAST) {
		lightpen_enabled = true;
		lightpen_port[lightpen_port_number()] = 1;
		return 1;
	}
	return 0;
}

static void isqualifier (int ei)
{
}

static void check_enable(int ei)
{
	iscd32(ei);
	isparport(ei);
	ismouse(ei);
	isdigitalbutton(ei);
	isqualifier(ei);
	islightpen(ei);
}

static void scanevents (struct uae_prefs *p)
{
	int i, j, k, ei;
	const struct inputevent *e;
	int n_joy = idev[IDTYPE_JOYSTICK].get_num ();
	int n_mouse = idev[IDTYPE_MOUSE].get_num ();

	cd32_pad_enabled[0] = cd32_pad_enabled[1] = 0;
	parport_joystick_enabled = 0;
	mouse_port[0] = mouse_port[1] = 0;
	qualifiers = 0;

	for (i = 0; i < NORMAL_JPORTS; i++) {
		for (j = 0; j < 2; j++) {
			digital_port[i][j] = 0;
			analog_port[i][j] = 0;
			joydirpot[i][j] = 128 / (312 * 100 / currprefs.input_analog_joystick_mult) + (128 * currprefs.input_analog_joystick_mult / 100) + currprefs.input_analog_joystick_offset;
		}
	}
	lightpen_enabled = false;
	lightpen_enabled2 = false;
	lightpen_active = 0;

	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		use_joysticks[i] = 0;
		use_mice[i] = 0;
		for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
			for (j = 0; j < ID_BUTTON_TOTAL; j++) {

				if ((joysticks[i].enabled && i < n_joy) || joysticks[i].enabled < 0) {
					ei = joysticks[i].eventid[ID_BUTTON_OFFSET + j][k];
					e = &events[ei];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isdigitalbutton (ei);
					isqualifier (ei);
					islightpen (ei);
					if (joysticks[i].eventid[ID_BUTTON_OFFSET + j][k] > 0)
						use_joysticks[i] = 1;
				}
				if ((mice[i].enabled && i < n_mouse) || mice[i].enabled < 0) {
					ei = mice[i].eventid[ID_BUTTON_OFFSET + j][k];
					e = &events[ei];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isdigitalbutton (ei);
					isqualifier (ei);
					islightpen (ei);
					if (mice[i].eventid[ID_BUTTON_OFFSET + j][k] > 0)
						use_mice[i] = 1;
				}

			}

			for (j = 0; j < ID_AXIS_TOTAL; j++) {

				if ((joysticks[i].enabled && i < n_joy) || joysticks[i].enabled < 0) {
					ei = joysticks[i].eventid[ID_AXIS_OFFSET + j][k];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isanalog (ei);
					isdigitalbutton (ei);
					isqualifier (ei);
					islightpen (ei);
					if (ei > 0)
						use_joysticks[i] = 1;
				}
				if ((mice[i].enabled && i < n_mouse) || mice[i].enabled < 0) {
					ei = mice[i].eventid[ID_AXIS_OFFSET + j][k];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isanalog (ei);
					isdigitalbutton (ei);
					isqualifier (ei);
					islightpen (ei);
					if (ei > 0)
						use_mice[i] = 1;
				}
			}
		}
	}
	memset (scancodeused, 0, sizeof scancodeused);
	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		use_keyboards[i] = 0;
		if (keyboards[i].enabled && i < idev[IDTYPE_KEYBOARD].get_num ()) {
			j = 0;
			while (j < MAX_INPUT_DEVICE_EVENTS && keyboards[i].extra[j] >= 0) {
				use_keyboards[i] = 1;
				for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
					ei = keyboards[i].eventid[j][k];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isdigitalbutton (ei);
					isqualifier (ei);
					islightpen (ei);
					if (ei > 0)
						scancodeused[i][keyboards[i].extra[j]] = ei;
				}
				j++;
			}
		}
	}
}

static const int axistable[] = {
	INPUTEVENT_MOUSE1_HORIZ, INPUTEVENT_MOUSE1_LEFT, INPUTEVENT_MOUSE1_RIGHT,
	INPUTEVENT_MOUSE1_VERT, INPUTEVENT_MOUSE1_UP, INPUTEVENT_MOUSE1_DOWN,
	INPUTEVENT_MOUSE2_HORIZ, INPUTEVENT_MOUSE2_LEFT, INPUTEVENT_MOUSE2_RIGHT,
	INPUTEVENT_MOUSE2_VERT, INPUTEVENT_MOUSE2_UP, INPUTEVENT_MOUSE2_DOWN,
	INPUTEVENT_JOY1_HORIZ, INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT,
	INPUTEVENT_JOY1_VERT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY2_HORIZ, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	INPUTEVENT_JOY2_VERT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_LEFT, INPUTEVENT_LIGHTPEN_RIGHT,
	INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_LIGHTPEN_UP, INPUTEVENT_LIGHTPEN_DOWN,
	INPUTEVENT_PAR_JOY1_HORIZ, INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT,
	INPUTEVENT_PAR_JOY1_VERT, INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN,
	INPUTEVENT_PAR_JOY2_HORIZ, INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT,
	INPUTEVENT_PAR_JOY2_VERT, INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN,
	INPUTEVENT_MOUSE_CDTV_HORIZ, INPUTEVENT_MOUSE_CDTV_LEFT, INPUTEVENT_MOUSE_CDTV_RIGHT,
	INPUTEVENT_MOUSE_CDTV_VERT, INPUTEVENT_MOUSE_CDTV_UP, INPUTEVENT_MOUSE_CDTV_DOWN,
	-1
};

int intputdevice_compa_get_eventtype (int evt, const int **axistablep)
{
	for (int i = 0; axistable[i] >= 0; i += 3) {
		*axistablep = &axistable[i];
		if (axistable[i] == evt)
			return IDEV_WIDGET_AXIS;
		if (axistable[i + 1] == evt)
			return IDEV_WIDGET_BUTTONAXIS;
		if (axistable[i + 2] == evt)
			return IDEV_WIDGET_BUTTONAXIS;
	}
	*axistablep = NULL;
	return IDEV_WIDGET_BUTTON;
}

static const int rem_port1[] = {
	INPUTEVENT_MOUSE1_HORIZ, INPUTEVENT_MOUSE1_VERT,
	INPUTEVENT_JOY1_HORIZ, INPUTEVENT_JOY1_VERT,
	INPUTEVENT_JOY1_HORIZ_POT, INPUTEVENT_JOY1_VERT_POT,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON, INPUTEVENT_JOY1_3RD_BUTTON,
	INPUTEVENT_JOY1_CD32_RED, INPUTEVENT_JOY1_CD32_BLUE, INPUTEVENT_JOY1_CD32_GREEN, INPUTEVENT_JOY1_CD32_YELLOW,
	INPUTEVENT_JOY1_CD32_RWD, INPUTEVENT_JOY1_CD32_FFW, INPUTEVENT_JOY1_CD32_PLAY,
	INPUTEVENT_MOUSE_CDTV_HORIZ, INPUTEVENT_MOUSE_CDTV_VERT,
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT,
	-1
};
static const int rem_port2[] = {
	INPUTEVENT_MOUSE2_HORIZ, INPUTEVENT_MOUSE2_VERT,
	INPUTEVENT_JOY2_HORIZ, INPUTEVENT_JOY2_VERT,
	INPUTEVENT_JOY2_HORIZ_POT, INPUTEVENT_JOY2_VERT_POT,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON, INPUTEVENT_JOY2_3RD_BUTTON,
	INPUTEVENT_JOY2_CD32_RED, INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW, INPUTEVENT_JOY2_CD32_PLAY,
	-1, -1,
	-1, -1,
	-1
};
static const int rem_port3[] = {
	INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT, INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN,
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON, INPUTEVENT_PAR_JOY1_2ND_BUTTON,
	-1
};
static const int rem_port4[] = {
	INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT, INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN,
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON, INPUTEVENT_PAR_JOY2_2ND_BUTTON,
	-1
};

static const int *rem_ports[] = { rem_port1, rem_port2, rem_port3, rem_port4 };
static const int ip_joy1[] = {
	INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON,
	-1
};
static const int ip_joy2[] = {
	INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON,
	-1
};
static const int ip_joypad1[] = {
	INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON, INPUTEVENT_JOY1_3RD_BUTTON,
	-1
};
static const int ip_joypad2[] = {
	INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON, INPUTEVENT_JOY2_3RD_BUTTON,
	-1
};
static const int ip_joycd321[] = {
	INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY1_CD32_RED, INPUTEVENT_JOY1_CD32_BLUE, INPUTEVENT_JOY1_CD32_GREEN, INPUTEVENT_JOY1_CD32_YELLOW,
	INPUTEVENT_JOY1_CD32_RWD, INPUTEVENT_JOY1_CD32_FFW, INPUTEVENT_JOY1_CD32_PLAY,
	-1
};
static const int ip_joycd322[] = {
	INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_JOY2_CD32_RED, INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW, INPUTEVENT_JOY2_CD32_PLAY,
	-1
};
static const int ip_parjoy1[] = {
	INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT, INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN,
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON, INPUTEVENT_PAR_JOY1_2ND_BUTTON,
	-1
};
static const int ip_parjoy2[] = {
	INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT, INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN,
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON, INPUTEVENT_PAR_JOY2_2ND_BUTTON,
	-1
};
static const int ip_parjoy1default[] = {
	INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT, INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN,
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON,
	-1
};
static const int ip_parjoy2default[] = {
	INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT, INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN,
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON,
	-1
};
static const int ip_mouse1[] = {
	INPUTEVENT_MOUSE1_LEFT, INPUTEVENT_MOUSE1_RIGHT, INPUTEVENT_MOUSE1_UP, INPUTEVENT_MOUSE1_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON,
	-1
};
static const int ip_mouse2[] = {
	INPUTEVENT_MOUSE2_LEFT, INPUTEVENT_MOUSE2_RIGHT, INPUTEVENT_MOUSE2_UP, INPUTEVENT_MOUSE2_DOWN,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON,
	-1
};
static const int ip_mousecdtv[] =
{
	INPUTEVENT_MOUSE_CDTV_LEFT, INPUTEVENT_MOUSE_CDTV_RIGHT, INPUTEVENT_MOUSE_CDTV_UP, INPUTEVENT_MOUSE_CDTV_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON,
	-1
};
static const int ip_mediacdtv[] =
{
	INPUTEVENT_KEY_CDTV_PLAYPAUSE, INPUTEVENT_KEY_CDTV_STOP, INPUTEVENT_KEY_CDTV_PREV, INPUTEVENT_KEY_CDTV_NEXT,
	-1
};
static const int ip_arcadia[] = {
	INPUTEVENT_SPC_ARCADIA_DIAGNOSTICS, INPUTEVENT_SPC_ARCADIA_PLAYER1, INPUTEVENT_SPC_ARCADIA_PLAYER2,
	INPUTEVENT_SPC_ARCADIA_COIN1, INPUTEVENT_SPC_ARCADIA_COIN2,
	-1
};
static const int ip_analog1[] = {
	INPUTEVENT_JOY1_HORIZ_POT, INPUTEVENT_JOY1_VERT_POT, INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT,
	-1
};
static const int ip_analog2[] = {
	INPUTEVENT_JOY2_HORIZ_POT, INPUTEVENT_JOY2_VERT_POT, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	-1
};

static const int ip_lightpen1[] = {
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_JOY1_3RD_BUTTON,
	-1
};
static const int ip_lightpen2[] = {
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_JOY2_3RD_BUTTON,
	-1
};
static const int ip_lightpen1_trojan[] = {
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_JOY1_LEFT,
	-1
};
static const int ip_lightpen2_trojan[] = {
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_JOY2_LEFT,
	-1
};


static const int ip_arcadiaxa[] = {
	-1
};

static void checkcompakb (int *kb, const int *srcmap)
{
	int found = 0, avail = 0;
	int j, k;

	k = j = 0;
	while (kb[j] >= 0) {
		struct uae_input_device *uid = &keyboards[0];
		while (kb[j] >= 0 && srcmap[k] >= 0) {
			int id = kb[j];
			for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
				if (uid->extra[l] == id) {
					avail++;
					if (uid->eventid[l][0] == srcmap[k])
						found++;
					break;
				}
			}
			j++;
		}
		if (srcmap[k] < 0)
			break;
		j++;
		k++;
	}
	if (avail != found || avail == 0)
		return;
	k = j = 0;
	while (kb[j] >= 0) {
		struct uae_input_device *uid = &keyboards[0];
		while (kb[j] >= 0) {
			int id = kb[j];
			k = 0;
			while (keyboard_default[k].scancode >= 0) {
				if (keyboard_default[k].scancode == kb[j]) {
					for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
						if (uid->extra[l] == id && uid->port[l][0] == 0) {
							for (int m = 0; m < MAX_INPUT_SUB_EVENT && keyboard_default[k].node[m].evt; m++) {
								uid->eventid[l][m] = keyboard_default[k].node[m].evt;
								uid->port[l][m] = 0;
								uid->flags[l][m] = 0;
							}
							break;
						}
					}
					break;
				}
				k++;
			}
			j++;
		}
		j++;
	}
}

static void inputdevice_sparerestore (struct uae_input_device *uid, int num, int sub)
{
	if (sub == 0 && uid->port[num][SPARE_SUB_EVENT]) {
		uid->eventid[num][sub] = uid->eventid[num][SPARE_SUB_EVENT];
		uid->flags[num][sub] = uid->flags[num][SPARE_SUB_EVENT];
		uid->custom[num][sub] = uid->custom[num][SPARE_SUB_EVENT];
	} else {
		uid->eventid[num][sub] = 0;
		uid->flags[num][sub] = 0;
		xfree (uid->custom[num][sub]);
		uid->custom[num][sub] = 0;
	}
	uid->eventid[num][SPARE_SUB_EVENT] = 0;
	uid->flags[num][SPARE_SUB_EVENT] = 0;
	uid->port[num][SPARE_SUB_EVENT] = 0;
	uid->custom[num][SPARE_SUB_EVENT] = 0;
	if (uid->flags[num][MAX_INPUT_SUB_EVENT - 1] & ID_FLAG_RESERVEDGAMEPORTSCUSTOM) {
		uid->eventid[num][MAX_INPUT_SUB_EVENT - 1] = 0;
		uid->flags[num][MAX_INPUT_SUB_EVENT - 1] = 0;
		uid->port[num][MAX_INPUT_SUB_EVENT - 1] = 0;
		uid->custom[num][MAX_INPUT_SUB_EVENT - 1] = 0;
	}
}

void inputdevice_sparecopy (struct uae_input_device *uid, int num, int sub)
{
	if (uid->port[num][SPARE_SUB_EVENT] != 0)
		return;
	if (uid->eventid[num][sub] <= 0 && uid->custom[num][sub] == NULL) {
		uid->eventid[num][SPARE_SUB_EVENT] = 0;
		uid->flags[num][SPARE_SUB_EVENT] = 0;
		uid->port[num][SPARE_SUB_EVENT] = 0;
		xfree (uid->custom[num][SPARE_SUB_EVENT]);
		uid->custom[num][SPARE_SUB_EVENT] = NULL;
	} else {
		uid->eventid[num][SPARE_SUB_EVENT] = uid->eventid[num][sub];
		uid->flags[num][SPARE_SUB_EVENT] = uid->flags[num][sub];
		uid->port[num][SPARE_SUB_EVENT] = MAX_JPORTS + 1;
		xfree (uid->custom[num][SPARE_SUB_EVENT]);
		uid->custom[num][SPARE_SUB_EVENT] = uid->custom[num][sub];
		uid->custom[num][sub] = NULL;
	}
}

static void setcompakb (struct uae_prefs *p, int *kb, const int *srcmap, int index, int af)
{
	int j, k;
	k = j = 0;
	while (kb[j] >= 0 && srcmap[k] >= 0) {
		while (kb[j] >= 0) {
			int id = kb[j];
			// default and active KB only
			for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
				struct uae_input_device *uid = &keyboards[m];
				if (m == 0 || uid->enabled) {
					for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
						if (uid->extra[l] == id) {
							setcompakbevent(p, uid, l, srcmap[k], index, af, 0);
							break;
						}
					}
				}
			}
			j++;
		}
		j++;
		k++;
	}
}

int inputdevice_get_compatibility_input (struct uae_prefs *prefs, int index, int *typelist, int *inputlist, const int **at)
{
	if (index >= MAX_JPORTS || joymodes[index] < 0)
		return -1;
	if (typelist != NULL)
		*typelist = joymodes[index];
	if (at != NULL)
		*at = axistable;
	if (inputlist == NULL)
		return -1;
	
	//write_log (_T("%d %p %p\n"), *typelist, *inputlist, *at);
	int cnt;
	for (cnt = 0; joyinputs[index] && joyinputs[index][cnt] >= 0; cnt++) {
		inputlist[cnt] = joyinputs[index][cnt];
	}
	inputlist[cnt] = -1;

	// find custom events (custom event = event that is mapped to same port but not included in joyinputs[]
	int devnum = 0;
	while (inputdevice_get_device_status (devnum) >= 0) {
		for (int j = 0; j < inputdevice_get_widget_num (devnum); j++) {
			for (int sub = 0; sub < MAX_INPUT_SUB_EVENT; sub++) {
				int port, k, l;
				uae_u64 flags;
				bool ignore = false;
				int evtnum2 = inputdevice_get_mapping (devnum, j, &flags, &port, NULL, NULL, sub);
				if (port - 1 != index)
					continue;
				for (k = 0; axistable[k] >= 0; k += 3) {
					if (evtnum2 == axistable[k] || evtnum2 == axistable[k + 1] || evtnum2 == axistable[k + 2]) {
						for (l = 0; inputlist[l] >= 0; l++) {
							if (inputlist[l] == axistable[k] || inputlist[l] == axistable[k + 1] || inputlist[l] == axistable[k + 1]) {
								ignore = true;
							}
						}
					}
				}
				if (!ignore) {
					for (k = 0; inputlist[k] >= 0; k++) {
						if (evtnum2 == inputlist[k])
							break;
					}
					if (inputlist[k] < 0) {
						inputlist[k] = evtnum2;
						inputlist[k + 1] = -1;
						cnt++;
					}
				}
			}
		}
		devnum++;
	}
#if 0
	for (int i = 0; inputlist[i] >= 0; i++) {
		struct inputevent *evt = inputdevice_get_eventinfo (inputlist[i]);
		write_log (_T("%d: %d %d %s\n"), i, index, inputlist[i], evt->name);
	}
#endif
	//write_log (_T("%d\n"), cnt);
	return cnt;
}

static void clearevent (struct uae_input_device *uid, int evt)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid->eventid[i][j] == evt) {
				uid->eventid[i][j] = 0;
				uid->flags[i][j] &= COMPA_RESERVED_FLAGS;
				xfree (uid->custom[i][j]);
				uid->custom[i][j] = NULL;
			}
		}
	}
}
static void clearkbrevent (struct uae_input_device *uid, int evt)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid->eventid[i][j] == evt) {
				uid->eventid[i][j] = 0;
				uid->flags[i][j] &= COMPA_RESERVED_FLAGS;
				xfree (uid->custom[i][j]);
				uid->custom[i][j] = NULL;
				if (j == 0)
					set_kbr_default_event (uid, keyboard_default, i);
			}
		}
	}
}

static void resetjport (struct uae_prefs *prefs, int index)
{
	const int *p = rem_ports[index];
	while (*p >= 0) {
		int evtnum = *p++;
		for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
			clearevent (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
			clearevent (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
			clearkbrevent (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
		}
		for (int i = 0; axistable[i] >= 0; i += 3) {
			if (evtnum == axistable[i] || evtnum == axistable[i + 1] || evtnum == axistable[i + 2]) {
				for (int j = 0; j < 3; j++) {
					int evtnum2 = axistable[i + j];
					for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
						clearevent (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum2);
						clearevent (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum2);
						clearkbrevent (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum2);
					}
				}
				break;
			}
		}
	}
}

static void remove_compa_config (struct uae_prefs *prefs, int index)
{
	int typelist;
	const int *atpp;
	int inputlist[MAX_COMPA_INPUTLIST];

	if (inputdevice_get_compatibility_input (prefs, index, &typelist, inputlist, &atpp) <= 0)
		return;
	for (int i = 0; inputlist[i] >= 0; i++) {
		int evtnum = inputlist[i];
		const int *atp = atpp;

		int atpidx = 0;
		while (*atp >= 0) {
			if (*atp == evtnum) {
				atp++;
				atpidx = 2;
				break;
			}
			if (atp[1] == evtnum || atp[2] == evtnum) {
				atpidx = 1;
				break;
			}
			atp += 3;
		}
		while (atpidx >= 0) {
			for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
				clearevent (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
				clearevent (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
				clearkbrevent (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
			}
			evtnum = *atp++;
			atpidx--;
		}
	}
}

static void cleardev_custom (struct uae_input_device *uid, int num, int index)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid[num].port[i][j] == index + 1) {
				uid[num].eventid[i][j] = 0;
				uid[num].flags[i][j] &= COMPA_RESERVED_FLAGS;
				xfree (uid[num].custom[i][j]);
				uid[num].custom[i][j] = NULL;
				uid[num].port[i][j] = 0;
				if (uid[num].port[i][SPARE_SUB_EVENT]) {
					inputdevice_sparerestore (&uid[num], i, j);
				}
			}
		}
	}
}
static void cleardevkbr_custom(struct uae_input_device *uid, int num, int index)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid[num].port[i][j] == index + 1) {
				uid[num].eventid[i][j] = 0;
				uid[num].flags[i][j] &= COMPA_RESERVED_FLAGS;
				xfree (uid[num].custom[i][j]);
				uid[num].custom[i][j] = NULL;
				uid[num].port[i][j] = 0;
				if (uid[num].port[i][SPARE_SUB_EVENT]) {
					inputdevice_sparerestore (&uid[num], i, j);
				} else if (j == 0) {
					set_kbr_default_event (&uid[num], keyboard_default, i);
				}
			}
		}
	}
}

// remove all gameports mappings mapped to port 'index'
static void remove_custom_config (struct uae_prefs *prefs, int index)
{
	for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
		cleardev_custom(joysticks, l, index);
		cleardev_custom(mice, l, index);
		cleardevkbr_custom (keyboards, l, index);
	}
}

// prepare port for custom mapping, remove all current Amiga side device mappings
void inputdevice_compa_prepare_custom (struct uae_prefs *prefs, int index, int newmode, bool removeold)
{
	int mode = prefs->jports[index].mode;
	if (newmode >= 0) {
		mode = newmode;
	} else if (mode == 0) {
		mode = index == 0 ? JSEM_MODE_WHEELMOUSE : (prefs->cs_cd32cd ? JSEM_MODE_JOYSTICK_CD32 : JSEM_MODE_JOYSTICK);
	}
	prefs->jports[index].mode = mode;

	if (removeold) {
		prefs->jports_custom[JSEM_GETCUSTOMIDX(index, prefs)].custom[0] = 0;
		remove_compa_config (prefs, index);
		remove_custom_config (prefs, index);
	}
}
// clear device before switching to new one
void inputdevice_compa_clear (struct uae_prefs *prefs, int index)
{
	freejport (prefs, index);
	resetjport (prefs, index);
	remove_compa_config (prefs, index);
}

static void cleardev (struct uae_input_device *uid, int num)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		inputdevice_sparecopy (&uid[num], i, 0);
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			uid[num].eventid[i][j] = 0;
			uid[num].flags[i][j] &= ID_FLAG_AUTOFIRE_MASK;
			xfree (uid[num].custom[i][j]);
			uid[num].custom[i][j] = NULL;
		}
	}
}

static void enablejoydevice (struct uae_input_device *uid, bool gameportsmode, int evtnum)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if ((gameportsmode && uid->eventid[i][j] == evtnum) || uid->port[i][j] > 0) {
				if (!uid->enabled)
					uid->enabled = -1;
			}
		}
	}
}

static void setjoydevices (struct uae_prefs *prefs, bool gameportsmode, int port)
{
	for (int i = 0; joyinputs[port] && joyinputs[port][i] >= 0; i++) {
		int evtnum = joyinputs[port][i];
		for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
			enablejoydevice (&joysticks[l], gameportsmode, evtnum);
			enablejoydevice (&mice[l], gameportsmode, evtnum);
			//enablejoydevice (&keyboards[l], gameportsmode, evtnum);
		}
		for (int k = 0; axistable[k] >= 0; k += 3) {
			if (evtnum == axistable[k] || evtnum == axistable[k + 1] || evtnum == axistable[k + 2]) {
				for (int j = 0; j < 3; j++) {
					int evtnum2 = axistable[k + j];
					for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
						enablejoydevice (&joysticks[l], gameportsmode, evtnum2);
						enablejoydevice (&mice[l], gameportsmode, evtnum2);
						//enablejoydevice (&keyboards[l], gameportsmode, evtnum2);
					}
				}
				break;
			}
		}

	}
}

static const int *getlightpen(int port, int submode)
{
	switch (submode)
	{
	case 1:
		return port ? ip_lightpen2_trojan : ip_lightpen1_trojan;
	default:
		return port ? ip_lightpen2 : ip_lightpen1;
	}
}

static void setjoyinputs (struct uae_prefs *prefs, int port)
{
	joyinputs[port] = NULL;
	switch (joymodes[port])
	{
		case JSEM_MODE_JOYSTICK:
			if (port >= 2)
				joyinputs[port] = port == 3 ? ip_parjoy2 : ip_parjoy1;
			else
				joyinputs[port] = port == 1 ? ip_joy2 : ip_joy1;
		break;
		case JSEM_MODE_GAMEPAD:
			joyinputs[port] = port ? ip_joypad2 : ip_joypad1;
		break;
		case JSEM_MODE_JOYSTICK_CD32:
			joyinputs[port] = port ? ip_joycd322 : ip_joycd321;
		break;
		case JSEM_MODE_JOYSTICK_ANALOG:
			joyinputs[port] = port ? ip_analog2 : ip_analog1;
		break;
		case JSEM_MODE_WHEELMOUSE:
		case JSEM_MODE_MOUSE:
			joyinputs[port] = port ? ip_mouse2 : ip_mouse1;
		break;
		case JSEM_MODE_LIGHTPEN:
			joyinputs[port] = getlightpen(port, joysubmodes[port]);
		break;
		break;
		case JSEM_MODE_MOUSE_CDTV:
			joyinputs[port] = ip_mousecdtv;
		break;
	}
	//write_log (_T("joyinput %d = %p\n"), port, joyinputs[port]);
}

static void setautofire (struct uae_input_device *uid, int port, int sub, int af, bool set)
{
	const int *afp = af_ports[port];
	for (int k = 0; afp[k] >= 0; k++) {
		for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
			for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
				if (uid->eventid[i][j] == afp[k]) {
					if (!set) {
						uid->flags[i][j] &= ~ID_FLAG_AUTOFIRE_MASK;
					} else {
						if (af >= JPORT_AF_NORMAL)
							uid->flags[i][j] |= ID_FLAG_AUTOFIRE;
						if (af == JPORT_AF_TOGGLE)
							uid->flags[i][j] |= ID_FLAG_TOGGLE;
						if (af == JPORT_AF_ALWAYS)
							uid->flags[i][j] |= ID_FLAG_INVERTTOGGLE;
						if (af == JPORT_AF_TOGGLENOAF)
							uid->flags[i][j] |= ID_FLAG_INVERT;
					}
				}
			}
		}
	}
}

static void setautofires (struct uae_prefs *prefs, int port, int sub, int af, bool set)
{
#ifdef RETROPLATFORM
	// don't override custom AF autofire mappings
	if (rp_isactive())
		return;
#endif
	for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
		setautofire(&joysticks[l], port, sub, af, set);
		setautofire(&mice[l], port, sub, af, set);
		setautofire(&keyboards[l], port, sub, af, set);
	}
}

// merge gameport settings with current input configuration
static void compatibility_copy (struct uae_prefs *prefs, bool gameports)
{
	int used[MAX_INPUT_DEVICES] = { 0 };
	int i, joy;

	for (i = 0; i < MAX_JPORTS; i++) {
		joymodes[i] = prefs->jports[i].mode;
		joysubmodes[i] = prefs->jports[i].submode;
		joyinputs[i] = NULL;
		// remove all mappings from this port
		if (gameports)
			remove_compa_config (prefs, i);
		remove_custom_config (prefs, i);
		setjoyinputs (prefs, i);
	}

	for (i = 0; i < 2; i++) {
		if (prefs->jports[i].id >= 0 && joymodes[i] <= 0) {
			int mode = prefs->jports[i].mode;
			if (jsem_ismouse (i, prefs) >= 0) {
				switch (mode)
				{
					case JSEM_MODE_DEFAULT:
					case JSEM_MODE_MOUSE:
					case JSEM_MODE_WHEELMOUSE:
					default:
					joymodes[i] = JSEM_MODE_WHEELMOUSE;
					joyinputs[i] = i ? ip_mouse2 : ip_mouse1;
					break;
					case JSEM_MODE_LIGHTPEN:
					joymodes[i] = JSEM_MODE_LIGHTPEN;
					joyinputs[i] = getlightpen(i, joysubmodes[i]);
					break;
					case JSEM_MODE_MOUSE_CDTV:
					joymodes[i] = JSEM_MODE_MOUSE_CDTV;
					joyinputs[i] = ip_mousecdtv;
					break;
				}
			} else if (jsem_isjoy (i, prefs) >= 0) {
				switch (mode)
				{
					case JSEM_MODE_DEFAULT:
					case JSEM_MODE_JOYSTICK:
					case JSEM_MODE_GAMEPAD:
					case JSEM_MODE_JOYSTICK_CD32:
					default:
					{
						bool iscd32 = mode == JSEM_MODE_JOYSTICK_CD32 || (mode == JSEM_MODE_DEFAULT && prefs->cs_cd32cd);
						if (iscd32) {
							joymodes[i] = JSEM_MODE_JOYSTICK_CD32;
							joyinputs[i] = i ? ip_joycd322 : ip_joycd321;
						} else if (mode == JSEM_MODE_GAMEPAD) {
							joymodes[i] = JSEM_MODE_GAMEPAD;
							joyinputs[i] = i ? ip_joypad2 : ip_joypad1;
						} else {
							joymodes[i] = JSEM_MODE_JOYSTICK;
							joyinputs[i] = i ? ip_joy2 : ip_joy1;
						}
						break;
					}
					case JSEM_MODE_JOYSTICK_ANALOG:
						joymodes[i] = JSEM_MODE_JOYSTICK_ANALOG;
						joyinputs[i] = i ? ip_analog2 : ip_analog1;
						break;
					case JSEM_MODE_MOUSE:
					case JSEM_MODE_WHEELMOUSE:
						joymodes[i] = JSEM_MODE_WHEELMOUSE;
						joyinputs[i] = i ? ip_mouse2 : ip_mouse1;
						break;
					case JSEM_MODE_LIGHTPEN:
						joymodes[i] = JSEM_MODE_LIGHTPEN;
						joyinputs[i] = getlightpen(i, joysubmodes[i]);
						break;
					case JSEM_MODE_MOUSE_CDTV:
						joymodes[i] = JSEM_MODE_MOUSE_CDTV;
						joyinputs[i] = ip_mousecdtv;
						break;
				}
			} else if (prefs->jports[i].id >= 0) {
				joymodes[i] = i ? JSEM_MODE_JOYSTICK : JSEM_MODE_WHEELMOUSE;
				joyinputs[i] = i ? ip_joy2 : ip_mouse1;
			}
		}
	}

	for (i = 2; i < MAX_JPORTS; i++) {
		if (prefs->jports[i].id >= 0 && joymodes[i] <= 0) {
			if (jsem_isjoy (i, prefs) >= 0) {
				joymodes[i] = JSEM_MODE_JOYSTICK;
				joyinputs[i] = i == 3 ? ip_parjoy2 : ip_parjoy1;
			} else if (prefs->jports[i].id >= 0) {
				prefs->jports[i].mode = joymodes[i] = JSEM_MODE_JOYSTICK;
				joyinputs[i] = i == 3 ? ip_parjoy2 : ip_parjoy1;
			}
		}
	}

	for (i = 0; i < 2; i++) {
		int af = prefs->jports[i].autofire;
		if (prefs->jports[i].id >= 0) {
			int mode = prefs->jports[i].mode;
			int submode = prefs->jports[i].submode;
			if ((joy = jsem_ismouse (i, prefs)) >= 0) {
				if (gameports)
					cleardev (mice, joy);
				switch (mode)
				{
				case JSEM_MODE_DEFAULT:
				case JSEM_MODE_MOUSE:
				case JSEM_MODE_WHEELMOUSE:
				default:
					input_get_default_mouse (mice, joy, i, af, !gameports, mode != JSEM_MODE_MOUSE, false);
					joymodes[i] = JSEM_MODE_WHEELMOUSE;
					break;
				case JSEM_MODE_LIGHTPEN:
					input_get_default_lightpen (mice, joy, i, af, !gameports, false, submode);
					joymodes[i] = JSEM_MODE_LIGHTPEN;
					break;
				case JSEM_MODE_JOYSTICK:
				case JSEM_MODE_GAMEPAD:
				case JSEM_MODE_JOYSTICK_CD32:
					input_get_default_joystick (mice, joy, i, af, mode, !gameports, true, prefs->input_default_onscreen_keyboard);
					joymodes[i] = mode;
					break;
				case JSEM_MODE_JOYSTICK_ANALOG:
					input_get_default_joystick_analog (mice, joy, i, af, !gameports, true, prefs->input_default_onscreen_keyboard);
					joymodes[i] = JSEM_MODE_JOYSTICK_ANALOG;
					break;
				}
				_tcsncpy (prefs->jports[i].idc.name, idev[IDTYPE_MOUSE].get_friendlyname (joy), MAX_JPORT_NAME - 1);
				_tcsncpy (prefs->jports[i].idc.configname, idev[IDTYPE_MOUSE].get_uniquename (joy), MAX_JPORT_CONFIG - 1);
				prefs->jports[i].idc.name[MAX_JPORT_NAME - 1] = 0;
				prefs->jports[i].idc.configname[MAX_JPORT_CONFIG - 1] = 0;
			}
		}
	}

	for (i = 1; i >= 0; i--) {
		int af = prefs->jports[i].autofire;
		if (prefs->jports[i].id >= 0) {
			int mode = prefs->jports[i].mode;
			int submode = prefs->jports[i].submode;
			joy = jsem_isjoy (i, prefs);
			if (joy >= 0) {
				if (gameports)
					cleardev (joysticks, joy);
				switch (mode)
				{
				case JSEM_MODE_DEFAULT:
				case JSEM_MODE_JOYSTICK:
				case JSEM_MODE_GAMEPAD:
				case JSEM_MODE_JOYSTICK_CD32:
				default:
				{
					bool iscd32 = mode == JSEM_MODE_JOYSTICK_CD32 || (mode == JSEM_MODE_DEFAULT && prefs->cs_cd32cd);
					int jmode = iscd32 ? JSEM_MODE_JOYSTICK_CD32 : mode;
					input_get_default_joystick(joysticks, joy, i, af, jmode, !gameports, false, prefs->input_default_onscreen_keyboard);
					if (iscd32)
						joymodes[i] = JSEM_MODE_JOYSTICK_CD32;
					else if (mode == JSEM_MODE_GAMEPAD)
						joymodes[i] = JSEM_MODE_GAMEPAD;
					else
						joymodes[i] = JSEM_MODE_JOYSTICK;
					break;
				}
				case JSEM_MODE_JOYSTICK_ANALOG:
					input_get_default_joystick_analog(joysticks, joy, i, af, !gameports, false, prefs->input_default_onscreen_keyboard);
					joymodes[i] = JSEM_MODE_JOYSTICK_ANALOG;
					break;
				case JSEM_MODE_MOUSE:
				case JSEM_MODE_WHEELMOUSE:
#ifdef AMIBERRY
					// This allows us to use Retroarch hotkeys when having a controller in Mouse mode
					if (currprefs.jports[i].id >= JSEM_MICE && currprefs.jports[i].id < JSEM_END)
					{
						input_get_default_mouse(joysticks, joy, i, af, !gameports, mode == JSEM_MODE_WHEELMOUSE, true);
						joymodes[i] = JSEM_MODE_WHEELMOUSE;
					}
					else
					{
						input_get_default_joystick(joysticks, joy, i, af, mode, !gameports, false, prefs->input_default_onscreen_keyboard);
						joymodes[i] = JSEM_MODE_WHEELMOUSE;
					}
#endif
					break;
				case JSEM_MODE_LIGHTPEN:
					input_get_default_lightpen(joysticks, joy, i, af, !gameports, true, submode);
					joymodes[i] = JSEM_MODE_LIGHTPEN;
					break;
				case JSEM_MODE_MOUSE_CDTV:
					joymodes[i] = JSEM_MODE_MOUSE_CDTV;
					input_get_default_joystick(joysticks, joy, i, af, mode, !gameports, false, prefs->input_default_onscreen_keyboard);
					break;

				}
				_tcsncpy(prefs->jports[i].idc.name, idev[IDTYPE_JOYSTICK].get_friendlyname (joy), MAX_JPORT_NAME - 1);
				_tcsncpy(prefs->jports[i].idc.configname, idev[IDTYPE_JOYSTICK].get_uniquename (joy), MAX_JPORT_CONFIG - 1);
				prefs->jports[i].idc.name[MAX_JPORT_NAME - 1] = 0;
				prefs->jports[i].idc.configname[MAX_JPORT_CONFIG - 1] = 0;
				used[joy] = 1;
			}
		}
	}

	if (gameports) {
		// replace possible old mappings with default keyboard mapping
		for (i = KBR_DEFAULT_MAP_FIRST; i <= KBR_DEFAULT_MAP_LAST; i++) {
			checkcompakb (keyboard_default_kbmaps[i], ip_joy2);
			checkcompakb (keyboard_default_kbmaps[i], ip_joy1);
			checkcompakb (keyboard_default_kbmaps[i], ip_joypad2);
			checkcompakb (keyboard_default_kbmaps[i], ip_joypad1);
			checkcompakb (keyboard_default_kbmaps[i], ip_parjoy2);
			checkcompakb (keyboard_default_kbmaps[i], ip_parjoy1);
			checkcompakb (keyboard_default_kbmaps[i], ip_mouse2);
			checkcompakb (keyboard_default_kbmaps[i], ip_mouse1);
		}
		for (i = KBR_DEFAULT_MAP_CD32_FIRST; i <= KBR_DEFAULT_MAP_CD32_LAST; i++) {
			checkcompakb (keyboard_default_kbmaps[i], ip_joycd321);
			checkcompakb (keyboard_default_kbmaps[i], ip_joycd322);
		}
	}

	for (i = 0; i < 2; i++) {
		if (prefs->jports[i].id >= 0) {
			int *kb = NULL;
			int mode = prefs->jports[i].mode;
			int af = prefs->jports[i].autofire;
			for (joy = 0; used[joy]; joy++);
			if (JSEM_ISANYKBD (i, prefs)) {
				bool cd32 = mode == JSEM_MODE_JOYSTICK_CD32 || (mode == JSEM_MODE_DEFAULT && prefs->cs_cd32cd);
				if (JSEM_ISNUMPAD (i, prefs)) {
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_NP];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_NP3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_NP];
				} else if (JSEM_ISCURSOR (i, prefs)) {
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_CK];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CK3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CK];
				} else if (JSEM_ISSOMEWHEREELSE(i, prefs)) {
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_SE];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_SE3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_SE];
				}
#ifdef AMIBERRY
				else if (JSEM_ISKEYRAH(i, prefs)) 
				{
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_KEYRAH];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_KEYRAH3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_KEYRAH];
				}
				else if (JSEM_ISRAPLAYER1(i, prefs))
				{
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_RAPLAYER1];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER1_3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER1];
				}
				else if (JSEM_ISRAPLAYER2(i, prefs))
				{
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_RAPLAYER2];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER2_3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER2];
				}
				else if (JSEM_ISRAPLAYER3(i, prefs))
				{
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_RAPLAYER3];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER3_3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER3];
				}
				else if (JSEM_ISRAPLAYER4(i, prefs))
				{
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_RAPLAYER4];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER4_3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_RAPLAYER4];
				}
#endif
				if (kb) {
					switch (mode)
					{
					case JSEM_MODE_JOYSTICK:
					case JSEM_MODE_GAMEPAD:
					case JSEM_MODE_JOYSTICK_CD32:
					case JSEM_MODE_DEFAULT:
						if (cd32) {
							setcompakb (prefs, kb, i ? ip_joycd322 : ip_joycd321, i, af);
							joymodes[i] = JSEM_MODE_JOYSTICK_CD32;
						} else if (mode == JSEM_MODE_GAMEPAD) {
							setcompakb (prefs, kb, i ? ip_joypad2 : ip_joypad1, i, af);
							joymodes[i] = JSEM_MODE_GAMEPAD;
						} else {
							setcompakb (prefs, kb, i ? ip_joy2 : ip_joy1, i, af);
							joymodes[i] = JSEM_MODE_JOYSTICK;
						}
						break;
					case JSEM_MODE_MOUSE:
					case JSEM_MODE_WHEELMOUSE:
						setcompakb (prefs, kb, i ? ip_mouse2 : ip_mouse1, i, af);
						joymodes[i] = JSEM_MODE_WHEELMOUSE;
						break;
					}
					used[joy] = 1;
				}
			}
		}
	}
#ifdef ARCADIA
	if (arcadia_bios) {
		setcompakb (prefs, keyboard_default_kbmaps[KBR_DEFAULT_MAP_ARCADIA], ip_arcadia, 0, 0);
	}
#endif
	if (0 && currprefs.cs_cdtvcd) {
		setcompakb (prefs, keyboard_default_kbmaps[KBR_DEFAULT_MAP_CDTV], ip_mediacdtv, 0, 0);
	}

	// parport
	for (i = 2; i < MAX_JPORTS; i++) {
		int af = prefs->jports[i].autofire;
		if (prefs->jports[i].id >= 0) {
			joy = jsem_isjoy (i, prefs);
			if (joy >= 0) {
				if (gameports)
					cleardev (joysticks, joy);
				input_get_default_joystick (joysticks, joy, i, af, 0, !gameports, false, prefs->input_default_onscreen_keyboard);
				_tcsncpy (prefs->jports[i].idc.name, idev[IDTYPE_JOYSTICK].get_friendlyname (joy), MAX_JPORT_NAME - 1);
				_tcsncpy (prefs->jports[i].idc.configname, idev[IDTYPE_JOYSTICK].get_uniquename (joy), MAX_JPORT_CONFIG - 1);
				prefs->jports[i].idc.name[MAX_JPORT_NAME - 1] = 0;
				prefs->jports[i].idc.configname[MAX_JPORT_CONFIG - 1] = 0;
				used[joy] = 1;
				joymodes[i] = JSEM_MODE_JOYSTICK;
			}
		}
	}
	for (i = 2; i < MAX_JPORTS; i++) {
		if (prefs->jports[i].id >= 0) {
			int *kb = NULL;
			for (joy = 0; used[joy]; joy++);
			if (JSEM_ISANYKBD (i, prefs)) {
				if (JSEM_ISNUMPAD (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_NP];
				else if (JSEM_ISCURSOR (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CK];
				else if (JSEM_ISSOMEWHEREELSE (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_SE];
				if (kb) {
					setcompakb(prefs, kb, i == 3 ? ip_parjoy2default : ip_parjoy1default, i, prefs->jports[i].autofire);
					used[joy] = 1;
					joymodes[i] = JSEM_MODE_JOYSTICK;
				}
			}
		}
	}

	for (i = 0; i < MAX_JPORTS; i++) {
		if (JSEM_ISCUSTOM(i, prefs)) {
			inputdevice_parse_jport_custom(prefs, prefs->jports[i].id - JSEM_CUSTOM, i, NULL);
		}
		if (gameports)
			setautofires (prefs, i, 0, prefs->jports[i].autofire, true);
	}

	for (i = 0; i < MAX_JPORTS; i++) {
		setjoyinputs (prefs, i);
		setjoydevices (prefs, gameports, i);
	}
}

static void disableifempty2 (struct uae_input_device *uid)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid->eventid[i][j] > 0 || uid->custom[i][j] != NULL)
				return;
		}
	}
	if (uid->enabled < 0)
		uid->enabled = 0;
}
static void disableifempty (struct uae_prefs *prefs)
{
	for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
		disableifempty2 (&joysticks[l]);
		disableifempty2 (&mice[l]);
		if (!input_get_default_keyboard(l))
			disableifempty2 (&keyboards[l]);
	}
	// Configuration #1-#3
	prefs->internalevent_settings[0]->enabled = true;
	prefs->internalevent_settings[1]->enabled = true;
	prefs->internalevent_settings[2]->enabled = true;
}

static void matchdevices(struct uae_prefs *p, struct inputdevice_functions *inf, struct uae_input_device *uid, int devnum, int match_mask)
{
	int i, j;

	for (int l = 0; l < 2; l++) {
		bool fullmatch = l == 0;
		int match = -1;
		for (i = 0; i < inf->get_num (); i++) {
			const TCHAR* aname1 = inf->get_friendlyname (i);
			const TCHAR* aname2 = inf->get_uniquename (i);
			for (j = 0; j < MAX_INPUT_DEVICES; j++) {
				if (aname2 && uid[j].configname) {
					bool matched = false;
					TCHAR bname[MAX_DPATH];
					TCHAR bname2[MAX_DPATH];
					TCHAR *p1 ,*p2;
					TCHAR *bname1 = uid[j].name;

					if (fullmatch) {
						if (!(match_mask & INPUT_MATCH_BOTH))
							continue;
					} else {
						if (!(match_mask & INPUT_MATCH_CONFIG_NAME_ONLY))
							continue;
					}

					_tcscpy (bname, uid[j].configname);
					_tcscpy (bname2, aname2);
					// strip possible local guid part
					p1 = _tcschr (bname, '{');
					p2 = _tcschr (bname2, '{');
					if (!p1 && !p2) {
						// check possible directinput names too
						p1 = _tcschr (bname, ' ');
						p2 = _tcschr (bname2, ' ');
					}
					if (!_tcscmp (bname, bname2)) {
						matched = true;
					} else if (p1 && p2 && p1 - bname == p2 - bname2) {
						*p1 = 0;
						*p2 = 0;
						if (bname[0] && !_tcscmp (bname2, bname))
							matched = true;
					}
					if (matched && fullmatch && aname1 && bname1 && _tcscmp(aname1, bname1) != 0)
						matched = false;
					if (matched) {
						if (match >= 0)
							match = -2;
						else
							match = j;
					}
					if (match == -2)
						break;
				}
			}
			if (!fullmatch) {
				// multiple matches -> use complete local-only id string for comparisons
				if (match == -2) {
					for (j = 0; j < MAX_INPUT_DEVICES; j++) {
						TCHAR *bname2 = uid[j].configname;
						if (aname2 && bname2 && (match_mask & INPUT_MATCH_CONFIG_NAME_ONLY) && !_tcscmp (aname2, bname2)) {
							match = j;
							break;
						}
					}
				}
				if (match < 0) {
					// no match, try friendly names only
					for (j = 0; j < MAX_INPUT_DEVICES; j++) {
						TCHAR *bname1 = uid[j].name;
						if (aname1 && bname1 && (match_mask & INPUT_MATCH_FRIENDLY_NAME_ONLY) && !_tcscmp (aname1, bname1)) {
							match = j;
							break;
						}
					}
				}
			}
			if (match >= 0) {
				j = match;
				if (j != i) {
					struct uae_input_device *tmp = xmalloc (struct uae_input_device, 1);
					memcpy (tmp, &uid[j], sizeof (struct uae_input_device));
					memcpy (&uid[j], &uid[i], sizeof (struct uae_input_device));
					memcpy (&uid[i], tmp, sizeof (struct uae_input_device));
					xfree (tmp);
					gp_swappeddevices[j][devnum] = i;
					gp_swappeddevices[i][devnum] = j;
				}
			}
		}
		if (match >= 0)
			break;
	}
	for (i = 0; i < inf->get_num (); i++) {
		if (uid[i].name == NULL)
			uid[i].name = my_strdup (inf->get_friendlyname (i));
		if (uid[i].configname == NULL)
			uid[i].configname = my_strdup (inf->get_uniquename (i));
	}
}

static void matchdevices_all (struct uae_prefs *prefs)
{
	for (int i = 1; i < MAX_INPUT_SETTINGS; i++) {
		matchdevices(prefs, &idev[IDTYPE_MOUSE], prefs->mouse_settings[i], IDTYPE_MOUSE, prefs->input_device_match_mask);
		matchdevices(prefs, &idev[IDTYPE_JOYSTICK], prefs->joystick_settings[i], IDTYPE_JOYSTICK, prefs->input_device_match_mask);
		matchdevices(prefs, &idev[IDTYPE_KEYBOARD], prefs->keyboard_settings[i], IDTYPE_KEYBOARD, INPUT_MATCH_ALL);
	}
	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		for (int j = 0; j < IDTYPE_MAX; j++) {
			gp_swappeddevices[i][j] = -1;
		}
	}
	matchdevices(prefs, &idev[IDTYPE_MOUSE], prefs->mouse_settings[0], IDTYPE_MOUSE, prefs->input_device_match_mask);
	matchdevices(prefs, &idev[IDTYPE_JOYSTICK], prefs->joystick_settings[0], IDTYPE_JOYSTICK, prefs->input_device_match_mask);
	matchdevices(prefs, &idev[IDTYPE_KEYBOARD], prefs->keyboard_settings[0], IDTYPE_KEYBOARD, INPUT_MATCH_ALL);

}

bool inputdevice_set_gameports_mapping (struct uae_prefs *prefs, int devnum, int num, int evtnum, uae_u64 flags, int port, int input_selected_setting)
{
	TCHAR name[256];
	const struct inputevent *ie;
	int sub;

	if (evtnum < 0) {
		joysticks = prefs->joystick_settings[input_selected_setting];
		mice = prefs->mouse_settings[input_selected_setting];
		keyboards = prefs->keyboard_settings[input_selected_setting];
		for (sub = 0; sub < MAX_INPUT_SUB_EVENT; sub++) {
			int port2 = 0;
			inputdevice_get_mapping (devnum, num, NULL, &port2, NULL, NULL, sub);
			if (port2 == port + 1) {
				inputdevice_set_mapping (devnum, num, NULL, NULL, 0, 0, sub);
			}
		}
		return true;
	}
	ie = inputdevice_get_eventinfo (evtnum);
	if (!inputdevice_get_eventname (ie, name))
		return false;
	joysticks = prefs->joystick_settings[input_selected_setting];
	mice = prefs->mouse_settings[input_selected_setting];
	keyboards = prefs->keyboard_settings[input_selected_setting];

	sub = 0;
	if (inputdevice_get_widget_type (devnum, num, NULL, false) != IDEV_WIDGET_KEY) {
		for (sub = 0; sub < MAX_INPUT_SUB_EVENT; sub++) {
			int port2 = 0;
			int evt = inputdevice_get_mapping (devnum, num, NULL, &port2, NULL, NULL, sub);
			if (port2 == port + 1 && evt == evtnum)
				break;
			if (!inputdevice_get_mapping (devnum, num, NULL, NULL, NULL, NULL, sub))
				break;
		}
	}
	if (sub >= MAX_INPUT_SUB_EVENT)
		sub = MAX_INPUT_SUB_EVENT - 1;
	inputdevice_set_mapping (devnum, num, name, NULL, IDEV_MAPPED_GAMEPORTSCUSTOM1 | flags, port + 1, sub);

	joysticks = prefs->joystick_settings[prefs->input_selected_setting];
	mice = prefs->mouse_settings[prefs->input_selected_setting];
	keyboards = prefs->keyboard_settings[prefs->input_selected_setting];

	if (prefs->input_selected_setting != GAMEPORT_INPUT_SETTINGS) {
		int xport;
		uae_u64 xflags;
		TCHAR xname[MAX_DPATH], xcustom[MAX_DPATH];
		inputdevice_get_mapping (devnum, num, &xflags, &xport, xname, xcustom, 0);
		if (xport == 0)
			inputdevice_set_mapping (devnum, num, xname, xcustom, xflags, MAX_JPORTS + 1, SPARE_SUB_EVENT);
		inputdevice_set_mapping (devnum, num, name, NULL, IDEV_MAPPED_GAMEPORTSCUSTOM1 | flags, port + 1, 0);
	}
	return true;
}

static void resetinput (void)
{
	if ((input_play || input_record) && hsync_counter > 0)
		return;
	cd32_shifter[0] = cd32_shifter[1] = 8;
	for (int i = 0; i < MAX_JPORTS; i++) {
		oleft[i] = 0;
		oright[i] = 0;
		otop[i] = 0;
		obot[i] = 0;
		joybutton[i] = 0;
		joydir[i] = 0;
		mouse_deltanoreset[i][0] = 0;
		mouse_delta[i][0] = 0;
		mouse_deltanoreset[i][1] = 0;
		mouse_delta[i][1] = 0;
		mouse_deltanoreset[i][2] = 0;
		mouse_delta[i][2] = 0;
	}
	for (int i = 0; i < 2; i++) {
		lightpen_delta[i][0] = lightpen_delta[i][1] = 0;
		lightpen_deltanoreset[i][0] = lightpen_deltanoreset[i][1] = 0;
	}
	memset (keybuf, 0, sizeof keybuf);
	for (int i = 0; i < INPUT_QUEUE_SIZE; i++)
		input_queue[i].linecnt = input_queue[i].nextlinecnt = -1;

	for (int i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		sublevdir[0][i] = i;
		sublevdir[1][i] = MAX_INPUT_SUB_EVENT - i - 1;
	}
	for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
		custom_autoswitch_joy[i] = 0;
		custom_autoswitch_mouse[i] = 0;
	}
}

void inputdevice_copyjports(struct uae_prefs *srcprefs, struct uae_prefs *dstprefs)
{
	for (int i = 0; i < MAX_JPORTS; i++) {
		copyjport(srcprefs, dstprefs, i);
	}
	if (srcprefs) {
		for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
			_tcscpy(dstprefs->jports_custom[i].custom, srcprefs->jports_custom[i].custom);
		}
	}
}

void inputdevice_updateconfig_internal (struct uae_prefs *srcprefs, struct uae_prefs *dstprefs)
{
	keyboard_default = keyboard_default_table[currprefs.input_keyboard_type];

	inputdevice_copyjports(srcprefs, dstprefs);
	resetinput ();

	joysticks = dstprefs->joystick_settings[dstprefs->input_selected_setting];
	mice = dstprefs->mouse_settings[dstprefs->input_selected_setting];
	keyboards = dstprefs->keyboard_settings[dstprefs->input_selected_setting];
	internalevents = dstprefs->internalevent_settings[dstprefs->input_selected_setting];

	matchdevices_all (dstprefs);

	memset (joysticks2, 0, sizeof joysticks2);
	memset (mice2, 0, sizeof mice2);

	int input_selected_setting = dstprefs->input_selected_setting;

	joysticks = dstprefs->joystick_settings[GAMEPORT_INPUT_SETTINGS];
	mice = dstprefs->mouse_settings[GAMEPORT_INPUT_SETTINGS];
	keyboards = dstprefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS];
	internalevents = dstprefs->internalevent_settings[GAMEPORT_INPUT_SETTINGS];
	dstprefs->input_selected_setting = GAMEPORT_INPUT_SETTINGS;

	for (int i = 0; i < MAX_INPUT_SETTINGS; i++) {
		joysticks[i].enabled = 0;
		mice[i].enabled = 0;
	}

	for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
		inputdevice_parse_jport_custom(dstprefs, i, -1, NULL);
	}

	compatibility_copy (dstprefs, true);

	dstprefs->input_selected_setting = input_selected_setting;

	joysticks = dstprefs->joystick_settings[dstprefs->input_selected_setting];
	mice = dstprefs->mouse_settings[dstprefs->input_selected_setting];
	keyboards = dstprefs->keyboard_settings[dstprefs->input_selected_setting];
	internalevents = dstprefs->internalevent_settings[dstprefs->input_selected_setting];

	if (dstprefs->input_selected_setting != GAMEPORT_INPUT_SETTINGS) {
		compatibility_copy (dstprefs, false);
	}

	disableifempty (dstprefs);
	scanevents (dstprefs);

	if (srcprefs) {
		for (int i = 0; i < MAX_JPORTS; i++) {
			copyjport(dstprefs, srcprefs, i);
		}
	}
}

void inputdevice_updateconfig (struct uae_prefs *srcprefs, struct uae_prefs *dstprefs)
{
	inputdevice_updateconfig_internal (srcprefs, dstprefs);
	
	set_config_changed ();

	for (int i = 0; i < MAX_JPORTS; i++) {
		inputdevice_store_used_device(&dstprefs->jports[i], i, false);
	}

#ifdef RETROPLATFORM
	rp_input_change (0);
	rp_input_change (1);
	rp_input_change (2);
	rp_input_change (3);
	for (int i = 0; i < MAX_JPORTS; i++)
		rp_update_gameport (i, -1, 0);
#endif
}

/* called when devices get inserted or removed
* store old devices temporarily, enumerate all devices
* restore old devices back (order may have changed)
*/
bool inputdevice_devicechange (struct uae_prefs *prefs)
{
	int acc = input_acquired;
	int idx;
	TCHAR *jports_name[MAX_JPORTS];
	TCHAR *jports_configname[MAX_JPORTS];
	int jportskb[MAX_JPORTS], jportscustom[MAX_JPORTS];
	int jportsmode[MAX_JPORTS];
	int jportssubmode[MAX_JPORTS];
	int jportid[MAX_JPORTS], jportaf[MAX_JPORTS];
	bool changed = false;

	for (i = 0; i < MAX_JPORTS; i++) {
		jportskb[i] = -1;
		jportscustom[i] = -1;
		jportid[i] = prefs->jports[i].id;
		jportaf[i] = prefs->jports[i].autofire;
		jports_name[i] = NULL;
		jports_configname[i] = NULL;
		idx = inputdevice_getjoyportdevice (i, prefs->jports[i].id);
		if (idx >= JSEM_LASTKBD + inputdevice_get_device_total(IDTYPE_MOUSE) + inputdevice_get_device_total(IDTYPE_JOYSTICK)) {
			jportscustom[i] = idx - (JSEM_LASTKBD + inputdevice_get_device_total(IDTYPE_MOUSE) + inputdevice_get_device_total(IDTYPE_JOYSTICK));
		} else if (idx >= JSEM_LASTKBD) {
			if (prefs->jports[i].idc.name[0] == 0 && prefs->jports[i].idc.configname[0] == 0) {
				struct inputdevice_functions *idf;
				int devidx;
				idx -= JSEM_LASTKBD;
				idf = getidf (idx);
				devidx = inputdevice_get_device_index (idx);
				jports_name[i] = my_strdup (idf->get_friendlyname (devidx));
				jports_configname[i] = my_strdup (idf->get_uniquename (devidx));
			}
		} else {
			jportskb[i] = idx;
		}
		jportsmode[i] = prefs->jports[i].mode;
		jportssubmode[i] = prefs->jports[i].submode;
		if (jports_name[i] == NULL)
			jports_name[i] = my_strdup(prefs->jports[i].idc.name);
		if (jports_configname[i] == NULL)
			jports_configname[i] = my_strdup(prefs->jports[i].idc.configname);
	}

	// store old devices
	struct inputdevconfig devcfg[MAX_INPUT_DEVICES][IDTYPE_MAX] = { 0 };
	int dev_nums[IDTYPE_MAX];
	for (int j = 0; j <= IDTYPE_KEYBOARD; j++) {
		struct inputdevice_functions *inf = &idev[j];
		dev_nums[j] = inf->get_num();
		for (int i = 0; i < dev_nums[j]; i++) {
			const TCHAR *un = inf->get_uniquename(i);
			const TCHAR *fn = inf->get_friendlyname(i);
			_tcscpy(devcfg[i][j].name, fn);
			_tcscpy(devcfg[i][j].configname, un);
		}
	}

	inputdevice_unacquire();
	idev[IDTYPE_JOYSTICK].close();
	idev[IDTYPE_MOUSE].close();
	idev[IDTYPE_KEYBOARD].close();
	idev[IDTYPE_JOYSTICK].init();
	idev[IDTYPE_MOUSE].init();
	idev[IDTYPE_KEYBOARD].init();
	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		for (int j = 0; j < IDTYPE_MAX; j++) {
			gp_swappeddevices[i][j] = -1;
		}
	}

	matchdevices_all(prefs);

	//matchdevices(prefs, &idev[IDTYPE_MOUSE], prefs->mouse_settings, IDTYPE_MOUSE, prefs->input_device_match_mask);
	//matchdevices(prefs, &idev[IDTYPE_JOYSTICK], prefs->joystick_settings, IDTYPE_JOYSTICK, prefs->input_device_match_mask);
	//matchdevices(prefs, &idev[IDTYPE_KEYBOARD], prefs->keyboard_settings, IDTYPE_KEYBOARD, INPUT_MATCH_ALL);

	write_log(_T("Checking for inserted/removed devices..\n"));

	// find out which one was removed or inserted
	for (int j = 0; j <= IDTYPE_KEYBOARD; j++) {
		struct inputdevice_functions *inf = &idev[j];
		int num = inf->get_num();
		bool df[MAX_INPUT_DEVICES] = { 0 };
		for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
			TCHAR *fn2 = devcfg[i][j].name;
			TCHAR *un2 = devcfg[i][j].configname;
			if (fn2[0] && un2[0]) {
				for (int k = 0; k < num; k++) {
					const TCHAR *un = inf->get_uniquename(k);
					const TCHAR *fn = inf->get_friendlyname(k);
					// device not removed or inserted
					if (!_tcscmp(fn2, fn) && !_tcscmp(un2, un)) {
						devcfg[i][j].name[0] = 0;
						devcfg[i][j].configname[0] = 0;
						df[k] = true;
					}
				}
			}
		}
		for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
			if (devcfg[i][j].name[0]) {
				write_log(_T("REMOVED: %d/%d %s (%s)\n"), j, i, devcfg[i][j].name, devcfg[i][j].configname);
				inputdevice_store_unplugged_port(prefs, &devcfg[i][j]);
				changed = true;
			}
		}
		for (int i = 0; i < num; i++) {
			if (df[i] == false) {
				struct inputdevconfig idc;
				struct stored_jport *sjp;
				_tcscpy(idc.configname, inf->get_uniquename(i));
				_tcscpy(idc.name, inf->get_friendlyname(i));
				write_log(_T("INSERTED: %d/%d %s (%s)\n"), j, i, idc.name, idc.configname);
				changed = true;
				int portnum = inputdevice_get_unplugged_device(&idc, &sjp);
				if (portnum >= 0) {
					write_log(_T("Inserting to port %d\n"), portnum);
					jportscustom[i] = -1;
					xfree(jports_name[portnum]);
					xfree(jports_configname[portnum]);
					jports_name[portnum] = my_strdup(idc.name);
					jports_configname[portnum] = my_strdup(idc.configname);
					jportaf[portnum] = sjp->jp.autofire;
					jportsmode[portnum] = sjp->jp.mode;
					jportssubmode[portnum] = sjp->jp.submode;
				} else {
					write_log(_T("Not inserted to any port\n"));
				}
			}
		}
	}

	bool fixedports[MAX_JPORTS];
	for (i = 0; i < MAX_JPORTS; i++) {
		freejport(prefs, i);
		fixedports[i] = false;
	}
	for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
		inputdevice_jportcustom_fixup(prefs, prefs->jports_custom[i].custom, i);
	}

	for (int i = 0; i < MAX_JPORTS; i++) {
		bool found = true;
		if (jportscustom[i] >= 0) {
			TCHAR tmp[10];
			_sntprintf(tmp, sizeof tmp, _T("custom%d"), jportscustom[i]);
			found = inputdevice_joyport_config(prefs, tmp, NULL, i, jportsmode[i], jportssubmode[i], 0, true) != 0;
		} else if ((jports_name[i] && jports_name[i][0]) || (jports_configname[i] && jports_configname[i][0])) {
			if (!inputdevice_joyport_config (prefs, jports_name[i], jports_configname[i], i, jportsmode[i], jportssubmode[i], 1, true)) {
				found = inputdevice_joyport_config (prefs, jports_name[i], NULL, i, jportsmode[i], jportssubmode[i], 1, true) != 0;
			}
			if (!found) {
				inputdevice_joyport_config(prefs, _T("joydefault"), NULL, i, jportsmode[i], jportssubmode[i], 0, true);
			}
		} else if (jportskb[i] >= 0) {
			TCHAR tmp[10];
			_sntprintf (tmp, sizeof tmp, _T("kbd%d"), jportskb[i] + 1);
			found = inputdevice_joyport_config (prefs, tmp, NULL, i, jportsmode[i], jportssubmode[i], 0, true) != 0;
			
		}
		fixedports[i] = found;
		prefs->jports[i].autofire = jportaf[i];
		xfree (jports_name[i]);
		xfree (jports_configname[i]);
		inputdevice_validate_jports(prefs, i, fixedports);
	}

	write_log(_T("Input remapping done. Changed=%d.\n"), changed);

	if (!changed)
		return false;

	if (prefs == &changed_prefs)
		inputdevice_copyconfig (&changed_prefs, &currprefs);
	if (acc)
		inputdevice_acquire (TRUE);
#ifdef RETROPLATFORM
	rp_enumdevices ();
#endif
	set_config_changed ();
	return true;
}


// set default prefs to all input configuration settings
void inputdevice_default_prefs (struct uae_prefs *p)
{
	inputdevice_init ();

	p->input_selected_setting = GAMEPORT_INPUT_SETTINGS;
	p->input_joymouse_multiplier = 100;
	p->input_joymouse_deadzone = 33;
	p->input_joystick_deadzone = 33;
	p->input_joymouse_speed = 10;
	p->input_analog_joystick_mult = 18;
	p->input_analog_joystick_offset = -5;
#ifdef AMIBERRY
	p->input_mouse_speed = amiberry_options.input_default_mouse_speed;
#endif
	p->input_autofire_linecnt = 600;
	p->input_keyboard_type = 0;
	p->input_autoswitch = true;
	p->input_autoswitchleftright = false;
	p->input_device_match_mask = -1;
	keyboard_default = keyboard_default_table[p->input_keyboard_type];
	p->input_default_onscreen_keyboard = true;
	inputdevice_default_kb_all (p);

}

// set default keyboard and keyboard>joystick layouts
void inputdevice_setkeytranslation (struct uae_input_device_kbr_default **trans, int **kbmaps)
{
	keyboard_default_table = trans;
	keyboard_default_kbmaps = kbmaps;
}

// return true if keyboard/scancode pair is mapped
int inputdevice_iskeymapped (int keyboard, int scancode)
{
	if (!keyboards || scancode < 0)
		return 0;
	return scancodeused[keyboard][scancode];
}

int inputdevice_synccapslock (int oldcaps, int *capstable)
{
	struct uae_input_device *na = &keyboards[0];
	int j, i;
	
	if (!keyboards)
		return -1;
	for (j = 0; na->extra[j]; j++) {
		if (na->extra[j] == INPUTEVENT_KEY_CAPS_LOCK) {
			for (i = 0; capstable[i]; i += 2) {
				if (na->extra[j] == capstable[i]) {
					if (oldcaps != capstable[i + 1]) {
						oldcaps = capstable[i + 1];
						inputdevice_translatekeycode (0, capstable[i], oldcaps ? -1 : 0, false);
					}
					return i;
				}
			}
		}
	}
	return -1;
}

static void rqualifiers (uae_u64 flags, bool release)
{
	uae_u64 mask = ID_FLAG_QUALIFIER1 << 1;
	for (int i = 0; i < MAX_INPUT_QUALIFIERS; i++) {
		if ((flags & mask) && (mask & (qualifiers << 1))) {
			if (release) {
				if (!(mask & qualifiers_r)) {
					qualifiers_r |= mask;
					for (int ii = 0; ii < MAX_INPUT_SUB_EVENT; ii++) {
						int qevt = qualifiers_evt[i][ii];
						if (qevt > 0) {
							write_log (_T("Released %d '%s'\n"), qevt, events[qevt].name);
							inputdevice_do_keyboard (events[qevt].data, 0);
						}
					}
				}
			} else {
				if ((mask & qualifiers_r)) {
					qualifiers_r &= ~mask;
					for (int ii = 0; ii < MAX_INPUT_SUB_EVENT; ii++) {
						int qevt = qualifiers_evt[i][ii];
						if (qevt > 0) {
							write_log (_T("Pressed %d '%s'\n"), qevt, events[qevt].name);
							inputdevice_do_keyboard (events[qevt].data, 1);
						}
					}
				}
			}
		}
		mask <<= 2;
	}
}

static int inputdevice_translatekeycode_2 (int keyboard, int scancode, int keystate, bool qualifiercheckonly, bool ignorecanrelease)
{
	struct uae_input_device *na = &keyboards[keyboard];
	int j, k;
	int handled = 0;
	bool didcustom = false;

	if (!keyboards || scancode < 0)
		return handled;

	j = 0;
	while (j < MAX_INPUT_DEVICE_EVENTS && na->extra[j] >= 0) {
		if (na->extra[j] == scancode) {
			bool qualonly;
			uae_u64 qualmask[MAX_INPUT_SUB_EVENT];
			getqualmask (qualmask, na, j, &qualonly);

			if (qualonly)
				qualifiercheckonly = true;
			for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {/* send key release events in reverse order */
				uae_u64 *flagsp = &na->flags[j][sublevdir[keystate == 0 ? 1 : 0][k]];
				int evt = na->eventid[j][sublevdir[keystate == 0 ? 1 : 0][k]];
				uae_u64 flags = *flagsp;
				int autofire = (flags & ID_FLAG_AUTOFIRE) ? 1 : 0;
				int toggle = (flags & ID_FLAG_TOGGLE) ? 1 : 0;
				int inverttoggle = (flags & ID_FLAG_INVERTTOGGLE) ? 1 : 0;
				int invert = (flags & ID_FLAG_INVERT) ? 1 : 0;
				int setmode = (flags & ID_FLAG_SET_ONOFF) ? 1: 0;
				int setvalval = (flags & (ID_FLAG_SET_ONOFF_VAL1 | ID_FLAG_SET_ONOFF_VAL2));
				int setval = setvalval == (ID_FLAG_SET_ONOFF_VAL1 | ID_FLAG_SET_ONOFF_VAL2) ? SET_ONOFF_PRESSREL_VALUE :
					(setvalval == ID_FLAG_SET_ONOFF_VAL2 ? SET_ONOFF_PRESS_VALUE : (setvalval == ID_FLAG_SET_ONOFF_VAL1 ? SET_ONOFF_ON_VALUE : SET_ONOFF_OFF_VALUE));
				int toggled;
				int state;
				int ie_flags = na->port[j][sublevdir[keystate == 0 ? 1 : 0][k]] == 0 ? HANDLE_IE_FLAG_ALLOWOPPOSITE : 0;

				if (keystate < 0) {
					state = keystate;
				} else if (invert) {
					state = keystate ? 0 : 1;
				} else {
					state = keystate;
				}
				if (setmode) {
					if (state || setval == SET_ONOFF_PRESS_VALUE || setval == SET_ONOFF_PRESSREL_VALUE)
						state = setval | (keystate ? 1 : 0);
				}

				setqualifiers (evt, state > 0);

				if (qualifiercheckonly) {
					if (!state && (flags & ID_FLAG_CANRELEASE)) {
						*flagsp &= ~ID_FLAG_CANRELEASE;
						handle_input_event (evt, state, 1, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
						if (k == 0) {
							process_custom_event (na, j, state, qualmask, autofire, k);
						}
					}
					continue;
				}

				if (!state) {
					didcustom |= process_custom_event (na, j, state, qualmask, autofire, k);
				}

				// if evt == caps and scan == caps: sync with native caps led
				if (evt == INPUTEVENT_KEY_CAPS_LOCK) {
					int v;
					if (state < 0)
						state = 1;
					v = target_checkcapslock (scancode, &state);
					if (v < 0)
						continue;
					if (v > 0)
						toggle = 0;
				} else if (state < 0) {
					// it was caps lock resync, ignore, not mapped to caps
					continue;
				}

				if (inverttoggle) {
					na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] &= ~ID_FLAG_TOGGLED;
					if (state) {
						queue_input_event (evt, NULL, -1, 0, 0, 1);
						handled |= handle_input_event (evt, 2, 1, HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
					} else {
						handled |= handle_input_event (evt, 2, 1, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
					}
					didcustom |= process_custom_event (na, j, state, qualmask, autofire, k);
				} else if (toggle) {
					if (!state)
						continue;
					if (!checkqualifiers (evt, flags, qualmask, na->eventid[j]))
						continue;
					*flagsp ^= ID_FLAG_TOGGLED;
					toggled = (*flagsp & ID_FLAG_TOGGLED) ? 2 : 0;
					handled |= handle_input_event (evt, toggled, 1, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
					if (k == 0) {
						didcustom |= process_custom_event (na, j, state, qualmask, autofire, k);
					}
				} else {
					rqualifiers (flags, state ? true : false);
					if (!checkqualifiers (evt, flags, qualmask, na->eventid[j])) {
						if (!state && !(flags & ID_FLAG_CANRELEASE)) {
							if (!invert)
								continue;
						} else if (state) {
							continue;
						}
					}

					if (state) {
						if (!invert)
							*flagsp |= ID_FLAG_CANRELEASE;
					} else {
						if (ignorecanrelease)
							flags |= ID_FLAG_CANRELEASE;
						if (!(flags & ID_FLAG_CANRELEASE) && !invert)
							continue;
						*flagsp &= ~ID_FLAG_CANRELEASE;
					}
					handled |= handle_input_event (evt, state, 1, (autofire ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK | ie_flags);
					didcustom |= process_custom_event (na, j, state, qualmask, autofire, k);
				}
			}
			if (!didcustom)
				queue_input_event (-1, NULL, -1, 0, 0, 1);
			return handled;
		}
		j++;
	}
	return handled;
}

#define IECODE_UP_PREFIX 0x80
#define RAW_STEALTH 0x68
#define STEALTHF_E0KEY 0x08
#define STEALTHF_UPSTROKE 0x04
#define STEALTHF_SPECIAL 0x02
#define STEALTHF_E1KEY 0x01

static void sendmmcodes (int code, int newstate)
{
	uae_u8 b;

	b = RAW_STEALTH | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff, true);
	b = IECODE_UP_PREFIX;
	if ((code >> 8) == 0x01)
		b |= STEALTHF_E0KEY;
	if ((code >> 8) == 0x02)
		b |= STEALTHF_E1KEY;
	if (!newstate)
		b |= STEALTHF_UPSTROKE;
	record_key(((b << 1) | (b >> 7)) & 0xff, true);
	b = ((code >> 4) & 0x0f) | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff, true);
	b = (code & 0x0f) | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff, true);
}

// main keyboard press/release entry point
int inputdevice_translatekeycode (int keyboard, int scancode, int state, bool alwaysrelease)
{
	// if not default keyboard and all events are empty: use default keyboard
	if (!input_get_default_keyboard(keyboard) && isemptykey(keyboard, scancode)) {
		keyboard = input_get_default_keyboard(-1);
	}
	if (inputdevice_translatekeycode_2 (keyboard, scancode, state, false, alwaysrelease))
		return 1;
	if (currprefs.mmkeyboard && scancode > 0) {
		sendmmcodes (scancode, state);
		return 1;
	}
	return 0;
}
void inputdevice_checkqualifierkeycode (int keyboard, int scancode, int state)
{
	inputdevice_translatekeycode_2 (keyboard, scancode, state, true, false);
}

static const TCHAR *internaleventlabels[] = {
	_T("CPU reset"),
	_T("Keyboard reset"),
	NULL
};
static int init_int (void)
{
	return 1;
}
static void close_int (void)
{
}
static int acquire_int (int num, int flags)
{
	return 1;
}
static void unacquire_int (int num)
{
}
static void read_int (void)
{
}
static int get_int_num (void)
{
	return 1;
}
static const TCHAR *get_int_friendlyname (int num)
{
	return my_strdup(_T("Internal events"));
}
static const TCHAR *get_int_uniquename (int num)
{
	return my_strdup(_T("INTERNALEVENTS1"));
}
static int get_int_widget_num (int num)
{
	int i;
	for (i = 0; internaleventlabels[i]; i++);
	return i;
}
static int get_int_widget_type (int kb, int num, TCHAR *name, uae_u32 *code)
{
	if (code)
		*code = num;
	if (name)
		_tcscpy (name, internaleventlabels[num]);
	return IDEV_WIDGET_BUTTON;
}
static int get_int_widget_first (int kb, int type)
{
	return 0;
}
static int get_int_flags (int num)
{
	return 0;
}
static struct inputdevice_functions inputdevicefunc_internalevent = {
	init_int, close_int, acquire_int, unacquire_int, read_int,
	get_int_num, get_int_friendlyname, get_int_uniquename,
	get_int_widget_num, get_int_widget_type,
	get_int_widget_first,
	get_int_flags
};

void send_internalevent (int eventid)
{
	setbuttonstateall (&internalevents[0], NULL, eventid, -1);
}


void inputdevice_init (void)
{
	idev[IDTYPE_JOYSTICK] = inputdevicefunc_joystick;
	idev[IDTYPE_JOYSTICK].init ();
	idev[IDTYPE_MOUSE] = inputdevicefunc_mouse;
	idev[IDTYPE_MOUSE].init ();
	idev[IDTYPE_KEYBOARD] = inputdevicefunc_keyboard;
	idev[IDTYPE_KEYBOARD].init ();
	idev[IDTYPE_INTERNALEVENT] = inputdevicefunc_internalevent;
	idev[IDTYPE_INTERNALEVENT].init ();
}

void inputdevice_close (void)
{
	idev[IDTYPE_JOYSTICK].close ();
	idev[IDTYPE_MOUSE].close ();
	idev[IDTYPE_KEYBOARD].close ();
	idev[IDTYPE_INTERNALEVENT].close ();
	inprec_close (true);
}

static struct uae_input_device *get_uid (const struct inputdevice_functions *id, int devnum)
{
	struct uae_input_device *uid = 0;
	if (id == &idev[IDTYPE_JOYSTICK]) {
		uid = &joysticks[devnum];
	} else if (id == &idev[IDTYPE_MOUSE]) {
		uid = &mice[devnum];
	} else if (id == &idev[IDTYPE_KEYBOARD]) {
		uid = &keyboards[devnum];
	} else if (id == &idev[IDTYPE_INTERNALEVENT]) {
		uid = &internalevents[devnum];
	}
	return uid;
}

static int get_event_data (const struct inputdevice_functions *id, int devnum, int num, int *eventid, TCHAR **custom, uae_u64 *flags, int *port, int sub)
{
	const struct uae_input_device *uid = get_uid (id, devnum);
	int type = id->get_widget_type (devnum, num, 0, 0);
	int i;
	if (type == IDEV_WIDGET_BUTTON || type == IDEV_WIDGET_BUTTONAXIS) {
		i = num - id->get_widget_first (devnum, IDEV_WIDGET_BUTTON) + ID_BUTTON_OFFSET;
		*eventid = uid->eventid[i][sub];
		if (flags)
			*flags = uid->flags[i][sub];
		if (port)
			*port = uid->port[i][sub];
		if (custom)
			*custom = uid->custom[i][sub];
		return i;
	} else if (type == IDEV_WIDGET_AXIS) {
		i = num - id->get_widget_first (devnum, type) + ID_AXIS_OFFSET;
		*eventid = uid->eventid[i][sub];
		if (flags)
			*flags = uid->flags[i][sub];
		if (port)
			*port = uid->port[i][sub];
		if (custom)
			*custom = uid->custom[i][sub];
		return i;
	} else if (type == IDEV_WIDGET_KEY) {
		i = num - id->get_widget_first (devnum, type);
		*eventid = uid->eventid[i][sub];
		if (flags)
			*flags = uid->flags[i][sub];
		if (port)
			*port = uid->port[i][sub];
		if (custom)
			*custom = uid->custom[i][sub];
		return i;
	}
	return -1;
}

static TCHAR *stripstrdup (const TCHAR *s)
{
	TCHAR *out = my_strdup (s);
	if (!out)
		return NULL;
	for (int i = 0; out[i]; i++) {
		if (out[i] < ' ')
			out[i] = ' ';
	}
	return out;
}

static int put_event_data (const struct inputdevice_functions *id, int devnum, int num, int eventid, TCHAR *custom, uae_u64 flags, int port, int sub)
{
	struct uae_input_device *uid = get_uid (id, devnum);
	int type = id->get_widget_type (devnum, num, 0, 0);
	int i, ret;

	for (i = 0; i < MAX_INPUT_QUALIFIERS; i++) {
		uae_u64 mask1 = ID_FLAG_QUALIFIER1 << (i * 2);
		uae_u64 mask2 = mask1 << 1;
		if ((flags & (mask1 | mask2)) == (mask1 | mask2))
			flags &= ~mask2;
	}
	if (custom && custom[0] == 0)
		custom = NULL;
	if (custom)
		eventid = 0;
	if (eventid <= 0 && !custom)
		flags = 0;

	ret = -1;
	if (type == IDEV_WIDGET_BUTTON || type == IDEV_WIDGET_BUTTONAXIS) {
		i = num - id->get_widget_first (devnum, IDEV_WIDGET_BUTTON) + ID_BUTTON_OFFSET;
		uid->eventid[i][sub] = eventid;
		uid->flags[i][sub] = flags;
		uid->port[i][sub] = port;
		xfree (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && custom[0] != '\0' ? stripstrdup (custom) : NULL;
		ret = i;
	} else if (type == IDEV_WIDGET_AXIS) {
		i = num - id->get_widget_first (devnum, type) + ID_AXIS_OFFSET;
		uid->eventid[i][sub] = eventid;
		uid->flags[i][sub] = flags;
		uid->port[i][sub] = port;
		xfree (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && custom[0] != '\0' ? stripstrdup (custom) : NULL;
		ret = i;
	} else if (type == IDEV_WIDGET_KEY) {
		i = num - id->get_widget_first (devnum, type);
		uid->eventid[i][sub] = eventid;
		uid->flags[i][sub] = flags;
		uid->port[i][sub] = port;
		xfree (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && custom[0] != '\0' ? stripstrdup (custom) : NULL;
		ret = i;
	}
	if (ret < 0)
		return -1;
	if (uid->custom[i][sub])
		uid->eventid[i][sub] = INPUTEVENT_SPC_CUSTOM_EVENT;
	return ret;
}

static int is_event_used (const struct inputdevice_functions *id, int devnum, int isnum, int isevent)
{
	struct uae_input_device *uid = get_uid (id, devnum);
	int num, evt, sub;

	for (num = 0; num < id->get_widget_num (devnum); num++) {
		for (sub = 0; sub < MAX_INPUT_SUB_EVENT; sub++) {
			if (get_event_data (id, devnum, num, &evt, NULL, NULL, NULL, sub) >= 0) {
				if (evt == isevent && isnum != num)
					return 1;
			}
		}
	}
	return 0;
}

// device based index from global device index
int inputdevice_get_device_index (int devnum)
{
	int jcnt = idev[IDTYPE_JOYSTICK].get_num ();
	int mcnt = idev[IDTYPE_MOUSE].get_num ();
	int kcnt = idev[IDTYPE_KEYBOARD].get_num ();

	if (devnum < jcnt)
		return devnum;
	else if (devnum < jcnt + mcnt)
		return devnum - jcnt;
	else if (devnum < jcnt + mcnt + kcnt)
		return devnum - (jcnt + mcnt);
	else if (devnum < jcnt + mcnt + kcnt + INTERNALEVENT_COUNT)
		return devnum - (jcnt + mcnt + kcnt);
	return -1;
}

/* returns number of devices of type "type" */
int inputdevice_get_device_total (int type)
{
	return idev[type].get_num ();
}
/* returns the name of device */
const TCHAR *inputdevice_get_device_name (int type, int devnum)
{
	return idev[type].get_friendlyname (devnum);
}
/* returns the name of device */
const TCHAR *inputdevice_get_device_name2 (int devnum)
{
	return getidf (devnum)->get_friendlyname (inputdevice_get_device_index (devnum));
}
/* returns machine readable name of device */
const TCHAR *inputdevice_get_device_unique_name (int type, int devnum)
{
	return idev[type].get_uniquename (devnum);
}
/* returns state (enabled/disabled) */
int inputdevice_get_device_status (int devnum)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	if (idf == NULL)
		return -1;
	struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	return uid->enabled != 0;
}

/* set state (enabled/disabled) */
void inputdevice_set_device_status (int devnum, int enabled)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	int num = inputdevice_get_device_index (devnum);
	struct uae_input_device *uid = get_uid (idf, num);
	if (enabled) { // disable incompatible devices ("super device" vs "raw device")
		for (int i = 0; i < idf->get_num (); i++) {
			if (idf->get_flags (i) != idf->get_flags (num)) {
				struct uae_input_device *uid2 = get_uid (idf, i);
				uid2->enabled = 0;
			}
		}
	}
	uid->enabled = enabled;
}

/* returns number of axis/buttons and keys from selected device */
int inputdevice_get_widget_num (int devnum)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	return idf->get_widget_num (inputdevice_get_device_index (devnum));
}

// return name of event, do not use ie->name directly
bool inputdevice_get_eventname (const struct inputevent *ie, TCHAR *out)
{
	if (!out)
		return false;
	_tcscpy (out, ie->name);
	return true;
}

int inputdevice_iterate (int devnum, int num, TCHAR *name, int *af)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	static int id_iterator;
	const struct inputevent *ie;
	int mask, data, type;
	uae_u64 flags;
	int devindex = inputdevice_get_device_index (devnum);

	*af = 0;
	*name = 0;
	for (;;) {
		ie = &events[++id_iterator];
		if (!ie->confname) {
			id_iterator = 0;
			return 0;
		}
		mask = 0;
		type = idf->get_widget_type (devindex, num, NULL, NULL);
		if (type == IDEV_WIDGET_BUTTON || type == IDEV_WIDGET_BUTTONAXIS) {
			if (idf == &idev[IDTYPE_JOYSTICK]) {
				mask |= AM_JOY_BUT;
			} else {
				mask |= AM_MOUSE_BUT;
			}
		} else if (type == IDEV_WIDGET_AXIS) {
			if (idf == &idev[IDTYPE_JOYSTICK]) {
				mask |= AM_JOY_AXIS;
			} else {
				mask |= AM_MOUSE_AXIS;
			}
		} else if (type == IDEV_WIDGET_KEY) {
			mask |= AM_K;
		}
		if (ie->allow_mask & AM_INFO) {
			const struct inputevent *ie2 = ie + 1;
			while (!(ie2->allow_mask & AM_INFO)) {
				if (is_event_used (idf, devindex, (int)(ie2 - ie), -1)) {
					ie2++;
					continue;
				}
				if (ie2->allow_mask & mask)
					break;
				ie2++;
			}
			if (!(ie2->allow_mask & AM_INFO))
				mask |= AM_INFO;
		}
		if (!(ie->allow_mask & mask))
			continue;
		get_event_data (idf, devindex, num, &data, NULL, &flags, NULL, 0);
		inputdevice_get_eventname (ie, name);
		*af = (flags & ID_FLAG_AUTOFIRE) ? 1 : 0;
		return 1;
	}
}

// return mapped event from devnum/num/sub
int inputdevice_get_mapping (int devnum, int num, uae_u64 *pflags, int *pport, TCHAR *name, TCHAR *custom, int sub)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	const struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	int port, data;
	uae_u64 flags = 0, flag;
	int devindex = inputdevice_get_device_index (devnum);
	TCHAR *customp = NULL;

	if (name)
		_tcscpy (name, _T("<none>"));
	if (custom)
		custom[0] = 0;
	if (pflags)
		*pflags = 0;
	if (pport)
		*pport = 0;
	if (uid == 0 || num < 0)
		return 0;
	if (get_event_data (idf, devindex, num, &data, &customp, &flag, &port, sub) < 0)
		return 0;
	if (customp && custom)
		_tcscpy (custom, customp);
	if (flag & ID_FLAG_AUTOFIRE)
		flags |= IDEV_MAPPED_AUTOFIRE_SET;
	if (flag & ID_FLAG_TOGGLE)
		flags |= IDEV_MAPPED_TOGGLE;
	if (flag & ID_FLAG_INVERTTOGGLE)
		flags |= IDEV_MAPPED_INVERTTOGGLE;
	if (flag & ID_FLAG_INVERT)
		flags |= IDEV_MAPPED_INVERT;
	if (flag & ID_FLAG_GAMEPORTSCUSTOM1)
		flags |= IDEV_MAPPED_GAMEPORTSCUSTOM1;
	if (flag & ID_FLAG_GAMEPORTSCUSTOM2)
		flags |= IDEV_MAPPED_GAMEPORTSCUSTOM2;
	if (flag & ID_FLAG_QUALIFIER_MASK)
		flags |= flag & ID_FLAG_QUALIFIER_MASK;
	if (flag & ID_FLAG_SET_ONOFF)
		flags |= IDEV_MAPPED_SET_ONOFF;
	if (flag & ID_FLAG_SET_ONOFF_VAL1)
		flags |= IDEV_MAPPED_SET_ONOFF_VAL1;
	if (flag & ID_FLAG_SET_ONOFF_VAL2)
		flags |= IDEV_MAPPED_SET_ONOFF_VAL2;
	if (pflags)
		*pflags = flags;
	if (pport)
		*pport = port;
	if (!data)
		return 0;
	if (events[data].allow_mask & AM_AF)
		flags |= IDEV_MAPPED_AUTOFIRE_POSSIBLE;
	if (pflags)
		*pflags = flags;
	inputdevice_get_eventname (&events[data], name);
	return data;
}

// set event name/custom/flags to devnum/num/sub
int inputdevice_set_mapping (int devnum, int num, const TCHAR *name, TCHAR *custom, uae_u64 flags, int port, int sub)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	const struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	int eid, data, portp, amask;
	uae_u64 flag;
	TCHAR ename[256];
	int devindex = inputdevice_get_device_index (devnum);
	TCHAR *customp = NULL;

	if (uid == 0 || num < 0)
		return 0;
	if (name) {
		eid = 1;
		while (events[eid].name) {
			inputdevice_get_eventname (&events[eid], ename);
			if (!_tcscmp(ename, name))
				break;
			eid++;
		}
		if (!events[eid].name)
			return 0;
		if (events[eid].allow_mask & AM_INFO)
			return 0;
	} else {
		eid = 0;
	}
	if (get_event_data (idf, devindex, num, &data, &customp, &flag, &portp, sub) < 0)
		return 0;
	if (data >= 0) {
		amask = events[eid].allow_mask;
		flag &= ~(ID_FLAG_AUTOFIRE_MASK | ID_FLAG_GAMEPORTSCUSTOM_MASK | IDEV_MAPPED_QUALIFIER_MASK | ID_FLAG_INVERT);
		if (amask & AM_AF) {
			flag |= (flags & IDEV_MAPPED_AUTOFIRE_SET) ? ID_FLAG_AUTOFIRE : 0;
			flag |= (flags & IDEV_MAPPED_TOGGLE) ? ID_FLAG_TOGGLE : 0;
			flag |= (flags & IDEV_MAPPED_INVERTTOGGLE) ? ID_FLAG_INVERTTOGGLE : 0;
		}
		flag |= (flags & IDEV_MAPPED_INVERT) ? ID_FLAG_INVERT : 0;
		flag |= (flags & IDEV_MAPPED_GAMEPORTSCUSTOM1) ? ID_FLAG_GAMEPORTSCUSTOM1 : 0;
		flag |= (flags & IDEV_MAPPED_GAMEPORTSCUSTOM2) ? ID_FLAG_GAMEPORTSCUSTOM2 : 0;
		flag |= flags & IDEV_MAPPED_QUALIFIER_MASK;
		flag &= ~(IDEV_MAPPED_SET_ONOFF | IDEV_MAPPED_SET_ONOFF_VAL1 | IDEV_MAPPED_SET_ONOFF_VAL2);
		if (amask & AM_SETTOGGLE) {
			flag |= (flags & IDEV_MAPPED_SET_ONOFF) ? ID_FLAG_SET_ONOFF : 0;
			flag |= (flags & IDEV_MAPPED_SET_ONOFF_VAL1) ? ID_FLAG_SET_ONOFF_VAL1 : 0;
			flag |= (flags & IDEV_MAPPED_SET_ONOFF_VAL2) ? ID_FLAG_SET_ONOFF_VAL2 : 0;
		}
		if (port >= 0)
			portp = port;
		put_event_data (idf, devindex, num, eid, custom, flag, portp, sub);
		return 1;
	}
	return 0;
}

int inputdevice_get_widget_type (int devnum, int num, TCHAR *name, bool inccode)
{
	uae_u32 code = 0;
	const struct inputdevice_functions *idf = getidf (devnum);
	int r = idf->get_widget_type (inputdevice_get_device_index (devnum), num, name, &code);
	if (r && inccode && &idev[IDTYPE_KEYBOARD] == idf) {
		TCHAR *p = name + _tcslen(name);
		if (_tcsncmp(name, _T("KEY_"), 4))
			_sntprintf(p, sizeof p, _T(" [0x%02X]"), code);
	}
	return r;
}

static int config_change;

void inputdevice_config_change (void)
{
	config_change = 1;
}

int inputdevice_config_change_test (void)
{
	int v = config_change;
	config_change = 0;
	return v;
}

static void copy_inputdevice_settings(const struct uae_input_device *src, struct uae_input_device *dst)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT_ALL; i++) {
			if (src->custom[l][i]) {
				dst->custom[l][i] = my_strdup(src->custom[l][i]);
			} else {
				dst->custom[l][i] = NULL;
			}
		}
	}
}

static void copy_inputdevice_settings_free(const struct uae_input_device *src, struct uae_input_device *dst)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT_ALL; i++) {
			if (dst->custom[l][i] == NULL && src->custom[l][i] == NULL)
				continue;
			// same string in both src and dest: separate them (fixme: this shouldn't happen!)
			if (dst->custom[l][i] == src->custom[l][i]) {
				dst->custom[l][i] = NULL;
			} else {
				if (dst->custom[l][i]) {
					xfree(dst->custom[l][i]);
					dst->custom[l][i] = NULL;
				}
			}
		}
	}
}

// copy configuration #src to configuration #dst
void inputdevice_copyconfig (struct uae_prefs *src, struct uae_prefs *dst)
{
	dst->input_selected_setting = src->input_selected_setting;
	dst->input_joymouse_multiplier = src->input_joymouse_multiplier;
	dst->input_joymouse_deadzone = src->input_joymouse_deadzone;
	dst->input_joystick_deadzone = src->input_joystick_deadzone;
	dst->input_joymouse_speed = src->input_joymouse_speed;
	dst->input_mouse_speed = src->input_mouse_speed;
	dst->input_autofire_linecnt = src->input_autofire_linecnt;
#ifdef AMIBERRY
	strcpy(dst->open_gui, src->open_gui);
	strcpy(dst->quit_amiberry, src->quit_amiberry);
	strcpy(dst->action_replay, src->action_replay);
	strcpy(dst->fullscreen_toggle, src->fullscreen_toggle);
	strcpy(dst->minimize, src->minimize);
	dst->use_retroarch_quit = src->use_retroarch_quit;
	dst->use_retroarch_menu = src->use_retroarch_menu;
	dst->use_retroarch_reset = src->use_retroarch_reset;
	dst->use_retroarch_vkbd = src->use_retroarch_vkbd;
#endif

	for (int i = 0; i < MAX_JPORTS; i++) {
		copyjport (src, dst, i);
	}
	for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
		_tcscpy(dst->jports_custom[i].custom, src->jports_custom[i].custom);
	}

	copy_inputdevice_prefs(src, dst);

	inputdevice_updateconfig (src, dst);
}

static void swapevent (struct uae_input_device *uid, int i, int j, int evt)
{
	uid->eventid[i][j] = evt;
	int port = uid->port[i][j];
	if (port == 1)
		port = 2;
	else if (port == 2)
		port = 1;
	else if (port == 3)
		port = 4;
	else if (port == 4)
		port = 3;
	uid->port[i][j] = port;
}

static void swapjoydevice (struct uae_input_device *uid, const int **swaps)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			bool found = false;
			for (int k = 0; k < 2 && !found; k++) {
				int evtnum;
				for (int kk = 0; (evtnum = swaps[k][kk]) >= 0 && !found; kk++) {
					if (uid->eventid[i][j] == evtnum) {
						swapevent (uid, i, j, swaps[1 - k][kk]);
						found = true;
					} else {
						for (int jj = 0; axistable[jj] >= 0; jj += 3) {
							if (evtnum == axistable[jj] || evtnum == axistable[jj + 1] || evtnum == axistable[jj + 2]) {
								for (int ii = 0; ii < 3; ii++) {
									if (uid->eventid[i][j] == axistable[jj + ii]) {
										int evtnum2 = swaps[1 - k][kk];
										for (int m = 0; axistable[m] >= 0; m += 3) {
											if (evtnum2 == axistable[m] || evtnum2 == axistable[m + 1] || evtnum2 == axistable[m + 2]) {
												swapevent (uid, i, j, axistable[m + ii]);												
												found = true;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

// swap gameports ports, remember to handle customized ports too
void inputdevice_swap_compa_ports (struct uae_prefs *prefs, int portswap)
{
#if 0
	if ((prefs->jports[portswap].id == JPORT_CUSTOM || prefs->jports[portswap + 1].id == JPORT_CUSTOM)) {
		const int *swaps[2];
		swaps[0] = rem_ports[portswap];
		swaps[1] = rem_ports[portswap + 1];
		for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
			swapjoydevice (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], swaps);
			swapjoydevice (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], swaps);
			swapjoydevice (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], swaps);
		}
	}
#endif
	struct jport tmp = { 0 };
	memcpy (&tmp, &prefs->jports[portswap], sizeof (struct jport));
	memcpy (&prefs->jports[portswap], &prefs->jports[portswap + 1], sizeof (struct jport));
	memcpy (&prefs->jports[portswap + 1], &tmp, sizeof (struct jport));

	inputdevice_updateconfig(NULL, prefs);
}

// swap device "devnum" ports 0<>1 and 2<>3
void inputdevice_swap_ports (struct uae_prefs *p, int devnum)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	int i, j, k, event, unit;
	const struct inputevent *ie, *ie2;

	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			event = uid->eventid[i][j];
			if (event <= 0)
				continue;
			ie = &events[event];
			if (ie->unit <= 0)
				continue;
			unit = ie->unit;
			k = 1;
			while (events[k].confname) {
				ie2 = &events[k];
				if (ie2->type == ie->type && ie2->data == ie->data && ie2->unit - 1 == ((ie->unit - 1) ^ 1) &&
					ie2->allow_mask == ie->allow_mask && uid->port[i][j] == 0) {
					uid->eventid[i][j] = k;
					break;
				}
				k++;
			}
		}
	}
}

//memcpy (p->joystick_settings[dst], p->joystick_settings[src], sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
static void copydev (struct uae_input_device *dst, struct uae_input_device *src, int selectedwidget)
{
	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		for (int j = 0; j < MAX_INPUT_DEVICE_EVENTS; j++) {
			if (j == selectedwidget || selectedwidget < 0) {
				for (int k = 0; k < MAX_INPUT_SUB_EVENT_ALL; k++) {
					xfree (dst[i].custom[j][k]);
				}
			}
		}
		if (selectedwidget < 0) {
			xfree (dst[i].configname);
			xfree (dst[i].name);
		}
	}
	if (selectedwidget < 0) {
		memcpy (dst, src, sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
	} else {
		int j = selectedwidget;
		for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
			for (int k = 0; k < MAX_INPUT_SUB_EVENT_ALL; k++) {
				dst[i].eventid[j][k] = src[i].eventid[j][k];
				dst[i].custom[j][k] = src[i].custom[j][k];
				dst[i].flags[j][k] = src[i].flags[j][k];
				dst[i].port[j][k] = src[i].port[j][k];
			}
			dst[i].extra[j] = src[i].extra[j];
		}
	}
	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		for (int j = 0; j < MAX_INPUT_DEVICE_EVENTS; j++) {
			if (j == selectedwidget || selectedwidget < 0) {
				for (int k = 0; k < MAX_INPUT_SUB_EVENT_ALL; k++) {
					if (dst[i].custom[0])
						dst[i].custom[j][k] = my_strdup (dst[i].custom[j][k]);
				}
			}
		}
		if (selectedwidget < 0) {
			dst[i].configname = my_strdup (dst[i].configname);
			dst[i].name = my_strdup (dst[i].name);
		}
	}
}

// copy whole configuration #x-slot to another
// +1 = default
// +2 = default (pc keyboard)
void inputdevice_copy_single_config (struct uae_prefs *p, int src, int dst, int devnum, int selectedwidget)
{
	if (selectedwidget >= 0) {
		if (devnum < 0)
			return;
		if (gettype (devnum) != IDTYPE_KEYBOARD)
			return;
	}
	if (src >= MAX_INPUT_SETTINGS) {
		if (gettype (devnum) == IDTYPE_KEYBOARD) {
			p->input_keyboard_type = src > MAX_INPUT_SETTINGS ? 1 : 0;
			keyboard_default = keyboard_default_table[p->input_keyboard_type];
			inputdevice_default_kb (p, dst);
		}
	}
	if (src == dst)
		return;
	if (src < MAX_INPUT_SETTINGS) {
		if (devnum < 0 || gettype (devnum) == IDTYPE_JOYSTICK)
			copydev (p->joystick_settings[dst], p->joystick_settings[src], selectedwidget);
		if (devnum < 0 || gettype (devnum) == IDTYPE_MOUSE)
			copydev (p->mouse_settings[dst], p->mouse_settings[src], selectedwidget);
		if (devnum < 0 || gettype (devnum) == IDTYPE_KEYBOARD)
			copydev (p->keyboard_settings[dst], p->keyboard_settings[src], selectedwidget);
	}
}

void inputdevice_releasebuttons(void)
{
	for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
		for (int j = 0; j < 32; j++) {
			uae_u32 mask = 1 << j;
			if (joysticks2[i].buttonmask & mask) {
				setbuttonstateall(&joysticks[i], &joysticks2[i], j, 0);
			}
			if (mice2[i].buttonmask & mask) {
				setbuttonstateall(&mice[i], &mice2[i], j, 0);
			}
		}
		mice2[i].buttonmask = 0;
		joysticks2[i].buttonmask = 0;
	}
}

void inputdevice_acquire (int allmode)
{
	int i;

	//write_log (_T("inputdevice_acquire\n"));

	for (i = 0; i < MAX_INPUT_DEVICES; i++)
		idev[IDTYPE_JOYSTICK].unacquire (i);
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
		idev[IDTYPE_MOUSE].unacquire (i);
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
		idev[IDTYPE_KEYBOARD].unacquire (i);

	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		if ((use_joysticks[i] && allmode >= 0) || (allmode && !idev[IDTYPE_JOYSTICK].get_flags (i)))
			idev[IDTYPE_JOYSTICK].acquire (i, 0);
	}
	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		if ((use_mice[i] && allmode >= 0) || (allmode && !idev[IDTYPE_MOUSE].get_flags (i)))
			idev[IDTYPE_MOUSE].acquire (i, allmode < 0);
	}
	// Always acquire first + enabled keyboards
	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		if (use_keyboards[i] || i == 0)
			idev[IDTYPE_KEYBOARD].acquire (i, allmode < 0);
	}

	if (input_acquired)
		return;

	idev[IDTYPE_JOYSTICK].acquire (-1, 0);
	idev[IDTYPE_MOUSE].acquire (-1, 0);
	idev[IDTYPE_KEYBOARD].acquire (-1, 0);

	target_inputdevice_acquire();

	//    if (!input_acquired)
	//	write_log (_T("input devices acquired (%s)\n"), allmode ? "all" : "selected only");
	input_acquired = 1;
}

void inputdevice_unacquire(int inputmask)
{
	if (!(inputmask & 4)) {
		for (int i = 0; i < MAX_INPUT_DEVICES; i++)
			idev[IDTYPE_JOYSTICK].unacquire(i);
	}
	if (!(inputmask & 2)) {
		for (int i = 0; i < MAX_INPUT_DEVICES; i++)
			idev[IDTYPE_MOUSE].unacquire(i);
	}
	if (!(inputmask & 1)) {
		for (int i = 0; i < MAX_INPUT_DEVICES; i++)
			idev[IDTYPE_KEYBOARD].unacquire(i);
	}

	if (!input_acquired)
		return;

	target_inputdevice_unacquire(false);

	input_acquired = 0;
	if (!(inputmask & 4))
		idev[IDTYPE_JOYSTICK].unacquire(-1);
	if (!(inputmask & 2))
		idev[IDTYPE_MOUSE].unacquire(-1);
	if (!(inputmask & 1))
		idev[IDTYPE_KEYBOARD].unacquire(-1);
}

void inputdevice_unacquire(void)
{
	inputdevice_unacquire(0);
}

static void inputdevice_testrecord_info(int type, int num, int wtype, int wnum, int state, int max)
{
#if 0
	TCHAR name[256];

	if (type == IDTYPE_KEYBOARD) {
		if (wnum >= 0x100) {
			wnum = 0x100 - wnum;
		} else {
			struct uae_input_device *na = &keyboards[num];
			int j = 0;
			while (j < MAX_INPUT_DEVICE_EVENTS && na->extra[j] >= 0) {
				if (na->extra[j] == wnum) {
					wnum = j;
					break;
				}
				j++;
			}
			if (j >= MAX_INPUT_DEVICE_EVENTS || na->extra[j] < 0)
				return;
		}
	}
	int devnum2 = getdevnum(type, num);
	int type2;
	if (wnum >= 0 && wnum < MAX_INPUT_DEVICE_EVENTS)
		type2 = idev[type].get_widget_first(num, wtype) + wnum;
	else
		type2 = wnum;
	int state2 = state;

	name[0] = 0;
	int type3 = inputdevice_get_widget_type(devnum2, type2, name, false);
	if (name[0]) {
		const struct inputdevice_functions *idf = getidf(devnum2);
		write_log(_T("%s %s\n"), idf->get_friendlyname(inputdevice_get_device_index(devnum2)), name);
	}
#endif
}

static void inputdevice_testrecord_test(int type, int num, int wtype, int wnum, int state, int max)
{
	if (wnum < 0) {
		testmode = -1;
		return;
	}
	if (testmode_count >= TESTMODE_MAX)
		return;
	if (type == IDTYPE_KEYBOARD) {
		if (wnum >= 0x100) {
			wnum = 0x100 - wnum;
		} else {
			struct uae_input_device *na = &keyboards[num];
			int j = 0;
			while (j < MAX_INPUT_DEVICE_EVENTS && na->extra[j] >= 0) {
				if (na->extra[j] == wnum) {
					wnum = j;
					break;
				}
				j++;
			}
			if (j >= MAX_INPUT_DEVICE_EVENTS || na->extra[j] < 0)
				return;
		}
	}
	// wait until previous event is released before accepting new ones
	for (int i = 0; i < TESTMODE_MAX; i++) {
		struct teststore *ts2 = &testmode_wait[i];
		if (ts2->testmode_num < 0)
			continue;
		if (ts2->testmode_num != num || ts2->testmode_type != type || ts2->testmode_wtype != wtype || ts2->testmode_wnum != wnum)
			continue;
		if (max <= 0) {
			if (state)
				continue;
		} else {
			if (state < -(max / 2) || state > (max / 2))
				continue;
		}
		ts2->testmode_num = -1;
	}
	if (max <= 0) {
		if (!state)
			return;
	} else {
		if (state >= -(max / 2) && state <= (max / 2))
			return;
	}

	//write_log (_T("%d %d %d %d %d/%d\n"), type, num, wtype, wnum, state, max);
	struct teststore *ts = &testmode_data[testmode_count];
	ts->testmode_type = type;
	ts->testmode_num = num;
	ts->testmode_wtype = wtype;
	ts->testmode_wnum = wnum;
	ts->testmode_state = state;
	ts->testmode_max = max;
	testmode_count++;
}

void inputdevice_testrecord(int type, int num, int wtype, int wnum, int state, int max)
{
	//write_log (_T("%d %d %d %d %d/%d\n"), type, num, wtype, wnum, state, max);
	if (testmode) {
		inputdevice_testrecord_test(type, num, wtype, wnum, state, max);
	} else if (key_specialpressed()) {
		inputdevice_testrecord_info(type, num, wtype, wnum, state, max);
	}
}

int inputdevice_istest (void)
{
	return testmode;
}
void inputdevice_settest (int set)
{
	testmode = set;
	testmode_count = 0;
	testmode_wait[0].testmode_num = -1;
	testmode_wait[1].testmode_num = -1;
}

int inputdevice_testread_count (void)
{
	inputdevice_read();
	if (testmode != 1) {
		testmode = 0;
		return -1;
	}
	return testmode_count;
}

int inputdevice_testread (int *devnum, int *wtype, int *state, bool doread)
{
	if (doread) {
		inputdevice_read();
		if (testmode != 1) {
			testmode = 0;
			return -1;
		}
	}
	if (testmode_count > 0) {
		testmode_count--;
		struct teststore *ts = &testmode_data[testmode_count];
		*devnum = getdevnum (ts->testmode_type, ts->testmode_num);
		if (ts->testmode_wnum >= 0 && ts->testmode_wnum < MAX_INPUT_DEVICE_EVENTS)
			*wtype = idev[ts->testmode_type].get_widget_first (ts->testmode_num, ts->testmode_wtype) + ts->testmode_wnum;
		else
			*wtype = ts->testmode_wnum;
		*state = ts->testmode_state;
		if (ts->testmode_state)
			memcpy (&testmode_wait[testmode_count], ts, sizeof (struct teststore));
		return 1;
	}
	return 0;
}

/* Call this function when host machine's joystick/joypad/etc button state changes
* This function translates button events to Amiga joybutton/joyaxis/keyboard events
*/

/* button states:
* state = -1 -> mouse wheel turned or similar (button without release)
* state = 1 -> button pressed
* state = 0 -> button released
*/

void setjoybuttonstate (int joy, int button, int state)
{
	if (testmode) {
		inputdevice_testrecord (IDTYPE_JOYSTICK, joy, IDEV_WIDGET_BUTTON, button, state, -1);
		if (state < 0)
			inputdevice_testrecord (IDTYPE_JOYSTICK, joy, IDEV_WIDGET_BUTTON, button, 0, -1);
		return;
	}
#if 0
	if (ignoreoldinput (joy)) {
		if (state)
			switchdevice (&joysticks[joy], button, 1);
		return;
	}
#endif
	setbuttonstateall (&joysticks[joy], &joysticks2[joy], button, state ? 1 : 0);
}

/* buttonmask = 1 = normal toggle button, 0 = mouse wheel turn or similar
*/
void setjoybuttonstateall (int joy, uae_u32 buttonbits, uae_u32 buttonmask)
{
	int i;

#if 0
	if (ignoreoldinput (joy))
		return;
#endif
	for (i = 0; i < ID_BUTTON_TOTAL; i++) {
		if (buttonmask & (1 << i))
			setbuttonstateall (&joysticks[joy], &joysticks2[joy], i, (buttonbits & (1 << i)) ? 1 : 0);
		else if (buttonbits & (1 << i))
			setbuttonstateall (&joysticks[joy], &joysticks2[joy], i, -1);
	}
}
/* mouse buttons (just like joystick buttons)
*/
void setmousebuttonstateall (int mouse, uae_u32 buttonbits, uae_u32 buttonmask)
{
	int i;
	uae_u32 obuttonmask = mice2[mouse].buttonmask;

	for (i = 0; i < ID_BUTTON_TOTAL; i++) {
		if (buttonmask & (1 << i))
			setbuttonstateall (&mice[mouse], &mice2[mouse], i, (buttonbits & (1 << i)) ? 1 : 0);
		else if (buttonbits & (1 << i))
			setbuttonstateall (&mice[mouse], &mice2[mouse], i, -1);
	}
	if (obuttonmask != mice2[mouse].buttonmask)
		mousehack_helper (mice2[mouse].buttonmask);
}

void setmousebuttonstate (int mouse, int button, int state)
{
	uae_u32 obuttonmask = mice2[mouse].buttonmask;
	if (testmode) {
		inputdevice_testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_BUTTON, button, state, -1);
		return;
	}
	setbuttonstateall (&mice[mouse], &mice2[mouse], button, state);
	if (obuttonmask != mice2[mouse].buttonmask)
		mousehack_helper (mice2[mouse].buttonmask);
}

/* same for joystick axis (analog or digital)
* (0 = center, -max = full left/top, max = full right/bottom)
*/
void setjoystickstate (int joy, int axis, int state, int max)
{
	struct uae_input_device *id = &joysticks[joy];
	struct uae_input_device2 *id2 = &joysticks2[joy];
	int deadzone = currprefs.input_joymouse_deadzone * max / 100;
	int i, v1, v2;

	if (testmode) {
		inputdevice_testrecord (IDTYPE_JOYSTICK, joy, IDEV_WIDGET_AXIS, axis, state, max);
		return;
	}
	v1 = state;
	v2 = id2->states[axis][MAX_INPUT_SUB_EVENT];

	if (v1 < deadzone && v1 > -deadzone)
		v1 = 0;
	if (v2 < deadzone && v2 > -deadzone)
		v2 = 0;

	//write_log (_T("%d:%d new=%d old=%d state=%d max=%d\n"), joy, axis, v1, v2, state, max);

	if (input_play && state) {
		if (v1 != v2)
			inprec_realtime ();
	}
	if (input_play)
		return;
	if (!joysticks[joy].enabled) {
#ifndef AMIBERRY
		if (currprefs.input_autoswitchleftright) {
			if (v1 > 0)
				v1 = 1;
			else if (v1 < 0)
				v1 = -1;
			if (v2 > 0)
				v2 = 1;
			else if (v2 < 0)
				v2 = -1;
			if (v1 && v1 != v2 && (axis == 0 || axis == 1)) {
				static int prevdir;
				static struct timeval tv1;
				struct timeval tv2;
				gettimeofday (&tv2, NULL);
				if ((uae_s64)tv2.tv_sec * 1000000 + tv2.tv_usec < (uae_s64)tv1.tv_sec * 1000000 + tv1.tv_usec + 500000 && prevdir == v1) {
					switchdevice (&joysticks[joy], v1 < 0 ? 0 : 1, false);
					tv1.tv_sec = 0;
					tv1.tv_usec = 0;
					prevdir = 0;
				} else {
					tv1.tv_sec = tv2.tv_sec;
					tv1.tv_usec = tv2.tv_usec;
					prevdir = v1;
				}
			}
		}
#endif
		return;
	}
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		uae_u64 flags = id->flags[ID_AXIS_OFFSET + axis][i];
		int state2 = v1;
		if (flags & ID_FLAG_INVERT)
			state2 = -state2;
		if (state2 != id2->states[axis][i]) {
			//write_log(_T("-> %d %d\n"), i, state2);
			handle_input_event (id->eventid[ID_AXIS_OFFSET + axis][i], state2, max, ((flags & ID_FLAG_AUTOFIRE) ? HANDLE_IE_FLAG_AUTOFIRE : 0) | HANDLE_IE_FLAG_CANSTOPPLAYBACK);
			id2->states[axis][i] = state2;
		}
	}
	id2->states[axis][MAX_INPUT_SUB_EVENT] = v1;
}
int getjoystickstate (int joy)
{
	if (testmode)
		return 1;
	return joysticks[joy].enabled;
}

void setmousestate (int mouse, int axis, int data, int isabs)
{
	int i, v, diff;
	int extraflags = 0, extrastate = 0;
	int *mouse_p, *oldm_p;
	float d;
	struct uae_input_device *id = &mice[mouse];
	static float fract[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];

#if OUTPUTDEBUG
	TCHAR xx1[256];
	_stprintf(xx1, _T("%p %d M=%d A=%d D=%d IA=%d\n"), GetCurrentProcess(), timeframes, mouse, axis, data, isabs);
	OutputDebugString(xx1);
#endif

	if (testmode) {
		inputdevice_testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_AXIS, axis, data, -1);
		// fake "release" event
		inputdevice_testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_AXIS, axis, 0, -1);
#if OUTPUTDEBUG
		OutputDebugString(_T("-> exit1\n"));
#endif
		return;
	}
	if (input_play) {
#if OUTPUTDEBUG
		OutputDebugString(_T("-> exit5\n"));
#endif
		return;
	}
	if (!mice[mouse].enabled) {
		if (isabs && currprefs.input_tablet > 0) {
			if (axis == 0)
				lastmx = data;
			else
				lastmy = data;
			if (axis)
				mousehack_helper (mice2[mouse].buttonmask);
		}
#if OUTPUTDEBUG
		OutputDebugString(_T("-> exit2\n"));
#endif
		return;
	}
	d = 0;
	mouse_p = &mouse_axis[mouse][axis];
	oldm_p = &oldm_axis[mouse][axis];
	if (!isabs) {
		// eat relative movements while in mousehack mode
		if (currprefs.input_tablet == TABLET_MOUSEHACK && mousehack_alive() && axis < 2) {
#if OUTPUTDEBUG
			OutputDebugString(_T("-> exit3\n"));
#endif
			return;
		}
		*oldm_p = *mouse_p;
		*mouse_p += data;
		d = (*mouse_p - *oldm_p) * currprefs.input_mouse_speed / 100.0f;
	} else {
		extraflags |= HANDLE_IE_FLAG_ABSOLUTE;
		extrastate = data;
		d = (float)(data - *oldm_p);
		*oldm_p = data;
		*mouse_p += (int)d;
		if (axis == 0) {
			lastmx = data;
		} else {
			lastmy = data;
		}
		if (axis)
			mousehack_helper (mice2[mouse].buttonmask);
		if (currprefs.input_tablet == TABLET_MOUSEHACK && mousehack_alive() && axis < 2) {
#if OUTPUTDEBUG
			OutputDebugString(_T("-> exit4\n"));
#endif
			return;
		}
	}

	*mouse_p = (*mouse_p) - (*oldm_p);
	*oldm_p = 0;

	v = (int)d;
	fract[mouse][axis] += d - v;
	diff = (int)fract[mouse][axis];
	v += diff;
	fract[mouse][axis] -= diff;
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		uae_u64 flags = id->flags[ID_AXIS_OFFSET + axis][i];
		if (!isabs && (flags & ID_FLAG_INVERT))
			v = -v;

#if OUTPUTDEBUG
		if (id->eventid[ID_AXIS_OFFSET + axis][i]) {
			TCHAR xx2[256];
			_stprintf(xx2, _T("%p %d -> %d ID=%d\n"), GetCurrentProcess(), timeframes, v, id->eventid[ID_AXIS_OFFSET + axis][i]);
			OutputDebugString(xx2);
		}
#endif

		handle_input_event_extra(id->eventid[ID_AXIS_OFFSET + axis][i], v, 0, HANDLE_IE_FLAG_CANSTOPPLAYBACK | extraflags, extrastate);
	}
#if OUTPUTDEBUG
	OutputDebugString(_T("-> exit0\n"));
#endif
}

int getmousestate (int joy)
{
	if (testmode)
		return 1;
	return mice[joy].enabled;
}

void bootwarpmode(void)
{
	if (currprefs.turbo_emulation == 2) {
		currprefs.turbo_emulation = changed_prefs.turbo_emulation = 2 + currprefs.turbo_boot_delay;
	}
}

void warpmode(int mode)
{
	if (mode < 0) {
		if (currprefs.turbo_emulation) {
			changed_prefs.gfx_framerate = currprefs.gfx_framerate = 1;
			currprefs.turbo_emulation = 0;
		} else {
			currprefs.turbo_emulation = 1;
		}
	} else if (mode == 0 && currprefs.turbo_emulation) {
		if (currprefs.turbo_emulation > 0)
			changed_prefs.gfx_framerate = currprefs.gfx_framerate = 1;
		currprefs.turbo_emulation = 0;
	} else if (mode > 0 && !currprefs.turbo_emulation) {
		currprefs.turbo_emulation = 1;
	}
	if (currprefs.turbo_emulation) {
		if (!currprefs.cpu_memory_cycle_exact && !currprefs.blitter_cycle_exact && currprefs.gfx_overscanmode < OVERSCANMODE_ULTRA)
			changed_prefs.gfx_framerate = currprefs.gfx_framerate = 10;
		pause_sound ();
	} else {
		resume_sound ();
	}
	compute_vsynctime ();
#ifdef RETROPLATFORM
	rp_turbo_cpu (currprefs.turbo_emulation);
#endif
	changed_prefs.turbo_emulation = currprefs.turbo_emulation;
	set_config_changed ();
	setsystime ();
}

void pausemode (int mode)
{
	if (mode < 0)
		pause_emulation = pause_emulation ? 0 : 9;
	else
		pause_emulation = mode;
	set_config_changed ();
	setsystime ();
}

int jsem_isjoy (int port, const struct uae_prefs *p)
{
	int v = JSEM_DECODEVAL(port, p);
	if (v < JSEM_JOYS)
		return -1;
	v -= JSEM_JOYS;
	if (v >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
		return -1;
	return v;
}

int jsem_ismouse (int port, const struct uae_prefs *p)
{
	int v = JSEM_DECODEVAL(port, p);
	if (v < JSEM_MICE)
		return -1;
	v -= JSEM_MICE;
	if (v >= inputdevice_get_device_total (IDTYPE_MOUSE))
		return -1;
	return v;
}

int jsem_iskbdjoy(int port, const struct uae_prefs *p)
{
	int v = JSEM_DECODEVAL(port, p);
	if (v < JSEM_KBDLAYOUT)
		return -1;
	v -= JSEM_KBDLAYOUT;
	if (v >= JSEM_LASTKBD)
		return -1;
	return v;
}

static bool fixjport(struct jport *port, int add, bool always)
{
	bool wasinvalid = false;
	int vv = port->id;
	if (vv == JPORT_NONE)
		return wasinvalid;
	if (vv >= JSEM_JOYS && vv < JSEM_MICE) {
		vv -= JSEM_JOYS;
		vv += add;
		if (vv >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
			vv = 0;
		vv += JSEM_JOYS;
	} else if (vv >= JSEM_MICE && vv < JSEM_END) {
		vv -= JSEM_MICE;
		vv += add;
		if (vv >= inputdevice_get_device_total (IDTYPE_MOUSE))
			vv = 0;
		vv += JSEM_MICE;
	} else if (vv >= JSEM_KBDLAYOUT && vv < JSEM_LASTKBD) {
		vv -= JSEM_KBDLAYOUT;
		vv += add;
		if (vv >= JSEM_LASTKBD)
			vv = 0;
		vv += JSEM_KBDLAYOUT;
	} else if (vv >= JSEM_CUSTOM && vv < JSEM_CUSTOM + MAX_JPORTS_CUSTOM) {
		vv -= JSEM_CUSTOM;
		vv += add;
		if (vv >= MAX_JPORTS_CUSTOM)
			vv = 0;
		vv += JSEM_CUSTOM;
	}
	if (port->id != vv || always) {
		port->idc.shortid[0] = 0;
		port->idc.configname[0] = 0;
		port->idc.name[0] = 0;
		if (vv >= JSEM_JOYS && vv < JSEM_MICE) {
			_tcsncpy(port->idc.name, inputdevice_get_device_name (IDTYPE_JOYSTICK, vv - JSEM_JOYS), MAX_JPORT_NAME - 1);
			_tcsncpy(port->idc.configname, inputdevice_get_device_unique_name (IDTYPE_JOYSTICK, vv - JSEM_JOYS), MAX_JPORT_CONFIG - 1);
		} else if (vv >= JSEM_MICE && vv < JSEM_END) {
			_tcsncpy(port->idc.name, inputdevice_get_device_name (IDTYPE_MOUSE, vv - JSEM_MICE), MAX_JPORT_NAME - 1);
			_tcsncpy(port->idc.configname, inputdevice_get_device_unique_name (IDTYPE_MOUSE, vv - JSEM_MICE), MAX_JPORT_CONFIG - 1);
		} else if (vv >= JSEM_KBDLAYOUT && vv < JSEM_CUSTOM) {
			_sntprintf(port->idc.shortid, sizeof port->idc.shortid, _T("kbd%d"), vv - JSEM_KBDLAYOUT + 1);
		} else if (vv >= JSEM_CUSTOM && vv < JSEM_JOYS) {
			_sntprintf(port->idc.shortid, sizeof port->idc.shortid, _T("custom%d"), vv - JSEM_CUSTOM);
		}
		port->idc.name[MAX_JPORT_NAME - 1] = 0;
		port->idc.configname[MAX_JPORT_CONFIG - 1] = 0;
		wasinvalid = true;
#if 0
		write_log(_T("fixjport %d %d %d (%s)\n"), port->id, vv, add, port->name);
#endif
	}
	port->id = vv;
	return wasinvalid;
}

static void inputdevice_get_previous_joy(struct uae_prefs *p, int portnum, bool userconfig)
{
	struct jport *jpx = &p->jports[portnum];

	if (!userconfig) {
		// default.uae with unplugged device -> NONE
		p->jports[portnum].id = JPORT_NONE;
		return;
	}
	bool found = false;
	int idx = 0;
	for (;;) {
		struct jport *jp = inputdevice_get_used_device(portnum, idx);
		if (!jp)
			break;
		if (jp->idc.configname[0]) {
			found = inputdevice_joyport_config(p, jp->idc.name, jp->idc.configname, portnum, jp->mode, jp->submode, 1, true) != 0;
			if (!found && jp->id == JPORT_UNPLUGGED)
				found = inputdevice_joyport_config(p, jp->idc.name, NULL, portnum, jp->mode, jp->submode, 1, true) != 0;
		} else if (jp->id < JSEM_JOYS && jp->id >= 0) {
			jpx->id = jp->id;
			found = true;
		}
		if (found) {
			jpx->mode = jp->mode;
			jpx->autofire = jp->autofire;
			inputdevice_set_newest_used_device(portnum, jp);
			break;
		}
		idx++;
	}
	if (!found) {
		if (default_keyboard_layout[portnum] > 0) {
			p->jports[portnum].id = default_keyboard_layout[portnum] - 1;
		} else {
			p->jports[portnum].id = JPORT_NONE;
		}
	}
}

void inputdevice_validate_jports (struct uae_prefs *p, int changedport, bool *fixedports)
{
	for (int i = 0; i < MAX_JPORTS; i++) {
		fixjport (&p->jports[i], 0, changedport == i);
	}

	for (int i = 0; i < MAX_JPORTS; i++) {
		if (p->jports[i].id < 0)
			continue;
		for (int j = 0; j < MAX_JPORTS; j++) {
			if (p->jports[j].id < 0)
				continue;
			if (j == i)
				continue;
			if (p->jports[i].id == p->jports[j].id) {
				int cnt = 0;
				for (;;) {
					int k;
					if (i == changedport) {
						k = j;
						if (fixedports && fixedports[k]) {
							k = i;
						}
					} else {
						k = i;
					}
					// same in other slots too?
					bool other = false;
					for (int l = 0; l < MAX_JPORTS; l++) {
						if (l == k)
							continue;
						if (p->jports[l].id == p->jports[k].id) {
							other = true;
						}
					}

					if (!other && p->jports[i].id != p->jports[j].id)
						break;

					struct jport *jp = NULL;
					for (;;) {
						jp = inputdevice_get_used_device(k, cnt);
						cnt++;
						if (!jp)
							break;
						if (jp->id < 0)
							continue;
						memcpy(&p->jports[k].id, jp, sizeof(struct jport));
						if (fixjport(&p->jports[k], 0, false))
							continue;
						inputdevice_set_newest_used_device(k, &p->jports[k]);
						break;
					}
					if (jp)
						continue;
					freejport(p, k);
					break;
				}
			}
		}
	}
}

void inputdevice_joyport_config_store(struct uae_prefs *p, const TCHAR *value, int portnum, int mode, int submode, int type)
{
	struct jport *jp = &p->jports[portnum];
	if (type == 2) {
		_tcscpy(jp->idc.name, value);
	} else if (type == 1) {
		_tcscpy(jp->idc.configname, value);
	} else {
		_tcscpy(jp->idc.shortid, value);
	}
	if (mode >= 0) {
		jp->mode = mode;
		jp->submode = submode;
	}
	jp->changed = true;
}

int inputdevice_joyport_config(struct uae_prefs *p, const TCHAR *value1, const TCHAR *value2, int portnum, int mode, int submode, int type, bool candefault)
{
	switch (type)
	{
	case 1: // check and set
	case 2: // check only
		{
			for (int j = 0; j < 2; j++) {
				int matched = -1;
				struct inputdevice_functions *idf;
				int dtype = IDTYPE_MOUSE;
				int idnum = JSEM_MICE;
				if (j > 0) {
					dtype = IDTYPE_JOYSTICK;
					idnum = JSEM_JOYS;
				}
				idf = &idev[dtype];
				if (value1 && value2 && (p->input_device_match_mask & INPUT_MATCH_BOTH)) {
					for (int i = 0; i < idf->get_num(); i++) {
						const TCHAR* name1 = idf->get_friendlyname(i);
						const TCHAR* name2 = idf->get_uniquename(i);
						if (name2 && !_tcscmp(name2, value2) && name1 && !_tcscmp(name1, value1)) {
							// config+friendlyname matched: don't bother to check for duplicates
							matched = i;
							break;
						}
					}
				}
				if (matched < 0 && value2 && (p->input_device_match_mask & INPUT_MATCH_CONFIG_NAME_ONLY)) {
					matched = -1;
					for (int i = 0; i < idf->get_num (); i++) {
						const TCHAR* name2 = idf->get_uniquename(i);
						if (name2 && !_tcscmp (name2, value2)) {
							if (matched >= 0) {
								matched = -2;
								break;
							} else {
								matched = i;
							}
						}
					}
				}
				if (matched < 0 && value1 && (p->input_device_match_mask & INPUT_MATCH_FRIENDLY_NAME_ONLY)) {
					matched = -1;
					for (int i = 0; i < idf->get_num (); i++) {
						const TCHAR* name1 = idf->get_friendlyname(i);
						if (name1 && !_tcscmp (name1, value1)) {
							if (matched >= 0) {
								matched = -2;
								break;
							} else {
								matched = i;
							}
						}
					}
				}
				if (matched >= 0) {
					if (type == 1) {
						if (value1)
							_tcscpy(p->jports[portnum].idc.name, value1);
						if (value2)
							_tcscpy(p->jports[portnum].idc.configname, value2);
						p->jports[portnum].id = idnum + matched;
						if (mode >= 0) {
							p->jports[portnum].mode = mode;
							p->jports[portnum].submode = submode;
						}
						set_config_changed ();
					}
					return 1;
				}
			}
			return 0;
		}
		break;
	case 0:
		{
			int start = JPORT_NONE, got = 0, max = -1;
			int type = -1;
			const TCHAR *pp = NULL;
			if (_tcsncmp (value1, _T("kbd"), 3) == 0) {
				start = JSEM_KBDLAYOUT;
				pp = value1 + 3;
				got = 1;
				max = JSEM_LASTKBD;
			} else if (_tcscmp(value1, _T("joydefault")) == 0) {
				type = IDTYPE_JOYSTICK;
				start = JSEM_JOYS;
				got = 1;
			} else if (_tcscmp(value1, _T("mousedefault")) == 0) {
				type = IDTYPE_MOUSE;
				start = JSEM_MICE;
				got = 1;
			} else if (_tcsncmp (value1, _T("joy"), 3) == 0) {
				type = IDTYPE_JOYSTICK;
				start = JSEM_JOYS;
				pp = value1 + 3;
				got = 1;
				max = idev[IDTYPE_JOYSTICK].get_num ();
			} else if (_tcsncmp (value1, _T("mouse"), 5) == 0) {
				type = IDTYPE_MOUSE;
				start = JSEM_MICE;
				pp = value1 + 5;
				got = 1;
				max = idev[IDTYPE_MOUSE].get_num ();
			} else if (_tcscmp(value1, _T("none")) == 0) {
				got = 2;
			} else if (_tcscmp (value1, _T("custom")) == 0) {
				// obsolete custom
				start = JSEM_CUSTOM + portnum;
				got = 3;
			} else if (_tcsncmp(value1, _T("custom"), 6) == 0) {
				// new custom
				start = JSEM_CUSTOM;
				pp = value1 + 6;
				got = 2;
				max = MAX_JPORTS_CUSTOM;
			}
			if (got) {
				if (pp && max != 0) {
					int v = _tstol (pp);
					if (start >= 0) {
						if (start == JSEM_KBDLAYOUT && v > 0)
							v--;
						if (v >= 0) {
							if (v >= max)
								v = 0;
							start += v;
							got = 2;
						}
					}
				}
				if (got >= 2) {
					p->jports[portnum].id = start;
					if (mode >= 0) {
						p->jports[portnum].mode = mode;
						p->jports[portnum].submode = submode;
					}
					if (start < JSEM_JOYS) {
						default_keyboard_layout[portnum] = start + 1;
					}
					if (got == 2 && candefault) {
						inputdevice_store_used_device(&p->jports[portnum], portnum, false);
					}
					if (got == 3) {
						// mark for old custom handling
						_tcscpy(p->jports_custom[portnum].custom, _T("#"));
					}
					set_config_changed ();
					return 1;
				}
				// joystick not found, select previously used or default
				if (start == JSEM_JOYS && p->jports[portnum].id < JSEM_JOYS) {
					inputdevice_get_previous_joy(p, portnum, true);
					set_config_changed ();
					return 1;
				}
			}
		}
		break;
	}
	return 0;
}

int inputdevice_getjoyportdevice (int port, int val)
{
	int idx;
	if (val < 0) {
		idx = -1;
	} else if (val >= JSEM_MICE) {
		idx = val - JSEM_MICE;
		if (idx >= inputdevice_get_device_total (IDTYPE_MOUSE))
			idx = 0;
		else
			idx += inputdevice_get_device_total (IDTYPE_JOYSTICK);
		idx += JSEM_LASTKBD;
	} else if (val >= JSEM_JOYS) {
		idx = val - JSEM_JOYS;
		if (idx >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
			idx = 0;
		idx += JSEM_LASTKBD;
	} else if (val >= JSEM_CUSTOM) {
		idx = val - JSEM_CUSTOM;
		if (idx >= MAX_JPORTS_CUSTOM)
			idx = 0;
		if (port >= 2) {
			idx += JSEM_LASTKBD + inputdevice_get_device_total(IDTYPE_JOYSTICK);
		} else {
			idx += JSEM_LASTKBD + inputdevice_get_device_total(IDTYPE_MOUSE) + inputdevice_get_device_total(IDTYPE_JOYSTICK);
		}
	} else {
		idx = val - JSEM_KBDLAYOUT;
	}
	return idx;
}

static struct jport jport_config_store[MAX_JPORTS];

void inputdevice_fix_prefs(struct uae_prefs *p, bool userconfig)
{
	bool changed = false;

	for (int i = 0; i < MAX_JPORTS; i++) {
		memcpy(&jport_config_store[i], &p->jports[i], sizeof(struct jport));
		changed |= p->jports[i].changed;
	}

	if (!changed)
		return;

	bool defaultports = userconfig == false;
	// Convert old style custom mapping to new style
	for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
		struct jport_custom *jpc = &p->jports_custom[i];
		if (!_tcscmp(jpc->custom, _T("#"))) {
			TCHAR tmp[16];
			_sntprintf(tmp, sizeof tmp, _T("custom%d"), i);
			inputdevice_joyport_config(p, tmp, NULL, i, -1, -1, 0, userconfig);
			inputdevice_generate_jport_custom(p, i);
		}
	}
	bool matched[MAX_JPORTS];
	// configname+friendlyname first
	for (int i = 0; i < MAX_JPORTS; i++) {
		struct jport *jp = &jport_config_store[i];
		matched[i] = false;
		if (jp->idc.configname[0] && jp->idc.name[0] && (p->input_device_match_mask & INPUT_MATCH_BOTH)) {
			if (inputdevice_joyport_config(p, jp->idc.name, jp->idc.configname, i, jp->mode, jp->submode, 1, userconfig)) {
				inputdevice_validate_jports(p, i, matched);
				inputdevice_store_used_device(&p->jports[i], i, defaultports);
				matched[i] = true;
				write_log(_T("Port%d: COMBO '%s' + '%s' matched\n"), i, jp->idc.name, jp->idc.configname);
			}
		}
	}
	// configname next
	for (int i = 0; i < MAX_JPORTS; i++) {
		if (!matched[i]) {
			struct jport *jp = &jport_config_store[i];
			if (jp->idc.configname[0] && (p->input_device_match_mask & INPUT_MATCH_CONFIG_NAME_ONLY)) {
				if (inputdevice_joyport_config(p, NULL, jp->idc.configname, i, jp->mode, jp->submode, 1, userconfig)) {
					inputdevice_validate_jports(p, i, matched);
					inputdevice_store_used_device(&p->jports[i], i, defaultports);
					matched[i] = true;
					write_log(_T("Port%d: CONFIG '%s' matched\n"), i, jp->idc.configname);
				}
			}
		}
	}
	// friendly name next
	for (int i = 0; i < MAX_JPORTS; i++) {
		if (!matched[i]) {
			struct jport *jp = &jport_config_store[i];
			if (jp->idc.name[0] && (p->input_device_match_mask & INPUT_MATCH_FRIENDLY_NAME_ONLY)) {
				if (inputdevice_joyport_config(p, jp->idc.name, NULL, i, jp->mode, jp->submode, 1, userconfig)) {
					inputdevice_validate_jports(p, i, matched);
					inputdevice_store_used_device(&p->jports[i], i, defaultports);
					matched[i] = true;
					write_log(_T("Port%d: NAME '%s' matched\n"), i, jp->idc.name);
				}
			}
		}
	}
	// joyportX last and only if no name/configname
	for (int i = 0; i < MAX_JPORTS; i++) {
		if (!matched[i]) {
			struct jport *jp = &jport_config_store[i];
			if (jp->idc.shortid[0] && !jp->idc.name[0] && !jp->idc.configname[0]) {
				if (inputdevice_joyport_config(p, jp->idc.shortid, NULL, i, jp->mode, jp->submode, 0, userconfig)) {
					inputdevice_validate_jports(p, i, matched);
					inputdevice_store_used_device(&p->jports[i], i, defaultports);
					matched[i] = true;
					write_log(_T("Port%d: ID '%s' matched\n"), i, jp->idc.shortid);
				}
			}
			if (!matched[i]) {
				if (jp->idc.configname[0] && jp->idc.name[0]) {
					struct jport jpt = { 0 };
					memcpy(&jpt.idc, &jp->idc, sizeof(struct inputdevconfig));
					jpt.id = JPORT_UNPLUGGED;
					jpt.mode = jp->mode;
					jpt.submode = jp->submode;
					jpt.autofire = jp->autofire;
					write_log(_T("Unplugged stored, port %d '%s' (%s) %d %d %d\n"), i, jp->idc.name, jp->idc.configname, jp->mode, jp->submode, jp->autofire);
					inputdevice_store_used_device(&jpt, i, defaultports);
					freejport(p, i);
					inputdevice_get_previous_joy(p, i, userconfig);
					matched[i] = true;
				}
			}
		}
	}
	for (int i = 0; i < MAX_JPORTS; i++) {
		if (!matched[i]) {
			struct jport *jp = &jport_config_store[i];
			freejport(p, i);
			if (jp->id != JPORT_NONE) {
				inputdevice_get_previous_joy(p, i, userconfig);
				write_log(_T("Port%d: ID=%d getting previous: %d\n"), i, jp->id, p->jports[i].id);
			} else {
				write_log(_T("Port%d: NONE\n"), i);
			}
		}
	}
}

// for state recorder use only!

uae_u8 *save_inputstate (size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	for (int i = 0; i < MAX_JPORTS; i++) {
		save_u16 (joydir[i]);
		save_u16 (joybutton[i]);
		save_u16 (otop[i]);
		save_u16 (obot[i]);
		save_u16 (oleft[i]);
		save_u16 (oright[i]);
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		save_u16 (cd32_shifter[i]);
		for (int j = 0; j < 2; j++) {
			save_u16 (pot_cap[i][j]);
			save_u16 (joydirpot[i][j]);
		}
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		for (int j = 0; j < MOUSE_AXIS_TOTAL; j++) {
			save_u16 (mouse_delta[i][j]);
			save_u16 (mouse_deltanoreset[i][j]);
		}
		save_u16 (mouse_frame_x[i]);
		save_u16 (mouse_frame_y[i]);
	}
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_inputstate (uae_u8 *src)
{
	for (int i = 0; i < MAX_JPORTS; i++) {
		joydir[i] = restore_u16 ();
		joybutton[i] = restore_u16 ();
		otop[i] = restore_u16 ();
		obot[i] = restore_u16 ();
		oleft[i] = restore_u16 ();
		oright[i] = restore_u16 ();
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		cd32_shifter[i] = restore_u16 ();
		for (int j = 0; j < 2; j++) {
			pot_cap[i][j] = restore_u16 ();
			joydirpot[i][j] = restore_u16 ();
		}
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		for (int j = 0; j < MOUSE_AXIS_TOTAL; j++) {
			mouse_delta[i][j] = restore_u16 ();
			mouse_deltanoreset[i][j] = restore_u16 ();
		}
		mouse_frame_x[i] = restore_u16 ();
		mouse_frame_y[i] = restore_u16 ();
	}
	return src;
}

void clear_inputstate (void)
{
	return;
	for (int i = 0; i < MAX_JPORTS; i++) {
		horizclear[i] = 1;
		vertclear[i] = 1;
		relativecount[i][0] = relativecount[i][1] = 0;
	}
}

void inputdevice_draco_key(int kc)
{
#ifdef WITH_DRACO
	int state = (kc & 1) == 0;
	kc >>= 1;
	for (int i = 1; events[i].name; i++) {
		if (events[i].data == kc && events[i].data2 && events[i].allow_mask == AM_K) {
			int code = events[i].data2;
			if ((code & 0xff00) == 0xe000) {
				code = (code & 0xff) | 0x100;
			} else {
				code &= 0xff;
			}
			draco_keycode(code, state);
			if (state) {
				int rate = draco_keyboard_get_rate();
				int init = ((rate >> 5) & 3) * 250 + 250;
				draco_keybord_repeat_cnt = (int)(vblank_hz * maxvpos * init / 1000);
				draco_keybord_repeat_code = code;
			} else {
				draco_keybord_repeat_code = 0;
			}
		}
	}
#endif
}
