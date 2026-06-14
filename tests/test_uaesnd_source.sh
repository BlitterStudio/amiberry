#!/bin/sh
set -eu

test ! -e src/uaesnd_ahi.s
test ! -e src/osdep/ahi_v2.cpp
test ! -e src/osdep/ahi_v2.h
if rg -n 'ahi_v2|AHI_v2|AHI_V2|uaesnd_ahi' CMakeLists.txt cmake libretro src; then
	echo "obsolete AHI v2 wrapper references must be removed" >&2
	exit 1
fi

grep -F -q 'static int uaesnd_sample_bytes' src/sndboard.cpp
grep -F -q 'case 3: return 4;' src/sndboard.cpp
grep -F -q 's->framesize = uaesnd_sample_bytes(s->bitmode) * s->ch;' src/sndboard.cpp
grep -F -q 'static void uaesnd_apply_mono_pan' src/sndboard.cpp
grep -F -q 's->sample[0] = smp * (32768 - s->panx) / 32768;' src/sndboard.cpp
grep -F -q 's->sample[1] = smp * (32768 + s->panx) / 32768;' src/sndboard.cpp
grep -F -q 'highestch = i + 1;' src/sndboard.cpp
if grep -F -q 'Panning values must be zeros' src/sndboard.cpp; then
	echo "UAESND should accept and apply non-zero panning values" >&2
	exit 1
fi
