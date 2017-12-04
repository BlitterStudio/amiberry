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

#define CIAA_DEBUG_R 0
#define CIAA_DEBUG_W 0
#define CIAA_DEBUG_IRQ 0

#define CIAB_DEBUG_R 0
#define CIAB_DEBUG_W 0
#define CIAB_DEBUG_IRQ 0

#define DONGLE_DEBUG 0
#define KB_DEBUG 0
#define CLOCK_DEBUG 0

#define TOD_HACK

  /* Akiko internal CIA differences:

  - BFE101 and BFD100: reads 3F if data direction is in.

  */

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
static int led_old_brightness;
static unsigned long led_cycles_on, led_cycles_off, led_cycle;

unsigned int ciabpra;

static unsigned long ciaala, ciaalb, ciabla, ciablb;
static int ciaatodon, ciabtodon;
static unsigned int ciaapra, ciaaprb, ciaadra, ciaadrb, ciaasdr, ciaasdr_cnt;
static unsigned int ciabprb, ciabdra, ciabdrb, ciabsdr, ciabsdr_cnt;
static int div10;
static int kbstate, kblostsynccnt;
static uae_u8 kbcode;

static uae_u8 serbits;
static int warned = 10;
static int rtc_delayed_write;

static void setclr(unsigned int *p, unsigned int val)
{
	if (val & 0x80) {
		*p |= val & 0x7F;
	}
	else {
		*p &= ~val;
	}
}

/* delay interrupt after current CIA register access if
* interrupt would have triggered mid access
*/
static int cia_interrupt_disabled;
static int cia_interrupt_delay;

static void ICR(uae_u32 data)
{
	INTREQ_0(0x8000 | data);
}

static void ICRA(uae_u32 data)
{
	ciaaicr |= 0x40;
#if 1
	if (currprefs.cpu_memory_cycle_exact && !(ciaaicr & 0x20) && (cia_interrupt_disabled & 1)) {
		cia_interrupt_delay |= 1;
#if CIAB_DEBUG_IRQ
		write_log(_T("ciab interrupt disabled ICR=%02X PC=%x\n"), ciabicr, M68K_GETPC);
#endif
		return;
	}
#endif
	ciaaicr |= 0x20;
	ICR(0x0008);
}

static void ICRB(uae_u32 data)
{
	ciabicr |= 0x40;
#if 1
	if (currprefs.cpu_memory_cycle_exact && !(ciabicr & 0x20) && (cia_interrupt_disabled & 2)) {
		cia_interrupt_delay |= 2;
#if CIAB_DEBUG_IRQ
		write_log(_T("ciab interrupt disabled ICR=%02X PC=%x\n"), ciabicr, M68K_GETPC);
#endif
		return;
	}
#endif
	ciabicr |= 0x20;
	if (currprefs.cs_compatible == CP_VELVET) {
		// Both CIAs in Velvet are connected to level 2.
		ICR(0x0008);
	}
	else {
		ICR(0x2000);
	}
}

static void RethinkICRA(void)
{
	if (ciaaicr & ciaaimask) {
#if CIAA_DEBUG_IRQ
		write_log(_T("CIAA IRQ %02X\n"), ciaaicr);
#endif
		if (!(ciaaicr & 0x80)) {
			ciaaicr |= 0x80;
			if (currprefs.cpu_memory_cycle_exact) {
				event2_newevent_xx(-1, DIV10 + 2 * CYCLE_UNIT + CYCLE_UNIT / 2, 0, ICRA);
			}
			else {
				ICRA(0x0008);
			}
		}
	}
}

static void RethinkICRB(void)
{
	if (ciabicr & ciabimask) {
#if CIAB_DEBUG_IRQ
		write_log(_T("CIAB IRQ %02X\n"), ciabicr);
#endif
		if (!(ciabicr & 0x80)) {
			ciabicr |= 0x80;
			if (currprefs.cpu_memory_cycle_exact) {
				event2_newevent_xx(-1, DIV10 + 2 * CYCLE_UNIT + CYCLE_UNIT / 2, 0, ICRB);
			}
			else {
				ICRB(0);
			}
		}
	}
}

void rethink_cias(void)
{
	if (ciaaicr & 0x40)
		ICRA(0);
	if (ciabicr & 0x40)
		ICRB(0);
}

/* Figure out how many CIA timer cycles have passed for each timer since the
   last call of CIA_calctimers.  */

static void compute_passed_time(void)
{
	unsigned long int ccount = (get_cycles() - eventtab[ev_cia].oldcycles + div10);
	unsigned long int ciaclocks = ccount / DIV10;

	ciaata_passed = ciaatb_passed = ciabta_passed = ciabtb_passed = 0;

	/* CIA A timers */
	if ((ciaacra & 0x21) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciaastarta)
			cc -= ciaastarta;
		else
			cc = 0;
		assert((ciaata + 1) >= cc);
		ciaata_passed = cc;
	}
	if ((ciaacrb & 0x61) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciaastartb)
			cc -= ciaastartb;
		else
			cc = 0;
		assert((ciaatb + 1) >= cc);
		ciaatb_passed = cc;
	}

	/* CIA B timers */
	if ((ciabcra & 0x21) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciabstarta)
			cc -= ciabstarta;
		else
			cc = 0;
		assert((ciabta + 1) >= cc);
		ciabta_passed = cc;
	}
	if ((ciabcrb & 0x61) == 0x01) {
		unsigned long int cc = ciaclocks;
		if (cc > ciabstartb)
			cc -= ciabstartb;
		else
			cc = 0;
		assert((ciabtb + 1) >= cc);
		ciabtb_passed = cc;
	}
}

/* Called to advance all CIA timers to the current time.  This expects that
   one of the timer values will be modified, and CIA_calctimers will be called
   in the same cycle.  */

static int CIA_update_check(void)
{
	unsigned long int ccount = (get_cycles() - eventtab[ev_cia].oldcycles + div10);
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
			}
			else {
				ciaastarta -= cc;
				check = false;
			}
		}
		if (check) {
			assert((ciaata + 1) >= cc);
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
			}
			else {
				ciaastartb -= cc;
				check = false;
			}
		}
		if (check) {
			assert((ciaatb + 1) >= cc);
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
			}
			else {
				ciabstarta -= cc;
				check = false;
			}
		}
		if (check) {
			assert((ciabta + 1) >= cc);
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
			}
			else {
				ciabstartb -= cc;
				check = false;
			}
		}
		if (check) {
			assert((ciabtb + 1) >= cc);
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
static void CIA_update(void)
{
	int icr = CIA_update_check();
	if (icr & 1)
		RethinkICRA();
	if (icr & 2)
		RethinkICRB();
}

/* Call this only after CIA_update has been called in the same cycle.  */

static void CIA_calctimers(void)
{
	long int ciaatimea = -1, ciaatimeb = -1, ciabtimea = -1, ciabtimeb = -1;
	int div10diff = DIV10 - div10;

	eventtab[ev_cia].oldcycles = get_cycles();

	if ((ciaacra & 0x21) == 0x01) {
		ciaatimea = div10diff + DIV10 * (ciaata + ciaastarta);
	}
#if 0
	if ((ciaacrb & 0x61) == 0x41) {
		/* Timer B will not get any pulses if Timer A is off. */
		if (ciaatimea >= 0) {
			/* If Timer A is in one-shot mode, and Timer B needs more than
			* one pulse, it will not underflow. */
			if (ciaatb == 0 || (ciaacra & 0x8) == 0) {
				/* Otherwise, we can determine the time of the underflow. */
				/* This may overflow, however.  So just ignore this timer and
				use the fact that we'll call CIA_handler for the A timer.  */
				/* ciaatimeb = ciaatimea + ciaala * DIV10 * ciaatb; */
			}
		}
	}
#endif
	if ((ciaacrb & 0x61) == 0x01) {
		ciaatimeb = div10diff + DIV10 * (ciaatb + ciaastartb);
	}

	if ((ciabcra & 0x21) == 0x01) {
		ciabtimea = div10diff + DIV10 * (ciabta + ciabstarta);
	}
#if 0
	if ((ciabcrb & 0x61) == 0x41) {
		/* Timer B will not get any pulses if Timer A is off. */
		if (ciabtimea >= 0) {
			/* If Timer A is in one-shot mode, and Timer B needs more than
			* one pulse, it will not underflow. */
			if (ciabtb == 0 || (ciabcra & 0x8) == 0) {
				/* Otherwise, we can determine the time of the underflow. */
				/* ciabtimeb = ciabtimea + ciabla * DIV10 * ciabtb; */
			}
		}
	}
#endif
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
		eventtab[ev_cia].evtime = ciatime + get_cycles();
	}
	events_schedule();
}

void CIA_handler(void)
{
	CIA_update();
	CIA_calctimers();
}

void cia_diskindex(void)
{
	ciabicr |= 0x10;
	RethinkICRB();
}
void cia_parallelack(void)
{
	ciaaicr |= 0x10;
	RethinkICRA();
}

static bool checkalarm(unsigned long tod, unsigned long alarm, bool inc, int ab)
{
	if (tod == alarm)
		return true;
	//	if (!ab)
	//		return false;
	if (!currprefs.cs_ciatodbug)
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

STATIC_INLINE bool ciab_checkalarm(bool inc, bool irq)
{
	// hack: do not trigger alarm interrupt if KS code and both
	// tod and alarm == 0. This incorrectly triggers on non-cycle exact
	// modes. Real hardware value written to ciabtod by KS is always
	// at least 1 or larger due to bus cycle delays when reading
	// old value.
#if 1
	if ((munge24(m68k_getpc()) & 0xFFF80000) != 0xF80000) {
		if (ciabtod == 0 && ciabalarm == 0)
			return false;
	}
#endif
	if (checkalarm(ciabtod, ciabalarm, inc, 1)) {
#if CIAB_DEBUG_IRQ
		write_log(_T("CIAB tod %08x %08x\n"), ciabtod, ciabalarm);
#endif
		if (irq) {
			ciabicr |= 4;
			RethinkICRB();
		}
		return true;
	}
	return false;
}

STATIC_INLINE void ciaa_checkalarm(bool inc)
{
	if (checkalarm(ciaatod, ciaaalarm, inc, 0)) {
#if CIAA_DEBUG_IRQ
		write_log(_T("CIAA tod %08x %08x\n"), ciaatod, ciaaalarm);
#endif
		ciaaicr |= 4;
		RethinkICRA();
	}
}


#ifdef TOD_HACK
static uae_u64 tod_hack_tv, tod_hack_tod, tod_hack_tod_last;
static int tod_hack_enabled;
static int tod_hack_delay;
static int tod_diff_cnt;
#define TOD_HACK_DELAY 50
#define TOD_HACK_TIME 312 * 50 * 10
static void tod_hack_reset(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tod_hack_tv = (uae_u64)tv.tv_sec * 1000000 + tv.tv_usec;
	tod_hack_tod = ciaatod;
	tod_hack_tod_last = tod_hack_tod;
	tod_diff_cnt = 0;
}
#endif

static int heartbeat_cnt;
void cia_heartbeat(void)
{
	heartbeat_cnt = 10;
}

static void do_tod_hack(int dotod)
{
	struct timeval tv;
	static int oldrate;
	uae_u64 t;
	int rate;
	int docount = 0;

	if (tod_hack_enabled == 0)
		return;
	if (!heartbeat_cnt) {
		if (tod_hack_enabled > 0)
			tod_hack_enabled = -1;
		return;
	}
	if (tod_hack_enabled < 0) {
		tod_hack_enabled = TOD_HACK_TIME;
		return;
	}
	if (tod_hack_enabled > 1) {
		tod_hack_enabled--;
		if (tod_hack_enabled == 1) {
			//write_log (_T("TOD HACK enabled\n"));
			tod_hack_reset();
		}
		return;
	}

	if (currprefs.cs_ciaatod == 0) {
		rate = (int)(vblank_hz + 0.5);
		if (rate >= 59 && rate <= 61)
			rate = 60;
		if (rate >= 49 && rate <= 51)
			rate = 50;
	}
	else if (currprefs.cs_ciaatod == 1) {
		rate = 50;
	}
	else {
		rate = 60;
	}
	if (rate <= 0)
		return;
	if (rate != oldrate || (ciaatod & 0xfff) != (tod_hack_tod_last & 0xfff)) {
		write_log(_T("TOD HACK reset %d,%d %ld,%lld\n"), rate, oldrate, ciaatod, tod_hack_tod_last);
		tod_hack_reset();
		oldrate = rate;
		docount = 1;
	}

	if (!dotod && currprefs.cs_ciaatod == 0)
		return;

	if (tod_hack_delay > 0) {
		tod_hack_delay--;
		if (tod_hack_delay > 0)
			return;
		tod_hack_delay = TOD_HACK_DELAY;
	}

	gettimeofday(&tv, NULL);
	t = (uae_u64)tv.tv_sec * 1000000 + tv.tv_usec;
	if (t - tod_hack_tv >= 1000000 / rate) {
		tod_hack_tv += 1000000 / rate;
		tod_diff_cnt += 1000000 - (1000000 / rate) * rate;
		tod_hack_tv += tod_diff_cnt / rate;
		tod_diff_cnt %= rate;
		docount = 1;
	}
	if (docount) {
		ciaatod++;
		ciaatod &= 0x00ffffff;
		tod_hack_tod_last = ciaatod;
		ciaa_checkalarm(false);
	}
}

static int resetwarning_phase, resetwarning_timer;

static void setcode(uae_u8 keycode)
{
	kbcode = ~((keycode << 1) | (keycode >> 7));
}

static void sendrw(void)
{
	setcode(AK_RESETWARNING);
	ciaasdr = kbcode;
	kblostsynccnt = 8 * maxvpos * 8; // 8 frames * 8 bits.
	ciaaicr |= 8;
	RethinkICRA();
	write_log(_T("KB: sent reset warning code (phase=%d)\n"), resetwarning_phase);
}

int resetwarning_do(int canreset)
{
	if (!currprefs.keyboard_connected)
		return 0;
	if (resetwarning_phase || regs.halted > 0) {
		/* just force reset if second reset happens during resetwarning */
		if (canreset) {
			resetwarning_phase = 0;
			resetwarning_timer = 0;
		}
		return 0;
	}
	resetwarning_phase = 1;
	resetwarning_timer = maxvpos_nom * 5;
	write_log(_T("KB: reset warning triggered\n"));
	sendrw();
	return 1;
}

static void resetwarning_check(void)
{
	if (resetwarning_timer > 0) {
		resetwarning_timer--;
		if (resetwarning_timer <= 0) {
			write_log(_T("KB: reset warning forced reset. Phase=%d\n"), resetwarning_phase);
			resetwarning_phase = -1;
			kblostsynccnt = 0;
			//send_internalevent(INTERNALEVENT_KBRESET);
			uae_reset(0, 1);
		}
	}
	if (resetwarning_phase == 1) {
		if (!kblostsynccnt) { /* first AK_RESETWARNING handshake received */
			write_log(_T("KB: reset warning second phase..\n"));
			resetwarning_phase = 2;
			resetwarning_timer = maxvpos_nom * 5;
			sendrw();
		}
	}
	else if (resetwarning_phase == 2) {
		if (ciaacra & 0x40) { /* second AK_RESETWARNING handshake active */
			resetwarning_phase = 3;
			write_log(_T("KB: reset warning SP = output\n"));
			/* System won't reset until handshake signal becomes inactive or 10s has passed */
			resetwarning_timer = 10 * maxvpos_nom * vblank_hz;
		}
	}
	else if (resetwarning_phase == 3) {
		if (!(ciaacra & 0x40)) { /* second AK_RESETWARNING handshake disabled */
			write_log(_T("KB: reset warning end by software. reset.\n"));
			resetwarning_phase = -1;
			kblostsynccnt = 0;
			//send_internalevent(INTERNALEVENT_KBRESET);
			uae_reset(0, 1);
		}
	}
}

void CIA_hsync_prehandler(void)
{
}

static void keyreq(void)
{
#if KB_DEBUG
	write_log(_T("code=%02x (%02x)\n"), kbcode, (uae_u8)(~((kbcode >> 1) | (kbcode << 7))));
#endif
	ciaasdr = kbcode;
	kblostsynccnt = 8 * maxvpos * 8; // 8 frames * 8 bits.
	ciaaicr |= 8;
	RethinkICRA();
}

/* All this complexity to lazy evaluate TOD increase.
* Only increase it cycle-exactly if it is visible to running program:
* causes interrupt or program is reading or writing TOD registers
*/

static int ciab_tod_hoffset;
static int ciab_tod_event_state;
// TOD increase has extra 14-16 E-clock delay
// Possibly TICK input pin has built-in debounce circuit
#define TOD_INC_DELAY (14 * (ECLOCK_DATA_CYCLE + ECLOCK_WAIT_CYCLE) / 2)

static void CIAB_tod_inc(bool irq)
{
	ciab_tod_event_state = 3; // done
	if (!ciabtodon)
		return;
	ciabtod++;
	ciabtod &= 0xFFFFFF;
	ciab_checkalarm(true, irq);
}

static void CIAB_tod_inc_event(uae_u32 v)
{
	if (ciab_tod_event_state != 2)
		return;
	CIAB_tod_inc(true);
}

// Someone reads or writes TOD registers, sync TOD increase
static void CIAB_tod_check(void)
{
	if (ciab_tod_event_state != 1 || !ciabtodon)
		return;
	int hpos = current_hpos();
	hpos -= ciab_tod_hoffset;
	if (hpos >= 0 || currprefs.m68k_speed < 0) {
		// Program should see the changed TOD
		CIAB_tod_inc(true);
		return;
	}
	// Not yet, add event to guarantee exact TOD inc position
	ciab_tod_event_state = 2; // event active
	event2_newevent_xx(-1, -hpos, 0, CIAB_tod_inc_event);
}

void CIAB_tod_handler(int hoffset)
{
	if (!ciabtodon)
		return;
	ciab_tod_hoffset = hoffset + TOD_INC_DELAY;
	ciab_tod_event_state = 1; // TOD inc needed
	if (checkalarm((ciabtod + 1) & 0xffffff, ciabalarm, true, 1)) {
		// causes interrupt on this line, add event
		ciab_tod_event_state = 2; // event active
		event2_newevent_xx(-1, ciab_tod_hoffset, 0, CIAB_tod_inc_event);
	}
}

void keyboard_connected(bool connect)
{
	if (connect) {
		write_log(_T("Keyboard connected\n"));
	}
	else {
		write_log(_T("Keyboard disconnected\n"));
	}
	kbstate = 0;
	kblostsynccnt = 0;
	resetwarning_phase = 0;
}

static void check_keyboard(void)
{
	if (currprefs.keyboard_connected) {
		if ((keys_available() || kbstate < 3) && !kblostsynccnt) {
			switch (kbstate)
			{
			case 0:
				kbcode = 0; /* powerup resync */
				kbstate++;
				break;
			case 1:
				setcode(AK_INIT_POWERUP);
				kbstate++;
				break;
			case 2:
				setcode(AK_TERM_POWERUP);
				kbstate++;
				break;
			case 3:
				kbcode = ~get_next_key();
				break;
			}
			keyreq();
		}
	}
	else {
		while (keys_available()) {
			get_next_key();
		}
	}
}

void CIA_hsync_posthandler(bool ciahsync, bool dotod)
{
	if (ciahsync) {
		// cia hysnc
		// Previous line was supposed to increase TOD but
		// no one cared. Do it now.
		if (ciab_tod_event_state == 1)
			CIAB_tod_inc(false);
		ciab_tod_event_state = 0;

		if (currprefs.tod_hack && ciaatodon)
			do_tod_hack(dotod);
	}
	else if (currprefs.keyboard_connected) {
		// custom hsync
		if (resetwarning_phase) {
			resetwarning_check();
			while (keys_available())
				get_next_key();
		}
		else {
			if ((hsync_counter & 15) == 0)
				check_keyboard();
		}
	}
	else {
		while (keys_available()) {
			get_next_key();
		}
	}
}

static void calc_led(int old_led)
{
	unsigned long c = get_cycles();
	unsigned long t = (c - led_cycle) / CYCLE_UNIT;
	if (old_led)
		led_cycles_on += t;
	else
		led_cycles_off += t;
	led_cycle = c;
}

static void led_vsync(void)
{
	int v;

	calc_led(led);
	if (led_cycles_on && !led_cycles_off)
		v = 255;
	else if (led_cycles_off && !led_cycles_on)
		v = 0;
	else if (led_cycles_off)
		v = led_cycles_on * 255 / (led_cycles_on + led_cycles_off);
	else
		v = 255;
	if (v < 0)
		v = 0;
	if (v > 255)
		v = 255;
	gui_data.powerled_brightness = v;
	led_cycles_on = 0;
	led_cycles_off = 0;
	if (led_old_brightness != gui_data.powerled_brightness) {
		gui_data.powerled = gui_data.powerled_brightness > 127;
		//gui_led(LED_POWER, gui_data.powerled, gui_data.powerled_brightness);
		gui_led(LED_POWER, gui_data.powerled);
		led_filter_audio();
	}
	led_old_brightness = gui_data.powerled_brightness;
	led_cycle = get_cycles();
}

static void write_battclock(void);
void CIA_vsync_prehandler(void)
{
	if (heartbeat_cnt > 0)
		heartbeat_cnt--;
	if (rtc_delayed_write < 0) {
		rtc_delayed_write = 50;
	}
	else if (rtc_delayed_write > 0) {
		rtc_delayed_write--;
		if (rtc_delayed_write == 0)
			write_battclock();
	}
	led_vsync();
	CIA_handler();
	if (kblostsynccnt > 0) {
		kblostsynccnt -= maxvpos;
		if (kblostsynccnt <= 0) {
			kblostsynccnt = 0;
			keyreq();
#if KB_DEBUG
			write_log(_T("lostsync\n"));
#endif
		}
	}
}

static void CIAA_tod_handler(uae_u32 v)
{
	ciaatod++;
	ciaatod &= 0xFFFFFF;
	ciaa_checkalarm(true);
}

void CIAA_tod_inc(int cycles)
{
#ifdef TOD_HACK
	if (currprefs.tod_hack && tod_hack_enabled == 1)
		return;
#endif
	if (!ciaatodon)
		return;
	event2_newevent_xx(-1, cycles + TOD_INC_DELAY, 0, CIAA_tod_handler);
}

static void check_led(void)
{
	uae_u8 v = ciaapra;
	bool led2;

	v |= ~ciaadra; /* output is high when pin's direction is input */
	led2 = (v & 2) ? 0 : 1;
	if (led2 != led) {
		calc_led(led);
		led = led2;
		led_old_brightness = -1;
	}
}

static void bfe001_change(void)
{
	uae_u8 v = ciaapra;
	check_led();
	if (currprefs.cs_ciaoverlay && (v & 1) != oldovl) {
		oldovl = v & 1;
		if (!oldovl) {
			map_overlay(1);
		}
		else {
			//activate_debugger ();
			map_overlay(0);
		}
	}
	if (currprefs.cs_cd32cd && (v & 1) != oldcd32mute) {
		oldcd32mute = v & 1;
		akiko_mute(oldcd32mute ? 0 : 1);
	}
}

static uae_u32 getciatod(uae_u32 tod)
{
	if (!currprefs.cs_cia6526)
		return tod;
	uae_u32 bcdtod = 0;
	for (int i = 0; i < 4; i++) {
		int val = tod % 10;
		bcdtod *= 16;
		bcdtod += val;
		tod /= 10;
	}
	return bcdtod;
}
static void setciatod(unsigned long *tod, uae_u32 v)
{
	if (!currprefs.cs_cia6526) {
		*tod = v;
		return;
	}
	uae_u32 bintod = 0;
	for (int i = 0; i < 4; i++) {
		int val = v / 16;
		bintod *= 10;
		bintod += val;
		v /= 16;
	}
	*tod = bintod;
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

static void WriteCIAA(uae_u16 addr, uae_u8 val)
{
	int reg = addr & 15;

#if CIAA_DEBUG_W > 0
	write_log(_T("W_CIAA: bfe%x01 %02X %08X\n"), reg, val, M68K_GETPC);
#endif
#ifdef ACTION_REPLAY
	ar_ciaa[reg] = val;
#endif
	if (!currprefs.cs_ciaoverlay && oldovl) {
		map_overlay(1);
		oldovl = 0;
	}
	switch (reg) {
	case 0:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFE001 W %02X %s\n"), val, debuginfo(0));
#endif
		ciaapra = (ciaapra & ~0xc3) | (val & 0xc3);
		bfe001_change();
		handle_cd32_joystick_cia(ciaapra, ciaadra);
		//dongle_cia_write(0, reg, val);
#ifdef AMAX
		if (is_device_rom(&currprefs, ROMTYPE_AMAX, 0) > 0)
			amax_bfe001_write(val, ciaadra);
#endif
		break;
	case 1:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFE101 W %02X %s\n"), val, debuginfo(0));
#endif
		ciaaprb = val;
		//dongle_cia_write(0, reg, val);
#ifdef PARALLEL_PORT
		if (isprinter() > 0) {
			doprinter(val);
			cia_parallelack();
		}
		else if (isprinter() < 0) {
			parallel_direct_write_data(val, ciaadrb);
			cia_parallelack();
#ifdef ARCADIA
		}
		else if (arcadia_bios) {
			arcadia_parport(1, ciaaprb, ciaadrb);
#endif
		}
		else if (parallel_port_scsi) {
			parallel_port_scsi_write(0, ciaaprb, ciaadrb);
		}
#endif
		break;
	case 2:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFE201 W %02X %s\n"), val, debuginfo(0));
#endif
		ciaadra = val;
		//dongle_cia_write(0, reg, val);
		bfe001_change();
		break;
	case 3:
		ciaadrb = val;
		//dongle_cia_write(0, reg, val);
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFE301 W %02X %s\n"), val, debuginfo(0));
#endif
#ifdef ARCADIA
		if (arcadia_bios)
			arcadia_parport(1, ciaaprb, ciaadrb);
#endif
		break;
	case 4:
		CIA_update();
		ciaala = (ciaala & 0xff00) | val;
		CIA_calctimers();
		break;
	case 5:
		CIA_update();
		ciaala = (ciaala & 0xff) | (val << 8);
		if ((ciaacra & 1) == 0)
			ciaata = ciaala;
		if (ciaacra & 8) {
			ciaata = ciaala;
			ciaacra |= 1;
			ciaastarta = CIASTARTCYCLESHI;
		}
		CIA_calctimers();
		break;
	case 6:
		CIA_update();
		ciaalb = (ciaalb & 0xff00) | val;
		CIA_calctimers();
		break;
	case 7:
		CIA_update();
		ciaalb = (ciaalb & 0xff) | (val << 8);
		if ((ciaacrb & 1) == 0)
			ciaatb = ciaalb;
		if (ciaacrb & 8) {
			ciaatb = ciaalb;
			ciaacrb |= 1;
			ciaastartb = CIASTARTCYCLESHI;
		}
		CIA_calctimers();
		break;
	case 8:
		if (ciaacrb & 0x80) {
			setciatod(&ciaaalarm, (getciatod(ciaaalarm) & ~0xff) | val);
		}
		else {
			setciatod(&ciaatod, (getciatod(ciaatod) & ~0xff) | val);
			ciaatodon = 1;
			ciaa_checkalarm(false);
		}
		break;
	case 9:
		if (ciaacrb & 0x80) {
			setciatod(&ciaaalarm, (getciatod(ciaaalarm) & ~0xff00) | (val << 8));
		}
		else {
			setciatod(&ciaatod, (getciatod(ciaatod) & ~0xff00) | (val << 8));
		}
		break;
	case 10:
		if (ciaacrb & 0x80) {
			setciatod(&ciaaalarm, (getciatod(ciaaalarm) & ~0xff0000) | (val << 16));
		}
		else {
			setciatod(&ciaatod, (getciatod(ciaatod) & ~0xff0000) | (val << 16));
			if (!currprefs.cs_cia6526)
				ciaatodon = 0;
		}
		break;
	case 11:
		if (currprefs.cs_cia6526) {
			if (ciaacrb & 0x80) {
				setciatod(&ciaaalarm, (getciatod(ciaaalarm) & ~0xff000000) | (val << 24));
			}
			else {
				setciatod(&ciaatod, (getciatod(ciaatod) & ~0xff000000) | (val << 24));
				ciaatodon = 0;
			}
		}
		break;
	case 12:
		CIA_update();
		ciaasdr = val;
		if ((ciaacra & 0x41) == 0x41 && ciaasdr_cnt == 0)
			ciaasdr_cnt = 8 * 2;
#if KB_DEBUG
		write_log(_T("CIAA serial port write: %02x cnt=%d PC=%08x\n"), ciaasdr, ciaasdr_cnt, M68K_GETPC);
#endif
		CIA_calctimers();
		break;
	case 13:
		setclr(&ciaaimask, val);
		RethinkICRA();
		break;
	case 14:
		CIA_update();
		val &= 0x7f; /* bit 7 is unused */
		if ((val & 1) && !(ciaacra & 1))
			ciaastarta = CIASTARTCYCLESCRA;
		//if (currprefs.cpuboard_type != 0 && (val & 0x40) != (ciaacra & 0x40)) {
		//	/* bleh, Phase5 CPU timed early boot key check fix.. */
		//	if (m68k_getpc() >= 0xf00000 && m68k_getpc() < 0xf80000)
		//		check_keyboard();
		//}
		if ((val & 0x40) == 0 && (ciaacra & 0x40) != 0) {
			/* todo: check if low to high or high to low only */
			kblostsynccnt = 0;
#if KB_DEBUG
			write_log(_T("KB_ACK %02x->%02x %08x\n"), ciaacra, val, M68K_GETPC);
#endif
		}
		ciaacra = val;
		if (ciaacra & 0x10) {
			ciaacra &= ~0x10;
			ciaata = ciaala;
		}
		CIA_calctimers();
		break;
	case 15:
		CIA_update();
		if ((val & 1) && !(ciaacrb & 1))
			ciaastartb = CIASTARTCYCLESCRA;
		ciaacrb = val;
		if (ciaacrb & 0x10) {
			ciaacrb &= ~0x10;
			ciaatb = ciaalb;
		}
		CIA_calctimers();
		break;
	}
}

static void WriteCIAB(uae_u16 addr, uae_u8 val)
{
	int reg = addr & 15;

#if CIAB_DEBUG_W > 0
	if (((addr >= 8 && addr <= 10) || addr == 15) || CIAB_DEBUG_W > 1)
		write_log(_T("W_CIAB: bfd%x00 %02X %08X\n"), reg, val, M68K_GETPC);
#endif
#ifdef ACTION_REPLAY
	ar_ciab[reg] = val;
#endif
	switch (reg) {
	case 0:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD000 W %02X %s\n"), val, debuginfo(0));
#endif
		//dongle_cia_write(1, reg, val);
		ciabpra = val;
#ifdef SERIAL_PORT
		if (currprefs.use_serial)
			serial_writestatus(ciabpra, ciabdra);
#endif
#ifdef PARALLEL_PORT
		if (isprinter() < 0) {
			parallel_direct_write_status(val, ciabdra);
		}
		else if (parallel_port_scsi) {
			parallel_port_scsi_write(1, ciabpra, ciabdra);
		}
#endif
		break;
	case 1:
#ifdef ACTION_REPLAY
		action_replay_cia_access(true);
#endif
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD100 W %02X %s\n"), val, debuginfo(0));
#endif
		//dongle_cia_write(1, reg, val);
		ciabprb = val;
		DISK_select(val);
		break;
	case 2:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD200 W %02X %s\n"), val, debuginfo(0));
#endif
		//dongle_cia_write(1, reg, val);
		ciabdra = val;
#ifdef SERIAL_PORT
		if (currprefs.use_serial)
			serial_writestatus(ciabpra, ciabdra);
#endif
		break;
	case 3:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD300 W %02X %s\n"), val, debuginfo(0));
#endif
		//dongle_cia_write(1, reg, val);
		ciabdrb = val;
		break;
	case 4:
		CIA_update();
		ciabla = (ciabla & 0xff00) | val;
		CIA_calctimers();
		break;
	case 5:
		CIA_update();
		ciabla = (ciabla & 0xff) | (val << 8);
		if ((ciabcra & 1) == 0)
			ciabta = ciabla;
		if (ciabcra & 8) {
			ciabta = ciabla;
			ciabcra |= 1;
			ciabstarta = CIASTARTCYCLESHI;
		}
		CIA_calctimers();
		break;
	case 6:
		CIA_update();
		ciablb = (ciablb & 0xff00) | val;
		CIA_calctimers();
		break;
	case 7:
		CIA_update();
		ciablb = (ciablb & 0xff) | (val << 8);
		if ((ciabcrb & 1) == 0)
			ciabtb = ciablb;
		if (ciabcrb & 8) {
			ciabtb = ciablb;
			ciabcrb |= 1;
			ciabstartb = CIASTARTCYCLESHI;
		}
		CIA_calctimers();
		break;
	case 8:
		CIAB_tod_check();
		if (ciabcrb & 0x80) {
			setciatod(&ciabalarm, (getciatod(ciabalarm) & ~0xff) | val);
		}
		else {
			setciatod(&ciabtod, (getciatod(ciabtod) & ~0xff) | val);
			ciabtodon = 1;
			ciab_checkalarm(false, true);
		}
		break;
	case 9:
		CIAB_tod_check();
		if (ciabcrb & 0x80) {
			setciatod(&ciabalarm, (getciatod(ciabalarm) & ~0xff00) | (val << 8));
		}
		else {
			setciatod(&ciabtod, (getciatod(ciabtod) & ~0xff00) | (val << 8));
		}
		break;
	case 10:
		CIAB_tod_check();
		if (ciabcrb & 0x80) {
			setciatod(&ciabalarm, (getciatod(ciabalarm) & ~0xff0000) | (val << 16));
		}
		else {
			setciatod(&ciabtod, (getciatod(ciabtod) & ~0xff0000) | (val << 16));
			if (!currprefs.cs_cia6526)
				ciabtodon = 0;
		}
		break;
	case 11:
		if (currprefs.cs_cia6526) {
			CIAB_tod_check();
			if (ciabcrb & 0x80) {
				setciatod(&ciabalarm, (getciatod(ciabalarm) & ~0xff000000) | (val << 24));
			}
			else {
				setciatod(&ciabtod, (getciatod(ciabtod) & ~0xff000000) | (val << 24));
				ciabtodon = 0;
			}
		}
		break;
	case 12:
		CIA_update();
		ciabsdr = val;
		if ((ciabcra & 0x40) == 0)
			ciabsdr_cnt = 0;
		if ((ciabcra & 0x41) == 0x41 && ciabsdr_cnt == 0)
			ciabsdr_cnt = 8 * 2;
		CIA_calctimers();
		break;
	case 13:
		setclr(&ciabimask, val);
		RethinkICRB();
		break;
	case 14:
		CIA_update();
		val &= 0x7f; /* bit 7 is unused */
		if ((val & 1) && !(ciabcra & 1))
			ciabstarta = CIASTARTCYCLESCRA;
		ciabcra = val;
		if (ciabcra & 0x10) {
			ciabcra &= ~0x10;
			ciabta = ciabla;
		}
		CIA_calctimers();
		break;
	case 15:
		CIA_update();
		if ((val & 1) && !(ciabcrb & 1))
			ciabstartb = CIASTARTCYCLESCRA;
		ciabcrb = val;
		if (ciabcrb & 0x10) {
			ciabcrb &= ~0x10;
			ciabtb = ciablb;
		}
		CIA_calctimers();
		break;
	}
}

void cia_set_overlay (bool overlay)
{
	oldovl = overlay;
}

void CIA_reset(void)
{
#ifdef TOD_HACK
	tod_hack_tv = 0;
	tod_hack_tod = 0;
	tod_hack_enabled = 0;
	if (currprefs.tod_hack)
		tod_hack_enabled = TOD_HACK_TIME;
#endif

	kblostsynccnt = 0;
	serbits = 0;
	oldcd32mute = 1;
	resetwarning_phase = resetwarning_timer = 0;
	heartbeat_cnt = 0;
	ciab_tod_event_state = 0;

	if (!savestate_state) {
		oldovl = true;
		kbstate = 0;
		ciaatlatch = ciabtlatch = 0;
		ciaapra = 0; ciaadra = 0;
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
		CIA_calctimers();
		DISK_select_set(ciabprb);
	}
	map_overlay(0);
	check_led();
#ifdef SERIAL_PORT
	if (currprefs.use_serial && !savestate_state)
		serial_dtr_off(); /* Drop DTR at reset */
#endif
	if (savestate_state) {
		if (currprefs.cs_ciaoverlay) {
			oldovl = true;
		}
		bfe001_change();
		if (!currprefs.cs_ciaoverlay) {
			map_overlay(oldovl ? 0 : 1);
		}
	}
#ifdef CD32
	if (!isrestore()) {
		akiko_reset();
		if (!akiko_init())
			currprefs.cs_cd32cd = changed_prefs.cs_cd32cd = 0;
	}
#endif
}

void dumpcia(void)
{
	write_log(_T("A: CRA %02x CRB %02x ICR %02x IM %02x TA %04x (%04x) TB %04x (%04x)\n"),
		ciaacra, ciaacrb, ciaaicr, ciaaimask, ciaata, ciaala, ciaatb, ciaalb);
	write_log(_T("TOD %06x (%06x) ALARM %06x %c%c CYC=%08X\n"),
		ciaatod, ciaatol, ciaaalarm, ciaatlatch ? 'L' : ' ', ciaatodon ? ' ' : 'S', get_cycles());
	write_log(_T("B: CRA %02x CRB %02x ICR %02x IM %02x TA %04x (%04x) TB %04x (%04x)\n"),
		ciabcra, ciabcrb, ciabicr, ciabimask, ciabta, ciabla, ciabtb, ciablb);
	write_log(_T("TOD %06x (%06x) ALARM %06x %c%c CLK=%d\n"),
		ciabtod, ciabtol, ciabalarm, ciabtlatch ? 'L' : ' ', ciabtodon ? ' ' : 'S', div10 / CYCLE_UNIT);
}

/* CIA memory access */

DECLARE_MEMORY_FUNCTIONS(cia);
addrbank cia_bank = {
	cia_lget, cia_wget, cia_bget,
	cia_lput, cia_wput, cia_bput,
	default_xlate, default_check, NULL, NULL, _T("CIA"),
	cia_lgeti, cia_wgeti,
	ABFLAG_IO | ABFLAG_CIA, S_READ, S_WRITE, NULL, 0x3f01, 0xbfc000
};

static void cia_wait_pre(int cianummask)
{
	if (currprefs.cachesize)
		return;

  int div = (get_cycles () - eventtab[ev_cia].oldcycles) % DIV10;
  int cycles;

  if (div >= DIV10 * ECLOCK_DATA_CYCLE / 10) {
	  cycles = DIV10 - div;
	  cycles += DIV10 * ECLOCK_DATA_CYCLE / 10;
  }
  else if (div) {
	  cycles = DIV10 + DIV10 * ECLOCK_DATA_CYCLE / 10 - div;
  }
  else {
	  cycles = DIV10 * ECLOCK_DATA_CYCLE / 10 - div;
  }
  if (cycles) {
	  do_cycles(cycles);
  }
}

static void cia_wait_post(int cianummask, uae_u32 value)
{
	if (currprefs.cachesize) {
		do_cycles (8 * CYCLE_UNIT / 2);
	} else {
		int c = 6 * CYCLE_UNIT / 2;
		do_cycles (c);
  }
}

// Gayle or Fat Gary does not enable CIA /CS lines if both CIAs are selected
// Old Gary based Amigas enable both CIAs in this situation

STATIC_INLINE bool issinglecia(void)
{
	return currprefs.cs_ide || currprefs.cs_pcmcia || currprefs.cs_mbdmac || currprefs.cs_cd32cd;
}
STATIC_INLINE bool isgayle(void)
{
	return currprefs.cs_ide || currprefs.cs_pcmcia;
}

// Gayle CIA select
STATIC_INLINE bool iscia(uaecptr addr)
{
	uaecptr mask = addr & 0xf000;
	return mask == 0xe000 || mask == 0xd000;
}

static bool isgaylenocia(uaecptr addr)
{
	if (!isgayle())
		return true;
	// gayle CIA region is only 4096 bytes at 0xbfd000 and 0xbfe000
	return iscia(addr);
}

static bool isgarynocia(uaecptr addr)
{
	return !iscia(addr) && currprefs.cs_fatgaryrev >= 0;
}

static int cia_chipselect(uaecptr addr)
{
	int cs = (addr >> 12) & 3;
	if (currprefs.cs_cd32cd) {
		// Unexpected Akiko CIA select:
		// 0,1 = CIA-A
		if (cs == 0)
			cs = 1;
		// 2,3 = CIA-B
		if (cs == 3)
			cs = 2;
	}
	return cs;
}

static uae_u32 REGPARAM2 cia_bget(uaecptr addr)
{
	int r = (addr & 0xf00) >> 8;
	uae_u8 v = 0;

	if (isgarynocia(addr))
		return dummy_get(addr, 1, false, 0);

	if (!isgaylenocia(addr))
		return dummy_get(addr, 1, false, 0);

	switch (cia_chipselect(addr))
	{
	case 0:
		if (!issinglecia()) {
			cia_wait_pre(1 | 2);
			v = (addr & 1) ? ReadCIAA(r) : ReadCIAB(r);
			cia_wait_post(1 | 2, v);
		}
		break;
	case 1:
		cia_wait_pre(2);
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
			v = (addr & 1) ? regs.irc : ReadCIAB(r);
		}
		else {
			v = (addr & 1) ? dummy_get(addr, 1, false, 0) : ReadCIAB(r);
		}
		cia_wait_post(2, v);
		break;
	case 2:
		cia_wait_pre(1);
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible)
			v = (addr & 1) ? ReadCIAA(r) : regs.irc >> 8;
		else
			v = (addr & 1) ? ReadCIAA(r) : dummy_get(addr, 1, false, 0);
		cia_wait_post(1, v);
		break;
	case 3:
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
			cia_wait_pre(0);
			v = (addr & 1) ? regs.irc : regs.irc >> 8;
			cia_wait_post(0, v);
		}
		if (warned > 0 || currprefs.illegal_mem) {
			write_log(_T("cia_bget: unknown CIA address %08X=%02X PC=%08X\n"), addr, v & 0xff, M68K_GETPC);
			warned--;
		}
		break;
	}

	return v;
}

static uae_u32 REGPARAM2 cia_wget(uaecptr addr)
{
	int r = (addr & 0xf00) >> 8;
	uae_u16 v = 0;

	if (isgarynocia(addr))
		return dummy_get(addr, 2, false, 0);

	if (!isgaylenocia(addr))
		return dummy_get(addr, 2, false, 0);

	switch (cia_chipselect(addr))
	{
	case 0:
		if (!issinglecia())
		{
			cia_wait_pre(1 | 2);
			v = (ReadCIAB(r) << 8) | ReadCIAA(r);
			cia_wait_post(1 | 2, v);
		}
		break;
	case 1:
		cia_wait_pre(2);
		v = (ReadCIAB(r) << 8) | dummy_get(addr, 1, false, 0);
		cia_wait_post(2, v);
		break;
	case 2:
		cia_wait_pre(1);
		v = (dummy_get(addr, 1, false, 0) << 8) | ReadCIAA(r);
		cia_wait_post(1, v);
		break;
	case 3:
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
			cia_wait_pre(0);
			v = regs.irc;
			cia_wait_post(0, v);
		}
		if (warned > 0 || currprefs.illegal_mem) {
			write_log(_T("cia_wget: unknown CIA address %08X=%04X PC=%08X\n"), addr, v & 0xffff, M68K_GETPC);
			warned--;
		}
		break;
	}
	return v;
}

static uae_u32 REGPARAM2 cia_lget(uaecptr addr)
{
	uae_u32 v;
	v = cia_wget(addr) << 16;
	v |= cia_wget(addr + 2);
	return v;
}

static uae_u32 REGPARAM2 cia_wgeti(uaecptr addr)
{
	if (currprefs.cpu_model >= 68020)
		return dummy_wgeti(addr);
	return cia_wget(addr);
}
static uae_u32 REGPARAM2 cia_lgeti(uae_u32 addr)
{
	if (currprefs.cpu_model >= 68020)
		return dummy_lgeti(addr);
	return cia_lget(addr);
}

static void REGPARAM2 cia_bput(uaecptr addr, uae_u32 value)
{
	int r = (addr & 0xf00) >> 8;

	if (isgarynocia(addr)) {
		dummy_put(addr, 1, false);
		return;
	}

	if (!isgaylenocia(addr))
		return;

	int cs = cia_chipselect(addr);

	if (!issinglecia() || (cs & 3) != 0) {
		cia_wait_pre(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0));
		if ((cs & 2) == 0)
			WriteCIAB(r, value);
		if ((cs & 1) == 0)
			WriteCIAA(r, value);
		cia_wait_post(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0), value);
		if (((cs & 3) == 3) && (warned > 0 || currprefs.illegal_mem)) {
			write_log(_T("cia_bput: unknown CIA address %08X=%02X PC=%08X\n"), addr, value & 0xff, M68K_GETPC);
			warned--;
		}
	}
}

static void REGPARAM2 cia_wput(uaecptr addr, uae_u32 value)
{
	int r = (addr & 0xf00) >> 8;

	if (isgarynocia(addr)) {
		dummy_put(addr, 2, false);
		return;
	}

	if (!isgaylenocia(addr))
		return;

	int cs = cia_chipselect(addr);

	if (!issinglecia() || (cs & 3) != 0) {
		cia_wait_pre(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0));
		if ((cs & 2) == 0)
			WriteCIAB(r, value >> 8);
		if ((cs & 1) == 0)
			WriteCIAA(r, value & 0xff);
		cia_wait_post(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0), value);
		if (((cs & 3) == 3) && (warned > 0 || currprefs.illegal_mem)) {
			write_log(_T("cia_wput: unknown CIA address %08X=%04X %08X\n"), addr, value & 0xffff, M68K_GETPC);
			warned--;
		}
	}
}

static void REGPARAM2 cia_lput(uaecptr addr, uae_u32 value)
{
	cia_wput(addr, value >> 16);
	cia_wput(addr + 2, value & 0xffff);
}

/* battclock memory access */

static uae_u32 REGPARAM3 clock_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 clock_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 clock_bget(uaecptr) REGPARAM;
static void REGPARAM3 clock_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 clock_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 clock_bput(uaecptr, uae_u32) REGPARAM;

addrbank clock_bank = {
	clock_lget, clock_wget, clock_bget,
	clock_lput, clock_wput, clock_bput,
	default_xlate, default_check, NULL, NULL, _T("Battery backed up clock (none)"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE, NULL, 0x3f, 0xd80000
};

static unsigned int clock_control_d;
static unsigned int clock_control_e;
static unsigned int clock_control_f;

#define RF5C01A_RAM_SIZE 16
static uae_u8 rtc_memory[RF5C01A_RAM_SIZE], rtc_alarm[RF5C01A_RAM_SIZE];

static uae_u8 getclockreg(int addr, struct tm *ct)
{
	uae_u8 v = 0;

	if (currprefs.cs_rtc == 1 || currprefs.cs_rtc == 3) { /* MSM6242B */
		switch (addr) {
		case 0x0: v = ct->tm_sec % 10; break;
		case 0x1: v = ct->tm_sec / 10; break;
		case 0x2: v = ct->tm_min % 10; break;
		case 0x3: v = ct->tm_min / 10; break;
		case 0x4: v = ct->tm_hour % 10; break;
		case 0x5:
			if (clock_control_f & 4) {
				v = ct->tm_hour / 10; // 24h
			}
			else {
				v = (ct->tm_hour % 12) / 10; // 12h
				v |= ct->tm_hour >= 12 ? 4 : 0; // AM/PM bit
			}
			break;
		case 0x6: v = ct->tm_mday % 10; break;
		case 0x7: v = ct->tm_mday / 10; break;
		case 0x8: v = (ct->tm_mon + 1) % 10; break;
		case 0x9: v = (ct->tm_mon + 1) / 10; break;
		case 0xA: v = ct->tm_year % 10; break;
		case 0xB: v = (ct->tm_year / 10) & 0x0f;  break;
		case 0xC: v = ct->tm_wday; break;
		case 0xD: v = clock_control_d; break;
		case 0xE: v = clock_control_e; break;
		case 0xF: v = clock_control_f; break;
		}
	}
	else if (currprefs.cs_rtc == 2) { /* RF5C01A */
		int bank = clock_control_d & 3;
		/* memory access */
		if (bank >= 2 && addr < 0x0d)
			return (rtc_memory[addr] >> ((bank == 2) ? 0 : 4)) & 0x0f;
		/* alarm */
		if (bank == 1 && addr < 0x0d) {
			v = rtc_alarm[addr];
#if CLOCK_DEBUG
			write_log(_T("CLOCK ALARM R %X: %X\n"), addr, v);
#endif
			return v;
		}
		switch (addr) {
		case 0x0: v = ct->tm_sec % 10; break;
		case 0x1: v = ct->tm_sec / 10; break;
		case 0x2: v = ct->tm_min % 10; break;
		case 0x3: v = ct->tm_min / 10; break;
		case 0x4: v = ct->tm_hour % 10; break;
		case 0x5:
			if (rtc_alarm[10] & 1)
				v = ct->tm_hour / 10; // 24h
			else
				v = ((ct->tm_hour % 12) / 10) | (ct->tm_hour >= 12 ? 2 : 0); // 12h
			break;
		case 0x6: v = ct->tm_wday; break;
		case 0x7: v = ct->tm_mday % 10; break;
		case 0x8: v = ct->tm_mday / 10; break;
		case 0x9: v = (ct->tm_mon + 1) % 10; break;
		case 0xA: v = (ct->tm_mon + 1) / 10; break;
		case 0xB: v = (ct->tm_year % 100) % 10; break;
		case 0xC: v = (ct->tm_year % 100) / 10; break;
		case 0xD: v = clock_control_d; break;
			/* E and F = write-only, reads as zero */
		case 0xE: v = 0; break;
		case 0xF: v = 0; break;
		}
	}
#if CLOCK_DEBUG
	write_log(_T("CLOCK R: %X = %X, PC=%08x\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static void write_battclock(void)
{
	if (!currprefs.rtcfile[0] || currprefs.cs_rtc == 0)
		return;
	struct zfile *f = zfile_fopen(currprefs.rtcfile, _T("wb"));
	if (f) {
		struct tm *ct;
		time_t t = time(0);
		t += currprefs.cs_rtc_adjust;
		ct = localtime(&t);
		uae_u8 od = clock_control_d;
		if (currprefs.cs_rtc == 2)
			clock_control_d &= ~3;
		for (int i = 0; i < 13; i++) {
			uae_u8 v = getclockreg(i, ct);
			zfile_fwrite(&v, 1, 1, f);
		}
		clock_control_d = od;
		zfile_fwrite(&clock_control_d, 1, 1, f);
		zfile_fwrite(&clock_control_e, 1, 1, f);
		zfile_fwrite(&clock_control_f, 1, 1, f);
		if (currprefs.cs_rtc == 2) {
			zfile_fwrite(rtc_alarm, RF5C01A_RAM_SIZE, 1, f);
			zfile_fwrite(rtc_memory, RF5C01A_RAM_SIZE, 1, f);
		}
		zfile_fclose(f);
	}
}

void rtc_hardreset(void)
{
	rtc_delayed_write = 0;
	if (currprefs.cs_rtc == 1 || currprefs.cs_rtc == 3) { /* MSM6242B */
		clock_bank.name = currprefs.cs_rtc == 1 ? _T("Battery backed up clock (MSM6242B)") : _T("Battery backed up clock A2000 (MSM6242B)");
		clock_control_d = 0x1;
		clock_control_e = 0;
		clock_control_f = 0x4; /* 24/12 */
	}
	else if (currprefs.cs_rtc == 2) { /* RF5C01A */
		clock_bank.name = _T("Battery backed up clock (RF5C01A)");
		clock_control_d = 0x8; /* Timer EN */
		clock_control_e = 0;
		clock_control_f = 0;
		memset(rtc_memory, 0, RF5C01A_RAM_SIZE);
		memset(rtc_alarm, 0, RF5C01A_RAM_SIZE);
		rtc_alarm[10] = 1; /* 24H mode */
	}
	if (currprefs.rtcfile[0]) {
		struct zfile *f = zfile_fopen(currprefs.rtcfile, _T("rb"));
		if (f) {
			uae_u8 empty[13];
			zfile_fread(empty, 13, 1, f);
			zfile_fread(&clock_control_d, 1, 1, f);
			zfile_fread(&clock_control_e, 1, 1, f);
			zfile_fread(&clock_control_f, 1, 1, f);
			zfile_fread(rtc_alarm, RF5C01A_RAM_SIZE, 1, f);
			zfile_fread(rtc_memory, RF5C01A_RAM_SIZE, 1, f);
			zfile_fclose(f);
		}
	}
}

static uae_u32 REGPARAM2 clock_lget(uaecptr addr)
{
	if ((addr & 0xffff) >= 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 4, false, 0);

	return (clock_wget(addr) << 16) | clock_wget(addr + 2);
}

static uae_u32 REGPARAM2 clock_wget(uaecptr addr)
{
	if ((addr & 0xffff) >= 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 2, false, 0);

	return (clock_bget(addr) << 8) | clock_bget(addr + 1);
}

static uae_u32 REGPARAM2 clock_bget(uaecptr addr)
{
	struct tm *ct;
	uae_u8 v = 0;

	if ((addr & 0xffff) >= 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 1, false, 0);

#ifdef CDTV
	if (currprefs.cs_cdtvram && (addr & 0xffff) >= 0x8000)
		return cdtv_battram_read(addr);
#endif

	addr &= 0x3f;
	if ((addr & 3) == 2 || (addr & 3) == 0 || currprefs.cs_rtc == 0) {
		return dummy_get(addr, 1, false, v);
	}
	time_t t = time(0);
	t += currprefs.cs_rtc_adjust;
	ct = localtime(&t);
	addr >>= 2;
	return getclockreg(addr, ct);
}

static void REGPARAM2 clock_lput(uaecptr addr, uae_u32 value)
{
	if ((addr & 0xffff) >= 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 4, value);
		return;
	}

	clock_wput(addr, value >> 16);
	clock_wput(addr + 2, value);
}

static void REGPARAM2 clock_wput(uaecptr addr, uae_u32 value)
{
	if ((addr & 0xffff) >= 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 2, value);
		return;
	}

	clock_bput(addr, value >> 8);
	clock_bput(addr + 1, value);
}

static void REGPARAM2 clock_bput(uaecptr addr, uae_u32 value)
{
	//	write_log(_T("W: %x (%x): %x, PC=%08x\n"), addr, (addr & 0xff) >> 2, value & 0xff, M68K_GETPC);

	if ((addr & 0xffff) >= 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 1, value);
		return;
	}

#ifdef CDTV
	if (currprefs.cs_cdtvram && (addr & 0xffff) >= 0x8000) {
		cdtv_battram_write(addr, value);
		return;
	}
#endif

	addr &= 0x3f;
	if ((addr & 1) != 1 || currprefs.cs_rtc == 0)
		return;
	addr >>= 2;
	value &= 0x0f;
	if (currprefs.cs_rtc == 1 || currprefs.cs_rtc == 3) { /* MSM6242B */
#if CLOCK_DEBUG
		write_log(_T("CLOCK W %X: %X\n"), addr, value);
#endif
		switch (addr)
		{
		case 0xD: clock_control_d = value & (1 | 8); break;
		case 0xE: clock_control_e = value; break;
		case 0xF: clock_control_f = value; break;
		}
	}
	else if (currprefs.cs_rtc == 2) { /* RF5C01A */
		int bank = clock_control_d & 3;
		/* memory access */
		if (bank >= 2 && addr < 0x0d) {
			uae_u8 ov = rtc_memory[addr];
			rtc_memory[addr] &= ((bank == 2) ? 0xf0 : 0x0f);
			rtc_memory[addr] |= value << ((bank == 2) ? 0 : 4);
			if (rtc_memory[addr] != ov)
				rtc_delayed_write = -1;
			return;
		}
		/* alarm */
		if (bank == 1 && addr < 0x0d) {
#if CLOCK_DEBUG
			write_log(_T("CLOCK ALARM W %X: %X\n"), addr, value);
#endif
			uae_u8 ov = rtc_alarm[addr];
			rtc_alarm[addr] = value;
			rtc_alarm[0] = rtc_alarm[1] = rtc_alarm[9] = rtc_alarm[12] = 0;
			rtc_alarm[3] &= ~0x8;
			rtc_alarm[5] &= ~0xc;
			rtc_alarm[6] &= ~0x8;
			rtc_alarm[8] &= ~0xc;
			rtc_alarm[10] &= ~0xe;
			rtc_alarm[11] &= ~0xc;
			if (rtc_alarm[addr] != ov)
				rtc_delayed_write = -1;
			return;
		}
#if CLOCK_DEBUG
		write_log(_T("CLOCK W %X: %X\n"), addr, value);
#endif
		switch (addr)
		{
		case 0xD: clock_control_d = value; break;
		case 0xE: clock_control_e = value; break;
		case 0xF: clock_control_f = value; break;
		}
	}
	rtc_delayed_write = -1;
}

#ifdef SAVESTATE

/* CIA-A and CIA-B save/restore code */

static void save_cia_prepare(void)
{
	CIA_update_check();
	CIA_calctimers();
	compute_passed_time();
}

void restore_cia_start(void)
{
	/* Fixes very old statefiles without keyboard state */
	kbstate = 3;
	setcapslockstate(0);
	kblostsynccnt = 0;
}

void restore_cia_finish(void)
{
	eventtab[ev_cia].oldcycles = get_cycles();
	CIA_update();
	CIA_calctimers();
	compute_passed_time();
	eventtab[ev_cia].oldcycles -= div10;
	//dumpcia ();
	DISK_select_set(ciabprb);
}

uae_u8 *restore_cia(int num, uae_u8 *src)
{
	uae_u8 b;
	uae_u16 w;
	uae_u32 l;

	/* CIA registers */
	b = restore_u8();						/* 0 PRA */
	if (num) ciabpra = b; else ciaapra = b;
	b = restore_u8();						/* 1 PRB */
	if (num) ciabprb = b; else ciaaprb = b;
	b = restore_u8();						/* 2 DDRA */
	if (num) ciabdra = b; else ciaadra = b;
	b = restore_u8();						/* 3 DDRB */
	if (num) ciabdrb = b; else ciaadrb = b;
	w = restore_u16();						/* 4 TA */
	if (num) ciabta = w; else ciaata = w;
	w = restore_u16();						/* 6 TB */
	if (num) ciabtb = w; else ciaatb = w;
	l = restore_u8();						/* 8/9/A TOD */
	l |= restore_u8() << 8;
	l |= restore_u8() << 16;
	if (num) ciabtod = l; else ciaatod = l;
	restore_u8();							/* B unused */
	b = restore_u8();						/* C SDR */
	if (num) ciabsdr = b; else ciaasdr = b;
	b = restore_u8();						/* D ICR INFORMATION (not mask!) */
	if (num) ciabicr = b; else ciaaicr = b;
	b = restore_u8();						/* E CRA */
	if (num) ciabcra = b; else ciaacra = b;
	b = restore_u8();						/* F CRB */
	if (num) ciabcrb = b; else ciaacrb = b;

	/* CIA internal data */

	b = restore_u8();						/* ICR MASK */
	if (num) ciabimask = b; else ciaaimask = b;
	w = restore_u8();						/* timer A latch */
	w |= restore_u8() << 8;
	if (num) ciabla = w; else ciaala = w;
	w = restore_u8();						/* timer B latch */
	w |= restore_u8() << 8;
	if (num) ciablb = w; else ciaalb = w;
	w = restore_u8();						/* TOD latched value */
	w |= restore_u8() << 8;
	w |= restore_u8() << 16;
	if (num) ciabtol = w; else ciaatol = w;
	l = restore_u8();						/* alarm */
	l |= restore_u8() << 8;
	l |= restore_u8() << 16;
	if (num) ciabalarm = l; else ciaaalarm = l;
	b = restore_u8();
	if (num) ciabtlatch = b & 1; else ciaatlatch = b & 1;	/* is TOD latched? */
	if (num) ciabtodon = b & 2; else ciaatodon = b & 2;		/* is TOD stopped? */
	b = restore_u8();
	if (num)
		div10 = CYCLE_UNIT * b;
	b = restore_u8();
	if (num) ciabsdr_cnt = b; else ciaasdr_cnt = b;
	return src;
}

uae_u8 *save_cia(int num, int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst, b;
	uae_u16 t;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);

	save_cia_prepare();

	/* CIA registers */

	b = num ? ciabpra : ciaapra;				/* 0 PRA */
	save_u8(b);
	b = num ? ciabprb : ciaaprb;				/* 1 PRB */
	save_u8(b);
	b = num ? ciabdra : ciaadra;				/* 2 DDRA */
	save_u8(b);
	b = num ? ciabdrb : ciaadrb;				/* 3 DDRB */
	save_u8(b);
	t = (num ? ciabta - ciabta_passed : ciaata - ciaata_passed);/* 4 TA */
	save_u16(t);
	t = (num ? ciabtb - ciabtb_passed : ciaatb - ciaatb_passed);/* 6 TB */
	save_u16(t);
	b = (num ? ciabtod : ciaatod);				/* 8 TODL */
	save_u8(b);
	b = (num ? ciabtod >> 8 : ciaatod >> 8);	/* 9 TODM */
	save_u8(b);
	b = (num ? ciabtod >> 16 : ciaatod >> 16);	/* A TODH */
	save_u8(b);
	save_u8(0);								/* B unused */
	b = num ? ciabsdr : ciaasdr;				/* C SDR */
	save_u8(b);
	b = num ? ciabicr : ciaaicr;				/* D ICR INFORMATION (not mask!) */
	save_u8(b);
	b = num ? ciabcra : ciaacra;				/* E CRA */
	save_u8(b);
	b = num ? ciabcrb : ciaacrb;				/* F CRB */
	save_u8(b);

	/* CIA internal data */

	save_u8(num ? ciabimask : ciaaimask);		/* ICR */
	b = (num ? ciabla : ciaala);				/* timer A latch LO */
	save_u8(b);
	b = (num ? ciabla >> 8 : ciaala >> 8);		/* timer A latch HI */
	save_u8(b);
	b = (num ? ciablb : ciaalb);				/* timer B latch LO */
	save_u8(b);
	b = (num ? ciablb >> 8 : ciaalb >> 8);		/* timer B latch HI */
	save_u8(b);
	b = (num ? ciabtol : ciaatol);				/* latched TOD LO */
	save_u8(b);
	b = (num ? ciabtol >> 8 : ciaatol >> 8);	/* latched TOD MED */
	save_u8(b);
	b = (num ? ciabtol >> 16 : ciaatol >> 16);	/* latched TOD HI */
	save_u8(b);
	b = (num ? ciabalarm : ciaaalarm);			/* alarm LO */
	save_u8(b);
	b = (num ? ciabalarm >> 8 : ciaaalarm >> 8);/* alarm MED */
	save_u8(b);
	b = (num ? ciabalarm >> 16 : ciaaalarm >> 16);	/* alarm HI */
	save_u8(b);
	b = 0;
	if (num)
		b |= ciabtlatch ? 1 : 0;
	else
		b |= ciaatlatch ? 1 : 0; /* is TOD latched? */
	if (num)
		b |= ciabtodon ? 2 : 0;
	else
		b |= ciaatodon ? 2 : 0;   /* TOD stopped? */
	save_u8(b);
	save_u8(num ? div10 / CYCLE_UNIT : 0);
	save_u8(num ? ciabsdr_cnt : ciaasdr_cnt);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_keyboard(int *len, uae_u8 *dstptr)
{
	uae_u8 *dst, *dstbak;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 4 + 4 + 1 + 1 + 1 + 1 + 1 + 2);
	save_u32(getcapslockstate() ? 1 : 0);
	save_u32(1);
	save_u8(kbstate);
	save_u8(0);
	save_u8(0);
	save_u8(0);
	save_u8(kbcode);
	save_u16(kblostsynccnt);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_keyboard(uae_u8 *src)
{
	setcapslockstate(restore_u32() & 1);
	uae_u32 v = restore_u32();
	kbstate = restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();
	kbcode = restore_u8();
	kblostsynccnt = restore_u16();
	if (!(v & 1)) {
		kbstate = 3;
		kblostsynccnt = 0;
	}
	return src;
}

#endif /* SAVESTATE */
