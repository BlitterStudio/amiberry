#!/usr/bin/env python3
"""Compile production CPU diagnostics and bank wrappers without the emulator."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class CpuThreadRegressionTests(unittest.TestCase):
    def compile_and_run(self, name, source):
        with tempfile.TemporaryDirectory(prefix=f"amiberry-cpu-thread-{name}-") as directory:
            work = Path(directory)
            fixture = work / "fixture.cpp"
            executable = work / "fixture"
            fixture.write_text(source, encoding="utf-8")
            command = shlex.split(os.environ.get("CXX", "c++")) + [
                "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pthread",
                "-DAMIBERRY",
                "-I", str(ROOT / "src/include"), str(fixture), "-o", str(executable),
            ]
            result = subprocess.run(command, capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_diagnostics_environment_counters_and_concurrency(self):
        production = (ROOT / "src/cpu_thread.cpp").read_text(encoding="utf-8")
        production = "\n".join(
            line for line in production.splitlines()
            if not line.startswith('#include "')
        )
        self.compile_and_run("diagnostics", r'''
// This header must supply evt_t itself, without events.h/sysdeps.h first.
#include "cpu_thread.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>
struct {
    int cpu_model = 68020;
    bool cpu_cycle_exact = false, cpu_memory_cycle_exact = false;
} currprefs;
bool cpu_threaded_run = true;
std::string report;
void write_log(const char *format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    report += buffer;
}
''' + production + r'''
uae_u64 load(uae_u64& counter) {
    return __atomic_load_n(&counter, __ATOMIC_RELAXED);
}
void check_empty() {
    assert(!load(cpu_thread_diag.wait_count));
    assert(!load(cpu_thread_diag.total_wait_ns));
    assert(!load(cpu_thread_diag.longest_wait_ns));
    assert(!load(cpu_thread_diag.skipped_cycle_count));
    assert(!load(cpu_thread_diag.total_skipped_cycles));
    assert(!load(cpu_thread_diag.longest_skipped_cycles));
}
int main() {
    unsetenv("AMIBERRY_CPU_THREAD_DIAGNOSTICS");
    cpu_thread_diag_init();
    assert(!cpu_thread_diag.enable_diagnostics);
    check_empty();
    for (const char *value : {"0", "false", "no", "off", "", "2", "-1",
                             "TRUE", "true ", "enabled"}) {
        setenv("AMIBERRY_CPU_THREAD_DIAGNOSTICS", value, 1);
        report.clear();
        cpu_thread_diag_init();
        assert(!cpu_thread_diag_enabled());
        bool valid = std::string(value) == "0" || std::string(value) == "false" ||
            std::string(value) == "no" || std::string(value) == "off";
        assert((report.find("Invalid AMIBERRY_CPU_THREAD_DIAGNOSTICS") ==
                std::string::npos) == valid);
        cpu_thread_diag_log_wait(500);
        cpu_thread_diag_log_stall(500);
        check_empty();
        report.clear();
        cpu_thread_diag_print_stats();
        assert(report.empty());
    }
    for (const char *value : {"1", "true", "yes", "on"}) {
        setenv("AMIBERRY_CPU_THREAD_DIAGNOSTICS", value, 1);
        cpu_thread_diag_init();
        assert(cpu_thread_diag_enabled());
        check_empty();
        cpu_thread_diag_log_wait(10);
        cpu_thread_diag_log_stall(20);
        assert(load(cpu_thread_diag.total_wait_ns) == 10);
        assert(load(cpu_thread_diag.total_skipped_cycles) == 20);
    }
    cpu_thread_diag_reset();
    assert(cpu_thread_diag_enabled());
    check_empty();
    cpu_thread_diag_log_stall(0);
    cpu_thread_diag_log_stall(-1);
    check_empty();
    constexpr uae_u64 large = 1ULL << 34;
    constexpr unsigned workers = 8, iterations = 10000;
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < workers; ++i) {
        threads.emplace_back([i]() {
            for (unsigned j = 0; j < iterations; ++j) {
                cpu_thread_diag_log_wait(large + i);
                cpu_thread_diag_log_stall(large + i + 10);
            }
        });
    }
    for (auto& thread : threads)
        thread.join();
    const uae_u64 count = workers * iterations;
    const uae_u64 sum = iterations * (workers * large + workers * (workers - 1) / 2);
    assert(load(cpu_thread_diag.wait_count) == count);
    assert(load(cpu_thread_diag.total_wait_ns) == sum);
    assert(load(cpu_thread_diag.longest_wait_ns) == large + workers - 1);
    assert(load(cpu_thread_diag.skipped_cycle_count) == count);
    assert(load(cpu_thread_diag.total_skipped_cycles) == sum + count * 10);
    assert(load(cpu_thread_diag.longest_skipped_cycles) == large + workers + 9);
    report.clear();
    cpu_thread_diag_print_stats();
    assert(report.find(std::to_string(sum) + " ns") != std::string::npos);
    assert(report.find("Mailbox waits (elapsed host time)") != std::string::npos);
    assert(report.find("Skipped guest cycles (not host wait time)") != std::string::npos);
    // Reset remains race-free when concurrent; a coherent snapshot requires idle.
    std::thread logger([]() {
        for (unsigned i = 0; i < iterations; ++i) {
            cpu_thread_diag_log_wait(i);
            cpu_thread_diag_log_stall(i);
        }
    });
    for (unsigned i = 0; i < iterations; ++i)
        cpu_thread_diag_reset();
    logger.join();
    cpu_thread_diag_reset();
    check_empty();
    unsetenv("AMIBERRY_CPU_THREAD_DIAGNOSTICS");
    cpu_thread_diag_init();
    assert(!cpu_thread_diag_enabled());
}
''')

    def test_wrapper_capacity_identity_and_remap_refresh(self):
        production = (ROOT / "src/memory.cpp").read_text(encoding="utf-8")
        start = production.index("struct addrbank_thread {")
        end = production.index("\nstatic void set_memory_cacheable", start)
        self.compile_and_run("banks", r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
using uae_u32 = uint32_t;
using uaecptr = uint32_t;
#define REGPARAM2
constexpr unsigned MEMORY_BANKS = 65536, ABFLAG_THREADSAFE = 1;
constexpr int S_READ = 2, S_WRITE = 4;
using read_handler = uae_u32 (*)(uaecptr);
using write_handler = void (*)(uaecptr, uae_u32);
struct addrbank {
    read_handler lget, wget, bget, lgeti, wgeti;
    write_handler lput, wput, bput;
    unsigned flags, mask, start, reserved_size, allocated_size;
    int jit_read_flag, jit_write_flag;
    void *baseaddr;
    const char *name;
};
addrbank dummy_bank{};
#define xcalloc(type, count) static_cast<type*>(std::calloc(count, sizeof(type)))
void process_cpu_indirect_memory_write(uaecptr, uae_u32, int) {}
uae_u32 process_cpu_indirect_memory_read(uaecptr, int size) { return size; }
addrbank *mem_banks[MEMORY_BANKS];
int icache_flushes, ce_fills;
void flush_icache(int) { ++icache_flushes; }
void fill_ce_banks() { ++ce_fills; }
''' + production[start:end] + r'''
uae_u32 instruction_a(uaecptr) { return 1; }
uae_u32 instruction_b(uaecptr) { return 2; }
int main() {
    assert(get_bank_cpu_thread(&dummy_bank) == &dummy_bank);
    addrbank safe{};
    safe.flags = ABFLAG_THREADSAFE;
    assert(get_bank_cpu_thread(&safe) == &safe);
    assert(thread_banks.empty());
    std::vector<addrbank> originals(1024);
    std::vector<addrbank*> wrappers;
    for (auto& bank : originals) {
        bank.lgeti = bank.wgeti = instruction_a;
        auto *wrapped = get_bank_cpu_thread(&bank);
        assert(wrapped != &bank);
        wrappers.push_back(wrapped);
    }
    assert(thread_banks.size() == originals.size());
    for (unsigned pass = 0; pass < 3; ++pass) {
        for (unsigned i = 0; i < originals.size(); ++i) {
            auto& bank = originals[i];
            bank.flags = 0x40;
            bank.mask = i + pass;
            bank.start = i * 65536;
            bank.reserved_size = 100 + pass;
            bank.allocated_size = 200 + pass;
            bank.baseaddr = &bank;
            bank.name = "remapped";
            bank.lget = bank.wget = bank.bget = instruction_a;
            bank.lgeti = bank.wgeti = instruction_b;
            bank.jit_read_flag = bank.jit_write_flag = 99;
            auto *wrapped = get_bank_cpu_thread(&bank);
            assert(wrapped == wrappers[i]);
            assert(wrapped->mask == bank.mask && wrapped->start == bank.start);
            assert(wrapped->reserved_size == bank.reserved_size);
            assert(wrapped->allocated_size == bank.allocated_size);
            assert(wrapped->baseaddr == bank.baseaddr && wrapped->name == bank.name);
            assert(wrapped->flags == bank.flags);
            assert(wrapped->jit_read_flag == S_READ && wrapped->jit_write_flag == S_WRITE);
            assert(wrapped->lget == threadcpu_lget && wrapped->wget == threadcpu_wget);
            assert(wrapped->bget == threadcpu_bget && wrapped->lput == threadcpu_lput);
            assert(wrapped->wput == threadcpu_wput && wrapped->bput == threadcpu_bput);
            assert(wrapped->lgeti == instruction_b && wrapped->wgeti == instruction_b);
        }
    }
    assert(thread_banks.size() == originals.size());
    originals[0].flags |= ABFLAG_THREADSAFE;
    assert(get_bank_cpu_thread(&originals[0]) == &originals[0]);
    originals[0].flags &= ~ABFLAG_THREADSAFE;
    assert(get_bank_cpu_thread(&originals[0]) == wrappers[0]);
    assert(thread_banks.size() == originals.size());
    // Selecting the CPU thread swaps the wrappers into the map, and back.
    thread_mem_banks[0] = &dummy_bank;
    thread_mem_banks[1] = &safe;
    thread_mem_banks[2] = &originals[1];
    mem_banks[0] = &dummy_bank;
    mem_banks[1] = &safe;
    mem_banks[2] = &originals[1];
    memory_set_cpu_thread_banks(true);
    assert(mem_banks[0] == &dummy_bank && mem_banks[1] == &safe);
    assert(mem_banks[2] == wrappers[1] && !mem_banks[3]);
    assert(icache_flushes == 1 && ce_fills == 1);
    memory_set_cpu_thread_banks(false);
    assert(mem_banks[0] == &dummy_bank && mem_banks[1] == &safe);
    assert(mem_banks[2] == &originals[1] && !mem_banks[3]);
    assert(thread_banks.size() == originals.size());
    // Production intentionally retains wrappers while mappings can reference them.
    for (auto *wrapper : thread_banks)
        std::free(wrapper);
}
''')


if __name__ == "__main__":
    unittest.main()
