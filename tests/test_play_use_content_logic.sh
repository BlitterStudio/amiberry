#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/imgui/play.cpp"
quickstart_source_file="src/osdep/imgui/quickstart.cpp"

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

if ! grep -F -q 'mark_selected_content_pending();' "$source_file"; then
	echo "Changing the Quickstart model after applying Play content must force content reapplication" >&2
	exit 1
fi

if ! grep -F -q 'selected_content_still_attached()' "$source_file"; then
	echo "Play start must verify previously applied content is still attached before skipping reapplication" >&2
	exit 1
fi

if ! grep -F -q 'play_mark_selected_content_pending();' "$quickstart_source_file"; then
	echo "Quickstart default changes must force any applied Play content to be reapplied" >&2
	exit 1
fi

if grep -F -q '_tcscpy(current_hfdlg.ci.rootdir, selected_content.original_path.c_str());' "$source_file"; then
	echo "Selected hardfile paths must be copied with a bounded helper" >&2
	exit 1
fi

if ! grep -F -q 'copy_path_to_buffer(current_hfdlg.ci.rootdir' "$source_file"; then
	echo "Selected hardfile paths must be bounded to the rootdir buffer" >&2
	exit 1
fi

if ! grep -F -q 'quickstart_override_changed()' "$source_file"; then
	echo "Play must detect manual Quickstart overrides before applying content" >&2
	exit 1
fi

if ! grep -F -q 'apply_quickstart_model_unless_overridden' "$source_file"; then
	echo "Play must preserve manual Quickstart model changes" >&2
	exit 1
fi

if ! grep -F -q 'refresh_quickstart_config_list();' "$source_file"; then
	echo "Play must refresh Quickstart configuration choices when changing the Quickstart model" >&2
	exit 1
fi

if ! grep -F -q 'const bool manual_quickstart_override = quickstart_override_changed();' "$source_file"; then
	echo "CD Play content must detect manual Quickstart overrides before CD autoload" >&2
	exit 1
fi

if ! grep -F -q 'mount_selected_cd_image(path);' "$source_file"; then
	echo "CD Play content with manual Quickstart overrides must mount the CD without rebuilding the model" >&2
	exit 1
fi

direct_quickstart_count=$(grep -F -c 'apply_quickstart_model(PlaySuggestedModel::' "$source_file" || true)
if [ "$direct_quickstart_count" -ne 0 ]; then
	echo "Play content model setup must preserve manual Quickstart overrides" >&2
	exit 1
fi

if ! grep -F -q 'OpenDirDialogKey("PLAY_CONTENT_DIR"' "$source_file"; then
	echo "Play content picker must allow selecting extracted WHDLoad directories" >&2
	exit 1
fi

if grep -F -q 'ConsumeFileDialogResultKey("PLAY_CONTENT_DIR"' "$source_file"; then
	echo "Play folder picker must use the directory result consumer" >&2
	exit 1
fi

if ! grep -F -q 'ConsumeDirDialogResultKey("PLAY_CONTENT_DIR"' "$source_file"; then
	echo "Play content picker must consume selected directory paths with directory semantics" >&2
	exit 1
fi
