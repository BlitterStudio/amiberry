#!/usr/bin/env python3
"""Behavioral test of the settings layer's apply classes.

Usage: settings_apply_test.py <amiberry binary> <model> [id=value ...] [option id ...]

id=value arguments configure the emulation at start (-s id=value), to make
options available that depend on them; plain IDs limit the scan to those
options.

Runs a headless emulator and, for every option the IPC settings session
offers (or the ones named), changes the value, commits it and applies it
through SETTINGS_APPLY live, then reset, then restart, reading the value
the running emulation uses (SETTINGS_GET_ACTIVE) after each step. The step
at which the emulation takes the value up must be the option's declared
apply class: a LIVE option is running after the live step, a RESET option
only after the reset, a RESTART option only after the restart. The whole
configuration is put back afterwards.
"""

import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ipc_actions_test as ipc  # noqa: E402  (Emulator, Connection)

STEPS = ("live", "reset", "restart")
# Options whose value is not a configuration of the running emulation the
# test can drive headless: paths and free-form strings (ROM files and other
# files that must exist).
SKIP_TYPES = ("path", "string")


class Controller:
    def __init__(self, emu):
        self.emu = emu
        self.connection = ipc.Connection(emu.endpoint, 30)

    def request(self, line):
        # A restart rebuilds the IPC server; reconnect once if it went away.
        try:
            return self.connection.request(line)
        except OSError:
            deadline = time.monotonic() + 30
            while True:
                try:
                    self.connection = ipc.Connection(self.emu.endpoint, 30)
                    return self.connection.request(line)
                except OSError:
                    if time.monotonic() > deadline:
                        raise
                    time.sleep(0.2)

    def fields(self, line):
        reply = self.request(line)
        parts = reply.split("\t")
        if parts[0] != "OK":
            return None
        return parts[1:]

    def value(self, command, option):
        fields = self.fields(f"{command}\t{option}")
        if fields is None:
            return None
        return fields[0].split("=", 1)[1]

    def describe(self, option):
        fields = self.fields(f"SETTINGS_DESCRIBE\t{option}")
        info = {"choices": []}
        for field in fields or []:
            key, _, value = field.partition("=")
            if key == "choice":
                info["choices"].append(value)
            elif key != "unavailable_choice":
                info[key] = value
        return info

    def wait_active(self, option, expected, seconds):
        deadline = time.monotonic() + seconds
        while True:
            active = self.value("SETTINGS_GET_ACTIVE", option)
            if active == expected or time.monotonic() > deadline:
                return active
            time.sleep(0.05)


def candidates(info, current):
    kind = info.get("type")
    if kind == "bool":
        return ["false" if current == "true" else "true"]
    if kind == "choice":
        return [c for c in info["choices"] if c != current]
    if kind in ("int", "real"):
        values = []
        for key in ("max", "min"):
            if key in info and info[key] != current:
                values.append(info[key])
        return values
    return []


# The reason validation gives an adjustment of an option without its own
# constraint text.
GENERIC_REASON = "Changed by the emulator's configuration checks."
unexplained = set()


def change(ctl, option, value):
    """Sets and commits option=value; the committed spelling, or None.
    Records options validation adjusts without saying why."""
    if ctl.request(f"SETTINGS_SET\t{option}\t{value}") != "OK":
        ctl.request("SETTINGS_RESET_SESSION")
        return None
    for field in ctl.fields("SETTINGS_VALIDATE") or []:
        if field != "none" and field.endswith(f"({GENERIC_REASON})"):
            unexplained.add(field.split("=", 1)[0])
    reply = ctl.request("SETTINGS_COMMIT")
    if not reply.startswith("OK\tchanged=true"):
        return None
    return ctl.value("SETTINGS_GET", option)


def snapshot(ctl, options):
    return {option: ctl.value("SETTINGS_GET", option) for option in options}


def restore(ctl, options, baseline):
    """Sets every option back to its baseline value and applies that with
    the class commit reports. Setters change other options, so it repeats
    until nothing differs."""
    changed = set()
    for attempt in range(6):
        current = snapshot(ctl, options)
        # An option that is unavailable now has no effect and cannot be set
        # (a CPU multiplier a CPU change wrote while the speed is "max").
        differing = [o for o in options if current[o] != baseline[o]
                     and ctl.describe(o).get("available") == "true"]
        if not differing:
            break
        changed.update(differing)
        # An option can only be set back while the ones it depends on still
        # allow it (a CPU multiplier before the CPU speed that hides it),
        # so the order alternates.
        for option in (reversed(differing) if attempt % 2 == 0 else differing):
            ctl.request(f"SETTINGS_SET\t{option}\t{baseline[option]}")
        reply = ctl.request("SETTINGS_COMMIT")
        apply = reply.rsplit("apply=", 1)[-1] if "apply=" in reply else "live"
        settle(ctl, apply)
    else:
        raise RuntimeError("the configuration does not return to its baseline: " + ", ".join(
            f"{o}={current[o]} (was {baseline[o]})" for o in differing))
    # The running emulation catches up with what was restored.
    for option in changed:
        ctl.wait_active(option, baseline[option], 30)


def settle(ctl, step):
    reply = ctl.request(f"SETTINGS_APPLY\t{step}")
    if reply != "OK":
        raise RuntimeError(f"SETTINGS_APPLY {step}: {reply}")


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    binary = sys.argv[1]
    model = sys.argv[2]
    prerequisites = [a for a in sys.argv[3:] if "=" in a]
    only = {a for a in sys.argv[3:] if "=" not in a}
    work = tempfile.mkdtemp(prefix="amiberry-apply-")
    env = dict(os.environ)
    env.update({"HOME": work, "XDG_RUNTIME_DIR": work, "XDG_CONFIG_HOME": os.path.join(work, "config"),
                "XDG_DATA_HOME": os.path.join(work, "data"), "AMIBERRY_HOME_DIR": os.path.join(work, "home"),
                "LOCALAPPDATA": os.path.join(work, "local"), "APPDATA": os.path.join(work, "roaming"),
                "SDL_VIDEODRIVER": "dummy" if sys.platform == "darwin" else "offscreen",
                "SDL_AUDIODRIVER": "dummy"})
    env.pop("AMIBERRY_CONFIG_DIR", None)

    emu = ipc.Emulator(binary, env, model, prerequisites)
    mismatches = []
    checked = skipped = 0
    try:
        ctl = Controller(emu)
        options = ctl.fields("SETTINGS_LIST") or []
        baseline = snapshot(ctl, options)
        for option in options:
            if only and option not in only:
                continue
            info = ctl.describe(option)
            if info.get("type") in SKIP_TYPES or info.get("available") != "true":
                print(f"skip {option}: {info.get('type')}, available={info.get('available')}")
                skipped += 1
                continue
            original = ctl.value("SETTINGS_GET", option)
            if ctl.value("SETTINGS_GET_ACTIVE", option) != original:
                # Something else is still pending; settle it first.
                settle(ctl, "restart")
                ctl.wait_active(option, original, 20)
            target = None
            for value in candidates(info, original):
                target = change(ctl, option, value)
                if target is not None and target != original:
                    break
                target = None
            if target is None:
                print(f"skip {option}: no other value commits ({original}; tried {candidates(info, original)})")
                skipped += 1
                continue
            observed = None
            for step in STEPS:
                settle(ctl, step)
                wait = {"live": 1.0, "reset": 5.0, "restart": 30.0}[step]
                if ctl.wait_active(option, target, wait) == target:
                    observed = step
                    break
            checked += 1
            declared = info.get("apply")
            if observed != declared:
                mismatches.append((option, declared, observed, original, target))
                print(f"MISMATCH {option}: declared {declared}, running after {observed} ({original} -> {target})",
                      flush=True)
            # Put the whole configuration back, dependent changes included,
            # so the next option starts from the same machine.
            restore(ctl, options, baseline)
    except Exception:
        # What the emulator said last explains a crash or a stuck restart.
        sys.stderr.write("emulator output (last lines):\n" + "".join(emu.output[-60:]))
        raise
    finally:
        emu.kill()

    for option in sorted(unexplained):
        print(f"UNEXPLAINED {option}: adjusted by validation without a reason of its own", flush=True)
    print(f"{model}: {checked} options checked, {skipped} skipped, {len(mismatches)} mismatches, "
          f"{len(unexplained)} unexplained adjustments")
    if mismatches or unexplained:
        sys.exit(1)


if __name__ == "__main__":
    main()
