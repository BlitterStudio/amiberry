#!/usr/bin/env python3
"""Exercise RTG reset sequencing and allocation with production code."""
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
    expansion = (ROOT / "src/expansion.cpp").read_text(encoding="utf-8")
    memory = (ROOT / "src/memory.cpp").read_text(encoding="utf-8")
    host_memory = (ROOT / "src/osdep/amiberry_mem.cpp").read_text(encoding="utf-8")
    options = (ROOT / "src/include/options.h").read_text(encoding="utf-8")
    graphics = (ROOT / "src/include/gfxboard.h").read_text(encoding="utf-8")
    gfxboard = (ROOT / "src/gfxboard.cpp").read_text(encoding="utf-8")
    board_struct = between(gfxboard, 'struct gfxboard\n', '#define ISP4()')
    a2410 = between(gfxboard, 'GFXBOARD_ID_A2410,', '\n\t},')
    allocation = between(expansion, "static void allocate_expamem (void)",
                         "static uaecptr check_boot_rom (")
    reset_request = between(memory, "void memory_hardreset (int mode)",
                            "// do not map if it conflicts with custom banks")
    shared_memory_init = between(host_memory, "static uae_u32 oz3fastmem_size",
                                 "void free_shm ()")
    reset_clear = between(memory, "\tif (mem_hardreset) {\n\t\tmemory_clear ();",
                          "#ifdef NATMEM_OFFSET")
    board_types = between(options, "#define MAX_RTG_BOARDS", "struct expansion_params")
    board_ids = between(graphics, "#define GFXBOARD_UAE_Z2", "#define GFXBOARD_BUSTYPE_Z")
    fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
using uae_u8 = uint8_t;
using uae_u16 = uint16_t;
using uae_u32 = uint32_t;
using uaecptr = uint32_t;
using TCHAR = char;
#define MAX_DPATH 1024
#define _T(s) s
#define PICASSO96
constexpr uaecptr Z3BASE_UAE = 0x40000000;
''' + board_types + board_ids + r'''
struct gfxboard_func {} a2410_func;
''' + board_struct + 'const gfxboard a2410_board = {' + a2410 + r'''};
const uae_u32 a2410_min = a2410_board.vrammin, a2410_max = a2410_board.vrammax;
struct addrbank {
    uae_u32 reserved_size = 0, allocated_size = 0, mask = 0, start = 0;
    uae_u8* baseaddr = nullptr;
};
struct uae_prefs {
    rtgboardconfig rtgboards[MAX_RTG_BOARDS]{};
    romboard romboards[MAX_ROM_BOARDS]{};
    ramboard fastmem[MAX_RAM_BOARDS]{}, z3fastmem[MAX_RAM_BOARDS]{};
    ramboard z3chipmem{}, mbresmem_high{};
} currprefs, changed_prefs;
addrbank romboardmem_bank[MAX_ROM_BOARDS], fastmem_bank[MAX_RAM_BOARDS];
addrbank z3fastmem_bank[MAX_RAM_BOARDS], z3chipmem_bank, graphics_bank;
addrbank* gfxmem_banks[MAX_RTG_BOARDS] = { &graphics_bank };
static int mem_hardreset;

// Host allocation is isolated; production init_shm (including its persistent
// configuration tracking), reset request and allocation make reset decisions.
int doinit_shm() { return 0; }
void resetmem(bool) {}
void clear_shm() {}
uae_u8 guest_ram_marker;
void memory_clear() {
    mem_hardreset = 0;
    guest_ram_marker = 0;
}
void memory_reset() {
''' + reset_clear + r'''
}
void mapped_free(addrbank* bank) {
    std::free(bank->baseaddr);
    *bank = {};
}
void mapped_malloc(addrbank* bank) {
    bank->baseaddr = static_cast<uae_u8*>(std::calloc(bank->reserved_size, 1));
    assert(!bank->reserved_size || bank->baseaddr);
    bank->allocated_size = bank->reserved_size;
}
void mapped_malloc_dynamic(uae_u32* size, uae_u32*, addrbank* bank, int, const TCHAR*) {
    bank->reserved_size = *size;
    mapped_malloc(bank);
}
void free_fastmemory(int index) { mapped_free(&fastmem_bank[index]); }
void write_log(const TCHAR*, ...) {}
uaecptr expansion_startaddress(uae_prefs*, uaecptr, uae_u32) {
    std::abort(); // Manual RAM mapping is outside these RTG scenarios.
}
''' + reset_request + shared_memory_init + allocation + r'''
int main() {
    constexpr uae_u32 MiB = 1024 * 1024;
    int failures = 0;
    auto expect = [&](bool ok, const char* message) {
        if (!ok) { std::cerr << message << '\n'; ++failures; }
    };
    auto& card = changed_prefs.rtgboards[0];

    // devices_reset calls init_shm before memory_reset; custom reset then
    // calls expamem_reset, which runs allocate_expamem. Keep that order and
    // let memory_reset consume requests instead of manually clearing them.
    auto reset = [&]() {
        expect(init_shm(), "Shared memory initialization must succeed");
        memory_reset();
        allocate_expamem();
    };
    auto check_configuration_change = [&]() {
        guest_ram_marker = 0x5a;
        expect(init_shm(), "Changed RTG configuration must initialize");
        expect(mem_hardreset != 0, "init_shm must request the configuration hard reset");
        memory_reset();
        expect(mem_hardreset == 0 && guest_ram_marker == 0,
               "Configuration hard reset must clear RAM and consume the request");
        allocate_expamem();
        expect(mem_hardreset == 0, "Expansion allocation must not re-arm the consumed reset");
        guest_ram_marker = 0x5a;
        reset();
        expect(mem_hardreset == 0 && guest_ram_marker == 0x5a,
               "Next unchanged reset must preserve the guest RAM marker");
    };

    // ZZ9000's private VRAM leaves the generic graphics bank unallocated.
    card.rtgmem_type = GFXBOARD_ID_ZZ9000_Z3;
    card.rtgmem_size = 128 * MiB;
    check_configuration_change();
    expect(mem_hardreset == 0, "ZZ9000 warm reset must not schedule guest RAM destruction");

    // A hardware card must not cancel a hard reset requested elsewhere.
    memory_hardreset(2);
    const int pending_reset = mem_hardreset;
    allocate_expamem();
    expect(mem_hardreset == pending_reset, "Hardware RTG must preserve an existing hard-reset request");

    memory_reset(); // Consume the independent request before changing cards.
    card.rtgmem_type = GFXBOARD_ID_ZZ9000_Z2;
    card.rtgmem_size = 4 * MiB;
    check_configuration_change();
    card.rtgmem_size = 8 * MiB; // Size-only change, with the same private-memory card.
    check_configuration_change();
    card.rtgmem_type = GFXBOARD_ID_ZZ9000_Z3;
    check_configuration_change(); // Type-only change, with the same VRAM size.

    // These framebuffer boards also keep VRAM outside the generic bank.
    const int framebuffer_cards[] = { GFXBOARD_ID_HARLEQUIN, GFXBOARD_ID_RAINBOWII };
    for (const int board : framebuffer_cards) {
        card.rtgmem_type = board;
        card.rtgmem_size = 2 * MiB;
        check_configuration_change(); // Also covers a type-only switch between these cards.
        expect(graphics_bank.reserved_size == 0 && graphics_bank.baseaddr == nullptr,
               "Framebuffer VRAM must leave the generic bank unallocated");
        for (int i = 0; i < 3; ++i) {
            guest_ram_marker = 0x5a;
            reset();
            expect(mem_hardreset == 0 && guest_ram_marker == 0x5a,
                   "Repeated framebuffer warm resets must preserve guest RAM");
        }
        card.rtgmem_size = 0; // Disable and re-enable private VRAM without changing type.
        check_configuration_change();
        card.rtgmem_size = 2 * MiB;
        check_configuration_change();
    }

    // A2410's separate program/overlay RAM is not configurable VRAM.
    mem_hardreset = 0;
    card.rtgmem_type = GFXBOARD_ID_A2410;
    expect(a2410_min == MiB && a2410_max == MiB, "A2410 must allow exactly 1 MiB VRAM");
    card.rtgmem_size = a2410_max;
    currprefs.rtgboards[0] = card;
    graphics_bank.reserved_size = MiB;
    mapped_malloc(&graphics_bank);
    graphics_bank.baseaddr[0] = 0x5a;
    allocate_expamem();
    expect(mem_hardreset == 0, "A2410 split VRAM must not turn a warm reset into a hard reset");
    expect(graphics_bank.baseaddr && graphics_bank.baseaddr[0] == 0x5a,
           "A2410 warm reset must preserve its VRAM allocation");

    card.rtgmem_type = GFXBOARD_ID_PICASSO2;
    card.rtgmem_size = 2 * MiB;
    currprefs.rtgboards[0] = card;
    mem_hardreset = 0;
    allocate_expamem();
    expect(mem_hardreset != 0, "Hardware VRAM allocation mismatch must request a hard reset");

    // A real UAE graphics resize must still invalidate the old memory layout.
    mem_hardreset = 0;
    card.rtgmem_type = GFXBOARD_UAE_Z3;
    card.rtgmem_size = 4 * MiB;
    allocate_expamem();
    expect(mem_hardreset != 0, "UAE graphics allocation must request a memory reset");
    mem_hardreset = 0; // The requested reset has been consumed.
    graphics_bank.baseaddr[0] = 0x5a;
    allocate_expamem();
    expect(mem_hardreset == 0 && graphics_bank.baseaddr[0] == 0x5a,
           "Unchanged UAE graphics must preserve its memory on warm reset");
    card.rtgmem_size = 8 * MiB;
    allocate_expamem();
    expect(mem_hardreset != 0, "UAE graphics resizing must still request a memory reset");
    mapped_free(&graphics_bank);
    if (!failures) std::cout << "RTG warm-reset regression: passed\n";
    return failures ? 1 : 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="amiberry-rtg-reset-") as tmp:
        cpp = Path(tmp) / "test.cpp"
        exe = Path(tmp) / ("test.exe" if os.name == "nt" else "test")
        cpp.write_text(fixture, encoding="utf-8")
        subprocess.run(shlex.split(os.environ.get("CXX", "c++"))
                       + ["-std=c++17", str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
