# src/include/ — Core Header API Contracts

## OVERVIEW

119 header files defining the emulator's architecture: data structures, memory bank interfaces, custom chip registers, CPU operation tables, and configuration options. These are the API contracts between all subsystems.

## KEY HEADERS

| Header | Lines | Purpose |
|--------|-------|---------|
| `options.h` | 1396 | **Master config struct** `uae_prefs` — ALL emulator settings |
| `memory.h` | 922 | **Memory bank system** — `addrbank`, autoconfig, address translation |
| `newcpu.h` | 929 | **CPU interface** — opcode tables, execution structs, JIT integration |
| `custom.h` | 347 | **Custom chip registers** — Agnus/Denise/Paula, DMACON, INTENA |
| `events.h` | 190 | **Event scheduling** — cycle-accurate timing, priority levels |
| `cpummu.h` | 1029 | **68030/040/060 MMU** emulation |
| `cpummu030.h` | 1029 | **68030-specific MMU** |
| `drawing.h` | — | Graphics rendering interfaces |
| `blit.h` | — | Blitter operation definitions |
| `inputdevice.h` | — | Input device abstractions |
| `savestate.h` | — | Save state serialization interface |
| `autoconf.h` | — | Autoconfig (Zorro) system |

## CRITICAL DATA STRUCTURES

**`uae_prefs`** (options.h:612+) — Master configuration:
- Chipset config (OCS/ECS/AGA, Agnus/Denise models)
- CPU settings (68000-68060, FPU, MMU, JIT cache size)
- Memory config (chip, fast, Z3, custom)
- Amiberry additions: `amiberry_gui_theme`, `amiberry_hotkey`, `rtg_zerocopy`

**`addrbank`** (memory.h:114-155) — Memory bank interface:
- Read/write function pointers: `lget`, `wget`, `bget`, `lput`, `wput`, `bput`
- Address translation: `xlateaddr`, `baseaddr`, `baseaddr_direct_r/w`
- JIT compatibility flags
- Sub-bank support for complex memory layouts

**`autoconfig_info`** (memory.h:167-201) — Zorro expansion:
- Autoconfig ROM data, memory mapping, device callbacks

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add config option | `options.h` (struct) + `cfgfile.cpp` (parser) | Also add GUI in `osdep/imgui/` |
| Add memory bank | `memory.h` (`addrbank` struct) | Implement function pointers |
| Custom chip register | `custom.h` | Register definitions + `custom.cpp` impl |
| CPU operation | `newcpu.h` | `cputbl[]` opcode dispatch table |
| Event handling | `events.h` | `eventtab[]`, priority: sync > cia > misc > audio |

## SUBDIRECTORY

```
include/
└── uae/                   # UAE compatibility headers (24 files)
    ├── types.h             # uae_u8/u16/u32/s8/s16/s32 type definitions
    ├── string.h            # UAE string utilities
    └── [platform abstractions]
```

## CONVENTIONS

- Headers match WinUAE upstream — `#ifdef AMIBERRY` for customizations
- `uae/` subdirectory provides POSIX-compatible UAE type abstractions
- Two-state preference pattern: `currprefs` (active) / `changed_prefs` (pending)
