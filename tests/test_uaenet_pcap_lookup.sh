#!/usr/bin/env bash
set -euo pipefail

# pcap device lookup by name: compiles the production uaenet_enumerate() from
# src/osdep/amiberry_uaenet.cpp against a stub pcap_findalldevs() and checks that
# a lookup keeps the entries of the full list that ethernet_enumerate() cached.

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
    source = (ROOT / "src/osdep/amiberry_uaenet.cpp").read_text(encoding="utf-8")
    enumerate_pcap = between(source, "struct netdriverdata *uaenet_enumerate(const TCHAR *name)", "#endif\n\n// Version that matches")
    fixture = r'''
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <strings.h>
#define WITH_UAENET_PCAP
#define _T(s) s
#define PCAP_ERRBUF_SIZE 256
#define MAX_TOTAL_NET_DEVICES 30
#define UAENET_PCAP 2
#define MAX_MTU 1500
typedef char TCHAR;
typedef unsigned char uae_u8;
struct netdriverdata {
    int type;
    const TCHAR *name;
    const TCHAR *desc;
    int mtu;
    unsigned char mac[6];
    unsigned char originalmac[6];
    int active;
    void *driverdata;
};
struct pcap_if_t { pcap_if_t *next; char *name; char *description; };
static pcap_if_t devs[3] = {
    { &devs[1], (char *)"lo", nullptr },
    { &devs[2], (char *)"eth0", nullptr },
    { nullptr, (char *)"pcaptest", nullptr },
};
int pcap_findalldevs(pcap_if_t **all, char *) { *all = devs; return 0; }
void pcap_freealldevs(pcap_if_t *) {}
TCHAR *my_strdup(const TCHAR *s) { return strdup(s); }
#define _tcsicmp strcasecmp
void write_log(const TCHAR *, ...) {}
static int enumerated;
static struct netdriverdata nd[MAX_TOTAL_NET_DEVICES + 1];
// Interface helpers enumeration may call.
int uaenet_host_mtu(const char *) { return 1500; }
bool uaenet_host_mac(const char *, uae_u8 *) { return false; }
void uaenet_set_guest_mac(struct netdriverdata *) {}
''' + enumerate_pcap + r'''
static int failures;
static void expect(bool ok, const char *what)
{
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        failures++;
    }
}

int main()
{
    // ethernet_enumerate(ndd, 0) keeps pointers to every active entry.
    struct netdriverdata *list = uaenet_enumerate(nullptr);
    struct netdriverdata *cached[3] = { &list[0], &list[1], &list[2] };
    expect(cached[2]->active && !strcmp(cached[2]->name, "pcaptest"), "full list must hold every device");

    // A board or uaenet.device then looks up its device by name.
    struct netdriverdata *found = uaenet_enumerate("pcaptest");
    expect(found == cached[2], "lookup must return the cached entry");
    expect(cached[1]->active && cached[1]->name && !strcmp(cached[1]->name, "eth0"),
        "lookup must not clear the other cached entries");
    expect(cached[2]->active && cached[2]->name && !strcmp(cached[2]->name, "pcaptest"),
        "lookup must not move the cached entry");

    if (!failures)
        std::cout << "uaenet pcap lookup: all tests passed\n";
    return failures ? 1 : 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="amiberry-uaenet-lookup-") as tmp:
        cpp = Path(tmp) / "test.cpp"
        exe = Path(tmp) / ("test.exe" if os.name == "nt" else "test")
        cpp.write_text(fixture, encoding="utf-8")
        subprocess.run(shlex.split(os.environ.get("CXX", "c++"))
                       + ["-std=c++17", "-Wall", "-Wextra", "-Wno-unused-parameter", str(cpp), "-o", str(exe)],
                       check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
PY
