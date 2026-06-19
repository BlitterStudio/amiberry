#!/usr/bin/env bash
set -euo pipefail

usage() {
	cat <<'EOF'
Usage: tools/run_libretro_local.sh [options]

Options:
  --roms PATH        Copy Kickstart ROM file(s) from PATH into the test system/Kickstarts dir.
                     PATH may be a file or a directory. May be passed more than once.
  --content PATH     Copy and launch this content file, usually a WHDLoad .lha/.lzh.
  --test-root PATH   Use this test root. Default: build/libretro-test
  --run-secs N       Launch RetroArch, wait N seconds, then terminate it.
  --no-build         Reuse the existing libretro core.
  --no-run           Prepare/build only, do not launch RetroArch.
  --refresh-assets   Replace the test-root WHDBoot assets from the repo.
  -h, --help         Show this help.

Environment:
  RETROARCH_BIN           RetroArch executable path.
  AMIBERRY_TEST_ROMS      Optional colon-separated ROM source path(s).
  AMIBERRY_TEST_CONTENT   Optional content path.
  AMIBERRY_TEST_ROOT      Optional test root.
  AMIBERRY_TEST_RUN_SECS  Optional timed-run duration.
EOF
}

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_root="${AMIBERRY_TEST_ROOT:-$repo_root/build/libretro-test}"
content="${AMIBERRY_TEST_CONTENT:-}"
run_secs="${AMIBERRY_TEST_RUN_SECS:-}"
do_build=1
do_run=1
refresh_assets=0
rom_sources=()

if [[ -n "${AMIBERRY_TEST_ROMS:-}" ]]; then
	IFS=':' read -r -a rom_sources <<< "$AMIBERRY_TEST_ROMS"
fi

while [[ $# -gt 0 ]]; do
	case "$1" in
		--roms)
			[[ $# -ge 2 ]] || { echo "--roms requires a path" >&2; exit 2; }
			rom_sources+=("$2")
			shift 2
			;;
		--content)
			[[ $# -ge 2 ]] || { echo "--content requires a path" >&2; exit 2; }
			content="$2"
			shift 2
			;;
		--test-root)
			[[ $# -ge 2 ]] || { echo "--test-root requires a path" >&2; exit 2; }
			test_root="$2"
			shift 2
			;;
		--run-secs)
			[[ $# -ge 2 ]] || { echo "--run-secs requires seconds" >&2; exit 2; }
			run_secs="$2"
			shift 2
			;;
		--no-build)
			do_build=0
			shift
			;;
		--no-run)
			do_run=0
			shift
			;;
		--refresh-assets)
			refresh_assets=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			usage >&2
			exit 2
			;;
	esac
done

if [[ -n "${RETROARCH_BIN:-}" ]]; then
	retroarch_bin="$RETROARCH_BIN"
elif [[ -x "/Applications/RetroArch Nightly.app/Contents/MacOS/RetroArch" ]]; then
	retroarch_bin="/Applications/RetroArch Nightly.app/Contents/MacOS/RetroArch"
elif [[ -x /Applications/RetroArch.app/Contents/MacOS/RetroArch ]]; then
	retroarch_bin=/Applications/RetroArch.app/Contents/MacOS/RetroArch
elif command -v retroarch >/dev/null 2>&1; then
	retroarch_bin="$(command -v retroarch)"
else
	echo "RetroArch not found. Install it or set RETROARCH_BIN." >&2
	exit 1
fi

system_dir="$test_root/system"
save_dir="$test_root/save"
state_dir="$test_root/state"
content_dir="$test_root/content"
log_dir="$test_root/logs"
kick_dir="$system_dir/Kickstarts"
cfg="$test_root/retroarch-test.cfg"

mkdir -p "$kick_dir" "$save_dir/Amiberry" "$state_dir/Amiberry" "$content_dir" "$log_dir"

if [[ "$refresh_assets" -eq 1 || ! -e "$system_dir/whdboot/WHDLoad" ]]; then
	rm -rf "$system_dir/whdboot"
	cp -R "$repo_root/whdboot" "$system_dir/"
fi

for src in "${rom_sources[@]}"; do
	[[ -n "$src" ]] || continue
	if [[ -d "$src" ]]; then
		find "$src" -maxdepth 1 -type f -exec cp -p {} "$kick_dir/" \;
	elif [[ -f "$src" ]]; then
		cp -p "$src" "$kick_dir/"
	else
		echo "ROM source not found: $src" >&2
		exit 1
	fi
done

launch_content=()
if [[ -n "$content" ]]; then
	if [[ ! -f "$content" ]]; then
		echo "Content not found: $content" >&2
		exit 1
	fi
	content_target="$content_dir/$(basename "$content")"
	cp -p "$content" "$content_target"
	launch_content=("$content_target")
fi

cat > "$cfg" <<EOF
system_directory = "$system_dir"
savefile_directory = "$save_dir"
savestate_directory = "$state_dir"
video_fullscreen = "false"
video_shader_enable = "false"
pause_nonactive = "false"
menu_pause_libretro = "false"
EOF

os_name="$(uname -s)"
host_arch="$(uname -m)"
core_ext="so"
target_arch="$host_arch"
run_prefix=()

if [[ "$os_name" == "Darwin" ]]; then
	core_ext="dylib"
	retroarch_file_info="$(file "$retroarch_bin")"
	if [[ "$retroarch_file_info" == *"x86_64"* && "$retroarch_file_info" != *"arm64"* ]]; then
		target_arch="x86_64"
		if [[ "$host_arch" == "arm64" ]]; then
			run_prefix=(arch -x86_64)
		fi
	elif [[ "$retroarch_file_info" == *"arm64"* ]]; then
		target_arch="arm64"
	fi
fi

core="$repo_root/libretro/amiberry_libretro.$core_ext"
arch_stamp="$repo_root/libretro/.amiberry_libretro_build_arch"

if [[ "$do_build" -eq 1 ]]; then
	build_args=(ARCH="$target_arch")
	if [[ "$os_name" == "Darwin" && "$target_arch" == "x86_64" ]]; then
		build_args+=(CC="clang -arch x86_64" CXX="clang++ -arch x86_64")
	fi

	needs_clean=0
	if [[ -e "$arch_stamp" ]]; then
		previous_arch="$(cat "$arch_stamp")"
		if [[ "$previous_arch" != "$target_arch" ]]; then
			needs_clean=1
		fi
	elif find "$repo_root/libretro" "$repo_root/src" -name '*.o' -print -quit | grep -q .; then
		needs_clean=1
	fi
	if [[ -e "$core" ]]; then
		core_file_info="$(file "$core")"
		if [[ "$core_file_info" != *"$target_arch"* ]]; then
			needs_clean=1
		fi
	fi
	if [[ "$needs_clean" -eq 1 ]]; then
		make -C "$repo_root/libretro" clean
		rm -f "$arch_stamp"
	fi

	if command -v sysctl >/dev/null 2>&1; then
		jobs="$(sysctl -n hw.logicalcpu 2>/dev/null || true)"
	else
		jobs=""
	fi
	if [[ -z "$jobs" && -r /proc/cpuinfo ]]; then
		jobs="$(grep -c '^processor' /proc/cpuinfo || true)"
	fi
	[[ -n "$jobs" ]] || jobs=4

	make -C "$repo_root/libretro" -j"$jobs" "${build_args[@]}"
	printf '%s\n' "$target_arch" > "$arch_stamp"
	rm -f "$repo_root/libretro/amiberry_libretro.info" "$repo_root/libretro/libco/libco.o"
fi

if [[ ! -e "$core" ]]; then
	echo "Core not found: $core" >&2
	exit 1
fi

echo "RetroArch: $retroarch_bin"
echo "Core:      $core ($(file "$core"))"
echo "Test root: $test_root"
echo "ROM dir:   $kick_dir"
if [[ ${#launch_content[@]} -gt 0 ]]; then
	echo "Content:   ${launch_content[0]}"
else
	echo "Content:   <none>"
fi
echo "Core log:  $save_dir/Amiberry/Amiberry.log"
echo "RA log:    $log_dir/retroarch.log"

if [[ "$do_run" -eq 0 ]]; then
	exit 0
fi

if [[ "$os_name" == "Darwin" ]]; then
	rm -rf "$HOME/Library/Saved Application State/com.libretro.RetroArch.savedState"
fi

run_env=(AMIBERRY_LIBRETRO_DEBUG=1)
if [[ "$os_name" == "Darwin" ]]; then
	run_env+=(ApplePersistenceIgnoreState=YES)
fi

cmd=("${run_prefix[@]}" "$retroarch_bin" --appendconfig "$cfg" -L "$core")
if [[ ${#launch_content[@]} -gt 0 ]]; then
	cmd+=("${launch_content[0]}")
fi
cmd+=(-v)

if [[ -n "$run_secs" ]]; then
	rm -f "$save_dir/Amiberry/Amiberry.log"
	env "${run_env[@]}" "${cmd[@]}" >"$log_dir/retroarch.log" 2>&1 &
	pid=$!
	sleep "$run_secs"
	kill -TERM "$pid" >/dev/null 2>&1 || true
	wait "$pid" >/dev/null 2>&1 || true
else
	rm -f "$save_dir/Amiberry/Amiberry.log"
	env "${run_env[@]}" "${cmd[@]}" 2>&1 | tee "$log_dir/retroarch.log"
fi
