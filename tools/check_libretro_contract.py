#!/usr/bin/env python3
"""Check libretro API contract details that are easy to regress by refactor."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "libretro" / "libretro.cpp"
STUB_SOURCE = ROOT / "libretro" / "libretro_stubs.cpp"
SDL_STUB_SOURCE = ROOT / "libretro" / "sdl_stub.cpp"
AMIBERRY_SOURCE = ROOT / "src" / "osdep" / "amiberry.cpp"
FILESYS_SOURCE = ROOT / "src" / "filesys.cpp"
DEBUG_SOURCE = ROOT / "src" / "debug.cpp"
MAIN_SOURCE = ROOT / "src" / "main.cpp"
MAIN_WINDOW_SOURCE = ROOT / "src" / "osdep" / "gui" / "main_window.cpp"
WHDBOOTER_SOURCE = ROOT / "src" / "osdep" / "amiberry_whdbooter.cpp"
SOUND_LIBRETRO_SOURCE = ROOT / "src" / "sounddep" / "sound_platform_internal_libretro.h"


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
	stub_text = STUB_SOURCE.read_text(encoding="utf-8")
	sdl_stub_text = SDL_STUB_SOURCE.read_text(encoding="utf-8")
	amiberry_text = AMIBERRY_SOURCE.read_text(encoding="utf-8")
	filesys_text = FILESYS_SOURCE.read_text(encoding="utf-8")
	debug_text = DEBUG_SOURCE.read_text(encoding="utf-8")
	main_text = MAIN_SOURCE.read_text(encoding="utf-8")
	main_window_text = MAIN_WINDOW_SOURCE.read_text(encoding="utf-8")
	whdbooter_text = WHDBOOTER_SOURCE.read_text(encoding="utf-8")
	sound_libretro_text = SOUND_LIBRETRO_SOURCE.read_text(encoding="utf-8")
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
	retro_load_game = extract_body(text, "bool retro_load_game(const struct retro_game_info *info)")
	reset_core_runtime_state = extract_body(text, "static void reset_core_runtime_state()")
	get_core_refresh_rate = extract_body(text, "static float get_core_refresh_rate()")
	audio_buffer_status_cb = extract_body(text, "static void RETRO_CALLCONV audio_buffer_status_cb(bool active, unsigned occupancy, bool underrun_likely)")
	libretro_emit_audio_starvation_guard = extract_body(text, "static void libretro_emit_audio_starvation_guard()")
	libretro_drain_audio_frame = extract_body(text, "static void libretro_drain_audio_frame()")
	libretro_fill_audio_padding = extract_body(text, "static void libretro_fill_audio_padding(int16_t* stereo_frames, unsigned frames)")
	libretro_audio_reset = extract_body(text, "void libretro_audio_reset(void)")
	libretro_scan_roms = extract_body(stub_text, "int scan_roms(int show)")
	sdl_create_semaphore = extract_body(sdl_stub_text, "SDL_Semaphore* SDL_CreateSemaphore(Uint32 initial_value)")
	sdl_wait_semaphore = extract_body(sdl_stub_text, "void SDL_WaitSemaphore(SDL_Semaphore* s)")
	sound_platform_output_audio = extract_body(
		sound_libretro_text,
		"static inline bool sound_platform_output_audio(struct sound_data* sd, uae_u16* sndbuffer)",
	)
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
	filesys_iteration = extract_body(filesys_text, "static int filesys_iteration(UnitInfo *ui)")
	filesys_start_thread = extract_body(filesys_text, "static void filesys_start_thread (UnitInfo *ui, int nr)")
	startup_handler = extract_body(filesys_text, "static uae_u32 REGPARAM2 startup_handler(TrapContext *ctx)")
	filesys_start_threads = extract_body(filesys_text, "void filesys_start_threads (void)")
	dump_xlate = extract_body(debug_text, "static uae_u8 *dump_xlate (uae_u32 addr)")
	dump_xlate_range = extract_body(debug_text, "static uae_u8 *dump_xlate_range (uae_u32 addr, uae_u32 size)")
	memory_map_dump_3 = extract_body(debug_text, "static void memory_map_dump_3(UaeMemoryMap *map, int log)")
	whdload_auto_prefs = extract_body(whdbooter_text, "void whdload_auto_prefs(uae_prefs* prefs, const char* filepath)")

	require(
		"RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY" not in set_environment,
		"minimum audio latency must not be set from retro_set_environment()",
		failures,
	)
	require(
		"return vblank_hz" not in get_core_refresh_rate
		and "currprefs.ntscmode ? 60.0f : 50.0f" in get_core_refresh_rate
		and "RETRO_ENVIRONMENT_SET_GEOMETRY" in text,
		"libretro frontend timing must stay PAL/NTSC-stable; native mode changes should use SET_GEOMETRY, not full A/V resets",
		failures,
	)
	require(
		"apply_minimum_audio_latency();" in retro_run,
		"retro_run() must apply minimum audio latency from the allowed callback",
		failures,
	)
	require(
		retro_run.count("libretro_audio_frames_this_run = 0;") >= 2
		and retro_run.count("libretro_emit_audio_starvation_guard();") >= 2
		and "audio_rate_for_av_info()" in libretro_emit_audio_starvation_guard
		and "get_core_refresh_rate()" in libretro_emit_audio_starvation_guard
		and "libretro_audio_deficit_frames += expected_frames;" in libretro_emit_audio_starvation_guard
		and "libretro_audio_deficit_frames -= libretro_audio_frames_this_run;" in libretro_emit_audio_starvation_guard
		and "libretro_audio_deficit_frames = max_deferred_frames * 4.0;" in libretro_emit_audio_starvation_guard
		and "if (libretro_audio_deficit_frames <= max_deferred_frames)" in libretro_emit_audio_starvation_guard
		and "libretro_fill_audio_padding(padding, chunk)" in libretro_emit_audio_starvation_guard
		and "audio_batch_cb(padding, chunk)" in libretro_emit_audio_starvation_guard
		and "audio_cb(padding[i * 2], padding[i * 2 + 1])" in libretro_emit_audio_starvation_guard
		and "libretro_audio_note_emitted(padding, accepted)" in libretro_emit_audio_starvation_guard
		and "libretro_audio_deficit_frames = 0.0;" in libretro_audio_reset
		and "libretro_audio_reset();" in reset_core_runtime_state,
		"retro_run() must guard sustained libretro audio starvation with click-safe padding, without padding normal bursty audio frames",
		failures,
	)
	require(
		"libretro_frontend_audio_active.store(active" in audio_buffer_status_cb
		and "libretro_frontend_audio_occupancy.store(occupancy" in audio_buffer_status_cb
		and "libretro_frontend_audio_underrun_likely.store(underrun_likely" in audio_buffer_status_cb
		and "libretro_frontend_audio_underrun_log_skip++ == 0" in audio_buffer_status_cb
		and "libretro_frontend_audio_underrun_likely.load" in libretro_drain_audio_frame
		and "libretro_frontend_audio_occupancy.load" in libretro_drain_audio_frame
		and "emit += std::min(boost" in libretro_drain_audio_frame
		and "libretro_audio_last_left" in libretro_fill_audio_padding
		and "std::fill(stereo_frames" in libretro_fill_audio_padding,
		"libretro audio must react to frontend underrun warnings and ramp starvation padding from the last emitted sample",
		failures,
	)
	# The Amiga engine flushes audio in fixed chunks that do not align with the
	# video-frame boundary (a 768/1536 per-frame sawtooth). The libretro path has
	# no SDL host queue to absorb it, so it must buffer in a FIFO and re-pace the
	# stream once per video frame, accounting only the frames the frontend accepts.
	require(
		"libretro_audio_enqueue(samples" in sound_platform_output_audio
		and "libretro_audio_frames_this_run" not in sound_platform_output_audio,
		"libretro audio output must enqueue into the smoothing FIFO, not push to the frontend directly",
		failures,
	)
	require(
		retro_run.count("libretro_drain_audio_frame();") >= 2
		and "libretro_audio_fifo" in libretro_drain_audio_frame
		and "libretro_audio_frames_this_run += static_cast<unsigned>(accepted);" in libretro_drain_audio_frame,
		"libretro audio must be paced per video frame and account frames accepted by the frontend callbacks",
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
	require(
		"scan_single_rom_file(f)" in stub_text
		and "regcreatetree(nullptr, _T(\"DetectedROMs\"))" in libretro_scan_roms
		and "AMIBERRY_LIBRETRO_SYSTEM_DIR" in libretro_scan_roms,
		"libretro scan_roms() must populate checksum-backed DetectedROMs from frontend ROM paths",
		failures,
	)
	require(
		"static bool libretro_is_rom_key_file" in stub_text
		and "if (libretro_is_rom_key_file(path)) {\n\t\taddkeyfile(path);\n\t\treturn 0;\n\t}" in stub_text
		and "if (!libretro_is_rom_key_file(path) && !libretro_is_rom_ext(path, deepscan))" in stub_text
		and "for (const auto& file : files) {\n\t\tif (libretro_is_rom_key_file(file))\n\t\t\tlibretro_scan_rom_file(file, fkey, deepscan);\n\t}" in stub_text,
		"libretro ROM scanning must let rom.key through and register keys before encrypted ROM candidates",
		failures,
	)
	require(
		"if (initial)\n\t\trescan_roms = true;" in amiberry_text,
		"libretro initial startup must rescan ROMs because it has no GUI rescan path",
		failures,
	)
	require(
		"#ifdef LIBRETRO\n\tmultithread_enabled = false;\n#else\n\tmultithread_enabled = true;\n#endif" in amiberry_text,
		"libretro must default multithreaded Denise drawing off to avoid reset-time queue flush stalls",
		failures,
	)
	require(
		"pick_whdload_kickstart(" not in text,
		"WHDLoad auto mode must rely on checksum ROM discovery, not strict Kickstart filenames",
		failures,
	)
	require(
		"whd_path = canonicalize_existing_host_path(whd_path);" in retro_load_game,
		"WHDLoad archive paths must be canonicalized before being passed to WHDBooter",
		failures,
	)
	require(
		"bool used_archive_auto_detect = false;" in whdload_auto_prefs
		and "used_archive_auto_detect = auto_detect_slave_from_archive(filepath, game_detail);" in whdload_auto_prefs
		and "const auto use_a1200_for_unknown = chipset_unknown && used_archive_auto_detect;" in whdload_auto_prefs,
		"WHDBooter must only use the A1200 unknown-chipset fallback for archive auto-detect, not matched database entries",
		failures,
	)
	require(
		"set_compatibility_settings(prefs, game_detail, a600_available, is_aga || is_cd32 || use_a1200_for_unknown);" in whdload_auto_prefs,
		"WHDBooter A1200 unknown-chipset fallback must also keep CPU compatibility on the A1200 path",
		failures,
	)
	require(
		"if (ctx)\n\t\t\ttrap_background_set_complete(ctx);" in filesys_iteration,
		"filesystem thread shutdown must not complete a null trap context from the death message",
		failures,
	)
	require(
		"clear_unit_state" not in filesys_start_thread
		and "filesys_start_thread (ui, i);" in filesys_start_threads
		and "is_virtual(i) && !ui->self" not in filesys_start_threads
		and "filesys_start_thread(uinfo, nr);" in startup_handler
		and "unit = startup_create_unit(ctx, uinfo, nr);" in startup_handler,
		"virtual filesystem threads must keep the normal startup order; only null teardown contexts should be guarded",
		failures,
	)
	require(
		"pthread_mutex_init(&s->mutex" in sdl_create_semaphore
		and "pthread_cond_init(&s->cond" in sdl_create_semaphore
		and "sem_init(" not in sdl_create_semaphore
		and "while (s->value == 0)" in sdl_wait_semaphore
		and "pthread_cond_wait(&s->cond" in sdl_wait_semaphore,
		"libretro SDL semaphore stub must use pthread condvars, not macOS-unsupported sem_init",
		failures,
	)
	require(
		"!bank->check || !bank->xlateaddr" in dump_xlate
		and "!bank->check || !bank->xlateaddr" in dump_xlate_range,
		"debug memory map dumping must validate bank callbacks before translating reset-time banks",
		failures,
	)
	require(
		"uae_u8 *crc_mem = dump_xlate_range((j << 16) | bankoffset, crc_size);" in memory_map_dump_3
		and "get_crc32(crc_mem, crc_size)" in memory_map_dump_3
		and "a1->xlateaddr((j << 16) | bankoffset)" not in memory_map_dump_3,
		"debug memory map ROM CRC dumping must use guarded ranged translation",
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
