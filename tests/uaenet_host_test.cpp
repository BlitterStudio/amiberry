#include <cstdint>
#include <cstdio>
#include <cstring>

#include "uaenet_host.h"

static int failures;

static void expect(bool condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		failures++;
	}
}

static bool mac_is(const uint8_t mac[6], uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
{
	const uint8_t expected[6] = { a, b, c, d, e, f };
	return memcmp(mac, expected, 6) == 0;
}

int main()
{
	const uint8_t host[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
	uint8_t guest[6];

	uaenet_guest_mac(host, 0, guest);
	expect(mac_is(guest, 0xaa, 0x82, 0x8a, 0x33, 0x44, 0x55), "guest MAC is aa:82:8a plus the low host bytes");
	expect((guest[0] & 0x01) == 0, "guest MAC is unicast");
	expect((guest[0] & 0x02) != 0, "guest MAC is locally administered");

	uaenet_guest_mac(host, 1, guest);
	expect(mac_is(guest, 0xaa, 0x82, 0x8a, 0x33, 0x44, 0x56), "the instance number is added to the low bytes");

	const uint8_t top[6] = { 0x00, 0x11, 0x22, 0xff, 0xff, 0xff };
	uaenet_guest_mac(top, 1, guest);
	expect(mac_is(guest, 0xaa, 0x82, 0x8a, 0x00, 0x00, 0x00), "the offset wraps within the low three bytes");

	uaenet_guest_mac(nullptr, 2, guest);
	expect(mac_is(guest, 0xaa, 0x82, 0x8a, 0x00, 0x00, 0x02), "an unknown host MAC uses zeros plus the instance");

	uint8_t mac[6];
	expect(!uaenet_host_mac("no-such-interface0", mac), "unknown interface has no MAC");
	expect(uaenet_host_mtu("no-such-interface0") < 0, "unknown interface has no MTU");
#ifdef __linux__
	expect(uaenet_host_mtu("lo") > 0, "loopback has an MTU");
#else
	expect(uaenet_host_mtu("lo0") > 0, "loopback has an MTU");
#endif

	if (failures)
		return 1;
	printf("uaenet_host_test: all tests passed\n");
	return 0;
}
