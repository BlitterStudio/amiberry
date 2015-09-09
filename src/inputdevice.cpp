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
#include "keyboard.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "memory.h"
#include "events.h"
#include "newcpu.h"
#include "uae.h"
#include "joystick.h"
#include "picasso96.h"


void write_inputdevice_config (struct uae_prefs *p, struct zfile *f)
{
  cfgfile_write (f, "input.joymouse_speed_analog=%d\n", p->input_joymouse_multiplier);
  cfgfile_write (f, "input.autofire=%d\n", p->input_autofire_framecnt);
}

static char *getstring (char **pp)
{
    int i;
    static char str[1000];
    char *p = *pp;

    if (*p == 0)
	return 0;
    i = 0;
    while (*p != 0 && *p !='.' && *p != ',') str[i++] = *p++;
    if (*p == '.' || *p == ',') p++;
    str[i] = 0;
    *pp = p;
    return str;
}

void reset_inputdevice_config (struct uae_prefs *pr)
{
}

void read_inputdevice_config (struct uae_prefs *pr, char *option, char *value)
{
  char *p;

  option += 6; /* "input." */
  p = getstring (&option);
    if (!strcasecmp (p, "joymouse_speed_analog"))
	pr->input_joymouse_multiplier = atol (value);
    if (!strcasecmp (p, "autofire"))
	pr->input_autofire_framecnt = atol (value);
}

/* Mousehack stuff */

#define defstepx (1<<16)
#define defstepy (1<<16)

static int lastsampledmx, lastsampledmy;
static int lastspr0x,lastspr0y,spr0pos,spr0ctl;
static int mstepx,mstepy;
static int sprvbfl;

int lastmx, lastmy;
int newmousecounters;
static int mouse_x, mouse_y;
static int ievent_alive = 0;

static enum mousestate mousestate;

extern int mouseMoving;
extern int fcounter;


void mousehack_handle (int sprctl, int sprpos)
{
    if (!sprvbfl && ((sprpos & 0xff) << 2) > 2 * DISPLAY_LEFT_SHIFT) {
	spr0ctl = sprctl;
	spr0pos = sprpos;
	sprvbfl = 2;
    }
}

static void mousehack_setunknown (void)
{
    mousestate = mousehack_unknown;
}

static void mousehack_setdontcare (void)
{
    if (mousestate == mousehack_dontcare)
	return;

    write_log ("Don't care mouse mode set\n");
    mousestate = mousehack_dontcare;
    lastspr0x = lastmx; lastspr0y = lastmy;
    mstepx = defstepx; mstepy = defstepy;
}

static void mousehack_setfollow (void)
{
    if (mousestate == mousehack_follow)
	return;

    if (!mousehack_allowed ()) {
	    mousehack_set (mousehack_normal);
	    return;
    }
    write_log ("Follow sprite mode set\n");
    mousestate = mousehack_follow;
    sprvbfl = 0;
    spr0ctl = spr0pos = 0;
    mstepx = defstepx; mstepy = defstepy;
}

void mousehack_set (enum mousestate state)
{
    switch (state)
    {
	case mousehack_dontcare:
	mousehack_setdontcare();
	break;
	case mousehack_follow:
	mousehack_setfollow();
	break;
	default:
	mousestate = (enum mousestate) state;
	break;
    }
}

int mousehack_get (void)
{
    return mousestate;
}

int mousehack_alive (void)
{
    return ievent_alive > 0;
}

void togglemouse (void)
{
    switch (mousestate) {
     case mousehack_dontcare: mousehack_setfollow (); break;
     case mousehack_follow: mousehack_setdontcare (); break;
     default: break; /* Nnnnnghh! */
    }
}

STATIC_INLINE int adjust (int val)
{
  if (val > 127)
	  return 127;
  else if (val < -127)
	  return -127;
  return val;
}

void do_mouse_hack (void)
{
  int spr0x = ((spr0pos & 0xff) << 2) | ((spr0ctl & 1) << 1);
  int spr0y = ((spr0pos >> 8) | ((spr0ctl & 4) << 6)) << 1;
  int diffx, diffy;

  if (ievent_alive > 0) 
  {
	  mouse_x = mouse_y = 0;
	  return;
  }
  switch (mousestate) {
    case mousehack_normal:
	    diffx = lastmx - lastsampledmx;
	    diffy = lastmy - lastsampledmy;
	    if (!newmousecounters) {
	      if (diffx > 127) diffx = 127;
	      if (diffx < -127) diffx = -127;
	      mouse_x += diffx;
	      if (diffy > 127) diffy = 127;
	      if (diffy < -127) diffy = -127;
	      mouse_y += diffy;
	    }
	    lastsampledmx += diffx; lastsampledmy += diffy;
	    break;

    case mousehack_dontcare:
	    diffx = adjust (((lastmx - lastspr0x) * mstepx) >> 16);
	    diffy = adjust (((lastmy - lastspr0y) * mstepy) >> 16);
	    lastspr0x = lastmx; 
	    lastspr0y = lastmy;
	    mouse_x += diffx;
	    mouse_y += diffy;
	    break;

    case mousehack_follow:
      //------------------------------------------
      // New stylus<->follow mouse mode
      //------------------------------------------
      #ifndef RASPBERRY
      printf("do_mouse_hack: sprvbfl=%d\n", sprvbfl);
      #endif
	    if (sprvbfl && (sprvbfl-- > 1)) 
      {
        int stylusxpos, stylusypos;          
#ifdef PICASSO96
        if (picasso_on) {
	        stylusxpos = lastmx - picasso96_state.XOffset;
	        stylusypos = lastmy - picasso96_state.YOffset;
        } else
#endif
        {
  	      stylusxpos = coord_native_to_amiga_x (lastmx);
	        stylusypos = coord_native_to_amiga_y (lastmy) << 1;
	      }
	      if(stylusxpos != spr0x || stylusypos != spr0y)
        {
          diffx = (stylusxpos - spr0x);
          diffy = (stylusypos - spr0y);
          if(diffx > 10 || diffx < -10)
            diffx = diffx * 50;
          else
            diffx = diffx * 100;
          if(diffy > 10 || diffy < -10)
            diffy = diffy * 50;
          else
            diffy = diffy * 100;
          mouse_x += adjust(diffx / 100);
          mouse_y += adjust(diffy / 100);
          mouseMoving = 1;
          fcounter = 0;
        }
      }          
      break;
	
    default:
	    return;
  }
}


uae_u16 JOY0DAT (void)
{
    do_mouse_hack ();
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
    mouse_x = v & 0xFC;
    mouse_y = (v >> 8) & 0xFC;
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

    if (buttonstate[2] || (joy0button & 2))
    	v &= 0xFBFF;

    if (buttonstate[1] || (joy0button & 4))
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
	
	if (buttonstate[2])
		cnt = ((cnt + 1) & 0xFF) | (cnt & 0xFF00);
	
	if (buttonstate[1])
		cnt += 0x100;
	
	return cnt;
}

void inputdevice_vsync (void)
{
	static int back_joy0button=0;

	getjoystate (0, &joy1dir, &joy1button);
	getjoystate (1, &joy0dir, &joy0button);
	if (joy0button!=back_joy0button)
  {
		back_joy0button= joy0button;
		buttonstate[0]= joy0button & 0x01;
	}

  if (ievent_alive > 0)
  	ievent_alive--;
}


void inputdevice_reset (void)
{
  if (needmousehack ())
  	mousehack_set (mousehack_dontcare);
  else
	  mousehack_set (mousehack_normal);
  ievent_alive = 0;
}

void inputdevice_updateconfig (struct uae_prefs *prefs)
{
  if(prefs->pandora_custom_dpad == 2)
  	mousehack_set(mousehack_follow);
  else
    mousehack_set (mousehack_dontcare);
  joystick_setting_changed ();
}

void inputdevice_default_prefs (struct uae_prefs *p)
{
  inputdevice_init ();
  p->input_joymouse_multiplier = 2;
  p->input_autofire_framecnt = 8;
}

void inputdevice_init (void)
{
  init_joystick ();
 	inputmode_init();
}

void inputdevice_close (void)
{
  close_joystick ();
}

int inputdevice_config_change_test (void)
{
  return 1;
}

void inputdevice_copyconfig (struct uae_prefs *src, struct uae_prefs *dst)
{
  dst->input_joymouse_multiplier = src->input_joymouse_multiplier;
  dst->input_autofire_framecnt = src->input_autofire_framecnt;

  dst->pandora_joyConf = src->pandora_joyConf;
  dst->pandora_joyPort = src->pandora_joyPort;
  dst->pandora_tapDelay = src->pandora_tapDelay;
  dst->pandora_stylusOffset = src->pandora_stylusOffset;

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
