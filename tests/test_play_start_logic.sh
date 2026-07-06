#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/gui/main_window.cpp"

if grep -F -q 'gui_active_panel_is("play") && play_has_content_selection()' "$source_file"; then
	echo "Start must prepare pending Play selections even after leaving the Play panel" >&2
	exit 1
fi

if ! grep -F -q 'const bool has_play_selection = play_has_content_selection();' "$source_file"; then
	echo "Start must derive pending Play selection from play_has_content_selection()" >&2
	exit 1
fi

if ! grep -F -q 'play_prepare_selected_content_for_start()' "$source_file"; then
	echo "Start must prepare selected Play content before launching" >&2
	exit 1
fi
