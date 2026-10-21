#!/usr/bin/env python3
"""Behavioral test for the IPC control transport: a Unix-domain socket, or a
named pipe on Windows. Starts a headless emulator and checks what a controller
relies on: replies arrive in order, a controller that stops reading does not
hold up others, oversized requests are refused, abandoned connections free
their slots, a second instance gets its own endpoint, and QUIT is answered
before the process exits.

Usage: ipc_transport_test.py <path-to-amiberry>
"""

import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time

IS_WINDOWS = sys.platform == "win32"
REQUEST_LIMIT = 64 * 1024
MAX_CLIENTS = 16
failures = []


def check(name, condition, detail=""):
    print(("ok   " if condition else "FAIL ") + name + (f" ({detail})" if detail else ""), flush=True)
    if not condition:
        failures.append(name)


class Connection:
    """One controller connection, with the same calls on both transports."""

    def __init__(self, endpoint, timeout=10.0):
        deadline = time.monotonic() + timeout
        while True:
            try:
                if IS_WINDOWS:
                    self._pipe = open(endpoint, "r+b", buffering=0)
                else:
                    self._sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    try:
                        self._sock.connect(endpoint)
                    except OSError:
                        self._sock.close()
                        raise
                    self._sock.settimeout(timeout)
                return
            except OSError:
                # ERROR_PIPE_BUSY: Amiberry creates the next pipe instance
                # within a frame.
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.02)

    def send(self, data):
        if IS_WINDOWS:
            self._pipe.write(data)
        else:
            self._sock.sendall(data)

    def recv(self):
        try:
            return self._pipe.read(65536) if IS_WINDOWS else self._sock.recv(65536)
        except (BrokenPipeError, ConnectionResetError):
            return b""

    def lines(self, count):
        """Read until count newline-terminated replies arrive or the stream ends."""
        data = b""
        while data.count(b"\n") < count:
            chunk = self.recv()
            if not chunk:
                break
            data += chunk
        return data.split(b"\n")[:count] if data.count(b"\n") >= count else data.split(b"\n")[:-1]

    def request(self, line):
        self.send(line.encode() + b"\n")
        replies = self.lines(1)
        return replies[0].decode(errors="replace") if replies else ""

    def shutdown_write(self):
        self._sock.shutdown(socket.SHUT_WR)

    def close(self):
        (self._pipe if IS_WINDOWS else self._sock).close()


class Emulator:
    """A headless emulator instance and the endpoint it reports."""

    def __init__(self, binary, env):
        self.output = []
        self.proc = subprocess.Popen(
            [binary, "--model", "A500", "-G"], env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
        threading.Thread(target=self._drain, daemon=True).start()
        deadline = time.monotonic() + 60
        self.endpoint = None
        while self.endpoint is None and time.monotonic() < deadline and self.proc.poll() is None:
            for line in list(self.output):
                match = re.search(r"IPC: Listening on (\S+)", line)
                if match:
                    self.endpoint = match.group(1)
            time.sleep(0.05)
        if self.endpoint is None:
            self.kill()
            sys.exit("IPC endpoint never reported; output:\n" + "".join(self.output[-40:]))

    def _drain(self):
        for raw in self.proc.stdout:
            self.output.append(raw.decode(errors="replace"))

    def kill(self):
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    binary = sys.argv[1]
    work = tempfile.mkdtemp(prefix="amiberry-ipc-")
    env = dict(os.environ)
    # Isolated settings, headless video and audio.
    env.update({"HOME": work, "XDG_RUNTIME_DIR": work, "XDG_CONFIG_HOME": os.path.join(work, "config"),
                "XDG_DATA_HOME": os.path.join(work, "data"), "AMIBERRY_HOME_DIR": os.path.join(work, "home"),
                "LOCALAPPDATA": os.path.join(work, "local"), "APPDATA": os.path.join(work, "roaming"),
                # SDL's offscreen driver is EGL-based and cannot create even
                # a plain window on macOS; the dummy driver can.
                "SDL_VIDEODRIVER": "dummy" if sys.platform == "darwin" else "offscreen",
                "SDL_AUDIODRIVER": "dummy"})
    env.pop("AMIBERRY_CONFIG_DIR", None)

    first = Emulator(binary, env)
    second = None
    try:
        endpoint = first.endpoint
        control = Connection(endpoint, 30)
        check("PING round trip", control.request("PING") == "OK\tPONG")

        # A controller that writes many requests and stops reading must not
        # hold up another controller, and must get every reply, in order,
        # once it reads again.
        hog = Connection(endpoint)
        hog.send(b"PING\n" * 2000 + b"HELP\n" * 300)
        time.sleep(1)
        started = time.monotonic()
        reply = control.request("PING")
        check("second controller answered while the first stops reading",
              reply == "OK\tPONG" and time.monotonic() - started < 2, reply)
        replies = hog.lines(2300)
        in_order = len(replies) == 2300 and all(r == b"OK\tPONG" for r in replies[:2000]) \
            and all(r.startswith(b"OK\t") and r != b"OK\tPONG" for r in replies[2000:])
        check("pipelined replies all arrive in order", in_order, f"{len(replies)} replies")
        hog.close()

        # A request line past the limit is refused and the connection closed.
        big = Connection(endpoint)
        big.send(b"A" * (REQUEST_LIMIT + 4096))
        replies = big.lines(1)
        check("oversized request refused", replies == [b"ERROR\tRequest too long"], repr(replies))
        check("oversized request connection closed", big.recv() == b"")
        big.close()

        # Connections that send a burst and close without reading the replies
        # must free their slots, or the next controller is locked out.
        for _ in range(MAX_CLIENTS):
            gone = Connection(endpoint)
            gone.send(b"HELP\n" * 100)
            gone.close()
        late = Connection(endpoint)
        late_reply = []
        worker = threading.Thread(target=lambda: late_reply.append(late.request("PING")), daemon=True)
        worker.start()
        worker.join(10)
        check("slots freed after abandoned connections", late_reply == ["OK\tPONG"], repr(late_reply))
        late.close()

        if not IS_WINDOWS:
            # Unix only: replies still arrive after the client half-closes.
            half = Connection(endpoint)
            half.send(b"PING\nPING\n")
            half.shutdown_write()
            replies = half.lines(3)
            check("half-closed client gets every reply", replies == [b"OK\tPONG", b"OK\tPONG"], repr(replies))
            half.close()

        # A second instance listens on its own endpoint.
        second = Emulator(binary, env)
        check("second instance uses its own endpoint",
              second.endpoint != endpoint and re.search(r"amiberry_\d", second.endpoint) is not None,
              second.endpoint)
        other = Connection(second.endpoint, 30)
        check("second instance answers QUIT", other.request("QUIT") == "OK")
        other.close()

        reply = control.request("QUIT")
        check("QUIT answered before exit", reply == "OK", reply)
        control.close()
        try:
            first.proc.wait(30)
        except subprocess.TimeoutExpired:
            pass
        check("process exits after QUIT", first.proc.poll() is not None)
    finally:
        first.kill()
        if second:
            second.kill()
        shutil.rmtree(work, ignore_errors=True)

    if failures:
        sys.exit(f"{len(failures)} IPC transport check(s) failed")
    print("IPC transport behavioral test passed")


if __name__ == "__main__":
    main()
