/*
* UAE - The Un*x Amiga Emulator
*
* CIA chip support
*
* Copyright 1995 Bernd Schmidt, Alessandro Bissacco
* Copyright 1996, 1997 Stefan Reinauer, Christian Schmitt
* Copyright 2022 Toni Wilen
*/


#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cia.h"
#ifdef SERIAL_PORT
#include "serial.h"
#endif
#include "disk.h"
#include "keybuf.h"
#include "gui.h"
#include "savestate.h"
#include "inputdevice.h"
#include "zfile.h"
#include "ar.h"
#ifdef PARALLEL_PORT
#include "parallel.h"
#endif
#include "akiko.h"
#include "cdtv.h"
#include "debug.h"
#ifdef ARCADIA
#include "arcadia.h"
#endif
#include "audio.h"
#include "keyboard.h"
#ifdef AMAX
#include "amax.h"
#endif
#include "sampler.h"
#include "dongle.h"
#include "inputrecord.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "rommgr.h"
#include "scsi.h"
#include "rtc.h"
#include "devices.h"
#include "keyboard_mcu.h"

#define CIAA_DEBUG_R 0
#define CIAA_DEBUG_W 0
#define CIAA_DEBUG_IRQ 0

#define CIAB_DEBUG_R 0
#define CIAB_DEBUG_W 0
#define CIAB_DEBUG_IRQ 0

#define DONGLE_DEBUG 0
#define KB_DEBUG 0
#define CLOCK_DEBUG 0
#define CIA_EVERY_CYCLE_DEBUG 0

#define TOD_HACK

#define CIA_IRQ_PROCESS_DELAY 0

#define CR_START 1
#define CR_PBON 2
#define CR_OUTMODE 4
#define CR_RUNMODE 8
#define CR_LOAD 0x10
#define CR_INMODE 0x20
#define CR_INMODE1 0x40
#define CR_SPMODE 0x40
#define CR_ALARM 0x80

#define ICR_A 1
#define ICR_B 2
#define ICR_ALARM 4
#define ICR_SP 8
#define ICR_FLAG 0x10
#define ICR_MASK 0x1f

#define CIA_PIPE_ALL_BITS 2
#define CIA_PIPE_ALL_MASK ((1 << CIA_PIPE_ALL_BITS) - 1)
#define CIA_PIPE_INPUT 2
#define CIA_PIPE_CLR1 1
#define CIA_PIPE_CLR2 3
#define CIA_PIPE_OUTPUT 1

/* Akiko internal CIA differences:

- BFE101 and BFD100: reads 3F if data direction is in.

 */

#define E_CLOCK_SYNC_N 2
#define E_CLOCK_START_N 4
#define E_CLOCK_END_N 6
#define E_CLOCK_TOD_N -2

#define E_CLOCK_SYNC_N2 4
#define E_CLOCK_START_N2 6
#define E_CLOCK_END_N2 6
#define E_CLOCK_TOD_N2 0

#define E_CLOCK_SYNC_X 4
#define E_CLOCK_START_X 2
#define E_CLOCK_END_X 6
#define E_CLOCK_TOD_X 0

static int e_clock_sync = E_CLOCK_SYNC_N;
static int e_clock_start = E_CLOCK_START_N;
static int e_clock_end = E_CLOCK_END_N;
static int e_clock_tod = E_CLOCK_TOD_N;

#define E_CLOCK_LENGTH 10
#define E_CYCLE_UNIT (CYCLE_UNIT / 2)
#define DIV10 (E_CLOCK_LENGTH * E_CYCLE_UNIT) /* Yes, a bad identifier. */


struct CIATimer
{
	uae_u32 timer;
	uae_u32 latch;
	uae_u32 passed;
	uae_u16 inputpipe;
	uae_u32 loaddelay;
	uae_u8 preovfl;
	uae_u8 cr;
};

struct CIA
{
	uae_u8 pra, prb;
	uae_u8 dra, drb;
	struct CIATimer t[2];
	uae_u32 tod;
	uae_u32 tol;
	uae_u32 alarm;
	uae_u32 tlatch;
	uae_u8 todon;
	int tod_event_state;
	int tod_offset;
	uae_u8 icr1, icr2;
	bool icr_change;
	uae_u8 imask;
	uae_u8 sdr;
	uae_u8 sdr_buf;
	uae_u8 sdr_load;
	uae_u8 sdr_cnt;
};

static struct CIA cia[2];

static bool oldovl;
static int led;
static int led_old_brightness;
static evt_t led_cycle;
static evt_t cia_now_evt;
static int led_cycles_on, led_cycles_off;

static int kbstate, kblostsynccnt;
static evt_t kbhandshakestart;
static uae_u8 kbcode;

static uae_u8 serbits;
static int warned = 100;

static struct rtc_msm_data rtc_msm;
static struct rtc_ricoh_data rtc_ricoh;

static int internaleclockphase;
static bool cia_cycle_accurate;

int cia_timer_hack_adjust = 1;

static bool acc_mode(void)
{
	return cia_cycle_accurate;
}

int blop, blop2;

void cia_adjust_eclock_phase(int diff)
{
	internaleclockphase += diff;
	if (internaleclockphase < 0) {
		internaleclockphase += ((-internaleclockphase) / 20) * 20;
		internaleclockphase += 20;
	}
	internaleclockphase %= 20;
	//write_log("CIA E-clock phase %d\n", internaleclockphase);
}

void cia_set_eclockphase(void)
{
	if (currprefs.cs_eclocksync == 3) {
		e_clock_sync = E_CLOCK_SYNC_X;
		e_clock_start = E_CLOCK_START_X;
		e_clock_end = E_CLOCK_END_X;
		e_clock_tod = E_CLOCK_TOD_X;
	} else if (currprefs.cs_eclocksync == 2) {
		e_clock_sync = E_CLOCK_SYNC_N2;
		e_clock_start = E_CLOCK_START_N2;
		e_clock_end = E_CLOCK_END_N2;
		e_clock_tod = E_CLOCK_TOD_N2;
	} else {
		e_clock_sync = E_CLOCK_SYNC_N;
		e_clock_start = E_CLOCK_START_N;
		e_clock_end = E_CLOCK_END_N;
		e_clock_tod = E_CLOCK_TOD_N;
	}
}

static evt_t get_e_cycles(void)
{
	// temporary e-clock phase shortcut
	if (0 && blop) {
		cia_adjust_eclock_phase(1);
		blop = 0;
	}
	if (0 && blop2) {
		if (currprefs.cs_eclocksync == 0) {
			currprefs.cs_eclocksync = 1;
		}
		currprefs.cs_eclocksync += 1;
		if (currprefs.cs_eclocksync >= 4) {
			currprefs.cs_eclocksync = 1;
		}
		changed_prefs.cs_eclocksync = currprefs.cs_eclocksync;
		cia_set_eclockphase();
		write_log("CIA elock timing mode %d\n", currprefs.cs_eclocksync);
		blop2 = 0;
	}

	evt_t c = get_cycles();
	c += currprefs.cs_eclockphase * E_CYCLE_UNIT;
	c += internaleclockphase * 2 * E_CYCLE_UNIT;
	return c;
}

static void setclr(uae_u8 *p, uae_u8 val)
{
	if (val & 0x80) {
		*p |= val & 0x7F;
	} else {
		*p &= ~val;
	}
}

#if CIA_IRQ_PROCESS_DELAY
/* delay interrupt after current CIA register access if
 * interrupt would have triggered mid access
 */
static int cia_interrupt_disabled;
static int cia_interrupt_delay;
#endif

static void ICRIRQ(uae_u32 data)
{
	safe_interrupt_set(IRQ_SOURCE_CIA, 0, (data & 0x2000) != 0);
}

static void ICR(uae_u32 num)
{
	struct CIA *c = &cia[num];

#if CIA_IRQ_PROCESS_DELAY
	if (currprefs.cpu_memory_cycle_exact && !(c->icr & 0x20) && (cia_interrupt_disabled & (1 << num))) {
		c->cia_interrupt_delay |= 1 << num;
#if CIAB_DEBUG_IRQ
		write_log(_T("cia%c interrupt disabled ICR=%02X PC=%x\n"), num ? 'b' : 'a', c->icr, M68K_GETPC);
#endif
		return;
	}
#endif
	c->icr1 |= 0x20;
	if (num && currprefs.cs_compatible != CP_VELVET) {
		ICRIRQ(0x2000);
	} else {
		ICRIRQ(0x0008);
	}
}

static void RethinkICR(int num)
{
	struct CIA *c = &cia[num];

	if (c->icr1 & c->imask & ICR_MASK) {
#if CIAA_DEBUG_IRQ
		write_log(_T("CIA%c IRQ %02X %02X\n"), num ? 'B' : 'A', c->icr1, c->icr2);
#endif
		if (!(c->icr1 & 0x80)) {
			c->icr1 |= 0x80 | 0x40;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event(num ? DMA_EVENT_CIAB_IRQ : DMA_EVENT_CIAA_IRQ);
			}
#endif
			ICR(num);
		}
	}

}

void rethink_cias(void)
{
	if (cia[0].icr1 & 0x40) {
		ICRIRQ(0x0008);
	}
	if (cia[1].icr1 & 0x40) {
		if (currprefs.cs_compatible == CP_VELVET) {
			ICRIRQ(0x0008);
		} else {
			ICRIRQ(0x2000);
		}
	}
}

static uae_u16 bitstodelay(uae_u16 v)
{
	switch (v)
	{
	case 0:
		return CIA_PIPE_ALL_BITS - 0;
	case 1:
	case 2:
#if CIA_PIPE_ALL_BITS > 2
	case 4:
#endif
		return CIA_PIPE_ALL_BITS - 1;
	case 3:
#if CIA_PIPE_ALL_BITS > 2
	case 5:
	case 6:
#endif
		return CIA_PIPE_ALL_BITS - 2;
#if CIA_PIPE_ALL_BITS > 2
	case 7:
		return CIA_PIPE_ALL_BITS - 3;
#endif
	default:
		abort();
		break;
	}
}

/* Figure out how many CIA timer cycles have passed for each timer since the
last call of CIA_calctimers.  */

static void compute_passed_time_cia(int num, uae_u32 ciaclocks)
{
	struct CIA *c = &cia[num];

	c->t[0].passed = 0;
	c->t[1].passed = 0;

	if ((c->t[0].cr & (CR_INMODE | CR_START)) == CR_START) {
		uae_u32 cc = ciaclocks;
		int pipe = bitstodelay(c->t[0].inputpipe);
		if (cc > pipe) {
			cc -= pipe;
		} else {
			cc = 0;
		}
		c->t[0].passed = cc;
		assert(cc < 65536);
	}
	if ((c->t[1].cr & (CR_INMODE | CR_INMODE1 | CR_START)) == CR_START) {
		uae_u32 cc = ciaclocks;
		int pipe = bitstodelay(c->t[1].inputpipe);
		if (cc > pipe) {
			cc -= pipe;
		} else {
			cc = 0;
		}
		c->t[1].passed = cc;
		assert(cc < 65536);
	}
}

static void compute_passed_time(void)
{
	evt_t ccount = get_cycles() - eventtab[ev_cia].oldcycles;
	if (ccount > INT_MAX) {
		ccount = INT_MAX;
	}
	uae_u32 ciaclocks = (uae_u32)ccount / DIV10;

	compute_passed_time_cia(0, ciaclocks);
	compute_passed_time_cia(1, ciaclocks);
}

static void timer_reset(struct CIATimer *t)
{
	t->timer = t->latch;
	if (acc_mode()) {
		if (t->cr & CR_RUNMODE) {
			t->inputpipe &= ~CIA_PIPE_CLR1;
		} else {
			t->inputpipe &= ~CIA_PIPE_CLR2;
		}
	}
}

static uae_u8 cia_inmode_cnt(int num)
{
	struct CIA *c = &cia[num];
	uae_u8 icr = 0;
	bool decb = false;

	// A INMODE=1 (count CNT pulses)
	if ((c->t[0].cr & (CR_INMODE | CR_START)) == (CR_INMODE | CR_START)) {
		if (c->t[0].timer == 0) {
			icr |= ICR_A;
			timer_reset(&c->t[0]);
			if (c->t[0].cr & CR_RUNMODE) {
				c->t[0].cr &= ~CR_START;
			}
			// B INMODE = 1x (count A undeflows)
			if ((c->t[1].cr & (CR_INMODE1 | CR_START)) == (CR_INMODE1 | CR_START)) {
				decb = true;
			}
		} else {
			c->t[0].timer--;
		}
	}
	// B INMODE=01 (count CNT pulses)
	if ((c->t[1].cr & (CR_INMODE1 | CR_INMODE | CR_START)) == (CR_INMODE | CR_START)) {
		decb = 1;
	}

	if (decb) {
		if (c->t[1].timer == 0) {
			icr |= ICR_B;
			timer_reset(&c->t[1]);
			if (c->t[1].cr & CR_RUNMODE) {
				c->t[1].cr &= ~CR_START;
			}
		} else {
			c->t[1].timer--;
		}
	}
	return icr;
}

static int process_pipe(struct CIATimer *t, int cc, uae_u8 crmask, int *ovfl, int loadednow)
{
	int ccout = cc;

	if (cc == 1 && acc_mode()) {
		int out = t->inputpipe & CIA_PIPE_OUTPUT;
		t->inputpipe >>= 1;
		if ((t->cr & crmask) == CR_START) {
			t->inputpipe |= CIA_PIPE_INPUT;
		}
		// interrupt 1 cycle early if timer is already zero
		if (t->timer == 0 && t->latch == 0 && (t->inputpipe & CIA_PIPE_OUTPUT)) {
			*ovfl = loadednow ? 1 : 2;
		}
		return out;
	}

	while (t->inputpipe != CIA_PIPE_ALL_MASK && cc > 0) {
		if (!(t->inputpipe & CIA_PIPE_OUTPUT)) {
			ccout--;
		}
		t->inputpipe >>= 1;
		if ((t->cr & crmask) == CR_START) {
			t->inputpipe |= CIA_PIPE_INPUT;
		}
		cc--;
	}

	return ccout;
}

/* Called to advance all CIA timers to the current time.  This expects that
one of the timer values will be modified, and CIA_calctimers will be called
in the same cycle.  */

static void CIA_update_check(void)
{
	evt_t ccount = get_cycles() - eventtab[ev_cia].oldcycles;
	if (ccount > INT_MAX) {
		ccount = INT_MAX;
	}
	int ciaclocks = (uae_u32)(ccount / DIV10);
	if (!ciaclocks) {
		return;
	}

	uae_u8 icr = 0;
	for (int num = 0; num < 2; num++) {
		struct CIA *c = &cia[num];
		int ovfl[2], sp;
		bool loaded[2], loaded2[2], loaded3[3];

		c->icr1 |= c->icr2;
		c->icr2 = 0;
		c->icr_change = false;

		ovfl[0] = 0;
		ovfl[1] = 0;
		sp = 0;

		for (int tn = 0; tn < 2; tn++) {
			struct CIATimer *t = &c->t[tn];
			
			loaded[tn] = false;
			loaded2[tn] = false;
			loaded3[tn] = false;

			// CIA special cases
			if (t->loaddelay) {
				if (ciaclocks > 1) {
					abort();
				}

				if (t->loaddelay & 0x00000001) {
					t->timer = t->latch;
					t->inputpipe &= ~CIA_PIPE_CLR1;
				}

				// timer=0 special cases. TODO: better way to do this..
				// delayed timer stop and interrupt (timer=0 condition)
				if ((t->loaddelay & 0x00010000)) {
					t->cr &= ~CR_START;
					ovfl[tn] = 2;
				}
				// Do not set START=0 until timer has started (timer==0 special case)
				if ((t->loaddelay & 0x00000100) && t->timer == 0) {
					loaded2[tn] = true;
				}
				if ((t->loaddelay & 0x01000000)) {
					loaded[tn] = true;
				}
				if ((t->loaddelay & 0x10000000)) {
					loaded3[tn] = true;
				}

				t->loaddelay >>= 1;
				t->loaddelay &= 0x77777777;
			}
		}

		// Timer A
		int cc = 0;
		if ((c->t[0].cr & (CR_INMODE | CR_START)) == CR_START || c->t[0].inputpipe) {
			cc = process_pipe(&c->t[0], ciaclocks, CR_INMODE | CR_START, &ovfl[0], loaded3[0]);
		}
		if (cc > 0) {
			c->t[0].timer -= cc;
			c->t[0].timer &= 0xffff;
			if (c->t[0].timer == 0) {
				// SP in output mode (data sent can be ignored if CIA-A)
				if ((c->t[0].cr & (CR_SPMODE | CR_RUNMODE)) == CR_SPMODE && c->sdr_cnt > 0) {
					c->sdr_cnt--;
					if (c->sdr_cnt & 1) {
						c->sdr_buf <<= 1;
					}
					if (c->sdr_cnt == 0) {
						sp = 1;
						if (c->sdr_load) {
							c->sdr_load = 0;
							c->sdr_buf = c->sdr;
							c->sdr_cnt = 8 * 2;
						}
					}
				}
				ovfl[0] = 2;
			}
		}
#ifndef AMIBERRY
		assert(c->t[0].timer < 0x10000);
#endif

		// Timer B
		cc = 0;
		if ((c->t[1].cr & (CR_INMODE | CR_INMODE1 | CR_START)) == CR_START || c->t[1].inputpipe) {
			cc = process_pipe(&c->t[1], ciaclocks, CR_INMODE | CR_INMODE1 | CR_START, &ovfl[1], loaded3[1]);
		}
		if (cc > 0) {
			if ((c->t[1].timer == 0 && (c->t[1].cr & (CR_INMODE | CR_INMODE1)))) {
				ovfl[1] = 2;
			} else {
				c->t[1].timer -= cc;
				c->t[1].timer &= 0xffff;
				if ((c->t[1].timer == 0 && !(c->t[1].cr & (CR_INMODE | CR_INMODE1)))) {
					ovfl[1] = 2;
				}
			}
		}
#ifndef AMIBERRY
		assert(c->t[1].timer < 0x10000);
#endif

		// B INMODE=10 or 11 (B counting A underflows)
		if (ovfl[0] && ((c->t[1].cr & (CR_INMODE | CR_INMODE1 | CR_START)) == (CR_INMODE1 | CR_START) || (c->t[1].cr & (CR_INMODE | CR_INMODE1 | CR_START)) == (CR_INMODE | CR_INMODE1 | CR_START))) {
			c->t[1].inputpipe |= CIA_PIPE_INPUT;
		}

		for (int tn = 0; tn < 2; tn++) {
			struct CIATimer *t = &c->t[tn];

			if (ovfl[tn] || t->preovfl) {
				if (ovfl[tn]) {
					if (ovfl[tn] > 1) {
						c->icr2 |= tn ? ICR_B : ICR_A;
						icr |= 1 << num;
					}
					t->timer = t->latch;
				}
				if (!loaded[tn]) {
					if (t->cr & CR_RUNMODE) {
						if (loaded2[tn]) {
							t->loaddelay |= 0x00010000;
						} else {
							t->cr &= ~CR_START;
						}
						if (!acc_mode()) {
							t->inputpipe = 0;
						}
						if (acc_mode()) {
							t->inputpipe &= ~CIA_PIPE_CLR2;
						}
					} else {
						if (acc_mode()) {
							t->inputpipe &= ~CIA_PIPE_CLR1;
						}
					}
				}
				t->preovfl = false;
			}
		}

		if (sp) {
			c->icr2 |= ICR_SP;
			icr |= 1 << num;
		}

		if (!acc_mode()) {
			c->icr1 |= c->icr2;
			c->icr2 = 0;
		} else {
			if (icr) {
				c->icr_change = true;
			}
		}
	}
}

static void CIA_check_ICR(void)
{
	if (cia[0].icr1 & ICR_MASK) {
		RethinkICR(0);
	}
	if (cia[1].icr1 & ICR_MASK) {
		RethinkICR(1);
	}
}

static void CIA_update(void)
{
	CIA_update_check();
	CIA_check_ICR();
}

/* Call this only after CIA_update has been called in the same cycle.  */
static void CIA_calctimers(void)
{
	uae_s32 timevals[4];

	timevals[0] = -1;
	timevals[1] = -1;
	timevals[2] = -1;
	timevals[3] = -1;

	eventtab[ev_cia].oldcycles = get_cycles();

	for (int num = 0; num < 2; num++) {
		struct CIA *c = &cia[num];
		int idx = num * 2;
		bool counting[2] = { false, false };

		if ((c->t[0].cr & (CR_INMODE | CR_START)) == CR_START) {
			int pipe = bitstodelay(c->t[0].inputpipe);
			timevals[idx + 0] = DIV10 * (c->t[0].timer + pipe);
			if (!timevals[idx + 0]) {
				timevals[idx + 0] = DIV10;
			}
			counting[0] = true;
		}

		if ((c->t[1].cr & (CR_INMODE | CR_INMODE1 | CR_START)) == CR_START) {
			int pipe = bitstodelay(c->t[1].inputpipe);
			timevals[idx + 1] = DIV10 * (c->t[1].timer + pipe);
			if (!timevals[idx + 1]) {
				timevals[idx + 1] = DIV10;
			}
			counting[1] = true;
		}

		for (int tn = 0; tn < 2; tn++) {
			struct CIATimer *t = &c->t[tn];
			bool timerspecial = t->loaddelay != 0;
			int tnidx = idx + tn;
			if (t->cr & CR_START) {
				if (t->inputpipe != CIA_PIPE_ALL_MASK) {
					if (counting[tn] || t->inputpipe != 0) {
						timerspecial = true;
					}
				}
			} else {
				if (t->inputpipe != 0) {
					timerspecial = true;
				}
			}
			if (timerspecial && (timevals[tnidx] < 0 || timevals[tnidx] > DIV10)) {
				timevals[tnidx] = DIV10;
			}
		}

		if (c->icr_change && (timevals[idx] < 0 || timevals[idx] > DIV10)) {
			timevals[idx] = DIV10;
		}

#if CIA_EVERY_CYCLE_DEBUG
		timevals[idx] = DIV10;
#endif

	}

	uae_s32 ciatime = INT_MAX;
	if (timevals[0] >= 0)
		ciatime = timevals[0];
	if (timevals[1] >= 0 && timevals[1] < ciatime)
		ciatime = timevals[1];
	if (timevals[2] >= 0 && timevals[2] < ciatime)
		ciatime = timevals[2];
	if (timevals[3] >= 0 && timevals[3] < ciatime)
		ciatime = timevals[3];
	if (ciatime < INT_MAX) {
		eventtab[ev_cia].evtime = get_cycles() + ciatime;
		eventtab[ev_cia].active = true;
	} else {
		eventtab[ev_cia].active = false;
	}

	events_schedule();
}

void CIA_handler(void)
{
	CIA_update();
	CIA_calctimers();
}

static int get_cia_sync_cycles(int *syncdelay)
{
	evt_t c = get_e_cycles();
	int div10 = c % DIV10;
	int add = 0;
	int synccycle = e_clock_sync * E_CYCLE_UNIT;
	if (div10 < synccycle) {
		add += synccycle - div10;
	} else if (div10 > synccycle) {
		add += DIV10 - div10;
		add += synccycle;
	}
	*syncdelay = add;
	// 4 first cycles of E-clock
	add = e_clock_start * E_CYCLE_UNIT;
	return add;
}

void event_CIA_synced_interrupt(uae_u32 v)
{
	CIA_update();
	CIA_calctimers();
}

static void CIA_sync_interrupt(int num, uae_u8 icr)
{
	struct CIA *c = &cia[num];

	if (acc_mode()) {
		if (!(icr & c->imask)) {
			c->icr1 |= icr;
			return;
		}
		c->icr2 |= icr;
		if ((c->icr1 & ICR_MASK) == (c->icr2 & ICR_MASK)) {
			return;
		}
		int syncdelay = 0;
		int delay = get_cia_sync_cycles(&syncdelay);
		delay += syncdelay;
		event2_newevent_xx(-1, DIV10 + delay, num, event_CIA_synced_interrupt);
	} else {
		c->icr1 |= icr;
		CIA_check_ICR();
	}
}

void cia_diskindex(void)
{
	CIA_sync_interrupt(1, ICR_FLAG);
}
void cia_parallelack(void)
{
	CIA_sync_interrupt(0, ICR_FLAG);
}

static bool checkalarm(uae_u32 tod, uae_u32 alarm, bool inc)
{
	if (tod == alarm)
		return true;
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

static bool cia_checkalarm(bool inc, bool irq, int num)
{
	struct CIA *c = &cia[num];

#if 0
	// hack: do not trigger alarm interrupt if KS code and both
	// tod and alarm == 0. This incorrectly triggers on non-cycle exact
	// modes. Real hardware value written to ciabtod by KS is always
	// at least 1 or larger due to bus cycle delays when reading
	// old value.
	if (num) {
		if (!currprefs.cpu_compatible && (munge24(m68k_getpc()) & 0xFFF80000) != 0xF80000) {
			if (c->tod == 0 && c->alarm == 0)
				return false;
		}
	}
#endif
	if (checkalarm(c->tod, c->alarm, inc)) {
#if CIAB_DEBUG_IRQ
		write_log(_T("CIAB tod %08x %08x\n"), c->tod, c->alarm);
#endif
		if (irq) {
			CIA_sync_interrupt(num, ICR_ALARM);
		}
		return true;
	}
	return false;
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
	tod_hack_tod = cia[0].tod;
	tod_hack_tod_last = tod_hack_tod;
	tod_diff_cnt = 0;
}
#endif

static int heartbeat_cnt;
void cia_heartbeat(void)
{
	heartbeat_cnt = 10;
}

static void do_tod_hack(bool dotod)
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
			//write_log(_T("TOD HACK enabled\n"));
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
	} else if (currprefs.cs_ciaatod == 1) {
		rate = 50;
	} else {
		rate = 60;
	}
	if (rate <= 0)
		return;
	if (rate != oldrate || (cia[0].tod & 0xfff) != (tod_hack_tod_last & 0xfff)) {
		write_log(_T("TOD HACK reset %d,%d %ld,%lld\n"), rate, oldrate, cia[0].tod, tod_hack_tod_last);
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
		cia[0].tod++;
		cia[0].tod &= 0x00ffffff;
		tod_hack_tod_last = cia[0].tod;
		cia_checkalarm(false, true, 0);
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
	cia[0].sdr = kbcode;
	kblostsynccnt = 8 * maxvpos * 8; // 8 frames * 8 bits.
	CIA_sync_interrupt(0, ICR_SP);
	write_log(_T("KB: sent reset warning code (phase=%d)\n"), resetwarning_phase);
}

int resetwarning_do(int canreset)
{
	if (currprefs.keyboard_mode < 0)
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
			inputdevice_do_kb_reset();
		}
	}
	if (resetwarning_phase == 1) {
		if (!kblostsynccnt) { /* first AK_RESETWARNING handshake received */
			write_log(_T("KB: reset warning second phase..\n"));
			resetwarning_phase = 2;
			resetwarning_timer = maxvpos_nom * 5;
			sendrw();
		}
	} else if (resetwarning_phase == 2) {
		if (cia[0].t[0].cr & CR_SPMODE) { /* second AK_RESETWARNING handshake active */
			resetwarning_phase = 3;
			write_log(_T("KB: reset warning SP = output\n"));
			/* System won't reset until handshake signal becomes inactive or 10s has passed */
			resetwarning_timer = (int)(10 * maxvpos_nom * vblank_hz);
		}
	} else if (resetwarning_phase == 3) {
		if (!(cia[0].t[0].cr & CR_SPMODE)) { /* second AK_RESETWARNING handshake disabled */
			write_log(_T("KB: reset warning end by software. reset.\n"));
			resetwarning_phase = -1;
			kblostsynccnt = 0;
			inputdevice_do_kb_reset();
		}
	}
}

void cia_keyreq(uae_u8 code)
{
#if KB_DEBUG
	write_log(_T("code=%02x (%02x)\n"), kbcode, (uae_u8)(~((kbcode >> 1) | (kbcode << 7))));
#endif
	cia[0].sdr = code;
	if (currprefs.keyboard_mode == 0) {
		kblostsynccnt = 8 * maxvpos * 8; // 8 frames * 8 bits.
	}
	CIA_sync_interrupt(0, ICR_SP);
}

/* All this complexity to lazy evaluate CIA-B TOD increase.
 * Only increase it cycle-exactly if it is visible to running program:
 * causes interrupt (ALARM) or program is reading or writing TOD registers
 */

// TOD increase has extra delay.
#define TOD_INC_DELAY (12 * E_CLOCK_LENGTH / 2)

static int tod_inc_delay(int hoffset)
{
	int hoff = hoffset + 1; // 1 = HSYNC/VSYNC Agnus pin output is delayed by 1 CCK
	evt_t c = get_e_cycles() + 6 * E_CYCLE_UNIT + hoff * CYCLE_UNIT;
	int offset = hoff;
	offset += TOD_INC_DELAY;
	int unit = (E_CLOCK_LENGTH * 4) / 2; // 4 E-clocks
	int div10 = (c / CYCLE_UNIT) % unit;
	offset += unit - div10;
	offset += e_clock_tod;
	return offset;
}

static void CIA_tod_inc(bool irq, int num)
{
	struct CIA *c = &cia[num];
	c->tod_event_state = 3; // done
	if (!c->todon) {
		return;
	}
	c->tod++;
	c->tod &= 0xFFFFFF;
	cia_checkalarm(true, irq, num);
}

void event_CIA_tod_inc_event(uae_u32 num)
{
	struct CIA *c = &cia[num];
	if (c->tod_event_state != 2) {
		return;
	}
	CIA_tod_inc(true, num);
}

static void CIA_tod_event_check(int num)
{
	struct CIA *c = &cia[num];
	if (c->tod_event_state == 1) {
		int hpos = current_hpos();
		if (hpos >= c->tod_offset) {
			CIA_tod_inc(false, num);
			c->tod_event_state = 0;
		}
	}
}

// Someone reads or writes TOD registers, sync TOD increase
static void CIA_tod_check(int num)
{
	struct CIA *c = &cia[num];

	CIA_tod_event_check(num);
	if (!c->todon || c->tod_event_state != 1 || c->tod_offset < 0)
		return;
	int hpos = current_hpos();
	hpos -= c->tod_offset;
	if (hpos >= 0 || !acc_mode()) {
		// Program should see the changed TOD
		CIA_tod_inc(true, num);
		return;
	}
	// Not yet, add event to guarantee exact TOD inc position
	c->tod_event_state = 2; // event active
	event2_newevent_xx(-1, -hpos * CYCLE_UNIT, num, event_CIA_tod_inc_event);
}

static void CIA_tod_handler(int hoffset, int num, bool delayedevent)
{
	struct CIA *c = &cia[num];
	c->tod_event_state = 0;
	c->tod_offset = tod_inc_delay(hoffset);
	if (c->tod_offset >= maxhpos) {
		if (!delayedevent) {
			return;
		}
		// crossed scanline, increase in next line
		c->tod_offset -= maxhpos;
		c->tod_event_state = 4;
		return;
	}
	c->tod_event_state = 1; // TOD inc needed
	if (checkalarm((c->tod + 1) & 0xffffff, c->alarm, true)) {
		// causes interrupt on this line, add event
		c->tod_event_state = 2; // event active
		event2_newevent_xx(-1, c->tod_offset * CYCLE_UNIT, num, event_CIA_tod_inc_event);
	}
}

void CIAA_tod_handler(int hoffset)
{
	struct CIA *c = &cia[0];
#ifdef TOD_HACK
	if (currprefs.tod_hack && tod_hack_enabled == 1) {
		c->tod_event_state = 0;
		return;
	}
#endif
	CIA_tod_handler(hoffset, 0, true);
}
void CIAB_tod_handler(int hoffset)
{
	CIA_tod_handler(hoffset, 1, false);
}

void keyboard_connected(bool connect)
{
	if (connect) {
		write_log(_T("Keyboard connected\n"));
		if (currprefs.keyboard_mode > 0) {
			keymcu_reset();
			keymcu2_reset();
			keymcu3_reset();
		}
	} else {
		write_log(_T("Keyboard disconnected\n"));
	}
	kbstate = 0;
	kblostsynccnt = 0;
	resetwarning_phase = 0;
}

static bool keymcu_execute(void)
{
	bool handshake = (cia[0].t[0].cr & 0x40) != 0 && (cia[0].sdr_buf & 0x80) == 0;

#if 1
	extern int blop;
	if (blop & 1) {
		handshake = true;
	}
#endif

	bool cyclemode = false;
	if (currprefs.keyboard_mode == KB_A500_6570 ||
		currprefs.keyboard_mode == KB_A600_6570 ||
		currprefs.keyboard_mode == KB_A1000_6570 ||
		currprefs.keyboard_mode == KB_Ax000_6570)
	{
		cyclemode = keymcu_run(handshake);
	}
	if (currprefs.keyboard_mode == KB_A1200_6805) {
		cyclemode = keymcu2_run(handshake);
	}
	if (currprefs.keyboard_mode == KB_A2000_8039) {
		cyclemode = keymcu3_run(handshake);
	}
	return cyclemode;
}

static void keymcu_event(uae_u32 v)
{
	bool cyclemode = keymcu_execute();
	if (cyclemode) {
		// execute few times / scanline, does not need to be accurate
		// because keyboard MCU has separate not that accurate clock crystal.
		event2_newevent_x_remove(keymcu_event);
		event2_newevent_xx(-1, 27 * CYCLE_UNIT, 0, keymcu_event);
	}
}

static void keymcu_do(void)
{
	keymcu_event(0);
}

static void check_keyboard(void)
{
	if (currprefs.keyboard_mode >= 0) {
		if ((keys_available() || kbstate < 3) && !kblostsynccnt ) {
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
			cia_keyreq(kbcode);
		}
	} else {
		while (keys_available()) {
			get_next_key();
		}
	}
}

static void cia_delayed_tod(int num)
{
	struct CIA *c = &cia[num];
	if (c->tod_event_state == 4) {
		c->tod_event_state = 1;
		return;
	}
	if (c->tod_event_state == 1) {
		CIA_tod_inc(false, num);
	}
	c->tod_event_state = 0;
	c->tod_offset = -1;
}

void CIA_hsync_posthandler(bool ciahsync, bool dotod)
{
	if (ciahsync) {
		// CIA-B HSync pulse
		// Delayed previous line TOD increase.
		cia_delayed_tod(1);
		if (currprefs.tod_hack && cia[0].todon) {
			do_tod_hack(dotod);
		}
	} else {
		if (currprefs.keyboard_mode > 0) {
			keymcu_do();
		} else {
			if (currprefs.keyboard_mode == 0) {
				// custom hsync
				if (resetwarning_phase) {
					resetwarning_check();
					while (keys_available()) {
						get_next_key();
					}
				} else {
					if ((hsync_counter & 15) == 0) {
						check_keyboard();
					}
				}
			} else {
				while (keys_available()) {
					get_next_key();
				}
			}
		}
	}

	if (!ciahsync) {
		// Increase CIA-A TOD if delayed from previous line
		cia_delayed_tod(0);
	}
}

static void calc_led(int old_led)
{
	evt_t c = get_cycles();
	int t = (int)((c - led_cycle) / CYCLE_UNIT);
	if (old_led > 0)
		led_cycles_on += t;
	else
		led_cycles_off += t;
	led_cycle = c;
}

static void led_vsync(void)
{
	int v;
	bool ledonoff;

	calc_led(led);
	if (led_cycles_on && !led_cycles_off)
		v = 255;
	else if (led_cycles_off && !led_cycles_on)
		v = 0;
	else if (led_cycles_off)
		v = (int)(led_cycles_on * 255 / (led_cycles_on + led_cycles_off));
	else
		v = 255;
	if (v < 0)
		v = 0;
	ledonoff = v > 96;
	if (currprefs.power_led_dim && v < currprefs.power_led_dim)
		v = currprefs.power_led_dim;
	if (v > 255)
		v = 255;
	gui_data.powerled_brightness = v;
	led_cycles_on = 0;
	led_cycles_off = 0;
	if (led_old_brightness != gui_data.powerled_brightness || ledonoff != gui_data.powerled) {
		gui_data.powerled = ledonoff;
		gui_led (LED_POWER, gui_data.powerled, gui_data.powerled_brightness);
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

	if (rtc_msm.delayed_write < 0) {
		rtc_msm.delayed_write = 50;
	} else if (rtc_msm.delayed_write > 0) {
		rtc_msm.delayed_write--;
		if (rtc_msm.delayed_write == 0)
			write_battclock();
	}
	if (rtc_ricoh.delayed_write < 0) {
		rtc_ricoh.delayed_write = 50;
	} else if (rtc_ricoh.delayed_write > 0) {
		rtc_ricoh.delayed_write--;
		if (rtc_ricoh.delayed_write == 0)
			write_battclock();
	}

	led_vsync();
	keybuf_vsync();
	if (kblostsynccnt > 0) {
		kblostsynccnt -= maxvpos;
		if (kblostsynccnt <= 0) {
			kblostsynccnt = 0;
			kbcode = 0;
			cia_keyreq(kbcode);
#if KB_DEBUG
			write_log(_T("lostsync\n"));
#endif
		}
	}

	cia_cycle_accurate = currprefs.m68k_speed >= 0 && currprefs.cpu_compatible;
}

static void check_led(void)
{
	uae_u8 v = cia[0].pra;
	int led2;

	v |= ~cia[0].dra; /* output is high when pin's direction is input */
	led2 = (v & 2) ? 0 : 1;
	if (led2 != led) {
		calc_led(led);
		led = led2;
		led_old_brightness = -1;
	}
}

bool get_power_led(void)
{
	return led;
}

static void bfe001_change(void)
{
	uae_u8 v = cia[0].pra;
	v |= ~cia[0].dra; /* output is high when pin's direction is input */
	check_led();
	if (currprefs.cs_ciaoverlay && (v & 1) != oldovl) {
		oldovl = v & 1;
		if (!oldovl) {
			map_overlay(1);
		} else {
			map_overlay(0);
		}
	}
	akiko_mute((v & 1) == 0);
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
static void setciatod(uae_u32 *tod, uae_u32 v)
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

// E-clock count mode?
static bool CIA_timer_02(int num, uae_u8 cr)
{
	if (num) {
		return (cr & (CR_INMODE | CR_INMODE1)) == 0;
	}
	return (cr & CR_INMODE) == 0;
}

static uae_u8 ReadCIAReg(int num, int reg)
{
	struct CIA *c = &cia[num];
	uae_u8 tmp;
	int tnum = 0;

	switch (reg)
	{
	case 6:
	case 7:
	case 15:
		tnum = 1;
		break;
	}

	struct CIATimer *t = &c->t[tnum];

	switch (reg)
	{
	case 4:
	case 6:
	case 5:
	case 7:
	{
		uae_u16 tval = t->timer - t->passed;
		// fast CPU timer hack
		if ((t->cr & CR_START) && !(t->cr & CR_INMODE1) && !(t->cr & CR_INMODE) && t->latch == t->timer) {
			if (currprefs.cachesize || currprefs.m68k_speed < 0) {
				uae_u16 adj = cia_timer_hack_adjust;
				if (adj >= tval && tval > 1) {
					adj = tval - 1;
				}
				tval -= adj;
			}
		}
		if (reg == 4 || reg == 6) {
			return tval & 0xff;
		}
		return tval >> 8;
	}
	case 8:
		CIA_tod_check(num);
		if (c->tlatch) {
			c->tlatch = 0;
			return getciatod(c->tol);
		} else {
			return getciatod(c->tod);
		}
	case 9:
		CIA_tod_check(num);
		if (c->tlatch) {
			return getciatod(c->tol) >> 8;
		} else {
			return getciatod(c->tod) >> 8;
		}
	case 10:
		CIA_tod_check(num);
		/* only if not already latched. A1200 confirmed. (TW) */
		if (!currprefs.cs_cia6526) {
			if (!c->tlatch) {
				/* no latching if ALARM is set */
				if (!(c->t[1].cr & CR_ALARM)) {
					c->tlatch = 1;
				}
				c->tol = c->tod;
			}
			return getciatod(c->tol) >> 16;
		} else {
			if (c->tlatch)
				return getciatod(c->tol) >> 16;
			else
				return getciatod(c->tod) >> 16;
		}
		break;
	case 11:
		CIA_tod_check(num);
		if (currprefs.cs_cia6526) {
			if (!c->tlatch) {
				if (!(c->t[1].cr & CR_ALARM)) {
					c->tlatch = 1;
				}
				c->tol = c->tod;
			}
			if (c->tlatch) {
				return getciatod(c->tol) >> 24;
			} else{
				return getciatod(c->tod) >> 24;
			}
		} else {
			return 0xff;
		}
		break;
	case 12:
#if KB_DEBUG
		write_log(_T("CIAA serial port: %02x %08x\n"), c->sdr, M68K_GETPC);
#endif
		return c->sdr;
	case 13:
		tmp = c->icr1 & ~(0x40 | 0x20);
		c->icr1 = 0;
		return tmp;
	case 14:
	case 15:
		return t->cr;
	}
	return 0xff;
}

static bool CIA_timer_inmode(int num, uae_u8 cr)
{
	if (num) {
		return (cr & (CR_INMODE | CR_INMODE1)) != 0;
	} else {
		return (cr & CR_INMODE) != 0;
	}
}

static void CIA_thi_write(int num, int tnum, uae_u8 val)
{
	struct CIA *c = &cia[num];
	struct CIATimer *t = &c->t[tnum];

	t->latch = (t->latch & 0xff) | (val << 8);

	// If ONESHOT: Load and start timer.
	// If CONTINUOUS: Load timer if not running.

	if (!acc_mode()) {
		// if inaccurate mode: do everything immediately

		if (!(t->cr & CR_START) || (t->cr & CR_RUNMODE)) {
			t->timer = t->latch;
		}

		if (t->cr & CR_RUNMODE) {
			t->cr |= CR_START;
			if (!CIA_timer_inmode(tnum, t->cr)) {
				t->inputpipe = CIA_PIPE_ALL_MASK;
			}
		}

		if (t->cr & CR_START) {
			if (!CIA_timer_inmode(tnum, t->cr)) {
				if (t->timer <= 1) {
					t->preovfl = true;
				}
			}
		}

	} else {
		// if accurate mode: handle delays cycle-accurately

		if (!(t->cr & CR_START)) {
			t->loaddelay |= 0x00000001 << 1;
			t->loaddelay |= 0x00000001 << 2;
		}

		if (t->cr & CR_RUNMODE) {
			t->cr |= CR_START;
			t->loaddelay |= 0x00000001 << 2;
			// timer=0 special case
			t->loaddelay |= 0x01000000 << 1;
			t->loaddelay |= 0x10000000 << 1;
		}

	}
}

static void CIA_cr_write(int num, int tnum, uae_u8 val)
{
	struct CIA *c = &cia[num];
	struct CIATimer *t = &c->t[tnum];

	if (!tnum) {
		val &= 0x7f; /* bit 7 is unused */
	}

	if (!acc_mode()) {
		// if inaccurate mode: do everything immediately

		// Fast CPU timer hack. If timer is stopped, add few extra ticks to timer before stopping it.
		if ((t->cr & CR_START) && !(val & CR_START) && !(t->cr & CR_INMODE1) && !(t->cr & CR_INMODE) && t->timer == t->latch) {
			if (currprefs.cachesize || currprefs.m68k_speed < 0) {
				uae_u16 adj = cia_timer_hack_adjust;
				if (adj >= t->timer && t->timer > 1) {
					adj = t->timer - 1;
				}
				if (t->timer > adj) {
					t->timer -= adj;
				}
			}
		}

		if (val & CR_LOAD) {
			val &= ~CR_LOAD;
			t->timer = t->latch;
		}

		if (val & CR_START) {
			if (!CIA_timer_inmode(tnum, val)) {
				t->inputpipe = CIA_PIPE_ALL_MASK;
				if (t->timer <= 1) {
					t->preovfl = true;
				}
			}
		} else {
			t->inputpipe = 0; 
		}

	} else {
		// if accurate mode: handle delays cycle-accurately

		if (val & CR_LOAD) {
			val &= ~CR_LOAD;
			t->loaddelay |= 0x00000001 << 2;
			t->loaddelay |= 0x00000100 << 0;
			t->loaddelay |= 0x00000100 << 1;
			if (!(t->cr & CR_START)) {
				t->loaddelay |= 0x00000001 << 1;
			}
			// timer=0 special case
			t->loaddelay |= 0x10000000 << 1;
		}

		if (!(val & CR_START)) {
			t->inputpipe &= ~CIA_PIPE_CLR1;
		}
	}

	// clear serial port state when switching TX<>RX
	if (num == 0 && (t->cr & 0x40) != (val & 0x040)) {
		c->sdr_cnt = 0;
		c->sdr_load = 0;
		c->sdr_buf = 0;
	}

	t->cr = val;
	
}

static void WriteCIAReg(int num, int reg, uae_u8 val)
{
	struct CIA *c = &cia[num];
	int tnum = 0;

	switch (reg)
	{
	case 6:
	case 7:
	case 15:
		tnum = 1;
		break;
	}

	struct CIATimer *t = &c->t[tnum];

	switch (reg) {
	case 4:
	case 6:
		t->latch = (t->latch & 0xff00) | val;
		break;
	case 5:
	case 7:
		CIA_thi_write(num, tnum, val);
		break;
	case 8:
		CIA_tod_check(num);
		if (c->t[1].cr & CR_ALARM) {
			setciatod(&c->alarm, (getciatod(c->alarm) & ~0xff) | val);
		} else {
			setciatod(&c->tod, (getciatod(c->tod) & ~0xff) | val);
			c->todon = 1;
			cia_checkalarm(false, true, num);
			CIA_tod_check(num);
		}
		break;
	case 9:
		CIA_tod_check(num);
		if (c->t[1].cr & CR_ALARM) {
			setciatod(&c->alarm, (getciatod(c->alarm) & ~0xff00) | (val << 8));
		} else {
			setciatod(&c->tod, (getciatod(c->tod) & ~0xff00) | (val << 8));
		}
		break;
	case 10:
		CIA_tod_check(num);
		if (c->t[1].cr & CR_ALARM) {
			setciatod(&c->alarm, (getciatod(c->alarm) & ~0xff0000) | (val << 16));
		} else {
			setciatod(&c->tod, (getciatod(c->tod) & ~0xff0000) | (val << 16));
			if (!currprefs.cs_cia6526) {
				c->todon = 0;
			}
		}
		break;
	case 11:
		CIA_tod_check(num);
		if (currprefs.cs_cia6526) {
			if (c->t[1].cr & CR_ALARM) {
				setciatod(&c->alarm, (getciatod(c->alarm) & ~0xff000000) | (val << 24));
			} else {
				setciatod(&c->tod, (getciatod(c->tod) & ~0xff000000) | (val << 24));
				c->todon = 0;
			}
		}
		break;
	case 12:
		c->sdr = val;
		if ((c->t[0].cr & (CR_INMODE1 | CR_START)) == (CR_INMODE1 | CR_START)) {
			c->sdr_load = 1;
			if (c->sdr_cnt == 0) {
				c->sdr_cnt = 8 * 2;
				c->sdr_load = 0;
				c->sdr_buf = c->sdr;
			}
		}
		break;
	case 13:
		setclr(&c->imask, val);
		RethinkICR(num);
		break;
	case 14:
	case 15:
		CIA_cr_write(num, tnum, val);
		break;
	}
}

static uae_u8 CIA_PBON(struct CIA *c, uae_u8 v)
{
	// A PBON
	if (c->t[0].cr & CR_PBON) {
		int pb6 = 0;
		if (c->t[0].cr & CR_OUTMODE)
			pb6 = c->t[0].cr & CR_START;
		v &= ~0x40;
		v |= pb6 ? 0x40 : 00;
	}
	// B PBON
	if (c->t[1].cr & CR_PBON) {
		int pb7 = 0;
		if (c->t[1].cr & CR_OUTMODE)
			pb7 = c->t[1].cr & CR_START;
		v &= ~0x80;
		v |= pb7 ? 0x80 : 00;
	}
	return v;
}

#if DONGLE_DEBUG > 0
static bool notinrom()
{
	return true;
}
#endif

static uae_u8 ReadCIAA(uae_u32 addr, uae_u32 *flags)
{
	struct CIA *c = &cia[0];
	uae_u32 tmp;
	int reg = addr & 15;

	compute_passed_time();

#if CIAA_DEBUG_R > 0
	if (CIAA_DEBUG_R > 1 || (munge24 (M68K_GETPC) & 0xFFF80000) != 0xF80000)
		write_log(_T("R_CIAA: bfe%x01 %08X\n"), reg, M68K_GETPC);
#endif

	switch (reg) {
	case 0:
	{
		if (flags) {
			*flags |= 1;
		}
		uae_u8 v = DISK_status_ciaa() & 0x3c;
		v |= handle_joystick_buttons(c->pra, c->dra);
		v |= (c->pra | (c->dra ^ 3)) & 0x03;
		v = dongle_cia_read(0, reg, c->dra, v);
#ifdef ARCADIA
		v = alg_joystick_buttons(c->pra, c->dra, v);
#endif

		// 391078-01 CIA: output mode bits always return PRA contents
		if (currprefs.cs_ciatype[0]) {
			v &= ~c->dra;
			v |= c->pra & c->dra;
		}

#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFE001 R %02X %s\n"), v, debuginfo(0));
#endif

		if (flags && (inputrecord_debug & 2)) {
			if (input_record > 0)
				inprec_recorddebug_cia(v, 0, m68k_getpc ());
			else if (input_play > 0)
				inprec_playdebug_cia(v, 0, m68k_getpc ());
		}

		return v;
	}
	case 1:
		tmp = (c->prb & c->drb) | (c->drb ^ 0xff);
#ifdef PARALLEL_PORT
		if (isprinter() > 0) {
			tmp = c->prb;
		} else if (isprinter() < 0) {
			uae_u8 v;
			parallel_direct_read_data(&v);
			tmp = v;
#ifdef ARCADIA
		} else if (arcadia_bios) {
			tmp = arcadia_parport(0, c->prb, c->drb);
#endif
		} else if (currprefs.samplersoundcard >= 0) {
			if (flags) {
				tmp = sampler_getsample((c->pra & 4) ? 1 : 0);
			}
#endif

		} else if (parallel_port_scsi) {

			if (flags) {
				tmp = parallel_port_scsi_read(0, c->prb, c->drb);
			}

		} else {
			tmp = handle_parport_joystick (0, tmp);
			tmp = dongle_cia_read (1, reg, c->drb, tmp);
#if DONGLE_DEBUG > 0
			if (notinrom())
				write_log(_T("BFE101 R %02X %s\n"), tmp, debuginfo(0));
#endif
		}
		tmp = CIA_PBON(c, tmp);
		if (currprefs.cs_ciatype[0]) {
			tmp &= ~c->drb;
			tmp |= c->prb & c->drb;
		}

		return tmp;
	case 2:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFE201 R %02X %s\n"), c->dra, debuginfo(0));
#endif
		return c->dra;
	case 3:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFE301 R %02X %s\n"), c->drb, debuginfo(0));
#endif
		return c->drb;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		return ReadCIAReg(0, reg);
	}
	return 0;
}

static uae_u8 ReadCIAB(uae_u32 addr, uae_u32 *flags)
{
	struct CIA *c = &cia[1];
	uae_u32 tmp;
	int reg = addr & 15;

#if CIAB_DEBUG_R > 0
	if (CIAB_DEBUG_R > 1 || (munge24 (M68K_GETPC) & 0xFFF80000) != 0xF80000) {
		if ((addr >= 8 && addr <= 10) || CIAB_DEBUG_R > 1)
			write_log(_T("R_CIAB: bfd%x00 %08X\n"), reg, M68K_GETPC);
	}
#endif

	compute_passed_time();

	switch (reg) {
	case 0:
		tmp = (c->pra & c->dra) | (c->dra ^ 0xff);
#ifdef PARALLEL_PORT
		if (isprinter() > 0) {
			tmp &= ~3; // clear BUSY and PAPEROUT
			tmp |= 4; // set SELECT
		} else if (isprinter() < 0) {
			uae_u8 v;
			tmp &= ~7;
			if (flags) {
				parallel_direct_read_status(&v);
			}
			tmp |= v & 7;
		} else if (parallel_port_scsi) {
			if (flags) {
				tmp = parallel_port_scsi_read(1, c->pra, c->dra);
			}
		} else {
			// serial port in output mode
			if (c->t[0].cr & 0x40) {
				tmp &= ~3;
				tmp |= (c->sdr_cnt & 1) ? 2 : 0; // clock
				tmp |= (c->sdr_buf & 0x80) ? 1 : 0; // data
			}
			tmp = handle_parport_joystick(1, tmp);
		}
#endif
#ifdef SERIAL_PORT
		tmp = serial_readstatus(tmp, c->dra);
#endif
		tmp = dongle_cia_read(1, reg, c->pra, tmp);
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD000 R %02X %s\n"), tmp, debuginfo(0));
#endif

		if (currprefs.cs_ciatype[1]) {
			tmp &= ~c->dra;
			tmp |= c->pra & c->dra;
		}

		return tmp;
	case 1:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD100 R %02X %s\n"), c->prb, debuginfo(0));
#endif
		tmp = c->prb;
		tmp = DISK_status_ciab(tmp);
		tmp = dongle_cia_read(1, reg, c->prb, tmp);
		tmp = CIA_PBON(c, tmp);
		if (currprefs.cs_ciatype[1]) {
			tmp &= ~c->drb;
			tmp |= c->prb & c->drb;
		}

		return tmp;
	case 2:
		return c->dra;
	case 3:
		return c->drb;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		return ReadCIAReg(1, reg);
	}
	return 0;
}

static void WriteCIAA(uae_u16 addr, uae_u8 val, uae_u32 *flags)
{
	struct CIA *c = &cia[0];
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
		if (notinrom ())
			write_log(_T("BFE001 W %02X %s\n"), val, debuginfo(0));
#endif
		c->pra = (c->pra & ~0xc3) | (val & 0xc3);
		bfe001_change();
		handle_cd32_joystick_cia(c->pra, c->dra);
		dongle_cia_write(0, reg, c->dra, val);
#ifdef AMAX
		if (is_device_rom(&currprefs, ROMTYPE_AMAX, 0) > 0)
			amax_bfe001_write(val, c->dra);
#endif
		break;
	case 1:
#if DONGLE_DEBUG > 0
		if (notinrom ())
			write_log(_T("BFE101 W %02X %s\n"), val, debuginfo(0));
#endif
		c->prb = val;
		dongle_cia_write(0, reg, c->drb, val);
#ifdef ARCADIA
		alg_parallel_port(c->drb, val);
#endif
#ifdef PARALLEL_PORT
		if (isprinter()) {
			if (isprinter() > 0) {
				doprinter(val);
				cia_parallelack();
			} else if (isprinter() < 0) {
				parallel_direct_write_data(val, c->drb);
				cia_parallelack();
			}
		}
#endif
#ifdef ARCADIA
		if (!isprinter() && arcadia_bios) {
			arcadia_parport(1, c->prb, c->drb);
		}
#endif
#ifdef PARALLEL_PORT
		if (!isprinter() && parallel_port_scsi) {
			parallel_port_scsi_write(0, c->prb, c->drb);
		}
#endif
		break;
	case 2:
#if DONGLE_DEBUG > 0
		if (notinrom ())
			write_log(_T("BFE201 W %02X %s\n"), val, debuginfo(0));
#endif
		c->dra = val;
		dongle_cia_write(0, reg, c->pra, val);
		bfe001_change();
		break;
	case 3:
		c->drb = val;
		dongle_cia_write(0, reg, c->prb, val);
#if DONGLE_DEBUG > 0
		if (notinrom ())
			write_log(_T("BFE301 W %02X %s\n"), val, debuginfo(0));
#endif
#ifdef ARCADIA
		if (arcadia_bios)
			arcadia_parport(1, c->prb, c->drb);
#endif
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 15:
		CIA_update();
		if (currprefs.keyboard_mode > 0 && reg == 12) {
			keymcu_do();
		}
		WriteCIAReg(0, reg, val);
		CIA_calctimers();
		break;
	case 14:
		{
			CIA_update();
			bool handshake = (val & CR_INMODE1) != (c->t[0].cr & CR_INMODE1);
			if (currprefs.keyboard_mode == 0) {
				// keyboard handshake handling
				if (currprefs.cpuboard_type != 0 && handshake) {
					/* bleh, Phase5 CPU timed early boot key check fix.. */
					if (m68k_getpc() >= 0xf00000 && m68k_getpc() < 0xf80000)
						check_keyboard();
				}
				if ((val & CR_INMODE1) != 0 && (c->t[0].cr & CR_INMODE1) == 0) {
					// handshake start
					if (kblostsynccnt > 0 && currprefs.cs_kbhandshake) {
						kbhandshakestart = get_cycles();
					}
#if KB_DEBUG
					write_log(_T("KB_ACK_START %02x->%02x %08x\n"), c->t[0].cr, val, M68K_GETPC);
#endif
				} else if ((val & CR_INMODE1) == 0 && (c->t[0].cr & CR_INMODE1) != 0) {
					// handshake end
					/* todo: check if low to high or high to low only */
					if (kblostsynccnt > 0 && currprefs.cs_kbhandshake) {
						evt_t len = get_cycles() - kbhandshakestart;
						if (len < currprefs.cs_kbhandshake * CYCLE_UNIT) {
							write_log(_T("Keyboard handshake pulse length %d < %d (CCKs)\n"), len / CYCLE_UNIT, currprefs.cs_kbhandshake);
						}
					}
					kblostsynccnt = 0;
#if KB_DEBUG
					write_log(_T("KB_ACK_END %02x->%02x %08x\n"), c->t[0].cr, val, M68K_GETPC);
#endif
				}
			}
			WriteCIAReg(0, reg, val);
			CIA_calctimers();
			if (currprefs.keyboard_mode > 0 && handshake) {
				keymcu_do();
			}
		}
		break;
	}
}

static void write_ciab_serial(uae_u8 ndata, uae_u8 odata, uae_u8 ndir, uae_u8 odir)
{
	struct CIA *c = &cia[1];
	uae_u8 val = ndata & ndir;
	uae_u8 icr = 0;

	// CNT 0->1?
	if ((val & 2) && !((odata & odir) & 2)) {
		// CIA-B SP in input mode
		if (!(c->t[0].cr & CR_SPMODE)) {
			c->sdr_buf <<= 1;
			c->sdr_buf |= (val & 1) ? 0x01 : 0x00;
			c->sdr_cnt++;
			if (c->sdr_cnt >= 8) {
				// Data received
				c->sdr = c->sdr_buf;
				icr |= ICR_SP;
				c->sdr_cnt = 0;
			}
		}
		icr |= cia_inmode_cnt(1);
	}
	if (icr) {
		CIA_sync_interrupt(1, icr);
	}
}

static void WriteCIAB(uae_u16 addr, uae_u8 val, uae_u32 *flags)
{
	struct CIA *c = &cia[1];
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
		dongle_cia_write(1, reg, c->dra, val);
		write_ciab_serial(val, c->pra, c->dra, c->dra);
		c->pra = val;
#ifdef SERIAL_PORT
		serial_writestatus(c->pra, c->dra);
#endif
#ifdef PARALLEL_PORT
		if (isprinter() < 0) {
			parallel_direct_write_status(val, c->dra);
		}
		else if (parallel_port_scsi) {
			parallel_port_scsi_write(1, c->pra, c->dra);
		}
#endif
		break;
	case 1:
		*flags |= 2;
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD100 W %02X %s\n"), val, debuginfo(0));
#endif
		dongle_cia_write(1, c->drb, reg, val);
		c->prb = val;
		val = CIA_PBON(c, val);
		DISK_select(val);
		break;
	case 2:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD200 W %02X %s\n"), val, debuginfo(0));
#endif
		dongle_cia_write(1, reg, c->pra, val);
		write_ciab_serial(c->pra, c->pra, val, c->dra);
		c->dra = val;
#ifdef SERIAL_PORT
		if (currprefs.use_serial)
			serial_writestatus(c->pra, c->dra);
#endif
		break;
	case 3:
#if DONGLE_DEBUG > 0
		if (notinrom())
			write_log(_T("BFD300 W %02X %s\n"), val, debuginfo(0));
#endif
		dongle_cia_write(1, reg, c->prb, val);
		c->drb = val;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		CIA_update();
		WriteCIAReg(1, reg, val);
		CIA_calctimers();
		break;
	}
}

void cia_set_overlay(bool overlay)
{
	oldovl = overlay;
}

void CIA_reset(int hardreset)
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
	resetwarning_phase = resetwarning_timer = 0;
	heartbeat_cnt = 0;
	cia[0].tod_event_state = 0;
	cia[1].tod_event_state = 0;
	led_cycles_off = 0;
	led_cycles_on = 0;
	led_cycle = get_cycles();
	led = 0;

	if (!savestate_state) {
		oldovl = true;
		kbstate = 0;
		// serial port data is not reset
		uae_u8 sdra = cia[0].sdr;
		uae_u8 sdrb = cia[1].sdr;
		memset(&cia, 0, sizeof(cia));
		cia[0].t[0].timer = 0xffff;
		cia[0].t[1].timer = 0xffff;
		cia[1].t[0].timer = 0xffff;
		cia[1].t[1].timer = 0xffff;
		cia[0].t[0].latch = 0xffff;
		cia[0].t[1].latch = 0xffff;
		cia[1].t[0].latch = 0xffff;
		cia[1].t[1].latch = 0xffff;
		cia[1].pra = 0x8c;
		if (!hardreset) {
			cia[0].sdr = sdra;
			cia[1].sdr = sdrb;
		}
		internaleclockphase = 0;
		CIA_calctimers();
		DISK_select_set(cia[1].prb);
	}
	cia_set_eclockphase();
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
		led = -1;
		bfe001_change();
		if (!currprefs.cs_ciaoverlay) {
			map_overlay(oldovl ? 0 : 1);
		}
	}
}

void dumpcia(void)
{
	struct CIA *a = &cia[0];
	struct CIA *b = &cia[1];

	compute_passed_time();

	uae_u8 apra = ReadCIAA(0, NULL);
	uae_u8 aprb = ReadCIAA(1, NULL);
	uae_u8 bpra = ReadCIAB(0, NULL);
	uae_u8 bprb = ReadCIAB(1, NULL);

	console_out_f(_T("A: CRA %02x CRB %02x ICR %02x IM %02x TA %04x (%04x) TB %04x (%04x)\n"),
		a->t[0].cr, a->t[1].cr, a->icr1, a->imask, a->t[0].timer - a->t[0].passed,
		a->t[0].latch, a->t[1].timer - a->t[1].passed, a->t[1].latch);
	console_out_f(_T("   PRA %02x [%02x] PRB %02x [%02x] DDRA %02x DDRB %02x SDR %02x\n"), a->pra, apra, a->prb, aprb, a->dra, a->drb, a->sdr);
	console_out_f(_T("   TOD %06x (%06x) ALARM %06x %c%c CYC=%016llX\n"),
		a->tod, a->tol, a->alarm, a->tlatch ? 'L' : '-', a->todon ? '-' : 'S', get_cycles());
	console_out_f(_T("B: CRA %02x CRB %02x ICR %02x IM %02x TA %04x (%04x) TB %04x (%04x)\n"),
		b->t[0].cr, b->t[1].cr, b->icr1, b->imask, b->t[0].timer - b->t[0].passed,
		b->t[0].latch, b->t[1].timer - b->t[1].passed, b->t[1].latch);
	console_out_f(_T("   PRA %02x [%02x] PRB %02x [%02x] DDRA %02x DDRB %02x SDR %02x\n"), b->pra, bpra, b->prb, bprb, b->dra, b->drb, b->sdr);
	console_out_f(_T("   TOD %06x (%06x) ALARM %06x %c%c\n"),
		b->tod, b->tol, b->alarm, b->tlatch ? 'L' : '-', b->todon ? '-' : 'S');
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

static int cia_cycles(int delay, int phase, int val, int post)
{
#ifdef DEBUGGER
	if (currprefs.cpu_memory_cycle_exact && debug_dma) {
		while (delay > 0) {
			int hpos = current_hpos();
			record_cia_access(0xfffff, 0, 0, 0, phase + 1);
			phase += 2;
			if (post) {
				x_do_cycles_post(CYCLE_UNIT, val);
			} else {
				x_do_cycles_pre(CYCLE_UNIT);
			}
			delay -= CYCLE_UNIT;
		}
	} else {
#endif
		if (delay > 0) {
			if (post) {
				x_do_cycles_post(delay, val);
			} else {
				x_do_cycles_pre(delay);
			}
		}
#ifdef DEBUGGER
	}
#endif
	return phase;
}

static void cia_wait_pre(int cianummask)
{
	if (currprefs.cachesize || currprefs.cpu_thread)
		return;
#ifdef WITH_PPC
	if (ppc_state)
		return;
#endif

#if CIA_IRQ_PROCESS_DELAY
	if (currprefs.cpu_memory_cycle_exact) {
		cia_interrupt_disabled |= cianummask;
	}
#endif

#ifndef CUSTOM_SIMPLE
	cia_now_evt = get_cycles();
	int syncdelay = 0;
	int delay = get_cia_sync_cycles(&syncdelay);
#ifdef DEBUGGER
	if (debug_dma) {
		cia_cycles(syncdelay, 100, 0, 0);
		cia_cycles(delay, 0, 0, 0);
	} else {
		cia_cycles(syncdelay + delay, 0, 0, 0);
	}
#else
	cia_cycles(syncdelay + delay, 0, 0, 0);
#endif
#endif
}

static void cia_wait_post(int cianummask, uaecptr addr, uae_u32 value, bool rw)
{
#ifdef DEBUGGER
	if (currprefs.cpu_memory_cycle_exact && debug_dma) {
		int r = (addr & 0xf00) >> 8;
		int hpos = current_hpos();
		record_cia_access(r, cianummask, value, rw, -1);
	}
#endif

#ifdef WITH_PPC
	if (ppc_state)
		return;
#endif
	if (currprefs.cpu_thread)
		return;
	if (currprefs.cachesize) {
		do_cycles(12 * E_CYCLE_UNIT);
	} else {
		// Last 6 cycles of E-clock
		// IPL fetch that got delayed by CIA access?
		if (cia_now_evt == regs.ipl_evt && currprefs.cpu_model <= 68010) {
			int phase = cia_cycles((e_clock_end - 2) * E_CYCLE_UNIT, 4, value, 1);
			regs.ipl[0] = regs.ipl_pin;
			cia_cycles(2 * E_CYCLE_UNIT, phase, value, 1);
		} else {
			cia_cycles(e_clock_end * E_CYCLE_UNIT, 4, value, 1);
		}
#if CIA_IRQ_PROCESS_DELAY
		if (currprefs.cpu_memory_cycle_exact) {
			cia_interrupt_disabled &= ~cianummask;
			if ((cia_interrupt_delay & cianummask) & 1) {
				cia_interrupt_delay &= ~1;
				ICR(0x0008);
			}
			if ((cia_interrupt_delay & cianummask) & 2) {
				cia_interrupt_delay &= ~2;
				ICR(0x2000);
			}
		}
#endif
	}
#if CIA_IRQ_PROCESS_DELAY
	if (!currprefs.cpu_memory_cycle_exact && cia_interrupt_delay) {
		int v = cia_interrupt_delay;
		cia_interrupt_delay = 0;
		if (v & 1)
			ICR(0x0008);
		if (v & 2)
			ICR(0x2000);
	}
#endif
}

static void validate_cia(uaecptr addr, int write, uae_u8 val)
{
	bool err = false;
	if (((addr >> 12) & 3) == 0 || ((addr >> 12) & 3) == 3)
		err = true;
	if (((addr & 0xf00) >> 8) == 11)
		err = true;
	int mask = addr & 0xf000;
	if (mask != 0xe000 && mask != 0xd000)
		err = true;
	if (mask == 0xe000 && (addr & 1) == 0)
		err = true;
	if (mask == 0xd000 && (addr & 1) != 0)
		err = true;
	if (err) {
		if (write) {
			write_log(_T("Invalid CIA write %08x = %02x PC=%08x\n"), addr, val, M68K_GETPC);
		} else {
			write_log(_T("Invalid CIA read %08x PC=%08x\n"), addr, M68K_GETPC);
		}
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
	if (!isgayle ())
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
	uae_u32 flags = 0;

	if (isgarynocia(addr))
		return dummy_get(addr, 1, false, 0);

	if (!isgaylenocia (addr))
		return dummy_get(addr, 1, false, 0);

#ifdef DEBUGGER
	if (memwatch_access_validator) {
		validate_cia(addr, 0, 0);
	}
#endif

	switch (cia_chipselect(addr))
	{
	case 0:
		if (!issinglecia()) {
			cia_wait_pre(1 | 2);
			v = (addr & 1) ? ReadCIAA(r, &flags) : ReadCIAB(r, &flags);
			cia_wait_post(1 | 2, addr, v, false);
		}
		break;
	case 1:
		cia_wait_pre(2);
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
			v = (addr & 1) ? regs.irc : ReadCIAB(r, &flags);
		} else {
			v = (addr & 1) ? dummy_get_safe(addr, 1, false, 0) : ReadCIAB(r, &flags);
		}
		cia_wait_post(2, addr, v, false);
		break;
	case 2:
		cia_wait_pre(1);
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible)
			v = (addr & 1) ? ReadCIAA(r, &flags) : regs.irc >> 8;
		else
			v = (addr & 1) ? ReadCIAA(r, &flags) : dummy_get_safe(addr, 1, false, 0);
		cia_wait_post(1, addr, v, false);
		break;
	case 3:
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
			cia_wait_pre(0);
			v = (addr & 1) ? regs.irc : regs.irc >> 8;
			cia_wait_post(0, addr, v, false);
		}
		if (warned > 0 || currprefs.illegal_mem) {
			write_log(_T("cia_bget: unknown CIA address %08X=%02X PC=%08X\n"), addr, v & 0xff, M68K_GETPC);
			warned--;
		}
		break;
	}
#ifdef ACTION_REPLAY
	if (flags) {
		action_replay_cia_access((flags & 2) != 0);
	}
#endif
	return v;
}

static uae_u32 REGPARAM2 cia_wget(uaecptr addr)
{
	int r = (addr & 0xf00) >> 8;
	uae_u16 v = 0;
	uae_u32 flags = 0;

	if (isgarynocia(addr))
		return dummy_get(addr, 2, false, 0);

	if (!isgaylenocia (addr))
		return dummy_get_safe(addr, 2, false, 0);

#ifdef DEBUGGER
	if (memwatch_access_validator) {
		write_log(_T("CIA word read %08x PC=%08x\n"), addr, M68K_GETPC);
	}
#endif

	switch (cia_chipselect(addr))
	{
	case 0:
		if (!issinglecia())
		{
			cia_wait_pre(1 | 2);
			v = ReadCIAB(r, &flags) << 8;
			v |= ReadCIAA(r, &flags);
			cia_wait_post(1 | 2, addr, v, false);
		}
		break;
	case 1:
		cia_wait_pre(2);
		v = ReadCIAB(r, &flags) << 8;
		v |= dummy_get_safe(addr + 1, 1, false, 0) & 0xff;
		cia_wait_post(2, addr, v, false);
		break;
	case 2:
		cia_wait_pre(1);
		v = ReadCIAA(r, &flags);
		v |= dummy_get_safe(addr, 1, false, 0) << 8;
		cia_wait_post(1, addr, v, false);
		break;
	case 3:
		if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
			cia_wait_pre(0);
			v = regs.irc;
			cia_wait_post(0, addr, v, false);
		}
		if (warned > 0 || currprefs.illegal_mem) {
			write_log(_T("cia_wget: unknown CIA address %08X=%04X PC=%08X\n"), addr, v & 0xffff, M68K_GETPC);
			warned--;
		}
		break;
	}
	if (addr & 1)
		v = (v << 8) | (v >> 8);
#ifdef ACTION_REPLAY
	if (flags) {
		action_replay_cia_access((flags & 2) != 0);
	}
#endif
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
static uae_u32 REGPARAM2 cia_lgeti(uaecptr addr)
{
	if (currprefs.cpu_model >= 68020)
		return dummy_lgeti(addr);
	return cia_lget(addr);
}


static bool cia_debug(uaecptr addr, uae_u32 value, int size)
{
#ifdef DEBUGGER
	if (addr == DEBUG_SPRINTF_ADDRESS || addr == DEBUG_SPRINTF_ADDRESS + 4 || addr == DEBUG_SPRINTF_ADDRESS + 8 ||
		(addr == DEBUG_SPRINTF_ADDRESS + 2 && currprefs.cpu_model < 68020) ||
		(addr == DEBUG_SPRINTF_ADDRESS + 6 && currprefs.cpu_model < 68020) ||
		(addr == DEBUG_SPRINTF_ADDRESS + 10 && currprefs.cpu_model < 68020)) {
		return debug_sprintf(addr, value, size);
	}
#endif
	return false;
}

static void REGPARAM2 cia_bput(uaecptr addr, uae_u32 value)
{
	int r = (addr & 0xf00) >> 8;

	if (cia_debug(addr, value, sz_byte))
		return;

	if (isgarynocia(addr)) {
		dummy_put(addr, 1, false);
		return;
	}

	if (!isgaylenocia(addr))
		return;

#ifdef DEBUGGER
	if (memwatch_access_validator) {
		validate_cia(addr, 1, value);
	}
#endif

	int cs = cia_chipselect(addr);

	if (!issinglecia() || (cs & 3) != 0) {
		uae_u32 flags = 0;
		cia_wait_pre(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0));
		if ((cs & 2) == 0)
			WriteCIAB(r, value, &flags);
		if ((cs & 1) == 0)
			WriteCIAA(r, value, &flags);
		cia_wait_post(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0), addr, value, true);
		if (((cs & 3) == 3) && (warned > 0 || currprefs.illegal_mem)) {
			write_log(_T("cia_bput: unknown CIA address %08X=%02X PC=%08X\n"), addr, value & 0xff, M68K_GETPC);
			warned--;
		}
#ifdef ACTION_REPLAY
		if (flags) {
			action_replay_cia_access((flags & 2) != 0);
		}
#endif
	}
}

static void REGPARAM2 cia_wput(uaecptr addr, uae_u32 v)
{
	int r = (addr & 0xf00) >> 8;

	if (cia_debug(addr, v, sz_word))
		return;

	if (isgarynocia(addr)) {
		dummy_put(addr, 2, false);
		return;
	}

	if (!isgaylenocia(addr))
		return;

#ifdef DEBUGGER
	if (memwatch_access_validator) {
		write_log(_T("CIA word write %08x = %04x PC=%08x\n"), addr, v & 0xffff, M68K_GETPC);
	}
#endif

	if (addr & 1)
		v = (v << 8) | (v >> 8);

	int cs = cia_chipselect(addr);

	if (!issinglecia() || (cs & 3) != 0) {
		uae_u32 flags = 0;
		cia_wait_pre(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0));
		if ((cs & 2) == 0)
			WriteCIAB(r, v >> 8, &flags);
		if ((cs & 1) == 0)
			WriteCIAA(r, v & 0xff, &flags);
		cia_wait_post(((cs & 2) == 0 ? 1 : 0) | ((cs & 1) == 0 ? 2 : 0), addr, v, true);
		if (((cs & 3) == 3) && (warned > 0 || currprefs.illegal_mem)) {
			write_log(_T("cia_wput: unknown CIA address %08X=%04X %08X\n"), addr, v & 0xffff, M68K_GETPC);
			warned--;
		}
#ifdef ACTION_REPLAY
		if (flags) {
			action_replay_cia_access((flags & 2) != 0);
		}
#endif
	}
}

static void REGPARAM2 cia_lput(uaecptr addr, uae_u32 value)
{
	if (cia_debug(addr, value, sz_long))
		return;
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
	ABFLAG_IO, S_READ, S_WRITE, NULL, 0x3f, 0xdc0000
};

static uae_u8 getclockreg(int addr, struct tm *ct)
{
	uae_u8 v = 0;

	if (currprefs.cs_rtc == 1 || currprefs.cs_rtc == 3) { /* MSM6242B */
		return get_clock_msm(&rtc_msm, addr, ct);
	} else if (currprefs.cs_rtc == 2) { /* RF5C01A */
		return get_clock_ricoh(&rtc_ricoh, addr, ct);
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
	TCHAR path[MAX_DPATH];
	cfgfile_resolve_path_out_load(currprefs.rtcfile, path, MAX_DPATH, PATH_ROM);
	struct zfile *f = zfile_fopen(path, _T("wb"));
	if (f) {
		struct tm *ct;
		time_t t = time(0);
		t += currprefs.cs_rtc_adjust;
		ct = localtime(&t);
		uae_u8 od;
		if (currprefs.cs_rtc == 2) {
			od = rtc_ricoh.clock_control_d;
			rtc_ricoh.clock_control_d &= ~3;
		} else {
			od = rtc_msm.clock_control_d;
		}
		for (int i = 0; i < 13; i++) {
			uae_u8 v = getclockreg(i, ct);
			zfile_fwrite(&v, 1, 1, f);
		}
		if (currprefs.cs_rtc == 2) {
			rtc_ricoh.clock_control_d = od;
			zfile_fwrite(&rtc_ricoh.clock_control_d, 1, 1, f);
			zfile_fwrite(&rtc_ricoh.clock_control_e, 1, 1, f);
			zfile_fwrite(&rtc_ricoh.clock_control_f, 1, 1, f);
		} else {
			rtc_msm.clock_control_d = od;
			zfile_fwrite(&rtc_msm.clock_control_d, 1, 1, f);
			zfile_fwrite(&rtc_msm.clock_control_e, 1, 1, f);
			zfile_fwrite(&rtc_msm.clock_control_f, 1, 1, f);
		}
		if (currprefs.cs_rtc == 2) {
			zfile_fwrite(rtc_ricoh.rtc_alarm, RF5C01A_RAM_SIZE, 1, f);
			zfile_fwrite(rtc_ricoh.rtc_memory, RF5C01A_RAM_SIZE, 1, f);
		}
		zfile_fclose(f);
	}
}

void rtc_hardreset(void)
{
	if (currprefs.cs_rtc == 1 || currprefs.cs_rtc == 3) { /* MSM6242B */
		clock_bank.name = currprefs.cs_rtc == 1 ? _T("Battery backed up clock (MSM6242B)") : _T("Battery backed up clock A2000 (MSM6242B)");
		rtc_msm.clock_control_d = 0x1;
		rtc_msm.clock_control_e = 0;
		rtc_msm.clock_control_f = 0x4; /* 24/12 */
		rtc_msm.delayed_write = 0;
	} else if (currprefs.cs_rtc == 2) { /* RF5C01A */
		clock_bank.name = _T("Battery backed up clock (RF5C01A)");
		rtc_ricoh.clock_control_d = 0x8; /* Timer EN */
		rtc_ricoh.clock_control_e = 0;
		rtc_ricoh.clock_control_f = 0;
		memset(rtc_ricoh.rtc_memory, 0, RF5C01A_RAM_SIZE);
		memset(rtc_ricoh.rtc_alarm, 0, RF5C01A_RAM_SIZE);
		rtc_ricoh.rtc_alarm[10] = 1; /* 24H mode */
		rtc_ricoh.delayed_write = 0;
	}
	if (currprefs.rtcfile[0]) {
		TCHAR path[MAX_DPATH];
		cfgfile_resolve_path_out_load(currprefs.rtcfile, path, MAX_DPATH, PATH_ROM);
		struct zfile *f = zfile_fopen(path, _T("rb"));
		if (f) {
			int size = zfile_size32(f);
			uae_u8 empty[16];
			zfile_fread(empty, sizeof(empty), 1, f);
			if (size > 16) {
				rtc_ricoh.clock_control_d = empty[13];
				rtc_ricoh.clock_control_e = empty[14];
				rtc_ricoh.clock_control_f = empty[15];
				zfile_fread(rtc_ricoh.rtc_alarm, RF5C01A_RAM_SIZE, 1, f);
				zfile_fread(rtc_ricoh.rtc_memory, RF5C01A_RAM_SIZE, 1, f);
			} else if (size == 16) {
				rtc_msm.clock_control_d = empty[13];
				rtc_msm.clock_control_e = empty[14];
				rtc_msm.clock_control_f = empty[15];
			}
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
		return dummy_get_safe(addr, 1, false, v);
	}
	time_t t = time(0);
	t += currprefs.cs_rtc_adjust;
	ct = localtime(&t);
	addr >>= 2;
	return getclockreg(addr, ct);
}

static void cputester_event(uae_u32 v)
{
	IRQ_forced(4, 28 / 2);
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

	if (currprefs.cputester && (addr & 65535) == 0) {
		event2_newevent_xx(-1, CYCLE_UNIT * (64 / 2 - 1), 0, cputester_event);
	}

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
		put_clock_msm(&rtc_msm, addr, value);
	} else if (currprefs.cs_rtc == 2) { /* RF5C01A */
		put_clock_ricoh(&rtc_ricoh, addr, value);
	}
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
	//dumpcia ();
	DISK_select_set(cia[1].prb);
}

uae_u8 *restore_cia(int num, uae_u8 *src)
{
	struct CIA *c = &cia[num];
	uae_u8 b;
	uae_u16 w;
	uae_u32 l;

	/* CIA registers */
	c->pra = restore_u8();				/* 0 PRA */
	c->prb = restore_u8();				/* 1 PRB */
	c->dra = restore_u8();				/* 2 DDRA */
	c->drb = restore_u8();				/* 3 DDRB */
	c->t[0].timer = restore_u16();		/* 4 TA */
	c->t[1].timer = restore_u16();		/* 6 TB */
	l = restore_u8();					/* 8/9/A TOD */
	l |= restore_u8() << 8;
	l |= restore_u8() << 16;
	c->tod = l;
	restore_u8();						/* B unused */
	c->sdr = restore_u8();				/* C SDR */
	c->icr2 = c->icr1 = restore_u8();	/* D ICR INFORMATION (not mask!) */
	c->t[0].cr = restore_u8();			/* E CRA */
	c->t[1].cr = restore_u8();			/* F CRB */

	/* CIA internal data */

	c->imask = restore_u8();			/* ICR MASK */
	w = restore_u8();					/* timer A latch */
	w |= restore_u8() << 8;
	c->t[0].latch = w;
	w = restore_u8();					/* timer B latch */
	w |= restore_u8() << 8;
	c->t[1].latch = w;
	l = restore_u8();					/* TOD latched value */
	l |= restore_u8() << 8;
	l |= restore_u8() << 16;
	c->tol = w;
	l = restore_u8();					/* alarm */
	l |= restore_u8() << 8;
	l |= restore_u8() << 16;
	c->alarm = l;
	b = restore_u8();
	c->tlatch = (b & 1) != 0;			/* flags */
	c->todon = (b & 2) != 0;
	c->sdr_load = (b & 4) != 0;
	b = restore_u8();
//	if (num)
//		div10 = CYCLE_UNIT * b;
	c->sdr_cnt = restore_u8();
	c->sdr_buf = restore_u8();

	// 4.9.2+
	c->icr1 = c->icr2;
	c->t[0].loaddelay = 0;
	c->t[0].inputpipe = 0;
	if (c->t[0].cr & CR_START) {
		c->t[0].inputpipe = CIA_PIPE_ALL_MASK;
	}
	c->t[1].loaddelay = 0;
	c->t[1].inputpipe = 0;
	if (c->t[1].cr & CR_START) {
		c->t[1].inputpipe = CIA_PIPE_ALL_MASK;
	}
	if (restore_u8() & 1) {
		c->icr1 = restore_u8();
		c->t[0].inputpipe = restore_u16();
		c->t[1].inputpipe = restore_u16();
		c->t[0].loaddelay = restore_u32();
		c->t[1].loaddelay = restore_u32();
		changed_prefs.cs_eclockphase = currprefs.cs_eclockphase = restore_u16();
		internaleclockphase = restore_u16();

	}

	return src;
}

uae_u8 *save_cia(int num, size_t *len, uae_u8 *dstptr)
{
	struct CIA *c = &cia[num];
	uae_u8 *dstbak,*dst, b;
	uae_u16 t;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);

	save_cia_prepare();

	/* CIA registers */

	save_u8(c->pra);					/* 0 PRA */
	save_u8(c->prb);					/* 1 PRB */
	save_u8(c->dra);					/* 2 DDRA */
	save_u8(c->drb);					/* 3 DDRB */
	t = c->t[0].timer - c->t[0].passed;	/* 4 TA */
	save_u16(t);
	t = c->t[1].timer - c->t[1].passed;	/* 6 TB */
	save_u16(t);
	save_u8((uae_u8)c->tod);			/* 8 TODL */
	save_u8((uae_u8)(c->tod >> 8));		/* 9 TODM */
	save_u8((uae_u8)(c->tod >> 16));	/* A TODH */
	save_u8(0);							/* B unused */
	save_u8(c->sdr);					/* C SDR */
	save_u8(c->icr2);					/* D ICR INFORMATION (not mask!) */
	save_u8(c->t[0].cr);				/* E CRA */
	save_u8(c->t[1].cr);				/* F CRB */

	/* CIA internal data */

	save_u8(c->imask);					/* ICR */
	save_u8((uae_u8)c->t[0].latch);		/* timer A latch LO */
	save_u8(c->t[0].latch >> 8);		/* timer A latch HI */
	save_u8((uae_u8)c->t[1].latch);		/* timer B latch LO */
	save_u8(c->t[1].latch >> 8);		/* timer B latch HI */
	save_u8(c->tol);					/* latched TOD LO */
	save_u8(c->tol >> 8);				/* latched TOD MED */
	save_u8(c->tol >> 16);				/* latched TOD HI */
	save_u8(c->alarm);					/* alarm LO */
	save_u8(c->alarm >> 8);				/* alarm MED */
	save_u8(c->alarm >> 16);			/* alarm HI */
	b = 0;
	b |= c->tlatch ? 1 : 0;				/* is TOD latched? */
	b |= c->todon ? 2 : 0;				/* TOD stopped? */
	b |= c->sdr_load ? 4 : 0;			/* SDR loaded */
	save_u8(b);
	save_u8(0); // save_u8(num ? div10 / CYCLE_UNIT : 0);
	save_u8(c->sdr_cnt);
	save_u8(c->sdr_buf);

	// 4.9.2+
	save_u8(1);
	save_u8(c->icr1);
	save_u16(c->t[0].inputpipe);
	save_u16(c->t[1].inputpipe);
	save_u32(c->t[0].loaddelay);
	save_u32(c->t[1].loaddelay);
	save_u16(currprefs.cs_eclockphase);
	save_u16(internaleclockphase);

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_keyboard(size_t *len, uae_u8 *dstptr)
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
