#!/bin/sh
set -eu

missing=0
voodoo_src=src/86box/video/vid_voodoo_banshee.cpp
# 86Box compiles banshee_log() out unless ENABLE_BANSHEE_LOG is defined, so
# the noisy unhandled-CMDFIFO-register logging PCem needed case labels for
# cannot come back without that define.
if ! grep -F -q '#ifdef ENABLE_BANSHEE_LOG' "$voodoo_src"; then
	echo "banshee_log must stay behind ENABLE_BANSHEE_LOG" >&2
	missing=1
fi

if grep -F -q 'write_log("hdf_close_target\n");' src/osdep/amiberry_hardfile.cpp; then
	echo "hdf_close_target must not emit an unconditional bare trace line" >&2
	missing=1
fi

if ! grep -F -q 'no autoconfig (unassigned)' src/expansion.cpp; then
	echo "non-autoconfig board logging must handle unassigned ranges explicitly" >&2
	missing=1
fi

cache_flush_body=$(sed -n '/void flush_cpu_caches_040/,/void cpu_invalidate_cache/p' src/newcpu.cpp)
if ! printf '%s\n' "$cache_flush_body" | grep -F -q 'if (cache & 2)' ||
	! printf '%s\n' "$cache_flush_body" | grep -F -q 'uae_ppc_mark_code_cache_dirty();'; then
	echo "68040 instruction-cache flushes must mark QEMU PPC JIT state dirty" >&2
	missing=1
fi

if ! grep -F -q 'uae_ppc_mark_code_cache_dirty();' src/cpummu.cpp; then
	echo "68040 MMU ATC flushes must mark QEMU PPC JIT/TLB state dirty" >&2
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

if ! grep -F -q 'cfgfile_ppc_cpu_idle' src/cfgfile.cpp ||
	! grep -F -q '!_tcsicmp(value, _T("max"))' src/cfgfile.cpp ||
	! grep -F -q 'idle >= 0 && idle <= 10' src/cfgfile.cpp; then
	echo "ppc_cpu_idle config parsing must accept disabled/max and numeric 0..10" >&2
	missing=1
fi

ppc_quick_body=$(sed -n '/void uae_ppc_execute_quick/,/void uae_ppc_emulate/p' src/ppc/ppc.cpp)
if ! grep -F -q 'QEMU_QUICK_HANDOFF_IDLE_MAX' src/ppc/ppc.cpp ||
	! grep -F -q 'const int sleep_interval = QEMU_QUICK_HANDOFF_IDLE_MAX + 1 - idle;' src/ppc/ppc.cpp ||
	! grep -F -q 'idle <= 0' src/ppc/ppc.cpp ||
	! grep -F -q 'idle >= QEMU_QUICK_HANDOFF_IDLE_MAX' src/ppc/ppc.cpp ||
	! printf '%s\n' "$ppc_quick_body" | grep -F -q 'sleep_millis_main(qemu_ppc_quick_handoff_sleep_ms());'; then
	echo "QEMU PPC quick handoffs must scale throttling with PPC idle" >&2
	missing=1
fi

exit "$missing"
