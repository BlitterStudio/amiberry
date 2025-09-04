
/*
* UAE - The Un*x Amiga Emulator
*
* Emulates simple protection dongles
*
* Copyright 2009 Toni Wilen
*/


#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "dongle.h"
#include "events.h"
#include "uae.h"
#include "debug.h"
#include "inputdevice.h"

#define ROBOCOP3 1
#define LEADERBOARD 2
#define BAT2 3
#define ITALY90 4
#define DAMESGRANDMAITRE 5
#define RUGBYCOACH 6
#define CRICKETCAPTAIN 7
#define LEVIATHAN 8
#define MUSICMASTER 9
#define LOGISTIX 10
#define SCALA_RED 11
#define SCALA_GREEN 12
#define STRIKERMANAGER 13
#define MPSOCCERMANAGER 14
#define FOOTBALLDIRECTOR2 15

static int dflag;
static frame_time_t cycles;

/*
RoboCop 3
- set firebutton as output
- read JOY1DAT
- pulse firebutton (high->low)
- read JOY1DAT
- JOY1DAT bit 8 must toggle

Leader Board
- JOY1DAT, both up and down active (0x0101)

B.A.T. II
- set all serial pins as output except CTS
- game pulses DTR (high->low)
- CTS must be one
- delay
- CTS must be zero

Italy '90 Soccer
- 220k resistor between pins 5 (+5v) and 7 (POTX)
- POT1DAT POTX must be between 0x32 and 0x60

Dames Grand Maitre
- read POT1
- POT1X != POT1Y
- POT1Y * 256 / POT1X must be between 450 and 500

Rugby Coach
- JOY1DAT, left, up and down active (0x0301)

Cricket Captain
- JOY0DAT bits 0 and 1:
- 10 01 11 allowed
- must continuously change state

Leviathan
- same as Leaderboard but in mouse port

Logistix/SuperBase
- second button must be high
- POT1X = 150k
- POT1Y = 100k
- POT1X * 10 / POT1Y must be between 12 and 33

Music Master
- sets joystick port 2 fire button output + low
- first JOY1DAT AND 0x0303 must be zero.
- following JOY1DAT AND 0x0303 reads must be nonzero.

Scala MM (Green)

- 470nF Capacitor between fire button and second button pin
- Drives firebutton high, then low
- Polls POTGOR second button pin, it must go low between about 30000-150000 DMA cycles.

Scala MM (Red)

- 10uF Capacitor between fire button and second button pin
- Drives firebutton high, then low
- Polls POTGOR second button pin, it must go low between about 350000-540000 DMA cycles.

Striker Manager

- Writes 0x0F00 to POTGO few times
- Reads JOY1DAT, expects AND 0x303 == 0x200 or 0x203
- Reads JOY1DAT in a loop until AND 0x303 == 0x200 or 0x203 (opposite from previous read)
- Resets the system if wrong value after 200 000 read attemps.

Multi-Player Soccer Manager

- Writes 0x0F00 to POTGO few times
- Reads JOY1DAT, expects AND 0x303 == 0x301 or 0x302
- Reads JOY1DAT in a loop until AND 0x303 == 0x301 or 0x302 (opposite from previous read)
- Resets the system if wrong value after 200 000 read attemps.

*/

static uae_u8 oldcia[2][16];

void dongle_reset (void)
{
	dflag = 0;
	memset (oldcia, 0, sizeof oldcia);
}

uae_u8 dongle_cia_read (int cia, int reg, uae_u8 extra, uae_u8 val)
{
	if (!currprefs.dongle)
		return val;
	switch (currprefs.dongle)
	{
	case BAT2:
		if (cia == 1 && reg == 0) {
			if (!dflag || get_cycles () > cycles + CYCLE_UNIT * 200) {
				val &= ~0x10;
				dflag = 0;
			} else {
				val |= 0x10;
			}
		}
		break;
	}
	return val;
}

void dongle_cia_write (int cia, int reg, uae_u8 extra, uae_u8 val)
{
	if (!currprefs.dongle)
		return;
	switch (currprefs.dongle)
	{
	case SCALA_GREEN:
	case SCALA_RED:
		if (cia == 0 && reg == 0) {
			if ((val & 0x80) != dflag) {
				dflag = val & 0x80;
				cycles = get_cycles();
			}
		}
		break;
	case ROBOCOP3:
		if (cia == 0 && reg == 0 && (val & 0x80) && !(oldcia[cia][reg] & 0x80)) {
			pulse_joydat(1, 1, 1);
		}
		break;
	case BAT2:
		if (cia == 1 && reg == 0 && !(val & 0x80)) {
			dflag = 1;
			cycles = get_cycles ();
		}
		break;
	case MUSICMASTER:
		if (cia == 0 && reg == 0) {
			if (!(val & 0x80) && (extra & 0x80)) {
				dflag = 1;
			} else {
				dflag = 0;
			}
		}
		break;
	}
	oldcia[cia][reg] = val;
}

void dongle_joytest (uae_u16 val)
{
}

uae_u16 dongle_joydat (int port, uae_u16 val)
{
	if (!currprefs.dongle)
		return val;
	switch (currprefs.dongle)
	{
	case ROBOCOP3:
		if (port == 1) {
			val &= 0x0101;
		}
		break;
	case LEADERBOARD:
		if (port == 1) {
			val &= ~0x0303;
			val |= 0x0101;
		}
		break;
	case LEVIATHAN:
		if (port == 0) {
			val &= ~0x0303;
			val |= 0x0101;
		}
		break;
	case RUGBYCOACH:
		if (port == 1) {
			val &= ~0x0303;
			val|= 0x0301;
		}
		break;
	case CRICKETCAPTAIN:
		if (port == 0) {
			val &= ~0x0003;
			if (dflag == 0)
				val |= 0x0001;
			else
				val |= 0x0002;
		}
		dflag ^= 1;
		break;
	case MUSICMASTER:
		if (port == 1 && !dflag) {
			val = 0;
		} else if (port == 1 && dflag == 1) {
			val = 0;
			dflag++;
		} else if (port == 1 && dflag == 2) {
			val = 0x0303;
		}
		break;
	case STRIKERMANAGER:
		if (port == 1) {
			if (dflag >= 4) {
				val &= ~0x0303;
				val |= 0x0203;
				dflag--;
			} else if (dflag > 0) {
				val &= ~0x0303;
				val |= 0x0200;
			}
		}
		break;
	case MPSOCCERMANAGER:
		if (port == 1) {
			if (dflag >= 4) {
				val &= ~0x0303;
				val |= 0x0302;
				dflag--;
			} else if (dflag > 0) {
				val &= ~0x0303;
				val |= 0x0301;
			}
		}
		break;
	case FOOTBALLDIRECTOR2:
		if (port == 1) {
			if (dflag >= 4) {
				val &= ~0x0303;
				val |= 0x0300;
				dflag--;
			} else if (dflag > 0) {
				val &= ~0x0303;
				val |= 0x0303;
			}
		}
		break;
	}
	return val;
}

void dongle_potgo (uae_u16 val)
{
	if (!currprefs.dongle)
		return;
	switch (currprefs.dongle)
	{
	case ITALY90:
	case LOGISTIX:
	case DAMESGRANDMAITRE:
		dflag = (uaerand () & 7) - 3;
		break;
	case STRIKERMANAGER:
	case MPSOCCERMANAGER:
	case FOOTBALLDIRECTOR2:
		if ((val & 0x0500) == 0x0500) {
			dflag++;
		} else {
			if (dflag > 0) {
				dflag--;
			}
		}
		break;
	}

}

uae_u16 dongle_potgor (uae_u16 val)
{
	if (!currprefs.dongle)
		return val;
	switch (currprefs.dongle)
	{
	case LOGISTIX:
		val |= 1 << 14;
		break;
	case SCALA_RED:
	case SCALA_GREEN:
		{
			uae_u8 mode = 0x80;
			if ((dflag & 1) || get_cycles() >= cycles + CYCLE_UNIT * (currprefs.dongle == SCALA_RED ? 450000 : 80000)) {
				mode = 0x00;
				dflag |= 1;
			}
			if (((dflag & 0x80) ^ mode) == 0x80)
				val |= 1 << 14;
			else
				val &= ~(1 << 14);
			break;
		}
	}
	return val;
}

int dongle_analogjoy (int joy, int axis)
{
	int v = -1;
	if (!currprefs.dongle)
		return -1;
	switch (currprefs.dongle)
	{
	case ITALY90:
		if (joy == 1 && axis == 0)
			v = 73;
		break;
	case LOGISTIX:
		if (joy == 1) {
			if (axis == 0)
				v = 21;
			if (axis == 1)
				v = 10;
		}
		break;
	case DAMESGRANDMAITRE:
		if (joy == 1) {
			if (axis == 1)
				v = 80;
			if (axis == 0)
				v = 43;
		}
		break;

	}
	if (v >= 0) {
		v += dflag;
		if (v < 0)
			v = 0;
	}
	return v;
}
