#!/usr/bin/env bash
# Checks the shared settings layer's descriptors, rules, edit sessions and
# validation against the core's fixup_cpu().
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/settings_layer_test.$$"
trap 'rm -f "$out" "$out.c"' EXIT

# Front ends in other languages bind the public header through their C
# importers, so it must stay strict C99.
printf '#include "amiberry_settings.h"\nint main(void) { return amiberry_settings_count() ? 0 : 1; }\n' > "$out.c"
${CC:-cc} -std=c99 -Wall -Wextra -pedantic -Werror -Isrc/osdep/settings -fsyntax-only "$out.c"

# The runtime actions header carries the same C99 contract.
printf '#include "amiberry_actions.h"\nint main(void) { amiberry_actions_status s; amiberry_actions_get_status(&s); return amiberry_actions_run_pending() ? 0 : 1; }\n' > "$out.c"
${CC:-cc} -std=c99 -Wall -Wextra -pedantic -Werror -Isrc/osdep/settings -fsyntax-only "$out.c"

# So does the media and folders header.
printf '#include "amiberry_media.h"\nint main(void) { char p[8]; return amiberry_folder_get(AMIBERRY_FOLDER_ROMS, p, sizeof p) == AMIBERRY_SETTINGS_OK && amiberry_media_refresh(AMIBERRY_MEDIA_CONFIGURATIONS) ? 0 : 1; }\n' > "$out.c"
${CC:-cc} -std=c99 -Wall -Wextra -pedantic -Werror -Isrc/osdep/settings -fsyntax-only "$out.c"

. tests/lib/settings_layer_build.sh
settings_layer_compile "$out"
"$out"
# Builds without the JIT, PPC and PCem (iOS, RISC-V, -DUSE_JIT=OFF) must
# hold the same rules.
SETTINGS_LAYER_DEFINES="-DAMIBERRY_NO_JIT" settings_layer_compile "$out"
"$out"
