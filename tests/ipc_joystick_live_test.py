#!/usr/bin/env python3
"""Exercise SEND_JOYSTICK through real JOYDAT, CIA and CD32 serial registers.

Usage: ipc_joystick_live_test.py <path-to-amiberry>
"""

import os
import shutil
import subprocess
import sys
import time

import ipc_actions_test
from ipc_actions_test import Connection, Emulator, check


def exit_status():
    return 1 if ipc_actions_test.failures else 0


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    binary = os.path.abspath(sys.argv[1])
    work = os.path.abspath(f".ipc-joystick-live-{os.getpid()}")
    os.mkdir(work)
    emu = None
    connections = []
    try:
        env = dict(os.environ)
        env.update({
            "HOME": work, "XDG_RUNTIME_DIR": work,
            "XDG_CONFIG_HOME": os.path.join(work, "config"),
            "XDG_DATA_HOME": os.path.join(work, "data"),
            "AMIBERRY_HOME_DIR": os.path.join(work, "home"),
            "LOCALAPPDATA": os.path.join(work, "local"),
            "APPDATA": os.path.join(work, "roaming"),
            "SDL_VIDEODRIVER": "dummy" if sys.platform == "darwin" else "offscreen",
            "SDL_AUDIODRIVER": "dummy",
        })
        env.pop("AMIBERRY_CONFIG_DIR", None)
        startup_config = os.path.join(work, "virtual-pad.uae")
        with open(startup_config, "w") as config:
            config.write(
                "config_version=8.5.0\nconfig_hardware=true\nconfig_host=true\n"
                "joyport0=none\njoyport0mode=djoy\n"
                "joyport1=none\njoyport1mode=cd32joy\n")
        resolved = subprocess.run(
            [binary, "--model", "A1200", "--config", startup_config, "--dump-config"],
            env=env, capture_output=True, text=True, timeout=60)
        check("config-file virtual pad modes survive startup normalization",
              resolved.returncode == 0
              and "joyport0mode=djoy" in resolved.stdout.splitlines()
              and "joyport1mode=cd32joy" in resolved.stdout.splitlines(),
              f"exit={resolved.returncode}")
        # No keyboard or physical controller is needed, including for CD32.
        options = [f"joyport{port}=none" for port in range(4)]
        options += [f"joyport{port}mode={'cd32joy' if port == 1 else 'djoy'}"
                    for port in range(4)]
        emu = Emulator(binary, env, options=options)
        control = Connection(emu.endpoint)
        connections.append(control)
        check("SEND_JOYSTICK is discoverable", "SEND_JOYSTICK" in control.request("HELP"))
        for port in range(4):
            mode = "7" if port == 1 else "3"
            check(f"port {port} mode", control.request(f"GET_JOYPORT_MODE\t{port}").startswith(f"OK\t{mode}\t"))
        check("pause for deterministic register reads", control.request("PAUSE") == "OK")

        def read(address, width=2):
            response = control.request(f"READ_MEM\t{address:#x}\t{width}")
            assert response.startswith("OK\t"), response
            return int(response.split("\t")[1])

        def write(address, value, width=2):
            response = control.request(f"WRITE_MEM\t{address:#x}\t{width}\t{value}")
            assert response == "OK", response

        def send(port, directions=0, buttons=0, client=control):
            response = client.request(f"SEND_JOYSTICK\t{port}\t{directions}\t{buttons}")
            assert response == "OK", response

        def directions(port):
            if port < 2:
                value = read(0xDFF00A + 2 * port)
                return (((value >> 9) & 1)
                        | (((value >> 1) & 1) << 1)
                        | ((((value >> 9) ^ (value >> 8)) & 1) << 2)
                        | ((((value >> 1) ^ value) & 1) << 3))
            value = (~read(0xBFE101, 1) >> (4 * (port - 2))) & 15
            return ((value & 4) >> 2) | ((value & 8) >> 2) | ((value & 1) << 2) | ((value & 2) << 2)

        write(0xBFE301, 0, 1)  # CIA-A DDRB: parallel data pins are inputs.
        for port in range(4):
            for mask in (0, 1, 2, 4, 8, 5, 6, 9, 10, 0):
                send(port, mask)
                check(f"port {port} direction mask {mask}", directions(port) == mask)

        for command in (
                "SEND_JOYSTICK", "SEND_JOYSTICK\t0\t0",
                "SEND_JOYSTICK\t0\t0\t0\t1", "SEND_JOYSTICK\t0\t0\t0\t",
                "SEND_JOYSTICK\t4\t0\t0", "SEND_JOYSTICK\t-1\t0\t0",
                "SEND_JOYSTICK\t0\t16\t0", "SEND_JOYSTICK\t0\t3\t0",
                "SEND_JOYSTICK\t0\t12\t0", "SEND_JOYSTICK\t0\t0\t128",
                "SEND_JOYSTICK\t0\t1junk\t0", "SEND_JOYSTICK\t0\t0\t0x1",
                "SEND_JOYSTICK\t0\t0\t", "SEND_JOYSTICK\t0\t0\t4",
                "SEND_JOYSTICK\t2\t0\t4"):
            send(0, 5, 1)
            response = control.request(command)
            check(f"reject {command!r}", response.startswith("ERROR\t"), response)
            check("invalid command preserves held state", directions(0) == 5)
        send(0)

        # Generic fire uses the existing CIA-A active-low input pins.
        write(0xBFE201, read(0xBFE201, 1) & ~0x40, 1)
        for buttons in (0, 1, 0):
            send(0, buttons=buttons)
            check(f"primary fire {buttons}", bool(read(0xBFE001, 1) & 0x40) == (buttons == 0))
        # Drive POTGO outputs high, as software does for the second button.
        write(0xDFF034, 0xFF00)
        for buttons in (0, 2, 0):
            send(0, buttons=buttons)
            for _ in range(8):
                value = read(0xDFF016)
            check(f"secondary fire {buttons}", bool(value & 0x400) == (buttons == 0))

        def serial():
            # Normal mode resets the real shifter to its first button.
            write(0xDFF034, 0xFF00)
            for _ in range(8):
                read(0xDFF016)
            write(0xBFE001, 0x02, 1)
            write(0xBFE201, 0x80, 1)
            write(0xDFF034, 0x2F00)
            for _ in range(8):
                read(0xDFF016)
            result = 0
            for bit in range(9):
                if read(0xDFF016) & 0x4000:
                    result |= 1 << bit
                write(0xBFE001, 0x82, 1)
                write(0xBFE001, 0x02, 1)
            return result

        for buttons, serial_pressed in (
                (0, 0), (1, 2), (2, 1), (4, 8), (8, 4),
                (16, 32), (32, 16), (64, 64), (3, 3), (127, 127), (0, 0)):
            send(1, buttons=buttons)
            actual = serial()
            check(f"CD32 buttons {buttons}: real serial bits and ID",
                  actual == (0xFF ^ serial_pressed), hex(actual))

        peer = Connection(emu.endpoint)
        connections.append(peer)
        send(0, 1, 1)
        send(0, 4, 2, peer)
        check("independent clients merge directions", directions(0) == 5)
        peer.close()
        connections.remove(peer)
        deadline = time.monotonic() + 3
        while directions(0) != 1 and time.monotonic() < deadline:
            time.sleep(0.02)
        check("disconnect releases only its own source", directions(0) == 1)
        send(0)
        check("explicit neutral releases directions", directions(0) == 0)

        peer = Connection(emu.endpoint)
        connections.append(peer)
        send(1, buttons=127, client=peer)
        peer.close()
        connections.remove(peer)
        # An unrelated request gives the server a chance to observe the EOF.
        deadline = time.monotonic() + 3
        actual = serial()
        while actual != 0xFF and time.monotonic() < deadline:
            actual = serial()
        check("disconnect releases every CD32 button", actual == 0xFF, hex(actual))
        check("resume", control.request("RESUME") == "OK")
        send(0, 5, 1)
        check("reset accepted", control.request("RESET") == "OK")
        time.sleep(0.2)
        check("pause after reset", control.request("PAUSE") == "OK")
        check("reset releases injected directions", directions(0) == 0)
    finally:
        for connection in connections:
            connection.close()
        if emu is not None:
            emu.kill()
        shutil.rmtree(work)
    return exit_status()


if __name__ == "__main__":
    sys.exit(main())
