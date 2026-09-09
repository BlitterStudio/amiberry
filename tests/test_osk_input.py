#!/usr/bin/env python3
"""Compile the production SDL-to-UAE OSK routing with deterministic device sinks.

Run from any directory. CXX selects the compiler; CXXFLAGS supplies platform
flags (for example -static with llvm-mingw). No emulator assets are required.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def between(text, start, end):
    begin = text.index(start)
    return text[begin:text.index(end, begin)]


def main():
    native = (ROOT / "src/osdep/amiberry.cpp").read_text(encoding="utf-8")
    mapping = (ROOT / "src/osdep/amiberry_input.cpp").read_text(encoding="utf-8")
    core = (ROOT / "src/inputdevice.cpp").read_text(encoding="utf-8")
    header = (ROOT / "src/osdep/amiberry_input.h").read_text(encoding="utf-8")
    input_header = (ROOT / "src/include/inputdevice.h").read_text(encoding="utf-8")
    button = between(core, "\t\tif (ie->type & 4) {", "\t\t} else if (ie->type & 8)") + "\n}"
    direction = between(core, "\t\t\tint left = oleft[joy]", "\n\t\t}\n\t\tbreak;")
    mouse = between(core, "\t\t\t/* real mouse / analog stick mouse emulation */", "\n\t\t\tmax = 32;")
    production = between(core, "// Generic joystick mappings", "static int isdevice")
    production += "\nvoid core_button(int joy, int data, int state) { Event e{4, data}; auto ie = &e;\n" + button + "\n}\n"
    production += "\nvoid core_direction(int joy, int data, int state) { int max=32767; bool allowoppositestick=false; Event e{16, data}; auto ie=&e;\n" + direction + "\n}\n"
    production += "\nvoid core_mouse(int joy, int data, int state, int max) { Event e{8, data}; auto ie=&e;\n" + mouse + "\n}\n"
    production += between(core, "void inputdevice_add_inputcode (", "static bool keyboardresetkeys")
    production += between(mapping, "int find_in_array(", "void fill_default_controller(")
    production += between(mapping, "void sync_controller_shortcuts(", "void setup_mapping(")
    production += between(mapping, "static bool invert_axis(", "static void read_joystick()")
    production += between(native, "struct OskControllerState", "static int normalize_host_key_scancode")
    with tempfile.TemporaryDirectory(prefix="amiberry-osk-input-") as temp:
        build = Path(temp)
        (build / "osk_mapping_under_test.inc").write_text(
            between(header, "struct controller_mapping", "struct didata")
            + between(input_header, "class inputdevice_osk_passthrough", "#define INTERNALEVENT_CPURESET"), encoding="utf-8")
        (build / "osk_native_under_test.inc").write_text(production, encoding="utf-8")
        compiler = os.environ.get("CXX", "c++")
        flags = shlex.split(os.environ.get("CXXFLAGS", ""))
        for android in (False, True):
            name = "android" if android else "standalone"
            binary = build / (name + (".exe" if os.name == "nt" else ""))
            command = [compiler, "-std=c++17", "-Wall", "-Wextra", *flags,
                       "-I" + str(build), "-I" + str(ROOT / "libretro"),
                       "-I" + str(ROOT / "src/osdep"), str(ROOT / "tests/osk_input_test.cpp"),
                       "-o", str(binary)]
            if android:
                command.insert(1, "-D__ANDROID__")
            subprocess.run(command, check=True)
            subprocess.run([str(binary)], check=True)
            print(f"OSK input routing ({name}): passed", flush=True)


if __name__ == "__main__":
    main()
