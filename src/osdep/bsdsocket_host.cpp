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
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "autoconf.h"
#include "traps.h"
#include "../threaddep/thread.h"
#include "bsdsocket.h"
#include "native2amiga.h"

#ifndef BSDSOCKET

volatile int bsd_int_requested;

void bsdsock_fake_int_handler(void) 
{
}

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <netdb.h>

#include <signal.h>
#include <arpa/inet.h>

//#define DEBUG_BSDSOCKET
#ifdef DEBUG_BSDSOCKET
#define DEBUG_LOG write_log
#else
#define DEBUG_LOG(...) do ; while(0)
#endif

#define WAITSIGNAL	waitsig (ctx, sb)
#ifdef ANDROID
#define IPPROTO_ENCAP 98
#endif

/* Sigqueue is unsafe on SMP machines.
 * Temporary work-around.
 */
//#define SETSIGNAL	addtosigqueue (sb, 0)
#define SETSIGNAL \
  do { \
  	uae_Signal (sb->ownertask, sb->sigstosend | ((uae_u32) 1) << sb->signal); \
	  sb->dosignal = 1; \
  } while (0)


#define SETERRNO	bsdsocklib_seterrno (ctx, sb,mapErrno (errno))
#define SETHERRNO	bsdsocklib_setherrno (ctx, sb, h_errno)


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

static uae_sem_t sem_queue = 0;

/**
 ** Helper functions
 **/

/*
 * Map host errno to amiga errno
 */
static int mapErrno (int e)
{
  switch (e) {
  	case EINTR:		e = 4;  break;
  	case EDEADLK:		e = 11; break;
  	case EAGAIN:		e = 35; break;
  	case EINPROGRESS:	e = 36; break;
  	case EALREADY:		e = 37; break;
  	case ENOTSOCK:		e = 38; break;
  	case EDESTADDRREQ:	e = 39; break;
  	case EMSGSIZE:		e = 40; break;
  	case EPROTOTYPE:	e = 41; break;
  	case ENOPROTOOPT:	e = 42; break;
  	case EPROTONOSUPPORT:	e = 43; break;
  	case ESOCKTNOSUPPORT:	e = 44; break;
  	case EOPNOTSUPP:	e = 45;	break;
  	case EPFNOSUPPORT:	e = 46; break;
  	case EAFNOSUPPORT:	e = 47; break;
  	case EADDRINUSE:	e = 48; break;
  	case EADDRNOTAVAIL:	e = 49; break;
  	case ENETDOWN:		e = 50; break;
  	case ENETUNREACH:	e = 51; break;
  	case ENETRESET:		e = 52; break;
  	case ECONNABORTED:	e = 53; break;
  	case ECONNRESET:	e = 54; break;
  	case ENOBUFS:		e = 55; break;
  	case EISCONN:		e = 56; break;
  	case ENOTCONN:		e = 57; break;
  	case ESHUTDOWN:		e = 58; break;
  	case ETOOMANYREFS:	e = 59; break;
  	case ETIMEDOUT:		e = 60; break;
  	case ECONNREFUSED:	e = 61; break;
  	case ELOOP:		e = 62; break;
  	case ENAMETOOLONG:	e = 63; break;
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
	    DEBUG_LOG ("Unknown sockopt level %d\n", level);
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
    		case  0x0100:
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
          DEBUG_LOG("Invalid setsockopt option 0x%x for level %d\n",
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
  		    DEBUG_LOG ("Invalid setsockopt option 0x%x for level %d\n",
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
		      DEBUG_LOG ("Invalid setsockopt option 0x%x for level %d\n",
			      optname, level);
  		    return -1;
	    }
	    break;

	  default:
	    DEBUG_LOG ("Unknown level %d\n", level);
	    return -1;
  }
}


/*
 * Map amiga (s|g)etsockopt return value into the correct form
 */
static void mapsockoptreturn(TrapContext *ctx, int level, int optname, uae_u32 optval, void *buf)
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
	  	    trap_put_long (ctx, optval, *static_cast<int *>(buf));
  		    break;

		    case SO_ERROR:
  		    DEBUG_LOG("New errno is %d\n", mapErrno(*(int *)buf));
  		    trap_put_long (ctx, optval, mapErrno(*static_cast<int *>(buf)));
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
  		    trap_put_long (ctx, optval, *static_cast<int *>(buf));
	  	    break;

		    default:
		      break;
	    }
	    break;

  	case IPPROTO_TCP:
	    switch (optname) {
    		case TCP_NODELAY:
    		case TCP_MAXSEG:
  		    trap_put_long (ctx, optval,*static_cast<int *>(buf));
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
static void mapsockoptvalue(TrapContext *ctx, int level, int optname, uae_u32 optval, void *buf)
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
  		    *static_cast<int *>(buf) = trap_get_long (ctx, optval);
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
  		    *static_cast<int *>(buf) = trap_get_long (ctx, optval);
	  	    break;

		    default:
		      break;
	    }
	    break;

  	case IPPROTO_TCP:
	    switch (optname) {
    		case TCP_NODELAY:
		    case TCP_MAXSEG:
		      *static_cast<int *>(buf) = trap_get_long (ctx, optval);
		      break;

    		default:
		      break;
	    }
	    break;

	  default:
	    break;
  }
}


STATIC_INLINE void fd_zero (TrapContext *ctx, uae_u32 fdset, uae_u32 nfds)
{
  unsigned int i;

  for (i = 0; i < nfds; i += 32, fdset += 4)
  	trap_put_long(ctx, fdset, 0);
}

STATIC_INLINE int bsd_amigaside_FD_ISSET (TrapContext *ctx, int n, uae_u32 set)
{
  uae_u32 foo = trap_get_long(ctx, set + (n / 32));
  if (foo & (1 << (n % 32)))
		return 1;
  return 0;
}

STATIC_INLINE void bsd_amigaside_FD_ZERO (TrapContext *ctx, uae_u32 set)
{
  trap_put_long(ctx, set, 0);
  trap_put_long(ctx, set + 4, 0);
}

STATIC_INLINE void bsd_amigaside_FD_SET (TrapContext *ctx, int n, uae_u32 set)
{
  set = set + (n / 32);
  trap_put_long(ctx, set, trap_get_long(ctx, set) | (1 << (n % 32)));
}

#ifdef DEBUG_BSDSOCKET
static void printSockAddr (struct sockaddr_in *in)
{
  DEBUG_LOG ("Family %d, ", in->sin_family);
  DEBUG_LOG ("Port %d,",    ntohs (in->sin_port));
  DEBUG_LOG ("Address %s,", inet_ntoa (in->sin_addr));
}
#else
#define printSockAddr(sockAddr)
#endif

/*
 * Copy a sockaddr object from amiga space to native space
 */
static int copysockaddr_a2n (TrapContext *ctx, struct sockaddr_in *addr, uae_u32 a_addr, unsigned int len)
{
  if ((len > sizeof (struct sockaddr_in)) || (len < 8))
  	return 1;

  if (a_addr == 0)
		return 0;

  addr->sin_family      = trap_get_byte (ctx, a_addr + 1);
  addr->sin_port        = htons (trap_get_word (ctx, a_addr + 2));
  addr->sin_addr.s_addr = htonl (trap_get_long (ctx, a_addr + 4));

  if (len > 8)
		memcpy (&addr->sin_zero, get_real_address (a_addr + 8), len - 8);   /* Pointless? */
  
  return 0;
}

/*
 * Copy a sockaddr object from native space to amiga space
 */
static int copysockaddr_n2a (TrapContext *ctx, uae_u32 a_addr, const struct sockaddr_in *addr, unsigned int len)
{
  if (len < 8)
		return 1;

  if (a_addr == 0)
		return 0;

  trap_put_byte (ctx, a_addr, 0);                       /* Anyone use this field? */
  trap_put_byte (ctx, a_addr + 1, addr->sin_family);
  trap_put_word (ctx, a_addr + 2, ntohs (addr->sin_port));
  trap_put_long (ctx, a_addr + 4, ntohl (addr->sin_addr.s_addr));

  if (len > 8)
		memset (get_real_address (a_addr + 8), 0, len - 8);

  return 0;
}

/*
 * Copy a hostent object from native space to amiga space
 */
static void copyHostent (const struct hostent *hostent, SB)
{
  int size = 28;
  int i;
  int numaddr = 0;
  int numaliases = 0;
  uae_u32 aptr;

  if (hostent->h_name != NULL)
		size += strlen(hostent->h_name)+1;

  if (hostent->h_aliases != NULL)
  	while (hostent->h_aliases[numaliases])
	    size += strlen(hostent->h_aliases[numaliases++]) + 5;

  if (hostent->h_addr_list != NULL) {
  	while (hostent->h_addr_list[numaddr])
	    numaddr++;
	  size += numaddr*(hostent->h_length+4);
  }

  aptr = sb->hostent + 28 + numaliases * 4 + numaddr * 4;

  // transfer hostent to Amiga memory
  trap_put_long (sb->context, sb->hostent + 4, sb->hostent + 20);
  trap_put_long (sb->context, sb->hostent + 8, hostent->h_addrtype);
  trap_put_long (sb->context, sb->hostent + 12, hostent->h_length);
  trap_put_long (sb->context, sb->hostent + 16, sb->hostent + 24 + numaliases*4);

  for (i = 0; i < numaliases; i++)
  	trap_put_long (sb->context, sb->hostent + 20 + i * 4, addstr (sb->context, &aptr, hostent->h_aliases[i]));
  trap_put_long (sb->context, sb->hostent + 20 + numaliases * 4, 0);

  for (i = 0; i < numaddr; i++) {
		trap_put_long (sb->context, sb->hostent + 24 + (numaliases + i) * 4, addmem (sb->context, &aptr, hostent->h_addr_list[i], hostent->h_length));
  }
  trap_put_long (sb->context, sb->hostent + 24 + numaliases * 4 + numaddr * 4, 0);
  trap_put_long (sb->context, sb->hostent, aptr);
  addstr (sb->context, &aptr, hostent->h_name);

  BSDTRACE (("OK (%s)\n",hostent->h_name));
  bsdsocklib_seterrno (sb->context, sb, 0);
}

/*
 * Copy a protoent object from native space to Amiga space
 */
static void copyProtoent (TrapContext *ctx, SB, const struct protoent *p)
{
  size_t size = 16;
  int numaliases = 0;
  int i;
  uae_u32 aptr;

  // compute total size of protoent
  if (p->p_name != NULL)
		size += strlen (p->p_name) + 1;

  if (p->p_aliases != NULL)
  	while (p->p_aliases[numaliases])
	    size += strlen (p->p_aliases[numaliases++]) + 5;

  if (sb->protoent) {
	  uae_FreeMem (ctx, sb->protoent, sb->protoentsize, sb->sysbase);
  }

  sb->protoent = uae_AllocMem (ctx, size, 0, sb->sysbase);

  if (!sb->protoent) {
		write_log ("BSDSOCK: WARNING - copyProtoent() ran out of Amiga memory (couldn't allocate %d bytes)\n", size);
		bsdsocklib_seterrno (ctx, sb, 12); // ENOMEM
		return;
  }

  sb->protoentsize = size;

  aptr = sb->protoent + 16 + numaliases * 4;

  // transfer protoent to Amiga memory
  trap_put_long (ctx, sb->protoent + 4, sb->protoent + 12);
  trap_put_long (ctx, sb->protoent + 8, p->p_proto);

  for (i = 0; i < numaliases; i++)
  	trap_put_long (ctx, sb->protoent + 12 + i * 4, addstr (ctx, &aptr, p->p_aliases[i]));
  trap_put_long (ctx, sb->protoent + 12 + numaliases * 4, 0);
  trap_put_long (ctx, sb->protoent, aptr);
  addstr (ctx, &aptr, p->p_name);
  bsdsocklib_seterrno(ctx, sb, 0);
}



uae_u32 bsdthr_WaitSelect (SB)
{
  fd_set sets [3];
  int i, s, set, a_s, max;
  uae_u32 a_set;
  struct timeval tv;
  int r;

  DEBUG_LOG ("WaitSelect: %d 0x%x 0x%x 0x%x 0x%x 0x%x\n", sb->nfds, sb->sets [0], sb->sets [1], sb->sets [2], sb->timeout, sb->sigmp);

  if (sb->timeout)
  	DEBUG_LOG ("WaitSelect: timeout %d %d\n", trap_get_long (sb->context, sb->timeout), trap_get_long (sb->context, sb->timeout + 4));

  FD_ZERO (&sets [0]);
  FD_ZERO (&sets [1]);
  FD_ZERO (&sets [2]);

  /* Set up the abort socket */
  FD_SET (sb->sockabort[0], &sets[0]);
  FD_SET (sb->sockabort[0], &sets[2]);
  max = sb->sockabort[0];

  for (set = 0; set < 3; set++) {
		if (sb->sets [set] != 0) {
	    a_set = sb->sets [set];
	    for (i = 0; i < sb->nfds; i++) {
  			if (bsd_amigaside_FD_ISSET (sb->context, i, a_set)) {
			    s = getsock (sb->context, sb, i + 1);
			    DEBUG_LOG ("WaitSelect: AmigaSide %d set. NativeSide %d.\n", i, s);
			    if (s == -1) {
	  				write_log ("BSDSOCK: WaitSelect() called with invalid descriptor %d in set %d.\n", i, set);
			    } else {
		  			FD_SET (s, &sets [set]);
			  		if (max < s) max = s;
			    }
			  }
	    }
		}
  }

  max++;

  if (sb->timeout) {
		tv.tv_sec  = trap_get_long (sb->context, sb->timeout);
		tv.tv_usec = trap_get_long (sb->context, sb->timeout + 4);
  }

  DEBUG_LOG("Select going to select\n");
  r = select (max, &sets [0], &sets [1], &sets [2], (sb->timeout == 0) ? NULL : &tv);
  DEBUG_LOG("Select returns %d, errno is %d\n", r, errno);
  if( r > 0 ) {
		/* Socket told us to abort */
		if (FD_ISSET (sb->sockabort[0], &sets[0])) {
	    /* read from the pipe to reset it */
	    DEBUG_LOG ("WaitSelect aborted from signal\n");
	    r = 0;
	    for (set = 0; set < 3; set++)
		    if (sb->sets [set] != 0)
    			bsd_amigaside_FD_ZERO (sb->context, sb->sets [set]);
	    clearsockabort (sb);
		}
  	else
  		/* This is perhaps slightly inefficient, but I don't care.. */
  		for (set = 0; set < 3; set++) {
		    a_set = sb->sets [set];
		    if (a_set != 0) {
  				bsd_amigaside_FD_ZERO (sb->context, a_set);
	  			for (i = 0; i < sb->nfds; i++) {
				    a_s = getsock (sb->context, sb, i + 1);
				    if (a_s != -1) {
		  				if (FD_ISSET (a_s, &sets [set])) {
			  				DEBUG_LOG ("WaitSelect: NativeSide %d set. AmigaSide %d.\n", a_s, i);

				  			bsd_amigaside_FD_SET (sb->context, i, a_set);
					    }
					  }
				  }
			  }
		  }
  } else if (r == 0) {         /* Timeout. I think we're supposed to clear the sets.. */
		for (set = 0; set < 3; set++)
	    if (sb->sets [set] != 0)
    		bsd_amigaside_FD_ZERO (sb->context, sb->sets [set]);
  }
  DEBUG_LOG ("WaitSelect: r=%d errno=%d\n", r, errno);
  return r;
}

uae_u32 bsdthr_Accept_2 (SB)
{
  int foo, s, s2;
  long flags;
  struct sockaddr_in addr;
  socklen_t hlen = sizeof (struct sockaddr_in);

  if ((s = accept (sb->s, reinterpret_cast<struct sockaddr *>(&addr), &hlen)) >= 0) {
  	if ((flags = fcntl (s, F_GETFL)) == -1)
	    flags = 0;
  	fcntl (s, F_SETFL, flags & ~O_NONBLOCK); /* @@@ Don't do this if it's supposed to stay nonblocking... */
  	s2 = getsd (sb->context, sb, s);
  	sb->ftable[s2-1] = sb->ftable[sb->len];	/* new socket inherits the old socket's properties */
  	DEBUG_LOG ("Accept: AmigaSide %d, NativeSide %d, len %d(%d)", sb->resultval, s, hlen, trap_get_long (sb->context, sb->a_addrlen));
  	printSockAddr (&addr);
  	foo = trap_get_long (sb->context, sb->a_addrlen);
  	if (foo > 16)
      trap_put_long (sb->context, sb->a_addrlen, 16);
  	copysockaddr_n2a (sb->context, sb->a_addr, &addr, foo);
  	return s2 - 1;
  } else {
		return -1;
  }
}

uae_u32 bsdthr_Recv_2 (SB)
{
  int foo;
  if (sb->from == 0) {
  	foo = recv (sb->s, sb->buf, sb->len, sb->flags /*| MSG_NOSIGNAL*/);
	  DEBUG_LOG ("recv2, recv returns %d, errno is %d\n", foo, errno);
  } else {
  	struct sockaddr_in addr;
  	socklen_t l = sizeof (struct sockaddr_in);
  	int i = trap_get_long (sb->context, sb->fromlen);
  	copysockaddr_a2n (sb->context, &addr, sb->from, i);
  	foo = recvfrom (sb->s, sb->buf, sb->len, sb->flags | MSG_NOSIGNAL, (struct sockaddr *)&addr, &l);
  	DEBUG_LOG ("recv2, recvfrom returns %d, errno is %d\n", foo, errno);
  	if (foo >= 0) {
	    copysockaddr_n2a (sb->context, sb->from, &addr, l);
	    trap_put_long (sb->context, sb->fromlen, l);
  	}
  }
  return foo;
}

uae_u32 bsdthr_Send_2 (SB)
{
  if (sb->to == 0) {
    return send (sb->s, sb->buf, sb->len, sb->flags | MSG_NOSIGNAL);
  } else {
		struct sockaddr_in addr;
		int l = sizeof (struct sockaddr_in);
		copysockaddr_a2n (sb->context, &addr, sb->to, sb->tolen);
    return sendto (sb->s, sb->buf, sb->len, sb->flags | MSG_NOSIGNAL, (struct sockaddr *)&addr, l);
  }
}

uae_u32 bsdthr_Connect_2 (SB)
{
  if (sb->action == 1) {
		struct sockaddr_in addr;
		int len = sizeof (struct sockaddr_in);
		int retval;
		copysockaddr_a2n (sb->context, &addr, sb->a_addr, sb->a_addrlen);
		retval = connect (sb->s, reinterpret_cast<struct sockaddr *>(&addr), len);
		DEBUG_LOG ("Connect returns %d, errno is %d\n", retval, errno);
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
		  DEBUG_LOG("Connect status is %d\n", foo);
		  return (foo == 0) ? 0 : -1;
		}
  	return -1;
  }
}

uae_u32 bsdthr_SendRecvAcceptConnect (uae_u32 (*tryfunc)(SB), SB)
{
  return bsdthr_blockingstuff (tryfunc, sb);
}

uae_u32 bsdthr_blockingstuff (uae_u32 (*tryfunc)(SB), SB)
{
  int done = 0, foo;
  long flags;
  int nonblock;
  if ((flags = fcntl (sb->s, F_GETFL)) == -1)
  	flags = 0;
  nonblock = (flags & O_NONBLOCK);
  fcntl (sb->s, F_SETFL, flags | O_NONBLOCK);
  while (!done) {
  	done = 1;
	  foo = tryfunc (sb);
	  if (foo < 0 && !nonblock) {
	    if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS)) {
    		fd_set readset, writeset, exceptset;
    		int maxfd = (sb->s > sb->sockabort[0]) ? sb->s : sb->sockabort[0];
    		int num;
    
    		FD_ZERO (&readset);
    		FD_ZERO (&writeset);
    		FD_ZERO (&exceptset);

    		if (sb->action == 3 || sb->action == 6)
		      FD_SET (sb->s, &readset);
		    if (sb->action == 2 || sb->action == 1 || sb->action == 4)
		      FD_SET (sb->s, &writeset);
		    FD_SET (sb->sockabort[0], &readset);

    		num = select (maxfd + 1, &readset, &writeset, &exceptset, NULL);
		    if (num == -1) {
		      DEBUG_LOG ("Blocking select(%d) returns -1,errno is %d\n", sb->sockabort[0],errno);
		      fcntl (sb->s, F_SETFL, flags);
		      return -1;
    		}

    		if (FD_ISSET (sb->sockabort[0], &readset) || FD_ISSET (sb->sockabort[0], &writeset)) {
		      /* reset sock abort pipe */
		      /* read from the pipe to reset it */
		      DEBUG_LOG ("select aborted from signal\n");

  		    clearsockabort (sb);
	  	    DEBUG_LOG ("Done read\n");
		      errno = EINTR;
		      done = 1;
		    } else {
		      done = 0;
		    }
	    } else if (errno == EINTR)
		    done = 1;
	  }
  }
  fcntl (sb->s, F_SETFL, flags);
  return foo;
}


static void *bsdlib_threadfunc (void *arg)
{
  struct socketbase *sb = static_cast<struct socketbase *>(arg);

  DEBUG_LOG ("THREAD_START\n");

  while (1) {
  	uae_sem_wait (&sb->sem);
    TrapContext *ctx = sb->context;

	  DEBUG_LOG ("Socket thread got action %d (sb=0x%08x)\n", sb->action, sb);

  	switch (sb->action) {
	    case 0:       /* kill thread (CloseLibrary) */
	    	DEBUG_LOG ("THREAD_END\n");
    		uae_sem_destroy (&sb->sem);
    		return NULL;

	    case 1:       /* Connect */
    		sb->resultval = bsdthr_SendRecvAcceptConnect (bsdthr_Connect_2, sb);
		    break;

	    /* @@@ Should check (from|to)len so it's 16.. */
	    case 2:       /* Send[to] */
    		sb->resultval = bsdthr_SendRecvAcceptConnect (bsdthr_Send_2, sb);
		    break;

	    case 3:       /* Recv[from] */
    		sb->resultval = bsdthr_SendRecvAcceptConnect (bsdthr_Recv_2, sb);
		    break;

	    case 4:       /* Gethostbyname */
        {
    		  struct hostent *tmphostent = gethostbyname (reinterpret_cast<char *>(get_real_address(sb->name)));
    		  if (tmphostent) {
  		      copyHostent (tmphostent, sb);
  		      bsdsocklib_setherrno (ctx, sb, 0);
    		  } else
  		      SETHERRNO;

    		  break;
	      }

	    case 5:       /* WaitSelect */
    		sb->resultval = bsdthr_WaitSelect (sb);
		    break;

	    case 6:       /* Accept */
    		sb->resultval = bsdthr_SendRecvAcceptConnect (bsdthr_Accept_2, sb);
		    break;

	    case 7: 
        {
      		struct hostent *tmphostent = gethostbyaddr (reinterpret_cast<const char*>(get_real_address(sb->name)), sb->a_addrlen, sb->flags);

		      if (tmphostent) {
		        copyHostent (tmphostent, sb);
		        bsdsocklib_setherrno (ctx, sb, 0);
		      } else
		        SETHERRNO;

      		break;
	      }
  	}
	  SETERRNO;
	  SETSIGNAL;
  }

  return NULL;        /* Just to keep GCC happy.. */
}


void host_connect (TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  sb->s = getsock (ctx, sb, sd + 1);
  if (sb->s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
  }
  sb->a_addr    = name;
  sb->a_addrlen = namelen;
  sb->action    = 1;
  sb->context   = ctx;
  
  uae_sem_post (&sb->sem);

  WAITSIGNAL;
}

void host_sendto (TrapContext *ctx, SB, uae_u32 sd, uae_u32 msg, uae_u8* hmsg, uae_u32 len, uae_u32 flags, uae_u32 to, uae_u32 tolen)
{
  sb->s = getsock (ctx, sb, sd + 1);
  if (sb->s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
  }
  if(hmsg == NULL)
    sb->buf    = get_real_address (msg);
  else
    sb->buf = hmsg;
  sb->len    = len;
  sb->flags  = flags;
  sb->to     = to;
  sb->tolen  = tolen;
  sb->action = 2;
  sb->context = ctx;

  uae_sem_post (&sb->sem);

  WAITSIGNAL;
}

void host_recvfrom (TrapContext *ctx, SB, uae_u32 sd, uae_u32 msg, uae_u8* hmsg, uae_u32 len, uae_u32 flags, uae_u32 addr, uae_u32 addrlen)
{
  int s = getsock (ctx, sb, sd + 1);

  DEBUG_LOG ("Recv[from](%p, %x, %x, %d, %x, %x, %u)\n",
    sb, sd, msg, len, flags, addr, addrlen);

  if (s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
  }

  sb->s      = s;
  if(hmsg == NULL)
    sb->buf    = get_real_address (msg);
  else
    sb->buf = hmsg;
  sb->len    = len;
  sb->flags  = flags;
  sb->from   = addr;
  sb->fromlen= addrlen;
  sb->action = 3;
  sb->context = ctx;

  uae_sem_post (&sb->sem);

  WAITSIGNAL;
}

void host_setsockopt (SB, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 optlen)
{
	TrapContext *ctx = NULL;
  int s = getsock (ctx, sb, sd + 1);
  int nativelevel = mapsockoptlevel (level);
  int nativeoptname = mapsockoptname(nativelevel, optname);
  void *buf;
  if (s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
  }

  if (optval) {
		buf = malloc(optlen);
		mapsockoptvalue(ctx, nativelevel, nativeoptname, optval, buf);
  } else {
		buf = NULL;
  }
  sb->resultval  = setsockopt (s, nativelevel, nativeoptname, buf, optlen);
  if (buf)
    free(buf);
  SETERRNO;

  DEBUG_LOG ("setsockopt: sock %d, level %d, 'name' %d(%d), len %d -> %d, %d\n",
	  s, level, optname, nativeoptname, optlen,
	  sb->resultval, errno);
}

uae_u32 host_getsockname (TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  int s;
  socklen_t len = sizeof (struct sockaddr_in);
  struct sockaddr_in addr;

  DEBUG_LOG ("getsockname(%u, 0x%x, %u) -> ", sd, name, len);

  s = getsock (ctx, sb, sd + 1);

  if (s != -1) {
  	if (getsockname (s, (struct sockaddr *)&addr, &len)) {
			SETERRNO;
			DEBUG_LOG ("failed (%d)\n", sb->sb_errno);
    } else {
			int a_nl;
			DEBUG_LOG ("okay\n");
			a_nl = trap_get_long (ctx, namelen);
  		if (!trap_valid_address(ctx, name, a_nl))
  			return -1;
			copysockaddr_n2a (ctx, name, &addr, a_nl);
			if (a_nl > 16)
	      trap_put_long (ctx, namelen, 16);
		  return 0;
	  }
	}

	return -1;
}

uae_u32 host_getpeername (TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  int s;
  socklen_t len = sizeof (struct sockaddr_in);
  struct sockaddr_in addr;

  DEBUG_LOG ("getpeername(%u, 0x%x, %u) -> ", sd, name, len);

  s = getsock (ctx, sb, sd + 1);

  if (s != -1) {
		if (getpeername (s, (struct sockaddr *)&addr, &len)) {
			SETERRNO;
			DEBUG_LOG ("failed (%d)\n", sb->sb_errno);
		} else {
	    int a_nl;
	    DEBUG_LOG ("okay\n");
	    a_nl = trap_get_long (ctx, namelen);
  		if (!trap_valid_address(ctx, name, a_nl))
  			return -1;
	    copysockaddr_n2a (ctx, name, &addr, a_nl);
	    if (a_nl > 16)
				trap_put_long (ctx, namelen, 16);
	    return 0;
		}
  }

  return -1;
}

void host_gethostbynameaddr (TrapContext *ctx, SB, uae_u32 name, uae_u32 namelen, long addrtype)
{
  sb->name      = name;
  sb->a_addrlen = namelen;
  sb->flags     = addrtype;
  if (addrtype == -1)
		sb->action  = 4;
  else
		sb->action  = 7;
  sb->context   = ctx;

  uae_sem_post (&sb->sem);

  WAITSIGNAL;
}

void host_WaitSelect (TrapContext *ctx, SB, uae_u32 nfds, uae_u32 readfds, uae_u32 writefds, uae_u32 exceptfds,
  uae_u32 timeout, uae_u32 sigmp)
{
  uae_u32 wssigs = (sigmp) ? trap_get_long (ctx, sigmp) : 0;
  uae_u32 sigs;

  if (wssigs) {
		trap_call_add_dreg (ctx, 0, 0);
		trap_call_add_dreg (ctx, 1, wssigs);
		sigs = trap_call_lib (ctx, trap_get_long (ctx, 4), -0x132) & wssigs;	// SetSignal()
		if (sigs) {
	    DEBUG_LOG ("WaitSelect preempted by signals 0x%08x\n", sigs & wssigs);
	    trap_put_long (ctx, sigmp, sigs);
	    // Check for zero address -> otherwise WinUAE crashes
	    if (readfds)   fd_zero (ctx, readfds, nfds);
	    if (writefds)  fd_zero (ctx, writefds, nfds);
	    if (exceptfds) fd_zero (ctx, exceptfds, nfds);
	    sb->resultval = 0;
	    bsdsocklib_seterrno (ctx, sb, 0);
	    return;
		}
  }

  if (nfds == 0) {
		/* No sockets - Just wait on signals */
		trap_call_add_dreg (ctx, 0, wssigs);
		sigs = trap_call_lib (ctx, trap_get_long (ctx, 4), -0x13e);	// Wait()

		if (sigmp)
			trap_put_long (ctx, sigmp, sigs & wssigs);

		if (readfds)   fd_zero (ctx, readfds, nfds);
		if (writefds)  fd_zero (ctx, writefds, nfds);
		if (exceptfds) fd_zero (ctx, exceptfds, nfds);
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
  sb->context  = ctx;

  uae_sem_post (&sb->sem);

  trap_call_add_dreg (ctx, 0, (((uae_u32)1) << sb->signal) | sb->eintrsigs | wssigs);
  sigs = trap_call_lib (ctx, trap_get_long (ctx, 4), -0x13e); // Wait()

  if (sigmp)
		trap_put_long (ctx, sigmp, sigs & (sb->eintrsigs | wssigs));

  if (sigs & wssigs) {
		/* Received the signals we were waiting on */
    DEBUG_LOG ("WaitSelect: got signal(s) %x\n", sigs);

		if (!(sigs & (((uae_u32)1) << sb->signal))) {
	    sockabort (sb);
	    WAITSIGNAL;
		}

		sb->resultval = 0;
		if (readfds)   fd_zero (ctx, readfds, nfds);
		if (writefds)  fd_zero (ctx, writefds, nfds);
		if (exceptfds) fd_zero (ctx, exceptfds, nfds);

		bsdsocklib_seterrno (ctx, sb, 0);
  } else if (sigs & sb->eintrsigs) {
		/* Wait select was interrupted */
		DEBUG_LOG ("WaitSelect: interrupted\n");

		if (!(sigs & (((uae_u32)1) << sb->signal))) {
	    sockabort (sb);
	    WAITSIGNAL;
		}

		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, mapErrno (EINTR));
  }
  clearsockabort(sb);
}



void host_accept (TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  sb->s = getsock (ctx, sb, sd + 1);
  if (sb->s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return;
  }

	if (name != 0) {
		if (!trap_valid_address(ctx, name, sizeof(struct sockaddr)) || !trap_valid_address(ctx, namelen, 4))
			return;
  }

  DEBUG_LOG ("accept(%d, %x, %x)\n", sb->s, name, namelen);
  sb->a_addr    = name;
  sb->a_addrlen = namelen;
  sb->action    = 6;
  sb->len       = sd;
  sb->context   = ctx;

  uae_sem_post (&sb->sem);

  WAITSIGNAL;
  DEBUG_LOG ("Accept returns %d\n", sb->resultval);
}

int host_socket (TrapContext *ctx, SB, int af, int type, int protocol)
{
  int sd;
  int s;

  DEBUG_LOG ("socket(%s,%s,%d) -> ",af == AF_INET ? "AF_INET" : "AF_other",
    type == SOCK_STREAM ? "SOCK_STREAM" : type == SOCK_DGRAM ?
    "SOCK_DGRAM " : "SOCK_RAW", protocol);

  if ((s = socket (af, type, protocol)) == -1)  {
		SETERRNO;
    DEBUG_LOG ("failed (%d)\n", sb->sb_errno);
		return -1;
  } else {
		int arg = 1;
		sd = getsd (ctx, sb, s);
		setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg));
  }

  sb->ftable[sd-1] = SF_BLOCKING;
  DEBUG_LOG ("socket returns Amiga %d, NativeSide %d\n", sd - 1, s);
  return sd - 1;
}

uae_u32 host_bind (TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  uae_u32 success = 0;
  struct sockaddr_in addr;
  int len = sizeof (struct sockaddr_in);
  int s;

  s = getsock (ctx, sb, sd + 1);
  if (s == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return -1;
  }

  DEBUG_LOG ("bind(%u[%d], 0x%x, %u) -> ", sd, s, name, namelen);
  copysockaddr_a2n (ctx, &addr, name, namelen);
  printSockAddr (&addr);
  if ((success = bind (s, reinterpret_cast<struct sockaddr *>(&addr), len)) != 0) {
		SETERRNO;
		DEBUG_LOG ("failed (%d)\n",sb->sb_errno);
  } else {
		DEBUG_LOG ("OK\n");
  }
  return success;
}

uae_u32 host_listen (TrapContext *ctx, SB, uae_u32 sd, uae_u32 backlog)
{
  int s;
  uae_u32 success = -1;

  DEBUG_LOG ("listen(%d,%d) -> ", sd, backlog);
  s = getsock (ctx, sb, sd + 1);

  if (s == -1) {
		bsdsocklib_seterrno (ctx, sb, 9);
		return -1;
  }

  if ((success = listen (s, backlog)) != 0) {
		SETERRNO;
		DEBUG_LOG ("failed (%d)\n", sb->sb_errno);
  } else {
		DEBUG_LOG ("OK\n");
  }
  return success;
}

void host_getprotobyname (TrapContext *ctx, SB, uae_u32 name)
{
  struct protoent *p = getprotobyname (reinterpret_cast<char *>(get_real_address(name)));

  DEBUG_LOG ("Getprotobyname(%s) = %p\n", get_real_address (name), p);

  if (p == NULL) {
		SETERRNO;
		return;
  }

  copyProtoent (ctx, sb, p);
  BSDTRACE (("OK (%s, %d)\n", p->p_name, p->p_proto));
}

void host_getprotobynumber (TrapContext *ctx, SB, uae_u32 number)
{
  struct protoent *p = getprotobynumber(number);
  DEBUG_LOG("getprotobynumber(%d) = %p\n", number, p);

  if (p == NULL) {
		SETERRNO;
		return;
  }

  copyProtoent (ctx, sb, p);
  BSDTRACE (("OK (%s, %d)\n", p->p_name, p->p_proto));
}

void host_getservbynameport (TrapContext *ctx, SB, uae_u32 name, uae_u32 proto, uae_u32 type)
{
  uae_char protobuf[256];
  uae_char namebuf[256];
	if (proto) {
		if (trap_valid_address(ctx, proto, 1)) {
			trap_get_string(ctx, protobuf, proto, sizeof protobuf);
		}
	}
  if(!type) {
		if (trap_valid_address(ctx, name, 1)) {
			trap_get_string(ctx, namebuf, name, sizeof namebuf);
		}
  }
  
  struct servent *s = (type) ?
  	getservbyport (name, protobuf) :
	  getservbyname (namebuf, protobuf);
  size_t size = 20;
  int numaliases = 0;
  uae_u32 aptr;
  int i;

  if (type) {
    DEBUG_LOG("Getservbyport(%d, %s) = %p\n", name, protobuf, s);
  } else {
    DEBUG_LOG("Getservbyname(%s, %s) = %p\n", namebuf, protobuf, s);
  }

  if (s == NULL) {
  	SETERRNO;
	  return;
  }

  // compute total size of servent
  if (s->s_name != NULL)
		size += strlen (s->s_name) + 1;

  if (s->s_proto != NULL)
		size += strlen (s->s_proto) + 1;

  if (s->s_aliases != NULL)
  	while (s->s_aliases[numaliases])
	    size += strlen (s->s_aliases[numaliases++]) + 5;

  if (sb->servent) {
		uae_FreeMem (ctx, sb->servent, sb->serventsize, sb->sysbase);
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
  trap_put_long (ctx, sb->servent + 4, sb->servent + 16);
  trap_put_long (ctx, sb->servent + 8, (unsigned short)htons (s->s_port));

  for (i = 0; i < numaliases; i++)
  	trap_put_long (ctx, sb->servent + 16 + i * 4, addstr (ctx, &aptr, s->s_aliases[i]));
  trap_put_long (ctx, sb->servent + 16 + numaliases * 4, 0);
  trap_put_long (ctx, sb->servent, aptr);
  addstr (ctx, &aptr, s->s_name);
  trap_put_long (ctx, sb->servent + 12, aptr);
  addstr (ctx, &aptr, s->s_proto);

  BSDTRACE (("OK (%s, %d)\n", s->s_name, (unsigned short)htons (s->s_port)));
  bsdsocklib_seterrno (ctx, sb,0);
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
  if (uae_start_thread ("bsdsocket", bsdlib_threadfunc, static_cast<void *>(sb), &sb->thread) == BAD_THREAD) {
		write_log ("BSDSOCK: Failed to create thread.\n");
		uae_sem_destroy (&sb->sem);
		close (sb->sockabort[0]);
		close (sb->sockabort[1]);
		return 0;
  }
  return 1;
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
  uae_wait_thread (thread);
}

void host_sbreset (void)
{
  /* TODO */
}

uae_u32 host_Inet_NtoA (TrapContext *ctx, SB, uae_u32 in)
{
  char *addr;
  struct in_addr ina;
  uae_u32 buf;

  *reinterpret_cast<uae_u32 *>(&ina) = htonl (in);

  BSDTRACE (("Inet_NtoA(%x) -> ", in));

  if ((addr = inet_ntoa(ina)) != NULL) {
		buf = trap_get_areg(ctx, 6) + offsetof (struct UAEBSDBase, scratchbuf);
		strncpyha(ctx, buf, addr, SCRATCHBUFSIZE);
		BSDTRACE (("%s\n", addr));
		return buf;
  } else
		SETERRNO;

  BSDTRACE (("failed (%d)\n", sb->sb_errno));

  return 0;
}

uae_u32 host_inet_addr (TrapContext *ctx, uae_u32 cp)
{
  uae_u32 addr;
  char *cp_rp;

	if (!trap_valid_address(ctx, cp, 4))
		return 0;
  cp_rp = trap_get_alloc_string(ctx, cp, 256);

  addr = htonl (inet_addr (cp_rp));

  BSDTRACE (("inet_addr(%s) -> 0x%08x\n", cp_rp, addr));
  xfree(cp_rp);

  return addr;
}

uae_u32 host_shutdown (SB, uae_u32 sd, uae_u32 how)
{
	TrapContext *ctx = NULL;
  SOCKET s;

  BSDTRACE (("shutdown(%d,%d) -> ", sd, how));
  s = getsock (ctx, sb, sd + 1);

  if (s != -1) {
		if (shutdown (s, how)) {
	    SETERRNO;
	    BSDTRACE (("failed (%d)\n", sb->sb_errno));
		} else {
	    BSDTRACE (("OK\n"));
	    return 0;
		}
  }

  return -1;
}

int host_dup2socket (TrapContext *ctx, SB, int fd1, int fd2) 
{
  int s1, s2;

  BSDTRACE (("dup2socket(%d,%d) -> ", fd1, fd2));
  fd1++;

  s1 = getsock (ctx, sb, fd1);
  if (s1 != -1) {
		if (fd2 != -1) {
	    if (static_cast<unsigned int>(fd2) >= static_cast<unsigned int>(sb->dtablesize)) {
  			BSDTRACE (("Bad file descriptor (%d)\n", fd2));
				bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
	    }
	    fd2++;
	    s2 = getsock (ctx, sb, fd2);
	    if (s2 != -1) {
				close (s2);
	    }
	    setsd (ctx, sb, fd2, dup (s1));
	    BSDTRACE (("0(%d)\n", getsock (ctx, sb, fd2)));
	    return 0;
		} else {
	    fd2 = getsd (ctx, sb, 1);
			if (fd2 != -1) {
		    setsd (ctx, sb, fd2, dup (s1));
				BSDTRACE (("%d(%d)\n", fd2, getsock (ctx, sb, fd2)));
		    return (fd2 - 1);
			} else {
				BSDTRACE(("-1\n"));
				return -1;
			}
		}
  }
  BSDTRACE (("-1\n"));
  return -1;
}

uae_u32 host_getsockopt (TrapContext *ctx, SB, uae_u32 sd, uae_u32 level, uae_u32 optname,
	uae_u32 optval, uae_u32 optlen)
{
  socklen_t len = 0;
  int r;
  int s;
  int nativelevel = mapsockoptlevel(level);
  int nativeoptname = mapsockoptname(nativelevel, optname);
  void *buf = NULL;
  s = getsock (ctx, sb, sd + 1);

  if (s == -1) {
		bsdsocklib_seterrno(ctx, sb, 9); /* EBADF */
		return -1;
  }

  if (optlen) {
		len = trap_get_long (ctx, optlen);
		buf = malloc(len);
		if (buf == NULL) {
		   return -1;
		}
  }

  r = getsockopt (s, nativelevel, nativeoptname,
  	optval ? buf : NULL, optlen ? &len : NULL);

  if (optlen)
		trap_put_long (ctx, optlen, len);

  SETERRNO;
  DEBUG_LOG ("getsockopt: sock AmigaSide %d NativeSide %d, level %d, 'name' %x(%d), len %d -> %d, %d\n",
    sd, s, level, optname, nativeoptname, len, r, errno);

  if (optval) {
		if (r == 0) {
	    mapsockoptreturn(ctx, nativelevel, nativeoptname, optval, buf);
		}
  }

  if (buf != NULL)
		free(buf);
  return r;
}

uae_u32 host_IoctlSocket (TrapContext *ctx, SB, uae_u32 sd, uae_u32 request, uae_u32 arg)
{
  int sock = getsock (ctx, sb, sd + 1);
  int r, argval = trap_get_long (ctx, arg);
  long flags;

  if (sock == -1) {
		sb->resultval = -1;
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return -1;
  }

  if ((flags = fcntl (sock, F_GETFL)) == -1) {
		SETERRNO;
		return -1;
  }

  DEBUG_LOG ("Ioctl code is %x, flags are %ld\n", request, flags);

  switch (request) {
  	case 0x8004667B: /* FIOGETOWN */
	    sb->ownertask = trap_get_long (ctx, arg);
	    return 0;

	  case 0x8004667C: /* FIOSETOWN */
	    trap_put_long (ctx, arg, sb->ownertask);
	    return 0;
	  case 0x8004667D: /* FIOASYNC */
#	ifdef O_ASYNC
	    r = fcntl (sock, F_SETFL, argval ? flags | O_ASYNC : flags & ~O_ASYNC);
	    return r;
#	else
	    /* O_ASYNC is only available on Linux and BSD systems */
      return fcntl (sock, F_GETFL);
#	endif

  	case 0x8004667E: /* FIONBIO */
	    r = fcntl (sock, F_SETFL, argval ?
        flags | O_NONBLOCK : flags & ~O_NONBLOCK);
	    if (argval) {
  			DEBUG_LOG ("nonblocking\n");
	  		sb->ftable[sd] &= ~SF_BLOCKING;
	    } else {
		  	DEBUG_LOG ("blocking\n");
			  sb->ftable[sd] |= SF_BLOCKING;
	    }
	    return r;

  	case 0x4004667F: /* FIONREAD */
	    r = ioctl (sock, FIONREAD, &flags);
	    if (r >= 0) {
	  		trap_put_long (ctx, arg, flags);
	    }
	    return r;

  } /* end switch */

  bsdsocklib_seterrno (ctx, sb, EINVAL);
  return -1;
}

int host_CloseSocket (TrapContext *ctx, SB, int sd)
{
  int s = getsock (ctx, sb, sd + 1);
  int retval;

  if (s == -1) {
		bsdsocklib_seterrno (ctx, sb, 9); /* EBADF */
		return -1;
  }

  /*
  if (checksd (sb, sd + 1) == 1) {
  	return 0;
  }
  */
  DEBUG_LOG ("CloseSocket Amiga: %d, NativeSide %d\n", sd, s);
  retval = close (s);
  SETERRNO;
  releasesock (ctx, sb, sd + 1);
  return retval;
}

void host_closesocketquick (int s)
{
  struct linger l;
  l.l_onoff = 0;
  l.l_linger = 0;
  if(s != -1) {
		setsockopt (s, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
		close (s);
  }
}

uae_u32 host_gethostname (TrapContext *ctx, uae_u32 name, uae_u32 namelen)
{
	if (!trap_valid_address(ctx, name, namelen))
		return -1;
	uae_char buf[256];
	trap_get_string(ctx, buf, name, sizeof buf);
	return gethostname(buf, namelen);
}

int init_socket_layer(void)
{
	int result = 0;

	if(sem_queue != 0) {
	  uae_sem_destroy(&sem_queue);
	  sem_queue = 0;
	}

	if (currprefs.socket_emu) {
    if (uae_sem_init(&sem_queue, 0, 1) < 0) {
  		DEBUG_LOG("Can't create sem %d\n", errno);
  		return 0;
    }
    result = 1;
  }

  return result;
}

void clearsockabort (SB)
{
  int chr;
  int num;

  while ((num = read (sb->sockabort[0], &chr, sizeof(chr))) >= 0) {
		DEBUG_LOG ("Sockabort got %d bytes\n", num);
  }
}

void sockabort (SB)
{
  int chr = 1;
  DEBUG_LOG ("Sock abort!!\n");
  write (sb->sockabort[1], &chr, sizeof (chr));
}

void locksigqueue (void)
{
  uae_sem_wait(&sem_queue);
}

void unlocksigqueue (void)
{
  uae_sem_post(&sem_queue);
}

#endif
