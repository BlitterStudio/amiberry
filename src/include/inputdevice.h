 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Joystick, mouse and keyboard emulation prototypes and definitions
  *
  * Copyright 1995 Bernd Schmidt
  * Copyright 2001-2002 Toni Wilen
  */

#ifndef UAE_INPUTDEVICE_H
#define UAE_INPUTDEVICE_H

#include "uae/types.h"

#define DIR_LEFT_BIT 0
#define DIR_RIGHT_BIT 1
#define DIR_UP_BIT 2
#define DIR_DOWN_BIT 3
#define DIR_LEFT (1 << DIR_LEFT_BIT)
#define DIR_RIGHT (1 << DIR_RIGHT_BIT)
#define DIR_UP (1 << DIR_UP_BIT)
#define DIR_DOWN (1 << DIR_DOWN_BIT)

#define JOYBUTTON_1 0 /* fire/left mousebutton */
#define JOYBUTTON_2 1 /* 2nd/right mousebutton */
#define JOYBUTTON_3 2 /* 3rd/middle mousebutton */
#define JOYBUTTON_CD32_PLAY 3
#define JOYBUTTON_CD32_RWD 4
#define JOYBUTTON_CD32_FFW 5
#define JOYBUTTON_CD32_GREEN 6
#define JOYBUTTON_CD32_YELLOW 7
#define JOYBUTTON_CD32_RED 8
#define JOYBUTTON_CD32_BLUE 9

#define JOYBUTTON_LIGHTPEN2 10

#define IDTYPE_JOYSTICK 0
#define IDTYPE_MOUSE 1
#define IDTYPE_KEYBOARD 2
#define IDTYPE_INTERNALEVENT 3
#define IDTYPE_MAX 4

struct inputdevice_functions {
	int (*init)(void);
	void (*close)(void);
	int (*acquire)(int,int);
	void (*unacquire)(int);
	void (*read)(void);
	int (*get_num)(void);
	const TCHAR* (*get_friendlyname)(int);
	const TCHAR* (*get_uniquename)(int);
	int (*get_widget_num)(int);
	int (*get_widget_type)(int,int,TCHAR*,uae_u32*);
	int (*get_widget_first)(int,int);
	int (*get_flags)(int);
};
extern struct inputdevice_functions inputdevicefunc_joystick;
extern struct inputdevice_functions inputdevicefunc_mouse;
extern struct inputdevice_functions inputdevicefunc_keyboard;
extern int pause_emulation;

struct uae_input_device_default_node
{
	int evt;
	uae_u64 flags;
};

struct uae_input_device_kbr_default {
	int scancode;
	struct uae_input_device_default_node node[MAX_INPUT_SUB_EVENT];
};

struct inputevent {
	const TCHAR *confname;
	const TCHAR *name;
	const TCHAR *shortname;
	int allow_mask;
	int type;
	int unit;
	int data;
	int portid;
	int data2;
};

#define MAX_INPUT_QUALIFIERS (8 + 5)

/* event flags */
#define ID_FLAG_AUTOFIRE 1
#define ID_FLAG_TOGGLE 2
#define ID_FLAG_GAMEPORTSCUSTOM1 4
#define ID_FLAG_GAMEPORTSCUSTOM2 8
#define ID_FLAG_INVERTTOGGLE 16
#define ID_FLAG_INVERT 32
#define ID_FLAG_RESERVEDGAMEPORTSCUSTOM 64
#define ID_FLAG_SET_ONOFF 128
#define ID_FLAG_SET_ONOFF_VAL1 256
#define ID_FLAG_SET_ONOFF_VAL2 512

#define ID_FLAG_GAMEPORTSCUSTOM_MASK (ID_FLAG_GAMEPORTSCUSTOM1 | ID_FLAG_GAMEPORTSCUSTOM2)
#define ID_FLAG_AUTOFIRE_MASK (ID_FLAG_TOGGLE | ID_FLAG_INVERTTOGGLE | ID_FLAG_INVERT | ID_FLAG_AUTOFIRE)

#define ID_FLAG_QUALIFIER1          0x000000100000000ULL
#define ID_FLAG_QUALIFIER1_R        0x000000200000000ULL
#define ID_FLAG_QUALIFIER2          0x000000400000000ULL
#define ID_FLAG_QUALIFIER3          0x000001000000000ULL
#define ID_FLAG_QUALIFIER4          0x000004000000000ULL
#define ID_FLAG_QUALIFIER5          0x000010000000000ULL
#define ID_FLAG_QUALIFIER6          0x000040000000000ULL
#define ID_FLAG_QUALIFIER7          0x000100000000000ULL
#define ID_FLAG_QUALIFIER8          0x000400000000000ULL
#define ID_FLAG_QUALIFIER_SPECIAL   0x001000000000000ULL
#define ID_FLAG_QUALIFIER_SPECIAL_R 0x002000000000000ULL
#define ID_FLAG_QUALIFIER_SHIFT     0x004000000000000ULL
#define ID_FLAG_QUALIFIER_CONTROL   0x010000000000000ULL
#define ID_FLAG_QUALIFIER_ALT       0x040000000000000ULL
#define ID_FLAG_QUALIFIER_WIN       0x100000000000000ULL
#define ID_FLAG_QUALIFIER_MASK      0xfffffff00000000ULL
#define ID_FLAG_QUALIFIER_MASK_R    0xaaaaaaa00000000ULL

#define ID_FLAG_SAVE_MASK_CONFIG 0x000003ff
#define ID_FLAG_SAVE_MASK_QUALIFIERS ID_FLAG_QUALIFIER_MASK
#define ID_FLAG_SAVE_MASK_FULL (ID_FLAG_SAVE_MASK_CONFIG | ID_FLAG_SAVE_MASK_QUALIFIERS)

#define IDEV_WIDGET_NONE 0
#define IDEV_WIDGET_BUTTON 1
#define IDEV_WIDGET_AXIS 2
#define IDEV_WIDGET_BUTTONAXIS 3
#define IDEV_WIDGET_KEY 4

#define IDEV_MAPPED_AUTOFIRE_POSSIBLE 1
#define IDEV_MAPPED_AUTOFIRE_SET 2
#define IDEV_MAPPED_TOGGLE 4
#define IDEV_MAPPED_INVERTTOGGLE 8
#define IDEV_MAPPED_GAMEPORTSCUSTOM1 16
#define IDEV_MAPPED_GAMEPORTSCUSTOM2 32
#define IDEV_MAPPED_INVERT 64
#define IDEV_MAPPED_SET_ONOFF 128
#define IDEV_MAPPED_SET_ONOFF_VAL1 256
#define IDEV_MAPPED_SET_ONOFF_VAL2 512

#define IDEV_MAPPED_QUALIFIER1          0x000000100000000ULL
#define IDEV_MAPPED_QUALIFIER2          0x000000400000000ULL
#define IDEV_MAPPED_QUALIFIER3          0x000001000000000ULL
#define IDEV_MAPPED_QUALIFIER4          0x000004000000000ULL
#define IDEV_MAPPED_QUALIFIER5          0x000010000000000ULL
#define IDEV_MAPPED_QUALIFIER6          0x000040000000000ULL
#define IDEV_MAPPED_QUALIFIER7          0x000100000000000ULL
#define IDEV_MAPPED_QUALIFIER8          0x000400000000000ULL
#define IDEV_MAPPED_QUALIFIER_SPECIAL   0x001000000000000ULL
#define IDEV_MAPPED_QUALIFIER_SHIFT     0x004000000000000ULL
#define IDEV_MAPPED_QUALIFIER_CONTROL   0x010000000000000ULL
#define IDEV_MAPPED_QUALIFIER_ALT       0x040000000000000ULL
#define IDEV_MAPPED_QUALIFIER_WIN       0x100000000000000ULL
#define IDEV_MAPPED_QUALIFIER_MASK      0xfffffff00000000ULL

#define SET_ONOFF_PRESSREL_VALUE 0x7fffff30
#define SET_ONOFF_PRESS_VALUE 0x7fffff20
#define SET_ONOFF_ON_VALUE  0x7fffff10
#define SET_ONOFF_OFF_VALUE 0x7fffff00
#define SET_ONOFF_MASK_PRESS 15

#define ID_BUTTON_OFFSET 0
#ifdef AMIBERRY
#define ID_BUTTON_TOTAL 64
#define ID_AXIS_OFFSET 64
#else
#define ID_BUTTON_TOTAL 32
#define ID_AXIS_OFFSET 32
#endif
#define ID_AXIS_TOTAL 32

#define MAX_COMPA_INPUTLIST 30

/* event masks */
#define AM_KEY 1 /* keyboard allowed */
#define AM_JOY_BUT 2 /* joystick buttons allowed */
#define AM_JOY_AXIS 4 /* joystick axis allowed */
#define AM_MOUSE_BUT 8 /* mouse buttons allowed */
#define AM_MOUSE_AXIS 16 /* mouse direction allowed */
#define AM_AF 32 /* supports autofire */
#define AM_INFO 64 /* information data for gui */
#define AM_DUMMY 128 /* placeholder */
#define AM_CUSTOM 256 /* custom event */
#define AM_SETTOGGLE 512 /* on/off/toggle */
#define AM_K (AM_KEY|AM_JOY_BUT|AM_MOUSE_BUT|AM_AF) /* generic button/switch */
#define AM_KK (AM_KEY|AM_JOY_BUT|AM_MOUSE_BUT)
#define AM_KT (AM_K|AM_SETTOGGLE)

extern int inputdevice_iterate (int devnum, int num, TCHAR *name, int *af);
extern bool inputdevice_set_gameports_mapping (struct uae_prefs *prefs, int devnum, int num, int evtnum, uae_u64 flags, int port, int input_selected_setting);
extern int inputdevice_set_mapping (int devnum, int num, const TCHAR *name, TCHAR *custom, uae_u64 flags, int port, int sub);
extern int inputdevice_get_mapping (int devnum, int num, uae_u64 *pflags, int *port, TCHAR *name, TCHAR *custom, int sub);
extern void inputdevice_copyconfig (struct uae_prefs *src, struct uae_prefs *dst);
extern void inputdevice_copy_single_config (struct uae_prefs *p, int src, int dst, int devnum, int selectedwidget);
extern void inputdevice_copyjports(struct uae_prefs *srcprefs, struct uae_prefs *dstprefs);
extern void inputdevice_swap_ports (struct uae_prefs *p, int devnum);
extern void inputdevice_swap_compa_ports (struct uae_prefs *p, int portswap);
extern void inputdevice_config_change (void);
extern int inputdevice_config_change_test (void);
extern int inputdevice_get_device_index (int devnum);
extern const TCHAR *inputdevice_get_device_name (int type, int devnum);
extern const TCHAR *inputdevice_get_device_name2 (int devnum);
extern const TCHAR *inputdevice_get_device_unique_name (int type, int devnum);
extern int inputdevice_get_device_status (int devnum);
extern void inputdevice_set_device_status (int devnum, int enabled);
extern int inputdevice_get_device_total (int type);
extern int inputdevice_get_widget_num (int devnum);
extern int inputdevice_get_widget_type (int devnum, int num, TCHAR *name, bool inccode);
extern int send_input_event (int nr, int state, int max, int autofire);
extern void release_keys(void);

extern int input_get_default_mouse (struct uae_input_device *uid, int num, int port, int af, bool gp, bool wheel, bool joymouseswap);
extern int input_get_default_lightpen (struct uae_input_device *uid, int num, int port, int af, bool gp, bool joymouseswap, int submode);
extern int input_get_default_joystick (struct uae_input_device *uid, int num, int port, int af, int mode, bool gp, bool joymouseswap, bool default_osk);
extern int input_get_default_joystick_analog (struct uae_input_device *uid, int num, int port, int af, bool gp, bool joymouseswap, bool default_osk);
extern int input_get_default_keyboard (int num);

#define DEFEVENT(A, B, C, D, E, F) INPUTEVENT_ ## A,
#define DEFEVENT2(A, B, B2, C, D, E, F, G) INPUTEVENT_ ## A,
#define DEFEVENTKB(A, B, C, F, PC) INPUTEVENT_ ## A,

enum inputevents {
INPUTEVENT_ZERO,
#include "../inputevents.def"
INPUTEVENT_END
};
#undef DEFEVENT
#undef DEFEVENT2
#undef DEFEVENTKB

extern void handle_cd32_joystick_cia (uae_u8, uae_u8);
extern uae_u8 handle_parport_joystick (int port, uae_u8 data);
extern uae_u8 handle_joystick_buttons (uae_u8, uae_u8);

#define MAGICMOUSE_BOTH 0
#define MAGICMOUSE_NATIVE_ONLY 1
#define MAGICMOUSE_HOST_ONLY 2

extern int magicmouse_alive (void);
extern int is_tablet (void);
extern int is_touch_lightpen (void);
extern int inputdevice_is_tablet (void);
extern int input_mousehack_status(TrapContext *ctx, int mode, uaecptr diminfo, uaecptr dispinfo, uaecptr vp, uae_u32 moffset);
extern void input_mousehack_mouseoffset (uaecptr pointerprefs);
extern int mousehack_alive (void);
extern void mousehack_wakeup(void);
extern void mousehack_write(int reg, uae_u16 val);
extern void setmouseactive(int monid, int);
extern bool ismouseactive(void);
extern void inputdevice_read_msg(bool);

extern void setmousebuttonstateall (int mouse, uae_u32 buttonbits, uae_u32 buttonmask);
extern void setjoybuttonstateall (int joy, uae_u32 buttonbits, uae_u32 buttonmask);
extern void setjoybuttonstate (int joy, int button, int state);
extern void setmousebuttonstate (int mouse, int button, int state);
extern void setjoystickstate (int joy, int axle, int state, int max);
extern int getjoystickstate (int mouse);
void setmousestate (int mouse, int axis, int data, int isabs);
extern int getmousestate (int mouse);
extern void inputdevice_updateconfig (struct uae_prefs *srcprefs, struct uae_prefs *dstprefs);
extern void inputdevice_updateconfig_internal (struct uae_prefs *srcprefs, struct uae_prefs *dstprefs);
extern bool inputdevice_devicechange (struct uae_prefs *prefs);

#define INTERNALEVENT_CPURESET 0
#define INTERNALEVENT_KBRESET 1
#define INTERNALEVENT_TOUCHLIGHTPEN 2

extern void send_internalevent (int eventid);

extern int inputdevice_translatekeycode (int keyboard, int scancode, int state, bool alwaysrelease);
extern void inputdevice_checkqualifierkeycode (int keyboard, int scancode, int state);
extern void inputdevice_setkeytranslation (struct uae_input_device_kbr_default **trans, int **kbmaps);
extern void inputdevice_do_keyboard(int code, int state);
extern void inputdevice_do_kb_reset(void);
extern int inputdevice_iskeymapped (int keyboard, int scancode);
extern int inputdevice_synccapslock (int, int*);
extern void inputdevice_testrecord (int type, int num, int wtype, int wnum, int state, int max);
extern int inputdevice_get_compatibility_input (struct uae_prefs*, int index, int *typelist, int *inputlist, const int **at);
extern const struct inputevent *inputdevice_get_eventinfo (int evt);
extern bool inputdevice_get_eventname (const struct inputevent *ie, TCHAR *out);
extern void inputdevice_compa_prepare_custom (struct uae_prefs *prefs, int index, int mode, bool removeold);
extern void inputdevice_compa_clear (struct uae_prefs *prefs, int index);
extern int intputdevice_compa_get_eventtype (int evt, const int **axistable);
extern void inputdevice_sparecopy (struct uae_input_device *uid, int num, int sub);
extern void inputdevice_parse_jport_custom(struct uae_prefs *pr, int index, int port, TCHAR *outname);
extern void inputdevice_generate_jport_custom(struct uae_prefs *pr, int port);
extern void inputdevice_forget_unplugged_device(int portnum);

extern uae_u16 potgo_value;
extern uae_u16 POTGOR (void);
extern void POTGO (uae_u16 v);
extern uae_u16 POT0DAT (void);
extern uae_u16 POT1DAT (void);
extern void JOYTEST (uae_u16 v);
extern uae_u16 JOY0DAT (void);
extern uae_u16 JOY1DAT (void);
extern void JOYSET (int num, uae_u16 v);
extern uae_u16 JOYGET (int num);
extern void pulse_joydat(int joy, int xy, int dir);


extern void inputdevice_vsync(void);
extern void inputdevice_hsync(bool);
extern void inputdevice_hsync_strobe(void);
extern void inputdevice_reset(void);

extern void write_inputdevice_config (struct uae_prefs *p, struct zfile *f);
extern void read_inputdevice_config (struct uae_prefs *p, const TCHAR *option, TCHAR *value);
extern void reset_inputdevice_config (struct uae_prefs *pr, bool reset);
extern int inputdevice_joyport_config(struct uae_prefs *p, const TCHAR *value1, const TCHAR *value2, int portnum, int mode, int submode, int type, bool candefault);
extern void inputdevice_joyport_config_store(struct uae_prefs *p, const TCHAR *value, int portnum, int mode, int submode, int type);
extern int inputdevice_getjoyportdevice (int port, int val);
extern void inputdevice_validate_jports (struct uae_prefs *p, int changedport, bool *fixedports);
extern void inputdevice_fix_prefs(struct uae_prefs *p, bool userconfig);
extern void inputdevice_jportcustom_fixup(struct uae_prefs *p, TCHAR*, int);

extern void inputdevice_init (void);
extern void inputdevice_close (void);
extern void inputdevice_default_prefs (struct uae_prefs *p);

extern void inputdevice_acquire (int allmode);
extern void inputdevice_unacquire(void);
extern void inputdevice_unacquire(int inputmask);
extern void inputdevice_releasebuttons(void);

extern void indicator_leds (int num, int state);

extern void warpmode(int mode);
extern void bootwarpmode(void);
extern void pausemode(int mode);

extern void inputdevice_add_inputcode (int code, int state, const TCHAR *);
extern void inputdevice_handle_inputcode (void);

extern void inputdevice_tablet (int x, int y, int z,
		  int pressure, uae_u32 buttonbits, int inproximity,
		  int ax, int ay, int az, int devid);
extern void inputdevice_tablet_info (int maxx, int maxy, int maxz, int maxax, int maxay, int maxaz, int xres, int yres);
extern void inputdevice_tablet_strobe (void);
extern void tablet_lightpen(int x, int y, int maxx, int maxy, int touch, int buttonmask, bool touchmode, int devid, int lpnum);
extern int inputdevice_get_lightpen_id(void);

extern uae_u64 input_getqualifiers (void);

extern void setsystime (void);

#define JSEM_MODE_DEFAULT 0
#define JSEM_MODE_WHEELMOUSE 1
#define JSEM_MODE_MOUSE 2
#define JSEM_MODE_JOYSTICK 3
#define JSEM_MODE_GAMEPAD 4
#define JSEM_MODE_JOYSTICK_ANALOG 5
#define JSEM_MODE_MOUSE_CDTV 6
#define JSEM_MODE_JOYSTICK_CD32 7
#define JSEM_MODE_LIGHTPEN 8

#define JSEM_KBDLAYOUT 0
#define JSEM_CUSTOM 10
#define JSEM_JOYS 100
#define JSEM_MICE 200
#define JSEM_END 300
#define JSEM_DECODEVAL(port,p) ((p)->jports[port].id)
#define JSEM_ISNUMPAD(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT)
#define JSEM_ISCURSOR(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 1)
#define JSEM_ISSOMEWHEREELSE(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 2)
#define JSEM_ISKEYRAH(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 3)
#define JSEM_ISRAPLAYER1(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 4)
#define JSEM_ISRAPLAYER2(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 5)
#define JSEM_ISRAPLAYER3(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 6)
#define JSEM_ISRAPLAYER4(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 7)

#define JSEM_ISCUSTOM(port,p) ((p)->jports[port].id >= JSEM_CUSTOM && (p)->jports[port].id < JSEM_CUSTOM + MAX_JPORTS_CUSTOM)
#define JSEM_GETCUSTOMIDX(port,p) ((p)->jports[port].id - JSEM_CUSTOM)
#define JSEM_LASTKBD 8
#define JSEM_ISANYKBD(port,p) (jsem_iskbdjoy(port,p) >= JSEM_KBDLAYOUT && jsem_iskbdjoy(port,p) < JSEM_KBDLAYOUT + JSEM_LASTKBD)

extern int jsem_isjoy (int port, const struct uae_prefs *p);
extern int jsem_ismouse (int port, const struct uae_prefs *p);
extern int jsem_iskbdjoy (int port, const struct uae_prefs *p);

extern int inputdevice_uaelib (const TCHAR *, const TCHAR *);
extern int inputdevice_uaelib(const TCHAR *s, int parm, int max, bool autofire);
extern int handle_custom_event (const TCHAR *custom, int append);
extern int inputdevice_geteventid(const TCHAR *s);
extern void inputdevice_playevents(void);

extern int inputdevice_testread(int*, int*, int*, bool);
extern int inputdevice_istest(void);
extern void inputdevice_settest(bool, bool);
extern int inputdevice_testread_count(void);

bool target_can_autoswitchdevice(void);
void target_inputdevice_acquire(void);
void target_inputdevice_unacquire(bool);

bool key_specialpressed(void);
bool key_shiftpressed(void);
bool key_altpressed(void);
bool key_ctrlpressed(void);
bool key_winpressed(void);

void inputdevice_draco_key(int kc);

extern int key_swap_hack, key_swap_hack2, key_swap_end_pgup;

#endif /* UAE_INPUTDEVICE_H */
