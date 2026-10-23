#!/usr/bin/env python3
"""Behavioral test: an RTG board that changes between Zorro III and Zorro II
is brought in by a reset without crashing the emulator.

Usage: rtg_reset_test.py <amiberry binary>

Runs a headless A4000 with the UAE Zorro III RTG board, switches the board to
the UAE Zorro II one through the settings session and resets, then back. The
reset used to lay the memory out for the old board, so the new one mapped
memory that was not there (SIGBUS at the first access).
"""

import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ipc_actions_test as ipc  # noqa: E402  (Emulator, Connection)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    work = tempfile.mkdtemp(prefix="amiberry-rtg-")
    env = dict(os.environ, HOME=work, AMIBERRY_HOME_DIR=os.path.join(work, "home"), SDL_AUDIODRIVER="dummy")
    # The guest only touches the board's memory with a real display behind
    # it (with SDL's dummy driver the old crash never showed), so on macOS
    # the emulator opens a window for the few seconds the test runs. Without
    # a display server (Linux CI) the test still runs, headless, but only
    # checks that the switches work.
    if sys.platform == "darwin":
        env.pop("SDL_VIDEODRIVER", None)
    elif not os.environ.get("DISPLAY") and not os.environ.get("WAYLAND_DISPLAY"):
        env["SDL_VIDEODRIVER"] = "offscreen"
    emu = ipc.Emulator(sys.argv[1], env, "A4000", ["gfxcard_size=16"])
    failures = 0
    try:
        control = ipc.Connection(emu.endpoint, 30)
        for board in ("ZorroII", "ZorroIII", "ZorroII"):
            replies = [control.request(f"SETTINGS_SET\tgfxcard_type\t{board}"),
                       control.request("SETTINGS_COMMIT"), control.request("SETTINGS_APPLY\treset")]
            if replies[0] != "OK" or not replies[1].startswith("OK") or replies[2] != "OK":
                print(f"FAIL switching to {board}: {replies}")
                failures += 1
                continue
            # Long enough for the reset to map the board and the guest to run
            # on it (its RTG driver touches the board's memory).
            time.sleep(4)
            active = control.request("SETTINGS_GET_ACTIVE\tgfxcard_type")
            status = control.request("GET_STATUS")
            if active != f"OK\tgfxcard_type={board}" or not status.startswith("OK"):
                print(f"FAIL {board} after reset: {active}, {status}")
                failures += 1
            else:
                print(f"ok   {board} board after reset, emulator running")
    except OSError as e:
        print(f"FAIL the emulator went away: {e}")
        sys.stderr.write("emulator output (last lines):\n" + "".join(emu.output[-40:]))
        failures += 1
    finally:
        emu.kill()
    if failures:
        sys.exit(f"RTG reset test: {failures} failure(s)")
    print("RTG reset test passed")


if __name__ == "__main__":
    main()
