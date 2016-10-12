 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Joystick, mouse and keyboard emulation prototypes and definitions
  *
  * Copyright 1995 Bernd Schmidt
  * Copyright 2001-2002 Toni Wilen
  */

#pragma once
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

#define IDTYPE_JOYSTICK 0
#define IDTYPE_MOUSE 1
#define IDTYPE_KEYBOARD 2
#ifndef INPUTDEVICE_SIMPLE
#define IDTYPE_INTERNALEVENT 3
#define IDTYPE_MAX 4
#else
#define IDTYPE_MAX 3
#endif

struct inputdevice_functions {
    int (*init)(void);
    void (*close)(void);
    int (*acquire)(int,int);
    void (*unacquire)(int);
    void (*read)(void);
    int (*get_num)(void);
    TCHAR* (*get_friendlyname)(int);
    TCHAR* (*get_uniquename)(int);
    int (*get_widget_num)(int);
    int (*get_widget_type)(int,int,TCHAR*,uae_u32*);
    int (*get_widget_first)(int,int);
    int (*get_flags)(int);
};
extern struct inputdevice_functions inputdevicefunc_joystick;
extern struct inputdevice_functions inputdevicefunc_mouse;
extern struct inputdevice_functions inputdevicefunc_keyboard;

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
	int allow_mask;
	int type;
	int unit;
	int data;
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
#define ID_FLAG_SET_ONOFF_VAL 256

#define ID_FLAG_GAMEPORTSCUSTOM_MASK (ID_FLAG_GAMEPORTSCUSTOM1 | ID_FLAG_GAMEPORTSCUSTOM2)
#define ID_FLAG_AUTOFIRE_MASK (ID_FLAG_TOGGLE | ID_FLAG_INVERTTOGGLE | ID_FLAG_AUTOFIRE)

#define ID_FLAG_QUALIFIER1          0x000000100000000
#define ID_FLAG_QUALIFIER1_R        0x000000200000000
#define ID_FLAG_QUALIFIER2          0x000000400000000
#define ID_FLAG_QUALIFIER3          0x000001000000000
#define ID_FLAG_QUALIFIER4          0x000004000000000
#define ID_FLAG_QUALIFIER5          0x000010000000000
#define ID_FLAG_QUALIFIER6          0x000040000000000
#define ID_FLAG_QUALIFIER7          0x000100000000000
#define ID_FLAG_QUALIFIER8          0x000400000000000
#define ID_FLAG_QUALIFIER_SPECIAL   0x001000000000000
#define ID_FLAG_QUALIFIER_SPECIAL_R 0x002000000000000
#define ID_FLAG_QUALIFIER_SHIFT     0x004000000000000
#define ID_FLAG_QUALIFIER_CONTROL   0x010000000000000
#define ID_FLAG_QUALIFIER_ALT       0x040000000000000
#define ID_FLAG_QUALIFIER_WIN       0x100000000000000
#define ID_FLAG_QUALIFIER_MASK      0xfffffff00000000
#define ID_FLAG_QUALIFIER_MASK_R    0xaaaaaaa00000000

#define ID_FLAG_SAVE_MASK_CONFIG 0x000001ff
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
#define IDEV_MAPPED_SET_ONOFF_VAL 256

#define IDEV_MAPPED_QUALIFIER1          0x000000100000000
#define IDEV_MAPPED_QUALIFIER2          0x000000400000000
#define IDEV_MAPPED_QUALIFIER3          0x000001000000000
#define IDEV_MAPPED_QUALIFIER4          0x000004000000000
#define IDEV_MAPPED_QUALIFIER5          0x000010000000000
#define IDEV_MAPPED_QUALIFIER6          0x000040000000000
#define IDEV_MAPPED_QUALIFIER7          0x000100000000000
#define IDEV_MAPPED_QUALIFIER8          0x000400000000000
#define IDEV_MAPPED_QUALIFIER_SPECIAL   0x001000000000000
#define IDEV_MAPPED_QUALIFIER_SHIFT     0x004000000000000
#define IDEV_MAPPED_QUALIFIER_CONTROL   0x010000000000000
#define IDEV_MAPPED_QUALIFIER_ALT       0x040000000000000
#define IDEV_MAPPED_QUALIFIER_WIN       0x100000000000000
#define IDEV_MAPPED_QUALIFIER_MASK      0xfffffff00000000

#define SET_ONOFF_ON_VALUE  0x7fffff01
#define SET_ONOFF_OFF_VALUE 0x7fffff00

#define ID_BUTTON_OFFSET 0
#define ID_BUTTON_TOTAL 32
#define ID_AXIS_OFFSET 32
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
extern void inputdevice_copyconfig (const struct uae_prefs *src, struct uae_prefs *dst);
extern void inputdevice_copy_single_config (struct uae_prefs *p, int src, int dst, int devnum, int selectedwidget);
extern void inputdevice_swap_ports (struct uae_prefs *p, int devnum);
extern void inputdevice_swap_compa_ports (struct uae_prefs *p, int portswap);
extern void inputdevice_config_change (void);
extern int inputdevice_config_change_test (void);
extern int inputdevice_get_device_index (int devnum);
extern TCHAR *inputdevice_get_device_name (int type, int devnum);
extern TCHAR *inputdevice_get_device_name2 (int devnum);
extern TCHAR *inputdevice_get_device_unique_name (int type, int devnum);
extern int inputdevice_get_device_status (int devnum);
extern void inputdevice_set_device_status (int devnum, int enabled);
extern int inputdevice_get_device_total (int type);
extern int inputdevice_get_widget_num (int devnum);
extern int inputdevice_get_widget_type (int devnum, int num, TCHAR *name);

extern int input_get_default_mouse (struct uae_input_device *uid, int num, int port, int af, bool gp, bool wheel, bool joymouseswap);
extern int input_get_default_joystick (struct uae_input_device *uid, int num, int port, int af, int mode, bool gp, bool joymouseswap);
extern int input_get_default_joystick_analog (struct uae_input_device *uid, int num, int port, int af, bool gp, bool joymouseswap);
extern int input_get_default_keyboard (int num);

#define DEFEVENT(A, B, C, D, E, F) INPUTEVENT_ ## A,
enum inputevents {
INPUTEVENT_ZERO,
#include "inputevents.def"
INPUTEVENT_END
};
#undef DEFEVENT

extern void handle_cd32_joystick_cia (uae_u8, uae_u8);
extern uae_u8 handle_parport_joystick (int port, uae_u8 pra, uae_u8 dra);
extern uae_u8 handle_joystick_buttons (uae_u8, uae_u8);

extern int inputdevice_is_tablet (void);
extern int input_mousehack_status (int mode, uaecptr diminfo, uaecptr dispinfo, uaecptr vp, uae_u32 moffset);
extern void input_mousehack_mouseoffset (uaecptr pointerprefs);

extern void setmousebuttonstateall (int mouse, uae_u32 buttonbits, uae_u32 buttonmask);
extern void setjoybuttonstateall (int joy, uae_u32 buttonbits, uae_u32 buttonmask);
extern void setjoybuttonstate (int joy, int button, int state);
extern void setmousebuttonstate (int mouse, int button, int state);
extern void setjoystickstate (int joy, int axle, int state, int max);
extern int getjoystickstate (int mouse);
void setmousestate (int mouse, int axis, int data, int isabs);

extern int getmousestate (int mouse);
extern void inputdevice_updateconfig (const struct uae_prefs *srcprefs, struct uae_prefs *dstprefs);
extern void inputdevice_updateconfig_internal (const struct uae_prefs *srcprefs, struct uae_prefs *dstprefs);
#ifndef INPUTDEVICE_SIMPLE
extern void inputdevice_devicechange (struct uae_prefs *prefs);

#define INTERNALEVENT_CPURESET 0
#define INTERNALEVENT_KBRESET 1

extern void send_internalevent (int eventid);
#endif

extern int inputdevice_translatekeycode (int keyboard, int scancode, int state);
extern void inputdevice_checkqualifierkeycode (int keyboard, int scancode, int state);
extern void inputdevice_setkeytranslation (struct uae_input_device_kbr_default **trans, int **kbmaps);
extern void inputdevice_do_keyboard (int code, int state);
extern int inputdevice_iskeymapped (int keyboard, int scancode);
extern int inputdevice_get_compatibility_input (struct uae_prefs*, int index, int *typelist, int *inputlist, const int **at);
extern struct inputevent *inputdevice_get_eventinfo (int evt);
extern bool inputdevice_get_eventname (const struct inputevent *ie, TCHAR *out);
extern void inputdevice_compa_prepare_custom (struct uae_prefs *prefs, int index, int mode, bool removeold);
extern void inputdevice_compa_clear (struct uae_prefs *prefs, int index);
extern int intputdevice_compa_get_eventtype (int evt, const int **axistable);
extern void inputdevice_sparecopy (struct uae_input_device *uid, int num, int sub);

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

extern void inputdevice_vsync (void);
extern void inputdevice_hsync (void);
extern void inputdevice_reset (void);

extern void write_inputdevice_config (struct uae_prefs *p, struct zfile *f);
extern void read_inputdevice_config (struct uae_prefs *p, const TCHAR *option, TCHAR *value);
extern void reset_inputdevice_config (struct uae_prefs *pr);
extern void store_inputdevice_config (struct uae_prefs *pr);
extern void restore_inputdevice_config (struct uae_prefs *p, int portnum);
extern int inputdevice_joyport_config (struct uae_prefs *p, const TCHAR *value, int portnum, int mode, int type, bool validate);
extern int inputdevice_getjoyportdevice (int port, int val);
extern void inputdevice_validate_jports (struct uae_prefs *p, int changedport);

extern void inputdevice_init (void);
extern void inputdevice_close (void);
extern void inputdevice_default_prefs (struct uae_prefs *p);

extern void inputdevice_acquire (int allmode);
extern void inputdevice_unacquire (void);

extern void inputdevice_add_inputcode (int code, int state);
extern void inputdevice_handle_inputcode (void);

extern void inputdevice_tablet_strobe (void);

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
#define JSEM_JOYS 100
#define JSEM_MICE 200
#define JSEM_END 300
#define JSEM_XARCADE1LAYOUT (JSEM_KBDLAYOUT + 3)
#define JSEM_XARCADE2LAYOUT (JSEM_KBDLAYOUT + 4)
#define JSEM_DECODEVAL(port,p) ((p)->jports[port].id)
#define JSEM_ISNUMPAD(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT)
#define JSEM_ISCURSOR(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 1)
#define JSEM_ISSOMEWHEREELSE(port,p) (jsem_iskbdjoy(port,p) == JSEM_KBDLAYOUT + 2)
#define JSEM_ISXARCADE1(port,p) (jsem_iskbdjoy(port,p) == JSEM_XARCADE1LAYOUT)
#define JSEM_ISXARCADE2(port,p) (jsem_iskbdjoy(port,p) == JSEM_XARCADE2LAYOUT)
#define JSEM_LASTKBD 5
#define JSEM_ISANYKBD(port,p) (jsem_iskbdjoy(port,p) >= JSEM_KBDLAYOUT && jsem_iskbdjoy(port,p) < JSEM_KBDLAYOUT + JSEM_LASTKBD)

extern int jsem_isjoy (int port, const struct uae_prefs *p);
extern int jsem_ismouse (int port, const struct uae_prefs *p);
extern int jsem_iskbdjoy (int port, const struct uae_prefs *p);

extern int inputdevice_uaelib (const TCHAR *, const TCHAR *);
