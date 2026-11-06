#!/usr/bin/env python3
"""Compile the production recorder/input functions and prove failure reporting."""

import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]


def extract(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    # Rebinding the shared collection also catches accidental from-import
    # snapshots, even if the helper changes its collection representation.
    negative = subprocess.run(
        [sys.executable, "-B", "-c",
         "import sys; sys.path.insert(0, 'tests'); "
         "import ipc_joystick_live_test as live; "
         "live.ipc_actions_test.failures = []; "
         "live.check('deliberate negative mutation', False); "
         "sys.exit(live.exit_status())"],
        cwd=ROOT, capture_output=True, text=True)
    assert negative.returncode == 1, (negative.returncode, negative.stdout, negative.stderr)
    assert "FAIL deliberate negative mutation" in negative.stdout, negative.stdout
    print("Live-test negative check exits 1", flush=True)

    source = (ROOT / "src/inputdevice.cpp").read_text()
    production = "\n\n".join(extract(source, signature) for signature in (
        "static bool fixjport(struct jport *port",
        "static void joymousecounter (int joy)",
        "static void update_joyport_input(int port)",
        "bool inputdevice_joystick_is_cd32(int port)",
        "void inputdevice_set_joystick_state(uae_u64 client",
        "void inputdevice_release_joystick(uae_u64 client)",
        "uae_u8 *save_inputstate (size_t *len",
        "uae_u8 *restore_inputstate (uae_u8 *src)"))
    physical_start = source.index("\t\t\t\tjoydir[joy] = 0;")
    physical_end = source.index("\n\t\t\t\t//gui_gameport_axis_change", physical_start)
    production += r"""
static void physical_directions(int joy, int mask, bool allowoppositestick)
{
    const bool left = mask & DIR_LEFT, right = mask & DIR_RIGHT;
    const bool top = mask & DIR_UP, bot = mask & DIR_DOWN;
""" + source[physical_start:physical_end] + "\n}\n"
    preamble = r"""
#include "joyport_input.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
using uae_u8 = unsigned char;
using uae_u64 = std::uint64_t;
constexpr int MAX_JPORTS = 4, NORMAL_JPORTS = 2, MOUSE_AXIS_TOTAL = 4;
constexpr int DIR_LEFT = 1, DIR_RIGHT = 2, DIR_UP = 4, DIR_DOWN = 8;
constexpr int JPORT_NONE = -1, JSEM_JOYS = 100, JSEM_MICE = 200, JSEM_END = 300;
constexpr int JSEM_KBDLAYOUT = 0, JSEM_LASTKBD = 9, JSEM_CUSTOM = 10, MAX_JPORTS_CUSTOM = 4;
constexpr int JSEM_MODE_JOYSTICK = 3, JSEM_MODE_GAMEPAD = 4, JSEM_MODE_JOYSTICK_CD32 = 7;
constexpr int IDTYPE_JOYSTICK = 0, IDTYPE_MOUSE = 1;
constexpr int MAX_JPORT_NAME = 128, MAX_JPORT_CONFIG = 128;
struct inputdevconfig { char shortid[64]{}, name[128]{}, configname[128]{}; };
struct jport { int id = JPORT_NONE; inputdevconfig idc; int mode = 0; };
int inputdevice_get_device_total(int) { return 1; }
const char* inputdevice_get_device_name(int, int) { return "device"; }
const char* inputdevice_get_device_unique_name(int, int) { return "device-id"; }
#define _tcscpy std::strcpy
#define _tcsncpy std::strncpy
#define _sntprintf std::snprintf
#define _T(x) x
static Amiberry::Input::JoyportInput joyport_input;
static int joydir[4], joybutton[4], otop[4], obot[4], oleft[4], oright[4];
static int cd32_shifter[2], pot_cap[2][2], joydirpot[4][2], cd32_pad_enabled[2];
static int mouse_delta[4][4], mouse_deltanoreset[4][4];
static int mouse_x[4], mouse_y[4], mouse_frame_x[4], mouse_frame_y[4];
#define xmalloc(type, count) static_cast<type*>(std::malloc(sizeof(type) * (count)))
#define save_u16(v) do { unsigned value = (v); *dst++ = value >> 8; *dst++ = value; } while (0)
#define restore_u16() ([&]() { int value = (src[0] << 8) | src[1]; src += 2; return value; }())
"""
    assertions = r"""
static int decoded(int port)
{
    const int value = mouse_x[port] | (mouse_y[port] << 8);
    return ((value >> 9) & 1) | (((value >> 1) & 1) << 1)
        | ((((value >> 9) ^ (value >> 8)) & 1) << 2)
        | ((((value >> 1) ^ value) & 1) << 3);
}
int main()
{
    // Execute the actual event-handler direction merge with both values of
    // HANDLE_IE_FLAG_ALLOWOPPOSITE, in each physical/IPC source order.
    for (int port = 0; port < 4; ++port) {
        for (bool allow : {false, true}) {
            for (int host = 0; host < 16; ++host) {
                for (int injected : {0, 1, 2, 4, 8, 5, 6, 9, 10}) {
                    joyport_input.reset();
                    update_joyport_input(port);
                    physical_directions(port, host, allow);
                    const int host_only = joydir[port];
                    if (allow)
                        assert(host_only == host);
                    inputdevice_set_joystick_state(100, port, injected, 0);
                    const int physical_first = joydir[port];
                    if (port < 2)
                        assert(decoded(port) == physical_first);

                    joyport_input.reset();
                    update_joyport_input(port);
                    inputdevice_set_joystick_state(100, port, injected, 0);
                    physical_directions(port, host, allow);
                    assert(joydir[port] == physical_first);
                    if (port < 2)
                        assert(decoded(port) == physical_first);
                    if (injected) {
                        assert((physical_first & 3) != 3);
                        assert((physical_first & 12) != 12);
                    }
                    inputdevice_release_joystick(100);
                    assert(joydir[port] == host_only);
                    if (port < 2)
                        assert(decoded(port) == host_only);
                }
            }
        }
    }
    joyport_input.reset();
    for (int port = 0; port < 4; ++port)
        update_joyport_input(port);
    // Repeated preference validation must keep a deliberately host-less pad
    // matchable, rather than freeing the port and losing its explicit mode.
    for (int mode : {3, 4, 7}) {
        jport none;
        none.mode = mode;
        std::strcpy(none.idc.shortid, "none");
        for (int pass = 0; pass < 3; ++pass) {
            assert(fixjport(&none, 0, true));
            assert(none.id == JPORT_NONE && none.mode == mode);
            assert(std::strcmp(none.idc.shortid, "none") == 0);
        }
    }
    jport unconfigured;
    std::strcpy(unconfigured.idc.shortid, "none");
    assert(fixjport(&unconfigured, 0, true));
    assert(unconfigured.idc.shortid[0] == 0 && unconfigured.mode == 0);
    jport physical;
    physical.id = JSEM_JOYS;
    assert(fixjport(&physical, 0, true));
    assert(std::strcmp(physical.idc.name, "device") == 0);
    assert(std::strcmp(physical.idc.configname, "device-id") == 0);
    jport keyboard;
    keyboard.id = 1;
    assert(fixjport(&keyboard, 0, true));
    assert(std::strcmp(keyboard.idc.shortid, "kbd2") == 0);
    std::array<uae_u8, 1000> buffer{};
    size_t length = 0;
    cd32_pad_enabled[1] = 1;
    for (int port = 0; port < 4; ++port)
        inputdevice_set_joystick_state(10, port, 5, port == 1 ? 127 : 3);
    save_inputstate(&length, buffer.data());
    assert(length == 108);
    // Saving must not change currently held inputs.
    assert(joydir[0] == 5 && joybutton[0] == 3 && decoded(0) == 5);
    // Restore while the connection still exists, then release/disconnect it.
    restore_inputstate(buffer.data());
    inputdevice_release_joystick(10);
    for (int port = 0; port < 4; ++port)
        assert(joydir[port] == 0 && joybutton[port] == 0);
    assert(decoded(0) == 0 && decoded(1) == 0);
    // The saved snapshot also cannot recreate a disconnected client's hold.
    inputdevice_set_joystick_state(20, 1, 10, 127);
    inputdevice_release_joystick(20);
    restore_inputstate(buffer.data());
    assert(joydir[1] == 0 && joybutton[1] == 0 && decoded(1) == 0);

    // A real physical hold overlapping IPC remains physical after restore.
    joyport_input.set_host_directions(0, 1);
    joyport_input.set_host_button(0, 0, true);
    update_joyport_input(0);
    inputdevice_set_joystick_state(30, 0, 6, 3);
    save_inputstate(&length, buffer.data());
    assert(decoded(0) == 6);
    restore_inputstate(buffer.data());
    inputdevice_release_joystick(30);
    assert(joydir[0] == 1 && joybutton[0] == 1 && decoded(0) == 1);
    joyport_input.set_host_button(0, 0, false);
    joyport_input.set_host_directions(0, 0);
    update_joyport_input(0);
    assert(joydir[0] == 0 && joybutton[0] == 0 && decoded(0) == 0);

    // Legacy physical snapshots keep their wire values and JOYTEST counters.
    for (int port = 0; port < 4; ++port) {
        joyport_input.restore_host(port, port + 1, 1 << port);
        update_joyport_input(port);
    }
    mouse_x[0] = 0x95;
    mouse_y[0] = 0xe7;
    save_inputstate(&length, buffer.data());
    for (int port = 0; port < 4; ++port) {
        assert(buffer[port * 12] == 0 && buffer[port * 12 + 1] == port + 1);
        assert(buffer[port * 12 + 2] == 0 && buffer[port * 12 + 3] == (1 << port));
    }
    joyport_input.reset();
    restore_inputstate(buffer.data());
    inputdevice_release_joystick(999);
    for (int port = 0; port < 4; ++port)
        assert(joydir[port] == port + 1 && joybutton[port] == (1 << port));
    assert(mouse_x[0] == 0x95 && mouse_y[0] == 0xe7);
    std::cout << "Production joystick recorder lifecycle tests passed\n";
}
"""
    work = ROOT / "tests" / f".ipc-joystick-state-{os.getpid()}"
    work.mkdir()
    try:
        env = dict(os.environ, TMPDIR=str(work))
        compiler = shlex.split(os.environ.get("CXX", "c++"))

        def compile_run(body, name):
            output = work / name
            subprocess.run(
                [*compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-fsanitize=undefined", "-fno-sanitize-recover=all",
                 "-Isrc/include", "-x", "c++", "-", "-o", str(output)],
                input=preamble + body + assertions, text=True, cwd=ROOT, env=env, check=True)
            return subprocess.run([str(output)], cwd=work, capture_output=True, text=True)

        normal = compile_run(production, "state")
        assert normal.returncode == 0, (normal.returncode, normal.stdout, normal.stderr)
        print(normal.stdout.strip(), flush=True)
        mutation = production.replace(
            "joyport_input.save_host(i)",
            "Amiberry::Input::JoyportState{joydir[i], joybutton[i]}")
        assert mutation != production
        broken = compile_run(mutation, "state-mutated")
        assert broken.returncode != 0, "Restoring merged IPC holds incorrectly passed"
        assert "Assertion" in broken.stderr or "assertion" in broken.stderr, broken.stderr
        print("Merged-input serialization mutation correctly fails", flush=True)
        mutation = production.replace(
            '_tcscpy(port->idc.shortid, _T("none"));',
            "port->idc.shortid[0] = 0;")
        assert mutation != production
        broken = compile_run(mutation, "startup-mutated")
        assert broken.returncode != 0, "Losing the explicit none identity incorrectly passed"
        assert "Assertion" in broken.stderr or "assertion" in broken.stderr, broken.stderr
        print("Lost virtual-pad startup identity mutation correctly fails", flush=True)
        mutation = production.replace(
            "joydir[joy] = joyport_input.directions(joy, false);",
            "joydir[joy] = joyport_input.directions(joy, allowoppositestick != 0);")
        assert mutation != production
        broken = compile_run(mutation, "source-order-mutated")
        assert broken.returncode != 0, "Source-order-dependent direction merge incorrectly passed"
        assert "Assertion" in broken.stderr or "assertion" in broken.stderr, broken.stderr
        print("Source-order-dependent merge mutation correctly fails", flush=True)
    finally:
        shutil.rmtree(work)


if __name__ == "__main__":
    main()
