#!/usr/bin/env python3
"""Exercise RP9 directory registration and command-line pre-scanning on real files."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    rp9 = (ROOT / "src/osdep/amiberry_rp9.cpp").read_text(encoding="utf-8")
    begin = rp9.index("int rp9_register_rom_directory(")
    scanner = rp9[begin:rp9.index("bool rp9_parse_file(", begin)]
    source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
    begin = source.index("static void register_cmdline_rp9_rom_sources(")
    cmdline = source[begin:source.index("\n#endif", begin)]
    fixture = r'''
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif
#define _T(x) x
#define TCHAR char
#define TARGET_NAME "amiberry"
#define _tcsicmp strcasecmp
#define _tcsnicmp strncasecmp
#define xfree free
constexpr std::uintmax_t maximum_scanned_rom_size = 10000000;
std::vector<std::filesystem::path> scanned, keys;
std::vector<std::string> events;
void write_log(const char*, ...) {}
char* parsetextpath(const char* value) {
    std::string path(value);
    if (!path.empty() && (path.front() == '"' || path.front() == '\''))
        path = path.substr(1, path.size() - 2);
    auto* result = static_cast<char*>(malloc(path.size() + 1));
    std::memcpy(result, path.c_str(), path.size() + 1);
    return result;
}
void addkeyfile(const char* path) { keys.emplace_back(path); events.emplace_back("key"); }
bool rp9_register_rom_override(const char* path) {
    // Read candidates, including rejected files: these are the expensive probes
    // that a ROM registry cache cannot prevent for unrelated BIOS-pack files.
    std::ifstream file(path);
    std::string data;
    std::getline(file, data);
    scanned.emplace_back(path);
    events.emplace_back("rom");
    return data == "rom";
}
'''
    checks = r'''
int main(int argc, char** argv) {
    assert(argc == 2);
    namespace fs = std::filesystem;
    const fs::path root(argv[1]);
    const auto system = root / "bios";
    const auto dedicated = system / "Kickstarts";
    const auto other = root / "other";
    fs::create_directories(system / "mame/plugins/cheat");
    fs::create_directories(dedicated / "nested");
    fs::create_directories(dedicated / ".hidden");
    fs::create_directories(other);
    auto put = [](const fs::path& path, const char* data) { std::ofstream(path) << data; };
    put(system / "kick.rom", "rom");
    put(system / "mame/plugins/cheat/init.lua", "not a ROM");
    put(dedicated / "nested/custom-firmware", "rom");
    put(dedicated / "nested/ROM.KEY", "key");
    put(dedicated / ".hidden/ignored.rom", "rom");
    put(other / "custom.rom", "rom");
    put(dedicated / "oversized.rom", "rom");
    fs::resize_file(dedicated / "oversized.rom", maximum_scanned_rom_size);
#ifdef _WIN32
    _putenv_s("AMIBERRY_LIBRETRO_SYSTEM_DIR", system.string().c_str());
#else
    setenv("AMIBERRY_LIBRETRO_SYSTEM_DIR", system.c_str(), 1);
#endif
#ifdef LIBRETRO
    // The normal libretro scanner owns this root, including aliased spellings.
    assert(rp9_register_rom_directory(system.string().c_str()) == 0);
    assert(rp9_register_rom_directory((system / ".").string().c_str()) == 0);
    assert(scanned.empty() && keys.empty());
#ifndef _WIN32
    fs::create_directory_symlink(system, root / "bios-link");
    assert(rp9_register_rom_directory((root / "bios-link").c_str()) == 0);
    assert(scanned.empty());
#endif
#else
    assert(rp9_register_rom_directory(system.string().c_str()) == 2);
    assert(std::find(scanned.begin(), scanned.end(),
        system / "mame/plugins/cheat/init.lua") != scanned.end());
#endif
    scanned.clear(); keys.clear(); events.clear();
    assert(rp9_register_rom_directory(dedicated.string().c_str()) == 1);
    assert(scanned == std::vector<fs::path>{dedicated / "nested/custom-firmware"});
    assert(keys == std::vector<fs::path>{dedicated / "nested/ROM.KEY"});
    assert(events == (std::vector<std::string>{"key", "rom"}));

    scanned.clear(); keys.clear(); events.clear();
    std::vector<std::string> args = {"amiberry", "-s", "amiberry.rom_path=" + dedicated.string(),
        "--autoload", "game.lha", "-srom_path=\"" + dedicated.string() + "\"",
        "-s", "amiberry.rom_path=" + other.string()};
    std::vector<char*> raw;
    for (auto& arg : args) raw.push_back(arg.data());
    register_cmdline_rp9_rom_sources(static_cast<int>(raw.size()), raw.data());
    const std::vector<fs::path> expected = {dedicated / "nested/custom-firmware", other / "custom.rom"};
    assert(scanned == expected);
    // Deduplication must not survive a content reload.
    scanned.clear();
    register_cmdline_rp9_rom_sources(static_cast<int>(raw.size()), raw.data());
    assert(scanned == expected);

    // Explicit overrides in the shared root remain eligible for registration.
    scanned.clear();
    args = {"amiberry", "-r", (system / "kick.rom").string(),
        "-K" + (other / "custom.rom").string()};
    raw.clear();
    for (auto& arg : args) raw.push_back(arg.data());
    register_cmdline_rp9_rom_sources(static_cast<int>(raw.size()), raw.data());
    assert(scanned == (std::vector<fs::path>{system / "kick.rom", other / "custom.rom"}));
    assert(rp9_register_rom_directory(nullptr) == 0);
    assert(rp9_register_rom_directory("") == 0);
    assert(rp9_register_rom_directory((root / "missing").string().c_str()) == 0);
    std::cout << "RP9 scan policy and per-command-line deduplication: passed\n";
}
'''
    with tempfile.TemporaryDirectory(prefix="amiberry-rp9-rom-scan-") as temp:
        build = Path(temp)
        cpp = build / "rp9_rom_scan.cpp"
        cpp.write_text(fixture + scanner + cmdline + checks, encoding="utf-8")
        for mode, flags in (("libretro", ["-DLIBRETRO"]), ("standalone", [])):
            binary = build / (mode + (".exe" if os.name == "nt" else ""))
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra",
                            *shlex.split(os.environ.get("CXXFLAGS", "")), *flags,
                            str(cpp), "-o", str(binary)], check=True)
            subprocess.run([str(binary), str(build / (mode + "-fixtures"))], check=True)
            print(f"{mode}: passed")


if __name__ == "__main__":
    main()
