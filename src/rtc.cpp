/*
* UAE - The Un*x Amiga Emulator
*
* RTC chips
*
* The MSM6242B and RF5C01A keep their own time: the counters are seeded
* from the host clock (+ cs_rtc_adjust) at the first access, advance with
* emulated time (the chip's 32.768 kHz time base is derived from the
* chipset clock), and take guest writes, HOLD, STOP and the divider reset
* like the chips do.
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "custom.h"
#include "events.h"

#include "rtc.h"

// MSM6242B CD BUSY: about 190 us around each second carry (data sheet)
#define RTC_BUSY_US 190

static uae_s64 rtc_cycles_per_second(void)
{
	return (uae_s64)(currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL) * CYCLE_UNIT;
}

void rtc_counter_reset(struct rtc_counter *c)
{
	c->init = false;
}

static void rtc_counter_init(struct rtc_counter *c, bool ricoh)
{
	time_t t = time(0);
	t += currprefs.cs_rtc_adjust;
	struct tm *ct = localtime(&t);
	c->sec = ct->tm_sec > 59 ? 59 : ct->tm_sec;
	c->min = ct->tm_min;
	c->hour = ct->tm_hour;
	c->mday = ct->tm_mday;
	c->mon = ct->tm_mon + 1;
	c->wday = ct->tm_wday;
	// MSM: year - 1900, as before (Kickstart decodes both encodings to the
	// same year); RF5C01A: the two-digit year and its leap counter
	c->year = ricoh ? ct->tm_year % 100 : ct->tm_year;
	c->leap = (ct->tm_year + 1900) & 3;
	c->last = get_cycles();
	c->phase = 0;
	c->pending = false;
	c->h1_pending = false;
	c->init = true;
}

static int rtc_days_in_month(const struct rtc_counter *c, bool ricoh)
{
	static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int m = c->mon;
	if (m < 1 || m > 12)
		return 31;
	if (m == 2) {
		bool leap = ricoh ? c->leap == 0 : (c->year % 4) == 0;
		return leap ? 29 : 28;
	}
	return days[m - 1];
}

static void rtc_tick(struct rtc_counter *c, bool ricoh)
{
	if (++c->sec < 60)
		return;
	c->sec = 0;
	if (++c->min < 60)
		return;
	c->min = 0;
	if (++c->hour < 24)
		return;
	c->hour = 0;
	c->wday = (c->wday + 1) % 7;
	if (++c->mday <= rtc_days_in_month(c, ricoh))
		return;
	c->mday = 1;
	if (++c->mon <= 12)
		return;
	c->mon = 1;
	if (ricoh) {
		c->year = (c->year + 1) % 100;
		c->leap = (c->leap + 1) & 3;
	} else {
		// two BCD digits count 99 -> 00; a seeded value above 99
		// (year - 1900) keeps counting
		c->year = c->year == 99 ? 0 : c->year + 1;
	}
}

// Advance the counters to now. stopped: the time base does not run
// (MSM STOP, RF5C01A TIMER EN = 0); hold: MSM HOLD, carries are held back
// (at most one, applied when HOLD is released).
static void rtc_update(struct rtc_counter *c, bool ricoh, bool stopped, bool hold)
{
	if (!c->init) {
		rtc_counter_init(c, ricoh);
		return;
	}
	uae_s64 now = get_cycles();
	uae_s64 elapsed = now - c->last;
	c->last = now;
	if (stopped || elapsed <= 0)
		return;
	uae_s64 cps = rtc_cycles_per_second();
	c->phase += elapsed;
	while (c->phase >= cps) {
		c->phase -= cps;
		if (hold) {
			c->pending = true;
		} else {
			rtc_tick(c, ricoh);
		}
	}
}

static void rtc_release_hold(struct rtc_counter *c, bool ricoh)
{
	if (c->pending) {
		c->pending = false;
		rtc_tick(c, ricoh);
	}
}

static bool rtc_busy(const struct rtc_counter *c)
{
	return c->phase < rtc_cycles_per_second() / 1000000 * RTC_BUSY_US;
}

// 12-hour format: 12, 1..11, PM flag
static int rtc_hour12(int hour) { int h = hour % 12; return h ? h : 12; }

static void rtc_set_hour12(struct rtc_counter *c, int h12, bool pm)
{
	c->hour = (h12 % 12) + (pm ? 12 : 0);
}

// 12-hour mode: H1 and H10 are separate registers, so H1 followed by H10
// can pass through an invalid value (e.g. 12 -> 13 -> 03). Keep a written
// H1 digit until H10 comes; apply whatever combination is a valid hour.
static void rtc_write_h1_12(struct rtc_counter *c, uae_u8 v)
{
	int h12 = (rtc_hour12(c->hour) / 10) * 10 + v;
	c->h1_pending = true;
	c->h1_digit = v;
	if (h12 >= 1 && h12 <= 12)
		rtc_set_hour12(c, h12, c->hour >= 12);
}

static void rtc_write_h10_12(struct rtc_counter *c, uae_u8 v, bool pm)
{
	int ones = c->h1_pending ? c->h1_digit : rtc_hour12(c->hour) % 10;
	int h12 = (v & 1) * 10 + ones;
	c->h1_pending = false;
	if (h12 >= 1 && h12 <= 12)
		rtc_set_hour12(c, h12, pm);
	else
		rtc_set_hour12(c, rtc_hour12(c->hour), pm);
}

static void rtc_set_digit(int *field, int digit, bool tens, uae_u8 v)
{
	(void)digit;
	if (tens)
		*field = v * 10 + *field % 10;
	else
		*field = (*field / 10) * 10 + v;
}

/* ---- MSM6242B
 * CD: bit 0 HOLD, bit 1 BUSY (read only), bit 2 IRQ FLAG, bit 3 30S ADJ
 * CF: bit 0 REST (divider reset), bit 1 STOP, bit 2 24/12, bit 3 TEST */

static bool msm_stopped(const struct rtc_msm_data *data) { return (data->clock_control_f & 2) != 0; }
static bool msm_24h(const struct rtc_msm_data *data) { return (data->clock_control_f & 4) != 0; }

static void msm_update(struct rtc_msm_data *data)
{
	// The reset value of CD has HOLD set; it only takes effect once the
	// guest writes CD, so a clock nothing touches keeps counting.
	bool hold = (data->clock_control_d & 1) && data->cnt.control_written;
	rtc_update(&data->cnt, false, msm_stopped(data), hold);
	if (data->clock_control_f & 1) // REST: the divider is held in reset
		data->cnt.phase = 0;
}

uae_u8 get_clock_msm(struct rtc_msm_data *data, int addr, struct tm *ct)
{
	(void)ct;
	struct rtc_counter *c = &data->cnt;
	uae_u8 v = 0;
	int year;

	msm_update(data);
	year = c->year;
	if (data->yearmode && year >= 100)
		year -= 100;

	switch (addr)
	{
		case 0x0: v = c->sec % 10; break;
		case 0x1: v = c->sec / 10; break;
		case 0x2: v = c->min % 10; break;
		case 0x3: v = c->min / 10; break;
		case 0x4:
			v = msm_24h(data) ? c->hour % 10 : rtc_hour12(c->hour) % 10;
			break;
		case 0x5:
			if (msm_24h(data)) {
				v = c->hour / 10;
			} else {
				v = rtc_hour12(c->hour) / 10;
				v |= c->hour >= 12 ? 4 : 0; // PM bit
			}
			break;
		case 0x6: v = c->mday % 10; break;
		case 0x7: v = c->mday / 10; break;
		case 0x8: v = c->mon % 10; break;
		case 0x9: v = c->mon / 10; break;
		case 0xA: v = year % 10; break;
		case 0xB: v = (year / 10) & 0x0f;  break;
		case 0xC: v = c->wday; break;
		case 0xD:
			v = data->clock_control_d & ~2;
			if (!msm_stopped(data) && rtc_busy(c))
				v |= 2; // BUSY
			break;
		case 0xE: v = data->clock_control_e; break;
		case 0xF: v = data->clock_control_f; break;
	}
	return v;
}

bool put_clock_msm(struct rtc_msm_data *data, int addr, uae_u8 v)
{
	struct rtc_counter *c = &data->cnt;

	v &= 0x0f;
	msm_update(data);
	switch (addr)
	{
		case 0x0: rtc_set_digit(&c->sec, 0, false, v); break;
		case 0x1: rtc_set_digit(&c->sec, 1, true, v & 7); break;
		case 0x2: rtc_set_digit(&c->min, 0, false, v); break;
		case 0x3: rtc_set_digit(&c->min, 1, true, v & 7); break;
		case 0x4:
			if (msm_24h(data))
				rtc_set_digit(&c->hour, 0, false, v);
			else
				rtc_write_h1_12(c, v);
			break;
		case 0x5:
			if (msm_24h(data))
				rtc_set_digit(&c->hour, 1, true, v & 3);
			else
				rtc_write_h10_12(c, v, (v & 4) != 0);
			break;
		case 0x6: rtc_set_digit(&c->mday, 0, false, v); break;
		case 0x7: rtc_set_digit(&c->mday, 1, true, v & 3); break;
		case 0x8: rtc_set_digit(&c->mon, 0, false, v); break;
		case 0x9: rtc_set_digit(&c->mon, 1, true, v & 1); break;
		case 0xA: rtc_set_digit(&c->year, 0, false, v); break;
		case 0xB: rtc_set_digit(&c->year, 1, true, v); break;
		case 0xC: c->wday = v & 7; break;
		case 0xD:
			v &= 1 | 8; // BUSY and IRQ FLAG are not written here
			c->control_written = true;
			if (v & 8) {
				// 30 SEC ADJ: 00-29 s -> 00, 30-59 s -> 00 of the next
				// minute; the bit clears itself
				if (c->sec >= 30) {
					c->sec = 59;
					rtc_tick(c, false);
				} else {
					c->sec = 0;
				}
				v &= ~8;
			}
			if ((data->clock_control_d & 1) && !(v & 1))
				rtc_release_hold(c, false);
			if (data->clock_control_d != v) {
				data->clock_control_d = v;
				data->delayed_write = -1;
			}
			break;
		case 0xE:
			if (data->clock_control_e != v) {
				data->clock_control_e = v;
				data->delayed_write = -1;
			}
			break;
		case 0xF:
			if (v & 1) // REST: the divider is reset
				c->phase = 0;
			if (data->clock_control_f != v) {
				data->clock_control_f = v;
				data->delayed_write = -1;
			}
			break;
	}
	if (addr < 0xD)
		data->delayed_write = -1;
	return false;
}

/* ---- RF5C01A
 * D (mode): bits 0-1 bank, bit 2 ALARM EN, bit 3 TIMER EN
 * F (reset): bit 0 alarm reset, bit 1 timer reset (divider), bits 2-3 16/1 Hz
 * bank 1: alarm registers; A bit 0 = 24h, B bits 0-1 = leap year counter */

static bool ricoh_stopped(const struct rtc_ricoh_data *data) { return !(data->clock_control_d & 8); }
static bool ricoh_24h(const struct rtc_ricoh_data *data) { return (data->rtc_alarm[10] & 1) != 0; }

uae_u8 get_clock_ricoh(struct rtc_ricoh_data *data, int addr, struct tm *ct)
{
	(void)ct;
	struct rtc_counter *c = &data->cnt;
	uae_u8 v = 0;
	int bank = data->clock_control_d & 3;

	rtc_update(c, true, ricoh_stopped(data), false);

	/* memory access */
	if (bank >= 2 && addr < 0x0d)
		return (data->rtc_memory[addr] >> ((bank == 2) ? 0 : 4)) & 0x0f;
	/* alarm */
	if (bank == 1 && addr < 0x0d) {
		if (addr == 11)
			return c->leap & 3;
		v = data->rtc_alarm[addr];
#if CLOCK_DEBUG
		write_log (_T("CLOCK ALARM R %X: %X\n"), addr, v);
#endif
		return v;
	}
	switch (addr)
	{
		case 0x0: v = c->sec % 10; break;
		case 0x1: v = c->sec / 10; break;
		case 0x2: v = c->min % 10; break;
		case 0x3: v = c->min / 10; break;
		case 0x4:
			v = ricoh_24h(data) ? c->hour % 10 : rtc_hour12(c->hour) % 10;
			break;
		case 0x5:
			if (ricoh_24h(data))
				v = c->hour / 10; // 24h
			else
				v = (rtc_hour12(c->hour) / 10) | (c->hour >= 12 ? 2 : 0); // 12h
		break;
		case 0x6: v = c->wday; break;
		case 0x7: v = c->mday % 10; break;
		case 0x8: v = c->mday / 10; break;
		case 0x9: v = c->mon % 10; break;
		case 0xA: v = c->mon / 10; break;
		case 0xB: v = c->year % 10; break;
		case 0xC: v = (c->year / 10) % 10; break;
		case 0xD: v = data->clock_control_d; break;
		/* E and F = write-only, reads as zero */
		case 0xE: v = 0; break;
		case 0xF: v = 0; break;
	}
	return v;
}

void put_clock_ricoh(struct rtc_ricoh_data *data, int addr, uae_u8 v)
{
	struct rtc_counter *c = &data->cnt;
	int bank = data->clock_control_d & 3;

	rtc_update(c, true, ricoh_stopped(data), false);

	/* memory access */
	if (bank >= 2 && addr < 0x0d) {
		uae_u8 ov = data->rtc_memory[addr];
		data->rtc_memory[addr] &= ((bank == 2) ? 0xf0 : 0x0f);
		data->rtc_memory[addr] |= v << ((bank == 2) ? 0 : 4);
		if (data->rtc_memory[addr] != ov)
			data->delayed_write = -1;
		return;
	}
	/* alarm */
	if (bank == 1 && addr < 0x0d) {
#if CLOCK_DEBUG
		write_log (_T("CLOCK ALARM W %X: %X\n"), addr, value);
#endif
		if (addr == 11) {
			c->leap = v & 3;
			data->delayed_write = -1;
			return;
		}
		uae_u8 ov = data->rtc_alarm[addr];
		data->rtc_alarm[addr] = v;
		data->rtc_alarm[0] = data->rtc_alarm[1] = data->rtc_alarm[9] = data->rtc_alarm[12] = 0;
		data->rtc_alarm[3] &= ~0x8;
		data->rtc_alarm[5] &= ~0xc;
		data->rtc_alarm[6] &= ~0x8;
		data->rtc_alarm[8] &= ~0xc;
		data->rtc_alarm[10] &= ~0xe;
		data->rtc_alarm[11] &= ~0xc;
		if (data->rtc_alarm[addr] != ov)
			data->delayed_write = -1;
		return;
	}
#if CLOCK_DEBUG
	write_log (_T("CLOCK W %X: %X\n"), addr, value);
#endif
	switch (addr)
	{
		case 0x0: rtc_set_digit(&c->sec, 0, false, v); break;
		case 0x1: rtc_set_digit(&c->sec, 1, true, v & 7); break;
		case 0x2: rtc_set_digit(&c->min, 0, false, v); break;
		case 0x3: rtc_set_digit(&c->min, 1, true, v & 7); break;
		case 0x4:
			if (ricoh_24h(data))
				rtc_set_digit(&c->hour, 0, false, v);
			else
				rtc_write_h1_12(c, v);
			break;
		case 0x5:
			if (ricoh_24h(data))
				rtc_set_digit(&c->hour, 1, true, v & 3);
			else
				rtc_write_h10_12(c, v, (v & 2) != 0);
			break;
		case 0x6: c->wday = v & 7; break;
		case 0x7: rtc_set_digit(&c->mday, 0, false, v); break;
		case 0x8: rtc_set_digit(&c->mday, 1, true, v & 3); break;
		case 0x9: rtc_set_digit(&c->mon, 0, false, v); break;
		case 0xA: rtc_set_digit(&c->mon, 1, true, v & 1); break;
		case 0xB: rtc_set_digit(&c->year, 0, false, v); break;
		case 0xC: rtc_set_digit(&c->year, 1, true, v); break;
		case 0xD:
			if (data->clock_control_d != v) {
				data->clock_control_d = v;
				data->delayed_write = -1;
			}
			break;
		case 0xE:
			if (data->clock_control_e != v) {
				data->clock_control_e = v;
				data->delayed_write = -1;
			}
			break;
		case 0xF:
			if (v & 2) // timer reset: the divider restarts
				c->phase = 0;
			if (data->clock_control_f != v) {
				data->clock_control_f = v;
				data->delayed_write = -1;
			}
			break;
	}
	if (addr < 0xD)
		data->delayed_write = -1;
}
