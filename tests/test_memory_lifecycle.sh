#!/usr/bin/env bash
set -euo pipefail

if command -v python3 >/dev/null 2>&1; then
	PYTHON=python3
elif command -v python >/dev/null 2>&1; then
	PYTHON=python
elif command -v py >/dev/null 2>&1; then
	PYTHON="py -3"
else
	echo "python3, python, or py is required" >&2
	exit 1
fi

$PYTHON - <<'PY'
import os
import re
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

devices = Path("src/devices.cpp").read_text()
memory = Path("src/osdep/amiberry_mem.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


def between(source: str, start: str, end: str) -> str:
	match = re.search(start, source)
	if not match:
		raise ValueError(start)
	return source[match.start():source.index(end, match.start())]


try:
	start = devices.index("void virtualdevice_free(void)")
	end = devices.index("void do_leave_program", start)
except ValueError as exc:
	fail(f"Could not find virtual-device shutdown code: {exc}")

shutdown = devices[start:end]
rtarea_free = shutdown.find("rtarea_free();")
free_shm = shutdown.find("free_shm();")
if rtarea_free < 0 or free_shm < 0:
	fail("Virtual-device shutdown must release rtarea and shared memory")
if rtarea_free > free_shm:
	fail("rtarea must be released before tracked shared-memory allocations")

try:
	release = between(memory, re.escape("static size_t page_round("), "static int find_shmid_by_address(")
	next_key = between(memory, re.escape("static uae_key_t get_next_shmkey ()"), "STATIC_INLINE uae_key_t find_shmkey")
	clear = between(memory, r"static void clear_shm\s*\(", "bool preinit_shm ()")
	lifecycle = between(memory, re.escape("static uae_u32 oz3fastmem_size"), "void mapped_free (")
	allocation = between(memory, re.escape("bool uae_mman_alloc_nodirect("), "void *uae_shmat")
except ValueError as exc:
	fail(f"Could not find shared-memory lifecycle code: {exc}")

if "shm_heapowners[shmid] = ab;" not in allocation:
	fail("Non-direct allocations must retain their owning bank")

# Production registry cleanup plus init_shm/free_shm, with host allocation
# and natmem setup stubbed out. The scenario is a memory-size change followed
# by a reset: rtarea is allocated once by virtualdevice_init() and must survive
# it, while every other non-direct block is released and reallocated.
fixture = r'''
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
using uae_u8 = uint8_t;
using uae_u32 = uint32_t;
using uae_key_t = int;
using DWORD = unsigned long;
#define MAX_SHMID 256
#define MAX_RAM_BOARDS 4
#define MAX_RTG_BOARDS 4
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define ABFLAG_MAPPED 0x01
#define ABFLAG_DIRECTMAP 0x02
#define xfree(p) std::free(p)
struct addrbank {
	uae_u8* baseaddr = nullptr;
	uae_u32 flags = 0, allocated_size = 0;
};
struct uae_shmid_ds {
	uae_key_t key;
	uae_u32 size, rosize;
	void* attached;
	int mode;
};
struct ramboard { uae_u32 size; };
struct rtgboardconfig { uae_u32 rtgmem_size; int rtgmem_type; };
struct uae_prefs {
	ramboard fastmem[MAX_RAM_BOARDS]{}, z3fastmem[MAX_RAM_BOARDS]{}, z3chipmem{};
	rtgboardconfig rtgboards[MAX_RTG_BOARDS]{};
} changed_prefs;
struct shmpiece;
static shmpiece* shm_start;
static uae_shmid_ds shmids[MAX_SHMID];
static size_t shm_allocsizes[MAX_SHMID];
static void* shm_heapallocs[MAX_SHMID];
static addrbank* shm_heapowners[MAX_SHMID];
addrbank rtarea_bank, kickmem_bank;
static size_t uae_vm_page_size() { return 4096; }
static bool VirtualProtect(void*, size_t, DWORD, DWORD*) { return true; }
''' + release + next_key + clear + r'''
static int doinit_shm() { return 0; }
static void resetmem(bool) {}
static void memory_hardreset(int) {}
''' + lifecycle + r'''
static int track(addrbank& bank)
{
	const int shmid = get_next_shmkey();
	void* block = std::calloc(1, 4096);
	shmids[shmid].attached = block;
	shmids[shmid].mode = PAGE_READONLY;
	shm_allocsizes[shmid] = 4096;
	shm_heapallocs[shmid] = block;
	shm_heapowners[shmid] = &bank;
	bank.baseaddr = static_cast<uae_u8*>(block);
	bank.flags = ABFLAG_MAPPED;
	bank.allocated_size = 4096;
	return shmid;
}

int main()
{
	int failures = 0;
	auto expect = [&](bool ok, const char* message) {
		if (!ok) { std::cerr << message << '\n'; ++failures; }
	};
	for (int i = 0; i < MAX_SHMID; i++)
		clear_shmid(i);

	const int rtarea_shmid = track(rtarea_bank);
	track(kickmem_bank);
	uae_u8* const rtarea = rtarea_bank.baseaddr;
	rtarea[0] = 0x5a;

	changed_prefs.z3fastmem[0].size = 64 * 1024 * 1024;
	expect(init_shm(), "Memory reconfiguration must succeed");
	expect(rtarea_bank.baseaddr == rtarea && rtarea[0] == 0x5a,
		"Reset reconfiguration must keep the UAE Boot ROM allocation and contents");
	expect(rtarea_bank.allocated_size == 4096 && (rtarea_bank.flags & ABFLAG_MAPPED),
		"Reset reconfiguration must keep the UAE Boot ROM bank mapped");
	expect(kickmem_bank.baseaddr == nullptr && kickmem_bank.allocated_size == 0 && kickmem_bank.flags == 0,
		"Reset reconfiguration must release and invalidate other non-direct banks");

	const int kick_shmid = track(kickmem_bank);
	expect(kick_shmid != rtarea_shmid, "A reallocated bank must not reuse the kept UAE Boot ROM slot");

	free_shm();
	expect(rtarea_bank.baseaddr == nullptr && rtarea_bank.allocated_size == 0,
		"Final cleanup must release the UAE Boot ROM");
	expect(kickmem_bank.baseaddr == nullptr && kickmem_bank.allocated_size == 0,
		"Final cleanup must release every non-direct bank");
	bool registry_empty = true;
	for (int i = 0; i < MAX_SHMID; i++)
		registry_empty = registry_empty && shmids[i].key == -1 && !shm_heapowners[i];
	expect(registry_empty, "Final cleanup must empty the shared-memory registry");
	return failures ? 1 : 0;
}
'''

with tempfile.TemporaryDirectory(prefix="amiberry-shm-") as tmp:
	cpp = Path(tmp) / "test.cpp"
	exe = Path(tmp) / ("test.exe" if os.name == "nt" else "test")
	cpp.write_text(fixture, encoding="utf-8")
	cxx = shlex.split(os.environ.get("CXX", "c++"))
	build = subprocess.run(cxx + ["-std=c++17", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)])
	if build.returncode:
		fail("Shared-memory lifecycle harness failed to compile")
	if subprocess.run([str(exe)]).returncode:
		fail("Shared-memory lifecycle harness failed")
PY
