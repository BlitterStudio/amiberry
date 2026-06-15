#!/bin/sh
set -eu

if grep -F -q '_sntprintf(p, sizeof p, _T("subtype=%s")' src/cfgfile.cpp; then
	echo "board ROM subtype options must not be formatted with pointer-sized buffers" >&2
	exit 1
fi

if rg -n '_sntprintf\((p|p2|s), sizeof (p|p2|s),' src/cfgfile.cpp; then
	echo "config saver options must not append with pointer-sized buffers" >&2
	exit 1
fi

if rg -n '_sntprintf\([^,]*\+[^,]*, sizeof [^,]+ \+ _tcslen' src/cfgfile.cpp; then
	echo "config saver options must append with remaining array capacity" >&2
	exit 1
fi
