#!/bin/sh
set -eu

if rg -n '_sntprintf\([^,\n]*\+[^,\n]*,\s*sizeof [^,\n]+ \+' src external libretro -g'*.cpp' -g'*.c' -g'*.h'; then
	echo "_sntprintf appends must pass remaining capacity, not full size plus offset" >&2
	exit 1
fi

if rg -n '_sntprintf\([^,\n]*\+[^,\n]*,\s*sizeof\s+[A-Za-z_][A-Za-z0-9_]*\s*,' src external libretro -g'*.cpp' -g'*.c' -g'*.h'; then
	echo "_sntprintf appends must pass remaining capacity, not full array size" >&2
	exit 1
fi

if rg -n '_sntprintf\([^,\n]*,\s*sizeof\s+(p|p2|p3|s|out|buffer)\s*,' src external libretro -g'*.cpp' -g'*.c' -g'*.h'; then
	echo "_sntprintf must not use sizeof on pointer destinations" >&2
	exit 1
fi

if rg -n '_tcscat\(tmp2, _T\("(,|readonly)"\)\)' src/cfgfile.cpp; then
	echo "RAM-board option suffixes must use bounded appends" >&2
	exit 1
fi
