#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/imgui/play.cpp"

if grep -F -q 'apply_selected_content(action_type);' "$source_file"; then
	echo "Use this content must route through play_prepare_selected_content_for_start()" >&2
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
