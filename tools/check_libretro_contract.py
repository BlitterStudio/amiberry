#!/usr/bin/env python3
"""Check libretro API contract details that are easy to regress by refactor."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "libretro" / "libretro.cpp"
AMIBERRY_SOURCE = ROOT / "src" / "osdep" / "amiberry.cpp"
MAIN_SOURCE = ROOT / "src" / "main.cpp"
MAIN_WINDOW_SOURCE = ROOT / "src" / "osdep" / "gui" / "main_window.cpp"


def extract_body(text, signature):
	start = text.find(signature)
	if start == -1:
		raise AssertionError(f"missing {signature}")
	brace = text.find("{", start)
	if brace == -1:
		raise AssertionError(f"missing body for {signature}")
	depth = 0
	for pos in range(brace, len(text)):
		if text[pos] == "{":
			depth += 1
		elif text[pos] == "}":
			depth -= 1
			if depth == 0:
				return text[brace + 1:pos]
	raise AssertionError(f"unterminated body for {signature}")


def extract_if_body(text, condition):
	start = text.find(condition)
	if start == -1:
		raise AssertionError(f"missing {condition}")
	brace = text.find("{", start)
	if brace == -1:
		raise AssertionError(f"missing body for {condition}")
	depth = 0
	for pos in range(brace, len(text)):
		if text[pos] == "{":
			depth += 1
		elif text[pos] == "}":
			depth -= 1
			if depth == 0:
				return text[brace + 1:pos]
	raise AssertionError(f"unterminated body for {condition}")


def require(condition, message, failures):
	if not condition:
		failures.append(message)


def main():
	text = SOURCE.read_text(encoding="utf-8")
	amiberry_text = AMIBERRY_SOURCE.read_text(encoding="utf-8")
	main_text = MAIN_SOURCE.read_text(encoding="utf-8")
	main_window_text = MAIN_WINDOW_SOURCE.read_text(encoding="utf-8")
	failures = []

	set_environment = extract_body(text, "void retro_set_environment(retro_environment_t cb)")
	poll_frontend_input = ""
	try:
		poll_frontend_input = extract_body(text, "static bool poll_frontend_input(void)")
	except AssertionError:
		failures.append("libretro first-frame input polling must be split from Amiberry input injection")
	poll_input = extract_body(text, "static void poll_input(void)")
	retro_run = extract_body(text, "void retro_run(void)")
	startup = extract_if_body(retro_run, "if (!core_started)")
	retro_reset = extract_body(text, "void retro_reset(void)")
	retro_deinit = extract_body(text, "void retro_deinit(void)")
	retro_unload_game = extract_body(text, "void retro_unload_game(void)")
	retro_get_system_info = extract_body(text, "void retro_get_system_info(struct retro_system_info *info)")
	retro_serialize = extract_body(text, "bool retro_serialize(void *data, size_t size)")
	retro_unserialize = extract_body(text, "bool retro_unserialize(const void *data, size_t size)")
	retro_get_memory_data = extract_body(text, "void *retro_get_memory_data(unsigned id)")
	retro_get_memory_size = extract_body(text, "size_t retro_get_memory_size(unsigned id)")
	setup_whdload_paths = extract_body(text, "static void setup_whdload_paths()")
	get_seed_source_candidates = extract_body(
		amiberry_text,
		"static std::vector<std::string> get_seed_source_candidates(const std::string& subdirectory,\n\tconst bool include_usr_local_share)",
	)
	libretro_whdboot_seed_candidates = extract_body(
		amiberry_text,
		"static void append_libretro_whdboot_seed_source_candidates(std::vector<std::string>& candidates,\n\tconst std::string& subdirectory)",
	)
	parse_cmdline = extract_body(main_text, "static void parse_cmdline (int argc, TCHAR **argv)")
	drop_handler = extract_body(main_window_text, "static void handle_drop_file_event(const SDL_Event& event)")
	copy_seed_dir = extract_body(
		amiberry_text,
		"static bool copy_missing_directory_contents_if_exists(const std::string& source_dir, const std::string& destination_dir)",
	)

	require(
		"RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY" not in set_environment,
		"minimum audio latency must not be set from retro_set_environment()",
		failures,
	)
	require(
		"apply_minimum_audio_latency();" in retro_run,
		"retro_run() must apply minimum audio latency from the allowed callback",
		failures,
	)
	frontend_poll_pos = startup.find("poll_frontend_input();")
	switch_pos = startup.find("co_switch(core_fiber)")
	require(
		frontend_poll_pos != -1 and switch_pos != -1 and frontend_poll_pos < switch_pos,
		"the first retro_run() path must poll the frontend before switching to the core fiber",
		failures,
	)
	require(
		"poll_input();" not in startup,
		"the first retro_run() path must not inject frontend input before Amiberry input devices are initialized",
		failures,
	)
	require(
		"input_poll_cb();" in poll_frontend_input and "poll_frontend_input()" in poll_input,
		"normal libretro input polling must keep calling the frontend input_poll callback",
		failures,
	)
	require(
		"static bool core_is_running()" in text
		and "return core_started && !core_shutdown_complete;" in text,
		"libretro callbacks must have a shared live-core guard",
		failures,
	)
	require(
		"if (core_shutdown_complete)" in retro_run
		and "if (!ensure_core_fiber()) {\n\t\tpoll_frontend_input();\n\t\treturn;\n\t}" in retro_run
		and "if (core_shutdown_complete)\n\t\t\treturn;" in startup
		and "if (core_shutdown_complete) {\n\t\tpoll_frontend_input();\n\t\treturn;\n\t}" in retro_run,
		"retro_run() must poll the frontend before early returns that skip runtime work",
		failures,
	)
	require(
		"if (!core_is_running())\n\t\treturn;" in retro_reset,
		"retro_reset() must not call UAE reset before the core is running",
		failures,
	)
	require(
		"if (!core_is_running() || !data || size == 0)\n\t\treturn false;" in retro_serialize,
		"retro_serialize() must reject calls before live core state exists",
		failures,
	)
	require(
		"if (!core_is_running() || !data || size == 0)\n\t\treturn false;" in retro_unserialize,
		"retro_unserialize() must reject calls before live core state exists",
		failures,
	)
	require(
		"if (!core_is_running() || !chipmem_bank.baseaddr)\n\t\treturn NULL;" in retro_get_memory_data,
		"retro_get_memory_data() must not expose memory before chip RAM is live",
		failures,
	)
	require(
		"if (!core_is_running() || !chipmem_bank.baseaddr)\n\t\treturn 0;" in retro_get_memory_size,
		"retro_get_memory_size() must not report memory before chip RAM is live",
		failures,
	)
	require(
		"shutdown_core_fiber();" in retro_deinit,
		"retro_deinit() must use the shared core shutdown helper",
		failures,
	)
	require(
		"shutdown_core_fiber();" in retro_unload_game,
		"retro_unload_game() must pump core shutdown, not just request uae_quit()",
		failures,
	)
	require(
		"info->block_extract    = true;" in retro_get_system_info,
		"retro_get_system_info() must block frontend archive extraction for Amiberry-managed archive formats",
		failures,
	)
	require(
		'info->valid_extensions = "adf|adz|dms|fdi|raw|ipf|hdf|hdz|lha|lzh|zip|7z|uae|rp9|m3u|m3u8|iso|cue|ccd|nrg|mds|chd";' in retro_get_system_info
		and '_tcscmp(txt2.c_str(), ".7z") == 0' in parse_cmdline,
		"libretro must not block frontend extraction for advertised .7z content unless .7z loads directly",
		failures,
	)
	require(
		'strcasecmp(ext.c_str(), ".7z") == 0' in drop_handler,
		"desktop drag/drop floppy archive handling must include .7z when CLI direct loading does",
		failures,
	)
	require(
		"ensure_whdload_save_dirs(save_dir);" in setup_whdload_paths,
		"setup_whdload_paths() must create the WHDLoad save-data subdirectories exposed via WHDBOOT_SAVE_DATA",
		failures,
	)
	require(
		"set_whdbootpath(" not in setup_whdload_paths,
		"setup_whdload_paths() must not point WHDBooter at the frontend system dir, which may be read-only",
		failures,
	)
	require(
		"append_libretro_whdboot_seed_source_candidates(candidates, subdirectory);" in get_seed_source_candidates
		and "AMIBERRY_WHDBOOT_PATH" in libretro_whdboot_seed_candidates
		and "AMIBERRY_LIBRETRO_SYSTEM_DIR" in libretro_whdboot_seed_candidates,
		"WHDLoad seed candidates must include libretro frontend asset directories",
		failures,
	)
	require(
		"path_strings_match(source_dir, destination_dir)" in copy_seed_dir,
		"directory seeding must skip identical source/destination paths",
		failures,
	)
	require(
		"#ifdef LIBRETRO\nstatic bool seed_whdboot_files_from_candidates" in amiberry_text,
		"libretro-specific WHDBooter seed validation must stay out of non-libretro builds",
		failures,
	)
	require(
		"#ifdef LIBRETRO\n\tif (!seed_whdboot_files_from_candidates(source_candidates))\n#else\n\tif (!seed_missing_directory_contents_from_candidates(whdboot_path, source_candidates))" in amiberry_text,
		"non-libretro WHDBooter seeding must keep using the established generic seed-copy path",
		failures,
	)

	if failures:
		for failure in failures:
			print(f"FAIL: {failure}")
		return 1

	print("libretro contract checks passed")
	return 0


if __name__ == "__main__":
	sys.exit(main())
