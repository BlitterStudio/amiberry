#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def between(source, start, end):
    begin = source.index(start)
    return source[begin:source.index(end, begin)]


gui = Path("src/osdep/amiberry_gui.cpp").read_text()
memory = Path("src/osdep/amiberry_mem.cpp").read_text()
jit = Path("src/jit/compemu_prefs.cpp").read_text()
gui_sync = between(gui, "static void prefs_to_gui()", "static void gui_to_prefs(")
protection = between(memory, "void protect_roms(bool protect)", "// Mark indirect regions")
jit_prefs = jit[jit.index("/* Direct access needs memory"):]

fixture = r'''
#include <cstddef>
#include <cstring>
#include <iostream>
using uae_u8 = unsigned char;
using DWORD = unsigned long;
#define _T(s) s
#define S_READ 1
#define S_WRITE 2
#define S_N_ADDR 4
#define MAX_SHMID 1
#define PAGE_READONLY 2
#define PAGE_READWRITE 4
struct uae_prefs {
    int cachesize = 0;
    int comptrustbyte = 1, comptrustword = 1, comptrustlong = 1, comptrustnaddr = 1;
    int compnf = 0, comp_hardflush = 0, comp_constjump = 0, compfpu = 0;
    int fpu_strict = 0, illegal_mem = 0, chipset = 0, mountitems = 0;
    int mountconfig[1]{};
    bool rom_readwrite = false;
} currprefs, changed_prefs;
struct uae_shmid_ds {
    int mode = PAGE_READONLY;
    void* attached = this;
    size_t rosize = 4096;
    int maprom = 0;
} shmids[MAX_SHMID];
static bool canbang, rom_write_enabled, rom_protected;
static int special_mem_default, protection_calls;
static bool jit_direct_pending(const uae_prefs*) { return false; }
static void write_log(const char*, ...) {}
static void alloc_cache() {}
static size_t shmid_protect_size(int) { return 4096; }
static int GetLastError() { return 0; }
static bool VirtualProtect(void*, size_t, DWORD mode, DWORD*) {
    ++protection_calls;
    rom_protected = mode == PAGE_READONLY;
    return true;
}
static void load_gui_theme() {}
static int get_reset_only_chipset_prefs(const uae_prefs& p) { return p.chipset; }
static void set_reset_only_chipset_prefs(uae_prefs& p, int v) { p.chipset = v; }
static void copy_prefs(const uae_prefs* from, uae_prefs* to) { *to = *from; }
''' + protection + jit_prefs + gui_sync + r'''
int main()
{
    int failures = 0;
    auto expect = [&](bool ok, const char* message) {
        if (!ok) { std::cerr << message << '\n'; ++failures; }
    };
    auto select_direct = []() {
        changed_prefs.comptrustbyte = changed_prefs.comptrustword =
            changed_prefs.comptrustlong = changed_prefs.comptrustnaddr = 0;
    };
    auto desired_direct = []() {
        return !changed_prefs.comptrustbyte && !changed_prefs.comptrustword &&
            !changed_prefs.comptrustlong && !changed_prefs.comptrustnaddr;
    };

    currprefs.cachesize = changed_prefs.cachesize = 4096;
    select_direct();
    check_prefs_changed_comp(false);
    expect(currprefs.comptrustbyte == 1 && desired_direct(),
        "Direct must remain pending while the memory layout is indirect");
    prefs_to_gui();
    expect(desired_direct(), "Reopening settings must retain pending Direct access");

    // A reset has now made the memory layout direct-compatible.
    select_direct();
    canbang = true;
    expect(check_prefs_changed_comp(true), "Direct activation must require a CPU prefs update");
    expect(currprefs.comptrustbyte == 1 && protection_calls == 0,
        "Checking preferences must not activate Direct or change protection");
    check_prefs_changed_comp(false);
    expect(!currprefs.comptrustbyte && rom_protected && protection_calls == 1,
        "Activating pending Direct must protect ROM before native stores can run");
    check_prefs_changed_comp(false);
    expect(protection_calls == 1, "Unchanged preferences must not reprotect ROM repeatedly");

    changed_prefs.cachesize = 0;
    check_prefs_changed_comp(false);
    prefs_to_gui();
    expect(desired_direct(), "Reopening settings with the JIT off must retain desired Direct");
    changed_prefs.cachesize = 4096;
    rom_protected = false;
    check_prefs_changed_comp(false);
    expect(!currprefs.comptrustbyte && rom_protected,
        "Re-enabling Direct JIT must protect ROM");

    changed_prefs.comptrustbyte = 1;
    changed_prefs.comptrustword = 0;
    changed_prefs.comptrustlong = 1;
    changed_prefs.comptrustnaddr = 0;
    canbang = false;
    check_prefs_changed_comp(false);
    prefs_to_gui();
    expect(changed_prefs.comptrustbyte == 1 && changed_prefs.comptrustword == 0 &&
        changed_prefs.comptrustlong == 1 && changed_prefs.comptrustnaddr == 0,
        "GUI synchronization must retain each independently configured access mode");

    canbang = true;
    select_direct();
    currprefs.rom_readwrite = changed_prefs.rom_readwrite = true;
    rom_protected = false;
    check_prefs_changed_comp(false);
    expect(!rom_protected, "Direct activation must respect deliberately writable ROM");
    return failures ? 1 : 0;
}
'''

with tempfile.TemporaryDirectory(prefix="amiberry-jit-prefs-") as tmp:
    cpp = Path(tmp) / "test.cpp"
    exe = Path(tmp) / ("test.exe" if os.name == "nt" else "test")
    cpp.write_text(fixture)
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) +
                   ["-std=c++17", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)],
                   check=True)
    subprocess.run([str(exe)], check=True)
PY
