 /*
  * UAE - The Un*x Amiga Emulator
  *
  * CIA chip support
  *
  * Copyright 1995 Bernd Schmidt, Alessandro Bissacco
  * Copyright 1996, 1997 Stefan Reinauer, Christian Schmitt
  */

#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cia.h"
#include "disk.h"
#include "xwin.h"
#include "keybuf.h"
#include "gui.h"
#include "savestate.h"
#include "inputdevice.h"
#include "zfile.h"
#include "akiko.h"
#include "audio.h"
#include "keyboard.h"
#include "uae.h"
#include "autoconf.h"
#include "drawing.h"

#define TOD_HACK

/* e-clock is 10 CPU cycles, 4 cycles high, 6 low
 * data transfer happens during 4 high cycles
 */

#define ECLOCK_DATA_CYCLE 4
#define ECLOCK_WAIT_CYCLE 6

#define DIV10 ((ECLOCK_DATA_CYCLE + ECLOCK_WAIT_CYCLE) * CYCLE_UNIT / 2) /* Yes, a bad identifier. */
#define CIASTARTCYCLESHI 3
#define CIASTARTCYCLESCRA 2

static unsigned int ciaaicr, ciaaimask, ciabicr, ciabimask;
static unsigned int ciaacra, ciaacrb, ciabcra, ciabcrb;
static unsigned int ciaastarta, ciaastartb, ciabstarta, ciabstartb;

/* Values of the CIA timers.  */
static unsigned long ciaata, ciaatb, ciabta, ciabtb;
/* Computed by compute_passed_time.  */
static unsigned long ciaata_passed, ciaatb_passed, ciabta_passed, ciabtb_passed;

static unsigned long ciaatod, ciabtod, ciaatol, ciabtol, ciaaalarm, ciabalarm;
static int ciaatlatch, ciabtlatch;
static bool oldovl, oldcd32mute;
static bool led;

static unsigned int ciabpra;

static unsigned long ciaala, ciaalb, ciabla, ciablb;
static int ciaatodon, ciabtodon;
static unsigned int ciaapra, ciaaprb, ciaadra, ciaadrb, ciaasdr, ciaasdr_cnt;
static unsigned int ciabprb, ciabdra, ciabdrb, ciabsdr, ciabsdr_cnt;
static int div10;
static int kbstate, kblostsynccnt;
static uae_u8 kbcode;

STATIC_INLINE void setclr (unsigned int *p, unsigned int val)
{
  if (val & 0x80) {
  	*p |= val & 0x7F;
  } else {
  	*p &= ~val;
  }
}

STATIC_INLINE void ICR (uae_u32 data)
{
	INTREQ_0 (0x8000 | data);
}

STATIC_INLINE void ICRA(uae_u32 data)
{
	ciaaicr |= 0x40;
	ciaaicr |= 0x20;
	ICR (0x0008);
}

STATIC_INLINE void ICRB(uae_u32 data)
{
	ciabicr |= 0x40;
	ciabicr |= 0x20;
	ICR (0x2000);
}

STATIC_INLINE void RethinkICRA (void)
{
  if (ciaaicr & ciaaimask) {
		if (!(ciaaicr & 0x80)) {
			ciaaicr |= 0x80;
      ICRA (0x0008);
    }
  }
}

STATIC_INLINE void RethinkICRB (void)
{
  if (ciabicr & ciabimask) {
		if (!(ciabicr & 0x80)) {
			ciabicr |= 0x80;
  	  ICRB (0);
    }
  }
}

void rethink_cias (void)
{
	if (ciaaicr & 0x40)
		ICRA (0);
	if (ciabicr & 0x40)
		ICRB (0);
}

/* Figure out how many CIA timer cycles have passed for each timer since the
   last call of CIA_calctimers.  */

static void compute_passed_time (void)
{
  unsigned long int ccount = (get_cycles () - eventtab[ev_cia].oldcycles + div10);
  unsigned long int ciaclocks = ccount / DIV10;

  ciaata_passed = ciaatb_passed = ciabta_passed = ciabtb_passed = 0;

  /* CIA A timers */
  if ((ciaacra & 0x21) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciaastarta)
			cc -= ciaastarta;
		else
			cc = 0;
		ciaata_passed = cc;
  }
  if ((ciaacrb & 0x61) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciaastartb)
			cc -= ciaastartb;
		else
			cc = 0;
		ciaatb_passed = cc;
  }

  /* CIA B timers */
  if ((ciabcra & 0x21) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciabstarta)
			cc -= ciabstarta;
		else
			cc = 0;
		ciabta_passed = cc;
  }
  if ((ciabcrb & 0x61) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciabstartb)
			cc -= ciabstartb;
		else
			cc = 0;
		ciabtb_passed = cc;
  }
}

/* Called to advance all CIA timers to the current time.  This expects that
   one of the timer values will be modified, and CIA_calctimers will be called
   in the same cycle.  */

static int CIA_update_check (void)
{
  unsigned long int ccount = (get_cycles () - eventtab[ev_cia].oldcycles + div10);
  unsigned long int ciaclocks = ccount / DIV10;

  int aovfla = 0, aovflb = 0, asp = 0, bovfla = 0, bovflb = 0, bsp = 0;
	int icr = 0;

  div10 = ccount % DIV10;

  /* CIA A timers */
  if ((ciaacra & 0x21) == 0x01) {
		bool check = true;
		unsigned long int cc = ciaclocks;
		if (ciaastarta > 0) {
			if (cc > ciaastarta) {
				cc -= ciaastarta;
				ciaastarta = 0;
			} else {
				ciaastarta -= cc;
				check = false;
			}
		}
		if (check) {
			if ((ciaata + 1) == cc) {
	      if ((ciaacra & 0x48) == 0x40 && ciaasdr_cnt > 0 && --ciaasdr_cnt == 0)
	        asp = 1;
	      aovfla = 1;
        if ((ciaacrb & 0x61) == 0x41 || (ciaacrb & 0x61) == 0x61) {
	        if (ciaatb-- == 0)
	          aovflb = 1;
        }
      }
			ciaata -= cc;
		}
  }
  if ((ciaacrb & 0x61) == 0x01) {
		bool check = true;
		unsigned long int cc = ciaclocks;
		if (ciaastartb > 0) {
			if (cc > ciaastartb) {
				cc -= ciaastartb;
				ciaastartb = 0;
			} else {
				ciaastartb -= cc;
				check = false;
			}
		}
		if (check) {
			if ((ciaatb + 1) == cc)
      aovflb = 1;
			ciaatb -= cc;
		}
  }

  /* CIA B timers */
  if ((ciabcra & 0x21) == 0x01) {
		bool check = true;
		unsigned long int cc = ciaclocks;
		if (ciabstarta > 0) {
			if (cc > ciabstarta) {
				cc -= ciabstarta;
				ciabstarta = 0;
			} else {
				ciabstarta -= cc;
				check = false;
			}
		}
		if (check) {
			if ((ciabta + 1) == cc) {
	      if ((ciabcra & 0x48) == 0x40 && ciabsdr_cnt > 0 && --ciabsdr_cnt == 0)
	        bsp = 1;
	      bovfla = 1;
        if ((ciabcrb & 0x61) == 0x41 || (ciabcrb & 0x61) == 0x61) {
	        if (ciabtb-- == 0)
	          bovflb = 1;
        }
      }
			ciabta -= cc;
		}
  }
  if ((ciabcrb & 0x61) == 0x01) {
		bool check = true;
		unsigned long int cc = ciaclocks;
		if (ciabstartb > 0) {
			if (cc > ciabstartb) {
				cc -= ciabstartb;
				ciabstartb = 0;
			} else {
				ciabstartb -= cc;
				check = false;
			}
		}
		if (check) {
			if ((ciabtb + 1) == cc)
      bovflb = 1;
			ciabtb -= cc;
		}
  }

  if (aovfla) {
		ciaaicr |= 1; icr = 1;
    ciaata = ciaala;
    if (ciaacra & 0x8) {
      ciaacra &= ~1;
    }
  }
  if (aovflb) {
		ciaaicr |= 2; icr = 1;
    ciaatb = ciaalb;
    if (ciaacrb & 0x8) {
      ciaacrb &= ~1;
    }
  }
  if (asp) {
		ciaaicr |= 8; icr = 1;
  }
  if (bovfla) {
		ciabicr |= 1; icr |= 2;
    ciabta = ciabla;
    if (ciabcra & 0x8) {
      ciabcra &= ~1;
    }
  }
  if (bovflb) {
		ciabicr |= 2; icr |= 2;
    ciabtb = ciablb;
    if (ciabcrb & 0x8) {
      ciabcrb &= ~1;
    }
  }
  if (bsp) {
		ciabicr |= 8; icr |= 2;
  }
	return icr;
}
static void CIA_update (void)
{
	int icr = CIA_update_check ();
	if (icr & 1)
		RethinkICRA ();
	if (icr & 2)
		RethinkICRB ();
}

/* Call this only after CIA_update has been called in the same cycle.  */

static void CIA_calctimers (void)
{
  long int ciaatimea = -1, ciaatimeb = -1, ciabtimea = -1, ciabtimeb = -1;
	int div10diff = DIV10 - div10;

  eventtab[ev_cia].oldcycles = get_cycles ();
  if ((ciaacra & 0x21) == 0x01) {
		ciaatimea = div10diff + DIV10 * (ciaata + ciaastarta);
  }
  if ((ciaacrb & 0x61) == 0x01) {
		ciaatimeb = div10diff + DIV10 * (ciaatb + ciaastartb);
  }

  if ((ciabcra & 0x21) == 0x01) {
		ciabtimea = div10diff + DIV10 * (ciabta + ciabstarta);
  }
  if ((ciabcrb & 0x61) == 0x01) {
		ciabtimeb = div10diff + DIV10 * (ciabtb + ciabstartb);
  }
  eventtab[ev_cia].active = (ciaatimea != -1 || ciaatimeb != -1
	  || ciabtimea != -1 || ciabtimeb != -1);
  if (eventtab[ev_cia].active) {
	  unsigned long int ciatime = ~0L;
	  if (ciaatimea != -1) 
      ciatime = ciaatimea;
	  if (ciaatimeb != -1 && ciaatimeb < ciatime) 
      ciatime = ciaatimeb;
	  if (ciabtimea != -1 && ciabtimea < ciatime) 
      ciatime = ciabtimea;
	  if (ciabtimeb != -1 && ciabtimeb < ciatime) 
      ciatime = ciabtimeb;
  	eventtab[ev_cia].evtime = ciatime + get_cycles ();
  }
  events_schedule();
}

void CIA_handler (void)
{
  CIA_update ();
  CIA_calctimers ();
}

void cia_diskindex (void)
{
  ciabicr |= 0x10;
  RethinkICRB();
}

static bool checkalarm (unsigned long tod, unsigned long alarm, bool inc)
{
  if (tod == alarm)
  	return true;
	if (aga_mode) // A1200 has no TOD bug
		return false;
  if (!inc)
  	return false;
  /* emulate buggy TODMED counter.
   * it counts: .. 29 2A 2B 2C 2D 2E 2F 20 30 31 32 ..
   * (2F->20->30 only takes couple of cycles but it will trigger alarm..
   */
  if (tod & 0x000fff)
  	return false;
  if (((tod - 1) & 0xfff000) == alarm)
  	return true;
  return false;
}

STATIC_INLINE void ciab_checkalarm (bool inc)
{
	// hack: do not trigger alarm interrupt if KS code and both
	// tod and alarm == 0. This incorrectly triggers on non-cycle exact
	// modes. Real hardware value written to ciabtod by KS is always
	// at least 1 or larger due to bus cycle delays when reading
	// old value.
#if 1
	if ((munge24 (m68k_getpc ()) & 0xFFF80000) != 0xF80000) {
		if (ciabtod == 0 && ciabalarm == 0)
			return;
	}
#endif
  if (checkalarm (ciabtod, ciabalarm, inc)) {
    ciabicr |= 4;
    RethinkICRB ();
  }
}

STATIC_INLINE void ciaa_checkalarm (bool inc)
{
  if (checkalarm (ciaatod, ciaaalarm, inc)) {
	  ciaaicr |= 4;
	  RethinkICRA ();
  }
}

#ifdef TOD_HACK
static int tod_hack, tod_hack_delay;
static void tod_hack_reset (void)
{
  struct timeval tv;
  uae_u32 rate = vblank_hz;
  gettimeofday (&tv, NULL);
  tod_hack = (uae_u32)(((uae_u64)tv.tv_sec) * rate  + tv.tv_usec / (1000000 / rate));
  tod_hack -= ciaatod;
  tod_hack_delay = 10 * 50;
}
#endif

static void setcode (uae_u8 keycode)
{
  kbcode = ~((keycode << 1) | (keycode >> 7));
}

static void keyreq (void)
{
	ciaasdr = kbcode;
	kblostsynccnt = 8 * maxvpos * 8; // 8 frames * 8 bits.
	ciaaicr |= 8;
	RethinkICRA ();
}

void CIA_hsync_posthandler (void)
{
  if (ciabtodon) {
	  ciabtod++;
	  ciabtod &= 0xFFFFFF;
    ciab_checkalarm (1);
  }

  if ((keys_available() || kbstate < 3) && !kblostsynccnt && (hsync_counter & 15) == 0) {
    switch (kbstate) 
    {
      case 0:
		    kbcode = 0; /* powerup resync */
		    kbstate++;
		    break;
      case 1:
		    setcode (AK_INIT_POWERUP);
		    kbstate++;
		    break;
	    case 2:
		    setcode (AK_TERM_POWERUP);
		    kbstate++;
		    break;
	    case 3:
		    kbcode = ~get_next_key ();
		    break;
    }
		keyreq ();
	}
}


void CIA_vsync_prehandler (void)
{
	CIA_handler ();
	if (kblostsynccnt > 0) {
		kblostsynccnt -= maxvpos;
		if (kblostsynccnt <= 0) {
			kblostsynccnt = 0;
			keyreq ();
		}
	}
}

void CIA_vsync_posthandler (void)
{
#ifdef TOD_HACK
  if (currprefs.tod_hack && ciaatodon) {
    struct timeval tv;
	  uae_u32 t, nt, rate = vblank_hz;
  	if (tod_hack_delay > 0) {
	    tod_hack_delay--;
	    if (tod_hack_delay == 0) {
    		tod_hack_reset ();
    		tod_hack_delay = 0;
    		write_log (_T("TOD_HACK re-initialized CIATOD=%06X\n"), ciaatod);
	    }
  	}
  	if (tod_hack_delay == 0) {
    	gettimeofday (&tv, NULL);
    	t = (uae_u32)(((uae_u64)tv.tv_sec) * rate + tv.tv_usec / (1000000 / rate));
    	nt = t - tod_hack;
    	if ((nt < ciaatod && ciaatod - nt < 10) || nt == ciaatod)
  	    return; /* try not to count backwards */
    	ciaatod = nt;
    	ciaatod &= 0xffffff;
    	ciaa_checkalarm (0);
	    return;
    }
  }
#endif
  if (ciaatodon) {
  	ciaatod++;
    ciaatod &= 0xFFFFFF;
  	ciaa_checkalarm (1);
  }
}

STATIC_INLINE void check_led (void)
{
  uae_u8 v = ciaapra;
  bool led2;

  v |= ~ciaadra; /* output is high when pin's direction is input */
  led2 = (v & 2) ? 0 : 1;
  if (led2 != led) {
    led = led2;
    gui_data.powerled = led2;
    led_filter_audio();
  }
}

static void bfe001_change (void)
{
  uae_u8 v = ciaapra;
	check_led ();
  if (!(aga_mode) && (v & 1) != oldovl) {
    oldovl = v & 1;
    if (!oldovl) {
      map_overlay (1);
    } else {
      map_overlay (0);
    }
  }
	if (currprefs.cs_cd32cd && (v & 1) != oldcd32mute) {
		oldcd32mute = v & 1;
		akiko_mute (oldcd32mute ? 0 : 1);
	}
}

static uae_u8 ReadCIAA (unsigned int addr)
{
  unsigned int tmp;
  int reg = addr & 15;

  compute_passed_time ();

  switch (reg) {
  case 0:
	  tmp = DISK_status() & 0x3c;
    tmp |= handle_joystick_buttons (ciaapra, ciaadra);	
	  tmp |= (ciaapra | (ciaadra ^ 3)) & 0x03;
	  return tmp;
  case 1:
#ifdef INPUTDEVICE_SIMPLE
    tmp = (ciaaprb & ciaadrb) | (ciaadrb ^ 0xff);
#else
		tmp = handle_parport_joystick (0, ciaaprb, ciaadrb);
#endif
	  if (ciaacrb & 2) {
	    int pb7 = 0;
	    if (ciaacrb & 4)
    		pb7 = ciaacrb & 1;
	    tmp &= ~0x80;
	    tmp |= pb7 ? 0x80 : 00;
  	}
  	if (ciaacra & 2) {
	    int pb6 = 0;
	    if (ciaacra & 4)
    		pb6 = ciaacra & 1;
	    tmp &= ~0x40;
	    tmp |= pb6 ? 0x40 : 00;
  	}
  	return tmp;
  case 2:
  	return ciaadra;
  case 3:
  	return ciaadrb;
  case 4:
  	return (uae_u8)((ciaata - ciaata_passed) & 0xff);
  case 5:
  	return (uae_u8)((ciaata - ciaata_passed) >> 8);
  case 6:
  	return (uae_u8)((ciaatb - ciaatb_passed) & 0xff);
  case 7:
  	return (uae_u8)((ciaatb - ciaatb_passed) >> 8);
  case 8:
  	if (ciaatlatch) {
	    ciaatlatch = 0;
	    return (uae_u8)ciaatol;
  	} else
	    return (uae_u8)ciaatod;
  case 9:
  	if (ciaatlatch)
	    return (uae_u8)(ciaatol >> 8);
  	else
	    return (uae_u8)(ciaatod >> 8);
  case 10:
  	if (!ciaatlatch) { /* only if not already latched. A1200 confirmed. (TW) */
      /* no latching if ALARM is set */
      if (!(ciaacrb & 0x80))
    	  ciaatlatch = 1;
    	ciaatol = ciaatod;
  	}
  	return (uae_u8)(ciaatol >> 16);
  case 12:
  	return ciaasdr;
  case 13:
		tmp = ciaaicr & ~(0x40 | 0x20);
		ciaaicr = 0;
	  return tmp;
  case 14:
  	return ciaacra;
  case 15:
  	return ciaacrb;
  }
  return 0;
}

static uae_u8 ReadCIAB (unsigned int addr)
{
  unsigned int tmp;
  int reg = addr & 15;

  compute_passed_time ();

  switch (reg) {
  case 0:
		tmp = 0;
#ifdef INPUTDEVICE_SIMPLE
		tmp = ((ciabpra & ciabdra) | (ciabdra ^ 0xff)) & 0x7;
#else
		tmp |= handle_parport_joystick (1, ciabpra, ciabdra);
#endif
		return tmp;
  case 1:
	  tmp = ciabprb;
	  if (ciabcrb & 2) {
	    int pb7 = 0;
	    if (ciabcrb & 4)
    		pb7 = ciabcrb & 1;
	    tmp &= ~0x80;
	    tmp |= pb7 ? 0x80 : 00;
  	}
  	if (ciabcra & 2) {
	    int pb6 = 0;
	    if (ciabcra & 4)
    		pb6 = ciabcra & 1;
	    tmp &= ~0x40;
	    tmp |= pb6 ? 0x40 : 00;
  	}
  	return tmp;
  case 2:
  	return ciabdra;
  case 3:
  	return ciabdrb;
  case 4:
  	return (uae_u8)((ciabta - ciabta_passed) & 0xff);
  case 5:
  	return (uae_u8)((ciabta - ciabta_passed) >> 8);
  case 6:
  	return (uae_u8)((ciabtb - ciabtb_passed) & 0xff);
  case 7:
  	return (uae_u8)((ciabtb - ciabtb_passed) >> 8);
  case 8:
  	if (ciabtlatch) {
	    ciabtlatch = 0;
	    return (uae_u8)ciabtol;
  	} else
	    return (uae_u8)ciabtod;
  case 9:
  	if (ciabtlatch)
	    return (uae_u8)(ciabtol >> 8);
  	else
	    return (uae_u8)(ciabtod >> 8);
  case 10:
  	if (!ciabtlatch) {
      /* no latching if ALARM is set */
      if (!(ciabcrb & 0x80))
    	  ciabtlatch = 1;
  	  ciabtol = ciabtod;
  	}
  	return (uae_u8)(ciabtol >> 16);
  case 12:
  	return ciabsdr;
  case 13:
		tmp = ciabicr & ~(0x40 | 0x20);
		ciabicr = 0;
	  return tmp;
  case 14:
  	return ciabcra;
  case 15:
  	return ciabcrb;
  }
  return 0;
}

static void WriteCIAA (uae_u16 addr,uae_u8 val)
{
	int reg = addr & 15;

  if ((aga_mode) && oldovl) {
  	map_overlay (1);
  	oldovl = 0;
  }
  switch (reg) {
  case 0:
  	ciaapra = (ciaapra & ~0xc3) | (val & 0xc3);
  	bfe001_change ();
		handle_cd32_joystick_cia (ciaapra, ciaadra);
  	break;
  case 1:
	  ciaaprb = val;
	  break;
  case 2:
	  ciaadra = val; 
    bfe001_change (); 
    break;
  case 3:
	  ciaadrb = val; 
	  break;
  case 4:
	  CIA_update ();
	  ciaala = (ciaala & 0xff00) | val;
	  CIA_calctimers ();
	  break;
  case 5:
	  CIA_update ();
	  ciaala = (ciaala & 0xff) | (val << 8);
	  if ((ciaacra & 1) == 0)
	    ciaata = ciaala;
  	if (ciaacra & 8) {
	    ciaata = ciaala;
	    ciaacra |= 1;
			ciaastarta = CIASTARTCYCLESHI;
  	}
  	CIA_calctimers ();
  	break;
  case 6:
	  CIA_update ();
	  ciaalb = (ciaalb & 0xff00) | val;
	  CIA_calctimers ();
	  break;
  case 7:
	  CIA_update ();
	  ciaalb = (ciaalb & 0xff) | (val << 8);
	  if ((ciaacrb & 1) == 0)
	    ciaatb = ciaalb;
  	if (ciaacrb & 8) {
	    ciaatb = ciaalb;
	    ciaacrb |= 1;
			ciaastartb = CIASTARTCYCLESHI;
	  }
	  CIA_calctimers ();
	  break;
  case 8:
  	if (ciaacrb & 0x80) {
	    ciaaalarm = (ciaaalarm & ~0xff) | val;
  	} else {
	    ciaatod = (ciaatod & ~0xff) | val;
	    ciaatodon = 1;
	    ciaa_checkalarm (false);
#ifdef TOD_HACK
      if (currprefs.tod_hack)
		    tod_hack_reset ();
#endif
	  }
	  break;
  case 9:
  	if (ciaacrb & 0x80) {
	    ciaaalarm = (ciaaalarm & ~0xff00) | (val << 8);
  	} else {
	    ciaatod = (ciaatod & ~0xff00) | (val << 8);
  	}
  	break;
  case 10:
  	if (ciaacrb & 0x80) {
	    ciaaalarm = (ciaaalarm & ~0xff0000) | (val << 16);
  	} else {
	    ciaatod = (ciaatod & ~0xff0000) | (val << 16);
	    ciaatodon = 0;
  	}
  	break;
  case 12:
	  CIA_update ();
	  ciaasdr = val;
	  if ((ciaacra & 0x41) == 0x41 && ciaasdr_cnt == 0)
	  ciaasdr_cnt = 8 * 2;
	  CIA_calctimers ();
	  break;
  case 13:
	  setclr(&ciaaimask,val);
		RethinkICRA ();
	  break;
  case 14:
	  CIA_update ();
	  val &= 0x7f; /* bit 7 is unused */
	  if ((val & 1) && !(ciaacra & 1))
		  ciaastarta = CIASTARTCYCLESCRA;
	  if ((val & 0x40) == 0 && (ciaacra & 0x40) != 0) {
		  /* todo: check if low to high or high to low only */
		  kblostsynccnt = 0;
	  }
	  ciaacra = val;
	  if (ciaacra & 0x10) {
      ciaacra &= ~0x10;
      ciaata = ciaala;
	  }
	  CIA_calctimers ();
	  break;
  case 15:
	  CIA_update ();
	  if ((val & 1) && !(ciaacrb & 1))
		  ciaastartb = CIASTARTCYCLESCRA;
	  ciaacrb = val;
	  if (ciaacrb & 0x10) {
	    ciaacrb &= ~0x10;
	    ciaatb = ciaalb;
  	}
  	CIA_calctimers ();
  	break;
  }
}

static void WriteCIAB (uae_u16 addr,uae_u8 val)
{
	int reg = addr & 15;

  switch (reg) {
  case 0:
    ciabpra  = val;
  	break;
  case 1:
	  ciabprb = val; 
    DISK_select(val); 
    break;
  case 2:
	  ciabdra = val;
	  break;
  case 3:
	  ciabdrb = val; 
    break;
  case 4:
	  CIA_update ();
	  ciabla = (ciabla & 0xff00) | val;
	  CIA_calctimers ();
	  break;
  case 5:
	  CIA_update ();
	  ciabla = (ciabla & 0xff) | (val << 8);
	  if ((ciabcra & 1) == 0)
	    ciabta = ciabla;
  	if (ciabcra & 8) {
	    ciabta = ciabla;
	    ciabcra |= 1;
			ciabstarta = CIASTARTCYCLESHI;
  	}
  	CIA_calctimers ();
  	break;
  case 6:
	  CIA_update ();
	  ciablb = (ciablb & 0xff00) | val;
	  CIA_calctimers ();
	  break;
  case 7:
	  CIA_update ();
	  ciablb = (ciablb & 0xff) | (val << 8);
	  if ((ciabcrb & 1) == 0)
	    ciabtb = ciablb;
  	if (ciabcrb & 8) {
	    ciabtb = ciablb;
	    ciabcrb |= 1;
			ciabstartb = CIASTARTCYCLESHI;
  	}
  	CIA_calctimers ();
  	break;
  case 8:
  	if (ciabcrb & 0x80) {
	    ciabalarm = (ciabalarm & ~0xff) | val;
  	} else {
	    ciabtod = (ciabtod & ~0xff) | val;
	    ciabtodon = 1;
	    ciab_checkalarm (false);
  	}
  	break;
  case 9:
  	if (ciabcrb & 0x80) {
	    ciabalarm = (ciabalarm & ~0xff00) | (val << 8);
  	} else {
	    ciabtod = (ciabtod & ~0xff00) | (val << 8);
  	}
  	break;
  case 10:
  	if (ciabcrb & 0x80) {
	    ciabalarm = (ciabalarm & ~0xff0000) | (val << 16);
  	} else {
	    ciabtod = (ciabtod & ~0xff0000) | (val << 16);
	    ciabtodon = 0;
  	}
  	break;
  case 12:
	  CIA_update ();
   	ciabsdr = val;
	  if ((ciabcra & 0x40) == 0)
	    ciabsdr_cnt = 0;
	  if ((ciabcra & 0x41) == 0x41 && ciabsdr_cnt == 0)
	    ciabsdr_cnt = 8 * 2;
  	CIA_calctimers ();
   	break;
  case 13:
	  setclr(&ciabimask,val);
		RethinkICRB ();
	  break;
  case 14:
	  CIA_update ();
	  val &= 0x7f; /* bit 7 is unused */
	  if ((val & 1) && !(ciabcra & 1))
		  ciabstarta = CIASTARTCYCLESCRA;
	  ciabcra = val;
	  if (ciabcra & 0x10) {
	    ciabcra &= ~0x10;
	    ciabta = ciabla;
	  }
	  CIA_calctimers ();
	  break;
  case 15:
	  CIA_update ();
	  if ((val & 1) && !(ciabcrb & 1))
		  ciabstartb = CIASTARTCYCLESCRA;
	  ciabcrb = val;
	  if (ciabcrb & 0x10) {
	    ciabcrb &= ~0x10;
	    ciabtb = ciablb;
	  }
	  CIA_calctimers ();
	  break;
  }
}

void cia_set_overlay (bool overlay)
{
	oldovl = overlay;
}

/* CIA memory access */

static uae_u32 REGPARAM3 cia_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 cia_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 cia_bget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 cia_bget_compatible (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 cia_lgeti (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 cia_wgeti (uaecptr) REGPARAM;
static void REGPARAM3 cia_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 cia_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 cia_bput (uaecptr, uae_u32) REGPARAM;

addrbank cia_bank = {
  cia_lget, cia_wget, cia_bget,
  cia_lput, cia_wput, cia_bput,
	default_xlate, default_check, NULL, _T("CIA"),
  cia_lgeti, cia_wgeti, ABFLAG_IO, 0x3f01, 0xbfc000
};

void CIA_reset (void)
{
  if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible)
    cia_bank.bget = cia_bget_compatible;
  else
    cia_bank.bget = cia_bget;

#ifdef TOD_HACK
  tod_hack = 0;
  if (currprefs.tod_hack)
  	tod_hack_reset ();
#endif
  kblostsynccnt = 0;
	oldcd32mute = 1;

  if (!savestate_state) {
		oldovl = true;
    kbstate = 0;
    ciaatlatch = ciabtlatch = 0;
    ciaapra = 0;  ciaadra = 0;
    ciaatod = ciabtod = 0; ciaatodon = ciabtodon = 0;
    ciaaicr = ciabicr = ciaaimask = ciabimask = 0;
    ciaacra = ciaacrb = ciabcra = ciabcrb = 0x4; /* outmode = toggle; */
    ciaala = ciaalb = ciabla = ciablb = ciaata = ciaatb = ciabta = ciabtb = 0xFFFF;
	  ciaaalarm = ciabalarm = 0;
  	ciabpra = 0x8C; ciabdra = 0;
    div10 = 0;
  	ciaasdr_cnt = 0; ciaasdr = 0;
  	ciabsdr_cnt = 0; ciabsdr = 0;
    ciaata_passed = ciaatb_passed = ciabta_passed = ciabtb_passed = 0;
    ciaatol = ciabtol = ciaaprb = ciaadrb = ciabprb = ciabdrb = 0;
    CIA_calctimers ();
	  DISK_select_set (ciabprb);
  }
	map_overlay (0);
	check_led ();
  if (savestate_state) {
		if (!(aga_mode)) {
			oldovl = true;
		}
    bfe001_change ();
		if (aga_mode) {
			map_overlay (oldovl ? 0 : 1);
		}
  }
#ifdef CD32
	if (!isrestore ()) {
		akiko_reset ();
		if (!akiko_init ())
			currprefs.cs_cd32cd = changed_prefs.cs_cd32cd = 0;
	}
#endif
}

static void cia_wait_pre (void)
{
	if (currprefs.cachesize)
		return;

  int div = (get_cycles () - eventtab[ev_cia].oldcycles) % DIV10;
  int cycles;

  if (div >= DIV10 * ECLOCK_DATA_CYCLE / 10) {
  	cycles = DIV10 - div;
	  cycles += DIV10 * ECLOCK_DATA_CYCLE / 10;
	} else if (div) {
		cycles = DIV10 + DIV10 * ECLOCK_DATA_CYCLE / 10 - div;
  } else {
	  cycles = DIV10 * ECLOCK_DATA_CYCLE / 10 - div;
  }
	if (cycles) {
  	do_cycles (cycles);
  }
}

static void cia_wait_post (uae_u32 value)
{
	if (currprefs.cachesize) {
		do_cycles (8 * CYCLE_UNIT / 2);
	} else {
		int c = 6 * CYCLE_UNIT / 2;
		do_cycles (c);
  }
}

static uae_u32 REGPARAM2 cia_bget_compatible (uaecptr addr)
{
	int r = (addr & 0xf00) >> 8;
  uae_u8 v;

#ifdef JIT
  special_mem |= S_READ;
#endif

  cia_wait_pre ();
  switch ((addr >> 12) & 3) {
  case 0:
	  v = (addr & 1) ? ReadCIAA (r) : ReadCIAB (r);
	  break;
  case 1:
	  v = (addr & 1) ? regs.irc : ReadCIAB (r);
	  break;
  case 2:
	  v = (addr & 1) ? ReadCIAA (r) : regs.irc >> 8;
	  break;
	case 3:
    v = (addr & 1) ? regs.irc : regs.irc >> 8;
  	break;
  }
	cia_wait_post (v);

  return v;
}

static uae_u32 REGPARAM2 cia_bget (uaecptr addr)
{
	int r = (addr & 0xf00) >> 8;
	uae_u8 v;

#ifdef JIT
  special_mem |= S_READ;
#endif

  switch ((addr >> 12) & 3) {
  case 0:
    cia_wait_pre ();
	  v = (addr & 1) ? ReadCIAA (r) : ReadCIAB (r);
  	cia_wait_post (v);
	  break;
  case 1:
    cia_wait_pre ();
	  v = (addr & 1) ? 0xff : ReadCIAB (r);
  	cia_wait_post (v);
	  break;
  case 2:
    cia_wait_pre ();
	  v = (addr & 1) ? ReadCIAA (r) : 0xff;
  	cia_wait_post (v);
	  break;
	case 3:
    v = 0xff;
  	break;
  }

  return v;
}

static uae_u32 REGPARAM2 cia_wget (uaecptr addr)
{
	int r = (addr & 0xf00) >> 8;
	uae_u16 v;

#ifdef JIT
  special_mem |= S_READ;
#endif

  cia_wait_pre ();
  switch ((addr >> 12) & 3)
  {
  case 0:
	  v = (ReadCIAB (r) << 8) | ReadCIAA (r);
	  break;
  case 1:
	  v = (ReadCIAB (r) << 8) | 0xff;
	  break;
  case 2:
    v = (0xff << 8) | ReadCIAA (r);
    break;
	case 3:
  	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible)
	    v = regs.irc;
    else
      v = 0xffff;
  	break;
  }
	cia_wait_post (v);
  return v;
}

static uae_u32 REGPARAM2 cia_lget (uaecptr addr)
{
  uae_u32 v;
  v = cia_wget (addr) << 16;
  v |= cia_wget (addr + 2);
  return v;
}

static uae_u32 REGPARAM2 cia_wgeti (uaecptr addr)
{
  if (currprefs.cpu_model >= 68020)
  	return dummy_wgeti(addr);
  return cia_wget(addr);
}
static uae_u32 REGPARAM2 cia_lgeti (uaecptr addr)
{
  if (currprefs.cpu_model >= 68020)
  	return dummy_lgeti(addr);
  return cia_lget(addr);
}

static void REGPARAM2 cia_bput (uaecptr addr, uae_u32 value)
{
  value &= 0xFF;
  int r = (addr & 0xf00) >> 8;

#ifdef JIT
  special_mem |= S_WRITE;
#endif

  cia_wait_pre ();
  if ((addr & 0x2000) == 0)
  	WriteCIAB (r, value);
  if ((addr & 0x1000) == 0)
  	WriteCIAA (r, value);
  cia_wait_post (value);
}

static void REGPARAM2 cia_wput (uaecptr addr, uae_u32 value)
{
  value &= 0xFFFF;
  int r = (addr & 0xf00) >> 8;

#ifdef JIT
  special_mem |= S_WRITE;
#endif

  cia_wait_pre ();
  if ((addr & 0x2000) == 0)
  	WriteCIAB (r, value >> 8);
  if ((addr & 0x1000) == 0)
  	WriteCIAA (r, value & 0xff);
  cia_wait_post (value);
}

static void REGPARAM2 cia_lput (uaecptr addr, uae_u32 value)
{
  cia_wput (addr, value >> 16);
  cia_wput (addr + 2, value & 0xffff);
}

/* battclock memory access */

static uae_u32 REGPARAM3 clock_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 clock_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 clock_bget (uaecptr) REGPARAM;
static void REGPARAM3 clock_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 clock_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 clock_bput (uaecptr, uae_u32) REGPARAM;

addrbank clock_bank = {
  clock_lget, clock_wget, clock_bget,
  clock_lput, clock_wput, clock_bput,
	default_xlate, default_check, NULL, _T("Battery backed up clock (none)"),
  dummy_lgeti, dummy_wgeti, ABFLAG_IO, 0x3f, 0xd80000
};

static unsigned int clock_control_d;
static unsigned int clock_control_e;
static unsigned int clock_control_f;

static uae_u8 getclockreg (int addr, struct tm *ct)
{
	uae_u8 v = 0;

  switch (addr) {
    case 0x0: v = ct->tm_sec % 10; break;
    case 0x1: v = ct->tm_sec / 10; break;
    case 0x2: v = ct->tm_min % 10; break;
    case 0x3: v = ct->tm_min / 10; break;
    case 0x4: v = ct->tm_hour % 10; break;
    case 0x5: 
			if (clock_control_f & 4) {
				v = ct->tm_hour / 10; // 24h
			} else {
				v = (ct->tm_hour % 12) / 10; // 12h
				v |= ct->tm_hour >= 12 ? 4 : 0; // AM/PM bit
			}
      break;
    case 0x6: v = ct->tm_mday % 10; break;
    case 0x7: v = ct->tm_mday / 10; break;
    case 0x8: v = (ct->tm_mon + 1) % 10; break;
    case 0x9: v = (ct->tm_mon + 1) / 10; break;
    case 0xA: v = ct->tm_year % 10; break;
    case 0xB: v = (ct->tm_year / 10) & 0x0f; break;
    case 0xC: v = ct->tm_wday; break;
    case 0xD: v = clock_control_d; break;
    case 0xE: v = clock_control_e; break;
    case 0xF: v = clock_control_f; break;
  }
	return v;
}

void rtc_hardreset(void)
{
	clock_bank.name = _T("Battery backed up clock (MSM6242B)");
  clock_control_d = 0x1;
  clock_control_e = 0;
  clock_control_f = 0x4; /* 24/12 */
}

static uae_u32 REGPARAM2 clock_lget (uaecptr addr)
{
  return (clock_wget (addr) << 16) | clock_wget (addr + 2);
}

static uae_u32 REGPARAM2 clock_wget (uaecptr addr)
{
  return (clock_bget (addr) << 8) | clock_bget (addr + 1);
}

static uae_u32 REGPARAM2 clock_bget (uaecptr addr)
{
  time_t t;
  struct tm *ct;
	uae_u8 v = 0;

#ifdef JIT
  special_mem |= S_READ;
#endif
  addr &= 0x3f;
  if ((addr & 3) == 2 || (addr & 3) == 0) {
	  if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible)
	    v = regs.irc >> 8;
	  return v;
  }
  t = time(0);
  ct = localtime (&t);
  addr >>= 2;
	return getclockreg (addr, ct);
}

static void REGPARAM2 clock_lput (uaecptr addr, uae_u32 value)
{
  clock_wput (addr, value >> 16);
  clock_wput (addr + 2, value);
}

static void REGPARAM2 clock_wput (uaecptr addr, uae_u32 value)
{
  clock_bput (addr, value >> 8);
  clock_bput (addr + 1, value);
}

static void REGPARAM2 clock_bput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
  addr &= 0x3f;
  if ((addr & 1) != 1)
  	return;
  addr >>= 2;
  value &= 0x0f;
  switch (addr) 
  {
    case 0xD: clock_control_d = value & (1|8); break;
    case 0xE: clock_control_e = value; break;
    case 0xF: clock_control_f = value; break;
  }
}

#ifdef SAVESTATE

/* CIA-A and CIA-B save/restore code */

static void save_cia_prepare (void)
{
	CIA_update_check ();
	CIA_calctimers ();
	compute_passed_time ();
}

void restore_cia_start (void)
{
	/* Fixes very old statefiles without keyboard state */
	kbstate = 3;
	kblostsynccnt = 0;
}

void restore_cia_finish (void)
{
	eventtab[ev_cia].oldcycles = get_cycles ();
	CIA_update ();
	CIA_calctimers ();
	compute_passed_time ();
	eventtab[ev_cia].oldcycles -= div10;
	DISK_select_set (ciabprb);
}

uae_u8 *restore_cia (int num, uae_u8 *src)
{
  uae_u8 b;
  uae_u16 w;
  uae_u32 l;

  /* CIA registers */
  b = restore_u8 ();					/* 0 PRA */
  if (num) ciabpra = b; else ciaapra = b;
  b = restore_u8 ();					/* 1 PRB */
  if (num) ciabprb = b; else ciaaprb = b;
  b = restore_u8 ();					/* 2 DDRA */
  if (num) ciabdra = b; else ciaadra = b;
  b = restore_u8 ();					/* 3 DDRB */
  if (num) ciabdrb = b; else ciaadrb = b;
  w = restore_u16 ();					/* 4 TA */
  if (num) ciabta = w; else ciaata = w;
  w = restore_u16 ();					/* 6 TB */
  if (num) ciabtb = w; else ciaatb = w;
  l = restore_u8 ();					/* 8/9/A TOD */
  l |= restore_u8 () << 8;
  l |= restore_u8 () << 16;
  if (num) ciabtod = l; else ciaatod = l;
  restore_u8 ();						/* B unused */
  b = restore_u8 ();					/* C SDR */
  if (num) ciabsdr = b; else ciaasdr = b;
  b = restore_u8 ();					/* D ICR INFORMATION (not mask!) */
  if (num) ciabicr = b; else ciaaicr = b;
  b = restore_u8 ();					/* E CRA */
  if (num) ciabcra = b; else ciaacra = b;
  b = restore_u8 ();					/* F CRB */
  if (num) ciabcrb = b; else ciaacrb = b;

  /* CIA internal data */

  b = restore_u8 ();					/* ICR MASK */
  if (num) ciabimask = b; else ciaaimask = b;
  w = restore_u8 ();					/* timer A latch */
  w |= restore_u8 () << 8;
  if (num) ciabla = w; else ciaala = w;
  w = restore_u8 ();					/* timer B latch */
  w |= restore_u8 () << 8;
  if (num) ciablb = w; else ciaalb = w;
  w = restore_u8 ();					/* TOD latched value */
  w |= restore_u8 () << 8;
  w |= restore_u8 () << 16;
  if (num) ciabtol = w; else ciaatol = w;
  l = restore_u8 ();					/* alarm */
  l |= restore_u8 () << 8;
  l |= restore_u8 () << 16;
  if (num) ciabalarm = l; else ciaaalarm = l;
  b = restore_u8 ();
  if (num) ciabtlatch = b & 1; else ciaatlatch = b & 1;	/* is TOD latched? */
  if (num) ciabtodon = b & 2; else ciaatodon = b & 2;		/* is TOD stopped? */
  b = restore_u8 ();
  if (num)
    div10 = CYCLE_UNIT * b;
  b = restore_u8 ();
  if (num) ciabsdr_cnt = b; else ciaasdr_cnt = b;
  return src;
}

uae_u8 *save_cia (int num, int *len, uae_u8 *dstptr)
{
  uae_u8 *dstbak,*dst, b;
  uae_u16 t;

  if (dstptr)
    dstbak = dst = dstptr;
  else
	  dstbak = dst = xmalloc (uae_u8, 100);

  save_cia_prepare ();

  /* CIA registers */

  b = num ? ciabpra : ciaapra;				/* 0 PRA */
  save_u8 (b);
  b = num ? ciabprb : ciaaprb;				/* 1 PRB */
  save_u8 (b);
  b = num ? ciabdra : ciaadra;				/* 2 DDRA */
  save_u8 (b); 
  b = num ? ciabdrb : ciaadrb;				/* 3 DDRB */
  save_u8 (b);
  t = (num ? ciabta - ciabta_passed : ciaata - ciaata_passed);/* 4 TA */
  save_u16 (t);
  t = (num ? ciabtb - ciabtb_passed : ciaatb - ciaatb_passed);/* 6 TB */
  save_u16 (t);
  b = (num ? ciabtod : ciaatod);			/* 8 TODL */
  save_u8 (b);
  b = (num ? ciabtod >> 8 : ciaatod >> 8);		/* 9 TODM */
  save_u8 (b);
  b = (num ? ciabtod >> 16 : ciaatod >> 16);		/* A TODH */
  save_u8 (b);
  save_u8 (0);						/* B unused */
  b = num ? ciabsdr : ciaasdr;				/* C SDR */
  save_u8 (b);
  b = num ? ciabicr : ciaaicr;				/* D ICR INFORMATION (not mask!) */
  save_u8 (b);
  b = num ? ciabcra : ciaacra;				/* E CRA */
  save_u8 (b);
  b = num ? ciabcrb : ciaacrb;				/* F CRB */
  save_u8 (b);

  /* CIA internal data */

  save_u8 (num ? ciabimask : ciaaimask);			/* ICR */
  b = (num ? ciabla : ciaala);			/* timer A latch LO */
  save_u8 (b);
  b = (num ? ciabla >> 8 : ciaala >> 8);		/* timer A latch HI */
  save_u8 (b);
  b = (num ? ciablb : ciaalb);			/* timer B latch LO */
  save_u8 (b);
  b = (num ? ciablb >> 8 : ciaalb >> 8);		/* timer B latch HI */
  save_u8 (b);
  b = (num ? ciabtol : ciaatol);			/* latched TOD LO */
  save_u8 (b);
  b = (num ? ciabtol >> 8 : ciaatol >> 8);		/* latched TOD MED */
  save_u8 (b);
  b = (num ? ciabtol >> 16 : ciaatol >> 16);		/* latched TOD HI */
  save_u8 (b);
  b = (num ? ciabalarm : ciaaalarm);			/* alarm LO */
  save_u8 (b);
  b = (num ? ciabalarm >> 8 : ciaaalarm >>8 );	/* alarm MED */
  save_u8 (b);
  b = (num ? ciabalarm >> 16 : ciaaalarm >> 16);	/* alarm HI */
  save_u8 (b);
  b = 0;
  if (num)
  	b |= ciabtlatch ? 1 : 0;
  else
  	b |= ciaatlatch ? 1 : 0; /* is TOD latched? */
  if (num)
  	b |= ciabtodon ? 2 : 0;
  else
  	b |= ciaatodon ? 2 : 0;   /* TOD stopped? */
  save_u8 (b);
  save_u8 (num ? div10 / CYCLE_UNIT : 0);
  save_u8 (num ? ciabsdr_cnt : ciaasdr_cnt);
  *len = dst - dstbak;
  return dstbak;
}

uae_u8 *save_keyboard (int *len, uae_u8 *dstptr)
{
	uae_u8 *dst, *dstbak;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 4 + 4 + 1 + 1 + 1 + 1 + 1 + 2);
	save_u32 (1);
	save_u8 (kbstate);
	save_u8 (0);
	save_u8 (0);
	save_u8 (0);
	save_u8 (kbcode);
	save_u16 (kblostsynccnt);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_keyboard (uae_u8 *src)
{
	uae_u32 v = restore_u32 ();
	kbstate = restore_u8 ();
	restore_u8 ();
	restore_u8 ();
	restore_u8 ();
	kbcode = restore_u8 ();
	kblostsynccnt = restore_u16 ();
	if (!(v & 1)) {
		kbstate = 3;
		kblostsynccnt = 0;
	}
  return src;
}

#endif /* SAVESTATE */
