/*
* UAE - The Un*x Amiga Emulator
*
* bsdsocket.library emulation machine-independent part
*
* Copyright 1997, 1998 Mathias Ortmann
*
* Library initialization code (c) Tauno Taipaleenmaki
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <assert.h>
#include <stddef.h>

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "bsdsocket.h"
#include "threaddep/thread.h"
#include "native2amiga.h"
#include "debug.h"

#ifdef BSDSOCKET

#define NEWTRAP 1

int log_bsd = 0;
struct socketbase *socketbases;
static uae_u32 SockLibBase;

#define SOCKPOOLSIZE 128
#define UNIQUE_ID (-1)

/* ObtainSocket()/ReleaseSocket() public socket pool */
struct sockd {
	long sockpoolids[SOCKPOOLSIZE];
	SOCKET_TYPE sockpoolsocks[SOCKPOOLSIZE];
	uae_u32 sockpoolflags[SOCKPOOLSIZE];
};

static long curruniqid = 65536;
static struct sockd *sockdata;

uae_u32 strncpyha(TrapContext *ctx, uae_u32 dst, const uae_char *src, int size)
{
	uae_u32 res = dst;
	if (trap_is_indirect()) {
		trap_put_string(ctx, src, dst, size);
	} else {
		if (!addr_valid(_T("strncpyha"), dst, size))
			return res;
		while (size--) {
			put_byte (dst++, *src);
			if (!*src++)
				return res;
		}
	}
	return res;
}

uae_u32 addstr(TrapContext *ctx, uae_u32 * dst, const TCHAR *src)
{
	uae_u32 res = *dst;
	int len;
	char *s = ua(src);
	len = uaestrlen(s) + 1;
	if (trap_is_indirect()) {
		trap_put_bytes(ctx, dst, res, len);
	} else {
		strcpyha_safe (*dst, s);
	}
	(*dst) += len;
	xfree (s);
	return res;
}
uae_u32 addstr_ansi(TrapContext *ctx, uae_u32 * dst, const uae_char *src)
{
	uae_u32 res = *dst;
	int len;
	len = uaestrlen (src) + 1;
	if (trap_is_indirect()) {
		trap_put_bytes(ctx, dst, res, len);
	} else {
		strcpyha_safe (*dst, src);
	}
	(*dst) += len;
	return res;
}

uae_u32 addmem(TrapContext *ctx, uae_u32 * dst, const uae_char *src, int len)
{
	uae_u32 res = *dst;

	if (!src)
		return 0;
	if (trap_is_indirect()) {
		trap_put_bytes(ctx, src, res, len);
	} else {
		memcpyha_safe (*dst, (uae_u8*)src, len);
	}
	(*dst) += len;

	return res;
}

/* Get current task */
static uae_u32 gettask (TrapContext *ctx)
{
	uae_u32 currtask, a1 = trap_get_areg(ctx, 1);
	TCHAR *tskname;

	trap_call_add_areg(ctx, 1, 0);
	currtask = trap_call_lib(ctx, trap_get_long(ctx, 4), -0x126); /* FindTask */

	trap_set_areg(ctx, 1, a1);

	if (ISBSDTRACE) {
		uae_char name[256];
		trap_get_string(ctx, name, trap_get_long(ctx, currtask + 10), sizeof name);
		tskname = au(name);
		BSDTRACE ((_T("[%s] "), tskname));
		xfree (tskname);
	}
	return currtask;
}

/* errno/herrno setting */
void bsdsocklib_seterrno (TrapContext *ctx, SB, int sb_errno)
{
	sb->sb_errno = sb_errno;
	if (sb->sb_errno >= 1001 && sb->sb_errno <= 1005)
		bsdsocklib_setherrno(ctx, sb,sb->sb_errno-1000);
	if (sb->errnoptr) {
		switch (sb->errnosize) {
		case 1:
			trap_put_byte(ctx, sb->errnoptr, sb_errno);
		break;
		case 2:
			trap_put_word(ctx, sb->errnoptr, sb_errno);
		break;
		case 4:
			trap_put_long(ctx, sb->errnoptr, sb_errno);
		break;
		}
	}
}

void bsdsocklib_setherrno(TrapContext *ctx, SB, int sb_herrno)
{
	sb->sb_herrno = sb_herrno;

	if (sb->herrnoptr) {
		switch (sb->herrnosize) {
		case 1:
			trap_put_byte(ctx, sb->herrnoptr, sb_herrno);
		break;
		case 2:
			trap_put_word(ctx, sb->herrnoptr, sb_herrno);
		break;
		case 4:
			trap_put_long(ctx, sb->herrnoptr, sb_herrno);
		break;
		}
	}
}

uae_u32 callfdcallback (TrapContext *ctx, SB, uae_u32 fd, uae_u32 action)
{
	uae_u32 v;
	if (!sb->fdcallback)
		return 0;
	BSDTRACE((_T("FD_CALLBACK(%d,%d) "), fd, action));
	trap_call_add_dreg(ctx, 0, fd);
	trap_call_add_dreg(ctx, 1, action);
	v = trap_call_func(ctx, sb->fdcallback);
	BSDTRACE((_T("-> %d\n"), v));
	return v;
}

bool checksd(TrapContext *ctx, SB, int sd)
{
	int iCounter;
	SOCKET s;

	s = getsock(ctx, sb, sd);
	if (s != INVALID_SOCKET) {
		for (iCounter  = 1; iCounter <= sb->dtablesize; iCounter++) {
			if (iCounter != sd) {
				if (getsock(ctx, sb, iCounter) == s) {
					releasesock(ctx, sb, sd);
					return true;
				}
			}
		}
		for (iCounter  = 0; iCounter < SOCKPOOLSIZE; iCounter++) {
			if (s == sockdata->sockpoolsocks[iCounter])
				return true;
		}
	}
	BSDTRACE((_T("checksd FALSE s 0x%x sd %d\n"),s,sd));
	return false;
}

void setsd(TrapContext *ctx, SB, int sd, SOCKET_TYPE s)
{
	callfdcallback(ctx, sb, sd - 1, FDCB_ALLOC);
	sb->dtable[sd - 1] = s;
}

/* Socket descriptor/opaque socket handle management */
int getsd (TrapContext *ctx, SB, SOCKET_TYPE s)
{
	int i, fdcb;
	SOCKET_TYPE *dt = sb->dtable;

	/* return socket descriptor if already exists */
	for (i = sb->dtablesize; i--;) {
		if (dt[i] == s)
			return i + 1;
	}

	/* create new table entry */
	fdcb = 0;
	for (i = 0; i < sb->dtablesize; i++) {
		if (dt[i] == -1) {
			if (callfdcallback(ctx, sb, i, FDCB_CHECK)) {
				/* fd was allocated by link lib */
				dt[i] = -2;
				continue;
			}
			dt[i] = s;
			sb->ftable[i] = SF_BLOCKING;
			return i + 1;
		} else if (dt[i] == -2) {
			fdcb = 1;
		}
	}
	/* recheck callback allocated FDs */
	if (fdcb) {
		for (i = 0; i < sb->dtablesize; i++) {
			if (dt[i] == -2) {
				if (!callfdcallback(ctx, sb, i, FDCB_CHECK)) {
					dt[i] = s;
					sb->ftable[i] = SF_BLOCKING;
					return i + 1;
				}
			}
		}
	}

	/* descriptor table full. */
	bsdsocklib_seterrno(ctx, sb, 24); /* EMFILE */

	return -1;
}

SOCKET_TYPE getsock (TrapContext *ctx, SB, int sd)
{
	if ((unsigned int) (sd - 1) >= (unsigned int) sb->dtablesize) {
		BSDTRACE ((_T("Invalid Socket Descriptor (%d)\n"), sd));
		bsdsocklib_seterrno(ctx, sb, 38); /* ENOTSOCK */
		return -1;
	}
	if (sb->dtable[sd - 1] == INVALID_SOCKET) {
		struct socketbase *sb1, *nsb;
		uaecptr ot;
		uae_char name[256];

		if (!trap_valid_address(ctx, sb->ownertask + 10, 4))
			return -1;
		ot = trap_get_long(ctx, sb->ownertask + 10);
		if (!trap_valid_address(ctx, ot, 1))
			return -1;
		trap_get_string(ctx, name, ot, sizeof name);
		// Fix for Newsrog (All Tasks of Newsrog using the same dtable)
		for (sb1 = socketbases; sb1; sb1 = nsb) {
			uaecptr ot1;
			uae_char name1[256];
			if (!trap_valid_address(ctx, sb1->ownertask + 10, 4))
				break;
			ot1 = trap_get_long(ctx, sb1->ownertask + 10);
			if (!trap_valid_address(ctx, ot1, 1))
				break;
			trap_get_string(ctx, name1, ot1, sizeof name1);
			if (strcmp(name, name1) == 0) {
				// Task with same name already exists -> use same dtable
				if (sb1->dtable[sd - 1] != INVALID_SOCKET)
					return sb1->dtable[sd - 1];
			}
			nsb = sb1->next;
		}
	}
	return sb->dtable[sd - 1];
}

void releasesock (TrapContext *ctx, SB, int sd)
{
	if ((unsigned int) (sd - 1) < (unsigned int) sb->dtablesize) {
		sb->dtable[sd - 1] = -1;
		callfdcallback(ctx, sb, sd - 1, FDCB_FREE);
	}
}

/* Signal queue */
/* @@@ TODO: ensure proper interlocking */
#if 1
struct socketbase *sbsigqueue;
volatile int bsd_int_requested;
#endif

void addtosigqueue (SB, int events)
{
	locksigqueue ();

	if (events)
		sb->sigstosend |= sb->eventsigs;
	else
		sb->sigstosend |= ((uae_u32) 1) << sb->signal;

	if (!sb->dosignal) {
		sb->nextsig = sbsigqueue;
		sbsigqueue = sb;
	}
	sb->dosignal = 1;

	bsd_int_requested |= 1;

	unlocksigqueue ();
}


void bsdsock_fake_int_handler(void)
{
	locksigqueue ();

	bsd_int_requested = 0;

	if (sbsigqueue != NULL) {
		SB;

		for (sb = sbsigqueue; sb; sb = sb->nextsig) {
			if (sb->dosignal == 1) {
				uae_Signal (sb->ownertask, sb->sigstosend);
				sb->sigstosend = 0;
			}
			sb->dosignal = 0;
		}

		sbsigqueue = NULL;
	}

	unlocksigqueue ();
}

void waitsig (TrapContext *ctx, SB)
{
	long sigs;
	trap_call_add_dreg(ctx, 0, (((uae_u32) 1) << sb->signal) | sb->eintrsigs);
	if ((sigs = trap_call_lib(ctx, sb->sysbase, -0x13e)) & sb->eintrsigs) { /* Wait */
		sockabort (sb);
		bsdsocklib_seterrno(ctx, sb, 4); /* EINTR */

		// Set signal
		trap_call_add_dreg(ctx, 0, sigs);
		trap_call_add_dreg(ctx, 1, sb->eintrsigs);
		sigs = trap_call_lib(ctx, sb->sysbase, -0x132); /* SetSignal() */

		sb->eintr = 1;
	} else
		sb->eintr = 0;
}

void cancelsig (TrapContext *ctx, SB)
{
	locksigqueue ();
	if (sb->dosignal)
		sb->dosignal = 2;
	unlocksigqueue ();

	trap_call_add_dreg(ctx, 0, 0);
	trap_call_add_dreg(ctx, 1, ((uae_u32) 1) << sb->signal);
	trap_call_lib(ctx, sb->sysbase, -0x132); /* SetSignal() */
}

/* Allocate and initialize per-task state structure */
static struct socketbase *alloc_socketbase (TrapContext *ctx)
{
	SB;
	int i;

	if ((sb = xcalloc (struct socketbase, 1)) != NULL) {
		sb->ownertask = gettask(ctx);
		sb->sysbase = trap_get_long(ctx, 4);

		trap_call_add_dreg(ctx, 0, -1);
		sb->signal = trap_call_lib(ctx, sb->sysbase, -0x14A); /* AllocSignal */

		if (sb->signal == -1) {
			write_log (_T("bsdsocket: ERROR: Couldn't allocate signal for task 0x%08x.\n"), sb->ownertask);
			free (sb);
			return NULL;
		}
//		trap_get_dreg(ctx, 0) = SCRATCHBUFSIZE;
//		trap_get_dreg(ctx, 1) = 0;

		sb->dtablesize = DEFAULT_DTABLE_SIZE;
		/* @@@ check malloc() result */
		sb->dtable = xmalloc(SOCKET, sb->dtablesize);
		sb->ftable = xcalloc(int, sb->dtablesize);

		for (i = sb->dtablesize; i--;)
			sb->dtable[i] = INVALID_SOCKET;

		sb->eintrsigs = 0x1000; /* SIGBREAKF_CTRL_C */

		sb->logfacility = 1 << 3; /* LOG_USER */
		sb->logmask = 0xff;

		if (!host_sbinit(ctx, sb)) {
			/* @@@ free everything   */
		}

		locksigqueue();

		if (socketbases)
			sb->next = socketbases;
		socketbases = sb;

		unlocksigqueue();

		return sb;
	}
	return NULL;
}

STATIC_INLINE struct socketbase *get_socketbase (TrapContext *ctx)
{
	return (struct socketbase*)get_pointer (trap_get_areg(ctx, 6) + offsetof (struct UAEBSDBase, sb));
}

static void free_socketbase (TrapContext *ctx)
{
	struct socketbase *sb, *nsb;

	if ((sb = get_socketbase (ctx)) != NULL) {
		trap_call_add_dreg(ctx, 0, sb->signal);
		trap_call_lib(ctx, sb->sysbase, -0x150); /* FreeSignal */

		if (sb->hostent) {
			trap_call_add_areg(ctx, 1, sb->hostent);
			trap_call_add_dreg(ctx, 0, sb->hostentsize);
			trap_call_lib(ctx, sb->sysbase, -0xD2); /* FreeMem */

		}
		if (sb->protoent) {
			trap_call_add_areg(ctx, 1, sb->protoent);
			trap_call_add_dreg(ctx, 0, sb->protoentsize);
			trap_call_lib(ctx, sb->sysbase, -0xD2); /* FreeMem */

		}
		if (sb->servent) {
			trap_call_add_areg(ctx, 1, sb->servent);
			trap_call_add_dreg(ctx, 0, sb->serventsize);
			trap_call_lib(ctx, sb->sysbase, -0xD2); /* FreeMem */

		}
		host_sbcleanup (sb);

		free (sb->dtable);
		free (sb->ftable);

		locksigqueue ();

		if (sb == socketbases)
			socketbases = sb->next;
		else {
			for (nsb = socketbases; nsb; nsb = nsb->next) {
				if (sb == nsb->next) {
					nsb->next = sb->next;
					break;
				}
			}
		}

#if 1
		if (sb == sbsigqueue)
			sbsigqueue = sb->next;
		else {
			for (nsb = sbsigqueue; nsb; nsb = nsb->next) {
				if (sb == nsb->next) {
					nsb->next = sb->next;
					break;
				}
			}
		}
#endif

		unlocksigqueue ();

		free (sb);
	}
}

static uae_u32 REGPARAM2 bsdsocklib_Expunge (TrapContext *ctx)
{
	BSDTRACE ((_T("Expunge() -> [ignored]\n")));
	return 0;
}

static uae_u32 functable, datatable, inittable;

static uae_u32 REGPARAM2 bsdsocklib_Open (TrapContext *ctx)
{
	uae_u32 result = 0;
	int opencount;
	SB;

	BSDTRACE ((_T("OpenLibrary() -> ")));

	if ((sb = alloc_socketbase(ctx)) != NULL) {
		trap_put_word(ctx, SockLibBase + 32, opencount = trap_get_word(ctx, SockLibBase + 32) + 1);

		
		trap_call_add_areg(ctx, 0, functable);
		trap_call_add_areg(ctx, 1, datatable);
		trap_call_add_areg(ctx, 2, 0);
		trap_call_add_dreg(ctx, 0, sizeof (struct UAEBSDBase));
		trap_call_add_dreg(ctx, 1, 0);
		result = trap_call_lib(ctx, sb->sysbase, -0x54); /* MakeLibrary */

		put_pointer(result + offsetof(struct UAEBSDBase, sb), sb);

		BSDTRACE ((_T("%0x [%d]\n"), result, opencount));
	} else
		BSDTRACE ((_T("failed (out of memory)\n")));

	return result;
}

static uae_u32 REGPARAM2 bsdsocklib_Close (TrapContext *ctx)
{
	int opencount;

	uae_u32 base = trap_get_areg(ctx, 6);
	uae_u32 negsize = get_word (base + 16);

	free_socketbase(ctx);

	trap_put_word(ctx, SockLibBase + 32, opencount = trap_get_word(ctx, SockLibBase + 32) - 1);

	trap_call_add_areg(ctx, 1, base - negsize);
	trap_call_add_dreg(ctx, 0, negsize + trap_get_word(ctx, base + 18));
	trap_call_lib(ctx, trap_get_long(ctx, 4), -0xD2); /* FreeMem */

	BSDTRACE ((_T("CloseLibrary() -> [%d]\n"), opencount));

	return 0;
}

/* socket(domain, type, protocol)(d0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_socket (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_socket(ctx, sb, trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1),
		trap_get_dreg(ctx, 2));
}

/* bind(s, name, namelen)(d0/a0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_bind (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_bind(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0),
		trap_get_dreg(ctx, 1));
}

/* listen(s, backlog)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_listen (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_listen(ctx, sb, trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1));
}

/* accept(s, addr, addrlen)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_accept (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_accept(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), trap_get_areg(ctx, 1));
	return sb->resultval;
}

/* connect(s, name, namelen)(d0/a0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_connect (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_connect(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), trap_get_dreg(ctx, 1));
	return sb->sb_errno ? -1 : 0;
}

/* sendto(s, msg, len, flags, to, tolen)(d0/a0/d1/d2/a1/d3) */
static uae_u32 REGPARAM2 bsdsocklib_sendto (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_sendto(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), NULL, trap_get_dreg(ctx, 1),
		trap_get_dreg(ctx, 2), trap_get_areg(ctx, 1), trap_get_dreg(ctx, 3));
	return sb->resultval;
}

/* send(s, msg, len, flags)(d0/a0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_send (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_sendto(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), NULL, trap_get_dreg(ctx, 1),
		trap_get_dreg(ctx, 2), 0, 0);
	return sb->resultval;
}

/* recvfrom(s, buf, len, flags, from, fromlen)(d0/a0/d1/d2/a1/a2) */
static uae_u32 REGPARAM2 bsdsocklib_recvfrom (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_recvfrom(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), NULL, trap_get_dreg(ctx, 1),
		trap_get_dreg(ctx, 2), trap_get_areg(ctx, 1), trap_get_areg(ctx, 2));
	return sb->resultval;
}

/* recv(s, buf, len, flags)(d0/a0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_recv (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_recvfrom(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), NULL, trap_get_dreg(ctx, 1),
		trap_get_dreg(ctx, 2), 0, 0);
	return sb->resultval;
}

/* shutdown(s, how)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_shutdown (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_shutdown (sb, trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1));
}

/* setsockopt(s, level, optname, optval, optlen)(d0/d1/d2/a0/d3) */
static uae_u32 REGPARAM2 bsdsocklib_setsockopt (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_setsockopt (sb, trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1), trap_get_dreg(ctx, 2),
		trap_get_areg(ctx, 0), trap_get_dreg(ctx, 3));
	return sb->resultval;
}

/* getsockopt(s, level, optname, optval, optlen)(d0/d1/d2/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getsockopt (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_getsockopt (ctx, sb, trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1), trap_get_dreg(ctx, 2),
		trap_get_areg(ctx, 0), trap_get_areg(ctx, 1));
}

/* getsockname(s, hostname, namelen)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getsockname (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_getsockname (ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), trap_get_areg(ctx, 1));
}

/* getpeername(s, hostname, namelen)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getpeername (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_getpeername (ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), trap_get_areg(ctx, 1));
}

/* *------ generic system calls related to sockets */
/* IoctlSocket(d, request, argp)(d0/d1/a0) */
static uae_u32 REGPARAM2 bsdsocklib_IoctlSocket (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_IoctlSocket(ctx, sb, trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1), trap_get_areg(ctx, 0));
}

/* *------ AmiTCP/IP specific stuff */
/* CloseSocket(d)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_CloseSocket (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_CloseSocket(ctx, sb, trap_get_dreg(ctx, 0));
}

/* WaitSelect(nfds, readfds, writefds, execptfds, timeout, maskp)(d0/a0/a1/a2/a3/d1) */
static uae_u32 REGPARAM2 bsdsocklib_WaitSelect (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_WaitSelect(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), trap_get_areg(ctx, 1),
		trap_get_areg(ctx, 2), trap_get_areg(ctx, 3), trap_get_dreg(ctx, 1));
	return sb->resultval;
}

/* SetSocketSignals(SIGINTR, SIGIO, SIGURG)(d0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_SetSocketSignals (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);

	BSDTRACE ((_T("SetSocketSignals(0x%08x,0x%08x,0x%08x) -> "), trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1), trap_get_dreg(ctx, 2)));
	sb->eintrsigs = trap_get_dreg(ctx, 0);
	sb->eventsigs = trap_get_dreg(ctx, 1);

	return 0;
}

/* SetDTableSize(size)(d0) */
static uae_u32 bsdsocklib_SetDTableSize (SB, int newSize)
{
	TrapContext *ctx = NULL;
	int *newdtable;
	int *newftable;
	unsigned int *newmtable;
	int i;

	if (newSize < sb->dtablesize) {
		/* I don't support lowering the size */
		return 0;
	}

	newdtable = xcalloc (int, newSize);
	newftable = xcalloc (int, newSize);
	newmtable = xcalloc (unsigned int, newSize);

	if (newdtable == NULL || newftable == NULL || newmtable == NULL) {
		sb->resultval = -1;
		bsdsocklib_seterrno(ctx, sb, ENOMEM);
		xfree(newdtable);
		xfree(newftable);
		xfree(newmtable);
		return -1;
	}

	memcpy (newdtable, sb->dtable, sb->dtablesize * sizeof(*sb->dtable));
	memcpy (newftable, sb->ftable, sb->dtablesize * sizeof(*sb->ftable));
	memcpy (newmtable, sb->mtable, sb->dtablesize * sizeof(*sb->mtable));
	for (i = sb->dtablesize + 1; i < newSize; i++)
		newdtable[i] = -1;

	sb->dtablesize = newSize;
	xfree(sb->dtable);
	xfree(sb->ftable);
	xfree(sb->mtable);
	sb->dtable = (SOCKET*)newdtable;
	sb->ftable = newftable;
	sb->mtable = newmtable;
	sb->resultval = 0;
	return 0;
}

static int sockpoolindex (long id)
{
	int i;

	for (i = 0; i < SOCKPOOLSIZE; i++)
		if (sockdata->sockpoolids[i] == id)
			return i;

	return -1;
}

/* ObtainSocket(id, domain, type, protocol)(d0/d1/d2/d3) */
static uae_u32 REGPARAM2 bsdsocklib_ObtainSocket (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	int sd;
	int id;
	SOCKET_TYPE s;
	int i;

	id = trap_get_dreg(ctx, 0);

	BSDTRACE ((_T("ObtainSocket(%d,%d,%d,%d) -> "), id, trap_get_dreg(ctx, 1), trap_get_dreg(ctx, 2), trap_get_dreg(ctx, 3)));

	i = sockpoolindex (id);

	if (i == -1) {
		BSDTRACE ((_T("[invalid key]\n")));
		return -1;
	}
	s = sockdata->sockpoolsocks[i];

	sd = getsd(ctx, sb, s);

	BSDTRACE ((_T(" -> Socket=%d\n"), sd));

	if (sd != -1) {
		sb->ftable[sd - 1] = sockdata->sockpoolflags[i];
		callfdcallback(ctx, sb, sd - 1, FDCB_ALLOC);
		sockdata->sockpoolids[i] = UNIQUE_ID;
		return sd - 1;
	}

	return -1;
}

/* ReleaseSocket(fd, id)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_ReleaseSocket (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	int sd;
	int id;
	SOCKET_TYPE s;
	int i;
	uae_u32 flags;

	sd = trap_get_dreg(ctx, 0);
	id = trap_get_dreg(ctx, 1);

	sd++;
	BSDTRACE ((_T("ReleaseSocket(%d,%d) -> "), sd, id));

	s = getsock (ctx, sb, sd);

	if (s != -1) {
		flags = sb->ftable[sd - 1];

		if (flags & REP_ALL) {
			write_log (_T("bsdsocket: ERROR: ReleaseSocket() is not supported for sockets with async event notification enabled!\n"));
			return -1;
		}
		releasesock(ctx, sb, sd);

		if (id == UNIQUE_ID) {
			for (;;) {
				if (sockpoolindex (curruniqid) == -1)
					break;
				curruniqid += 129;
				if ((unsigned long) (curruniqid + 1) < 65536)
					curruniqid += 65537;
			}

			id = curruniqid;
		} else if (id < 0 && id > 65535) {
			if (sockpoolindex (id) != -1) {
				BSDTRACE ((_T("[unique ID already exists]\n")));
				return -1;
			}
		}
		i = sockpoolindex (-1);

		if (i == -1) {
			BSDTRACE ((_T("-1\n")));
			write_log (_T("bsdsocket: ERROR: Global socket pool overflow\n"));
			return -1;
		}
		sockdata->sockpoolids[i] = id;
		sockdata->sockpoolsocks[i] = s;
		sockdata->sockpoolflags[i] = flags;

		BSDTRACE ((_T("id %d s 0x%x\n"), id,s));
	} else {
		BSDTRACE ((_T("[invalid socket descriptor]\n")));
		return -1;
	}

	return id;
}

/* ReleaseCopyOfSocket(fd, id)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_ReleaseCopyOfSocket (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	int sd;
	int id;
	SOCKET_TYPE s;
	int i;
	uae_u32 flags;

	sd = trap_get_dreg(ctx, 0);
	id = trap_get_dreg(ctx, 1);

	sd++;
	BSDTRACE ((_T("ReleaseSocket(%d,%d) -> "), sd, id));

	s = getsock (ctx, sb, sd);

	if (s != -1) {
		flags = sb->ftable[sd - 1];

		if (flags & REP_ALL) {
			write_log (_T("bsdsocket: ERROR: ReleaseCopyOfSocket() is not supported for sockets with async event notification enabled!\n"));
			return -1;
		}
		if (id == UNIQUE_ID) {
			for (;;) {
				if (sockpoolindex (curruniqid) == -1)
					break;
				curruniqid += 129;
				if ((unsigned long) (curruniqid + 1) < 65536)
					curruniqid += 65537;
			}
			id = curruniqid;
		} else if (id < 0 && id > 65535) {
			if (sockpoolindex (id) != -1) {
				BSDTRACE ((_T("[unique ID already exists]\n")));
				return -1;
			}
		}
		i = sockpoolindex (-1);

		if (i == -1) {
			BSDTRACE ((_T("-1\n")));
			write_log (_T("bsdsocket: ERROR: Global socket pool overflow\n"));
			return -1;
		}
		sockdata->sockpoolids[i] = id;
		sockdata->sockpoolsocks[i] = s;
		sockdata->sockpoolflags[i] = flags;

		BSDTRACE ((_T("id %d s 0x%x\n"), id,s));

	} else {

		BSDTRACE ((_T("[invalid socket descriptor]\n")));
		return -1;
	}

	return id;
}

/* Errno()() */
static uae_u32 REGPARAM2 bsdsocklib_Errno (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	BSDTRACE ((_T("Errno() -> %d\n"), sb->sb_errno));
	return sb->sb_errno;
}

/* SetErrnoPtr(errno_p, size)(a0/d0) */
static uae_u32 REGPARAM2 bsdsocklib_SetErrnoPtr (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	uae_u32 errnoptr = trap_get_areg(ctx, 0);
	uae_u32 size = trap_get_dreg(ctx, 0);

	BSDTRACE ((_T("SetErrnoPtr(0x%08x,%d) -> "), errnoptr, size));

	if (size == 1 || size == 2 || size == 4) {
		sb->errnoptr = errnoptr;
		sb->errnosize = size;
		BSDTRACE ((_T("OK\n")));
		return 0;
	}
	bsdsocklib_seterrno(ctx, sb, 22); /* EINVAL */

	return -1;
}

/* *------ inet library calls related to inet address manipulation */
/* Inet_NtoA(in)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_NtoA (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_Inet_NtoA(ctx, sb, trap_get_dreg(ctx, 0));
}

/* inet_addr(cp)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_inet_addr (TrapContext *ctx)
{
	return host_inet_addr(ctx, trap_get_areg(ctx, 0));
}

/* Inet_LnaOf(in)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_LnaOf (TrapContext *ctx)
{
	write_log (_T("bsdsocket: UNSUPPORTED: Inet_LnaOf()\n"));
	return 0;
}

/* Inet_NetOf(in)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_NetOf (TrapContext *ctx)
{
	write_log (_T("bsdsocket: UNSUPPORTED: Inet_NetOf()\n"));
	return 0;
}

/* Inet_MakeAddr(net, host)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_MakeAddr (TrapContext *ctx)
{
	write_log (_T("bsdsocket: UNSUPPORTED: Inet_MakeAddr()\n"));
	return 0;
}

/* inet_network(cp)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_inet_network (TrapContext *ctx)
{
	return host_inet_addr(ctx, trap_get_areg(ctx, 0));
}

/* *------ gethostbyname etc */
/* gethostbyname(name)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_gethostbyname (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_gethostbynameaddr(ctx, sb, trap_get_areg(ctx, 0), 0, -1);
	return sb->sb_herrno ? 0 : sb->hostent;
}

/* gethostbyaddr(addr, len, type)(a0/d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_gethostbyaddr (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_gethostbynameaddr(ctx, sb, trap_get_areg(ctx, 0), trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1));
	return sb->sb_herrno ? 0 : sb->hostent;
}

/* getnetbyname(name)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_getnetbyname (TrapContext *ctx)
{
	write_log (_T("bsdsocket: UNSUPPORTED: getnetbyname()\n"));
	return 0;
}

/* getnetbyaddr(net, type)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_getnetbyaddr (TrapContext *ctx)
{
	write_log (_T("bsdsocket: UNSUPPORTED: getnetbyaddr()\n"));
	return 0;
}

/* getservbyname(name, proto)(a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getservbyname (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_getservbynameport(ctx, sb, trap_get_areg(ctx, 0), trap_get_areg(ctx, 1), 0);
	return sb->sb_errno ? 0 : sb->servent;
}

/* getservbyport(port, proto)(d0/a0) */
static uae_u32 REGPARAM2 bsdsocklib_getservbyport (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_getservbynameport(ctx, sb, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0), 1);
	return sb->sb_errno ? 0 : sb->servent;
}

/* getprotobyname(name)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_getprotobyname (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_getprotobyname(ctx, sb, trap_get_areg(ctx, 0));
	return sb->sb_errno ? 0 : sb->protoent;
}

/* getprotobynumber(proto)(d0)  */
static uae_u32 REGPARAM2 bsdsocklib_getprotobynumber (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	host_getprotobynumber(ctx, sb, trap_get_dreg(ctx, 0));
	return sb->sb_errno ? 0 : sb->protoent;
}

/* *------ AmiTCP/IP 1.1 extensions */
/* Dup2Socket(fd1, fd2)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_Dup2Socket (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	return host_dup2socket(ctx, sb, trap_get_dreg(ctx, 0), trap_get_dreg(ctx, 1));
}

#if 0
struct iovec {
	caddr_t	iov_base;
	int	iov_len;
};

struct msghdr {
	caddr_t	msg_name;		/* optional address */
	u_int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	u_int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_control;		/* ancillary data, see below */
	u_int	msg_controllen;		/* ancillary data buffer len */
	int	msg_flags;		/* flags on received message */
};
#endif

#define MSG_EOR		0x08	/* data completes record */
#define	MSG_TRUNC	0x10	/* data discarded before delivery */

static uae_u32 REGPARAM2 bsdsocklib_sendmsg (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	uaecptr sd = trap_get_dreg(ctx, 0);
	uaecptr msg = trap_get_areg(ctx, 0);
	uae_u32 flags = trap_get_dreg(ctx, 1);

	SOCKET s = getsock (ctx, sb, sd + 1);
	if (s == INVALID_SOCKET)
		return -1;

	int iovlen = trap_get_long(ctx, msg + 12);
	int total = 0;
	uaecptr iovec = trap_get_long(ctx, msg + 8);
	for (int i = 0; i < iovlen; i++) {
		uaecptr iovecp = iovec + i * 8;
		int cnt = trap_get_long(ctx, iovecp + 4);
		if (total + cnt < total)
			return -1;
		total += cnt;
	}
	if (total < 0) {
		bsdsocklib_seterrno(ctx, sb, 22); // EINVAL
		return -1;
	}
	if (trap_get_long(ctx, msg + 16)) { // msg_control
		if (trap_get_long(ctx, msg + 20) < 10) { // msg_controllen
			bsdsocklib_seterrno(ctx, sb, 22); // EINVAL
			return -1;
		}
		// control is not supported
	}
	uae_u8 *data = xmalloc(uae_u8, total);
	if (!data) {
		bsdsocklib_seterrno(ctx, sb, 55); // ENOBUFS
		return -1;
	}
	uae_u8 *p = data;
	for (int i = 0; i < iovlen; i++) {
		uaecptr iovecp = iovec + i * 8;
		int cnt = trap_get_long(ctx, iovecp + 4);
		trap_get_bytes(ctx, p, trap_get_long(ctx, iovecp), cnt);
		p += cnt;
	}
	uaecptr to = trap_get_long(ctx, msg + 0);
	host_sendto(ctx, sb, sd, 0, data, total, flags, to, msg + 4);
	xfree(data);
	return sb->resultval;
}

static uae_u32 REGPARAM2 bsdsocklib_recvmsg (TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase (ctx);
	uaecptr sd = trap_get_dreg(ctx, 0);
	uaecptr msg = trap_get_areg(ctx, 0);
	uae_u32 flags = trap_get_dreg(ctx, 1);

	SOCKET s = getsock (ctx, sb, sd + 1);
	if (s == INVALID_SOCKET)
		return -1;

	uae_u32 msg_flags = trap_get_long(ctx, msg + 24);
	int iovlen = trap_get_long(ctx, msg + 12);
	int total = 0;
	uaecptr iovec = trap_get_long(ctx, msg + 8);
	for (int i = 0; i < iovlen; i++) {
		uaecptr iovecp = iovec + i * 8;
		int cnt = trap_get_long(ctx, iovecp + 4);
		if (total + cnt < total)
			return -1;
		total += cnt;
	}
	if (total < 0) {
		bsdsocklib_seterrno(ctx, sb, 22); // EINVAL
		return -1;
	}
	uae_u8 *data = xmalloc(uae_u8, total);
	if (!data) {
		bsdsocklib_seterrno(ctx, sb, 55); // ENOBUFS
		return -1;
	}
	uaecptr from = trap_get_long(ctx, msg + 0);
	host_recvfrom(ctx, sb, sd, 0, data, total, flags, from, msg + 4);
	if (sb->resultval > 0) {
		uae_u8 *p = data;
		int total2 = 0;
		total = sb->resultval;
		for (int i = 0; i < iovlen && total > 0; i++) {
			uaecptr iovecp = iovec + i * 8;
			int cnt = trap_get_long(ctx, iovecp + 4);
			if (cnt > total)
				cnt = total;
			trap_put_bytes(ctx, p, trap_get_long(ctx, iovecp), cnt);
			p += cnt;
			total -= cnt;
			total2 += cnt;
		}
		if (total2 == sb->resultval)
			msg_flags |= MSG_EOR;
		if (total > 0 && (sb->ftable[sd - 1] & SF_DGRAM))
			msg_flags |= MSG_TRUNC;
		trap_put_long(ctx, msg + 24, msg_flags);
	}
	xfree(data);
	return sb->resultval;
}

static uae_u32 REGPARAM2 bsdsocklib_gethostname (TrapContext *ctx)
{
	return host_gethostname(ctx, trap_get_areg(ctx, 0), trap_get_dreg(ctx, 0));
}

static uae_u32 REGPARAM2 bsdsocklib_gethostid (TrapContext *ctx)
{
	write_log (_T("bsdsocket: WARNING: Process %08x calls deprecated function gethostid() - returning 127.0.0.1\n"),
		trap_get_long(ctx, gettask(ctx) + 10));
	return 0x7f000001;
}

static const TCHAR *errortexts[] =
{_T("No error"), _T("Operation not permitted"), _T("No such file or directory"),
_T("No such process"), _T("Interrupted system call"), _T("Input/output error"), _T("Device not configured"),
_T("Argument list too long"), _T("Exec format error"), _T("Bad file descriptor"), _T("No child processes"),
_T("Resource deadlock avoided"), _T("Cannot allocate memory"), _T("Permission denied"), _T("Bad address"),
_T("Block device required"), _T("Device busy"), _T("Object exists"), _T("Cross-device link"),
_T("Operation not supported by device"), _T("Not a directory"), _T("Is a directory"), _T("Invalid argument"),
_T("Too many open files in system"), _T("Too many open files"), _T("Inappropriate ioctl for device"),
_T("Text file busy"), _T("File too large"), _T("No space left on device"), _T("Illegal seek"),
_T("Read-only file system"), _T("Too many links"), _T("Broken pipe"), _T("Numerical argument out of domain"),
_T("Result too large"), _T("Resource temporarily unavailable"), _T("Operation now in progress"),
_T("Operation already in progress"), _T("Socket operation on non-socket"), _T("Destination address required"),
_T("Message too long"), _T("Protocol wrong type for socket"), _T("Protocol not available"),
_T("Protocol not supported"), _T("Socket type not supported"), _T("Operation not supported"),
_T("Protocol family not supported"), _T("Address family not supported by protocol family"),
_T("Address already in use"), _T("Can't assign requested address"), _T("Network is down"),
_T("Network is unreachable"), _T("Network dropped connection on reset"), _T("Software caused connection abort"),
_T("Connection reset by peer"), _T("No buffer space available"), _T("Socket is already connected"),
_T("Socket is not connected"), _T("Can't send after socket shutdown"), _T("Too many references: can't splice"),
_T("Connection timed out"), _T("Connection refused"), _T("Too many levels of symbolic links"),
_T("File name too long"), _T("Host is down"), _T("No route to host"), _T("Directory not empty"),
_T("Too many processes"), _T("Too many users"), _T("Disc quota exceeded"), _T("Stale NFS file handle"),
_T("Too many levels of remote in path"), _T("RPC struct is bad"), _T("RPC version wrong"),
_T("RPC prog. not avail"), _T("Program version wrong"), _T("Bad procedure for program"), _T("No locks available"),
_T("Function not implemented"), _T("Inappropriate file type or format"), _T("PError 0")};

static uae_u32 errnotextptrs[sizeof (errortexts) / sizeof (*errortexts)];
static const uae_u32 number_sys_error = sizeof (errortexts) / sizeof (*errortexts);


/* *------ syslog functions */
/* Syslog(level, format, ap)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_vsyslog (TrapContext *ctx)
{
#if 0
	struct socketbase *sb = get_socketbase (ctx);
	uae_char format_dst[512];
	char out[256];
	TCHAR *s;
	uae_u8 paramtable[32 * 4];
	int paramcnt, len;
	uae_char *found = NULL;

	uae_u32 level = trap_get_dreg(ctx, 0);
	uaecptr format = trap_get_areg(ctx, 0);
	uaecptr params = trap_get_areg(ctx, 1);

	strcpyah_safe (format_dst, format, sizeof format_dst);

	((uae_u8**)paramtable)[0] = (uae_u8*)format_dst;
	paramcnt = 4;
	for (int i = 0; format_dst[i]; i++) {
		if (format_dst[i] == '%') {
			if (found)
				found = NULL;
			else
				found = &format_dst[i];
			len = 4;
		} else if (found) {
			char c = toupper (format_dst[i]);
			if (c < 'A' || c > 'Z')
				continue;
			if (c == 'H') {
				len = 2;
				continue;
			}
			if (c == 'M') {
				int err = sb->sb_errno;
				if (sb->sb_errno < 0 || sb->sb_errno >= sizeof (errortexts) / sizeof (*errortexts))
					err = sizeof (errortexts) / sizeof (*errortexts) - 1;
				int errlen = _tcslen (errortexts[err]) - (&format_dst[i] - found);
				memmove (&format_dst[i] + errlen, &format_dst[i] + 1, strlen (&format_dst[i] + 1) + 1);
				ua_copy (found, sizeof format_dst, errortexts[err]);
				i += errlen - 1;
				continue;
			}

			if (c == 'P' || c == 'S' || c == 'N') {
				uaecptr pt = get_long (params);
				if (!valid_address (pt, 2))
					goto end;
				((uae_u8**)(paramtable + paramcnt))[0] = get_real_address (pt);
				params += 4;
				paramcnt += sizeof (uae_u8*);
			} else {
				if (len == 2)
					((uae_u16*)(paramtable + paramcnt))[0] = get_word (params);
				else
					((uae_u32*)(paramtable + paramcnt))[0] = get_long (params);
				params += len;
				paramcnt += len;
			}
			found = NULL;
		}
	}

	va_list parms;
	va_start (parms, paramtable);
	_vsnprintf (out, sizeof out, format_dst, parms);
	va_end (parms);

	s = au (out);
	write_log (_T("SYSLOG: %s\n"), s);
	xfree (s);

end:
#endif
	return 0;
}

static const TCHAR *herrortexts[] =
{_T("No error"), _T("Unknown host"), _T("Host name lookup failure"), _T("Unknown server error"),
_T("No address associated with name")};

static uae_u32 herrnotextptrs[sizeof (herrortexts) / sizeof (*herrortexts)];
static const uae_u32 number_host_error = sizeof (herrortexts) / sizeof (*herrortexts);


static const TCHAR *sana2io_errlist[] = 
{
  _T("No error"),				/* S2ERR_NO_ERROR */
  _T("Resource allocation failure"),	/* S2ERR_NO_RESOURCES */
  _T("Unknown error code (2)"), 
  _T("Invalid argument"),			/* S2ERR_BAD_ARGUMENT */
  _T("Inappropriate state"),		/* S2ERR_BAD_STATE */
  _T("Invalid address"),			/* S2ERR_BAD_ADDRESS */
  _T("Requested packet too large"),		/* S2ERR_MTU_EXCEEDED */
  _T("Unknown error (7)"),
  _T("Command is not supporter"),		/* S2ERR_NOT_SUPPORTED */
  _T("Driver software error detected"),	/* S2ERR_SOFTWARE */
  _T("Device driver is offline"),		/* S2ERR_OUTOFSERVICE */
  _T("Transmission attempt failed")		/* S2ERR_TX_FAILURE */
};

static uae_u32 sana2iotextptrs[sizeof (sana2io_errlist) / sizeof (*sana2io_errlist)];
static const uae_u32 number_sana2io_error = sizeof (sana2io_errlist) / sizeof (*sana2io_errlist);


static const TCHAR *sana2wire_errlist[] = 
{
  _T("Generic error: 0"),                   /* S2WERR_GENERIC_ERROR */
  _T("Unit not configured"),		/* S2WERR_NOT_CONFIGURED */
  _T("Unit is currently online"),		/* S2WERR_UNIT_ONLINE */
  _T("Unit is currently offline"),		/* S2WERR_UNIT_OFFLINE */
  _T("Protocol already tracked"),		/* S2WERR_ALREADY_TRACKED */
  _T("Protocol not tracked"),		/* S2WERR_NOT_TRACKED */
  _T("Buff management function error"),	/* S2WERR_BUFF_ERROR */
  _T("Source address error"),		/* S2WERR_SRC_ADDRESS */
  _T("Destination address error"),		/* S2WERR_DST_ADDRESS */
  _T("Broadcast address error"),		/* S2WERR_BAD_BROADCAST */
  _T("Multicast address error"),		/* S2WERR_BAD_MULTICAST */
  _T("Multicast address list full"),	/* S2WERR_MULTICAST_FULL */
  _T("Unsupported event class"),		/* S2WERR_BAD_EVENT */
  _T("Statdata failed sanity check"),	/* S2WERR_BAD_STATDATA */
  _T("Unknown wire error (14)"),
  _T("Attempt to config twice"),		/* S2WERR_IS_CONFIGURED */
  _T("Null pointer detected"),		/* S2WERR_NULL_POINTER */
  _T("tx failed - too many retries"),	/* S2WERR_TOO_MANY_RETIRES */
  _T("Driver fixable HW error")		/* S2WERR_RCVREL_HDW_ERR */
};

static uae_u32 sana2wiretextptrs[sizeof (sana2wire_errlist) / sizeof (*sana2wire_errlist)];
static const uae_u32 number_sana2wire_error = sizeof (sana2wire_errlist) / sizeof (*sana2wire_errlist);


static const TCHAR *io_errlist[] =
{ 
  _T("Unknown error"),			/* 0 */
  _T("Device or unit failed to open"),	/* IOERR_OPENFAIL */
  _T("Request aborted"),			/* IOERR_ABORTED */
  _T("Command not supported by device"),	/* IOERR_NOCMD */
  _T("Invalid length"),			/* IOERR_BADLENGTH */
  _T("Invalid address"),			/* IOERR_BADADDRESS */
  _T("Requested unit is busy"),		/* IOERR_UNITBUSY */
  _T("Hardware self-test failed")		/* IOERR_SELFTEST */
};

static uae_u32 iotextptrs[sizeof (io_errlist) / sizeof (*io_errlist)];
static const uae_u32 number_io_error = sizeof (io_errlist) / sizeof (*io_errlist);


static const TCHAR * const strErr = _T("Errlist lookup error");
static uae_u32 strErrptr, strReleaseVer;


#define TAG_DONE   (0L)		/* terminates array of TagItems. ti_Data unused */
#define TAG_IGNORE (1L)		/* ignore this item, not end of array */
#define TAG_MORE   (2L)		/* ti_Data is pointer to another array of TagItems */
#define TAG_SKIP   (3L)		/* skip this and the next ti_Data items */
#define TAG_USER   ((uae_u32)(1L << 31))

#define SBTF_VAL 0x0000
#define SBTF_REF 0x8000
#define SBTB_CODE 1
#define SBTS_CODE 0x3FFF
#define SBTM_CODE(tag) ((((UWORD)(tag)) >> SBTB_CODE) & SBTS_CODE)
#define SBTF_GET  0x0
#define SBTF_SET  0x1
#define SBTM_GETREF(code) \
	(TAG_USER | SBTF_REF | (((code) & SBTS_CODE) << SBTB_CODE))
#define SBTM_GETVAL(code) (TAG_USER | (((code) & SBTS_CODE) << SBTB_CODE))
#define SBTM_SETREF(code) \
	(TAG_USER | SBTF_REF | (((code) & SBTS_CODE) << SBTB_CODE) | SBTF_SET)
#define SBTM_SETVAL(code) \
	(TAG_USER | (((code) & SBTS_CODE) << SBTB_CODE) | SBTF_SET)
#define SBTC_BREAKMASK	    1
#define SBTC_SIGIOMASK	    2
#define SBTC_SIGURGMASK	    3
#define SBTC_SIGEVENTMASK   4
#define SBTC_ERRNO	    6
#define SBTC_HERRNO	    7
#define SBTC_DTABLESIZE	    8
#define SBTC_FDCALLBACK	    9
#define SBTC_LOGSTAT	    10
#define SBTC_LOGTAGPTR	    11
#define SBTC_LOGFACILITY    12
#define SBTC_LOGMASK	    13
#define SBTC_ERRNOSTRPTR    14 /* <sys/errno.h> */
#define SBTC_HERRNOSTRPTR   15 /* <netdb.h> */
#define SBTC_IOERRNOSTRPTR  16 /* <exec/errors.h> */
#define SBTC_S2ERRNOSTRPTR  17 /* <devices/sana2.h> */
#define SBTC_S2WERRNOSTRPTR 18 /* <devices/sana2.h> */
#define SBTC_ERRNOBYTEPTR   21
#define SBTC_ERRNOWORDPTR   22
#define SBTC_ERRNOLONGPTR   24
#define SBTC_HERRNOLONGPTR  25
#define SBTC_RELEASESTRPTR  29
#define SBTC_GET_BYTES_RECEIVED 64
#define SBTC_GET_BYTES_SENT 65

#define LOG_FACMASK     0x03f8

static void tagcopy(TrapContext *ctx, uae_u32 currtag, uae_u32 currval, uae_u32 tagptr, uae_u32 * ptr)
{
	switch (currtag & 0x8001)
	{
		case 0x0000:	/* SBTM_GETVAL */
		trap_put_long(ctx, tagptr + 4, ptr ? *ptr : 0);
		break;
		case 0x8000:	/* SBTM_GETREF */
		trap_put_long(ctx, currval, ptr ? *ptr : 0);
		break;
		case 0x0001:	/* SBTM_SETVAL */
		if (ptr)
			*ptr = currval;
		break;
		default:		/* SBTM_SETREF */
		if (ptr)
			*ptr = trap_get_long(ctx, currval);
		break;
	}
}

static uae_u32 REGPARAM2 bsdsocklib_SocketBaseTagList(TrapContext *ctx)
{
	struct socketbase *sb = get_socketbase(ctx);
	uae_u32 tagptr = trap_get_areg(ctx, 0);
	uae_u32 tagsprocessed = 0;
	uae_u32 currtag;
	uae_u32 currval;

	BSDTRACE ((_T("SocketBaseTagList(")));

	for (;;) {
		currtag = trap_get_long(ctx, tagptr);
		currval = trap_get_long(ctx, tagptr + 4);
		tagsprocessed++;

		if (!(currtag & TAG_USER)) {

			switch (currtag)
			{
				case TAG_DONE:
					BSDTRACE ((_T("TAG_DONE")));
					tagsprocessed = 0;
					goto done;
				case TAG_IGNORE:
					BSDTRACE ((_T("TAG_IGNORE")));
					break;
				case TAG_MORE:
					BSDTRACE ((_T("TAG_MORE(0x%x)"), currval));
					tagptr = currval;
					break;
				case TAG_SKIP:
					BSDTRACE ((_T("TAG_SKIP(%d)"), currval));
					tagptr += currval * 8;
					break;
				default:
					write_log (_T("bsdsocket: WARNING: Unsupported tag type (%08x) in SocketBaseTagList(%x)\n"),
						currtag, trap_get_areg(ctx, 0));
					goto done;
			}

		} else {

			BSDTRACE ((_T("SBTM_")));
			BSDTRACE ((currtag & 0x0001 ? _T("SET") : _T("GET")));
			BSDTRACE ((currtag & 0x8000 ? _T("REF(") : _T("VAL(")));

			switch ((currtag >> 1) & SBTS_CODE)
			{
				case SBTC_BREAKMASK:
					BSDTRACE ((_T("SBTC_BREAKMASK),0x%x,0x%x"), currval, sb->eintrsigs));
					tagcopy(ctx, currtag, currval, tagptr, &sb->eintrsigs);
					break;
				case SBTC_SIGIOMASK:
					BSDTRACE ((_T("SBTC_SIGIOMASK),0x%x,0x%x"), currval, sb->eventsigs));
					tagcopy(ctx, currtag, currval, tagptr, &sb->eventsigs);
					break;
				case SBTC_SIGURGMASK:
					BSDTRACE ((_T("SBTC_SIGURGMASK),0x%x"), currval));
					break;
				case SBTC_SIGEVENTMASK:
					BSDTRACE ((_T("SBTC_SIGEVENTMASK),0x%x,0x%x"), currval, sb->eventsigs));
					tagcopy(ctx, currtag, currval, tagptr, &sb->eventsigs);
					break;
				case SBTC_ERRNO:
					BSDTRACE ((_T("SBTC_ERRNO),%x,%d"), currval, sb->sb_errno));
					tagcopy(ctx, currtag, currval, tagptr, (uae_u32*)&sb->sb_errno);
					break;
				case SBTC_HERRNO:
					BSDTRACE ((_T("SBTC_HERRNO),%x,%d"), currval, sb->sb_herrno));
					tagcopy(ctx, currtag, currval, tagptr, (uae_u32*)&sb->sb_herrno);
					break;
				case SBTC_DTABLESIZE:
					BSDTRACE ((_T("SBTC_DTABLESIZE),0x%x"), currval));
					if (currtag & 1) {
						bsdsocklib_SetDTableSize(sb, currval);
					} else {
						put_long (tagptr + 4, sb->dtablesize);
					}
					break;

				case SBTC_FDCALLBACK:
					BSDTRACE ((_T("SBTC_FDCALLBACK),%08x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->fdcallback);
					break;

				case SBTC_LOGSTAT:
					BSDTRACE ((_T("SBTC_LOGSTAT),%08x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->logstat);
					sb->logstat &= 0xff;
					break;
				case SBTC_LOGTAGPTR:
					BSDTRACE ((_T("SBTC_LOGTAGPTR),%08x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->logptr);
					break;
				case SBTC_LOGFACILITY:
					BSDTRACE ((_T("SBTC_LOGFACILITY),%08x"), currval));
					if (((currtag & 1) && currval != 0 && (currval & ~LOG_FACMASK)) || !(currtag & 1))
						tagcopy(ctx, currtag, currval, tagptr, &sb->logfacility);
					break;
				case SBTC_LOGMASK:
					BSDTRACE ((_T("SBTC_LOGMASK),%08x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->logmask);
					sb->logmask &= 0xff;
					break;

				case SBTC_IOERRNOSTRPTR:
					if (currtag & 1) {
						BSDTRACE ((_T("IOERRNOSTRPTR),invalid")));
						goto done;
					} else {
						unsigned long ulTmp;
						if (currtag & 0x8000) { /* SBTM_GETREF */
							ulTmp = trap_get_long(ctx, currval);
						} else { /* SBTM_GETVAL */
							ulTmp = currval;
						}
						BSDTRACE ((_T("IOERRNOSTRPTR),%lu"), ulTmp));
						if (ulTmp < number_sys_error) {
							tagcopy(ctx, currtag, currval, tagptr, &iotextptrs[ulTmp]);
						} else {
							tagcopy(ctx, currtag, currval, tagptr, &strErrptr);
						}
					}
					break;
				case SBTC_S2ERRNOSTRPTR:
					if (currtag & 1) {
						BSDTRACE ((_T("S2ERRNOSTRPTR),invalid")));
						goto done;
					} else {
						unsigned long ulTmp;
						if (currtag & 0x8000) { /* SBTM_GETREF */
							ulTmp = trap_get_long(ctx, currval);
						} else { /* SBTM_GETVAL */
							ulTmp = currval;
						}
						BSDTRACE ((_T("S2ERRNOSTRPTR),%lu"), ulTmp));
						if (ulTmp < number_sys_error) {
							tagcopy(ctx, currtag, currval, tagptr, &sana2iotextptrs[ulTmp]);
						} else {
							tagcopy(ctx, currtag, currval, tagptr, &strErrptr);
						}
					}
					break;
				case SBTC_S2WERRNOSTRPTR:
					if (currtag & 1) {
						BSDTRACE ((_T("S2WERRNOSTRPTR),invalid")));
						goto done;
					} else {
						unsigned long ulTmp;
						if (currtag & 0x8000) { /* SBTM_GETREF */
							ulTmp = trap_get_long(ctx, currval);
						} else { /* SBTM_GETVAL */
							ulTmp = currval;
						}
						BSDTRACE ((_T("S2WERRNOSTRPTR),%lu"), ulTmp));
						if (ulTmp < number_sys_error) {
							tagcopy(ctx, currtag, currval, tagptr, &sana2wiretextptrs[ulTmp]);
						} else {
							tagcopy(ctx, currtag, currval, tagptr, &strErrptr);
						}
					}
					break;
				case SBTC_ERRNOSTRPTR:
					if (currtag & 1) {
						BSDTRACE ((_T("ERRNOSTRPTR),invalid")));
						goto done;
					} else {
						unsigned long ulTmp;
						if (currtag & 0x8000) { /* SBTM_GETREF */
							ulTmp = trap_get_long(ctx, currval);
						} else { /* SBTM_GETVAL */
							ulTmp = currval;
						}
						BSDTRACE ((_T("ERRNOSTRPTR),%lu"), ulTmp));
						if (ulTmp < number_sys_error) {
							tagcopy(ctx, currtag, currval, tagptr, &errnotextptrs[ulTmp]);
						} else {
							tagcopy(ctx, currtag, currval, tagptr, &strErrptr);
						}
					}
					break;
				case SBTC_HERRNOSTRPTR:
					if (currtag & 1) {
						BSDTRACE ((_T("HERRNOSTRPTR),invalid")));
						goto done;
					} else {
						unsigned long ulTmp;
						if (currtag & 0x8000) { /* SBTM_GETREF */
							ulTmp = trap_get_long(ctx, currval);
						} else { /* SBTM_GETVAL */
							ulTmp = currval;
						}
						BSDTRACE ((_T("HERRNOSTRPTR),%lu"), ulTmp));
						if (ulTmp < number_host_error) {
							tagcopy(ctx, currtag, currval, tagptr, &herrnotextptrs[ulTmp]);
						} else {
							tagcopy(ctx, currtag, currval, tagptr, &strErrptr);
						}
					}
					break;

				case SBTC_ERRNOBYTEPTR:
					BSDTRACE ((_T("SBTC_ERRNOBYTEPTR),0x%x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->errnoptr);
					sb->errnosize = 1;
					break;
				case SBTC_ERRNOWORDPTR:
					BSDTRACE ((_T("SBTC_ERRNOWORDPTR),0x%x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->errnoptr);
					sb->errnosize = 2;
					break;
				case SBTC_ERRNOLONGPTR:
					BSDTRACE ((_T("SBTC_ERRNOLONGPTR),0x%x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->errnoptr);
					sb->errnosize = 4;
					break;
				case SBTC_HERRNOLONGPTR:
					BSDTRACE ((_T("SBTC_HERRNOLONGPTR),0x%x"), currval));
					tagcopy(ctx, currtag, currval, tagptr, &sb->herrnoptr);
					sb->herrnosize = 4;
					break;
				case SBTC_RELEASESTRPTR:
					BSDTRACE((_T("SBTC_RELEASESTRPTR),0x%x"), currval));
					if (!(currtag & 1)) {
						tagcopy(ctx, currtag, currval, tagptr, &strReleaseVer);
					}
					break;
				case SBTC_GET_BYTES_RECEIVED:
					BSDTRACE ((_T("SBTC_GET_BYTES_RECEIVED),%08x"), currval));
					if ((currtag & 0x8001) == 0x8000) { /* SBTM_GETREF */
						trap_put_long(ctx, currval + 0, sb->bytesreceived >> 32);
						trap_put_long(ctx, currval + 4, sb->bytesreceived >> 0);
					}
					break;
				case SBTC_GET_BYTES_SENT:
					BSDTRACE ((_T("SBTC_GET_BYTES_SENT),%08x"), currval));
					if ((currtag & 0x8001) == 0x8000) { /* SBTM_GETREF */
						trap_put_long(ctx, currval + 0, sb->bytestransmitted >> 32);
						trap_put_long(ctx, currval + 4, sb->bytestransmitted >> 0);
					}
					break;

				default:
					write_log (_T("bsdsocket: WARNING: Unsupported tag type (%08x=%d) in SocketBaseTagList(%x)\n"),
						currtag, (currtag / 2) & SBTS_CODE, trap_get_areg(ctx, 0));
					goto done;
			}
		}

		BSDTRACE ((_T(",")));
		tagptr += 8;
	}

done:
	BSDTRACE ((_T(") -> %d\n"), tagsprocessed));

	return tagsprocessed;
}

static uae_u32 REGPARAM2 bsdsocklib_GetSocketEvents(TrapContext *ctx)
{
#ifdef _WIN32
	struct socketbase *sb = get_socketbase(ctx);
	int i;
	int flags;
	uae_u32 ptr = trap_get_areg(ctx, 0);

	BSDTRACE ((_T("GetSocketEvents(0x%x) -> "), ptr));

	for (i = sb->dtablesize; i--; sb->eventindex++) {
		if (sb->eventindex >= sb->dtablesize)
			sb->eventindex = 0;

		if (sb->mtable[sb->eventindex]) {
			flags = sb->ftable[sb->eventindex] & SET_ALL;
			if (flags) {
				sb->ftable[sb->eventindex] &= ~SET_ALL;
				trap_put_long(ctx, trap_get_areg(ctx, 0), flags >> 8);
				BSDTRACE ((_T("%d (0x%x)\n"), sb->eventindex + 1, flags >> 8));
				return sb->eventindex; // xxx
			}
		}
	}
#endif
	BSDTRACE ((_T("-1\n")));
	return -1;
}

static uae_u32 REGPARAM2 bsdsocklib_getdtablesize(TrapContext *ctx)
{
	return get_socketbase(ctx)->dtablesize;
}

static uae_u32 REGPARAM2 bsdsocklib_null(TrapContext *ctx)
{
	return 0;
}

static uae_u32 REGPARAM2 bsdsocklib_init(TrapContext *ctx)
{
	TCHAR verStr[32];
	uae_u32 tmp1;
	int i;

	write_log (_T("Creating UAE bsdsocket.library 4.1\n"));
	if (SockLibBase)
		bsdlib_reset ();

#if NEWTRAP
	trap_call_add_areg(ctx, 0, functable);
	trap_call_add_areg(ctx, 1, datatable);
	trap_call_add_areg(ctx, 2, 0);
	trap_call_add_dreg(ctx, 0, LIBRARY_SIZEOF);
	trap_call_add_dreg(ctx, 1, 0);
	tmp1 = trap_call_lib(ctx, trap_get_areg(ctx, 6), -0x54); /* MakeLibrary */
#else
	trap_get_areg(ctx, 0) = functable;
	trap_get_areg(ctx, 1) = datatable;
	trap_get_areg(ctx, 2) = 0;
	trap_get_dreg(ctx, 0) = LIBRARY_SIZEOF;
	trap_get_dreg(ctx, 1) = 0;
	tmp1 = CallLib(ctx, trap_get_areg(ctx, 6), -0x54); /* MakeLibrary */
#endif

	if (!tmp1) {
		write_log (_T("bsdoscket: FATAL: Cannot create bsdsocket.library!\n"));
		return 0;
	}
	
#if NEWTRAP
	trap_call_add_areg(ctx, 1, tmp1);
	trap_call_lib(ctx, trap_get_areg(ctx, 6), -0x18c); /* AddLibrary */
#else
	trap_get_areg(ctx, 1) = tmp1;
	CallLib (ctx, trap_get_areg(ctx, 6), -0x18c); /* AddLibrary */
#endif

	SockLibBase = tmp1;

	/* Install error strings in Amiga memory */
	_stprintf(verStr, _T("UAE %d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
	tmp1 = 0;
	for (i = number_sys_error; i--;)
		tmp1 += uaetcslen (errortexts[i]) + 1;
	for (i = number_host_error; i--;)
		tmp1 += uaetcslen (herrortexts[i]) + 1;
	for (i = number_sana2io_error; i--;)
		tmp1 += uaetcslen (sana2io_errlist[i]) + 1;
	for (i = number_sana2wire_error; i--;)
		tmp1 += uaetcslen (sana2wire_errlist[i]) + 1;
	tmp1 += uaetcslen(strErr) + 1;
	tmp1 += uaetcslen(verStr) + 1;

#if NEWTRAP
	trap_call_add_dreg(ctx, 0, tmp1);
	trap_call_add_dreg(ctx, 1, 0);
	tmp1 = trap_call_lib(ctx, trap_get_areg(ctx, 6), -0xC6); /* AllocMem */
#else		
	trap_get_dreg(ctx, 0) = tmp1;
	trap_get_dreg(ctx, 1) = 0;
	tmp1 = CallLib (ctx, trap_get_areg(ctx, 6), -0xC6); /* AllocMem */
#endif

	if (!tmp1) {
		write_log (_T("bsdsocket: FATAL: Ran out of memory while creating bsdsocket.library!\n"));
		return 0;
	}

	for (i = 0; i < (int) (number_sys_error); i++)
		errnotextptrs[i] = addstr(ctx, &tmp1, errortexts[i]);
	for (i = 0; i < (int) (number_host_error); i++)
		herrnotextptrs[i] = addstr(ctx, &tmp1, herrortexts[i]);
	for (i = 0; i < (int) (number_sana2io_error); i++)
		sana2iotextptrs[i] = addstr(ctx, &tmp1, sana2io_errlist[i]);
	for (i = 0; i < (int) (number_sana2wire_error); i++)
		sana2wiretextptrs[i] = addstr(ctx, &tmp1, sana2wire_errlist[i]);
	strErrptr = addstr(ctx, &tmp1, strErr);
	strReleaseVer = addstr(ctx, &tmp1, verStr);

#if 0
	/* @@@ someone please implement a proper interrupt handler setup here :) */
	tmp1 = here ();
	calltrap (deftrap2 (bsdsock_int_handler, TRAPFLAG_EXTRA_STACK | TRAPFLAG_NO_RETVAL, "bsdsock_int_handler"));
	dw (0x4ef9);
	dl (get_long (context->regs.vbr + 0x78));
	put_long (context->regs.vbr + 0x78, tmp1);
#endif

	trap_set_dreg(ctx, 0, 1);
	return 0;
}

void bsdlib_reset (void)
{
	SB, *nsb;
	int i;

	if (!SockLibBase)
		return;

	SockLibBase = 0;

	write_log (_T("BSDSOCK: cleanup start..\n"));
	host_sbcleanup (NULL);
	for (sb = socketbases; sb; sb = nsb) {
		nsb = sb->next;

		write_log (_T("BSDSOCK: cleanup start socket %p\n"), sb);
		host_sbcleanup (sb);

		free (sb->dtable);
		free (sb->ftable);

		free (sb);
	}
	write_log (_T("BSDSOCK: cleanup end\n"));

	socketbases = NULL;
#if 1
	sbsigqueue = NULL;
#endif

	for (i = 0; i < SOCKPOOLSIZE; i++) {
		if (sockdata->sockpoolids[i] != UNIQUE_ID) {
			sockdata->sockpoolids[i] = UNIQUE_ID;
			host_closesocketquick (sockdata->sockpoolsocks[i]);
		}
	}

	host_sbreset ();
	write_log (_T("BSDSOCK: cleanup finished\n"));
}

static const TrapHandler sockfuncs[] = {
	bsdsocklib_init, bsdsocklib_Open, bsdsocklib_Close, bsdsocklib_Expunge,
	bsdsocklib_socket, bsdsocklib_bind, bsdsocklib_listen, bsdsocklib_accept,
	bsdsocklib_connect, bsdsocklib_sendto, bsdsocklib_send, bsdsocklib_recvfrom, bsdsocklib_recv,
	bsdsocklib_shutdown, bsdsocklib_setsockopt, bsdsocklib_getsockopt, bsdsocklib_getsockname,
	bsdsocklib_getpeername, bsdsocklib_IoctlSocket, bsdsocklib_CloseSocket, bsdsocklib_WaitSelect,
	bsdsocklib_SetSocketSignals, bsdsocklib_getdtablesize, bsdsocklib_ObtainSocket, bsdsocklib_ReleaseSocket,
	bsdsocklib_ReleaseCopyOfSocket, bsdsocklib_Errno, bsdsocklib_SetErrnoPtr, bsdsocklib_Inet_NtoA,
	bsdsocklib_inet_addr, bsdsocklib_Inet_LnaOf, bsdsocklib_Inet_NetOf, bsdsocklib_Inet_MakeAddr,
	bsdsocklib_inet_network, bsdsocklib_gethostbyname, bsdsocklib_gethostbyaddr, bsdsocklib_getnetbyname,
	bsdsocklib_getnetbyaddr, bsdsocklib_getservbyname, bsdsocklib_getservbyport, bsdsocklib_getprotobyname,
	bsdsocklib_getprotobynumber, bsdsocklib_vsyslog, bsdsocklib_Dup2Socket, bsdsocklib_sendmsg,
	bsdsocklib_recvmsg, bsdsocklib_gethostname, bsdsocklib_gethostid, bsdsocklib_SocketBaseTagList,
	bsdsocklib_GetSocketEvents
};

static const TCHAR * const funcnames[] = {
	_T("bsdsocklib_init"), _T("bsdsocklib_Open"), _T("bsdsocklib_Close"), _T("bsdsocklib_Expunge"),
	_T("bsdsocklib_socket"), _T("bsdsocklib_bind"), _T("bsdsocklib_listen"), _T("bsdsocklib_accept"),
	_T("bsdsocklib_connect"), _T("bsdsocklib_sendto"), _T("bsdsocklib_send"), _T("bsdsocklib_recvfrom"), _T("bsdsocklib_recv"),
	_T("bsdsocklib_shutdown"), _T("bsdsocklib_setsockopt"), _T("bsdsocklib_getsockopt"), _T("bsdsocklib_getsockname"),
	_T("bsdsocklib_getpeername"), _T("bsdsocklib_IoctlSocket"), _T("bsdsocklib_CloseSocket"), _T("bsdsocklib_WaitSelect"),
	_T("bsdsocklib_SetSocketSignals"), _T("bsdsocklib_getdtablesize"), _T("bsdsocklib_ObtainSocket"), _T("bsdsocklib_ReleaseSocket"),
	_T("bsdsocklib_ReleaseCopyOfSocket"), _T("bsdsocklib_Errno"), _T("bsdsocklib_SetErrnoPtr"), _T("bsdsocklib_Inet_NtoA"),
	_T("bsdsocklib_inet_addr"), _T("bsdsocklib_Inet_LnaOf"), _T("bsdsocklib_Inet_NetOf"), _T("bsdsocklib_Inet_MakeAddr"),
	_T("bsdsocklib_inet_network"), _T("bsdsocklib_gethostbyname"), _T("bsdsocklib_gethostbyaddr"), _T("bsdsocklib_getnetbyname"),
	_T("bsdsocklib_getnetbyaddr"), _T("bsdsocklib_getservbyname"), _T("bsdsocklib_getservbyport"), _T("bsdsocklib_getprotobyname"),
	_T("bsdsocklib_getprotobynumber"), _T("bsdsocklib_vsyslog"), _T("bsdsocklib_Dup2Socket"), _T("bsdsocklib_sendmsg"),
	_T("bsdsocklib_recvmsg"), _T("bsdsocklib_gethostname"), _T("bsdsocklib_gethostid"), _T("bsdsocklib_SocketBaseTagList"),
	_T("bsdsocklib_GetSocketEvents")
};

static uae_u32 sockfuncvecs[sizeof (sockfuncs) / sizeof (*sockfuncs)];

static uae_u32 res_name, res_id, res_init;

uaecptr bsdlib_startup (TrapContext *ctx, uaecptr resaddr)
{
	if (res_name == 0 || !currprefs.socket_emu)
		return resaddr;
	trap_put_word(ctx, resaddr + 0x0, 0x4AFC);
	trap_put_long(ctx, resaddr + 0x2, resaddr);
	trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
	if (kickstart_version >= 37) {
		trap_put_long(ctx, resaddr + 0xA, 0x84040900 | AFTERDOS_PRI); /* RTF_AUTOINIT, RT_VERSION NT_LIBRARY, RT_PRI */
	} else {
		trap_put_long(ctx, resaddr + 0xA, 0x80040905); /* RTF_AUTOINIT, RT_VERSION NT_LIBRARY, RT_PRI */
	}
	trap_put_long(ctx, resaddr + 0xE, res_name);
	trap_put_long(ctx, resaddr + 0x12, res_id);
	trap_put_long(ctx, resaddr + 0x16, res_init);
	resaddr += 0x1A;
	return resaddr;
}

void bsdlib_install (void)
{
	int i;

	if (!sockdata) {
		sockdata = xcalloc (struct sockd, 1);
		for (i = 0; i < SOCKPOOLSIZE; i++)
			sockdata->sockpoolids[i] = UNIQUE_ID;
	}

	if (!init_socket_layer ())
		return;

	res_name = ds (_T("bsdsocket.library"));
	res_id = ds (_T("UAE bsdsocket.library 4.1"));

	for (i = 0; i < (int) (sizeof (sockfuncs) / sizeof (sockfuncs[0])); i++) {
		sockfuncvecs[i] = here ();
		calltrap (deftrap2 (sockfuncs[i], TRAPFLAG_EXTRA_STACK, funcnames[i]));
		dw (RTS);
	}

	/* FuncTable */
	functable = here ();
	for (i = 1; i < 4; i++)
		dl (sockfuncvecs[i]);	/* Open / Close / Expunge */
	dl (EXPANSION_nullfunc);	/* Null */
	for (i = 4; i < (int) (sizeof (sockfuncs) / sizeof (sockfuncs[0])); i++)
		dl (sockfuncvecs[i]);
	dl (0xFFFFFFFF);		/* end of table */

	/* DataTable */
	datatable = here ();
	dw (0xE000);		/* INITBYTE */
	dw (0x0008);		/* LN_TYPE */
	dw (0x0900);		/* NT_LIBRARY */
	dw (0xE000);		/* INITBYTE */
	dw (0x0009);		/* LN_PRI */
	dw (0xCE00);		/* -50 */
	dw (0xC000);		/* INITLONG */
	dw (0x000A);		/* LN_NAME */
	dl (res_name);
	dw (0xE000);		/* INITBYTE */
	dw (0x000E);		/* LIB_FLAGS */
	dw (0x0600);		/* LIBF_SUMUSED | LIBF_CHANGED */
	dw (0xD000);		/* INITWORD */
	dw (0x0014);		/* LIB_VERSION */
	dw (0x0004);
	dw (0xD000);
	dw (0x0016);		/* LIB_REVISION */
	dw (0x0001);
	dw (0xC000);
	dw (0x0018);		/* LIB_IDSTRING */
	dl (res_id);
	dl (0x00000000);		/* end of table */

	res_init = here ();
	dl (512);
	dl (functable);
	dl (datatable);
	dl (*sockfuncvecs);

	write_log (_T("bsdsocket.library installed\n"));
}

#endif /* ! BSDSOCKET */
