#!/bin/sh
set -eu

missing=0
voodoo_src=src/pcem/vid_voodoo_banshee.cpp

for reg in cmdBump0 cmdRdPtrH0 cmdHoleCnt0; do
	if ! grep -F -q "case ${reg}:" "$voodoo_src"; then
		echo "Banshee command register ${reg} must be handled explicitly" >&2
		missing=1
	fi
done

if grep -F -q 'write_log("hdf_close_target\n");' src/osdep/amiberry_hardfile.cpp; then
	echo "hdf_close_target must not emit an unconditional bare trace line" >&2
	missing=1
fi

if ! grep -F -q 'no autoconfig (unassigned)' src/expansion.cpp; then
	echo "non-autoconfig board logging must handle unassigned ranges explicitly" >&2
	missing=1
fi

if ! grep -F -q 'uae_ppc_mark_code_cache_dirty();' src/newcpu.cpp; then
	echo "68040 cache flushes must mark QEMU PPC JIT state dirty" >&2
	missing=1
fi

if grep -n -A 6 -E 'void uae_ppc_execute_(check|quick)' src/ppc/ppc.cpp | grep -F -q 'impl.flush_jit();'; then
	echo "QEMU PPC JIT flush must not be called directly from execute hot paths" >&2
	missing=1
fi

if ! grep -F -q 'qemu_ppc_jit_flush_pending' src/ppc/ppc.cpp; then
	echo "QEMU PPC JIT flush requests must be coalesced with a pending flag" >&2
	missing=1
fi

exit "$missing"
