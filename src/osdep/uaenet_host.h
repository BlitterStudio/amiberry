#ifndef AMIBERRY_UAENET_HOST_H
#define AMIBERRY_UAENET_HOST_H

#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_dl.h>
#elif defined(__linux__)
#include <netpacket/packet.h>
#endif

// Reads the hardware address of a host network interface.
inline bool uaenet_host_mac(const char *ifname, uint8_t mac[6])
{
	struct ifaddrs *ifaddr;
	if (getifaddrs(&ifaddr) != 0)
		return false;

	bool found = false;
	for (struct ifaddrs *ifa = ifaddr; ifa && !found; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr || strcmp(ifa->ifa_name, ifname) != 0)
			continue;
#if defined(__APPLE__) || defined(__FreeBSD__)
		if (ifa->ifa_addr->sa_family == AF_LINK) {
			const auto *sdl = reinterpret_cast<const struct sockaddr_dl *>(ifa->ifa_addr);
			if (sdl->sdl_alen == 6) {
				memcpy(mac, LLADDR(sdl), 6);
				found = true;
			}
		}
#elif defined(__linux__)
		if (ifa->ifa_addr->sa_family == AF_PACKET) {
			const auto *sll = reinterpret_cast<const struct sockaddr_ll *>(ifa->ifa_addr);
			if (sll->sll_halen == 6) {
				memcpy(mac, sll->sll_addr, 6);
				found = true;
			}
		}
#endif
	}
	freeifaddrs(ifaddr);
	return found;
}

// Reads the MTU of a host network interface, or returns -1.
inline int uaenet_host_mtu(const char *ifname)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	int mtu = ioctl(fd, SIOCGIFMTU, &ifr) == 0 ? ifr.ifr_mtu : -1;
	close(fd);
	return mtu;
}

// Guest address for a host interface, derived as in WinUAE: the locally
// administered unicast prefix aa:82:8a ("UAE" << 1) followed by the low three
// bytes of the host address, or zeros if the host address is unknown.
inline void uaenet_guest_mac(const uint8_t *host, uint8_t guest[6])
{
	static const uint8_t prefix[3] = { 0xaa, 0x82, 0x8a };
	memcpy(guest, prefix, 3);
	if (host)
		memcpy(guest + 3, host + 3, 3);
	else
		memset(guest + 3, 0, 3);
}

#endif
