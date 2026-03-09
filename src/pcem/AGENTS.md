# src/pcem/ — PCem x86 CPU Emulation

## OVERVIEW

Vendored PCem library (193 files, 110K lines). Emulates x86 CPUs (8088 through Pentium) and PC peripherals for Amiga x86 bridgeboard emulation. Guarded by `USE_PCEM` build flag. Disabled on RISC-V.

## STRUCTURE

```
pcem/
├── [CPU Cores]
│   ├── 808x.cpp            # 8088/8086 execution
│   ├── 386.cpp             # 386+ instruction execution
│   ├── 386_dynarec.cpp     # Dynamic recompilation (JIT)
│   ├── x86_ops_*.h         # Instruction implementations (30+ files)
│   ├── x87.cpp             # FPU emulation
│   └── cpu.cpp/.h          # CPU state management
├── [Chipset/Peripherals]
│   ├── pit.cpp             # Programmable Interval Timer
│   ├── pic.cpp             # Programmable Interrupt Controller
│   ├── dma.cpp             # DMA controller
│   ├── keyboard_at*.cpp    # Keyboard controllers
│   ├── mouse*.cpp          # Mouse (PS/2, serial)
│   └── serial.cpp          # Serial ports
├── [Video]
│   ├── vid_vga.cpp         # VGA core
│   ├── vid_svga*.cpp       # SVGA variants (Cirrus, S3, etc.)
│   ├── vid_voodoo.cpp      # 3dfx Voodoo
│   └── vid_*.cpp           # Other video chipsets
├── [Sound]
│   ├── sound_opl*.cpp      # OPL FM synthesis
│   ├── sound_sb*.cpp       # Sound Blaster
│   └── sound_speaker.cpp   # PC speaker
├── [Integration]
│   ├── pcemglue.cpp        # Amiberry ↔ PCem bridge (1328 lines)
│   └── keyboard_at_draco.cpp # Draco keyboard integration
└── [Memory]
    └── mem.cpp             # x86 paging/segmentation
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Integration with Amiberry | `pcemglue.cpp` | CPU state, logging, timers, memory, I/O, sound |
| CPU emulation bugs | `386.cpp` / `808x.cpp` | Check upstream PCem first |
| Video emulation | `vid_*.cpp` | Many chipset variants |
| Build flag | Root `CMakeLists.txt` | `USE_PCEM` option |

## CONVENTIONS

- **Vendored code** — minimize modifications, document upstream delta
- Integration ONLY through `pcemglue.cpp`
- `pclog()` redirects to Amiberry logging
- Timer callbacks bridge PCem timing to Amiberry event system

## ANTI-PATTERNS

- **NEVER** modify vendored PCem files without documenting the change
- **NEVER** enable on RISC-V (auto-disabled in CMakeLists.txt)
- Keyboard type selection is incomplete (TODO at `keyboard_at_draco.cpp:979`)
