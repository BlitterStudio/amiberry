#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/imgui/play.cpp"

action_apply_count=$(grep -F -c 'apply_selected_content(action_type);' "$source_file")
if [ "$action_apply_count" -ne 1 ]; then
	echo "Play content actions must be applied only by the shared preparation helper" >&2
	exit 1
fi

if grep -F -q 'apply_selected_content(selected_content_choice);' "$source_file"; then
	echo "Ambiguous hardfile choices must route through play_prepare_selected_content_for_start()" >&2
	exit 1
fi

if ! grep -F -q 'play_prepare_selected_content_for_start();' "$source_file"; then
	echo "Use this content must avoid reapplying already-prepared Play content" >&2
	exit 1
fi

if ! grep -F -q 'if (play_prepare_selected_content_for_start())' "$source_file"; then
	echo "Hardfile storage editing must attach selected content before opening the storage panel" >&2
	exit 1
fi
