#!/usr/bin/env python3
"""Check that the x86 JIT generator preserves checked-in output layouts."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
GENCOMP = ROOT / "src" / "jit" / "x86" / "gencomp.cpp"


AMIBERRY_FRAGMENTS = (
	'"comptbl_x86.h"',
	'"compstbl_x86.cpp"',
	'"compemu_x86.cpp"',
	'write_amiberry_dispatchers',
	'AMIBERRY_COMPTBL_H',
	'"arm/comptbl_arm.h"',
	'"x86/comptbl_x86.h"',
	'"arm/compstbl_arm.cpp"',
	'"x86/compstbl_x86.cpp"',
	'"arm/compemu_arm.cpp"',
	'"x86/compemu_x86.cpp"',
	'defined(_M_ARM64EC)',
)

WINUAE_FRAGMENTS = (
	'write_winuae_header_prefix',
	'write_winuae_compemu_prefix',
	'write_winuae_source_prefix',
	'write_winuae_compemu_suffix',
	'write_winuae_source_suffix',
	'#if defined(CPU_AARCH64)',
	'"arm/comptbl_arm.h"',
	'"arm/compstbl_arm.cpp"',
	'"arm/compemu_arm.cpp"',
	'#endif /* CPU_AARCH64 */',
	'uintptr v2;\\n"',
	'"\\tv2=get_const(offs);\\n"',
	'(uintptr)(uae_s32)(uae_u32)get_const(src)',
)

FORBIDDEN_FRAGMENTS = (
	'(uintptr)(uae_s32)%s',
)


def main() -> int:
	source = GENCOMP.read_text(encoding="utf-8")
	missing = [
		fragment
		for fragment in AMIBERRY_FRAGMENTS + WINUAE_FRAGMENTS
		if fragment not in source
	]
	if missing:
		for fragment in missing:
			print(f"missing gencomp output fragment: {fragment}", file=sys.stderr)
		return 1
	for fragment in FORBIDDEN_FRAGMENTS:
		if fragment in source:
			print(f"forbidden gencomp output fragment: {fragment}", file=sys.stderr)
			return 1
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
