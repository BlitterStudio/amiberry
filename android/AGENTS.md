# android/ — Android App

## OVERVIEW

Kotlin/Jetpack Compose front-end launching native SDL emulator in a separate `:sdl` process. No JNI runtime control — communication is CLI-style via `Intent.putExtra("SDL_ARGS", String[])`. Version derived from root `CMakeLists.txt`.

## STRUCTURE

```
android/app/src/main/java/com/blitterstudio/amiberry/
├── [Activities]
│   ├── ui/MainActivity.kt          # Compose entry: asset extraction + theme
│   └── AmiberryActivity.java       # SDL Activity in :sdl process
│
├── [Navigation]
│   ├── ui/AmiberryApp.kt           # Root Compose: responsive nav (rail/bar)
│   └── ui/navigation/Screen.kt    # 4 routes: QuickStart, Settings, Files, Configs
│
├── [ViewModels]
│   ├── ui/viewmodel/QuickStartViewModel.kt     # Media file scanning
│   ├── ui/viewmodel/SettingsViewModel.kt       # ROM detection + hardware constraints
│   ├── ui/viewmodel/ConfigurationsViewModel.kt # Config file CRUD
│   └── ui/viewmodel/FileManagerViewModel.kt    # SAF import + file scanning
│
├── [Screens]
│   ├── ui/screens/QuickStartScreen.kt          # Model picker + media + launch
│   ├── ui/screens/ConfigurationsScreen.kt      # Config list management
│   ├── ui/screens/FileManagerScreen.kt         # File browser + import
│   └── ui/screens/settings/
│       ├── SettingsScreen.kt        # 8-tab settings container
│       ├── CpuTab.kt               # CPU/FPU/JIT settings
│       ├── ChipsetTab.kt           # Chipset config
│       ├── MemoryTab.kt            # RAM configuration
│       ├── SoundTab.kt             # Audio settings
│       ├── DisplayTab.kt           # Display options
│       ├── InputTab.kt             # Joystick/keyboard config
│       └── StorageTab.kt           # ROM/media paths
│
├── [Data Layer]
│   ├── data/EmulatorLauncher.kt    # Static methods to launch SDL with CLI args
│   ├── data/ConfigRepository.kt    # .uae file I/O singleton
│   ├── data/ConfigGenerator.kt     # Generates .uae from EmulatorSettings
│   ├── data/ConfigParser.kt        # Parses .uae, preserves unknown keys
│   ├── data/FileRepository.kt      # StateFlow of scanned files by category
│   └── data/FileManager.kt         # File scanning, CRC32, SAF import
│
└── [Models]
    ├── data/model/EmulatorSettings.kt  # All configurable params
    ├── data/model/AmigaModel.kt        # 10 Amiga models enum
    └── data/model/AmigaFile.kt         # File metadata + categories
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Launch emulator | `EmulatorLauncher.kt` | 4 launch patterns: QuickStart, Config, WHDLoad, AdvancedGUI |
| Add config option | `EmulatorSettings.kt` + `ConfigGenerator.kt` + `ConfigParser.kt` | Must match `cfgfile.cpp` key names |
| ROM detection | `SettingsViewModel.kt` | CRC32 lookup → ROM ID → model profile |
| Hardware constraints | `SettingsViewModel.applyConstraints()` | 68000→24bit, cycle_exact→real speed, JIT→68020+ |
| Asset extraction | `MainActivity.kt` | Cached by `.extracted_version` marker |
| Add new screen | `Screen.kt` + `AmiberryApp.kt` | Sealed class + NavHost route |

## CONVENTIONS

- **Version single source of truth**: Root `CMakeLists.txt` `project(VERSION)` → Gradle parses at build
- **versionCode formula**: `major*1M + minor*10K + patch*100 + slot` (slot 99 = final release)
- **Config round-trip preservation**: Unknown .uae keys survive Kotlin parse→save cycle
- **Dual-process**: UI in main process, SDL in `:sdl` process (killed on finish)
- **Responsive nav**: NavigationRail (landscape) / NavigationBar (portrait)
- **ROM profiles**: `MODEL_ROM_PROFILE` maps model → prioritized ROM ID list (must match C++ `bip_*`)

## ANTI-PATTERNS

- **NEVER** add JNI calls — use Intent CLI args for native communication
- **NEVER** modify `MODEL_ROM_PROFILE` without updating C++ `bip_*()` functions
- **DO NOT** skip `--rescan-roms` in launch args — ensures newly added ROMs detected
- **DO NOT** add `use_gui=yes` with `-G` flag — they conflict
