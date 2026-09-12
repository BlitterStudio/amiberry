#!/usr/bin/env python3
"""Exercise global port defaults with the production joyport parser and catalog."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def between(source, start, end):
    begin = source.index(start)
    return source[begin:source.index(end, begin)]


def main():
    native = (ROOT / "src/osdep/amiberry.cpp").read_text(encoding="utf-8")
    core = (ROOT / "src/inputdevice.cpp").read_text(encoding="utf-8")
    panel = (ROOT / "src/osdep/imgui/input.cpp").read_text(encoding="utf-8")
    cfgfile = (ROOT / "src/cfgfile.cpp").read_text(encoding="utf-8")
    options = (ROOT / "src/include/options.h").read_text(encoding="utf-8")
    port_types = between(options, "struct inputdevconfig {", "\n#define JPORT_UNPLUGGED")
    defaults = between(native, "\tp->use_retroarch_vkbd =", "\twhdload_prefs.button_wait")
    parser = between(core, "void inputdevice_joyport_config_store(", "int inputdevice_getjoyportdevice (")
    catalog = between(panel, "static std::vector<InputDeviceOption>", "static int get_device_index(")
    fixup = between(core, "void inputdevice_fix_prefs(", "// for state recorder use only!")
    config_ports = between(cfgfile, '\tif (_tcscmp (option, _T("joyport0"))',
                           '\tif (cfgfile_strval(option, value, _T("joyport0mode")')
    fixture = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#define TCHAR char
#define _T(s) s
#define _tcscpy std::strcpy
#define _tcscmp std::strcmp
#define _tcsncmp std::strncmp
#define _tstol std::atol
#define _sntprintf std::snprintf
constexpr int JPORT_UNPLUGGED = -2;
constexpr int JPORT_NONE = -1, JSEM_KBDLAYOUT = 0, JSEM_CUSTOM = 10;
constexpr int JSEM_JOYS = 100, JSEM_MICE = 200, JSEM_LASTKBD = 9;
constexpr int MAX_JPORTS = 4, MAX_JPORTS_CUSTOM = 6, MAX_INPUT_DEVICES = 8;
constexpr int MAX_JPORT_NAME = 128, MAX_JPORT_CONFIG = 256;
constexpr int IDTYPE_JOYSTICK = 0, IDTYPE_MOUSE = 1;
constexpr int INPUT_MATCH_BOTH = 1, INPUT_MATCH_CONFIG_NAME_ONLY = 2, INPUT_MATCH_FRIENDLY_NAME_ONLY = 4;
''' + port_types + r'''
struct jport_custom { char custom[256]{}; };
struct uae_prefs {
    jport jports[MAX_JPORTS];
    jport_custom jports_custom[MAX_JPORTS_CUSTOM];
    int input_device_match_mask = 7;
    bool use_retroarch_vkbd = false;
};
struct {
    const char* default_controller1 = "kbd9";
    const char* default_controller2 = "joy2";
    const char* default_controller3 = "";
    const char* default_controller4 = "";
    const char* default_mouse1 = "mouse";
    bool default_retroarch_vkbd = false;
} amiberry_options;
int default_keyboard_layout[MAX_JPORTS]{};
int joystick_count = 3;
int joy_count() { return joystick_count; }
int mouse_count() { return 1; }
const char* joy_name(int i) { return i == 0 ? "USB Gamepad" : "USB Joystick"; }
const char* mouse_name(int) { return "System mouse"; }
struct inputdevice_functions {
    int (*get_num)(); const char* (*get_friendlyname)(int); const char* (*get_uniquename)(int);
};
inputdevice_functions idev[] = {{joy_count, joy_name, joy_name}, {mouse_count, mouse_name, mouse_name}};
void set_config_changed() {}
void inputdevice_store_used_device(jport*, int, bool) {}
void inputdevice_get_previous_joy(uae_prefs* p, int port, bool) { p->jports[port].id = JPORT_NONE; }
// Port arbitration and device history are outside this isolated parser fixture.
// Every assigned device in these checks is distinct, so arbitration is inert.
jport jport_config_store[MAX_JPORTS];
void inputdevice_validate_jports(uae_prefs*, int, bool*) {}
void inputdevice_generate_jport_custom(uae_prefs*, int) {}
void freejport(uae_prefs* p, int port) { p->jports[port].id = JPORT_NONE; }
void write_log(const char*, ...) {}
struct InputDeviceOption { std::string label, config_value; int id; };
struct { bool is_controller = false; } di_joystick[MAX_INPUT_DEVICES];
int inputdevice_get_device_total(int type) { return idev[type].get_num(); }
const char* inputdevice_get_device_name(int type, int i) { return idev[type].get_friendlyname(i); }
'''
    checks = r'''
int main() {
    // The catalog must offer physical devices after the keyboard layouts.
    di_joystick[0].is_controller = true;
    poll_input_devices();
    bool gamepad = false, joystick = false;
    for (const auto& option : input_device_options) {
        if (option.id == JSEM_JOYS) gamepad = option.config_value == "joy0";
        if (option.id == JSEM_JOYS + 1) joystick = option.config_value == "joy1";
    }
    assert(gamepad && joystick);
    std::cout << "Connected gamepad and joystick are selectable in the shared catalog\n";

    uae_prefs p{};
    apply_defaults(&p);
#ifdef LIBRETRO
    assert(p.jports[0].id == JSEM_MICE && p.jports[1].id == JSEM_JOYS);
#else
    if (p.jports[1].id != JSEM_KBDLAYOUT + 8) {
        std::cerr << "Keyboard Layout D ignored: port 1 ID=" << p.jports[1].id << ", expected 8\n";
        return 1;
    }
    assert(p.jports[0].id == JSEM_MICE);
    // A later fixup must not recover an old USB identity over the keyboard.
    std::strcpy(p.jports[1].idc.name, "USB Gamepad");
    std::strcpy(p.jports[1].idc.configname, "old-usb-id");
    apply_defaults(&p);
    inputdevice_fix_prefs(&p, true);
    assert(p.jports[1].id == JSEM_KBDLAYOUT + 8);

    // Loading a per-game .uae binding must still override the global default.
    parse_port_config(&p, "joyport1", "kbd2");
    inputdevice_fix_prefs(&p, true);
    assert(p.jports[1].id == JSEM_KBDLAYOUT + 1);

    // Device tokens retain the core parser's zero-based joystick indexing.
    amiberry_options.default_controller1 = "joy2";
    amiberry_options.default_controller3 = "kbd1";
    amiberry_options.default_controller4 = "kbd2";
    apply_defaults(&p);
    assert(p.jports[1].id == JSEM_JOYS + 2);
    assert(p.jports[2].id == JSEM_KBDLAYOUT && p.jports[3].id == JSEM_KBDLAYOUT + 1);

    amiberry_options.default_controller1 = "none";
    amiberry_options.default_mouse1 = "none";
    apply_defaults(&p);
    assert(p.jports[0].id == JPORT_NONE && p.jports[1].id == JPORT_NONE);

    // Empty legacy settings keep the normal mouse/first-joystick fallback.
    amiberry_options.default_controller1 = "";
    amiberry_options.default_mouse1 = "";
    apply_defaults(&p);
    assert(p.jports[0].id == JSEM_MICE && p.jports[1].id == JSEM_JOYS);
    // A keyboard layout needs no physical controller to be connected.
    joystick_count = 0;
    amiberry_options.default_controller1 = "kbd9";
    apply_defaults(&p);
    assert(p.jports[1].id == JSEM_KBDLAYOUT + 8);
    // A token that fits exactly, including its terminator, remains valid.
    const std::string longest_keyboard = "kbd"
        + std::string(sizeof p.jports[1].idc.shortid - 5, '0') + "9";
    amiberry_options.default_controller1 = longest_keyboard.c_str();
    apply_defaults(&p);
    inputdevice_fix_prefs(&p, true);
    assert(p.jports[1].id == JSEM_KBDLAYOUT + 8);

    // Manually edited global options can be longer than a port short ID.
    // Exercise the first overflowing length and the largest option value,
    // without truncating malformed values into a different device token.
    joystick_count = 3;
    amiberry_options.default_controller1 = "kbd9";
    amiberry_options.default_mouse1 = "mouse";
    amiberry_options.default_controller3 = "";
    amiberry_options.default_controller4 = "";
    const char** device_defaults[] = {
        &amiberry_options.default_mouse1, &amiberry_options.default_controller1,
        &amiberry_options.default_controller3, &amiberry_options.default_controller4
    };
    const int fallback_ids[] = { JSEM_MICE, JSEM_JOYS, JPORT_NONE, JPORT_NONE };
    for (const size_t length : { size_t(16), size_t(127) }) {
        const std::string oversized(length, 'x');
        for (int port = 0; port < MAX_JPORTS; ++port) {
            const char* saved = *device_defaults[port];
            *device_defaults[port] = oversized.c_str();
            uae_prefs malformed{};
            for (auto& jp : malformed.jports) {
                jp.nokeyboardoverride = true;
                jp.mode = 3;
                jp.autofire = 2;
            }
            apply_defaults(&malformed);
            for (const auto& jp : malformed.jports) {
                assert(jp.nokeyboardoverride);
                assert(jp.mode == 3 && jp.autofire == 2);
            }
            inputdevice_fix_prefs(&malformed, true);
            assert(malformed.jports[port].id == fallback_ids[port]);
            *device_defaults[port] = saved;
        }
    }
#endif
    std::cout << "Global input defaults: passed\n";
}
'''
    source = (fixture + parser + fixup + catalog
              + "\nint parse_port_config(uae_prefs* p, const char* option, const char* value) {\n"
              + config_ports + "return 0;\n}\n"
              + "\nvoid apply_defaults(uae_prefs* p) {\n" + defaults + "}\n" + checks)
    with tempfile.TemporaryDirectory(prefix="amiberry-input-defaults-") as tmp:
        cpp = Path(tmp) / "test.cpp"
        exe = Path(tmp) / ("test.exe" if os.name == "nt" else "test")
        cpp.write_text(source, encoding="utf-8")
        for mode in ([], ["-DLIBRETRO"]):
            subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + ["-std=c++17", *mode, str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
