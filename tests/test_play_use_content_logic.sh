#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/imgui/play.cpp"
quickstart_source_file="src/osdep/imgui/quickstart.cpp"
whdbooter_source_file="src/osdep/amiberry_whdbooter.cpp"
options_source_file="src/include/options.h"
whdload_source_file="src/osdep/imgui/whdload.cpp"

action_apply_count=$(grep -F -c 'apply_selected_content(action_type);' "$source_file")
if [ "$action_apply_count" -ne 1 ]; then
	echo "Play content actions must be applied only by the shared preparation helper" >&2
	exit 1
fi

if grep -F -q 'apply_selected_content(selected_content_choice);' "$source_file"; then
	echo "Ambiguous hardfile choices must route through play_prepare_selected_content_for_start()" >&2
	exit 1
fi

if grep -F -q 'Use this content' "$source_file"; then
	echo "Play setup apply action must not use the old content-action label" >&2
	exit 1
fi

if ! grep -F -q 'AmigaButton("Apply setup"' "$source_file"; then
	echo "Play must expose an explicit setup-apply action before Start" >&2
	exit 1
fi

if ! awk '
	/if \(!selected_content_applied\)/ { in_pending = 1; next }
	in_pending && /AmigaButton\("Apply setup"/ { found = 1; exit }
	in_pending && /ICON_FA_ROCKET " Change model\.\.\."/ { exit }
	END { exit found ? 0 : 1 }
' "$source_file"; then
	echo "Play must hide Apply setup after selected content is already applied" >&2
	exit 1
fi

if ! grep -F -q 'play_prepare_selected_content_for_start();' "$source_file"; then
	echo "Apply setup must avoid reapplying already-prepared Play content" >&2
	exit 1
fi

if ! grep -F -q 'return has_selected_content && !selected_content_applied;' "$source_file"; then
	echo "Global Start must only treat unapplied Play selections as pending" >&2
	exit 1
fi

if ! grep -F -q 'void play_clear_content_selection()' "$source_file"; then
	echo "Play must expose a way to clear stale selections after non-Play config loads" >&2
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

if grep -F -q 'play_mark_selected_content_pending();' "$quickstart_source_file"; then
	echo "Quickstart defaults must not globally mark Play content pending" >&2
	exit 1
fi

if ! grep -F -q 'apply_quickstart_defaults_from_quickstart();' "$quickstart_source_file"; then
	echo "Quickstart UI defaults must use the ownership-aware helper" >&2
	exit 1
fi

if ! grep -F -q 'clear_play_content_if_quickstart_source();' "$quickstart_source_file"; then
	echo "Quickstart configuration-only controls must preserve Play selections while editing the selected model" >&2
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

if ! grep -F -q 'current_hardfile_attachment_path().empty()' "$source_file"; then
	echo "Reapplying selected hardfile content must not add duplicate mount entries" >&2
	exit 1
fi

if ! grep -F -q 'auto_detect_slave_from_directory' "$whdbooter_source_file"; then
	echo "WHDLoad folder selections must scan plain directories for slave files" >&2
	exit 1
fi

if ! grep -F -q 'std::filesystem::is_directory(filepath' "$whdbooter_source_file"; then
	echo "WHDLoad fallback must branch on selected folders before archive scanning" >&2
	exit 1
fi

if ! grep -F -q 'std::filesystem::recursive_directory_iterator' "$whdbooter_source_file"; then
	echo "WHDLoad folder fallback must recurse selected directories for .slave files" >&2
	exit 1
fi

if ! grep -F -q 'auto_detect_slave_from_directory(filepath, game_detail)' "$whdbooter_source_file"; then
	echo "WHDLoad folder fallback must call the directory scanner" >&2
	exit 1
fi

if ! grep -F -q 'bool preserve_quickstart_hardware = false' "$options_source_file"; then
	echo "WHDBooter must expose an opt-in path for preserving Play Quickstart overrides" >&2
	exit 1
fi

if ! grep -F -q 'whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str(), manual_quickstart_override);' "$source_file"; then
	echo "Play WHDLoad content must preserve manual Quickstart model overrides" >&2
	exit 1
fi

if ! grep -F -q 'preserve_quickstart_hardware' "$whdbooter_source_file" ||
	! grep -F -q 'preserving Play Quickstart hardware override' "$whdbooter_source_file"; then
	echo "WHDBooter must skip hardware reset when Play asks to preserve Quickstart overrides" >&2
	exit 1
fi

if ! grep -F -q 'std::string sub_path;' "$options_source_file" ||
	! grep -F -q 'bool has_sub_path = false;' "$options_source_file"; then
	echo "WHDLoad slave entries must retain per-slave subpaths" >&2
	exit 1
fi

if ! grep -F -q 'slave.sub_path = s.subpath;' "$whdbooter_source_file" ||
	! grep -F -q 'slave.has_sub_path = true;' "$whdbooter_source_file" ||
	! grep -F -q 'selected_slave_sub_path()' "$whdbooter_source_file"; then
	echo "Auto-detected WHDLoad slaves must rebuild startup with the selected slave subpath" >&2
	exit 1
fi

if ! grep -F -q 's.sub_path + "/" + s.filename' "$whdload_source_file" ||
	! grep -F -q 'same_slave(whdload_prefs.slaves[i], whdload_prefs.selected_slave)' "$whdload_source_file"; then
	echo "WHDLoad panel must distinguish auto-detected slaves by subpath" >&2
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

if ! awk '
	/void apply_display_defaults_to_changed_prefs\(\)/ { in_apply = 1; next }
	in_apply && /\{/ { in_body = 1; next }
	in_body && /initialize_display_defaults\(\);/ { found = 1; exit }
	in_body && /play_apply_display_defaults/ { exit }
	END { exit found ? 0 : 1 }
' "$source_file"; then
	echo "Play content must initialize display defaults before applying them" >&2
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

if ! grep -F -q '*.fdi,*.scp,*.wrp,*.dsq' "$source_file" ||
	! grep -F -q '.fdi,.scp,.wrp,.dsq' "$source_file"; then
	echo "Play content picker must include all detected floppy formats" >&2
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
