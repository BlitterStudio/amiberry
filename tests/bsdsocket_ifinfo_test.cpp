#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdio>
#include <cstring>

#include "bsdsocket_ifinfo.h"

using namespace bsdsock_ifinfo;

static int failures;

static void expect(bool condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		failures++;
	}
}

static sockaddr_in ipv4(const char *text)
{
	sockaddr_in sa{};
	sa.sin_family = AF_INET;
	inet_pton(AF_INET, text, &sa.sin_addr);
	return sa;
}

/* Amiga struct sockaddr_in bytes for a dotted address. */
static bool is_sockaddr(const uint8_t *p, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	const uint8_t want[value_size] = { 16, 2, 0, 0, a, b, c, d };
	return memcmp(p, want, value_size) == 0;
}

/*
 * The host list the tests query, in getifaddrs() order:
 * lo 127.0.0.1/8, eth0 192.168.1.20/24 (broadcast .255), wlan0 with only an
 * IPv6 address, ppp0 10.0.0.1 -> 10.0.0.2.
 */
struct host_list {
	sockaddr_in lo_addr = ipv4("127.0.0.1"), lo_mask = ipv4("255.0.0.0");
	sockaddr_in eth_addr = ipv4("192.168.1.20"), eth_mask = ipv4("255.255.255.0"), eth_bcast = ipv4("192.168.1.255");
	sockaddr_in6 wlan_addr{};
	sockaddr_in ppp_addr = ipv4("10.0.0.1"), ppp_mask = ipv4("255.255.255.255"), ppp_peer = ipv4("10.0.0.2");
	ifaddrs lo{}, eth{}, wlan{}, ppp{};

	host_list()
	{
		char *names[] = { (char *)"lo", (char *)"eth0", (char *)"wlan0", (char *)"ppp0" };
		lo.ifa_name = names[0];
		lo.ifa_flags = IFF_UP | IFF_LOOPBACK | IFF_RUNNING;
		lo.ifa_addr = (sockaddr *)&lo_addr;
		lo.ifa_netmask = (sockaddr *)&lo_mask;
		lo.ifa_next = &eth;

		eth.ifa_name = names[1];
		eth.ifa_flags = IFF_UP | IFF_BROADCAST | IFF_RUNNING | IFF_MULTICAST;
		eth.ifa_addr = (sockaddr *)&eth_addr;
		eth.ifa_netmask = (sockaddr *)&eth_mask;
		eth.ifa_broadaddr = (sockaddr *)&eth_bcast;
		eth.ifa_next = &wlan;

		wlan_addr.sin6_family = AF_INET6;
		wlan.ifa_name = names[2];
		wlan.ifa_flags = IFF_UP | IFF_BROADCAST | IFF_RUNNING | IFF_MULTICAST;
		wlan.ifa_addr = (sockaddr *)&wlan_addr;
		wlan.ifa_next = &ppp;

		ppp.ifa_name = names[3];
		ppp.ifa_flags = IFF_UP | IFF_POINTOPOINT | IFF_RUNNING;
		ppp.ifa_addr = (sockaddr *)&ppp_addr;
		ppp.ifa_netmask = (sockaddr *)&ppp_mask;
		ppp.ifa_dstaddr = (sockaddr *)&ppp_peer;
	}
};

static void test_decode()
{
	expect(decode(0xc0206921) == request::addr, "SIOCGIFADDR with sizeof(struct ifreq)");
	expect(decode(0xc0106921) == request::addr, "SIOCGIFADDR with a 16-byte length");
	expect(decode(0xc0206922) == request::dstaddr, "SIOCGIFDSTADDR");
	expect(decode(0xc0206923) == request::brdaddr, "SIOCGIFBRDADDR");
	expect(decode(0xc0206925) == request::netmask, "SIOCGIFNETMASK");
	expect(decode(0xc0206911) == request::flags, "SIOCGIFFLAGS");
	expect(decode(0xc0086924) == request::conf, "SIOCGIFCONF");
	expect(decode(0x8004667e) == request::none, "FIONBIO is not an interface request");
	expect(decode(0x8020690c) == request::none, "SIOCSIFADDR is not answered");
}

static void test_query()
{
	host_list hosts;
	uint8_t out[value_size];

	expect(query(&hosts.lo, "eth0", request::flags, out) == 0, "eth0 flags");
	expect(out[0] == 0x80 && out[1] == 0x43, "eth0 flags are UP|BROADCAST|RUNNING|MULTICAST, big-endian");

	expect(query(&hosts.lo, "eth0", request::addr, out) == 0 && is_sockaddr(out, 192, 168, 1, 20), "eth0 address");
	expect(query(&hosts.lo, "eth0", request::netmask, out) == 0 && is_sockaddr(out, 255, 255, 255, 0), "eth0 netmask");
	expect(query(&hosts.lo, "eth0", request::brdaddr, out) == 0 && is_sockaddr(out, 192, 168, 1, 255), "eth0 broadcast");
	expect(query(&hosts.lo, "eth0", request::dstaddr, out) == amiga_EINVAL, "eth0 has no point-to-point peer");

	expect(query(&hosts.lo, "ppp0", request::dstaddr, out) == 0 && is_sockaddr(out, 10, 0, 0, 2), "ppp0 peer");
	expect(query(&hosts.lo, "ppp0", request::brdaddr, out) == amiga_EINVAL, "ppp0 has no broadcast address");

	expect(query(&hosts.lo, "wlan0", request::flags, out) == 0 && out[1] == 0x43, "wlan0 flags without an IPv4 address");
	expect(query(&hosts.lo, "wlan0", request::addr, out) == amiga_EADDRNOTAVAIL, "wlan0 has no IPv4 address");
	expect(query(&hosts.lo, "eth1", request::flags, out) == amiga_ENXIO, "unknown interface");
}

static void test_conf()
{
	host_list hosts;
	uint8_t buf[4 * ifreq_size];

	expect(conf_size(&hosts.lo) == 3 * ifreq_size, "three IPv4 addresses need three records");

	memset(buf, 0xee, sizeof(buf));
	const size_t used = conf(&hosts.lo, buf, 2 * ifreq_size + 10);
	expect(used == 2 * ifreq_size, "only whole records fit");
	expect(strcmp((const char *)buf, "lo") == 0 && is_sockaddr(buf + name_size, 127, 0, 0, 1), "first record is lo");
	expect(strcmp((const char *)buf + ifreq_size, "eth0") == 0
		&& is_sockaddr(buf + ifreq_size + name_size, 192, 168, 1, 20), "second record is eth0 at a 32-byte stride");
	expect(buf[used] == 0xee, "nothing written past the last whole record");

	expect(conf(&hosts.lo, buf, sizeof(buf)) == 3 * ifreq_size, "wlan0 without IPv4 is not listed");
	expect(strcmp((const char *)buf + 2 * ifreq_size, "ppp0") == 0, "third record is ppp0");
}

int main()
{
	test_decode();
	test_query();
	test_conf();
	if (failures)
		return 1;
	printf("bsdsocket_ifinfo_test: all tests passed\n");
	return 0;
}
