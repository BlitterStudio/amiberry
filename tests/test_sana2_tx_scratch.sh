#!/usr/bin/env bash
set -euo pipefail

# SANA-II transmit copying when guest callbacks re-enter: compiles the production
# createwritepacket()/handleread() from src/sana2.cpp with a CopyFromBuff() that
# receives a packet, or starts a second write, while it copies.

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
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(".")  # run_tests.sh runs from the repository root


def between(source, start, end):
    begin = source.index(start)
    return source[begin:source.index(end, begin)]


def main():
    source = (ROOT / "src/sana2.cpp").read_text(encoding="utf-8")
    packet = between(source, "struct s2packet {", "static int uaenet_int_late;")
    callbacks = between(source, "static uae_u32 copytobuff ", "static uae_u32 packetfilter ")
    addresses = between(source, "static int isbroadcast ", "static uae_u64 amigaaddrto64")
    receive = between(source, "static int handleread ", "static void uaenet_gotdata ")
    transmit = between(source, "static struct s2packet *createwritepacket(", "static int uaenet_getdata(")
    fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>
using uae_u8 = uint8_t;
using uae_u16 = uint16_t;
using uae_u32 = uint32_t;
using uae_u64 = uint64_t;
using uaecptr = uint32_t;
#define _T(s) s
#define xcalloc(type, n) static_cast<type *>(std::calloc(n, sizeof(type)))
#define xmalloc(type, n) static_cast<type *>(std::malloc((n) * sizeof(type)))
constexpr int ADDR_SIZE = 6, ETH_HEADER_SIZE = 14, SANA2_MAX_ADDR_BYTES = 16;
constexpr int SANA2IOF_RAW = 128, SANA2IOF_BCAST = 64, SANA2IOF_MCAST = 32;
constexpr int CMD_READ = 2, S2WERR_BUFF_ERROR = 6, S2ERR_NO_RESOURCES = 1;
struct TrapContext { uaecptr a[8]{}; uae_u32 d[8]{}; };
struct netdriverdata { int mtu = 1500; uae_u8 mac[6]{2, 3, 4, 5, 6, 7}; } driver;
// Same fields as the production struct that the extracted code uses. tempbuf
// is the guest scratch dev_open_2() allocates.
struct priv_s2devstruct {
    uaecptr tempbuf = 256, copyfrombuff = 1, copytobuff = 2, packetfilter = 0;
    bool txbusy = false;
    netdriverdata *td = &driver;
    bool tracks[65536]{};
    unsigned packetssent = 0, bytessent = 0, packetsreceived = 0, bytesreceived = 0;
} device;
struct mcast { mcast *next; uae_u64 start, end; };
struct s2devstruct { mcast *mc = nullptr; } unit;
static uae_u8 ram[65536];
static bool reject_copy, missing_device;
static int failures;
static std::function<void()> during_copy;
static int log_net;
void write_log(const char *, ...) {}
const char *dumphead(const uae_u8 *, int) { return ""; }
uae_u8 get_byte_host(const uae_u8 *p) { return *p; }
void put_byte_host(uae_u8 *p, uae_u8 v) { *p = v; }
uae_u32 get_long_host(const uae_u8 *p) { return (uae_u32(p[0]) << 24) | (uae_u32(p[1]) << 16) | (uae_u32(p[2]) << 8) | p[3]; }
void put_long_host(uae_u8 *p, uae_u32 v) { for (int i = 3; i >= 0; --i) { p[i] = v; v >>= 8; } }
priv_s2devstruct *getps2devstruct(TrapContext *, uae_u8 *, uaecptr) { return missing_device ? nullptr : &device; }
void trap_call_add_areg(TrapContext *c, int r, uaecptr v) { c->a[r] = v; }
void trap_call_add_dreg(TrapContext *c, int r, uae_u32 v) { c->d[r] = v; }
void trap_get_bytes(TrapContext *, void *to, uaecptr from, int n) { assert(n >= 0 && from + unsigned(n) <= sizeof ram); std::memcpy(to, ram + from, n); }
void trap_put_bytes(TrapContext *, const void *from, uaecptr to, int n) { assert(n >= 0 && to + unsigned(n) <= sizeof ram); std::memcpy(ram + to, from, n); }
// The guest CopyFromBuff/CopyToBuff callbacks. Guest code can run other traps
// after copying; during_copy stands in for them.
uae_u32 trap_call_func(TrapContext *c, uaecptr func) {
    if (func == 1 && reject_copy)
        return 0;
    std::memmove(ram + c->a[0], ram + c->a[1], c->d[0]);
    if (func == 1 && during_copy) {
        auto run = std::move(during_copy);
        during_copy = nullptr;
        run();
    }
    return 1;
}
uae_u32 packetfilter(TrapContext *, uaecptr, uaecptr, uaecptr) { return 1; }
''' + packet + callbacks + addresses + receive + transmit + r'''
void expect(bool condition, const char *message) { if (!condition) { std::cerr << message << '\n'; ++failures; } }
std::vector<uae_u8> payload(unsigned n, uae_u8 salt) { std::vector<uae_u8> v(n); for (unsigned i = 0; i < n; ++i) v[i] = uae_u8(i * 17 + salt); return v; }
void request(uae_u8 *r, uaecptr data, unsigned n, bool raw) {
    std::memset(r, 0, 88);
    r[30] = raw ? SANA2IOF_RAW : 0;
    put_long_host(r + 36, 0x0800);
    put_long_host(r + 72, n);
    put_long_host(r + 76, data);
    for (int i = 0; i < 6; ++i)
        r[56 + i] = uae_u8(10 + i);
}
void check_packet(s2packet *p, const std::vector<uae_u8> &data, bool raw) {
    expect(p != nullptr, "write must produce a packet");
    if (!p)
        return;
    const unsigned offset = raw ? 0 : ETH_HEADER_SIZE;
    expect(p->len == int(data.size() + offset), "packet length changed");
    expect(std::memcmp(p->data + offset, data.data(), data.size()) == 0, "transmit bytes overwritten during CopyFromBuff");
    std::free(p->data);
    std::free(p);
}
void receive_packet() {
    TrapContext rx;
    uae_u8 r[88];
    auto incoming = payload(167, 7);
    incoming[0] = 2;
    incoming[12] = 8;
    incoming[13] = 0;
    request(r, 8192, incoming.size(), true);
    expect(handleread(&rx, &device, &unit, r, 9000, incoming.data(), incoming.size(), CMD_READ) == 1, "receive rejected");
}
void reset() { reject_copy = missing_device = false; during_copy = nullptr; device.txbusy = false; std::memset(ram, 0, sizeof ram); }
int main() {
    for (bool raw : { false, true }) {
        TrapContext ctx;
        uae_u8 r[88];
        auto data = payload(raw ? 1500 : 1400, 23);

        // A packet received while CopyFromBuff runs must not end up in the write.
        reset();
        std::memcpy(ram + 4096, data.data(), data.size());
        request(r, 4096, data.size(), raw);
        during_copy = receive_packet;
        check_packet(createwritepacket(&ctx, r, 9000), data, raw);

        // A second write on the same opener while the first one copies is
        // refused, and the first one stays intact.
        reset();
        std::memcpy(ram + 4096, data.data(), data.size());
        request(r, 4096, data.size(), raw);
        s2packet *nested_result = reinterpret_cast<s2packet *>(1);
        during_copy = [&] {
            TrapContext nested;
            uae_u8 other[88];
            auto second = payload(96, 99);
            std::memcpy(ram + 6144, second.data(), second.size());
            request(other, 6144, second.size(), raw);
            nested_result = createwritepacket(&nested, other, 9100);
        };
        check_packet(createwritepacket(&ctx, r, 9000), data, raw);
        expect(nested_result == nullptr, "overlapping write must be refused");
        expect(!device.txbusy, "transmit scratch must be released after a write");

        // A rejected copy releases the transmit scratch for the next write.
        reset();
        std::memcpy(ram + 4096, data.data(), data.size());
        request(r, 4096, data.size(), raw);
        reject_copy = true;
        expect(createwritepacket(&ctx, r, 9000) == nullptr, "CopyFromBuff rejection must fail the write");
        expect(!device.txbusy, "rejected copy must release the transmit scratch");
        reject_copy = false;
        check_packet(createwritepacket(&ctx, r, 9000), data, raw);

        reset();
        missing_device = true;
        expect(createwritepacket(&ctx, r, 9000) == nullptr, "unknown opener must fail");
    }
    if (!failures)
        std::cout << "SANA-II transmit scratch: all tests passed\n";
    return failures ? 1 : 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="amiberry-sana2-tx-") as tmp:
        cpp = Path(tmp) / "test.cpp"
        exe = Path(tmp) / ("test.exe" if os.name == "nt" else "test")
        cpp.write_text(fixture, encoding="utf-8")
        subprocess.run(shlex.split(os.environ.get("CXX", "c++"))
                       + ["-std=c++17", "-Wall", "-Wextra", "-Wno-unused-variable",
                          "-Wno-unused-parameter", "-Wno-unused-function", str(cpp), "-o", str(exe)],
                       check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
PY
