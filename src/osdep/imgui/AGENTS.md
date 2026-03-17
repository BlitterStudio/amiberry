# src/osdep/imgui/ — ImGui GUI Panels

## OVERVIEW

30+ modular Dear ImGui panels for Amiga hardware configuration. Each file = one self-contained panel. Panels are stateless — they read/write `currprefs`/`changed_prefs` directly.

## STRUCTURE

```
imgui/
├── imgui_panels.h          # Panel declarations + IMGUI_PANEL_LIST macro + helper widgets
├── imgui_help_text.h       # Tooltip help text constants
├── [Hardware Configuration Panels]
│   ├── quickstart.cpp      # Quick-start wizard (model selection)
│   ├── cpu.cpp             # CPU model, FPU, JIT cache
│   ├── chipset.cpp         # Chipset type (OCS/ECS/AGA)
│   ├── adv_chipset.cpp     # Advanced chipset settings
│   ├── rom.cpp             # ROM selection
│   ├── ram.cpp             # Memory configuration
│   ├── floppy.cpp          # Floppy drive config
│   ├── hd.cpp              # Hard drives / CD-ROM
│   ├── expansions.cpp      # Expansion boards
│   ├── rtg.cpp             # RTG (Picasso96) board
│   └── hwinfo.cpp          # Hardware info display
├── [Output Configuration Panels]
│   ├── display.cpp         # Display settings
│   ├── filter.cpp          # Shader filter panel (accesses OpenGLRenderer!)
│   ├── sound.cpp           # Sound configuration
│   └── themes.cpp          # Theme selection
├── [Input/IO Panels]
│   ├── input.cpp           # Input device config
│   ├── io.cpp              # IO ports
│   ├── custom.cpp          # Custom controls
│   └── controller_map.cpp  # Controller mapping modal
├── [Utility Panels]
│   ├── configurations.cpp  # Config file management
│   ├── paths.cpp           # Path configuration
│   ├── diskswapper.cpp     # Disk swapper
│   ├── misc.cpp            # Miscellaneous settings
│   ├── prio.cpp            # Priority/threading
│   ├── savestates.cpp      # Save state management
│   ├── virtualkeyboard.cpp # Virtual keyboard
│   ├── whdload.cpp         # WHDLoad settings
│   └── about.cpp           # About dialog
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add new panel | Create `new_panel.cpp`, add to `IMGUI_PANEL_LIST` in `imgui_panels.h` | Also add to `cmake/SourceFiles.cmake` |
| Add widget helpers | `imgui_panels.h` | `AmigaButton()`, `AmigaCheckbox()`, `BeginGroupBox()`, etc. |
| Shader params UI | `filter.cpp` | Only panel that accesses renderer internals |
| File dialogs | `file_dialog.h` | `OpenFileDialogKey()` / `ConsumeFileDialogResultKey()` — dispatches to native OS dialog (NFD) or ImGuiFileDialog fallback |

## CONVENTIONS

- **One panel per file** — `render_panel_<name>()` function signature
- **Stateless panels** — all state in `currprefs`/`changed_prefs` globals
- **Amiga-styled widgets** — `AmigaButton()`, `AmigaCheckbox()`, `AmigaBevel()` for consistent look
- **Panel registration** via `IMGUI_PANEL_LIST` macro with icon and display name
- **Textures** via `gui_create_texture()` / `gui_destroy_texture()` — abstracts SDL_Texture vs GLuint

## ANTI-PATTERNS

- **DO NOT** store state in panel code — use `currprefs`/`changed_prefs`
- **DO NOT** access renderer from panels — except `filter.cpp` (known exception)
- **DO NOT** forget to add new files to `cmake/SourceFiles.cmake`
