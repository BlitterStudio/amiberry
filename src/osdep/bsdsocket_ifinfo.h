#pragma once

/*
 * Interface queries for the POSIX bsdsocket emulation's IoctlSocket().
 *
 * Amiga stacks (AmiTCP, Roadshow) use the 4.4BSD layout: struct ifreq is a
 * 16-byte name followed by a 16-byte union, and SIOCGIFCONF fills an array of
 * those 32-byte records. Host layouts differ (a 40-byte ifreq on 64-bit Linux,
 * variable-length records on macOS and the BSDs), so the replies are built
 * here from getifaddrs() in the Amiga layout: big-endian, IPv4 only.
 */

#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bsdsock_ifinfo {

constexpr size_t name_size = 16;   /* ifr_name */
constexpr size_t value_size = 16;  /* the ifr_ifru union */
constexpr size_t ifreq_size = name_size + value_size;

/* Amiga errno values (BSD numbering). */
constexpr int amiga_ENXIO = 6;
constexpr int amiga_EINVAL = 22;
constexpr int amiga_EADDRNOTAVAIL = 49;

enum class request { none, flags, addr, dstaddr, brdaddr, netmask, conf };

/*
 * Decodes an Amiga ioctl code: _IOWR('i', n, ...). The length field is not
 * compared, so codes built with either sizeof(struct ifreq) or the 16-byte
 * size some SDKs use for the address requests are accepted.
 */
inline request decode(uint32_t code)
{
	if ((code & 0xe000ff00) != 0xc0006900)
		return request::none;
	switch (code & 0xff) {
	case 17: return request::flags;
	case 33: return request::addr;
	case 34: return request::dstaddr;
	case 35: return request::brdaddr;
	case 36: return request::conf;
	case 37: return request::netmask;
	default: return request::none;
	}
}

/* Host IFF_* flags to the Amiga values; IFF_MULTICAST differs on Linux. */
inline uint16_t amiga_flags(unsigned int host)
{
	uint16_t flags = 0;
	if (host & IFF_UP) flags |= 0x0001;
	if (host & IFF_BROADCAST) flags |= 0x0002;
#ifdef IFF_DEBUG
	if (host & IFF_DEBUG) flags |= 0x0004;
#endif
	if (host & IFF_LOOPBACK) flags |= 0x0008;
	if (host & IFF_POINTOPOINT) flags |= 0x0010;
#ifdef IFF_RUNNING
	if (host & IFF_RUNNING) flags |= 0x0040;
#endif
	if (host & IFF_NOARP) flags |= 0x0080;
	if (host & IFF_PROMISC) flags |= 0x0100;
	if (host & IFF_ALLMULTI) flags |= 0x0200;
	if (host & IFF_MULTICAST) flags |= 0x8000;
	return flags;
}

/* An Amiga struct sockaddr_in for an IPv4 host address, port 0. */
inline void put_sockaddr(uint8_t out[value_size], const sockaddr* sa)
{
	std::memset(out, 0, value_size);
	out[0] = 16;  /* sin_len */
	out[1] = 2;   /* AF_INET */
	std::memcpy(out + 4, &reinterpret_cast<const sockaddr_in*>(sa)->sin_addr, 4);
}

inline bool is_ipv4(const ifaddrs* ifa)
{
	return ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET;
}

/*
 * Answers SIOCGIFFLAGS or SIOCGIF{ADDR,DSTADDR,BRDADDR,NETMASK} for the
 * interface called name. Returns 0 and fills out (the union that follows
 * ifr_name), or an Amiga errno.
 */
inline int query(const ifaddrs* list, const char* name, request req, uint8_t out[value_size])
{
	bool known = false;
	for (const ifaddrs* ifa = list; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_name || std::strcmp(ifa->ifa_name, name) != 0)
			continue;
		known = true;
		if (req == request::flags) {
			const uint16_t flags = amiga_flags(ifa->ifa_flags);
			std::memset(out, 0, value_size);
			out[0] = static_cast<uint8_t>(flags >> 8);
			out[1] = static_cast<uint8_t>(flags);
			return 0;
		}
		if (!is_ipv4(ifa))
			continue;
		const sockaddr* sa = ifa->ifa_addr;
		if (req == request::netmask) {
			sa = ifa->ifa_netmask;
		} else if (req == request::brdaddr) {
			if (!(ifa->ifa_flags & IFF_BROADCAST))
				return amiga_EINVAL;
			sa = ifa->ifa_broadaddr;
		} else if (req == request::dstaddr) {
			if (!(ifa->ifa_flags & IFF_POINTOPOINT))
				return amiga_EINVAL;
			sa = ifa->ifa_dstaddr;
		}
		if (!sa)
			return amiga_EADDRNOTAVAIL;
		put_sockaddr(out, sa);
		return 0;
	}
	return known ? amiga_EADDRNOTAVAIL : amiga_ENXIO;
}

/* Bytes SIOCGIFCONF needs to list every IPv4 address. */
inline size_t conf_size(const ifaddrs* list)
{
	size_t size = 0;
	for (const ifaddrs* ifa = list; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_name && is_ipv4(ifa))
			size += ifreq_size;
	}
	return size;
}

/*
 * Fills buf with one struct ifreq per IPv4 address, as many whole records as
 * capacity holds, and returns the bytes written (the new ifc_len).
 */
inline size_t conf(const ifaddrs* list, uint8_t* buf, size_t capacity)
{
	size_t used = 0;
	for (const ifaddrs* ifa = list; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_name || !is_ipv4(ifa))
			continue;
		if (capacity - used < ifreq_size)
			break;
		uint8_t* rec = buf + used;
		std::memset(rec, 0, name_size);
		std::memcpy(rec, ifa->ifa_name, strnlen(ifa->ifa_name, name_size - 1));
		put_sockaddr(rec + name_size, ifa->ifa_addr);
		used += ifreq_size;
	}
	return used;
}

} // namespace bsdsock_ifinfo
