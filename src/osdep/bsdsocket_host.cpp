/*
 * UAE - The Un*x Amiga Emulator
 *
 * bsdsocket.library emulation - Unix
 *
 * Copyright 2000-2001 Carl Drougge <carl.drougge@home.se> <bearded@longhaired.org>
 * Copyright 2003-2005 Richard Drummond
 * Copyright 2004      Jeff Shepherd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "threaddep/thread.h"
#include "native2amiga.h"
#include "bsdsocket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstddef>
#include <netdb.h>

#include <csignal>
#include <arpa/inet.h>
#include <cstring>
#if defined(__linux__)
#include <pthread.h>
#elif defined(__APPLE__)
#include <mutex>
static std::mutex bsdsock_mutex;
#else
#include <mutex>
static std::mutex bsdsock_mutex;
#endif

#define WAITSIGNAL  waitsig (ctx, sb)

/* Sigqueue is unsafe on SMP machines.
 * Temporary work-around.
 */
#define SETSIGNAL \
	do { \
	uae_Signal (sb->ownertask, sb->sigstosend | ((uae_u32) 1) << sb->signal); \
	sb->dosignal = 1; \
	} while (0)


#define SETERRNO    bsdsocklib_seterrno (ctx, sb,mapErrno (errno))
#define SETHERRNO   bsdsocklib_setherrno (ctx, sb, h_errno)


/* BSD-systems don't seem to have MSG_NOSIGNAL..
   @@@ We need to catch SIGPIPE on those systems! (?) */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define S_GL_result(res) sb->resultval = (res)

uae_u32 bsdthr_Accept_2 (SB);
uae_u32 bsdthr_Recv_2 (SB);
uae_u32 bsdthr_blockingstuff (uae_u32 (*tryfunc)(SB), SB);
uae_u32 bsdthr_SendRecvAcceptConnect (uae_u32 (*tryfunc)(SB), SB);
uae_u32 bsdthr_Send_2 (SB);
uae_u32 bsdthr_Connect_2 (SB);
uae_u32 bsdthr_WaitSelect (SB);
uae_u32 bsdthr_Wait (SB);
void clearsockabort (SB);

static uae_sem_t sem_queue;

/**
 ** Helper functions
 **/

/*
 * Map host errno to amiga errno
 */
static int mapErrno (int e)
{
	switch (e) {
	case EINTR:     e = 4;  break;
	case EDEADLK:       e = 11; break;
	case EAGAIN:        e = 35; break;
	case EINPROGRESS:   e = 36; break;
	case EALREADY:      e = 37; break;
	case ENOTSOCK:      e = 38; break;
	case EDESTADDRREQ:  e = 39; break;
	case EMSGSIZE:      e = 40; break;
	case EPROTOTYPE:    e = 41; break;
	case ENOPROTOOPT:   e = 42; break;
	case EPROTONOSUPPORT:   e = 43; break;
	case ESOCKTNOSUPPORT:   e = 44; break;
	case EOPNOTSUPP:    e = 45; break;
	case EPFNOSUPPORT:  e = 46; break;
	case EAFNOSUPPORT:  e = 47; break;
	case EADDRINUSE:    e = 48; break;
	case EADDRNOTAVAIL: e = 49; break;
	case ENETDOWN:      e = 50; break;
	case ENETUNREACH:   e = 51; break;
	case ENETRESET:     e = 52; break;
	case ECONNABORTED:  e = 53; break;
	case ECONNRESET:    e = 54; break;
	case ENOBUFS:       e = 55; break;
	case EISCONN:       e = 56; break;
	case ENOTCONN:      e = 57; break;
	case ESHUTDOWN:     e = 58; break;
	case ETOOMANYREFS:  e = 59; break;
	case ETIMEDOUT:     e = 60; break;
	case ECONNREFUSED:  e = 61; break;
	case ELOOP:     e = 62; break;
	case ENAMETOOLONG:  e = 63; break;
	default: break;
	}
	return e;
}

/*
 * Map amiga (s|g)etsockopt level into native one
 */
static int mapsockoptlevel (int level)
{
	switch (level) {
	case 0xffff:
		return SOL_SOCKET;
	case 0:
		return IPPROTO_IP;
	case 1:
		return IPPROTO_ICMP;
	case 2:
		return IPPROTO_IGMP;
#ifdef IPPROTO_IPIP
	case 4:
		return IPPROTO_IPIP;
#endif
	case 6:
		return IPPROTO_TCP;
	case 8:
		return IPPROTO_EGP;
	case 12:
		return IPPROTO_PUP;
	case 17:
		return IPPROTO_UDP;
	case 22:
		return IPPROTO_IDP;
#ifdef IPPROTO_TP
	case 29:
		return IPPROTO_TP;
#endif
	case 98:
		return IPPROTO_ENCAP;
	default:
		write_log ("Unknown sockopt level %d\n", level);
		return level;
	}
}

/*
 * Map amiga (s|g)etsockopt optname into native one
 */
static int mapsockoptname (int level, int optname)
{
	switch (level) {

	case SOL_SOCKET:
		switch (optname) {
		case 0x0001:
			return SO_DEBUG;
		case 0x0002:
			return SO_ACCEPTCONN;
		case 0x0004:
			return SO_REUSEADDR;
		case 0x0008:
			return SO_KEEPALIVE;
		case 0x0010:
			return SO_DONTROUTE;
		case 0x0020:
			return SO_BROADCAST;
#ifdef SO_USELOOPBACK
		case 0x0040:
			return SO_USELOOPBACK;
#endif
		case 0x0080:
			return SO_LINGER;
		case 0x0100:
			return SO_OOBINLINE;
#ifdef SO_REUSEPORT
		case 0x0200:
			return SO_REUSEPORT;
#endif
		case 0x1001:
			return SO_SNDBUF;
		case 0x1002:
			return SO_RCVBUF;
		case 0x1003:
			return SO_SNDLOWAT;
		case 0x1004:
			return SO_RCVLOWAT;
		case 0x1005:
			return SO_SNDTIMEO;
		case 0x1006:
			return SO_RCVTIMEO;
		case 0x1007:
			return SO_ERROR;
		case 0x1008:
			return SO_TYPE;

		default:
			write_log("Invalid setsockopt option 0x%x for level %d\n",
					  optname, level);
			return -1;
		}
		break;

	case IPPROTO_IP:
		switch (optname) {
		case 1:
			return IP_OPTIONS;
		case 2:
			return IP_HDRINCL;
		case 3:
			return IP_TOS;
		case 4:
			return IP_TTL;
		case 5:
			return IP_RECVOPTS;
		case 6:
			return IP_RECVRETOPTS;
		case 8:
			return IP_RETOPTS;
		case 9:
			return IP_MULTICAST_IF;
		case 10:
			return IP_MULTICAST_TTL;
		case 11:
			return IP_MULTICAST_LOOP;
		case 12:
			return IP_ADD_MEMBERSHIP;

		default:
			write_log("Invalid setsockopt option 0x%x for level %d\n",
					  optname, level);
			return -1;
		}
		break;

	case IPPROTO_TCP:
		switch (optname) {
		case 1:
			return TCP_NODELAY;
		case 2:
			return TCP_MAXSEG;

		default:
			write_log("Invalid setsockopt option 0x%x for level %d\n",
					  optname, level);
			return -1;
		}
		break;

	default:
		write_log("Unknown level %d\n", level);
		return -1;
	}
}

/*
 * Map amiga (s|g)etsockopt return value into the correct form
 */
static void mapsockoptreturn(int level, int optname, uae_u32 optval, void *buf)
{
	switch (level) {

	case SOL_SOCKET:
		switch (optname) {
		case SO_DEBUG:
		case SO_ACCEPTCONN:
		case SO_REUSEADDR:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_BROADCAST:
#ifdef SO_USELOOPBACK
		case SO_USELOOPBACK:
#endif
		case SO_LINGER:
		case SO_OOBINLINE:
#ifdef SO_REUSEPORT
		case SO_REUSEPORT:
#endif
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		case SO_TYPE:
			put_long (optval, *(int *)buf);
			break;

		case SO_ERROR:
			write_log("New errno is %d\n", mapErrno(*(int *)buf));
			put_long (optval, mapErrno(*(int *)buf));
			break;
		default:
			break;
		}
		break;

	case IPPROTO_IP:
		switch (optname) {
		case IP_OPTIONS:
		case IP_HDRINCL:
		case IP_TOS:
		case IP_TTL:
		case IP_RECVOPTS:
		//case IP_RECVRETOPTS:
		//case IP_RETOPTS:
		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
			put_long (optval, *(int *)buf);
			break;

		default:
			break;
		}
		break;

	case IPPROTO_TCP:
		switch (optname) {
		case TCP_NODELAY:
		case TCP_MAXSEG:
			put_long (optval,*(int *)buf);
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}

/*
 * Map amiga (s|g)etsockopt value from amiga to the appropriate value
 */
static void mapsockoptvalue(int level, int optname, uae_u32 optval, void *buf)
{
	switch (level) {

	case SOL_SOCKET:
		switch (optname) {
		case SO_DEBUG:
		case SO_ACCEPTCONN:
		case SO_REUSEADDR:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_BROADCAST:
#ifdef SO_USELOOPBACK
		case SO_USELOOPBACK:
#endif
		case SO_LINGER:
		case SO_OOBINLINE:
#ifdef SO_REUSEPORT
		case SO_REUSEPORT:
#endif
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		case SO_TYPE:
		case SO_ERROR:
			*((int *)buf) = get_long (optval);
			break;
		default:
			break;
		}
		break;

	case IPPROTO_IP:
		switch (optname) {
		case IP_OPTIONS:
		case IP_HDRINCL:
		case IP_TOS:
		case IP_TTL:
		case IP_RECVOPTS:
		//case IP_RECVRETOPTS:
		//case IP_RETOPTS:
		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
			*((int *)buf) = get_long (optval);
			break;

		default:
			break;
		}
		break;

	case IPPROTO_TCP:
		switch (optname) {
		case TCP_NODELAY:
		case TCP_MAXSEG:
			*((int *)buf) = get_long (optval);
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}

STATIC_INLINE int bsd_amigaside_FD_ISSET (int n, uae_u32 set)
{
	uae_u32 foo = get_long (set + (n / 32));
	if (foo & (1 << (n % 32)))
		return 1;
	return 0;
}

STATIC_INLINE void bsd_amigaside_FD_ZERO (uae_u32 set)
{
	put_long (set, 0);
	put_long (set + 4, 0);
}

STATIC_INLINE void bsd_amigaside_FD_SET (int n, uae_u32 set)
{
	set = set + (n / 32);
	put_long (set, get_long (set) | (1 << (n % 32)));
}

static void printSockAddr(struct sockaddr_in* in)
{
	write_log("Family %d, ", in->sin_family);
	write_log("Port %d,", ntohs(in->sin_port));
	write_log("Address %s,", inet_ntoa(in->sin_addr));
}

/*
 * Copy a sockaddr object from amiga space to native space
 */
static int copysockaddr_a2n(struct sockaddr_in* addr, uae_u32 a_addr, unsigned int len)
{
	if ((len > sizeof(struct sockaddr_in)) || (len < 8))
		return 1;

	if (a_addr == 0)
		return 0;

	addr->sin_family = get_byte(a_addr + 1);
	addr->sin_port = htons(get_word(a_addr + 2));
	addr->sin_addr.s_addr = htonl(get_long(a_addr + 4));

	if (len > 8)
		memcpy(&addr->sin_zero, get_real_address(a_addr + 8), static_cast<size_t>(len) - 8);   /* Pointless? */

	return 0;
}

/*
 * Copy a sockaddr object from native space to amiga space
 */
static int copysockaddr_n2a (uae_u32 a_addr, const struct sockaddr_in *addr, unsigned int len)
{
	if (len < 8)
		return 1;

	if (a_addr == 0)
		return 0;

	put_byte (a_addr, 0);                       /* Anyone use this field? */
	put_byte (a_addr + 1, addr->sin_family);
	put_word (a_addr + 2, ntohs (addr->sin_port));
	put_long (a_addr + 4, ntohl (addr->sin_addr.s_addr));

	if (len > 8)
		memset (get_real_address (a_addr + 8), 0, static_cast<size_t>(len) - 8);

	return 0;
}

/*
 * Copy a hostent object from native space to amiga space
 */
static void copyHostent(TrapContext* ctx, const struct hostent* hostent, SB)
{
	int size = 28;
	int i;
	int numaddr = 0;
	int numaliases = 0;
	uae_u32 aptr;

	if (hostent->h_name != NULL)
		size += strlen(hostent->h_name) + 1;

	if (hostent->h_aliases != NULL)
		while (hostent->h_aliases[numaliases])
			size += strlen(hostent->h_aliases[numaliases++]) + 5;

	if (hostent->h_addr_list != NULL) {
		while (hostent->h_addr_list[numaddr])
			numaddr++;
		size += numaddr * (hostent->h_length + 4);
	}

	aptr = sb->hostent + 28 + numaliases * 4 + numaddr * 4;

	// transfer hostent to Amiga memory
	trap_put_long(ctx, sb->hostent + 4, sb->hostent + 20);
	trap_put_long(ctx, sb->hostent + 8, hostent->h_addrtype);
	trap_put_long(ctx, sb->hostent + 12, hostent->h_length);
	trap_put_long(ctx, sb->hostent + 16, sb->hostent + 24 + numaliases * 4);

	for (i = 0; i < numaliases; i++)
		trap_put_long(ctx, sb->hostent + 20 + i * 4, addstr(ctx, &aptr, hostent->h_aliases[i]));
	trap_put_long(ctx, sb->hostent + 20 + numaliases * 4, 0);

	for (i = 0; i < numaddr; i++) {
		trap_put_long(ctx, sb->hostent + 24 + (numaliases + i) * 4, addmem(ctx, &aptr, hostent->h_addr_list[i], hostent->h_length));
	}
	trap_put_long(ctx, sb->hostent + 24 + numaliases * 4 + numaddr * 4, 0);
	trap_put_long(ctx, sb->hostent, aptr);
	addstr(ctx, &aptr, hostent->h_name);
	
	bsdsocklib_seterrno(ctx, sb, 0);
}

/*
 * Copy a protoent object from native space to Amiga space
 */
static void copyProtoent(TrapContext* ctx, SB, const struct protoent* p)
{
	int size = 16;
	int numaliases = 0;
	int i;
	uae_u32 aptr;

	// compute total size of protoent
	if (p->p_name != NULL)
		size += strlen(p->p_name) + 1;

	if (p->p_aliases != NULL)
		while (p->p_aliases[numaliases])
			size += strlen(p->p_aliases[numaliases++]) + 5;

	if (sb->protoent) {
		uae_FreeMem(ctx, sb->protoent, sb->protoentsize, sb->sysbase);
	}

	sb->protoent = uae_AllocMem(ctx, size, 0, sb->sysbase);

	if (!sb->protoent) {
		write_log("BSDSOCK: WARNING - copyProtoent() ran out of Amiga memory (couldn't allocate %d bytes)\n", size);
		bsdsocklib_seterrno(ctx, sb, 12); // ENOMEM
		return;
	}

	sb->protoentsize = size;

	aptr = sb->protoent + 16 + numaliases * 4;

	// transfer protoent to Amiga memory
	trap_put_long(ctx, sb->protoent + 4, sb->protoent + 12);
	trap_put_long(ctx, sb->protoent + 8, p->p_proto);

	for (i = 0; i < numaliases; i++)
		trap_put_long(ctx, sb->protoent + 12 + i * 4, addstr(ctx, &aptr, p->p_aliases[i]));
	trap_put_long(ctx, sb->protoent + 12 + numaliases * 4, 0);
	trap_put_long(ctx, sb->protoent, aptr);
	addstr(ctx, &aptr, p->p_name);
	bsdsocklib_seterrno(ctx, sb, 0);
}

uae_u32 bsdthr_Accept_2 (SB)
{
	int foo, s, s2;
	long flags;
	struct sockaddr_in addr{};
	socklen_t hlen = sizeof (struct sockaddr_in);

	if ((s = accept (sb->s, (struct sockaddr *)&addr, &hlen)) >= 0) {
		if ((flags = fcntl (s, F_GETFL)) == -1)
			flags = 0;
		fcntl (s, F_SETFL, flags & ~O_NONBLOCK); /* @@@ Don't do this if it's supposed to stay nonblocking... */
		s2 = getsd (sb->context, sb, s);
		sb->ftable[s2-1] = sb->ftable[sb->len]; /* new socket inherits the old socket's properties */
		write_log ("Accept: AmigaSide %d, NativeSide %d, len %d(%d)", sb->resultval, s, hlen, get_long (sb->a_addrlen));
		printSockAddr (&addr);
		foo = get_long (sb->a_addrlen);
		if (foo > 16)
			put_long (sb->a_addrlen, 16);
		copysockaddr_n2a (sb->a_addr, &addr, foo);
		return s2 - 1;
	} else {
		return -1;
	}
}

uae_u32 bsdthr_Recv_2 (SB)
{
    int foo;
    int socktype = 0;
    socklen_t optlen = sizeof(socktype);
    getsockopt(sb->s, SOL_SOCKET, SO_TYPE, &socktype, &optlen);
    int retries = (socktype == SOCK_RAW) ? 5 : 1;
    if (sb->from == 0) {
        ssize_t n;
        do {
            if (sb->s != -1 && socktype == SOCK_RAW) {
                write_log("[RAW RECV] fd=%d, buf=%p, len=%d, flags=0x%x\n", sb->s, sb->buf, sb->len, sb->flags);
            }
            n = recv(sb->s, sb->buf, sb->len, sb->flags /*| MSG_NOSIGNAL*/);
            foo = (int)n;
            write_log("recv2, recv returns %d, errno is %d\n", foo, errno);
            if (foo >= 0) break;
        } while (errno == EINTR && --retries > 0);
    } else {
        struct sockaddr_in addr{};
        socklen_t l = sizeof(struct sockaddr_in);
        int i = get_long(sb->fromlen);
        ssize_t n;
        copysockaddr_a2n(&addr, sb->from, i);
        do {
            if (sb->s != -1 && socktype == SOCK_RAW) {
                write_log("[RAW RECVFROM] fd=%d, buf=%p, len=%d, flags=0x%x\n", sb->s, sb->buf, sb->len, sb->flags);
            }
            n = recvfrom(sb->s, sb->buf, sb->len, sb->flags | MSG_NOSIGNAL, (struct sockaddr *)&addr, &l);
            foo = (int)n;
            write_log("recv2, recvfrom returns %d, errno is %d\n", foo, errno);
            if (foo >= 0) {
                copysockaddr_n2a(sb->from, &addr, l);
                put_long(sb->fromlen, l);
                break;
            }
        } while (errno == EINTR && --retries > 0);
    }
    return foo;
}

uae_u32 bsdthr_Send_2 (SB)
{
    if (sb->to == 0) {
        ssize_t n;
        if (sb->s != -1) {
            int socktype = 0;
            socklen_t optlen = sizeof(socktype);
            getsockopt(sb->s, SOL_SOCKET, SO_TYPE, &socktype, &optlen);
            if (socktype == SOCK_RAW) {
                write_log("[RAW SEND] fd=%d, buf=%p, len=%d, flags=0x%x\n", sb->s, sb->buf, sb->len, sb->flags);
            }
        }
        n = send (sb->s, sb->buf, sb->len, sb->flags | MSG_NOSIGNAL);
        return (int)n;
    } else {
        struct sockaddr_in addr{};
        int l = sizeof (struct sockaddr_in);
        ssize_t n;
        copysockaddr_a2n (&addr, sb->to, sb->tolen);
        if (sb->s != -1) {
            int socktype = 0;
            socklen_t optlen = sizeof(socktype);
            getsockopt(sb->s, SOL_SOCKET, SO_TYPE, &socktype, &optlen);
            if (socktype == SOCK_RAW) {
                write_log("[RAW SENDTO] fd=%d, buf=%p, len=%d, flags=0x%x\n", sb->s, sb->buf, sb->len, sb->flags);
            }
        }
        n = sendto (sb->s, sb->buf, sb->len, sb->flags | MSG_NOSIGNAL, (struct sockaddr *)&addr, l);
        return (int)n;
    }
}

uae_u32 bsdthr_Connect_2 (SB)
{
	if (sb->action == 1) {
		struct sockaddr_in addr{};
		int len = sizeof (struct sockaddr_in);
		int retval;
		copysockaddr_a2n (&addr, sb->a_addr, sb->a_addrlen);
		retval = connect (sb->s, (struct sockaddr *)&addr, len);
		write_log ("Connect returns %d, errno is %d\n", retval, errno);
		/* Hack: I need to set the action to something other than
		 * 1 but I know action == 2 does the correct thing
		 */
		sb->action = 2;
		if (retval == 0) {
			 errno = 0;
		}
		return retval;
	} else {
		int foo;
		socklen_t bar;
		bar = sizeof (foo);
		if (getsockopt (sb->s, SOL_SOCKET, SO_ERROR, &foo, &bar) == 0) {
			errno = foo;
			write_log("Connect status is %d\n", foo);
			return (foo == 0) ? 0 : -1;
		}
		return -1;
	}
}

uae_u32 bsdthr_SendRecvAcceptConnect (uae_u32 (*tryfunc)(SB), SB)
{
	return bsdthr_blockingstuff (tryfunc, sb);
}

uae_u32 bsdthr_blockingstuff(uae_u32(*tryfunc)(SB), SB)
{
    int done = 0, foo = 0;
    long flags;
    int nonblock;
    int socktype = 0;
    socklen_t optlen = sizeof(socktype);
    int is_raw = 0;
    struct timeval orig_timeout = {0}, timeout = {0};
    socklen_t tvlen = sizeof(orig_timeout);
    int timeout_set = 0;
    if ((flags = fcntl(sb->s, F_GETFL)) == -1)
        flags = 0;
    // Check if this is a raw socket
    if (getsockopt(sb->s, SOL_SOCKET, SO_TYPE, &socktype, &optlen) == 0 && socktype == SOCK_RAW) {
        is_raw = 1;
        // Save original timeout
        if (getsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, &orig_timeout, &tvlen) == 0) {
            timeout_set = 1;
        }
        // Set a 1 second timeout for raw sockets
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }
    nonblock = (flags & O_NONBLOCK);
    // Only set non-blocking for non-raw sockets
    if (!is_raw) {
        fcntl(sb->s, F_SETFL, flags | O_NONBLOCK);
    }
    while (!done) {
        done = 1;
        do {
            foo = tryfunc(sb);
        } while (foo < 0 && errno == EINTR); // retry on EINTR
        if (foo < 0 && !nonblock) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS)) {
                fd_set readset, writeset, exceptset;
                int maxfd = (sb->s > sb->sockabort[0]) ? sb->s : sb->sockabort[0];
                int num;

                FD_ZERO(&readset);
                FD_ZERO(&writeset);
                FD_ZERO(&exceptset);

                if (sb->action == 3 || sb->action == 6)
                    FD_SET(sb->s, &readset);
                if (sb->action == 2 || sb->action == 1 || sb->action == 4)
                    FD_SET(sb->s, &writeset);
                FD_SET(sb->sockabort[0], &readset);

                do {
                    num = select(maxfd + 1, &readset, &writeset, &exceptset, NULL);
                } while (num == -1 && errno == EINTR); // retry on EINTR
                if (num == -1) {
                    write_log("Blocking select(%d) returns -1,errno is %d\n", sb->sockabort[0], errno);
                    if (!is_raw) fcntl(sb->s, F_SETFL, flags);
                    if (is_raw && timeout_set) setsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, &orig_timeout, sizeof(orig_timeout));
                    return -1;
                }

                if (FD_ISSET(sb->sockabort[0], &readset) || FD_ISSET(sb->sockabort[0], &writeset)) {
                    /* reset sock abort pipe */
                    /* read from the pipe to reset it */
                    write_log("select aborted from signal\n");

                    clearsockabort(sb);
                    write_log("Done read\n");
                    errno = EINTR;
                    done = 1;
                }
                else {
                    done = 0;
                }
            }
            else if (errno == EINTR)
                done = 1;
        }
    }
    if (!is_raw) fcntl(sb->s, F_SETFL, flags);
    if (is_raw && timeout_set) setsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, &orig_timeout, sizeof(orig_timeout));
    return foo;
}

static int bsdlib_threadfunc(void* arg)
{
	auto* sb = (struct socketbase*)arg;

	write_log("THREAD_START\n");

	while (1) {
		uae_sem_wait(&sb->sem);

		write_log("Socket thread got action %d\n", sb->action);

		TrapContext* ctx = sb->context;  // FIXME: Correct?

		switch (sb->action) {
		case 0:       /* kill thread (CloseLibrary) */

			write_log("THREAD_END\n");

			uae_sem_destroy(&sb->sem);
			return 0;

		case 1:       /* Connect */
			sb->resultval = bsdthr_SendRecvAcceptConnect(bsdthr_Connect_2, sb);
			break;

			/* @@@ Should check (from|to)len so it's 16.. */
		case 2:       /* Send[to] */
			sb->resultval = bsdthr_SendRecvAcceptConnect(bsdthr_Send_2, sb);
			break;

		case 3:       /* Recv[from] */
			sb->resultval = bsdthr_SendRecvAcceptConnect(bsdthr_Recv_2, sb);
			break;

		case 4: {     /* Gethostbyname */
#if defined(__linux__)
			struct hostent hent, *tmphostent = nullptr;
			char buf[1024];
			int herr, ret;
			ret = gethostbyname_r((char*)get_real_address(sb->name), &hent, buf, sizeof(buf), &tmphostent, &herr);
			if (ret == 0 && tmphostent) {
				copyHostent(ctx, tmphostent, sb);
				bsdsocklib_setherrno(ctx, sb, 0);
			} else {
				bsdsocklib_setherrno(ctx, sb, herr);
				SETERRNO;
			}
#else
			std::lock_guard<std::mutex> lock(bsdsock_mutex);
			struct hostent* tmphostent = gethostbyname((char*)get_real_address(sb->name));
			if (tmphostent) {
				copyHostent(ctx, tmphostent, sb);
				bsdsocklib_setherrno(ctx, sb, 0);
			} else {
				SETHERRNO;
				SETERRNO;
			}
#endif
			break;
		}

		case 5:       /* WaitSelect */
			sb->resultval = bsdthr_WaitSelect(sb);
			break;

		case 6:       /* Accept */
			sb->resultval = bsdthr_SendRecvAcceptConnect(bsdthr_Accept_2, sb);
			break;

		case 7: {
#if defined(__linux__)
			struct hostent hent, *tmphostent = nullptr;
			char buf[1024];
			int herr, ret;
			ret = gethostbyaddr_r((char*)get_real_address(sb->name), sb->a_addrlen, sb->flags, &hent, buf, sizeof(buf), &tmphostent, &herr);
			if (ret == 0 && tmphostent) {
				copyHostent(ctx, tmphostent, sb);
				bsdsocklib_setherrno(ctx, sb, 0);
			} else {
				bsdsocklib_setherrno(ctx, sb, herr);
				SETERRNO;
			}
#else
			std::lock_guard<std::mutex> lock(bsdsock_mutex);
			struct hostent* tmphostent = gethostbyaddr(get_real_address(sb->name), sb->a_addrlen, sb->flags);
			if (tmphostent) {
				copyHostent(ctx, tmphostent, sb);
				bsdsocklib_setherrno(ctx, sb, 0);
			} else {
				SETHERRNO;
				SETERRNO;
			}
#endif
			break;
		}
		}
		SETSIGNAL;
	}
	return 0;        /* Just to keep GCC happy.. */
}

void clearsockabort(SB)
{
	int chr;
	int num;

	while ((num = read(sb->sockabort[0], &chr, sizeof(chr))) >= 0) {
		write_log("Sockabort got %d bytes\n", num);
	}
}

int init_socket_layer(void)
{
	int result = 0;

	if (currprefs.socket_emu) {
		if (uae_sem_init(&sem_queue, 0, 1) < 0) {
			write_log("Can't create sem %d\n", errno);
			return 0;
		}
		return 1;
	}

	return result;
}

void locksigqueue(void)
{
	uae_sem_wait(&sem_queue);
}

void unlocksigqueue(void)
{
	uae_sem_post(&sem_queue);
}

int host_sbinit (TrapContext *ctx, SB)
{
	if (pipe (sb->sockabort) < 0) {
		return 0;
	}

	if (fcntl (sb->sockabort[0], F_SETFL, O_NONBLOCK) < 0) {
		write_log ("Set nonblock failed %d\n", errno);
	}

	if (uae_sem_init (&sb->sem, 0, 0)) {
		write_log ("BSDSOCK: Failed to create semaphore.\n");
		close (sb->sockabort[0]);
		close (sb->sockabort[1]);
		return 0;
	}

	/* Alloc hostent buffer */
	sb->hostent = uae_AllocMem (ctx, 1024, 0, sb->sysbase);
	sb->hostentsize = 1024;

	/* @@@ The thread should be PTHREAD_CREATE_DETACHED */
	if (uae_start_thread ("bsdsocket", bsdlib_threadfunc, (void *)sb, &sb->thread) == BAD_THREAD) {
		write_log ("BSDSOCK: Failed to create thread.\n");
		uae_sem_destroy (&sb->sem);
		close (sb->sockabort[0]);
		close (sb->sockabort[1]);
		return 0;
	}
	return 1;
}

void host_closesocketquick (int s)
{
	struct linger l{};
	l.l_onoff = 0;
	l.l_linger = 0;
	if(s != -1) {
		setsockopt (s, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
		close (s);
	}
}

void host_sbcleanup (SB)
{
	int i;

	if (!sb) {
		return;
	}

	uae_thread_id thread = sb->thread;
	close (sb->sockabort[0]);
	close (sb->sockabort[1]);
	for (i = 0; i < sb->dtablesize; i++) {
		if (sb->dtable[i] != -1) {
			close(sb->dtable[i]);
		}
	}
	sb->action = 0;

	uae_sem_post (&sb->sem); /* destroy happens on socket thread */

	/* We need to join with the socket thread to allow the thread to die
	 * and clean up resources when the underlying thread layer is pthreads.
	 * Ideally, this shouldn't be necessary, but, for example, when SDL uses
	 * pthreads, it always creates joinable threads - and we can't do anything
	 * about that. */
	uae_wait_thread (&thread);
}

void host_sbreset (void)
{
	//STUB("");
}

void sockabort (SB)
{
	int chr = 1;
	write_log ("Sock abort!!\n");
	if (write (sb->sockabort[1], &chr, sizeof (chr)) != sizeof (chr)) {
		write_log("sockabort - did not write %zd bytes\n", sizeof(chr));
	}
}

int host_dup2socket(TrapContext *ctx, SB, int fd1, int fd2)
{
	int s1, s2;

	fd1++;

	s1 = getsock(ctx, sb, fd1);
	if (s1 != -1) {
		if (fd2 != -1) {
			if ((unsigned int) (fd2) >= (unsigned int) sb->dtablesize) {
				bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
			}
			fd2++;
			s2 = getsock(ctx, sb, fd2);
			if (s2 != -1) {
				close (s2);
			}
			setsd (ctx, sb, fd2, dup (s1));
			return 0;
		} else {
			fd2 = getsd (ctx, sb, 1);
			if (fd2 != -1) {
				setsd (ctx, sb, fd2, dup (s1));
				return (fd2 - 1);
			} else {
				return -1;
			}
		}
	}
	return -1;
}

int host_socket(TrapContext *ctx, SB, int af, int type, int protocol)
{
    int sd;
    int s;

    write_log("socket(%s,%s,%d) -> ",af == AF_INET ? "AF_INET" : "AF_other",
           type == SOCK_STREAM ? "SOCK_STREAM" : type == SOCK_DGRAM ?
           "SOCK_DGRAM " : type == SOCK_RAW ? "SOCK_RAW" : "SOCK_other", protocol);
    if (type == SOCK_RAW) {
        write_log("[RAW SOCKET] af=%d, type=%d, protocol=%d\n", af, type, protocol);
    }

    if ((s = socket (af, type, protocol)) == -1)  {
        SETERRNO;
        write_log("failed (%d)\n", sb->sb_errno);
        return -1;
    } else {
        int arg = 1;
        sd = getsd (ctx, sb, s);
        setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg));
    }

    sb->ftable[sd-1] = SF_BLOCKING;
    write_log("socket returns Amiga %d, NativeSide %d\n", sd - 1, s);
    return sd - 1;
}

uae_u32 host_bind(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	uae_u32 success = 0;
	struct sockaddr_in addr{};
	int len = sizeof (struct sockaddr_in);
	int s;

	s = getsock(ctx, sb, sd + 1);
	if (s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return -1;
	}

	write_log("bind(%u[%d], 0x%x, %u) -> ", sd, s, name, namelen);
	copysockaddr_a2n (&addr, name, namelen);
	printSockAddr (&addr);
	if ((success = ::bind (s, (struct sockaddr *)&addr, len)) != (uae_u32)0) {
		SETERRNO;
		// Improved error logging
		write_log("failed (%d: %s)\n", sb->sb_errno, strerror(errno));
		// Special message for privileged ports
		if (errno == EACCES && ntohs(addr.sin_port) < 1024) {
			write_log("bind() failed: Port %d is privileged (<1024), requires root privileges.\n", ntohs(addr.sin_port));
		}
	} else {
		write_log("OK\n");
	}
	return success;
}

uae_u32 host_listen(TrapContext *ctx, SB, uae_u32 sd, uae_u32 backlog)
{
	int s;
	uae_u32 success = -1;

	write_log("listen(%d,%d) -> ", sd, backlog);
	s = getsock(ctx, sb, sd + 1);

	if (s == -1) {
		bsdsocklib_seterrno (ctx, sb, 9);
		return -1;
	}

	if ((success = listen (s, backlog)) != 0) {
		SETERRNO;
		write_log("failed (%d)\n", sb->sb_errno);
	} else {
		write_log("OK\n");
	}
	return success;
}

void host_accept(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	sb->s = getsock(ctx, sb, sd + 1);
	if (sb->s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
	}

	write_log("accept(%d, %x, %x)\n", sb->s, name, namelen);
	sb->a_addr    = name;
	sb->a_addrlen = namelen;
	sb->action    = 6;
	sb->len       = sd;
	// used by bsdthr_Accept_2
	sb->context = ctx;

	uae_sem_post (&sb->sem);

	WAITSIGNAL;
	write_log("Accept returns %d\n", sb->resultval);
}

// FIXME: PUT THREAD CODE AT THIS POINT?

void host_connect(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	SOCKET s;
	static int wscounter;
	int wscnt;

	sd++;
	wscnt = ++wscounter;

	if (!addr_valid (_T("host_connect"), name, namelen))
		return;

	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (namelen <= MAXADDRLEN) {
			sb->s = getsock(ctx, sb, sd);
			sb->a_addr    = name;
			sb->a_addrlen = namelen;
			sb->action    = 1;

			uae_sem_post (&sb->sem);

			WAITSIGNAL;
		} else {
			write_log (_T("BSDSOCK: WARNING - Excessive namelen (%d) in connect():%d!\n"), namelen, wscnt);
		}
	} else {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
	}
}

void host_sendto (TrapContext *ctx, SB, uae_u32 sd, uae_u32 msg, uae_u8 *hmsg, uae_u32 len, uae_u32 flags, uae_u32 to, uae_u32 tolen)
{
	SOCKET s;
	char *realpt;
	static int wscounter;
	int wscnt;

	wscnt = ++wscounter;

	sd++;
	s = getsock(ctx, sb, sd);

	if (sb->s != INVALID_SOCKET) {
		if (hmsg == NULL) {
			if (!addr_valid (_T("host_sendto1"), msg, 4))
				return;
			realpt = (char*)get_real_address (msg);
		} else {
			realpt = (char*)hmsg;
		}

		sb->s = s;
		sb->buf    = get_real_address (msg);
		sb->len    = len;
		sb->flags  = flags;
		sb->to     = to;
		sb->tolen  = tolen;
		sb->action = 2;

		uae_sem_post (&sb->sem);

		WAITSIGNAL;

	} else {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
	}
}

void host_recvfrom(TrapContext *ctx, SB, uae_u32 sd, uae_u32 msg, uae_u8 *hmsg, uae_u32 len, uae_u32 flags, uae_u32 addr, uae_u32 addrlen)
{
	int s;
	uae_char *realpt;
	static int wscounter;
	int wscnt;

	wscnt = ++wscounter;

	s = getsock(ctx, sb, sd + 1);

	if (s != -1) {
		if (hmsg == NULL) {
			if (!addr_valid (_T("host_recvfrom1"), msg, 4))
				return;
			realpt = (char*)get_real_address (msg);
		} else {
			realpt = (char*)hmsg;
		}
	} else {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */;
		return;
	}

	sb->s      = s;
	sb->buf    = realpt;
	sb->len    = len;
	sb->flags  = flags;
	sb->from   = addr;
	sb->fromlen= addrlen;
	sb->action = 3;

	uae_sem_post (&sb->sem);

	WAITSIGNAL;
}

uae_u32 host_shutdown(SB, uae_u32 sd, uae_u32 how)
{
	TrapContext *ctx = NULL;
	SOCKET s;

	write_log("shutdown(%d,%d) -> ", sd, how);
	s = getsock(ctx, sb, sd + 1);

	if (s != INVALID_SOCKET) {
		if (shutdown (s, how)) {
			SETERRNO;
			write_log("failed (%d)\n", sb->sb_errno);
		} else {
			write_log("OK\n");
			return 0;
		}
	}

	return -1;
}

void host_setsockopt(SB, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 len)
{
	TrapContext* ctx = NULL;
	int s = getsock(ctx, sb, sd + 1);
	int nativelevel = mapsockoptlevel(level);
	int nativeoptname = mapsockoptname(nativelevel, optname);
	void* buf = NULL;
	struct linger sl;
	struct timeval timeout;

	if (s == INVALID_SOCKET) {
		sb->resultval = -1;
		bsdsocklib_seterrno(ctx, sb, 9); /* EBADF */
		return;
	}

	// Prevent invalid setsockopt calls
	if (nativeoptname == -1) {
		write_log("host_setsockopt: Invalid option 0x%x for level %d (native level %d), not calling setsockopt.\n", optname, level, nativelevel);
		sb->resultval = -1;
		errno = EINVAL;
		SETERRNO;
		return;
	}

	if (optval) {
		buf = malloc(len);
		if (buf == NULL) {
			sb->resultval = -1;
			bsdsocklib_seterrno(ctx, sb, 12); // ENOMEM
			return;
		}
		if (nativeoptname == SO_LINGER) {
			sl.l_onoff = get_long(optval);
			sl.l_linger = get_long(optval + 4);
		}
		else if (nativeoptname == SO_RCVTIMEO || nativeoptname == SO_SNDTIMEO) {
			timeout.tv_sec = get_long(optval);
			timeout.tv_usec = get_long(optval + 4);
		}
		else {
			mapsockoptvalue(nativelevel, nativeoptname, optval, buf);
		}
	}
	if (nativeoptname == SO_RCVTIMEO || nativeoptname == SO_SNDTIMEO) {
		sb->resultval = setsockopt(s, nativelevel, nativeoptname, &timeout, sizeof(timeout));
	}
	else if (nativeoptname == SO_LINGER) {
		sb->resultval = setsockopt(s, nativelevel, nativeoptname, &sl, sizeof(sl));
	}
	else {
		sb->resultval = setsockopt(s, nativelevel, nativeoptname, buf, len);
	}
	if (buf)
		free(buf);
	SETERRNO;

	write_log("setsockopt: sock %d, level %d, 'name' %d(%d), len %d -> %d, %d\n",
		s, level, optname, nativeoptname, len,
		sb->resultval, errno);
}

uae_u32 host_getsockopt(TrapContext* ctx, SB, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 optlen)
{
	socklen_t len = 0;
	int r;
	int s;
	int nativelevel = mapsockoptlevel(level);
	int nativeoptname = mapsockoptname(nativelevel, optname);
	void* buf = NULL;
	struct linger sl;
	struct timeval timeout;

	s = getsock(ctx, sb, sd + 1);

	if (s == INVALID_SOCKET) {
		bsdsocklib_seterrno(ctx, sb, 9); /* EBADF */
		return -1;
	}

	if (optlen) {
		len = get_long(optlen);
		buf = malloc(len);
		if (buf == NULL) {
			return -1;
		}
	}

	if (nativeoptname == SO_RCVTIMEO || nativeoptname == SO_SNDTIMEO) {
		r = getsockopt(s, nativelevel, nativeoptname, &timeout, &len);
	}
	else if (nativeoptname == SO_LINGER) {
		r = getsockopt(s, nativelevel, nativeoptname, &sl, &len);
	}
	else {
		r = getsockopt(s, nativelevel, nativeoptname, optval ? buf : NULL, optlen ? &len : NULL);
	}

	if (optlen)
		put_long(optlen, len);

	SETERRNO;
	write_log("getsockopt: sock AmigaSide %d NativeSide %d, level %d, 'name' %x(%d), len %d -> %d, %d\n",
		sd, s, level, optname, nativeoptname, len, r, errno);

	if (optval) {
		if (r == 0) {
			if (nativeoptname == SO_RCVTIMEO || nativeoptname == SO_SNDTIMEO) {
				put_long(optval, timeout.tv_sec);
				put_long(optval + 4, timeout.tv_usec);
			}
			else if (nativeoptname == SO_LINGER) {
				put_long(optval, sl.l_onoff);
				put_long(optval + 4, sl.l_linger);
			}
			else {
				mapsockoptreturn(nativelevel, nativeoptname, optval, buf);
			}
		}
	}

	if (buf != NULL)
		free(buf);
	return r;
}

uae_u32 host_getsockname(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	int s;
	socklen_t len = sizeof (struct sockaddr_in);
	struct sockaddr_in addr{};

	write_log("getsockname(%u, 0x%x, %u) -> ", sd, name, len);
	
	s = getsock(ctx, sb, sd + 1);

	if (s != INVALID_SOCKET) {
		if (getsockname (s, (struct sockaddr *)&addr, &len)) {
			SETERRNO;
			write_log("failed (%d)\n", sb->sb_errno);
		} else {
			int a_nl;
			write_log("okay\n");
			a_nl = get_long (namelen);
			copysockaddr_n2a (name, &addr, a_nl);
			if (a_nl > 16)
				put_long (namelen, 16);
			return 0;
		}
	}

	return -1;
}

uae_u32 host_getpeername(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	int s;
	socklen_t len = sizeof (struct sockaddr_in);
	struct sockaddr_in addr{};

	write_log("getpeername(%u, 0x%x, %u) -> ", sd, name, len);

	s = getsock(ctx, sb, sd + 1);

	if (s != INVALID_SOCKET) {
		if (getpeername (s, (struct sockaddr *)&addr, &len)) {
			SETERRNO;
			write_log("failed (%d)\n", sb->sb_errno);
		} else {
			int a_nl;
			write_log("okay\n");
			a_nl = get_long (namelen);
			copysockaddr_n2a (name, &addr, a_nl);
			if (a_nl > 16)
				put_long (namelen, 16);
			return 0;
		}
	}

	return -1;
}

uae_u32 host_IoctlSocket(TrapContext *ctx, SB, uae_u32 sd, uae_u32 request, uae_u32 arg)
{
	sd++;
	int sock = getsock(ctx, sb, sd);
	int r, argval = get_long (arg);
	long flags;

	if (sock == INVALID_SOCKET) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return -1;
	}

	if ((flags = fcntl (sock, F_GETFL)) == -1) {
		SETERRNO;
		return -1;
	}

	// Only log non-FIONREAD ioctls, or errors for FIONREAD
	if (request != 0x4004667F) {
		write_log("Ioctl code is %x, flags are %ld\n", request, flags);
	}

	switch (request) {
	case 0x4004667B: /* FIOGETOWN */
		sb->ownertask = get_long (arg);
		return 0;

	case 0x8004667C: /* FIOSETOWN */
		trap_put_long(ctx, arg,sb->ownertask);
		return 0;
	case 0x8004667D: /* FIOASYNC */
#   ifdef O_ASYNC
		r = fcntl (sock, F_SETFL, argval ? flags | O_ASYNC : flags & ~O_ASYNC);
		return r;
#   else
		/* O_ASYNC is only available on Linux and BSD systems */
		return fcntl (sock, F_GETFL);
#   endif

	case 0x8004667E: /* FIONBIO */
		r = fcntl (sock, F_SETFL, argval ?
			   flags | O_NONBLOCK : flags & ~O_NONBLOCK);
		if (argval) {
			write_log("nonblocking\n");
			sb->ftable[sd] &= ~SF_BLOCKING;
		} else {
			write_log("blocking\n");
			sb->ftable[sd] |= SF_BLOCKING;
		}
		return r;

	case 0x4004667F: /* FIONREAD */
		r = ioctl (sock, FIONREAD, &flags);
		if (r >= 0) {
			trap_put_long(ctx, arg, flags);
		} else {
			write_log("Ioctl FIONREAD failed: %d\n", errno);
		}
		return r;

	} /* end switch */

	bsdsocklib_seterrno (ctx, sb, EINVAL);
	return -1;
}

int host_CloseSocket(TrapContext *ctx, SB, int sd)
{
	int s = getsock(ctx, sb, sd + 1);
	int retval;

	if (s == INVALID_SOCKET) {
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return -1;
	}

	/*
	if (checksd (sb, sd + 1) == 1) {
	return 0;
	}
	*/
	write_log("CloseSocket Amiga: %d, NativeSide %d\n", sd, s);
	retval = close (s);
	SETERRNO;
	releasesock (ctx, sb, sd + 1);
	return retval;
}

static void fd_zero(TrapContext *ctx, uae_u32 fdset, uae_u32 nfds)
{
	unsigned int i;
	for (i = 0; i < nfds; i += 32, fdset += 4)
		trap_put_long(ctx, fdset,0);
}

uae_u32 bsdthr_WaitSelect(SB)
{
	fd_set sets[3];
	int i, s, set, a_s, max;
	uae_u32 a_set;
	struct timeval tv {};
	int r;
	TrapContext* ctx = NULL;  // FIXME: Correct?

	write_log("WaitSelect: %d 0x%x 0x%x 0x%x 0x%x 0x%x\n", sb->nfds, sb->sets[0], sb->sets[1], sb->sets[2], sb->timeout, sb->sigmp);

	if (sb->timeout)
		write_log("WaitSelect: timeout %d %d\n", get_long(sb->timeout), get_long(sb->timeout + 4));

	FD_ZERO(&sets[0]);
	FD_ZERO(&sets[1]);
	FD_ZERO(&sets[2]);

	/* Set up the abort socket */
	FD_SET(sb->sockabort[0], &sets[0]);
	FD_SET(sb->sockabort[0], &sets[2]);
	max = sb->sockabort[0];

	for (set = 0; set < 3; set++) {
		if (sb->sets[set] != 0) {
			a_set = sb->sets[set];
			for (i = 0; i < sb->nfds; i++) {
				if (bsd_amigaside_FD_ISSET(i, a_set)) {
					s = getsock(ctx, sb, i + 1);
					write_log("WaitSelect: AmigaSide %d set. NativeSide %d.\n", i, s);
					if (s == -1) {
						write_log("BSDSOCK: WaitSelect() called with invalid descriptor %d in set %d.\n", i, set);
					} else {
						FD_SET(s, &sets[set]);
						if (max < s) max = s;
					}
				}
			}
		}
	}

	max++;

	if (sb->timeout) {
		tv.tv_sec = get_long(sb->timeout);
		tv.tv_usec = get_long(sb->timeout + 4);
	}

	write_log("Select going to select\n");
	r = select(max, &sets[0], &sets[1], &sets[2], (sb->timeout == 0) ? NULL : &tv);
	write_log("Select returns %d, errno is %d\n", r, errno);
	if (r > 0) {
		/* Socket told us to abort */
		if (FD_ISSET(sb->sockabort[0], &sets[0])) {
			/* read from the pipe to reset it */
			write_log("WaitSelect aborted from signal\n");
			r = 0;
			for (set = 0; set < 3; set++)
				if (sb->sets[set] != 0)
					bsd_amigaside_FD_ZERO(sb->sets[set]);
			clearsockabort(sb);
		}
		else
			/* This is perhaps slightly inefficient, but I don't care.. */
			for (set = 0; set < 3; set++) {
				a_set = sb->sets[set];
				if (a_set != 0) {
					bsd_amigaside_FD_ZERO(a_set);
					for (i = 0; i < sb->nfds; i++) {
						a_s = getsock(ctx, sb, i + 1);
						if (!(a_s < 0)) {
							if (FD_ISSET(a_s, &sets[set])) {
								write_log("WaitSelect: NativeSide %d set. AmigaSide %d.\n", a_s, i);

								bsd_amigaside_FD_SET(i, a_set);
							}
						}
					}
				}
			}
	} else if (r == 0) {         /* Timeout. I think we're supposed to clear the sets.. */
		for (set = 0; set < 3; set++)
			if (sb->sets[set] != 0)
				bsd_amigaside_FD_ZERO(sb->sets[set]);
	}
	write_log("WaitSelect: r=%d errno=%d\n", r, errno);
	return r;
}

void host_WaitSelect(TrapContext *ctx, SB, uae_u32 nfds, uae_u32 readfds, uae_u32 writefds, uae_u32 exceptfds, uae_u32 timeout, uae_u32 sigmp)
{
	uae_u32 wssigs = (sigmp) ? trap_get_long(ctx, sigmp) : 0;
	uae_u32 sigs;

	if (wssigs) {
		trap_call_add_dreg(ctx, 0, 0);
		trap_call_add_dreg(ctx, 1, wssigs);
		sigs = trap_call_lib(ctx, sb->sysbase, -0x132) & wssigs; // SetSignal()
		if (sigs) {
			put_long (sigmp, sigs);
			// Check for zero address -> otherwise WinUAE crashes
			if (readfds)
				fd_zero(ctx, readfds,nfds);
			if (writefds)
				fd_zero(ctx, writefds,nfds);
			if (exceptfds)
				fd_zero(ctx, exceptfds,nfds);
			sb->resultval = 0;
			bsdsocklib_seterrno (ctx, sb, 0);
			return;
		}
	}

	if (nfds == 0) {
		/* No sockets - Just wait on signals */
		if (wssigs != 0) {
			trap_call_add_dreg(ctx, 0, wssigs);
			sigs = trap_call_lib(ctx, sb->sysbase, -0x13e); // Wait()
			trap_put_long(ctx, sigmp, sigs & wssigs);
		}

		if (readfds)
			fd_zero (ctx, readfds,nfds);
		if (writefds)
			fd_zero (ctx, writefds,nfds);
		if (exceptfds)
			fd_zero (ctx, exceptfds,nfds);
		sb->resultval = 0;
		return;
	}

	sb->nfds = nfds;
	sb->sets [0] = readfds;
	sb->sets [1] = writefds;
	sb->sets [2] = exceptfds;
	sb->timeout  = timeout;
	sb->sigmp    = wssigs;
	sb->action   = 5;

	uae_sem_post (&sb->sem);

	trap_call_add_dreg(ctx, 0, (((uae_u32)1) << sb->signal) | sb->eintrsigs | wssigs);
	sigs = trap_call_lib(ctx, sb->sysbase, -0x13e);	// Wait()

	if (sigmp)
		trap_put_long(ctx, sigmp, sigs & wssigs);

	if (sigs & wssigs) {
		/* Received the signals we were waiting on */
		write_log("WaitSelect: got signal(s) %x\n", sigs);

		if (!(sigs & (((uae_u32)1) << sb->signal))) {
			sockabort (sb);
			WAITSIGNAL;
		}

		sb->resultval = 0;
		if (readfds)
			fd_zero(ctx, readfds, nfds);
		if (writefds)
			fd_zero(ctx, writefds, nfds);
		if (exceptfds)
			fd_zero(ctx, exceptfds, nfds);

		bsdsocklib_seterrno (ctx, sb, 0);
	} else if (sigs & sb->eintrsigs) {
		/* Wait select was interrupted */
		write_log("WaitSelect: interrupted\n");

		if (!(sigs & (((uae_u32)1) << sb->signal))) {
			sockabort (sb);
			WAITSIGNAL;
		}

		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, mapErrno (EINTR));
	}
	clearsockabort(sb);
}

uae_u32 host_Inet_NtoA(TrapContext *ctx, SB, uae_u32 in)
{
	uae_char *addr;
	struct in_addr ina;
	uae_u32 buf;

	*(uae_u32 *)&ina = htonl (in);

	if ((addr = inet_ntoa(ina)) != NULL) {
		buf = m68k_areg (regs, 6) + offsetof (struct UAEBSDBase, scratchbuf);
		strncpyha (ctx, buf, addr, SCRATCHBUFSIZE);
		return buf;
	} else
		SETERRNO;

	return 0;
}

uae_u32 host_inet_addr(TrapContext *ctx, uae_u32 cp)
{
	uae_u32 addr;
	char *cp_rp;

	if (!trap_valid_address(ctx, cp, 4))
		return 0;
	cp_rp = trap_get_alloc_string(ctx, cp, 256);
	addr = htonl(inet_addr(cp_rp));

	xfree(cp_rp);
	return addr;
}

// --- Wrap getservbyname, getservbyport, getprotobyname, getprotobynumber with mutex ---

void host_getprotobyname (TrapContext *ctx, SB, uae_u32 name)
{
#if defined(__linux__)
	struct protoent *p = getprotobyname ((char *)get_real_address (name));
#else
	// Thread safety: protect non-reentrant getprotobyname
	std::lock_guard<std::mutex> lock(bsdsock_mutex);
	struct protoent *p = getprotobyname ((char *)get_real_address (name));
#endif
	write_log("Getprotobyname(%s) = %p\n", get_real_address (name), p);
	if (p == NULL) {
		SETHERRNO;
		SETERRNO;
		return;
	}
	copyProtoent(ctx, sb, p);
}

void host_getprotobynumber(TrapContext *ctx, SB, uae_u32 number)
{
#if defined(__linux__)
	struct protoent *p = getprotobynumber(number);
#else
	// Thread safety: protect non-reentrant getprotobynumber
	std::lock_guard<std::mutex> lock(bsdsock_mutex);
	struct protoent *p = getprotobynumber(number);
#endif
	write_log("getprotobynumber(%d) = %p\n", number, p);
	if (p == NULL) {
		SETHERRNO;
		SETERRNO;
		return;
	}
	copyProtoent(ctx, sb, p);
}

void host_getservbynameport(TrapContext *ctx, SB, uae_u32 nameport, uae_u32 proto, uae_u32 type)
{
	struct servent *s;
#if defined(__linux__)
	s = (type) ?
		getservbyport (nameport, (char *)get_real_address (proto)) :
		getservbyname ((char *)get_real_address (nameport), (char *)get_real_address (proto));
#else
	// Thread safety: protect non-reentrant getservby* functions
	std::lock_guard<std::mutex> lock(bsdsock_mutex);
	s = (type) ?
		getservbyport (nameport, (char *)get_real_address (proto)) :
		getservbyname ((char *)get_real_address (nameport), (char *)get_real_address (proto));
#endif
	int size;
	int numaliases = 0;
	uae_u32 aptr;
	int i;
	if (type) {
		write_log("Getservbyport(%d, %s) = %p\n", nameport, get_real_address (proto), s);
	} else {
		write_log("Getservbyname(%s, %s) = %p\n", get_real_address (nameport), get_real_address (proto), s);
	}
	if (s != NULL) {
		// compute total size of servent
		size = 20;
		if (s->s_name != NULL)
			size += strlen (s->s_name) + 1;
		if (s->s_proto != NULL)
			size += strlen (s->s_proto) + 1;

		if (s->s_aliases != NULL)
			while (s->s_aliases[numaliases])
				size += strlen (s->s_aliases[numaliases++]) + 5;

		if (sb->servent) {
			uae_FreeMem(ctx, sb->servent, sb->serventsize, sb->sysbase);
		}

		sb->servent = uae_AllocMem (ctx, size, 0, sb->sysbase);

		if (!sb->servent) {
			write_log ("BSDSOCK: WARNING - getservby%s() ran out of Amiga memory (couldn't allocate %d bytes)\n",type ? "port" : "name", size);
			bsdsocklib_seterrno (ctx, sb, 12); // ENOMEM
			return;
		}

		sb->serventsize = size;

		aptr = sb->servent + 20 + numaliases * 4;

		// transfer servent to Amiga memory
		trap_put_long(ctx, sb->servent + 4, sb->servent + 16);
		trap_put_long(ctx, sb->servent + 8, (unsigned short)htons (s->s_port));

		for (i = 0; i < numaliases; i++)
			trap_put_long(ctx, sb->servent + 16 + i * 4, addstr_ansi(ctx, &aptr, s->s_aliases[i]));
		trap_put_long(ctx, sb->servent + 16 + numaliases * 4, 0);
		trap_put_long(ctx, sb->servent, aptr);
		addstr_ansi(ctx, &aptr, s->s_name);
		trap_put_long(ctx, sb->servent + 12, aptr);
		addstr_ansi(ctx, &aptr, s->s_proto);

		bsdsocklib_seterrno (ctx, sb,0);
	} else {
		SETHERRNO;
		SETERRNO;
		return;
	}
}

void host_gethostbynameaddr (TrapContext *ctx, SB, uae_u32 name, uae_u32 namelen, long addrtype)
{
	sb->name      = name;
	sb->a_addrlen = namelen;
	sb->flags     = addrtype;
	if (addrtype == -1)
		sb->action  = 4;
	else
		sb->action = 7;

	uae_sem_post (&sb->sem);

	WAITSIGNAL;
}

uae_u32 host_gethostname(TrapContext *ctx, uae_u32 name, uae_u32 namelen)
{
	if (!trap_valid_address(ctx, name, namelen))
		return -1;
	uae_char buf[256];
	trap_get_string(ctx, buf, name, sizeof buf);
	return gethostname(buf, namelen);
}
