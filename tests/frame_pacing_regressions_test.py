#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FramePacingProductionTest(unittest.TestCase):
    def test_production_wait(self):
        source = (ROOT / "src/custom.cpp").read_text()
        start = source.index("\t\tif (!cpu_threaded_run) {", source.index("static bool framewait(void)"))
        end = source.index("\n\t\tevt_t tnow =", start)
        wait = source[start:end]
        fixture = r"""
#include <cassert>
#include <cstdint>
#include <iostream>
#include "amiberry_frame_pacing.h"
#define AMIBERRY 1
using frame_time_t = int64_t;
static constexpr int64_t syncbase = 1000000;
static struct { bool turbo_emulation = false; } currprefs;
static bool cpu_threaded_run;
static int64_t now, deadline;
static int sleeps, audio_calls, rtg_calls, audio_cost, overshoot;
static bool sleep_fails;
static frame_time_t read_processor_time() { now += 5; return now; }
static frame_time_t rpt_vsync(int) { return read_processor_time() - deadline; }
static void rtg_vsynccheck() { ++rtg_calls; }
static void maybe_process_pull_audio() { ++audio_calls; now += audio_cost; }
static int target_sleep_nanos(int us) {
    assert(us >= 50 && us <= 1000);
    ++sleeps;
    if (sleep_fails) return -1;
    now += us + overshoot;
    return 0;
}
static int cpu_sleep_millis(int ms) {
    assert(ms == 1);
    return target_sleep_nanos(1000);
}
static void production_wait() {
    const int clockadjust = 0;
"""
        fixture += wait
        fixture += r"""
}
static void reset(int period) {
    now = 0; deadline = period;
    sleeps = audio_calls = rtg_calls = audio_cost = overshoot = 0;
    sleep_fails = false;
    currprefs.turbo_emulation = false;
    cpu_threaded_run = false;
}
int main() {
    for (int period : {16683, 20032}) {
        reset(period);
        production_wait();
        assert(now >= deadline && now <= deadline + 20);
        assert(sleeps >= 10 && audio_calls >= sleeps && rtg_calls >= sleeps);

        reset(period);
        audio_cost = 400;
        production_wait();
        assert(now >= deadline && now <= deadline + 20);

        reset(period);
        overshoot = 900;
        production_wait();
        assert(now >= deadline && now <= deadline + 20);

        reset(period);
        sleep_fails = true;
        production_wait();
        assert(sleeps == 1 && now >= deadline && now <= deadline + 20);

        reset(period);
        currprefs.turbo_emulation = true;
        production_wait();
        assert(sleeps == 0 && audio_calls == 0);

        // A threaded CPU (fastest speed only) paces the frame itself.
        reset(period);
        cpu_threaded_run = true;
        production_wait();
        assert(sleeps == 0 && now == 0);

        reset(period);
        now = deadline + 100;
        production_wait();
        assert(sleeps == 0 && audio_calls == 0 && rtg_calls == 0);
    }
    std::cout << "Production frame pacing tests passed\n";
}
"""
        with tempfile.TemporaryDirectory(prefix="amiberry-frame-pacing-") as directory:
            path = Path(directory)
            cpp = path / "wait.cpp"
            binary = path / "wait"
            cpp.write_text(fixture)
            subprocess.run(
                [os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-fsanitize=undefined", "-fno-sanitize-recover=all",
                 f"-I{ROOT / 'src/osdep'}", str(cpp), "-o", str(binary)],
                check=True,
            )
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
