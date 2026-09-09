#!/usr/bin/env python3
"""Exercise the production ROM scanner's acceptance rules without emulator assets."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = (ROOT / "src/osdep/amiberry_gui.cpp").read_text(encoding="utf-8")
    begin = source.index("static int isromext(")
    scanner = source[begin:source.index("static bool scan_rom_hook(", begin)]
    fixture = r'''
#include <cassert>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif
const char* uae_archive_extensions[] = {"zip", nullptr};
'''
    checks = r'''
int main() {
    assert(isromext("Kickstart.A1000", false));
    assert(isromext("Kickstart.CdTv", false));
    assert(isromext("Kickstart.U1", false));
    assert(!isromext("Kickstart.zip", false));
    assert(isromext("Kickstart.zip", true));
    assert(!isromext("Kickstart", true));
    assert(!isromext("notes.txt", true));
}
'''
    with tempfile.TemporaryDirectory(prefix="amiberry-rom-extensions-") as temp:
        build = Path(temp)
        cpp = build / "rom_extensions.cpp"
        cpp.write_text(fixture + scanner + checks, encoding="utf-8")
        binary = build / ("rom_extensions.exe" if os.name == "nt" else "rom_extensions")
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra",
                        *shlex.split(os.environ.get("CXXFLAGS", "")), str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print("Native ROM extension acceptance: passed")


if __name__ == "__main__":
    main()
