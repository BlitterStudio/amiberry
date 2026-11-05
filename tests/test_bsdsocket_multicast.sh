#!/usr/bin/env bash
set -euo pipefail

# bsdsocket IPv4 multicast socket options: compiles the production option
# mapping from src/osdep/bsdsocket_host.cpp (POSIX part) and checks the bytes
# that reach the host for struct ip_mreq, struct in_addr and the u_char/int
# TTL and loop flags, in both directions.

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
    source = (ROOT / "src/osdep/bsdsocket_host.cpp").read_text(encoding="utf-8")
    posix = source.split("#else /* !_WIN32 */", 1)[1]
    options = between(posix, "static int mapsockoptname", "STATIC_INLINE int bsd_amigaside_FD_ISSET")
    # The value mapping takes the option length once it handles multicast options.
    with_len = "void *buf, uae_u32 len)" in options
    fixture = r'''
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
using uae_u8 = uint8_t;
using uae_u32 = uint32_t;
static uae_u8 ram[256];
uae_u32 get_long(uae_u32 a) { return uae_u32(ram[a]) << 24 | uae_u32(ram[a + 1]) << 16 | uae_u32(ram[a + 2]) << 8 | ram[a + 3]; }
void put_long(uae_u32 a, uae_u32 v) { for (int i = 3; i >= 0; i--, v >>= 8) ram[a + i] = uae_u8(v); }
uae_u8 get_byte(uae_u32 a) { return ram[a]; }
void put_byte(uae_u32 a, uae_u8 v) { ram[a] = v; }
int mapErrno(int e) { return e; }
void write_log(const char *, ...) {}
#define BSDLOG(...) ((void)0)
''' + options + r'''
#if WITH_LEN
#define MAP_VALUE(level, name, optval, buf, len) mapsockoptvalue(level, name, optval, buf, len)
#define MAP_RETURN(level, name, optval, buf, len) mapsockoptreturn(level, name, optval, buf, len)
#else
#define MAP_VALUE(level, name, optval, buf, len) mapsockoptvalue(level, name, optval, buf)
#define MAP_RETURN(level, name, optval, buf, len) mapsockoptreturn(level, name, optval, buf)
#endif

static int failures;
static void expect(bool ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "FAIL: %s\n", what);
        failures++;
    }
}

int main()
{
    // Amiga option 13 is IP_DROP_MEMBERSHIP.
    expect(mapsockoptname(IPPROTO_IP, 13) == IP_DROP_MEMBERSHIP, "IP_DROP_MEMBERSHIP is mapped");

    expect(setsockopt_argument_size(IPPROTO_IP, IP_ADD_MEMBERSHIP) == 8, "ip_mreq is 8 bytes");
    expect(setsockopt_argument_size(IPPROTO_IP, IP_DROP_MEMBERSHIP) == 8, "ip_mreq is 8 bytes when dropping");
    expect(setsockopt_argument_size(IPPROTO_IP, IP_MULTICAST_IF) == 4, "in_addr is 4 bytes");
    expect(setsockopt_argument_size(IPPROTO_IP, IP_MULTICAST_TTL) == 1, "a u_char TTL is accepted");
    expect(setsockopt_argument_size(IPPROTO_IP, IP_MULTICAST_LOOP) == 1, "a u_char loop flag is accepted");
    expect(getsockopt_result_size(IPPROTO_IP, IP_ADD_MEMBERSHIP) == -1, "memberships can't be read back");
    expect(getsockopt_result_size(IPPROTO_IP, IP_MULTICAST_TTL) == 1, "a u_char TTL can be read");

    // struct ip_mreq: group 224.0.0.251 on interface 192.0.2.7, as the guest stores it.
    const uae_u8 mreq[8] = { 224, 0, 0, 251, 192, 0, 2, 7 };
    memcpy(ram + 16, mreq, sizeof mreq);
    struct ip_mreq native {};
    MAP_VALUE(IPPROTO_IP, IP_ADD_MEMBERSHIP, 16, &native, 8);
    expect(native.imr_multiaddr.s_addr == inet_addr("224.0.0.251"), "membership group reaches the host unchanged");
    expect(native.imr_interface.s_addr == inet_addr("192.0.2.7"), "membership interface reaches the host unchanged");
    native = {};
    MAP_VALUE(IPPROTO_IP, IP_DROP_MEMBERSHIP, 16, &native, 8);
    expect(native.imr_multiaddr.s_addr == inet_addr("224.0.0.251"), "dropped group reaches the host unchanged");

    struct in_addr ifaddr {};
    MAP_VALUE(IPPROTO_IP, IP_MULTICAST_IF, 20, &ifaddr, 4);
    expect(ifaddr.s_addr == inet_addr("192.0.2.7"), "multicast interface reaches the host unchanged");

    // TTL as a u_char, then as a 32-bit big-endian int.
    alignas(int) uae_u8 value[4] = {};
    ram[32] = 255;
    MAP_VALUE(IPPROTO_IP, IP_MULTICAST_TTL, 32, value, 1);
    expect(value[0] == 255, "u_char TTL reaches the host");
    put_long(36, 5);
    memset(value, 0, sizeof value);
    MAP_VALUE(IPPROTO_IP, IP_MULTICAST_TTL, 36, value, 4);
    expect(*(int *)value == 5, "int TTL reaches the host");

    // Results back to the guest: in_addr bytes, u_char and int flags; nothing past them.
    memset(ram + 48, 0xaa, 16);
    MAP_RETURN(IPPROTO_IP, IP_MULTICAST_IF, 48, &ifaddr, 4);
    expect(!memcmp(ram + 48, mreq + 4, 4) && ram[52] == 0xaa, "multicast interface returns as in_addr");
    uae_u8 loop = 1;
    MAP_RETURN(IPPROTO_IP, IP_MULTICAST_LOOP, 56, &loop, 1);
    expect(ram[56] == 1 && ram[57] == 0xaa, "u_char loop flag returns as one byte");
    int ttl = 64;
    MAP_RETURN(IPPROTO_IP, IP_MULTICAST_TTL, 60, &ttl, 4);
    expect(get_long(60) == 64, "int TTL returns as a 32-bit value");

    // Plain integer options are unchanged.
    put_long(64, 32);
    int ipttl = 0;
    MAP_VALUE(IPPROTO_IP, IP_TTL, 64, &ipttl, 4);
    expect(ipttl == 32, "IP_TTL is still a 32-bit value");

    if (!failures)
        printf("bsdsocket multicast options: all tests passed\n");
    return failures ? 1 : 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="amiberry-bsdsocket-mcast-") as tmp:
        cpp = Path(tmp) / "test.cpp"
        exe = Path(tmp) / "test"
        cpp.write_text(fixture, encoding="utf-8")
        subprocess.run(shlex.split(os.environ.get("CXX", "c++"))
                       + ["-std=c++17", "-Wall", "-Wextra", "-Wno-unused-parameter",
                          "-Wno-unused-function", "-DWITH_LEN=%d" % with_len, str(cpp), "-o", str(exe)],
                       check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
PY
