 /*
  * UAE - The Un*x Amiga Emulator
  *
  * joystick/mouse emulation
  *
  * Copyright 2001, 2002 Toni Wilen
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "xwin.h"
#include "drawing.h"
#include "uae.h"
#include "joystick.h"
#include "picasso96.h"

int joy0button, joy1button;
static unsigned int joy0dir, joy1dir;

extern int bootrom_header, bootrom_items;

void write_inputdevice_config (struct uae_prefs *p, struct zfile *f)
{
  cfgfile_write (f, "input.joymouse_speed_analog", "%d", p->input_joymouse_multiplier);
  cfgfile_write (f, "input.autofire", "%d", p->input_autofire_framecnt);
}

static TCHAR *getstring (TCHAR **pp)
{
    int i;
    static TCHAR str[1000];
    TCHAR *p = *pp;

    if (*p == 0)
	return 0;
    i = 0;
    while (*p != 0 && *p !='.' && *p != ',') 
  str[i++] = *p++;
    if (*p == '.' || *p == ',') 
  p++;
    str[i] = 0;
    *pp = p;
    return str;
}

void reset_inputdevice_config (struct uae_prefs *pr)
{
}

void read_inputdevice_config (struct uae_prefs *pr, TCHAR *option, TCHAR *value)
{
  char *p;

  option += 6; /* "input." */
  p = getstring (&option);
    if (!strcasecmp (p, "joymouse_speed_analog"))
	pr->input_joymouse_multiplier = _tstol (value);
    if (!strcasecmp (p, "autofire"))
	pr->input_autofire_framecnt = _tstol (value);
}

/* Mousehack stuff */

static int lastsampledmx, lastsampledmy;
static int mouse_x, mouse_y;
static int mouse_maxx, mouse_maxy;

static int mousehack_alive_cnt;
static int lastmx, lastmy;
static int mouseoffset_x, mouseoffset_y;
static int tablet_maxx, tablet_maxy, tablet_data;

int mousehack_alive (void)
{
    return mousehack_alive_cnt > 0 ? mousehack_alive_cnt : 0;
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
    if (!uae_boot_rom)
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

static int getmhoffset (void)
{
    if (!uae_boot_rom)
	return 0;
    return get_long (rtarea_base + bootrom_header + 7 * 4) + bootrom_header;
}

static void mousehack_reset (void)
{
    int off;

    mouseoffset_x = mouseoffset_y = 0;
    mousehack_alive_cnt = 0;
    tablet_data = 0;
    off = getmhoffset ();
    if (off)
	rtarea[off + MH_E] = 0;
}

static void mousehack_enable (void)
{
    int off, mode;

    if (!uae_boot_rom || currprefs.input_tablet == TABLET_OFF)
	return;
    off = getmhoffset ();
    if (rtarea[off + MH_E])
	return;
    mode = 0x80;
    if (currprefs.input_tablet == TABLET_MOUSEHACK)
	mode |= 1;
    if (inputdevice_is_tablet () > 0)
	mode |= 2;
    write_log ("Mouse driver enabled (%s)\n", ((mode & 3) == 3 ? "tablet+mousehack" : ((mode & 3) == 2) ? "tablet" : "mousehack"));
    rtarea[off + MH_E] = 0x80;
}

void input_mousehack_mouseoffset (uaecptr pointerprefs)
{
    mouseoffset_x = (uae_s16)get_word (pointerprefs + 28);
    mouseoffset_y = (uae_s16)get_word (pointerprefs + 30);
}

void input_mousehack_status (int mode, uaecptr diminfo, uaecptr dispinfo, uaecptr vp, uae_u32 moffset)
{
    if (mode == 0) {
	uae_u8 v = rtarea[getmhoffset ()];
	v |= 0x40;
	rtarea[getmhoffset ()] = v;
	write_log ("Tablet driver running (%02x)\n", v);
    } else if (mode == 2) {
	if (mousehack_alive_cnt == 0)
	    mousehack_alive_cnt = -100;
	else if (mousehack_alive_cnt > 0)
	    mousehack_alive_cnt = 100;
    }
}

void inputdevice_tablet_strobe (void)
{
    uae_u8 *p;
    uae_u32 off;

    mousehack_enable ();
    if (!uae_boot_rom)
	return;
    if (!tablet_data)
	return;
    off = getmhoffset ();
    p = rtarea + off;
    p[MH_CNT]++;
}


void getgfxoffset (int *dx, int *dy, int*,int*);

static void inputdevice_mh_abs (int x, int y)
{
    uae_u8 *p;
    uae_u8 tmp[4];
    uae_u32 off;

    mousehack_enable ();
    off = getmhoffset ();
    p = rtarea + off;

    memcpy (tmp, p + MH_ABSX, 4);

    x -= mouseoffset_x + 1;
    y -= mouseoffset_y + 2;

    p[MH_ABSX] = x >> 8;
    p[MH_ABSX + 1] = x;
    p[MH_ABSY] = y >> 8;
    p[MH_ABSY + 1] = y;

    if (!memcmp (tmp, p + MH_ABSX, 4))
	return;
    rtarea[off + MH_E] = 0xc0 | 1;
    p[MH_CNT]++;
    tablet_data = 1;
}

static void mousehack_helper (void)
{
    int x, y;

    if (/*currprefs.input_magic_mouse == 0 ||*/ currprefs.input_tablet < TABLET_MOUSEHACK)
	return;
    x = lastmx;
    y = lastmy;

#ifdef PICASSO96
    if (picasso_on) {
	x -= picasso96_state.XOffset;
	y -= picasso96_state.YOffset;
    } else
#endif
    {
  x = coord_native_to_amiga_x (x);
	y = coord_native_to_amiga_y (y) << 1;
    }
    inputdevice_mh_abs (x, y);
}


static void update_mouse_xy(void)
{
  int diffx, diffy;

  diffx = lastmx - lastsampledmx;
  diffy = lastmy - lastsampledmy;
  lastsampledmx = lastmx; 
  lastsampledmy = lastmy;

  if (diffx > 127) 
    diffx = 127;
  if (diffx < -127) 
    diffx = -127;
  mouse_x += diffx;
  
  if (diffy > 127) 
    diffy = 127;
  if (diffy < -127) 
    diffy = -127;
  mouse_y += diffy;
}

uae_u16 JOY0DAT (void)
{
  update_mouse_xy();
#ifdef RASPBERRY
    if (currprefs.pandora_custom_dpad == 0)
        return joy0dir;
    if (currprefs.pandora_custom_dpad == 1)
        return ((uae_u8)mouse_x) | ((uae_u16)mouse_y << 8);
#else
    return ((uae_u8)mouse_x) + ((uae_u16)mouse_y << 8) + joy0dir;
#endif
}

uae_u16 JOY1DAT (void)
{
    return joy1dir;
}

void JOYTEST (uae_u16 v)
{
  mouse_x = (mouse_x & 3) | (v & 0xFC);
  mouse_y = (mouse_y & 3) | ((v >> 8) & 0xFC);
}


uae_u16 potgo_value;

void inputdevice_hsync (void)
{
}

uae_u16 POT1DAT (void)
{
    return 0;
}

void POTGO (uae_u16 v)
{
  potgo_value = v;
}

uae_u16 POTGOR (void)
{
    uae_u16 v = (potgo_value | (potgo_value >> 1)) & 0x5500;

    v |= (~potgo_value & 0xAA00) >> 1;

    if (joy0button & 2)
    	v &= 0xFBFF;

    if (joy0button & 4)
	v &= 0xFEFF;

    if (joy1button & 2)
	    v &= 0xbfff;

    if (joy1button & 4)
	    v &= 0xefff;

    return v;
}

uae_u16 POT0DAT (void)
{
	static uae_u16 cnt = 0;
	
	if (joy0button & 2)
		cnt = ((cnt + 1) & 0xFF) | (cnt & 0xFF00);
	
	if (joy0button & 4)
		cnt += 0x100;
	
	return cnt;
}

void inputdevice_vsync (void)
{
	getjoystate (1, &joy1dir, &joy1button);
	getjoystate (0, &joy0dir, &joy0button);
}


void inputdevice_reset (void)
{
  mousehack_reset ();
	lastmx = lastmy = 0;
  lastsampledmx = lastsampledmy = 0;
  potgo_value = 0;
}

void inputdevice_updateconfig (struct uae_prefs *prefs)
{
  joystick_setting_changed ();
}

void inputdevice_default_prefs (struct uae_prefs *p)
{
#ifdef PANDORA_SPECIFIC
  p->input_joymouse_multiplier = 20;
#else
  p->input_joymouse_multiplier = 2;
#endif
  p->input_autofire_framecnt = 8;
}

void inputdevice_init (void)
{
  lastsampledmx = 0;
  lastsampledmy = 0;
  
  init_joystick ();
  inputmode_init();
  init_keyboard();
}

void inputdevice_close (void)
{
  close_joystick ();
  inputmode_close();
}

int inputdevice_config_change_test (void)
{
  return 1;
}

void inputdevice_copyconfig (struct uae_prefs *src, struct uae_prefs *dst)
{
  dst->input_joymouse_multiplier = src->input_joymouse_multiplier;
  dst->input_autofire_framecnt = src->input_autofire_framecnt;
  dst->input_tablet = src->input_tablet;
  
  dst->pandora_joyConf = src->pandora_joyConf;
  dst->pandora_joyPort = src->pandora_joyPort;
  dst->pandora_tapDelay = src->pandora_tapDelay;

  dst->pandora_customControls = src->pandora_customControls;
  dst->pandora_custom_dpad = src->pandora_custom_dpad;
  dst->pandora_custom_A = src->pandora_custom_A;
  dst->pandora_custom_B = src->pandora_custom_B;
  dst->pandora_custom_X = src->pandora_custom_X;
  dst->pandora_custom_Y = src->pandora_custom_Y;
  dst->pandora_custom_L = src->pandora_custom_L;
  dst->pandora_custom_R = src->pandora_custom_R;
  dst->pandora_custom_up = src->pandora_custom_up;
  dst->pandora_custom_down = src->pandora_custom_down;
  dst->pandora_custom_left = src->pandora_custom_left;
  dst->pandora_custom_right = src->pandora_custom_right;
  
  dst->pandora_button1 = src->pandora_button1;
  dst->pandora_button2 = src->pandora_button2;
  dst->pandora_autofireButton1 = src->pandora_autofireButton1;
  dst->pandora_jump = src->pandora_jump;

  inputdevice_updateconfig (dst);
}

void inputdevice_mouselimit(int x, int y)
{
  mouse_maxx = x;
  mouse_maxy = y;
}

void setmousestate (int mouse, int axis, int data, int isabs)
{
  if(currprefs.input_tablet > 0) {
    // Use mousehack to set position
    if(isabs) {
      if (axis == 0)
	      lastmx = data;
      else
	      lastmy = data;
    } else {
      if(axis == 0) {
        lastmx += data;
        if(lastmx < 0)
          lastmx = 0;
        else if (lastmx >= mouse_maxx)
          lastmx = mouse_maxx - 1;
      } else {
        lastmy += data;
        if(lastmy < 0)
          lastmy = 0;
        else if (lastmy >= mouse_maxy)
          lastmy = mouse_maxy - 1;
      }
    }

    if (axis)
  	  mousehack_helper ();
  } else {
    // Set mouseposition without hack
    if(axis == 0) {
      lastmx += data;
    } else {
      lastmy += data;
    }
  }
}
