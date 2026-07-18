#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry_rp9.cpp"

if ! grep -F -q 'std::string loaded_snapshot_path;' "$source_file" ||
	! grep -F -q 'loaded_snapshot_path = savestate_fname;' "$source_file"; then
	echo "RP9 snapshot ownership must be tracked independently of its storage root" >&2
	exit 1
fi

if ! awk '
	/bool pending_snapshot_belongs_to_loaded_rp9\(\)/ { in_check = 1 }
	in_check && /savestate_state == STATE_DORESTORE/ { pending = 1 }
	in_check && /loaded_snapshot_path == savestate_fname/ { owned = 1 }
	in_check && /^}/ { exit }
	END { exit pending && owned ? 0 : 1 }
' "$source_file"; then
	echo "Only the still-pending snapshot owned by the loaded RP9 may be cleared" >&2
	exit 1
fi

if ! awk '
	/void rp9_clear_loaded_path\(\)/ { in_clear = 1 }
	in_clear && /pending_snapshot_belongs_to_loaded_rp9\(\)/ { ownership_check = 1 }
	in_clear && ownership_check && /savestate_state = 0;/ { state = 1 }
	in_clear && ownership_check && /savestate_fname\[0\] = 0;/ { filename = 1 }
	in_clear && /loaded_snapshot_path\.clear\(\);/ { tracking = 1 }
	in_clear && /^}/ { exit }
	END { exit state && filename && tracking ? 0 : 1 }
' "$source_file"; then
	echo "Clearing RP9 content must discard its pending snapshot regardless of media root" >&2
	exit 1
fi
