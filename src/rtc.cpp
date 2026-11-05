/*
* UAE - The Un*x Amiga Emulator
*
* RTC chips
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "custom.h"
#include "events.h"

#include "rtc.h"

#include <algorithm>
#include <chrono>

// The RTC counts emulated time: on the real machine its 32.768 kHz crystal
// and the colour clock both keep real time, so a guest timing one against
// the other (seconds digit against CIA or beam) must see them agree also
// when the emulation runs slower or faster than real time. It starts at the
// host clock and re-anchors to it (rtc_rebase) on reset, state restore,
// unpause and warp changes; warp itself keeps the host clock. While running
// it slews towards the host clock by at most 1 % once they are more than
// 1 s apart, so the Amiga clock follows the host's over a long session; a
// host clock jump of over an hour (suspend, NTP) re-anchors at once.
struct rtc_timebase {
	uae_s64 sample_us;
	uae_u64 cycles, remainder;
	int clock_hz;
	bool initialized;
};

static uae_s64 rtc_project_microseconds(struct rtc_timebase *base, uae_s64 host_us,
	uae_u64 cycles, int clock_hz, bool warp)
{
	const uae_s64 slew_threshold_us = 1000000, reanchor_us = 3600LL * 1000000;
	const uae_u64 frequency = (uae_u64)clock_hz * CYCLE_UNIT;
	if (!base->initialized || warp || base->clock_hz != clock_hz || cycles < base->cycles) {
		base->sample_us = host_us;
		base->remainder = 0;
	} else {
		const uae_u64 delta = cycles - base->cycles;
		// whole seconds first: delta * 1000000 overflows after ~3 h
		const uae_u64 frac = (delta % frequency) * 1000000 + base->remainder;
		const uae_s64 elapsed_us = (uae_s64)(delta / frequency * 1000000 + frac / frequency);
		base->remainder = frac % frequency;
		base->sample_us += elapsed_us;
		const uae_s64 drift = host_us - base->sample_us;
		if (drift > reanchor_us || drift < -reanchor_us) {
			base->sample_us = host_us;
			base->remainder = 0;
		} else if (drift > slew_threshold_us || drift < -slew_threshold_us) {
			const uae_s64 slew = std::min(elapsed_us / 100, (drift > 0 ? drift : -drift) - slew_threshold_us);
			base->sample_us += drift > 0 ? slew : -slew;
		}
	}
	base->cycles = cycles;
	base->clock_hz = clock_hz;
	base->initialized = true;
	return base->sample_us;
}

static struct rtc_timebase rtc_base;

void rtc_rebase(void)
{
	rtc_base.initialized = false;
}

static uae_s64 rtc_microseconds(void)
{
	uae_s64 host_us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	return rtc_project_microseconds(&rtc_base, host_us, get_cycles(),
		currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL,
		currprefs.turbo_emulation != 0);
}

static void clock_advance(struct rtc_clock_data *clock, uae_s64 seconds, uae_u8 *leap)
{
	struct tm *ct = &clock->value;
	clock->hour12_written = false;
	seconds += ct->tm_sec;
	ct->tm_sec = seconds % 60;
	seconds = seconds / 60 + ct->tm_min;
	ct->tm_min = seconds % 60;
	seconds = seconds / 60 + ct->tm_hour;
	ct->tm_hour = seconds % 24;
	uae_s64 days = seconds / 24;
	ct->tm_wday = (ct->tm_wday + days) % 7;
	while (days > 0) {
		int length = 31;
		switch (ct->tm_mon) {
			case 3: case 5: case 8: case 10: length = 30; break;
			case 1: length = 28 + (leap ? (*leap & 3) == 0 : (ct->tm_year % 4) == 0); break;
		}
		uae_s64 left = length - ct->tm_mday + 1;
		if (left > 0 && days < left) {
			ct->tm_mday += days;
			break;
		}
		days -= left > 0 ? left : 1;
		ct->tm_mday = 1;
		if (++ct->tm_mon >= 12) {
			ct->tm_mon = 0;
			ct->tm_year++;
			if (leap)
				*leap = (*leap + 1) & 3;
		}
	}
}

static struct tm *clock_sample(struct rtc_clock_data *clock, bool running, uae_u8 *leap, bool hold = false)
{
	uae_s64 now = rtc_microseconds();
	if (!clock->initialized || (!clock->written && running && !hold)) {
		time_t t = now / 1000000 + currprefs.cs_rtc_adjust;
		clock->value = *localtime(&t);
		clock->hour12_written = false;
		if (!clock->initialized)
			clock->held_value = clock->value;
		clock->phase_us = now % 1000000;
		if (leap)
			*leap = clock->value.tm_year & 3;
		clock->initialized = true;
	} else if (running && now > clock->last_us) {
		uae_s64 elapsed = now - clock->last_us + clock->phase_us;
		if (elapsed >= 1000000) {
			if (hold)
				clock->carry_pending = true;
			else
				clock_advance(clock, elapsed / 1000000, leap);
		}
		clock->phase_us = elapsed % 1000000;
	}
	clock->last_us = now;
	return &clock->value;
}

static int clock_hour12(int hour)
{
	int value = hour % 12;
	return value ? value : 12;
}

static void clock_write_digit(struct rtc_clock_data *clock, int addr, uae_u8 v, bool mode24, int pm)
{
	struct tm *ct = &clock->value;
	int *field = NULL, value = 0, offset = 0;
	switch (addr) {
		case 0: case 1: field = &ct->tm_sec; break;
		case 2: case 3: field = &ct->tm_min; break;
		case 4: case 5:
			if (mode24) {
				clock->hour12_written = false;
				field = &ct->tm_hour;
			} else {
				int hour = clock->hour12_written ? clock->hour12 : clock_hour12(ct->tm_hour);
				if (addr == 4)
					hour = hour / 10 * 10 + v;
				else
					hour = (v & 1) * 10 + hour % 10;
				clock->hour12 = hour;
				clock->hour12_written = true;
				ct->tm_hour = hour % 12 + ((addr == 5 ? (v & pm) != 0 : ct->tm_hour >= 12) ? 12 : 0);
			}
			break;
		case 6: case 7: field = &ct->tm_mday; break;
		case 8: case 9: field = &ct->tm_mon; offset = 1; break;
		case 10: case 11:
			field = &ct->tm_year;
			offset = -(ct->tm_year / 100 * 100);
			break;
		case 12: ct->tm_wday = v & 7; break;
	}
	if (field) {
		value = *field + offset;
		*field = ((addr & 1) ? v * 10 + value % 10 : value / 10 * 10 + v) - offset;
	}
	clock->written = true;
}

// A clock restored from a savestate runs on from now: the time it was
// saved is its last sample (no jump for the time the snapshot was stored).
void rtc_clock_resume(struct rtc_clock_data *clock)
{
	clock->last_us = rtc_microseconds();
}

// The calendar digits of an rtcfile (registers 0-12, as write_battclock()
// stored them age_us ago), restored into a clock that keeps running while
// the emulator is off, like the real battery-backed chip. A calendar that
// was the host's (the guest never set it, or a file from before the RTC
// kept its own time) leaves the clock following the host clock.
static bool clock_restore_calendar(struct rtc_clock_data *clock, const uae_u8 *digits,
	bool ricoh, bool mode24, uae_s64 age_us, bool running, bool hold, uae_u8 *leap)
{
	// register index of each field: MSM6242B, RF5C01A (weekday at 6)
	static const int msm[13] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
	static const int rf[13] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 6 };
	const int *r = ricoh ? rf : msm;
	int d[13];
	for (int i = 0; i < 13; i++)
		d[i] = digits[r[i]] & 15;
	struct tm t = {};
	t.tm_sec = d[1] * 10 + d[0];
	t.tm_min = d[3] * 10 + d[2];
	if (mode24) {
		t.tm_hour = (d[5] & 3) * 10 + d[4];
	} else {
		const int pm = ricoh ? 2 : 4;
		const int h12 = (d[5] & 1) * 10 + d[4];
		if (h12 < 1 || h12 > 12)
			return false;
		t.tm_hour = h12 % 12 + ((d[5] & pm) ? 12 : 0);
	}
	t.tm_mday = d[7] * 10 + d[6];
	t.tm_mon = d[9] * 10 + d[8] - 1;
	const int year = d[11] * 10 + d[10];
	t.tm_year = year < 78 ? year + 100 : year;  // battclock: 78-99 19xx, 00-77 20xx
	t.tm_wday = d[12] % 7;
	if (t.tm_sec > 59 || t.tm_min > 59 || t.tm_hour > 23 || t.tm_mday < 1 || t.tm_mday > 31
		|| t.tm_mon < 0 || t.tm_mon > 11 || year > 99)
		return false;
	struct tm local = t;
	local.tm_isdst = -1;
	const time_t guest = mktime(&local);
	if (guest == (time_t)-1)
		return false;
	const uae_s64 now = rtc_microseconds();
	const uae_s64 written = (now - age_us) / 1000000 + currprefs.cs_rtc_adjust;  // host calendar then
	const uae_s64 offset = (uae_s64)guest - written;
	if (running && !hold && offset >= -2 && offset <= 2
		&& t.tm_wday == local.tm_wday && (!leap || (*leap & 3) == (t.tm_year & 3)))
		return false;  // the host's calendar: keep following it
	clock->value = t;
	clock->phase_us = running ? now % 1000000 : 0;
	clock->last_us = now;
	clock->hour12_written = false;
	clock->carry_pending = false;
	if (running) {
		const uae_s64 seconds = now / 1000000 - (now - age_us) / 1000000;
		if (hold)
			clock->carry_pending = seconds > 0;
		else
			clock_advance(clock, seconds, leap);
	}
	clock->held_value = clock->value;
	clock->initialized = true;
	clock->written = true;
	return true;
}

bool rtc_restore_calendar(struct rtc_msm_data *data, const uae_u8 *digits, uae_s64 age_us)
{
	return clock_restore_calendar(&data->clock, digits, false, (data->clock_control_f & 4) != 0,
		age_us, !(data->clock_control_f & 3), (data->clock_control_d & 1) != 0, NULL);
}

bool rtc_restore_calendar(struct rtc_ricoh_data *data, const uae_u8 *digits, uae_s64 age_us)
{
	return clock_restore_calendar(&data->clock, digits, true, (data->rtc_alarm[10] & 1) != 0,
		age_us, (data->clock_control_d & 8) != 0, false, &data->rtc_alarm[11]);
}

uae_u8 get_clock_msm(struct rtc_msm_data *data, int addr, struct tm *ct)
{
	uae_u8 v = 0;
	int year;

	if (!ct) {
		ct = clock_sample(&data->clock, !(data->clock_control_f & 3), NULL,
			(data->clock_control_d & 1) != 0);
		if (data->clock_control_d & 1)
			ct = &data->clock.held_value;
	}
	year = ct->tm_year % 100;

	switch (addr)
	{
		case 0x0: v = ct->tm_sec % 10; break;
		case 0x1: v = ct->tm_sec / 10; break;
		case 0x2: v = ct->tm_min % 10; break;
		case 0x3: v = ct->tm_min / 10; break;
		case 0x4:
			if (data->clock_control_f & 4)
				v = ct->tm_hour % 10; // 24h
			else
				v = clock_hour12(ct->tm_hour) % 10; // 12h
			break;
		case 0x5:
			if (data->clock_control_f & 4) {
				v = ct->tm_hour / 10; // 24h
			} else {
				v = clock_hour12(ct->tm_hour) / 10; // 12h
				v |= ct->tm_hour >= 12 ? 4 : 0; // AM/PM bit
			}
			break;
		case 0x6: v = ct->tm_mday % 10; break;
		case 0x7: v = ct->tm_mday / 10; break;
		case 0x8: v = (ct->tm_mon + 1) % 10; break;
		case 0x9: v = (ct->tm_mon + 1) / 10; break;
		case 0xA: v = year % 10; break;
		case 0xB: v = (year / 10) & 0x0f;  break;
		case 0xC: v = ct->tm_wday; break;
		case 0xD:
			v = data->clock_control_d;
			if (!(data->clock_control_f & 3) && !(v & 1) && data->clock.phase_us < 244)
				v |= 2;
			break;
		case 0xE: v = data->clock_control_e; break;
		case 0xF: v = data->clock_control_f; break;
	}
	return v;
}

bool put_clock_msm(struct rtc_msm_data *data, int addr, uae_u8 v)
{
	v &= 15;
	clock_sample(&data->clock, !(data->clock_control_f & 3), NULL,
		(data->clock_control_d & 1) != 0);
	if (addr < 13) {
		clock_write_digit(&data->clock, addr, v, (data->clock_control_f & 4) != 0, 4);
		if (data->clock_control_d & 1)
			data->clock.held_value = data->clock.value;
		data->delayed_write = -1;
		return false;
	}
	switch (addr)
	{
		case 0xD:
			if (!(v & 1) && (data->clock_control_d & 1)) {
				if (data->clock.carry_pending)
					clock_advance(&data->clock, 1, NULL);
				data->clock.carry_pending = false;
			}
			if (v & 8) {
				int seconds = data->clock.value.tm_sec;
				data->clock.value.tm_sec = 0;
				if (seconds >= 30)
					clock_advance(&data->clock, 60, NULL);
				data->clock.phase_us = 0;
				data->clock.written = true;
			}
			if (v & 1)
				data->clock.held_value = data->clock.value;
			if (data->clock_control_d != (v & 1)) {
				data->clock_control_d = v & 1;
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
			if ((data->clock_control_f ^ v) & 4)
				data->clock.hour12_written = false;
			if (v & 3)
				data->clock.written = true;
			if (v & 1) {
				data->clock.phase_us = 0;
				data->clock.carry_pending = false;
			}
			if (data->clock_control_f != v) {
				data->clock_control_f = v;
				data->delayed_write = -1;
			}
		break;
	}
	return false;
}

uae_u8 get_clock_ricoh(struct rtc_ricoh_data *data, int addr, struct tm *ct)
{
	uae_u8 v = 0;
	int bank = data->clock_control_d & 3;

	if (!ct)
		ct = clock_sample(&data->clock, (data->clock_control_d & 8) != 0, &data->rtc_alarm[11]);

	/* memory access */
	if (bank >= 2 && addr < 0x0d)
		return (data->rtc_memory[addr] >> ((bank == 2) ? 0 : 4)) & 0x0f;
	/* alarm */
	if (bank == 1 && addr < 0x0d) {
		v = data->rtc_alarm[addr];
#if CLOCK_DEBUG
		write_log (_T("CLOCK ALARM R %X: %X\n"), addr, v);
#endif
		return v;
	}
	switch (addr)
	{
		case 0x0: v = ct->tm_sec % 10; break;
		case 0x1: v = ct->tm_sec / 10; break;
		case 0x2: v = ct->tm_min % 10; break;
		case 0x3: v = ct->tm_min / 10; break;
		case 0x4:
			if (data->rtc_alarm[10] & 1)
				v = ct->tm_hour % 10; // 24h
			else
				v = clock_hour12(ct->tm_hour) % 10; // 12h
			break;
		case 0x5:
			if (data->rtc_alarm[10] & 1)
				v = ct->tm_hour / 10; // 24h
			else
				v = (clock_hour12(ct->tm_hour) / 10) | (ct->tm_hour >= 12 ? 2 : 0); // 12h
		break;
		case 0x6: v = ct->tm_wday; break;
		case 0x7: v = ct->tm_mday % 10; break;
		case 0x8: v = ct->tm_mday / 10; break;
		case 0x9: v = (ct->tm_mon + 1) % 10; break;
		case 0xA: v = (ct->tm_mon + 1) / 10; break;
		case 0xB: v = (ct->tm_year % 100) % 10; break;
		case 0xC: v = (ct->tm_year % 100) / 10; break;
		case 0xD: v = data->clock_control_d; break;
		/* E and F = write-only, reads as zero */
		case 0xE: v = 0; break;
		case 0xF: v = 0; break;
	}
	return v;
}

void put_clock_ricoh(struct rtc_ricoh_data *data, int addr, uae_u8 v)
{
	v &= 15;
	int bank = data->clock_control_d & 3;
	clock_sample(&data->clock, (data->clock_control_d & 8) != 0, &data->rtc_alarm[11]);
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
		uae_u8 ov = data->rtc_alarm[addr];
		data->rtc_alarm[addr] = v;
		if (addr == 10 && ((ov ^ v) & 1))
			data->clock.hour12_written = false;
		data->rtc_alarm[0] = data->rtc_alarm[1] = data->rtc_alarm[9] = data->rtc_alarm[12] = 0;
		data->rtc_alarm[3] &= ~0x8;
		data->rtc_alarm[5] &= ~0xc;
		data->rtc_alarm[6] &= ~0x8;
		data->rtc_alarm[8] &= ~0xc;
		data->rtc_alarm[10] &= ~0xe;
		data->rtc_alarm[11] &= ~0xc;
		if (addr == 11)
			data->clock.written = true;
		if (data->rtc_alarm[addr] != ov)
			data->delayed_write = -1;
		return;
	}
	if (addr < 13) {
		static const int msm_addr[13] = {0, 1, 2, 3, 4, 5, 12, 6, 7, 8, 9, 10, 11};
		clock_write_digit(&data->clock, msm_addr[addr], v, (data->rtc_alarm[10] & 1) != 0, 2);
		data->delayed_write = -1;
		return;
	}
#if CLOCK_DEBUG
	write_log (_T("CLOCK W %X: %X\n"), addr, value);
#endif
	switch (addr)
	{
		case 0xD:
			if (!(v & 8))
				data->clock.written = true;
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
			if (v & 2) {
				data->clock.phase_us = 0;
				data->clock.written = true;
			}
			if (data->clock_control_f != v) {
				data->clock_control_f = v;
				data->delayed_write = -1;
			}
			break;
	}
}
