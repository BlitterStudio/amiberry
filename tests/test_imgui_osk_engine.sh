#!/bin/sh
set -eu

# Real-engine regression for the on-screen keyboard (src/osdep/imgui_osk.cpp).
#
# Compiles the untouched production translation unit — copied into a scratch
# directory so its "sysconfig.h"/"sysdeps.h" quote-includes resolve to
# minimal platform shims instead of the full UAE platform layer — against the
# real external/imgui and the real src/include headers, then links it with
# tests/imgui_osk_engine_test.cpp, which replaces only the platform/keyboard
# sink boundaries (SDL time source, overlay font accessors, guest keyboard,
# and the input-layer osk_control()/osk_clear_controller_holds() entry
# points). The engine under test is the actual production source, refreshed
# from the working tree on every run.
#
# Requirements: a C++17 compiler (CXX, default c++) — Linux, macOS or MSYS2.
# No SDL3 development files are needed: the SDL3 header is shimmed down to
# the monotonic clock the OSK engine actually consumes.

cxx=${CXX:-c++}
root=$(cd "$(dirname "$0")/.." && pwd)
scratch="${TMPDIR:-/tmp}/imgui_osk_engine_test.$$"
trap 'rm -rf "$scratch"' EXIT
mkdir -p "$scratch/SDL3"

cp "$root/src/osdep/imgui_osk.cpp" "$scratch/imgui_osk_engine.cpp"

cat > "$scratch/sysconfig.h" <<'EOF'
#pragma once
#include <cstddef>
/* Minimal platform shim: only what uae/types.h needs for its typedefs. */
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG __SIZEOF_LONG__
#define SIZEOF_LONG_LONG 8
#define MAX_INPUT_SUB_EVENT 8
struct TrapContext;
EOF

cat > "$scratch/sysdeps.h" <<'EOF'
#pragma once
#include <strings.h> /* strcasecmp */
EOF

cat > "$scratch/SDL3/SDL.h" <<'EOF'
#pragma once
/* Minimal SDL3 shim: the OSK engine only consumes a monotonic clock. */
#include <cstdint>
typedef uint64_t Uint64;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Event SDL_Event;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
Uint64 SDL_GetTicks(void);
EOF

inc="-I$scratch -I$root/src/osdep -I$root/src/include -I$root/external/imgui"
out="$scratch/imgui_osk_engine_test"

# The production unit is compiled without -Werror so unrelated latent
# warnings cannot mask the regression; the harness follows the repo's
# strict convention.
"$cxx" -std=c++17 -Wall -Wextra $inc -c "$scratch/imgui_osk_engine.cpp" -o "$scratch/imgui_osk_engine.o"
"$cxx" -std=c++17 -Wall -Wextra -Werror $inc -c "$root/tests/imgui_osk_engine_test.cpp" -o "$scratch/imgui_osk_engine_test.o"
"$cxx" -std=c++17 "$scratch/imgui_osk_engine.o" "$scratch/imgui_osk_engine_test.o" \
	"$root/external/imgui/imgui.cpp" "$root/external/imgui/imgui_draw.cpp" \
	"$root/external/imgui/imgui_tables.cpp" "$root/external/imgui/imgui_widgets.cpp" -o "$out"
"$out"
