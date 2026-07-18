#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry_rp9.cpp"

if ! awk '
	/void apply_ram\(/ { in_apply = 1 }
	in_apply && /ram.type == "z3"/ { in_z3 = 1 }
	in_apply && in_z3 && /address_space_24 = false;/ { addressing = 1 }
	in_apply && in_z3 && /z3fastmem\[0\]\.size/ { memory = 1; exit }
	in_apply && /^}/ { exit }
	END { exit addressing && memory ? 0 : 1 }
' "$source_file"; then
	echo "RP9 Z3 RAM must enable 32-bit addressing before assigning memory" >&2
	exit 1
fi
