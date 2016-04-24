 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Joystick, mouse and keyboard emulation prototypes and definitions
  *
  * Copyright 1995 Bernd Schmidt
  * Copyright 2001-2002 Toni Wilen
  */

extern void inputdevice_copyconfig (struct uae_prefs *src, struct uae_prefs *dst);
extern int inputdevice_config_change_test (void);

extern int joy0button, joy1button;
extern int buttonstate[3];

extern int buttonstate[3];
static __inline__ uae_u8 handle_joystick_buttons (uae_u8 pra, uae_u8 dra)
{
  uae_u8 but = 0;

  if (!(joy0button & 1))
	    but |= 0x40;
	if (!(joy1button & 1))
	    but |= 0x80;
	if (dra & 0x40)
		but = (but & ~0x40) | (pra & 0x40);
	if (dra & 0x80)
		but = (but & ~0x80) | (pra & 0x80);

  return but;
}

extern int is_tablet (void);
extern int inputdevice_is_tablet (void);
extern void input_mousehack_status (int mode, uaecptr diminfo, uaecptr dispinfo, uaecptr vp, uae_u32 moffset);
extern void input_mousehack_mouseoffset (uaecptr pointerprefs);
extern void inputdevice_mouselimit(int x, int y);

void setmousestate (int mouse, int axis, int data, int isabs);

extern void inputdevice_updateconfig (struct uae_prefs *prefs);

extern uae_u16 potgo_value;
extern uae_u16 POTGOR (void);
extern void POTGO (uae_u16 v);
extern uae_u16 POT0DAT (void);
extern uae_u16 POT1DAT (void);
extern void JOYTEST (uae_u16 v);
extern uae_u16 JOY0DAT (void);
extern uae_u16 JOY1DAT (void);

extern void inputdevice_vsync (void);
extern void inputdevice_hsync (void);
extern void inputdevice_reset (void);

extern void write_inputdevice_config (struct uae_prefs *p, struct zfile *f);
extern void read_inputdevice_config (struct uae_prefs *p, TCHAR *option, TCHAR *value);
extern void reset_inputdevice_config (struct uae_prefs *pr);

extern void inputdevice_init (void);
extern void inputdevice_close (void);
extern void inputdevice_default_prefs (struct uae_prefs *p);

extern void inputdevice_tablet_strobe (void);
