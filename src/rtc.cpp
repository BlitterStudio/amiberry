/*
* UAE - The Un*x Amiga Emulator
*
* RTC chips
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"

#include "rtc.h"

uae_u8 get_clock_msm(struct rtc_msm_data *data, int addr, struct tm *ct)
{
	uae_u8 v;
	int year;

	if (!ct) {
		time_t t = time (0);
		ct = localtime (&t);
	}
	year = ct->tm_year;
	if (data->yearmode && year >= 100)
		year -= 100;

	switch (addr)
	{
		case 0x0: v = ct->tm_sec % 10; break;
		case 0x1: v = ct->tm_sec / 10; break;
		case 0x2: v = ct->tm_min % 10; break;
		case 0x3: v = ct->tm_min / 10; break;
		case 0x4: v = ct->tm_hour % 10; break;
		case 0x5:
			if (data->clock_control_f & 4) {
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
		case 0xA: v = year % 10; break;
		case 0xB: v = (year / 10) & 0x0f;  break;
		case 0xC: v = ct->tm_wday; break;
		case 0xD: v = data->clock_control_d; break;
		case 0xE: v = data->clock_control_e; break;
		case 0xF: v = data->clock_control_f; break;
	}
	return v;
}

bool put_clock_msm(struct rtc_msm_data *data, int addr, uae_u8 v)
{
	switch (addr)
	{
		case 0xD: data->clock_control_d = v & (1|8); break;
		case 0xE: data->clock_control_e = v; break;
		case 0xF: data->clock_control_f = v; break;
	}
	return false;
}

uae_u8 get_clock_ricoh(struct rtc_ricoh_data *data, int addr, struct tm *ct)
{
	uae_u8 v = 0;
	int bank = data->clock_control_d & 3;

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
	if (!ct) {
		time_t t = time (0);
		ct = localtime (&t);
	}
	switch (addr)
	{
		case 0x0: v = ct->tm_sec % 10; break;
		case 0x1: v = ct->tm_sec / 10; break;
		case 0x2: v = ct->tm_min % 10; break;
		case 0x3: v = ct->tm_min / 10; break;
		case 0x4: v = ct->tm_hour % 10; break;
		case 0x5:
			if (data->rtc_alarm[10] & 1)
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
		case 0xD: v = data->clock_control_d; break;
		/* E and F = write-only, reads as zero */
		case 0xE: v = 0; break;
		case 0xF: v = 0; break;
	}
	return v;
}

void put_clock_ricoh(struct rtc_ricoh_data *data, int addr, uae_u8 v)
{
	int bank = data->clock_control_d & 3;
	/* memory access */
	if (bank >= 2 && addr < 0x0d) {
		uae_u8 ov = data->rtc_memory[addr];
		data->rtc_memory[addr] &= ((bank == 2) ? 0xf0 : 0x0f);
		data->rtc_memory[addr] |= v << ((bank == 2) ? 0 : 4);
		return;
	}
	/* alarm */
	if (bank == 1 && addr < 0x0d) {
#if CLOCK_DEBUG
		write_log (_T("CLOCK ALARM W %X: %X\n"), addr, value);
#endif
		uae_u8 ov = data->rtc_alarm[addr];
		data->rtc_alarm[addr] = v;
		data->rtc_alarm[0] = data->rtc_alarm[1] = data->rtc_alarm[9] = data->rtc_alarm[12] = 0;
		data->rtc_alarm[3] &= ~0x8;
		data->rtc_alarm[5] &= ~0xc;
		data->rtc_alarm[6] &= ~0x8;
		data->rtc_alarm[8] &= ~0xc;
		data->rtc_alarm[10] &= ~0xe;
		data->rtc_alarm[11] &= ~0xc;
		return;
	}
#if CLOCK_DEBUG
	write_log (_T("CLOCK W %X: %X\n"), addr, value);
#endif
	switch (addr)
	{
		case 0xD: data->clock_control_d = v; break;
		case 0xE: data->clock_control_e = v; break;
		case 0xF: data->clock_control_f = v; break;
	}
}
