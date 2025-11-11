
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"

#ifdef WITH_MIDIEMU

#include "fsdb.h"
#include "uae.h"
#include "audio.h"
#define MT32EMU_API_TYPE 1
#include <mt32emu.h>
#include "midiemu.h"
#include "parser.h"

// MUNT MT-32/CM-32L emulation

static mt32emu_context mt32context;
static int midi_emu_streamid;
static float base_midi_emu_event;
static int midi_evt_time;
static int midi_emu_freq;
int midi_emu;

static const TCHAR* cm32lctl[] = {
		_T("cm32l_control"),
		_T("ctrl_cm32l"),
		_T("ctrl_cm32ln_1_00"),
		_T("ctrl_cm32l_1_02"),
		_T("ctrl_cm32l_1_00"),
		_T("CM32L_CONTROL"),
		_T("CTRL_CM32L"),
		_T("CTRL_CM32LN_1_00"),
		_T("CTRL_CM32L_1_02"),
		_T("CTRL_CM32L_1_00"),
		NULL
};
static const TCHAR *mt32ctl[] = {
		_T("mt32_control"),
		_T("ctrl_mt32"),
		_T("ctrl_mt32_1_07"),
		_T("ctrl_mt32_1_06"),
		_T("ctrl_mt32_1_05"),
		_T("ctrl_mt32_1_04"),
		_T("ctrl_mt32_bluer"),
		_T("ctrl_mt32_2_04"),
		_T("ctrl_mt32_2_07"),
		_T("ctrl_mt32_2_06"),
		_T("ctrl_mt32_2_03"),
		_T("MT32_CONTROL"),
		_T("CTRL_MT32"),
		_T("CTRL_MT32_1_07"),
		_T("CTRL_MT32_1_06"),
		_T("CTRL_MT32_1_05"),
		_T("CTRL_MT32_1_04"),
		_T("CTRL_MT32_BLUER"),
		_T("CTRL_MT32_2_04"),
		_T("CTRL_MT32_2_07"),
		_T("CTRL_MT32_2_06"),
		_T("CTRL_MT32_2_03"),
		NULL
};

static bool check_rom(const TCHAR *path, const TCHAR *name)
{
	TCHAR rpath[MAX_DPATH];
	_tcscpy(rpath, path);
	_tcscat(rpath, name);
	_tcscat(rpath, _T(".rom"));
	bool exists = my_existsfile(rpath);
	if (!exists)
	{
		// try with uppercase also
		_tcscpy(rpath, path);
		_tcscat(rpath, name);
		_tcscat(rpath, _T(".ROM"));
		exists = my_existsfile(rpath);
	}
	return exists;
}

static bool load_rom(const TCHAR* path, const TCHAR* name)
{
	mt32emu_return_code err;
	TCHAR rpath[MAX_DPATH];
	char* n;

	// Helper function to attempt loading a ROM file
	auto try_load_rom = [&](const TCHAR* extension) -> bool {
		_tcscpy(rpath, path);
		_tcscat(rpath, name);
		_tcscat(rpath, extension);
		write_log(_T("mt32emu_add_rom_file(%s)\n"), rpath);
		n = ua(rpath);
		err = mt32emu_add_rom_file(mt32context, n);
		xfree(n);
		return err >= 0;
		};

	// Try loading with ".rom" and ".ROM" extensions
	if (try_load_rom(_T(".rom")) || try_load_rom(_T(".ROM"))) {
		write_log("-> %d\n", err);
		return true;
	}

	// Try loading with "_a.rom" and "_a.ROM" extensions
	if (try_load_rom(_T("_a.rom")) || try_load_rom(_T("_A.ROM"))) {
		// Try loading with "_b.rom" and "_b.ROM" extensions
		if (try_load_rom(_T("_b.rom")) || try_load_rom(_T("_B.ROM"))) {
			write_log("-> %d\n", err);
			return true;
		}
	}

	write_log("-> %d\n", err);
	return false;
}


static void midi_emu_add_roms(void)
{
	TCHAR path[MAX_DPATH];
	get_rom_path(path, sizeof(path) / sizeof(TCHAR));
	_tcscat(path, _T("mt32-roms/"));
//	if (!my_existsdir(path)) {
//		_tcscpy(path, _T("c:\\mt32-rom-data\\"));
//	}
	if (!my_existsdir(path)) {
		write_log(_T("mt32emu: rom path missing\n"));
		return;
	}
	if (midi_emu == 1) {
		if (!load_rom(path, _T("pcm_mt32")) && !load_rom(path, _T("PCM_MT32"))) {
			if (!load_rom(path, _T("mt32_pcm")) && !load_rom(path, _T("MT32_PCM"))) {
				load_rom(path, _T("pcm_mt32_l"));
				load_rom(path, _T("PCM_MT32_L"));
				load_rom(path, _T("pcm_mt32_h"));
				load_rom(path, _T("PCM_MT32_H"));
			}
		}
	} else {
		if (!load_rom(path, _T("pcm_cm32l")) && !load_rom(path, _T("PCM_CM32L"))) {
			if (!load_rom(path, _T("cm32l_pcm")) && !load_rom(path, _T("CM32L_PCM"))) {
				load_rom(path, _T("pcm_mt32"));
				load_rom(path, _T("PCM_MT32"));
				load_rom(path, _T("pcm_cm32l_h"));
				load_rom(path, _T("PCM_CM32L_H"));
			}
		}
	}
	const TCHAR **ctl = midi_emu == 1 ? mt32ctl : cm32lctl;
	for (int i = 0; ctl[i]; i++) {
		if (load_rom(path, ctl[i])) {
			break;
		}
	}
}

bool midi_emu_available(const TCHAR *id)
{
	TCHAR path[MAX_DPATH];
	int me = 0;

	if (_tcsstr(id, _T("MT-32"))) {
		me = 1;
	} else if (_tcsstr(id, _T("CM-32L"))) {
		me = 2;
	} else {
		return false;
	}

	for (int rc = 0; rc < 2; rc++) {
		if (rc == 0) {
			get_rom_path(path, sizeof(path) / sizeof(TCHAR));
		} else if (rc == 1) {
			get_rom_path(path, sizeof(path) / sizeof(TCHAR));
			_tcscat(path, _T("mt32-roms/"));
		//} else if (rc == 2) {
		//	_tcscpy(path, _T("C:\\mt32-rom-data\\"));
		}
		if (!my_existsdir(path)) {
			continue;
		}
		if (me == 1) {
			if (!check_rom(path, _T("pcm_mt32")) && !check_rom(path, _T("mt32_pcm"))
				&& !check_rom(path, _T("PCM_MT32")) && !check_rom(path, _T("MT32_PCM"))) {
				if ((!check_rom(path, _T("pcm_mt32_l")) && !check_rom(path, _T("PCM_MT32_L")))
					|| (!check_rom(path, _T("pcm_mt32_h")) && !check_rom(path, _T("PCM_MT32_H")))) {
					continue;
				}
			}
		} else {
			if (!check_rom(path, _T("pcm_cm32l")) && !check_rom(path, _T("cm32l_pcm"))
				&& !check_rom(path, _T("PCM_CM32L")) && !check_rom(path, _T("CM32L_PCM"))) {
				if ((!check_rom(path, _T("pcm_mt32")) && !check_rom(path, _T("PCM_MT32")))
					|| (!check_rom(path, _T("pcm_cm32l_h")) && !check_rom(path, _T("PCM_CM32L_H")))) {
					continue;
				}
			}
		}
		const TCHAR **ctl = me == 1 ? mt32ctl : cm32lctl;
		for (int i = 0; ctl[i]; i++) {
			if (check_rom(path, ctl[i])) {
				return true;
			}
		}
	}
	return false;
}

void midi_update_sound(float v)
{
	base_midi_emu_event = v;
}

static bool audio_state_midi_emu(int streamid, void *params)
{
	int sample[2] = { 0 };

	if (mt32context) {
		int vol = (100 - currprefs.sound_volume_midi) * 32768 / 100;
		mt32emu_bit16s stream[2];
		mt32emu_render_bit16s(mt32context, stream, 1);
		sample[0] = stream[0] * vol / 32768;
		sample[1] = stream[1] * vol / 32768;
	}

	midi_evt_time = (int)(base_midi_emu_event * CYCLE_UNIT / midi_emu_freq);
	audio_state_stream_state(streamid, sample, 2, midi_evt_time);
	return true;
}

void midi_emu_parse(uae_u8 *midi, int len)
{
	if (mt32context) {
		mt32emu_parse_stream(mt32context, midi, len);
	}
}

void midi_emu_close(void)
{
	midi_emu = 0;
	if (midi_emu_streamid) {
		audio_enable_stream(false, midi_emu_streamid, 0, NULL, NULL);
		midi_emu_streamid = 0;
	}
	if (mt32context) {
		mt32emu_close_synth(mt32context);
		mt32emu_free_context(mt32context);
	}
	mt32context = NULL;
}

int midi_emu_open(const TCHAR *id)
{
	mt32emu_return_code err;
	mt32emu_rom_info ri;

	if (_tcsstr(id, _T("MT-32"))) {
		midi_emu = 1;
	} else if (_tcsstr(id, _T("CM-32L"))) {
		midi_emu = 2;
	} else {
		return 0;
	}
	const char *s = mt32emu_get_library_version_string();
	write_log("mt32emu version: %s\n", s);
	mt32emu_report_handler_i handler;
	mt32context = mt32emu_create_context(handler, NULL);
	if (!mt32context) {
		write_log("mt32emu_create_context() failed\n");
		return 0;
	}
	midi_emu_add_roms();
	mt32emu_set_analog_output_mode(mt32context, MT32EMU_AOM_ACCURATE);
	mt32emu_get_rom_info(mt32context, &ri);
	write_log("mt32emu control_rom_id: %s\n", ri.control_rom_id);
	write_log("mt32emu control_rom_description: %s\n", ri.control_rom_description);
	write_log("mt32emu control_rom_sha1_digest: %s\n", ri.control_rom_sha1_digest);
	write_log("mt32emu pcm_rom_id: %s\n", ri.pcm_rom_id);
	write_log("mt32emu pcm_rom_description: %s\n", ri.pcm_rom_description);
	write_log("mt32emu pcm_rom_sha1_digest: %s\n", ri.pcm_rom_sha1_digest);
	err = mt32emu_open_synth(mt32context);
	if (err != MT32EMU_RC_OK) {
		write_log("mt32emu_open_synth() failed: %d\n", err);
		midi_emu_close();
		return 0;
	}
	midi_emu_freq = mt32emu_get_actual_stereo_output_samplerate(mt32context);
	write_log("mt32emu frequency: %d\n", midi_emu_freq);
	midi_emu_streamid = audio_enable_stream(true, -1, 2, audio_state_midi_emu, NULL);

	return 1;
}

void midi_emu_reopen(void)
{
	if (midi_emu) {
		midi_emu_close();
		if (currprefs.midioutdev[0]) {
			midi_emu_open(currprefs.midioutdev);
		}
	}
}

#endif
