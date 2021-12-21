/*
* UAE - The Un*x Amiga Emulator
*
* Keyboard buffer. Not really needed for X, but for SVGAlib and possibly
* Mac and DOS ports.
*
* Note: it's possible to have two threads in UAE, one reading keystrokes
* and the other one writing them. Despite this, no synchronization effort
* is needed. This code should be perfectly thread safe. At least if you
* assume that integer store instructions are atomic.
*
* Copyright 1995, 1997 Bernd Schmidt
*/

#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "keybuf.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "custom.h"
#include "savestate.h"

static int kpb_first, kpb_last;

#define KEYBUF_SIZE 256
static int keybuf[KEYBUF_SIZE];
static uae_char *keyinject;
static int keyinject_offset;
static uae_u8 keyinject_previous;
static bool keyinject_state;
static bool keyinject_do;
static bool ignore_next_release;
static int delayed_released_code;
static int delayed_released_time;


struct kbtab
{
	int ascii;
	uae_u8 code;
	uae_u8 qual;
};

static const struct kbtab kbtable[] =
{
	{ '~', 0x00, 0x60 },
	{ '!', 0x01, 0x60 },
	{ '@', 0x02, 0x60 },
	{ '#', 0x03, 0x60 },
	{ '$', 0x04, 0x60 },
	{ '%', 0x05, 0x60 },
	{ '^', 0x06, 0x60 },
	{ '&', 0x07, 0x60 },
	{ '*', 0x08, 0x60 },
	{ '(', 0x09, 0x60 },
	{ ')', 0x0a, 0x60 },
	{ '_', 0x0b, 0x60 },
	{ '+', 0x0c, 0x60 },
	{ '|', 0x0d, 0x60 },

	{ '`', 0x00 },
	{ '1', 0x01 },
	{ '2', 0x02 },
	{ '3', 0x03 },
	{ '4', 0x04 },
	{ '5', 0x05 },
	{ '6', 0x06 },
	{ '7', 0x07 },
	{ '8', 0x08 },
	{ '9', 0x09 },
	{ '0', 0x0a },
	{ '-', 0x0b },
	{ '=', 0x0c },
	{ '\\', 0x0d },

	{ '\t', 0x42 },
	{ '\n', 0x44 },

	{ '{', 0x1a, 0x60 },
	{ '}', 0x1b, 0x60 },
	{ ':', 0x29, 0x60 },
	{ '"', 0x2b, 0x60 },
	{ '[', 0x1a },
	{ ']', 0x1b },
	{ ';', 0x29 },
	{ '\'', 0x2b },


	{ '<', 0x38, 0x60 },
	{ '>', 0x39, 0x60 },
	{ '?', 0x3a, 0x60 },
	{ ',', 0x38 },
	{ '.', 0x39 },
	{ '/', 0x3a },

	{ ' ', 0x40 },

	{ 'q', 0x10 },
	{ 'w', 0x11 },
	{ 'e', 0x12 },
	{ 'r', 0x13 },
	{ 't', 0x14 },
	{ 'y', 0x15 },
	{ 'u', 0x16 },
	{ 'i', 0x17 },
	{ 'o', 0x18 },
	{ 'p', 0x19 },

	{ 'a', 0x20 },
	{ 's', 0x21 },
	{ 'd', 0x22 },
	{ 'f', 0x23 },
	{ 'g', 0x24 },
	{ 'h', 0x25 },
	{ 'j', 0x26 },
	{ 'k', 0x27 },
	{ 'l', 0x28 },

	{ 'z', 0x31 },
	{ 'x', 0x32 },
	{ 'c', 0x33 },
	{ 'v', 0x34 },
	{ 'b', 0x35 },
	{ 'n', 0x36 },
	{ 'm', 0x37 },

	{ 'Q', 0x10, 0x60 },
	{ 'W', 0x11, 0x60 },
	{ 'E', 0x12, 0x60 },
	{ 'R', 0x13, 0x60 },
	{ 'T', 0x14, 0x60 },
	{ 'Y', 0x15, 0x60 },
	{ 'U', 0x16, 0x60 },
	{ 'I', 0x17, 0x60 },
	{ 'O', 0x18, 0x60 },
	{ 'P', 0x19, 0x60 },

	{ 'A', 0x20, 0x60 },
	{ 'S', 0x21, 0x60 },
	{ 'D', 0x22, 0x60 },
	{ 'F', 0x23, 0x60 },
	{ 'G', 0x24, 0x60 },
	{ 'H', 0x25, 0x60 },
	{ 'J', 0x26, 0x60 },
	{ 'K', 0x27, 0x60 },
	{ 'L', 0x28, 0x60 },

	{ 'Z', 0x31, 0x60 },
	{ 'X', 0x32, 0x60 },
	{ 'C', 0x33, 0x60 },
	{ 'V', 0x34, 0x60 },
	{ 'B', 0x35, 0x60 },
	{ 'N', 0x36, 0x60 },
	{ 'M', 0x37, 0x60 },

	{ 0 }
};

static void keytoscancode(int key, bool release)
{
	int v = 0x40;
	int q = 0x00;
	int mask = release ? 0x80 : 0x00;
	for (int i = 0; kbtable[i].ascii; i++) {
		if (kbtable[i].ascii == key) {
			v = kbtable[i].code;
			q = kbtable[i].qual;
			break;
		}
	}
	v |= mask;
	v = (v << 1) | (v >> 7);
	q |= mask;
	if (release) {
		record_key(v);
		if (q & 0x7f) {
			q = (q << 1) | (q >> 7);
			record_key(q);
		}
	} else {
		if (q & 0x7f) {
			q = (q << 1) | (q >> 7);
			record_key(q);
		}
		record_key(v);
	}
}

// delay injection, some programs don't like too fast key events
static bool can_inject(void)
{
	static int ovpos, cnt;

	if (ovpos == vpos)
		return false;
	ovpos = vpos;
	cnt++;
	if (cnt < 4)
		return false;
	cnt = 0;
	keyinject_do = true;
	return true;
}

int keys_available (void)
{
	int val;
	val = kpb_first != kpb_last;
	if (can_inject() && ((keyinject && keyinject[keyinject_offset]) || keyinject_state))
		val = 1;
	return val;
}

int get_next_key (void)
{
	int key;

	if (keyinject_do) {
		keyinject_do = false;
		if (keyinject_state) {
			key = keyinject_previous;
			keyinject_state = false;
			keytoscancode(key, true);
		} else if (keyinject) {
			key = keyinject[keyinject_offset++];
			keyinject_previous = key;
			keyinject_state = true;
			if (keyinject[keyinject_offset] == 0) {
				xfree(keyinject);
				keyinject = NULL;
				keyinject_offset =0;
			}
			keytoscancode(key, false);
		}
	}

	assert (kpb_first != kpb_last);

	key = keybuf[kpb_last];
	if (++kpb_last == KEYBUF_SIZE)
		kpb_last = 0;

	// send release immediately in warp mode if not qualifier key
	delayed_released_time = 0;
	if (currprefs.turbo_emulation && !(key & 0x01) && (key >> 1) < 0x60) {
		if (!keys_available()) {
			delayed_released_code = key | 0x01;
			delayed_released_time = 5;
		}
	}

	//write_log (_T("%02x:%d\n"), key >> 1, key & 1);
	return key;
}

void keybuf_vsync(void)
{
	if (delayed_released_time > 0) {
		delayed_released_time--;
		if (delayed_released_time == 0) {
			record_key(delayed_released_code);
		}
	}
}

int record_key (int kc)
{
	if (pause_emulation)
		return 0;
	return record_key_direct (kc);
}

int record_key_direct (int kc)
{
	int kpb_next = kpb_first + 1;
	int kcd = (kc << 7) | (kc >> 1);

	if (ignore_next_release) {
		ignore_next_release = false;
		if (kcd & 0x80) {
			return 0;
		}
	}

	//write_log (_T("got kc %02X\n"), kcd & 0xff);
	if (kpb_next == KEYBUF_SIZE)
		kpb_next = 0;
	if (kpb_next == kpb_last) {
		write_log (_T("Keyboard buffer overrun. Congratulations.\n"));
		return 0;
	}
	keybuf[kpb_first] = kc;
	kpb_first = kpb_next;
	return 1;
}

void keybuf_init (void)
{
	kpb_first = kpb_last = 0;
	keyinject_offset = 0;
	xfree(keyinject);
	keyinject = NULL;
	delayed_released_code = -1;
	delayed_released_time = 0;
	inputdevice_updateconfig (&changed_prefs, &currprefs);
}

void keybuf_ignore_next_release(void)
{
	ignore_next_release = true;
}

void keybuf_inject(const uae_char *txt)
{
	uae_char *newbuf = xmalloc(uae_char, strlen(txt) + 1);
	uae_char *p = newbuf;

	for (int j = 0; j < strlen(txt); j++) {
		uae_char c = txt[j];
		bool found = false;
		for (int i = 0; kbtable[i].ascii; i++) {
			if (kbtable[i].ascii == c)
				found = true;
		}
		if (found)
			*p++= c;

	}
	*p = 0;
	if (p == newbuf) {
		xfree(newbuf);
		return;
	}
	xfree(keyinject);
	keyinject_offset = 0;
	keyinject = newbuf;
}
