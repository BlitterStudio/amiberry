#!/usr/bin/env python3
"""Behavioral test: expansion boards a reset brings in get memory of their own.

Usage: board_reset_test.py <amiberry binary>

A reset used to lay memory out for the running boards before it brought the
new ones in, so:
- an RTG board switched between Zorro III and Zorro II kept its memory at the
  old address and the new one mapped memory that was not there (SIGBUS at the
  first access);
- a Zorro III RAM board turned on got the memory of chip RAM (writes to the
  board changed chip RAM).
"""

import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ipc_actions_test as ipc  # noqa: E402  (Emulator, Connection)


def environment(windowed):
    work = tempfile.mkdtemp(prefix="amiberry-boards-")
    env = dict(os.environ, HOME=work, AMIBERRY_HOME_DIR=os.path.join(work, "home"), SDL_AUDIODRIVER="dummy")
    if windowed:
        env.pop("SDL_VIDEODRIVER", None)
    else:
        # SDL's offscreen driver cannot create windows on macOS; the dummy
        # driver can.
        env["SDL_VIDEODRIVER"] = "dummy" if sys.platform == "darwin" else "offscreen"
    return env


def bring_in(control, option, value):
    replies = [control.request(f"SETTINGS_SET\t{option}\t{value}"),
               control.request("SETTINGS_COMMIT"), control.request("SETTINGS_APPLY\treset")]
    if replies[0] != "OK" or not replies[1].startswith("OK") or replies[2] != "OK":
        return f"{option}={value} not applied: {replies}"
    # Long enough for the reset to map the board and the guest to run on it.
    time.sleep(4)
    return None


def check_rtg(binary):
    """The RTG board switched between Zorro III and Zorro II, then back."""
    # The guest only touches the board's memory with a real display behind
    # it (with SDL's dummy driver the crash never showed), so on macOS the
    # emulator opens a window for the few seconds this takes. Without a
    # display server (Linux CI) the check runs headless and only shows that
    # the switches work.
    windowed = sys.platform == "darwin" or bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"))
    emu = ipc.Emulator(binary, environment(windowed), "A4000", ["gfxcard_size=16"])
    failures = []
    try:
        control = ipc.Connection(emu.endpoint, 30)
        for board in ("ZorroII", "ZorroIII", "ZorroII"):
            error = bring_in(control, "gfxcard_type", board)
            active = control.request("SETTINGS_GET_ACTIVE\tgfxcard_type")
            status = control.request("GET_STATUS")
            if error or active != f"OK\tgfxcard_type={board}" or not status.startswith("OK"):
                failures.append(f"RTG {board} board after reset: {error or (active, status)}")
            else:
                print(f"ok   RTG {board} board after reset, emulator running")
    except OSError as e:
        failures.append(f"RTG board switch: the emulator went away ({e})")
        sys.stderr.write("emulator output (last lines):\n" + "".join(emu.output[-40:]))
    finally:
        emu.kill()
    return failures


def check_ram(binary):
    """A Zorro III RAM board turned on: its memory is not chip RAM."""
    emu = ipc.Emulator(binary, environment(False), "A4000")
    failures = []
    try:
        control = ipc.Connection(emu.endpoint, 30)
        error = bring_in(control, "z3mem2_size", "64")
        if error:
            return [error]
        control.request("PAUSE")
        board, chip = 0x40000100, 0x00000100  # the board's first Zorro III address
        for marker in (0x5A5AA5A5, 0x12345678):
            control.request(f"WRITE_MEM\t{board:#x}\t4\t{marker:#x}")
            on_board = control.request(f"READ_MEM\t{board:#x}\t4")
            in_chip = control.request(f"READ_MEM\t{chip:#x}\t4")
            if on_board != f"OK\t{marker}":
                failures.append(f"Z3 RAM board does not hold {marker:#x}: {on_board}")
            elif in_chip == f"OK\t{marker}":
                failures.append(f"Z3 RAM board shares chip RAM: {marker:#x} written to it is in chip RAM")
        if not failures:
            print("ok   Z3 RAM board brought in by a reset has memory of its own")
    except OSError as e:
        failures.append(f"Z3 RAM board: the emulator went away ({e})")
    finally:
        emu.kill()
    return failures


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    failures = check_rtg(sys.argv[1]) + check_ram(sys.argv[1])
    for failure in failures:
        print(f"FAIL {failure}")
    if failures:
        sys.exit(f"board reset test: {len(failures)} failure(s)")
    print("board reset test passed")


if __name__ == "__main__":
    main()
