#!/usr/bin/env bash
set -euo pipefail

if command -v python3 >/dev/null 2>&1; then
	PYTHON=python3
elif command -v python >/dev/null 2>&1; then
	PYTHON=python
elif command -v py >/dev/null 2>&1; then
	PYTHON="py -3"
else
	echo "python3, python, or py is required" >&2
	exit 1
fi

$PYTHON - <<'PY'
from pathlib import Path
import sys

cfgfile = Path("src/cfgfile.cpp").read_text()
gfx_prefs_check = Path("src/osdep/gfx_prefs_check.cpp").read_text()
custom = Path("src/custom.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


def region_between(text: str, start_marker: str, end_marker: str) -> str:
	try:
		start = text.index(start_marker)
		end = text.index(end_marker, start)
	except ValueError as exc:
		fail(f"Could not find source marker: {exc}")
	return text[start:end]


def require_outside_gfxfilter(region: str, snippet: str, label: str) -> None:
	guard = region.find("#ifdef GFXFILTER")
	if guard < 0:
		fail(f"Expected {label} region to retain a GFXFILTER-only subsection")
	pos = region.find(snippet)
	if pos < 0:
		fail(f"Missing {label} for {snippet}")
	if pos > guard:
		fail(f"{snippet} must be outside the GFXFILTER-only block")
	if region.find(snippet, guard) >= 0:
		fail(f"{snippet} must not also remain inside the GFXFILTER-only block")


save_region = region_between(
	cfgfile,
	'cfgfile_dwrite_bool(f, _T("gfx_ntscpixels"), p->gfx_ntscpixels);',
	'cfgfile_write_bool (f, _T("immediate_blits"), p->immediate_blits);',
)

unconditional_save_options = [
	'cfgfile_dwrite_strarr(f, _T("gfx_filter_autoscale")',
	'cfgfile_dwrite (f, _T("gfx_luminance")',
	'cfgfile_dwrite (f, _T("gfx_contrast")',
	'cfgfile_dwrite (f, _T("gfx_gamma")',
	'cfgfile_dwrite (f, _T("gfx_gamma_r")',
	'cfgfile_dwrite (f, _T("gfx_gamma_g")',
	'cfgfile_dwrite (f, _T("gfx_gamma_b")',
	'cfgfile_dwrite (f, _T("gfx_center_horizontal_position")',
	'cfgfile_dwrite (f, _T("gfx_center_vertical_position")',
	'cfgfile_dwrite (f, _T("gfx_center_horizontal_size")',
	'cfgfile_dwrite (f, _T("gfx_center_vertical_size")',
	'cfgfile_dwrite (f, _T("rtg_vert_zoom_multf")',
	'cfgfile_dwrite (f, _T("rtg_horiz_zoom_multf")',
	'cfgfile_dwrite_ext(f, _T("gfx_filter_vert_zoom_multf")',
	'cfgfile_dwrite_ext(f, _T("gfx_filter_horiz_zoom_multf")',
	'cfgfile_dwrite_ext(f, _T("gfx_filter_left_border")',
	'cfgfile_dwrite_ext(f, _T("gfx_filter_right_border")',
	'cfgfile_dwrite_ext(f, _T("gfx_filter_top_border")',
	'cfgfile_dwrite_ext(f, _T("gfx_filter_bottom_border")',
	'cfgfile_dwrite_ext(f, _T("gfx_filter_blur")',
]

for snippet in unconditional_save_options:
	require_outside_gfxfilter(save_region, snippet, "cfgfile save")

for option in ("rtg_vert_zoom_multf", "rtg_horiz_zoom_multf"):
	rounded = f'cfgfile_dwrite (f, _T("{option}"), _T("%.f")'
	fractional = f'cfgfile_dwrite (f, _T("{option}"), _T("%f")'
	if rounded in save_region:
		fail(f"{option} must preserve fractional values when saved")
	if fractional not in save_region:
		fail(f"{option} must be saved with a fractional float format")

parse_region = region_between(
	cfgfile,
	'if (cfgfile_yesno(option, value, _T("magic_mouse"), &vb))',
	'if (cfgfile_intval (option, value, _T("floppy_volume"), &v, 1))',
)

unconditional_parse_options = [
	'cfgfile_strval (option, value, _T("gfx_filter_autoscale")',
	'cfgfile_floatval(option, value, _T("gfx_filter_vert_zoom_multf")',
	'cfgfile_floatval(option, value, _T("gfx_filter_horiz_zoom_multf")',
	'cfgfile_intval(option, value, _T("gfx_filter_left_border")',
	'cfgfile_intval(option, value, _T("gfx_filter_right_border")',
	'cfgfile_intval(option, value, _T("gfx_filter_top_border")',
	'cfgfile_intval(option, value, _T("gfx_filter_bottom_border")',
	'cfgfile_intval(option, value, _T("gfx_filter_blur")',
]

for snippet in unconditional_parse_options:
	require_outside_gfxfilter(parse_region, snippet, "cfgfile parse")

for channel in range(3):
	compare = f"currprefs.gfx_gamma_ch[{channel}] != changed_prefs.gfx_gamma_ch[{channel}]"
	copy = f"currprefs.gfx_gamma_ch[{channel}] = changed_prefs.gfx_gamma_ch[{channel}];"
	if compare not in gfx_prefs_check:
		fail(f"gfx_gamma_ch[{channel}] changes must be detected in gfx_prefs_check.cpp")
	if copy not in gfx_prefs_check:
		fail(f"gfx_gamma_ch[{channel}] changes must be copied in gfx_prefs_check.cpp")

custom_region = region_between(
	custom,
	"void check_prefs_changed_custom(void)",
	"static uae_u16 fetch16",
)

border_assignments = [
	"fd->gfx_filter_left_border = fdcp->gfx_filter_left_border;",
	"fd->gfx_filter_right_border = fdcp->gfx_filter_right_border;",
	"fd->gfx_filter_top_border = fdcp->gfx_filter_top_border;",
	"fd->gfx_filter_bottom_border = fdcp->gfx_filter_bottom_border;",
]

for snippet in border_assignments:
	pos = custom_region.find(snippet)
	if pos < 0:
		fail(f"Missing custom prefs copy for {snippet}")
	guard_start = custom_region.rfind("#ifdef GFXFILTER", 0, pos)
	if guard_start >= 0:
		guard_end = custom_region.find("#endif", guard_start)
		if guard_end < 0 or pos < guard_end:
			fail(f"{snippet} must not be guarded by GFXFILTER")
PY
