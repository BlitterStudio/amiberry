#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/gui/main_window.cpp"
configurations_source_file="src/osdep/imgui/configurations.cpp"

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

if ! grep -F -q 'amiberry_options.quickstart_start ? "quickstart" : "play"' "$source_file"; then
	echo "Startup without a loaded config must honor the saved Quickstart startup preference" >&2
	exit 1
fi

if ! grep -F -q 'amiberry_options.quickstart_start);' "$source_file"; then
	echo "Quickstart startup must enable expert mode so simple-mode filtering does not switch back to Play" >&2
	exit 1
fi

if ! grep -F -q 'action_type == PlayContentType::Unknown' "src/osdep/imgui/play.cpp"; then
	echo "Unsupported Play selections must not block starting the current configuration" >&2
	exit 1
fi

if ! grep -F -q 'play_clear_content_selection();' "$configurations_source_file"; then
	echo "Loading a config outside Play must clear stale Play selections before Start" >&2
	exit 1
fi

direct_start_resets=$(grep -F -c 'uae_reset(0, 1);' "$source_file")
if [ "$direct_start_resets" -ne 1 ]; then
	echo "GUI start paths must route through one shared Play preparation helper" >&2
	exit 1
fi
