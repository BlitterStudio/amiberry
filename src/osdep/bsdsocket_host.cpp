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

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <iphlpapi.h>
#endif
#include <atomic>
#include <cstdarg>

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "threaddep/thread.h"
#include "native2amiga.h"
#include "bsdsocket.h"

#ifdef _WIN32
/*
 * WinUAE-derived native Windows bsdsocket implementation
 *
 * Uses WSAAsyncSelect + hidden HWND for async socket event notification,
 * worker thread pools for WaitSelect and DNS lookups,
 * CRITICAL_SECTION for synchronization.
 *
 * Based on WinUAE od-win32/bsdsock.cpp
 * Copyright 1997,98 Mathias Ortmann
 * Copyright 1999,2000 Brian King
 */

int rawsockets = 0;
static int hWndSelector = 0;
static HWND hAmigaSockWnd;

struct threadargs {
	struct socketbase *sb;
	uae_u32 args1;
	uae_u32 args2;
	uae_u32 args3;
	int args3i;
	long args4;
	uae_char buf[MAXGETHOSTSTRUCT];
	int wscnt;
};

struct threadargsw {
	struct socketbase *sb;
	uae_u32 nfds;
	uae_u32 readfds;
	uae_u32 writefds;
	uae_u32 exceptfds;
	uae_u32 timeout;
	int wscnt;
};

#define MAX_SELECT_THREADS 64
#define MAX_GET_THREADS 64

struct bsdsockdata {
	HWND hSockWnd;
	HANDLE hSockThread;
	HANDLE hSockReq;
	HANDLE hSockReqHandled;
	CRITICAL_SECTION csSigQueueLock;
	CRITICAL_SECTION SockThreadCS;
	unsigned int threadid;
	WSADATA wsbData;

	volatile HANDLE hGetThreads[MAX_GET_THREADS];
	volatile struct threadargs *threadGetargs[MAX_GET_THREADS];
	volatile int threadGetargs_inuse[MAX_GET_THREADS];
	volatile HANDLE hGetEvents[MAX_GET_THREADS];
	volatile HANDLE hGetEvents2[MAX_GET_THREADS];

	volatile HANDLE hThreads[MAX_SELECT_THREADS];
	volatile struct threadargsw *threadargsw[MAX_SELECT_THREADS];
	volatile HANDLE hEvents[MAX_SELECT_THREADS];

	struct socketbase *asyncsb[MAXPENDINGASYNC];
	SOCKET asyncsock[MAXPENDINGASYNC];
	uae_u32 asyncsd[MAXPENDINGASYNC];
	int asyncindex;
};

static struct bsdsockdata *bsd;
static int threadindextable[MAX_GET_THREADS];

static unsigned int __stdcall sock_thread(void *);

#define THREAD(func,arg) (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, &bsd->threadid)
#define THREADEND(result) _endthreadex(result)

#define SETERRNO bsdsocklib_seterrno(ctx, sb, WSAGetLastError() - WSABASEERR)
#define SETHERRNO bsdsocklib_setherrno(ctx, sb, WSAGetLastError() - WSABASEERR)
#define WAITSIGNAL waitsig(ctx, sb)

#define SETSIGNAL addtosigqueue(sb,0)
#define CANCELSIGNAL cancelsig(ctx, sb)

#define FIOSETOWN _IOW('f', 124, long)
#define FIOGETOWN _IOR('f', 123, long)

#define _IOWR_BSD(x,y,t) (IOC_OUT|IOC_IN|(((long)(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define SIOCGIFADDR     _IOWR_BSD('i', 33, 16)
#define SIOCGIFFLAGS    _IOWR_BSD('i', 17, 32)
#define SIOCGIFBRDADDR  _IOWR_BSD('i', 35, 16)
#define SIOCGIFCONF     _IOWR_BSD('i', 36, 8)
#define SIOCGIFNETMASK  _IOWR_BSD('i', 37, 16)
#define SIOCGIFMETRIC   _IOWR_BSD('i', 23, 32)
#define SIOCGIFMTU      _IOWR_BSD('i', 51, 32)
#define SIOCGIFPHYS     _IOWR_BSD('i', 53, 32)

#define BEGINBLOCKING if (sb->ftable[sd - 1] & SF_BLOCKING) sb->ftable[sd - 1] |= SF_BLOCKINGINPROGRESS
#define ENDBLOCKING sb->ftable[sd - 1] &= ~SF_BLOCKINGINPROGRESS

static LRESULT CALLBACK SocketWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#define PREPARE_THREAD EnterCriticalSection(&bsd->SockThreadCS)
#define TRIGGER_THREAD { SetEvent(bsd->hSockReq); WaitForSingleObject(bsd->hSockReqHandled, INFINITE); LeaveCriticalSection(&bsd->SockThreadCS); }

#define SOCKVER_MAJOR 2
#define SOCKVER_MINOR 2

#define SF_RAW_RAW      0x10000000
#define SF_RAW_UDP      0x08000000
#define SF_RAW_RUDP     0x04000000
#define SF_RAW_RICMP    0x02000000
#define SF_RAW_HDR      0x01000000

static void bsdsetpriority(HANDLE thread)
{
	SetThreadPriority(thread, THREAD_PRIORITY_NORMAL);
}

static int mySockStartup(void)
{
	int result = 0;
	SOCKET dummy;

	if (!bsd) {
		bsd = xcalloc(struct bsdsockdata, 1);
		if (!bsd)
			return 0;
		for (int i = 0; i < MAX_GET_THREADS; i++)
			threadindextable[i] = i;
	}
	if (WSAStartup(MAKEWORD(SOCKVER_MAJOR, SOCKVER_MINOR), &bsd->wsbData)) {
		DWORD lasterror = WSAGetLastError();
		if (lasterror == WSAVERNOTSUPPORTED)
			write_log(_T("BSDSOCK: ERROR - Winsock 2.2 or higher required\n"));
		else
			write_log(_T("BSDSOCK: ERROR - Unable to initialize Windows socket layer! Error code: %d\n"), lasterror);
		return 0;
	}

	write_log(_T("BSDSOCK: using %s\n"), bsd->wsbData.szDescription);
	if ((dummy = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET) {
		closesocket(dummy);
		result = 1;
	} else {
		write_log(_T("BSDSOCK: ERROR - WSPStartup/NSPStartup failed! Error code: %d\n"),
			WSAGetLastError());
		result = 0;
	}

	return result;
}

int init_socket_layer(void)
{
	int result = 0;

	if (bsd)
		return -1;
	deinit_socket_layer();
	if (currprefs.socket_emu) {
		if ((result = mySockStartup())) {
			if (!bsd)
				return 0;
			if (bsd->hSockThread == NULL) {
				WNDCLASS wc;

				InitializeCriticalSection(&bsd->csSigQueueLock);
				InitializeCriticalSection(&bsd->SockThreadCS);
				bsd->hSockReq = CreateEvent(NULL, FALSE, FALSE, NULL);
				bsd->hSockReqHandled = CreateEvent(NULL, FALSE, FALSE, NULL);

				wc.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
				wc.lpfnWndProc = SocketWindowProc;
				wc.cbClsExtra = 0;
				wc.cbWndExtra = 0;
				wc.hInstance = GetModuleHandle(NULL);
				wc.hIcon = NULL;
				wc.hCursor = LoadCursor(NULL, IDC_ARROW);
				wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
				wc.lpszMenuName = 0;
				wc.lpszClassName = _T("SocketFun");
				RegisterClass(&wc);
				bsd->hSockWnd = CreateWindowEx(0,
					_T("SocketFun"), _T("Amiberry Socket Window"),
					WS_POPUP,
					0, 0, 1, 1,
					NULL, NULL, 0, NULL);
				bsd->hSockThread = THREAD(sock_thread, NULL);
				if (!bsd->hSockWnd) {
					write_log(_T("bsdsocket initialization failed\n"));
					deinit_socket_layer();
					return 0;
				}
			}
		}
	}
	return result;
}

static void close_selectget_threads(void)
{
	int i;

	for (i = 0; i < MAX_SELECT_THREADS; i++) {
		if (bsd->hEvents[i]) {
			HANDLE h = bsd->hEvents[i];
			bsd->hEvents[i] = NULL;
			CloseHandle(h);
		}
		if (bsd->hThreads[i]) {
			CloseHandle(bsd->hThreads[i]);
			bsd->hThreads[i] = NULL;
		}
	}

	for (i = 0; i < MAX_GET_THREADS; i++) {
		if (bsd->hGetThreads[i]) {
			HANDLE h = bsd->hGetThreads[i];
			bsd->hGetThreads[i] = NULL;
			CloseHandle(h);
		}
		if (bsd->hGetEvents[i]) {
			CloseHandle(bsd->hGetEvents[i]);
			bsd->hGetEvents[i] = NULL;
		}
		if (bsd->hGetEvents2[i]) {
			CloseHandle(bsd->hGetEvents2[i]);
			bsd->hGetEvents2[i] = NULL;
		}
		bsd->threadGetargs_inuse[i] = 0;
	}
}

void deinit_socket_layer(void)
{
	if (!bsd)
		return;
	if (bsd->hSockThread) {
		HANDLE t = bsd->hSockThread;
		DeleteCriticalSection(&bsd->csSigQueueLock);
		DeleteCriticalSection(&bsd->SockThreadCS);
		bsd->hSockThread = NULL;
		SetEvent(bsd->hSockReq);
		WaitForSingleObject(t, INFINITE);
		CloseHandle(t);
		CloseHandle(bsd->hSockReq);
		CloseHandle(bsd->hSockReqHandled);
		bsd->hSockReq = NULL;
		bsd->hSockThread = NULL;
		bsd->hSockReqHandled = NULL;
		DestroyWindow(bsd->hSockWnd);
		bsd->hSockWnd = NULL;
		UnregisterClass(_T("SocketFun"), GetModuleHandle(NULL));
	}
	close_selectget_threads();
	WSACleanup();
	xfree(bsd);
	bsd = NULL;
}

#ifdef BSDSOCKET

void locksigqueue(void)
{
	EnterCriticalSection(&bsd->csSigQueueLock);
}

void unlocksigqueue(void)
{
	LeaveCriticalSection(&bsd->csSigQueueLock);
}

/* Async message management */

static void sockmsg(unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	SB;
	unsigned int index;
	int sdi;
	TrapContext *ctx = NULL;

	index = (msg - 0xb000) / 2;
	sb = bsd->asyncsb[index];

	if (!(msg & 1)) {
		if ((SOCKET)wParam != bsd->asyncsock[index]) {
			WSAAsyncSelect((SOCKET)wParam, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, 0, 0);
			BSDTRACE((_T("unknown sockmsg %d\n"), index));
			return;
		}

		sdi = bsd->asyncsd[index] - 1;

		if (sb && !(sb->ftable[sdi] & SF_BLOCKINGINPROGRESS) && sb->mtable[sdi]) {
			long wsbevents = WSAGETSELECTEVENT(lParam);
			int fmask = 0;

			if (wsbevents & FD_READ) fmask = REP_READ;
			else if (wsbevents & FD_WRITE) fmask = REP_WRITE;
			else if (wsbevents & FD_OOB) fmask = REP_OOB;
			else if (wsbevents & FD_ACCEPT) fmask = REP_ACCEPT;
			else if (wsbevents & FD_CONNECT) fmask = REP_CONNECT;
			else if (wsbevents & FD_CLOSE) fmask = REP_CLOSE;

			if (WSAGETSELECTERROR(lParam)) fmask |= REP_ERROR;

			if (sb->ftable[sdi] & fmask) sb->ftable[sdi] |= fmask << 8;

			addtosigqueue(sb, 1);
			return;
		}
	}

	locksigqueue();

	if (sb != NULL) {
		bsd->asyncsb[index] = NULL;

		if (WSAGETASYNCERROR(lParam)) {
			bsdsocklib_seterrno(NULL, sb, WSAGETASYNCERROR(lParam) - WSABASEERR);
			if (sb->sb_errno >= 1001 && sb->sb_errno <= 1005) {
				TrapContext *actx = alloc_host_thread_trap_context();
				bsdsocklib_setherrno(actx, sb, sb->sb_errno - 1000);
				free_host_trap_context(actx);
			} else if (sb->sb_errno == 55) {
				write_log(_T("BSDSOCK: ERROR - Buffer overflow - %d bytes requested\n"),
					WSAGETASYNCBUFLEN(lParam));
			}
		} else {
			TrapContext *actx = alloc_host_thread_trap_context();
			bsdsocklib_seterrno(actx, sb, 0);
			free_host_trap_context(actx);
		}

		SETSIGNAL;
	}

	unlocksigqueue();
}

static unsigned int allocasyncmsg(TrapContext *ctx, SB, uae_u32 sd, SOCKET s)
{
	int i;

	locksigqueue();
	for (i = bsd->asyncindex + 1; i != bsd->asyncindex; i++) {
		if (i >= MAXPENDINGASYNC)
			i = 0;
		if (!bsd->asyncsb[i]) {
			bsd->asyncsb[i] = sb;
			if (++bsd->asyncindex >= MAXPENDINGASYNC)
				bsd->asyncindex = 0;
			unlocksigqueue();
			if (s == INVALID_SOCKET) {
				return i * 2 + 0xb001;
			} else {
				bsd->asyncsd[i] = sd;
				bsd->asyncsock[i] = s;
				return i * 2 + 0xb000;
			}
		}
	}
	unlocksigqueue();

	bsdsocklib_seterrno(ctx, sb, 12); // ENOMEM
	write_log(_T("BSDSOCK: ERROR - Async operation completion table overflow\n"));

	return 0;
}

static void cancelasyncmsg(TrapContext *ctx, unsigned int wMsg)
{
	SB;

	wMsg = (wMsg - 0xb000) / 2;

	sb = bsd->asyncsb[wMsg];

	if (sb != NULL) {
		bsd->asyncsb[wMsg] = NULL;
		CANCELSIGNAL;
	}
}

void sockabort(SB)
{
	locksigqueue();
	unlocksigqueue();
}

static void setWSAAsyncSelect(SB, uae_u32 sd, SOCKET s, long lEvent)
{
	if (sb->mtable[sd - 1]) {
		long wsbevents = 0;
		long eventflags;
		int i;
		locksigqueue();

		eventflags = sb->ftable[sd - 1] & REP_ALL;

		if (eventflags & REP_ACCEPT) wsbevents |= FD_ACCEPT;
		if (eventflags & REP_CONNECT) wsbevents |= FD_CONNECT;
		if (eventflags & REP_OOB) wsbevents |= FD_OOB;
		if (eventflags & REP_READ) wsbevents |= FD_READ;
		if (eventflags & REP_WRITE) wsbevents |= FD_WRITE;
		if (eventflags & REP_CLOSE) wsbevents |= FD_CLOSE;
		wsbevents |= lEvent;
		i = (sb->mtable[sd - 1] - 0xb000) / 2;
		bsd->asyncsb[i] = sb;
		bsd->asyncsd[i] = sd;
		bsd->asyncsock[i] = s;
		WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, sb->mtable[sd - 1], wsbevents);

		unlocksigqueue();
	}
}

static void prephostaddr(SOCKADDR_IN *addr)
{
	addr->sin_family = AF_INET;
}

static void prepamigaaddr(struct sockaddr *realpt, int len)
{
	((uae_u8*)realpt)[1] = *((uae_u8*)realpt);
	*((uae_u8*)realpt) = len;
}

/* socketbase init/cleanup */

int host_sbinit(TrapContext *context, SB)
{
	sb->sockAbort = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sb->sockAbort == INVALID_SOCKET)
		return 0;
	if ((sb->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
		return 0;

	sb->mtable = xcalloc(unsigned int, sb->dtablesize);

	return 1;
}

void host_closesocketquick(SOCKET s)
{
	BOOL b = 1;

	if (s) {
		setsockopt(s, SOL_SOCKET, SO_DONTLINGER, (uae_char*)&b, sizeof(b));
		shutdown(s, 1);
		closesocket(s);
	}
}

void host_sbcleanup(SB)
{
	int i;

	if (!sb) {
		close_selectget_threads();
		return;
	}

	for (i = 0; i < MAXPENDINGASYNC; i++) {
		if (bsd->asyncsb[i] == sb)
			bsd->asyncsb[i] = NULL;
	}

	if (sb->hEvent != NULL) {
		CloseHandle(sb->hEvent);
		sb->hEvent = NULL;
	}

	for (i = sb->dtablesize; i--;) {
		if (sb->dtable[i] != INVALID_SOCKET)
			host_closesocketquick(sb->dtable[i]);
		sb->dtable[i] = INVALID_SOCKET;

		if (sb->mtable && sb->mtable[i])
			bsd->asyncsb[(sb->mtable[i] - 0xb000) / 2] = NULL;
	}

	if (sb->sockAbort != INVALID_SOCKET) {
		shutdown(sb->sockAbort, 1);
		closesocket(sb->sockAbort);
		sb->sockAbort = INVALID_SOCKET;
	}

	free(sb->mtable);
	sb->mtable = NULL;
}

void host_sbreset(void)
{
	int i;
	for (i = 0; i < MAXPENDINGASYNC; i++) {
		bsd->asyncsb[i] = 0;
		bsd->asyncsock[i] = 0;
		bsd->asyncsd[i] = 0;
	}
	for (i = 0; i < MAX_GET_THREADS; i++) {
		bsd->threadargsw[i] = 0;
	}
}

/* Socket window procedure */

static LRESULT CALLBACK SocketWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message >= 0xB000 && message < 0xB000 + MAXPENDINGASYNC * 2) {
		BSDTRACE((_T("sockmsg(0x%x[%d], 0x%x, 0x%x)\n"), message, (message - 0xb000) / 2, wParam, lParam));
		sockmsg(message, wParam, lParam);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

/* Sock thread request packet */

typedef enum {
	connect_req,
	recvfrom_req,
	sendto_req,
	last_req
} threadsock_e;

struct threadsock_packet {
	threadsock_e packet_type;
	union {
		struct sendto_params {
			uae_char *buf;
			uae_char *realpt;
			uae_u32 sd;
			uae_u32 len;
			uae_u32 flags;
			uae_u32 to;
			uae_u32 tolen;
		} sendto_s;
		struct recvfrom_params {
			uae_char *realpt;
			uae_u32 addr;
			uae_u32 len;
			uae_u32 flags;
			struct sockaddr *rp_addr;
			int *hlen;
		} recvfrom_s;
		struct connect_params {
			uae_char *buf;
			uae_u32 namelen;
		} connect_s;
	} params;
	SOCKET s;
	SB;
	int wscnt;
} sockreq;

static BOOL HandleStuff(void)
{
	BOOL quit = FALSE;
	SB = NULL;
	BOOL handled = TRUE;
	TrapContext *ctx = NULL;

	if (bsd->hSockReq) {
		BSDTRACE((_T("sockreq start %d:%d\n"), sockreq.packet_type, sockreq.wscnt));
		switch (sockreq.packet_type) {
		case connect_req:
			sockreq.sb->resultval = connect(sockreq.s, (struct sockaddr *)(sockreq.params.connect_s.buf), sockreq.params.connect_s.namelen);
			break;
		case sendto_req:
			if (sockreq.params.sendto_s.to) {
				sockreq.sb->resultval = sendto(sockreq.s, sockreq.params.sendto_s.realpt, sockreq.params.sendto_s.len, sockreq.params.sendto_s.flags, (struct sockaddr *)(sockreq.params.sendto_s.buf), sockreq.params.sendto_s.tolen);
			} else {
				sockreq.sb->resultval = send(sockreq.s, sockreq.params.sendto_s.realpt, sockreq.params.sendto_s.len, sockreq.params.sendto_s.flags);
			}
			break;
		case recvfrom_req:
			if (sockreq.params.recvfrom_s.addr) {
				sockreq.sb->resultval = recvfrom(sockreq.s, sockreq.params.recvfrom_s.realpt, sockreq.params.recvfrom_s.len,
					sockreq.params.recvfrom_s.flags, sockreq.params.recvfrom_s.rp_addr,
					sockreq.params.recvfrom_s.hlen);
			} else {
				sockreq.sb->resultval = recv(sockreq.s, sockreq.params.recvfrom_s.realpt, sockreq.params.recvfrom_s.len,
					sockreq.params.recvfrom_s.flags);
			}
			if (sockreq.sb->resultval == -1 && WSAGetLastError() == WSAEMSGSIZE) {
				int v = 0, len = sizeof(v);
				if (!getsockopt(sockreq.s, SOL_SOCKET, SO_TYPE, (char*)&v, &len)) {
					if (v == SOCK_DGRAM) {
						sockreq.sb->resultval = sockreq.params.recvfrom_s.len;
					}
				}
				if (sockreq.sb->resultval == -1) {
					WSASetLastError(WSAEMSGSIZE);
				}
			}
			break;
		case last_req:
		default:
			write_log(_T("BSDSOCK: Invalid sock-thread request!\n"));
			handled = FALSE;
			break;
		}
		if (handled) {
			if (sockreq.sb->resultval == SOCKET_ERROR) {
				sb = sockreq.sb;
				SETERRNO;
			}
		}
		BSDTRACE((_T("sockreq end %d,%d,%d:%d\n"), sockreq.packet_type, sockreq.sb->resultval, sockreq.sb->sb_errno, sockreq.wscnt));
		SetEvent(bsd->hSockReqHandled);
	} else {
		quit = TRUE;
	}
	return quit;
}

static unsigned int sock_thread2(void *blah)
{
	unsigned int result = 0;
	HANDLE WaitHandle;
	MSG msg;

	if (bsd->hSockWnd) {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

		while (bsd->hSockThread && bsd->hSockWnd) {
			DWORD wait;
			WaitHandle = bsd->hSockReq;
			wait = MsgWaitForMultipleObjects(1, &WaitHandle, FALSE, INFINITE, QS_POSTMESSAGE);
			if (wait == WAIT_ABANDONED_0)
				break;
			if (wait == WAIT_OBJECT_0) {
				if (!bsd->hSockThread || !bsd->hSockWnd)
					break;
				if (HandleStuff())
					break;
			} else if (wait == WAIT_OBJECT_0 + 1) {
				if (!bsd->hSockThread || !bsd->hSockWnd)
					break;
				while (PeekMessage(&msg, NULL, WM_USER, 0xB000 + MAXPENDINGASYNC * 2, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}
	write_log(_T("BSDSOCK: We have exited our sock_thread()\n"));
	THREADEND(result);
	return result;
}

static unsigned int __stdcall sock_thread(void *p)
{
	return sock_thread2(p);
}

/* host_* functions */

int host_dup2socket(TrapContext *ctx, SB, int fd1, int fd2)
{
	SOCKET s1, s2;

	BSDTRACE((_T("dup2socket(%d,%d) -> "), fd1, fd2));
	fd1++;

	s1 = getsock(ctx, sb, fd1);
	if (s1 != INVALID_SOCKET) {
		if (fd2 != -1) {
			if ((unsigned int)(fd2) >= (unsigned int)sb->dtablesize) {
				BSDTRACE((_T("Bad file descriptor (%d)\n"), fd2));
				bsdsocklib_seterrno(ctx, sb, 9);
			}
			fd2++;
			s2 = getsock(ctx, sb, fd2);
			if (s2 != INVALID_SOCKET) {
				shutdown(s2, 1);
				closesocket(s2);
			}
			setsd(ctx, sb, fd2, s1);
			BSDTRACE((_T("0\n")));
			return 0;
		} else {
			fd2 = getsd(ctx, sb, 1);
			setsd(ctx, sb, fd2, s1);
			BSDTRACE((_T("%d\n"), fd2));
			return (fd2 - 1);
		}
	}
	BSDTRACE((_T("-1\n")));
	return -1;
}

int host_socket(TrapContext *ctx, SB, int af, int type, int protocol)
{
	int sd;
	SOCKET s;
	unsigned long nonblocking = 1;
	int faketype;

	BSDTRACE((_T("socket(%s,%s,%d) -> "),
		af == AF_INET ? _T("AF_INET") : _T("AF_other"),
		type == SOCK_STREAM ? _T("SOCK_STREAM") : type == SOCK_DGRAM ? _T("SOCK_DGRAM ") : _T("SOCK_RAW"), protocol));

	faketype = type;
	if (protocol == IPPROTO_UDP && type == SOCK_RAW && !rawsockets)
		faketype = SOCK_DGRAM;

	if ((s = socket(af, faketype, protocol)) == INVALID_SOCKET) {
		SETERRNO;
		BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
		return -1;
	} else {
		sd = getsd(ctx, sb, s);
	}

	sb->ftable[sd - 1] = SF_BLOCKING;
	if (faketype == SOCK_DGRAM || protocol != IPPROTO_TCP)
		sb->ftable[sd - 1] |= SF_DGRAM;

	ioctlsocket(s, FIONBIO, &nonblocking);
	BSDTRACE((_T(" -> Socket=%d %x\n"), sd, s));

	if (type == SOCK_RAW) {
		if (protocol == IPPROTO_UDP) {
			sb->ftable[sd - 1] |= SF_RAW_UDP;
		} else if (protocol == IPPROTO_ICMP) {
			struct sockaddr_in sin = { 0 };
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			if (bind(s, (struct sockaddr *)&sin, sizeof(sin)))
				write_log(_T("IPPROTO_ICMP socket bind() failed: %d\n"), WSAGetLastError());
		} else if (protocol == IPPROTO_RAW) {
			sb->ftable[sd - 1] |= SF_RAW_RAW;
		}
	}
	callfdcallback(ctx, sb, sd - 1, FDCB_ALLOC);
	return sd - 1;
}

uae_u32 host_bind(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	uae_char buf[MAXADDRLEN];
	uae_u32 success = 0;
	SOCKET s;

	sd++;
	BSDTRACE((_T("bind(%d,0x%x,%d) -> "), sd, name, namelen));
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (namelen <= sizeof buf) {
			if (!addr_valid(_T("host_bind"), name, namelen))
				return 0;
			memcpy(buf, get_real_address(name), namelen);
			prephostaddr((SOCKADDR_IN *)buf);

			if ((success = bind(s, (struct sockaddr *)buf, namelen)) != 0) {
				SETERRNO;
				BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
			} else
				BSDTRACE((_T("OK\n")));
		} else
			write_log(_T("BSDSOCK: ERROR - Excessive namelen (%d) in bind()!\n"), namelen);
	}

	return success;
}

uae_u32 host_listen(TrapContext *ctx, SB, uae_u32 sd, uae_u32 backlog)
{
	SOCKET s;
	uae_u32 success = -1;

	sd++;
	BSDTRACE((_T("listen(%d,%d) -> "), sd, backlog));
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if ((success = listen(s, backlog)) != 0) {
			SETERRNO;
			BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
		} else
			BSDTRACE((_T("OK\n")));
	}
	return success;
}

void host_accept(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	struct sockaddr *rp_name, *rp_nameuae;
	struct sockaddr sockaddr;
	int hlen, hlenuae = 0;
	SOCKET s, s2;
	unsigned int wMsg;

	sd++;
	if (name != 0) {
		if (!trap_valid_address(ctx, name, sizeof(struct sockaddr)) || !trap_valid_address(ctx, namelen, 4))
			return;
		rp_nameuae = rp_name = (struct sockaddr *)get_real_address(name);
		hlenuae = hlen = trap_get_long(ctx, namelen);
		if (hlenuae < (int)sizeof(sockaddr)) {
			rp_name = &sockaddr;
			hlen = sizeof(sockaddr);
		}
	} else {
		rp_name = &sockaddr;
		hlen = sizeof(sockaddr);
	}
	BSDTRACE((_T("accept(%d,%d,%d) -> "), sd, name, hlenuae));

	s = getsock(ctx, sb, (int)sd);

	if (s != INVALID_SOCKET) {
		BEGINBLOCKING;

		s2 = accept(s, rp_name, &hlen);

		if (s2 == INVALID_SOCKET) {
			SETERRNO;

			if ((sb->ftable[sd - 1] & SF_BLOCKING) && sb->sb_errno == WSAEWOULDBLOCK - WSABASEERR) {
				if (sb->mtable[sd - 1] || (wMsg = allocasyncmsg(ctx, sb, sd, s)) != 0) {
					if (sb->mtable[sd - 1] == 0) {
						WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, wMsg, FD_ACCEPT);
					} else {
						setWSAAsyncSelect(sb, sd, s, FD_ACCEPT);
					}

					WAITSIGNAL;

					if (sb->mtable[sd - 1] == 0) {
						cancelasyncmsg(ctx, wMsg);
					} else {
						setWSAAsyncSelect(sb, sd, s, 0);
					}

					if (sb->eintr) {
						BSDTRACE((_T("[interrupted]\n")));
						ENDBLOCKING;
						return;
					}

					s2 = accept(s, rp_name, &hlen);

					if (s2 == INVALID_SOCKET) {
						SETERRNO;
						if (sb->sb_errno == WSAEWOULDBLOCK - WSABASEERR)
							write_log(_T("BSDSOCK: ERROR - accept() would block despite FD_ACCEPT message\n"));
					}
				}
			}
		}

		if (s2 == INVALID_SOCKET) {
			sb->resultval = -1;
			BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
		} else {
			sb->resultval = getsd(ctx, sb, s2);
			sb->ftable[sb->resultval - 1] = sb->ftable[sd - 1];
			callfdcallback(ctx, sb, sb->resultval - 1, FDCB_ALLOC);
			sb->resultval--;
			if (rp_name != 0) {
				if (hlen <= hlenuae) {
					prepamigaaddr(rp_name, hlen);
					if (namelen != 0) {
						trap_put_long(ctx, namelen, hlen);
					}
				} else {
					if (hlenuae != 0) {
						prepamigaaddr(rp_name, hlenuae);
						memcpy(rp_nameuae, rp_name, hlenuae);
						trap_put_long(ctx, namelen, hlenuae);
					}
				}
			}
			BSDTRACE((_T("%d/%d\n"), sb->resultval, hlen));
		}

		ENDBLOCKING;
	}
}

void host_connect(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	SOCKET s;
	unsigned int wMsg;
	uae_char buf[MAXADDRLEN];
	static int wscounter;
	int wscnt;

	sd++;
	wscnt = ++wscounter;

	BSDTRACE((_T("connect(%d,0x%x,%d):%d -> "), sd, name, namelen, wscnt));

	if (!addr_valid(_T("host_connect"), name, namelen))
		return;

	s = getsock(ctx, sb, (int)sd);

	if (s != INVALID_SOCKET) {
		if (namelen <= MAXADDRLEN) {
			if (sb->mtable[sd - 1] || (wMsg = allocasyncmsg(ctx, sb, sd, s)) != 0) {
				if (sb->mtable[sd - 1] == 0) {
					WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, wMsg, FD_CONNECT);
				} else {
					setWSAAsyncSelect(sb, sd, s, FD_CONNECT);
				}

				BEGINBLOCKING;
				PREPARE_THREAD;

				memcpy(buf, get_real_address(name), namelen);
				prephostaddr((SOCKADDR_IN *)buf);

				sockreq.packet_type = connect_req;
				sockreq.s = s;
				sockreq.sb = sb;
				sockreq.params.connect_s.buf = buf;
				sockreq.params.connect_s.namelen = namelen;
				sockreq.wscnt = wscnt;

				TRIGGER_THREAD;

				if (sb->resultval) {
					if (sb->sb_errno == WSAEWOULDBLOCK - WSABASEERR) {
						if (sb->ftable[sd - 1] & SF_BLOCKING) {
							bsdsocklib_seterrno(ctx, sb, 0);

							WAITSIGNAL;

							if (sb->eintr) {
								shutdown(s, 1);
								closesocket(s);
								sb->dtable[sd - 1] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
							}
						} else {
							bsdsocklib_seterrno(ctx, sb, 36); // EINPROGRESS
						}
					} else {
						CANCELSIGNAL;
					}
				}

				ENDBLOCKING;
				if (sb->mtable[sd - 1] == 0) {
					cancelasyncmsg(ctx, wMsg);
				} else {
					setWSAAsyncSelect(sb, sd, s, 0);
				}
			}
		} else {
			write_log(_T("BSDSOCK: WARNING - Excessive namelen (%d) in connect():%d!\n"), namelen, wscnt);
		}
	}
	BSDTRACE((_T(" -> connect %d:%d\n"), sb->sb_errno, wscnt));
}

void host_sendto(TrapContext *ctx, SB, uae_u32 sd, uae_u32 msg, uae_u8 *hmsg, uae_u32 len, uae_u32 flags, uae_u32 to, uae_u32 tolen)
{
	SOCKET s;
	char *realpt;
	unsigned int wMsg;
	uae_char buf[MAXADDRLEN];
	SOCKADDR_IN *sa = NULL;
	int iCut = 0;
	static int wscounter;
	int wscnt;

	wscnt = ++wscounter;

	sd++;
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (hmsg == NULL) {
			if (!addr_valid(_T("host_sendto1"), msg, 4))
				return;
			realpt = (char*)get_real_address(msg);
		} else {
			realpt = (char*)hmsg;
		}

		if (to) {
			if (tolen > sizeof buf) {
				write_log(_T("BSDSOCK: WARNING - Target address in sendto() too large (%d):%d!\n"), tolen, wscnt);
				sb->resultval = -1;
				bsdsocklib_seterrno(ctx, sb, 22);
				goto sendto_error;
			} else {
				if (!addr_valid(_T("host_sendto2"), to, tolen))
					return;
				memcpy(buf, get_real_address(to), tolen);
				sa = (SOCKADDR_IN*)buf;
				prephostaddr(sa);
			}
		}
		if (sb->ftable[sd - 1] & SF_RAW_RAW) {
			if (realpt[9] == IPPROTO_ICMP) {
				struct sockaddr_in sin;
				shutdown(s, 1);
				closesocket(s);
				s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
				sin.sin_family = AF_INET;
				sin.sin_addr.s_addr = INADDR_ANY;
				sin.sin_port = htons(realpt[20] * 256 + realpt[21]);
				bind(s, (struct sockaddr *)&sin, sizeof(sin));
				sb->dtable[sd - 1] = s;
				sb->ftable[sd - 1] &= ~SF_RAW_RAW;
				sb->ftable[sd - 1] |= SF_RAW_RICMP;
			} else if (realpt[9] == IPPROTO_UDP) {
				struct sockaddr_in sin;
				shutdown(s, 1);
				closesocket(s);
				s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				sin.sin_family = AF_INET;
				sin.sin_addr.s_addr = INADDR_ANY;
				sin.sin_port = htons(realpt[20] * 256 + realpt[21]);
				bind(s, (struct sockaddr *)&sin, sizeof(sin));
				sb->dtable[sd - 1] = s;
				sb->ftable[sd - 1] &= ~SF_RAW_RAW;
				sb->ftable[sd - 1] |= SF_RAW_RUDP;
			} else {
				write_log(_T("Unknown RAW protocol %d\n"), realpt[9]);
				sb->resultval = -1;
				bsdsocklib_seterrno(ctx, sb, 22);
				goto sendto_error;
			}
		}

		BEGINBLOCKING;

		for (;;) {
			PREPARE_THREAD;

			if (sb->ftable[sd - 1] & SF_RAW_UDP) {
				sa->sin_port = htons(realpt[2] * 256 + realpt[3]);
				iCut = 8;
			} else if (sb->ftable[sd - 1] & SF_RAW_RUDP) {
				int iTTL = realpt[8];
				setsockopt(s, IPPROTO_IP, IP_TTL, (char*)&iTTL, sizeof(iTTL));
				sa->sin_port = htons(realpt[22] * 256 + realpt[23]);
				iCut = 28;
			} else if (sb->ftable[sd - 1] & SF_RAW_RICMP) {
				int iTTL = realpt[8];
				setsockopt(s, IPPROTO_IP, IP_TTL, (char*)&iTTL, sizeof(iTTL));
				iCut = 20;
			}

			sockreq.params.sendto_s.realpt = realpt + iCut;
			sockreq.params.sendto_s.len = len - iCut;
			sockreq.packet_type = sendto_req;
			sockreq.s = s;
			sockreq.sb = sb;
			sockreq.params.sendto_s.buf = buf;
			sockreq.params.sendto_s.sd = sd;
			sockreq.params.sendto_s.flags = flags;
			sockreq.params.sendto_s.to = to;
			sockreq.params.sendto_s.tolen = tolen;
			sockreq.wscnt = wscnt;

			TRIGGER_THREAD;

			sb->resultval += iCut;

			if (sb->resultval == -1) {
				if (sb->sb_errno != WSAEWOULDBLOCK - WSABASEERR || !(sb->ftable[sd - 1] & SF_BLOCKING))
					break;
			} else {
				realpt += sb->resultval;
				len -= sb->resultval;
				sb->bytestransmitted += sb->resultval;
				if ((int)len <= 0)
					break;
				else
					continue;
			}

			if (sb->mtable[sd - 1] || (wMsg = allocasyncmsg(ctx, sb, sd, s)) != 0) {
				if (sb->mtable[sd - 1] == 0) {
					WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, wMsg, FD_WRITE);
				} else {
					setWSAAsyncSelect(sb, sd, s, FD_WRITE);
				}

				WAITSIGNAL;

				if (sb->mtable[sd - 1] == 0) {
					cancelasyncmsg(ctx, wMsg);
				} else {
					setWSAAsyncSelect(sb, sd, s, 0);
				}

				if (sb->eintr) {
					BSDTRACE((_T("[interrupted]\n")));
					return;
				}
			} else
				break;
		}

		ENDBLOCKING;

	} else
		sb->resultval = -1;

sendto_error:
	if (sb->resultval == -1)
		BSDTRACE((_T("sendto failed (%d):%d\n"), sb->sb_errno, wscnt));
	else
		BSDTRACE((_T("sendto %d:%d\n"), sb->resultval, wscnt));
}

void host_recvfrom(TrapContext *ctx, SB, uae_u32 sd, uae_u32 msg, uae_u8 *hmsg, uae_u32 len, uae_u32 flags, uae_u32 addr, uae_u32 addrlen)
{
	SOCKET s;
	uae_char *realpt;
	struct sockaddr *rp_addr = NULL;
	int hlen;
	unsigned int wMsg;
	int waitall, waitallgot;
	static int wscounter;
	int wscnt;

	wscnt = ++wscounter;

	sd++;
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (hmsg == NULL) {
			if (!addr_valid(_T("host_recvfrom1"), msg, 4))
				return;
			realpt = (char*)get_real_address(msg);
		} else {
			realpt = (char*)hmsg;
		}

		if (addr) {
			if (!addr_valid(_T("host_recvfrom1"), addrlen, 4))
				return;
			hlen = get_long(addrlen);
			if (!addr_valid(_T("host_recvfrom2"), addr, hlen))
				return;
			rp_addr = (struct sockaddr *)get_real_address(addr);
		}

		BEGINBLOCKING;

		waitall = flags & 0x40;
		flags &= ~0x40;
		waitallgot = 0;

		for (;;) {
			PREPARE_THREAD;

			sockreq.packet_type = recvfrom_req;
			sockreq.s = s;
			sockreq.sb = sb;
			sockreq.params.recvfrom_s.addr = addr;
			sockreq.params.recvfrom_s.flags = flags;
			sockreq.params.recvfrom_s.hlen = &hlen;
			sockreq.params.recvfrom_s.len = len;
			sockreq.params.recvfrom_s.realpt = realpt;
			sockreq.params.recvfrom_s.rp_addr = rp_addr;
			sockreq.wscnt = wscnt;

			TRIGGER_THREAD;

			if (waitall) {
				if (sb->resultval > 0) {
					int l = sb->resultval;
					realpt += l;
					len -= l;
					sb->bytesreceived += l;
					waitallgot += l;
					if ((int)len <= 0) {
						sb->resultval = waitallgot;
						break;
					} else {
						sb->sb_errno = WSAEWOULDBLOCK - WSABASEERR;
						sb->resultval = -1;
					}
				} else if (sb->resultval == 0) {
					sb->resultval = waitallgot;
				}
			}

			if (sb->resultval == -1) {
				if (sb->sb_errno == WSAEWOULDBLOCK - WSABASEERR && (sb->ftable[sd - 1] & SF_BLOCKING)) {
					if (sb->mtable[sd - 1] || (wMsg = allocasyncmsg(ctx, sb, sd, s)) != 0) {
						if (sb->mtable[sd - 1] == 0) {
							WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, wMsg, FD_READ | FD_CLOSE);
						} else {
							setWSAAsyncSelect(sb, sd, s, FD_READ | FD_CLOSE);
						}

						WAITSIGNAL;

						if (sb->mtable[sd - 1] == 0) {
							cancelasyncmsg(ctx, wMsg);
						} else {
							setWSAAsyncSelect(sb, sd, s, 0);
						}

						if (sb->eintr) {
							BSDTRACE((_T("[interrupted]\n")));
							return;
						}
					} else
						break;
				} else
					break;
			} else
				break;
		}

		ENDBLOCKING;

		if (addr) {
			prepamigaaddr(rp_addr, hlen);
			put_long(addrlen, hlen);
		}
	} else
		sb->resultval = -1;

	if (sb->resultval == -1)
		BSDTRACE((_T("recv failed (%d):%d\n"), sb->sb_errno, wscnt));
	else
		BSDTRACE((_T("recv %d:%d\n"), sb->resultval, wscnt));
}

uae_u32 host_shutdown(SB, uae_u32 sd, uae_u32 how)
{
	TrapContext *ctx = NULL;
	SOCKET s;

	BSDTRACE((_T("shutdown(%d,%d) -> "), sd, how));
	sd++;
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (shutdown(s, how)) {
			SETERRNO;
			BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
		} else {
			BSDTRACE((_T("OK\n")));
			return 0;
		}
	}

	return -1;
}

void host_setsockopt(SB, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 len)
{
	TrapContext *ctx = NULL;
	SOCKET s;
	uae_char buf[MAXADDRLEN];
	int i;

	BSDTRACE((_T("setsockopt(%d,%d,0x%x,0x%x[0x%x],%d) -> "), sd, (short)level, optname, optval, get_long(optval), len));
	sd++;
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (len > sizeof buf) {
			write_log(_T("BSDSOCK: WARNING - Excessive optlen in setsockopt() (%d)\n"), len);
			len = sizeof buf;
		}
		if (level == IPPROTO_IP && optname == IP_HDRINCL) {
			sb->resultval = 0;
			return;
		}
		for (i = 0; (uae_u32)i < len / 4; i++) {
			((long*)buf)[i] = get_long(optval + i * 4);
		}
		if (len - i == 2)
			((long*)buf)[i] = get_word(optval + i * 4);
		else if (len - i == 1)
			((long*)buf)[i] = get_byte(optval + i * 4);

		if (level == SOL_SOCKET && (optname == SO_SNDTIMEO || optname == SO_RCVTIMEO)) {
			uae_u32 millis = ((long*)buf)[0] * 1000 + ((long*)buf)[1] / 1000;
			((long*)buf)[0] = millis;
			len = 4;
		}

		if (level == SOL_SOCKET && optname == 0x2001) {
			long wsbevents = 0;
			uae_u32 eventflags = get_long(optval);

			sb->ftable[sd - 1] = (sb->ftable[sd - 1] & ~REP_ALL) | (eventflags & REP_ALL);

			if (eventflags & REP_ACCEPT) wsbevents |= FD_ACCEPT;
			if (eventflags & REP_CONNECT) wsbevents |= FD_CONNECT;
			if (eventflags & REP_OOB) wsbevents |= FD_OOB;
			if (eventflags & REP_READ) wsbevents |= FD_READ;
			if (eventflags & REP_WRITE) wsbevents |= FD_WRITE;
			if (eventflags & REP_CLOSE) wsbevents |= FD_CLOSE;

			if (sb->mtable[sd - 1] || (sb->mtable[sd - 1] = allocasyncmsg(ctx, sb, sd, s))) {
				WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, sb->mtable[sd - 1], wsbevents);
				sb->resultval = 0;
			} else
				sb->resultval = -1;
		} else {
			sb->resultval = setsockopt(s, level, optname, buf, len);
		}

		if (!sb->resultval) {
			BSDTRACE((_T("OK\n")));
			return;
		} else
			SETERRNO;

		BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
	}
}

uae_u32 host_getsockopt(TrapContext *ctx, SB, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 optlen)
{
	SOCKET s;
	uae_char buf[MAXADDRLEN];
	int len = sizeof buf;
	uae_u32 outlen;

	if (optval)
		outlen = get_long(optlen);
	else
		outlen = 0;

	BSDTRACE((_T("getsockopt(%d,%d,0x%x,0x%x,0x%x[%d]) -> "), sd, (short)level, optname, optval, optlen, outlen));
	sd++;
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (!getsockopt(s, level, optname, buf, &len)) {
			BSDTRACE((_T("0x%x, %d -> "), *((long*)buf), len));
			uae_u32 outcnt = 0;
			if (outlen) {
				if (level == SOL_SOCKET && (optname == SO_SNDTIMEO || optname == SO_RCVTIMEO)) {
					uae_u32 millis = *((long*)buf);
					((long*)buf)[0] = millis / 1000;
					((long*)buf)[1] = (millis % 1000) * 1000;
					len = 8;
				}
				for (int i = 0; i < len; i += 4) {
					uae_u32 v;
					if (len - i >= 4)
						v = *((long*)(buf + i));
					else if (len - i >= 2)
						v = *((short*)(buf + i));
					else
						v = buf[i];
					if (outlen >= 4) {
						put_long(optval + outcnt, v);
						outlen -= 4;
						outcnt += 4;
					} else if (outlen >= 2) {
						put_word(optval + outcnt, v);
						outlen -= 2;
						outcnt += 2;
					} else if (outlen > 0) {
						put_byte(optval + outcnt, v);
						outlen -= 1;
						outcnt += 1;
					}
				}
				put_long(optlen, outcnt);
			}
			BSDTRACE((_T("OK (%d,0x%x)\n"), outcnt, get_long(optval)));
			return 0;
		} else {
			SETERRNO;
			BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
		}
	}

	return -1;
}

uae_u32 host_getsockname(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	SOCKET s;
	int len;
	struct sockaddr *rp_name;

	sd++;
	if (!addr_valid(_T("host_getsockname1"), namelen, 4))
		return -1;
	len = get_long(namelen);

	BSDTRACE((_T("getsockname(%d,0x%x,%d) -> "), sd, name, len));

	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (!trap_valid_address(ctx, name, len))
			return -1;
		rp_name = (struct sockaddr *)get_real_address(name);

		if (getsockname(s, rp_name, &len)) {
			SETERRNO;
			BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
		} else {
			BSDTRACE((_T("%d\n"), len));
			prepamigaaddr(rp_name, len);
			put_long(namelen, len);
			return 0;
		}
	}

	return -1;
}

uae_u32 host_getpeername(TrapContext *ctx, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
	SOCKET s;
	int len;
	struct sockaddr *rp_name;

	sd++;
	if (!addr_valid(_T("host_getpeername1"), namelen, 4))
		return -1;
	len = get_long(namelen);

	BSDTRACE((_T("getpeername(%d,0x%x,%d) -> "), sd, name, len));

	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		if (!trap_valid_address(ctx, name, len))
			return -1;
		rp_name = (struct sockaddr *)get_real_address(name);

		if (getpeername(s, rp_name, &len)) {
			SETERRNO;
			BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));
		} else {
			BSDTRACE((_T("%d\n"), len));
			prepamigaaddr(rp_name, len);
			put_long(namelen, len);
			return 0;
		}
	}

	return -1;
}

/* Network interface queries */

#define MAX_NET_INTERFACES 30
struct bsdsock_interface_info {
	bool inuse;
	int mtu;
	int metric;
	uae_u8 mac[6];
	bool hasmac;
};
static INTERFACE_INFO *net_interfaces;
static bsdsock_interface_info *net_interfaces2;
static int net_interfaces_num;

static void get_net_if_extra(void)
{
	xfree(net_interfaces2);
	net_interfaces2 = NULL;
	DWORD ret = 16 * 1024;
	DWORD gaaFlags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_GATEWAYS;
	IP_ADAPTER_ADDRESSES *ipaa = (IP_ADAPTER_ADDRESSES*)malloc(ret);
	if (GetAdaptersAddresses(AF_INET, gaaFlags, NULL, ipaa, &ret) == ERROR_BUFFER_OVERFLOW) {
		xfree(ipaa);
		ipaa = (IP_ADAPTER_ADDRESSES*)malloc(ret);
		if (GetAdaptersAddresses(AF_INET, gaaFlags, NULL, ipaa, &ret) != ERROR_SUCCESS) {
			xfree(ipaa);
			ipaa = NULL;
		}
	}
	if (ipaa) {
		net_interfaces2 = xcalloc(struct bsdsock_interface_info, MAX_NET_INTERFACES);
		if (net_interfaces2) {
			for (int i = 0; i < net_interfaces_num; i++) {
				INTERFACE_INFO *ii = &net_interfaces[i];
				struct bsdsock_interface_info *ii2 = &net_interfaces2[i];
				IP_ADAPTER_ADDRESSES *ipa = ipaa;
				while (ipa && ii2->inuse == false) {
					if (ipa->Ipv4Enabled && (ipa->IfType == IF_TYPE_ETHERNET_CSMACD || ipa->IfType == IF_TYPE_IEEE80211)) {
						PIP_ADAPTER_UNICAST_ADDRESS pua = ipa->FirstUnicastAddress;
						while (pua) {
							if (pua->Address.iSockaddrLength == 16 && !memcmp(pua->Address.lpSockaddr, &ii->iiAddress.Address, 16)) {
								ii2->mtu = ipa->Mtu;
								if (ipa->PhysicalAddressLength == 6) {
									memcpy(ii2->mac, ipa->PhysicalAddress, 6);
									ii2->hasmac = true;
								}
								ii2->metric = ipa->Ipv4Metric;
								ii2->inuse = true;
							}
							pua = pua->Next;
						}
					}
					ipa = ipa->Next;
				}
			}
		}
		xfree(ipaa);
	}
}

static int get_net_if(TrapContext *ctx, uaecptr addr, bool extra)
{
	char name[17];
	trap_get_bytes(ctx, name, addr, 16);
	name[16] = 0;
	if (name[0] == 'l' && name[1] == 'o' && name[2] == 0) {
		for (int i = 0; i < net_interfaces_num; i++) {
			if (net_interfaces[i].iiFlags & IFF_LOOPBACK) {
				return i;
			}
		}
	}
	if (name[0] == 'e' && name[1] == 't' && name[2] == 'h') {
		int num = atoi(name + 3);
		if (num == 0 && name[3] != '0') {
			return -1;
		}
		if (num >= net_interfaces_num) {
			return -1;
		}
		if (extra && !net_interfaces2) {
			get_net_if_extra();
		}
		return num;
	}
	return -1;
}

uae_u32 host_IoctlSocket(TrapContext *ctx, SB, uae_u32 sd, uae_u32 request, uae_u32 arg)
{
	SOCKET s;
	uae_u32 data;
	int success = SOCKET_ERROR;

	BSDTRACE((_T("IoctlSocket(%d,0x%x,0x%x) "), sd, request, arg));
	sd++;
	s = getsock(ctx, sb, sd);

	if (s != INVALID_SOCKET) {
		switch (request) {
		case FIOSETOWN:
			sb->ownertask = trap_get_long(ctx, arg);
			success = 0;
			break;
		case FIOGETOWN:
			trap_put_long(ctx, arg, sb->ownertask);
			success = 0;
			break;
		case FIONBIO:
			BSDTRACE((_T("[FIONBIO] -> ")));
			if (trap_get_long(ctx, arg)) {
				BSDTRACE((_T("nonblocking\n")));
				sb->ftable[sd - 1] &= ~SF_BLOCKING;
			} else {
				BSDTRACE((_T("blocking\n")));
				sb->ftable[sd - 1] |= SF_BLOCKING;
			}
			success = 0;
			break;
		case FIONREAD:
			ioctlsocket(s, request, (u_long *)&data);
			BSDTRACE((_T("[FIONREAD] -> %d\n"), data));
			trap_put_long(ctx, arg, data);
			success = 0;
			break;
		case FIOASYNC:
			if (trap_get_long(ctx, arg)) {
				sb->ftable[sd - 1] |= REP_ALL;

				BSDTRACE((_T("[FIOASYNC] -> enabled\n")));
				if (sb->mtable[sd - 1] || (sb->mtable[sd - 1] = allocasyncmsg(ctx, sb, sd, s))) {
					WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, sb->mtable[sd - 1],
						FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE | FD_CLOSE);
					success = 0;
					break;
				}
			} else
				write_log(_T("BSDSOCK: WARNING - FIOASYNC disabling unsupported.\n"));
			success = -1;
			break;

		case SIOCGIFFLAGS:
			{
				int idx = get_net_if(ctx, arg, false);
				if (idx >= 0) {
					INTERFACE_INFO *ii = &net_interfaces[idx];
					uae_u16 flags = 0;
					if (ii->iiFlags & IFF_UP) {
						flags |= 1;
						flags |= 0x40;
					}
					if (ii->iiFlags & IFF_BROADCAST) flags |= 2;
					if (ii->iiFlags & IFF_LOOPBACK) flags |= 8;
					if (ii->iiFlags & IFF_POINTTOPOINT) flags |= 0x10;
					if (ii->iiFlags & IFF_MULTICAST) flags |= 0x8000;
					trap_put_word(ctx, arg + 16, flags);
					success = 0;
				}
			}
			break;
		case SIOCGIFMTU:
		case SIOCGIFMETRIC:
		case SIOCGIFPHYS:
			{
				int idx = get_net_if(ctx, arg, true);
				if (idx >= 0) {
					struct bsdsock_interface_info *ii2 = &net_interfaces2[idx];
					if (ii2->inuse) {
						if (request == SIOCGIFMETRIC) {
							trap_put_long(ctx, arg + 16, ii2->metric);
							success = 0;
						} else if (request == SIOCGIFMTU) {
							trap_put_long(ctx, arg + 16, ii2->mtu);
							success = 0;
						} else if (request == SIOCGIFPHYS) {
							if (ii2->hasmac) {
								trap_put_bytes(ctx, ii2->mac, arg + 16, 6);
								success = 0;
							}
						}
					}
				}
			}
			break;
		case SIOCGIFADDR:
		case SIOCGIFNETMASK:
		case SIOCGIFBRDADDR:
			{
				int idx = get_net_if(ctx, arg, false);
				if (idx >= 0) {
					INTERFACE_INFO *ii = &net_interfaces[idx];
					sockaddr_gen *sa = &ii->iiAddress;
					if (request == SIOCGIFNETMASK)
						sa = &ii->iiNetmask;
					else if (request == SIOCGIFBRDADDR)
						sa = &ii->iiBroadcastAddress;
					trap_put_byte(ctx, arg + 16, 16);
					trap_put_byte(ctx, arg + 17, (uae_u8)sa->Address.sa_family);
					trap_put_bytes(ctx, sa->Address.sa_data, arg + 16 + 2, 14);
					success = 0;
				}
			}
			break;
		case SIOCGIFCONF:
			{
				xfree(net_interfaces);
				xfree(net_interfaces2);
				net_interfaces2 = NULL;
				net_interfaces_num = 0;
				net_interfaces = xcalloc(INTERFACE_INFO, MAX_NET_INTERFACES);
				if (net_interfaces) {
					DWORD ret;
					if (WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, (LPVOID*)net_interfaces, MAX_NET_INTERFACES * sizeof(INTERFACE_INFO), &ret, NULL, NULL) != SOCKET_ERROR) {
						net_interfaces_num = ret / sizeof(INTERFACE_INFO);
						int bytes = trap_get_long(ctx, arg);
						uaecptr ptr = trap_get_long(ctx, arg + 4);
						int ethcnt = 0;
						int bytecnt = 0;
						for (int i = 0; i < net_interfaces_num; i++) {
							INTERFACE_INFO *ii = &net_interfaces[i];
							if (bytes < 32) break;
							if (ii->iiFlags & IFF_LOOPBACK) {
								trap_put_string(ctx, "lo", ptr, 16);
							} else {
								char tmp[10];
								sprintf(tmp, "eth%d", ethcnt++);
								trap_put_string(ctx, tmp, ptr, 16);
							}
							trap_put_byte(ctx, ptr + 16, 16);
							trap_put_byte(ctx, ptr + 17, (uae_u8)ii->iiAddress.Address.sa_family);
							trap_put_bytes(ctx, ii->iiAddress.Address.sa_data, ptr + 16 + 2, 14);
							ptr += 32;
							bytes -= 32;
							bytecnt += 32;
						}
						trap_put_long(ctx, arg, bytecnt);
						success = 0;
					}
				}
			}
			break;

		default:
			write_log(_T("BSDSOCK: WARNING - Unknown IoctlSocket request: 0x%08lx\n"), request);
			bsdsocklib_seterrno(ctx, sb, 22);
			break;
		}
	}

	return success;
}

int host_CloseSocket(TrapContext *ctx, SB, int sd)
{
	unsigned int wMsg;
	SOCKET s;

	BSDTRACE((_T("CloseSocket(%d) -> "), sd));
	sd++;

	s = getsock(ctx, sb, sd);
	if (s != INVALID_SOCKET) {
		if (sb->mtable[sd - 1]) {
			bsd->asyncsb[(sb->mtable[sd - 1] - 0xb000) / 2] = NULL;
			sb->mtable[sd - 1] = 0;
		}

		if (checksd(ctx, sb, sd) == true)
			return 0;

		BEGINBLOCKING;

		for (;;) {
			shutdown(s, 1);
			if (!closesocket(s)) {
				releasesock(ctx, sb, sd);
				BSDTRACE((_T("OK\n")));
				return 0;
			}

			SETERRNO;

			if (sb->sb_errno != WSAEWOULDBLOCK - WSABASEERR || !(sb->ftable[sd - 1] & SF_BLOCKING))
				break;

			if ((wMsg = allocasyncmsg(ctx, sb, sd, s)) != 0) {
				WSAAsyncSelect(s, hWndSelector ? hAmigaSockWnd : bsd->hSockWnd, wMsg, FD_CLOSE);

				WAITSIGNAL;

				cancelasyncmsg(ctx, wMsg);

				if (sb->eintr) {
					BSDTRACE((_T("[interrupted]\n")));
					break;
				}
			} else
				break;
		}

		ENDBLOCKING;
	}

	BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));

	return -1;
}

/* WaitSelect thread pool */

static void makesocktable(TrapContext *ctx, SB, uae_u32 fd_set_amiga, struct fd_set *fd_set_win, int nfds, SOCKET addthis, const TCHAR *name)
{
	int i, j;
	uae_u32 currlong, mask;
	SOCKET s;

	if (addthis != INVALID_SOCKET) {
		*fd_set_win->fd_array = addthis;
		fd_set_win->fd_count = 1;
	} else
		fd_set_win->fd_count = 0;

	if (!fd_set_amiga) {
		fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
		return;
	}

	if (nfds > sb->dtablesize) {
		write_log(_T("BSDSOCK: ERROR - select()ing more sockets (%d) than socket descriptors available (%d)!\n"), nfds, sb->dtablesize);
		nfds = sb->dtablesize;
	}

	for (j = 0;; j += 32, fd_set_amiga += 4) {
		currlong = trap_get_long(ctx, fd_set_amiga);
		mask = 1;
		for (i = 0; i < 32; i++, mask <<= 1) {
			if (i + j > nfds) {
				fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
				return;
			}
			if (currlong & mask) {
				s = getsock(ctx, sb, j + i + 1);
				if (s != INVALID_SOCKET) {
					BSDTRACE((_T("%s:%d=%x\n"), name, fd_set_win->fd_count, s));
					fd_set_win->fd_array[fd_set_win->fd_count++] = s;
					if (fd_set_win->fd_count >= FD_SETSIZE) {
						write_log(_T("BSDSOCK: ERROR - select()ing more sockets (%d) than the hard-coded fd_set limit (%d)\n"), nfds, FD_SETSIZE);
						return;
					}
				}
			}
		}
	}

	fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
}

static void makesockbitfield(TrapContext *ctx, SB, uae_u32 fd_set_amiga, struct fd_set *fd_set_win, int nfds)
{
	int n, i, j, val, mask;
	SOCKET currsock;

	for (n = 0; n < nfds; n += 32) {
		val = 0;
		mask = 1;
		for (i = 0; i < 32; i++, mask <<= 1) {
			if ((currsock = getsock(ctx, sb, n + i + 1)) != INVALID_SOCKET) {
				for (j = fd_set_win->fd_count; j--;) {
					if (fd_set_win->fd_array[j] == currsock) {
						val |= mask;
						break;
					}
				}
			}
		}
		trap_put_long(ctx, fd_set_amiga, val);
		fd_set_amiga += 4;
	}
}

static void fd_zero(TrapContext *ctx, uae_u32 fdset, uae_u32 nfds)
{
	unsigned int i;
	for (i = 0; i < nfds; i += 32, fdset += 4)
		trap_put_long(ctx, fdset, 0);
}

static unsigned int thread_WaitSelect2(void *indexp)
{
	int index = *((int*)indexp);
	unsigned int result = 0, resultval;
	int wscnt;
	long nfds;
	uae_u32 readfds, writefds, exceptfds;
	uae_u32 timeout;
	struct fd_set readsocks, writesocks, exceptsocks;
	struct timeval tv;
	volatile struct threadargsw *args;
	TrapContext *ctx = NULL;

	SB;

	while (bsd->hEvents[index]) {
		if (WaitForSingleObject(bsd->hEvents[index], INFINITE) == WAIT_ABANDONED)
			break;
		if (bsd->hEvents[index] == NULL)
			break;

		if ((args = bsd->threadargsw[index]) != NULL) {
			sb = args->sb;
			nfds = args->nfds;
			readfds = args->readfds;
			writefds = args->writefds;
			exceptfds = args->exceptfds;
			timeout = args->timeout;
			wscnt = args->wscnt;

			makesocktable(ctx, sb, readfds, &readsocks, nfds, sb->sockAbort, _T("R"));
			if (writefds)
				makesocktable(ctx, sb, writefds, &writesocks, nfds, INVALID_SOCKET, _T("W"));
			if (exceptfds)
				makesocktable(ctx, sb, exceptfds, &exceptsocks, nfds, INVALID_SOCKET, _T("E"));

			if (timeout) {
				tv.tv_sec = get_long(timeout);
				tv.tv_usec = get_long(timeout + 4);
				BSDTRACE((_T("(to: %d.%06d) "), tv.tv_sec, tv.tv_usec));
			}

			BSDTRACE((_T("tWS2(%d) -> "), wscnt));

			resultval = select(nfds + 1,
				readsocks.fd_count > 0 ? &readsocks : NULL,
				writefds && writesocks.fd_count > 0 ? &writesocks : NULL,
				exceptfds && exceptsocks.fd_count > 0 ? &exceptsocks : NULL,
				timeout ? &tv : NULL);
			if (bsd->hEvents[index] == NULL)
				break;

			sb->resultval = resultval;
			if (sb->resultval == SOCKET_ERROR) {
				if (readsocks.fd_count > 1) {
					makesocktable(ctx, sb, readfds, &readsocks, nfds, INVALID_SOCKET, _T("R2"));
					tv.tv_sec = 0;
					tv.tv_usec = 10000;
					resultval = select(nfds + 1, &readsocks, writefds ? &writesocks : NULL, exceptfds ? &exceptsocks : NULL, &tv);
					if (bsd->hEvents[index] == NULL)
						break;
					sb->resultval = resultval;
				}
			}
			if (FD_ISSET(sb->sockAbort, &readsocks)) {
				BSDTRACE((_T("tWS2 abort %d:%d\n"), sb->resultval, wscnt));
				if (sb->resultval != SOCKET_ERROR) {
					sb->resultval--;
				}
			} else {
				sb->needAbort = 0;
			}
			if (sb->resultval == SOCKET_ERROR) {
				SETERRNO;
				if (readfds) fd_zero(ctx, readfds, nfds);
				if (writefds) fd_zero(ctx, writefds, nfds);
				if (exceptfds) fd_zero(ctx, exceptfds, nfds);
			} else {
				if (readfds) makesockbitfield(ctx, sb, readfds, &readsocks, nfds);
				if (writefds) makesockbitfield(ctx, sb, writefds, &writesocks, nfds);
				if (exceptfds) makesockbitfield(ctx, sb, exceptfds, &exceptsocks, nfds);
			}

			SETSIGNAL;

			bsd->threadargsw[index] = NULL;
			SetEvent(sb->hEvent);
		}
	}
	write_log(_T("BSDSOCK: thread_WaitSelect2 terminated\n"));
	THREADEND(result);
	return result;
}

static unsigned int __stdcall thread_WaitSelect(void *p)
{
	return thread_WaitSelect2(p);
}

void host_WaitSelect(TrapContext *ctx, SB, uae_u32 nfds, uae_u32 readfds, uae_u32 writefds, uae_u32 exceptfds, uae_u32 timeout, uae_u32 sigmp)
{
	static int wscount;
	uae_u32 sigs, wssigs;
	int i;
	struct threadargsw taw;
	int wscnt;

	wscnt = ++wscount;

	wssigs = sigmp ? trap_get_long(ctx, sigmp) : 0;

	BSDTRACE((_T("WaitSelect(%d,0x%x,0x%x,0x%x,0x%x,0x%x):%d\n"),
		nfds, readfds, writefds, exceptfds, timeout, wssigs, wscnt));

	if (!readfds && !writefds && !exceptfds && !timeout && !wssigs) {
		sb->resultval = 0;
		BSDTRACE((_T("-> [ignored]\n")));
		return;
	}
	if (wssigs) {
		trap_call_add_dreg(ctx, 0, 0);
		trap_call_add_dreg(ctx, 1, wssigs);
		sigs = trap_call_lib(ctx, sb->sysbase, -0x132) & wssigs;

		if (sigs) {
			BSDTRACE((_T("-> [preempted by signals 0x%08lx]\n"), sigs & wssigs));
			put_long(sigmp, sigs & wssigs);
			if (readfds) fd_zero(ctx, readfds, nfds);
			if (writefds) fd_zero(ctx, writefds, nfds);
			if (exceptfds) fd_zero(ctx, exceptfds, nfds);
			sb->resultval = 0;
			bsdsocklib_seterrno(ctx, sb, 0);
			return;
		}
	}
	if (nfds == 0) {
		if (wssigs != 0) {
			trap_call_add_dreg(ctx, 0, wssigs);
			sigs = trap_call_lib(ctx, sb->sysbase, -0x13e);
			trap_put_long(ctx, sigmp, sigs & wssigs);
		}
		if (readfds) fd_zero(ctx, readfds, nfds);
		if (writefds) fd_zero(ctx, writefds, nfds);
		if (exceptfds) fd_zero(ctx, exceptfds, nfds);
		sb->resultval = 0;
		return;
	}

	ResetEvent(sb->hEvent);
	sb->needAbort = 1;

	locksigqueue();

	for (i = 0; i < MAX_SELECT_THREADS; i++) {
		if (bsd->hThreads[i] && !bsd->threadargsw[i])
			break;
	}

	if (i >= MAX_SELECT_THREADS) {
		for (i = 0; i < MAX_SELECT_THREADS; i++) {
			if (!bsd->hThreads[i]) {
				bsd->hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
				bsd->hThreads[i] = THREAD(thread_WaitSelect, &threadindextable[i]);
				if (bsd->hEvents[i] == NULL || bsd->hThreads[i] == NULL) {
					bsd->hThreads[i] = 0;
					unlocksigqueue();
					write_log(_T("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n"),
						GetLastError());
					bsdsocklib_seterrno(ctx, sb, 12);
					sb->resultval = -1;
					return;
				}
				SetThreadPriority(bsd->hThreads[i], THREAD_PRIORITY_ABOVE_NORMAL);
				break;
			}
		}
	}

	unlocksigqueue();

	if (i >= MAX_SELECT_THREADS) {
		write_log(_T("BSDSOCK: ERROR - Too many select()s, %d\n"), wscnt);
	} else {
		SOCKET newsock = INVALID_SOCKET;

		taw.sb = sb;
		taw.nfds = nfds;
		taw.readfds = readfds;
		taw.writefds = writefds;
		taw.exceptfds = exceptfds;
		taw.timeout = timeout;
		taw.wscnt = wscnt;

		bsd->threadargsw[i] = &taw;

		SetEvent(bsd->hEvents[i]);

		trap_call_add_dreg(ctx, 0, (((uae_u32)1) << sb->signal) | sb->eintrsigs | wssigs);
		sigs = trap_call_lib(ctx, sb->sysbase, -0x13e);

		if (sb->needAbort) {
			if ((newsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
				write_log(_T("BSDSOCK: ERROR - Cannot create socket: %d, %d\n"), WSAGetLastError(), wscnt);
			shutdown(sb->sockAbort, 1);
			if (newsock != sb->sockAbort) {
				shutdown(sb->sockAbort, 1);
				closesocket(sb->sockAbort);
			}
		}

		WaitForSingleObject(sb->hEvent, INFINITE);

		CANCELSIGNAL;

		if (newsock != INVALID_SOCKET)
			sb->sockAbort = newsock;

		if (sigmp)
			trap_put_long(ctx, sigmp, sigs & wssigs);

		if (sigs & wssigs) {
			BSDTRACE((_T("[interrupted by signals 0x%08lx]:%d\n"), sigs & wssigs, wscnt));
			if (readfds) fd_zero(ctx, readfds, nfds);
			if (writefds) fd_zero(ctx, writefds, nfds);
			if (exceptfds) fd_zero(ctx, exceptfds, nfds);
			bsdsocklib_seterrno(ctx, sb, 0);
			sb->resultval = 0;
		} else if (sigs & sb->eintrsigs) {
			BSDTRACE((_T("[interrupted 0x%08x]:%d\n"), sigs & sb->eintrsigs, wscnt));
			sb->resultval = -1;
			bsdsocklib_seterrno(ctx, sb, 4);
			trap_call_add_dreg(ctx, 0, sigs & sb->eintrsigs);
			trap_call_add_dreg(ctx, 1, sigs & sb->eintrsigs);
			trap_call_lib(ctx, sb->sysbase, -0x132);
		}

		if (sb->resultval >= 0) {
			BSDTRACE((_T("WaitSelect, %d:%d\n"), sb->resultval, wscnt));
		} else {
			BSDTRACE((_T("WaitSelect error, %d errno %d:%d\n"), sb->resultval, sb->sb_errno, wscnt));
		}
	}
}

uae_u32 host_Inet_NtoA(TrapContext *ctx, SB, uae_u32 in)
{
	uae_char *addr;
	struct in_addr ina;
	uae_u32 scratchbuf;

	*(uae_u32 *)&ina = htonl(in);

	BSDTRACE((_T("Inet_NtoA(%x) -> "), in));

	if ((addr = inet_ntoa(ina)) != NULL) {
		scratchbuf = trap_get_areg(ctx, 6) + offsetof(struct UAEBSDBase, scratchbuf);
		strncpyha(ctx, scratchbuf, addr, SCRATCHBUFSIZE);
		BSDTRACE((_T("%s\n"), addr));
		return scratchbuf;
	} else
		SETERRNO;

	BSDTRACE((_T("failed (%d)\n"), sb->sb_errno));

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
	BSDTRACE((_T("inet_addr(%s) -> 0x%08lx\n"), cp_rp, addr));
	xfree(cp_rp);
	return addr;
}

/* DNS/proto/serv thread pool */

#define GET_STATE_FREE 0
#define GET_STATE_ACTIVE 1
#define GET_STATE_CANCEL 2
#define GET_STATE_FINISHED 3
#define GET_STATE_DONE 4
#define GET_STATE_REALLY_DONE 5

static unsigned int thread_get2(void *indexp)
{
	int index = *((int*)indexp);
	int wscnt;
	unsigned int result = 0;
	volatile struct threadargs *args;
	uae_u32 name;
	uae_u32 namelen;
	long addrtype;
	char *name_rp;
	SB;
	TrapContext *ctx = NULL;

	while (bsd->hGetEvents[index]) {
		if (WaitForSingleObject(bsd->hGetEvents[index], INFINITE) == WAIT_ABANDONED)
			break;
		if (bsd->hGetEvents[index] == NULL)
			break;

		args = bsd->threadGetargs[index];

		if (bsd->threadGetargs_inuse[index] == GET_STATE_ACTIVE) {
			wscnt = args->wscnt;
			sb = args->sb;

			if (args->args1 == 0) {
				struct hostent *host;
				name = args->args2;
				namelen = args->args3;
				addrtype = args->args4;
				if (addr_valid(_T("thread_get1"), name, 1))
					name_rp = (char*)get_real_address(name);
				else
					name_rp = "";

				if (addrtype == -1) {
					host = gethostbyname(name_rp);
				} else {
					host = gethostbyaddr(name_rp, namelen, addrtype);
				}
				if (bsd->threadGetargs_inuse[index] != GET_STATE_CANCEL) {
					if (host == 0) {
						SETERRNO;
					} else {
						bsdsocklib_seterrno(ctx, sb, 0);
						memcpy((void*)args->buf, host, sizeof(HOSTENT));
					}
				}

			} else if (args->args1 == 1) {
				struct protoent *proto;
				name = args->args2;
				if (addr_valid(_T("thread_get2"), name, 1))
					name_rp = (char*)get_real_address(name);
				else
					name_rp = "";
				proto = getprotobyname(name_rp);
				if (bsd->threadGetargs_inuse[index] != GET_STATE_CANCEL) {
					if (proto == 0) {
						SETERRNO;
					} else {
						bsdsocklib_seterrno(ctx, sb, 0);
						memcpy((void*)args->buf, proto, sizeof(struct protoent));
					}
				}

			} else if (args->args1 == 2) {
				uae_u32 nameport;
				uae_u32 proto;
				uae_u32 type;
				char *proto_rp = 0;
				struct servent *serv = NULL;

				nameport = args->args2;
				proto = args->args3;
				type = args->args4;

				if (proto) {
					if (addr_valid(_T("thread_get3"), proto, 1))
						proto_rp = (char*)get_real_address(proto);
				}

				if (type) {
					serv = getservbyport(nameport, proto_rp);
				} else {
					if (addr_valid(_T("thread_get4"), nameport, 1)) {
						name_rp = (char*)get_real_address(nameport);
						serv = getservbyname(name_rp, proto_rp);
					}
				}
				if (bsd->threadGetargs_inuse[index] != GET_STATE_CANCEL) {
					if (serv == 0) {
						SETERRNO;
					} else {
						bsdsocklib_seterrno(ctx, sb, 0);
						memcpy((void*)args->buf, serv, sizeof(struct servent));
					}
				}
			}

			locksigqueue();
			bsd->threadGetargs_inuse[index] = GET_STATE_FINISHED;
			unlocksigqueue();

			SETSIGNAL;

			locksigqueue();
			bsd->threadGetargs_inuse[index] = GET_STATE_DONE;
			unlocksigqueue();

			SetEvent(bsd->hGetEvents2[index]);
		}
	}
	write_log(_T("BSDSOCK: thread_get2 terminated\n"));
	THREADEND(result);
	return result;
}

static unsigned int __stdcall thread_get(void *p)
{
	return thread_get2(p);
}

static int run_get_thread(TrapContext *ctx, SB, struct threadargs *args)
{
	int i;

	sb->eintr = 0;

	locksigqueue();

	for (i = 0; i < MAX_GET_THREADS; i++) {
		if (bsd->threadGetargs_inuse[i] == GET_STATE_REALLY_DONE) {
			bsd->threadGetargs_inuse[i] = GET_STATE_FREE;
		}
		if (bsd->hGetThreads[i] && bsd->threadGetargs_inuse[i] == GET_STATE_FREE) {
			break;
		}
	}

	if (i >= MAX_GET_THREADS) {
		for (i = 0; i < MAX_GET_THREADS; i++) {
			if (bsd->hGetThreads[i] == NULL) {
				bsd->threadGetargs_inuse[i] = GET_STATE_FREE;
				bsd->hGetEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
				bsd->hGetEvents2[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
				if (bsd->hGetEvents[i] && bsd->hGetEvents2[i])
					bsd->hGetThreads[i] = THREAD(thread_get, &threadindextable[i]);
				if (bsd->hGetEvents[i] == NULL || bsd->hGetEvents2[i] == NULL || bsd->hGetThreads[i] == NULL) {
					if (bsd->hGetEvents[i])
						CloseHandle(bsd->hGetEvents[i]);
					bsd->hGetEvents[i] = NULL;
					if (bsd->hGetEvents2[i])
						CloseHandle(bsd->hGetEvents2[i]);
					bsd->hGetEvents2[i] = NULL;
					write_log(_T("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d:%d\n"),
						GetLastError(), args->wscnt);
					bsdsocklib_seterrno(ctx, sb, 12);
					sb->resultval = -1;
					unlocksigqueue();
					return -1;
				}
				bsdsetpriority(bsd->hGetThreads[i]);
				break;
			}
		}
	}

	if (i >= MAX_GET_THREADS) {
		write_log(_T("BSDSOCK: ERROR - Too many gethostbyname()s:%d\n"), args->wscnt);
		bsdsocklib_seterrno(ctx, sb, 12);
		sb->resultval = -1;
		unlocksigqueue();
		return -1;
	} else {
		bsd->threadGetargs[i] = args;
		bsd->threadGetargs_inuse[i] = GET_STATE_ACTIVE;
		ResetEvent(bsd->hGetEvents2[i]);
		SetEvent(bsd->hGetEvents[i]);
	}

	unlocksigqueue();

	while (bsd->threadGetargs_inuse[i] != GET_STATE_FINISHED && bsd->threadGetargs_inuse[i] != GET_STATE_DONE) {
		WAITSIGNAL;
		locksigqueue();
		int inuse = bsd->threadGetargs_inuse[i];
		if (sb->eintr == 1 && inuse != GET_STATE_FINISHED && inuse != GET_STATE_DONE)
			bsd->threadGetargs_inuse[i] = GET_STATE_CANCEL;
		unlocksigqueue();
	}

	if (bsd->threadGetargs_inuse[i] >= GET_STATE_FINISHED)
		WaitForSingleObject(bsd->hGetEvents2[i], INFINITE);

	CANCELSIGNAL;

	return i;
}

static void release_get_thread(int index)
{
	if (index < 0)
		return;
	bsd->threadGetargs_inuse[index] = GET_STATE_REALLY_DONE;
}

void host_gethostbynameaddr(TrapContext *ctx, SB, uae_u32 name, uae_u32 namelen, long addrtype)
{
	HOSTENT *h;
	int size, numaliases = 0, numaddr = 0;
	uae_u32 aptr;
	char *name_rp;
	int i, tindex;
	struct threadargs args;
	volatile struct threadargs *argsp;
	uae_u32 addr;
	uae_u32 *addr_list[2];
	volatile uae_char *buf;
	static int wscounter;

	tindex = -1;
	memset(&args, 0, sizeof(args));
	argsp = &args;
	argsp->wscnt = ++wscounter;
	buf = argsp->buf;

	name_rp = "";

	if (addr_valid(_T("host_gethostbynameaddr"), name, 1))
		name_rp = (char*)get_real_address(name);

	if (addrtype == -1) {
		BSDTRACE((_T("gethostbyname(%s) -> "), name_rp));
		if ((addr = inet_addr(name_rp)) != INADDR_NONE) {
			bsdsocklib_seterrno(ctx, sb, 0);
			((HOSTENT *)buf)->h_name = name_rp;
			((HOSTENT *)buf)->h_aliases = NULL;
			((HOSTENT *)buf)->h_addrtype = AF_INET;
			((HOSTENT *)buf)->h_length = 4;
			((HOSTENT *)buf)->h_addr_list = (char**)&addr_list;
			addr_list[0] = &addr;
			addr_list[1] = NULL;

			goto kludge;
		}
	} else {
		BSDTRACE((_T("gethostbyaddr(0x%x,0x%x,%ld):%d -> "), name, namelen, addrtype, argsp->wscnt));
	}

	argsp->sb = sb;
	argsp->args1 = 0;
	argsp->args2 = name;
	argsp->args3 = namelen;
	argsp->args4 = addrtype;

	tindex = run_get_thread(ctx, sb, &args);
	if (tindex < 0)
		return;
	buf = argsp->buf;

	if (!sb->sb_errno) {
kludge:
		h = (HOSTENT *)buf;

		size = 28;
		if (h->h_name != NULL)
			size += uaestrlen(h->h_name) + 1;

		if (h->h_aliases != NULL)
			while (h->h_aliases[numaliases])
				size += uaestrlen(h->h_aliases[numaliases++]) + 5;

		if (h->h_addr_list != NULL) {
			while (h->h_addr_list[numaddr]) numaddr++;
			size += numaddr * (h->h_length + 4);
		}

		if (sb->hostent) {
			uae_FreeMem(ctx, sb->hostent, sb->hostentsize, sb->sysbase);
		}

		sb->hostent = uae_AllocMem(ctx, size, 0, sb->sysbase);

		if (!sb->hostent) {
			write_log(_T("BSDSOCK: WARNING - gethostby%s() ran out of Amiga memory (couldn't allocate %ld bytes):%d\n"),
				addrtype == -1 ? _T("name") : _T("addr"), size, argsp->wscnt);
			bsdsocklib_seterrno(ctx, sb, 12);
			release_get_thread(tindex);
			return;
		}

		sb->hostentsize = size;

		aptr = sb->hostent + 28 + numaliases * 4 + numaddr * 4;

		put_long(sb->hostent + 4, sb->hostent + 20);
		put_long(sb->hostent + 8, h->h_addrtype);
		put_long(sb->hostent + 12, h->h_length);
		put_long(sb->hostent + 16, sb->hostent + 24 + numaliases * 4);

		for (i = 0; i < numaliases; i++)
			put_long(sb->hostent + 20 + i * 4, addstr_ansi(ctx, &aptr, h->h_aliases[i]));
		put_long(sb->hostent + 20 + numaliases * 4, 0);
		for (i = 0; i < numaddr; i++)
			put_long(sb->hostent + 24 + (numaliases + i) * 4, addmem(ctx, &aptr, h->h_addr_list[i], h->h_length));
		put_long(sb->hostent + 24 + numaliases * 4 + numaddr * 4, 0);
		put_long(sb->hostent, aptr);
		addstr_ansi(ctx, &aptr, h->h_name);

		BSDTRACE((_T("OK (%s):%d\n"), h->h_name, argsp->wscnt));

		bsdsocklib_seterrno(ctx, sb, 0);
		bsdsocklib_setherrno(ctx, sb, 0);

	} else {
		BSDTRACE((_T("failed (%d/%d):%d\n"), sb->sb_errno, sb->sb_herrno, argsp->wscnt));
	}

	release_get_thread(tindex);
}

void host_getprotobyname(TrapContext *ctx, SB, uae_u32 name)
{
	static int wscounter;
	PROTOENT *p;
	int size, numaliases = 0;
	uae_u32 aptr;
	char *name_rp;
	int i, tindex;
	struct threadargs args;
	volatile struct threadargs *argsp;

	memset(&args, 0, sizeof(args));
	argsp = &args;
	argsp->sb = sb;
	argsp->wscnt = ++wscounter;

	name_rp = NULL;
	if (trap_valid_address(ctx, name, 1)) {
		name_rp = trap_get_alloc_string(ctx, name, 256);
	}

	BSDTRACE((_T("getprotobyname(%s):%d -> "), name_rp ? name_rp : "", argsp->wscnt));

	argsp->args1 = 1;
	argsp->args2 = name;

	tindex = run_get_thread(ctx, sb, &args);
	if (tindex < 0)
		return;

	if (!sb->sb_errno) {
		p = (PROTOENT *)argsp->buf;

		size = 16;
		if (p->p_name != NULL)
			size += uaestrlen(p->p_name) + 1;

		if (p->p_aliases != NULL)
			while (p->p_aliases[numaliases])
				size += uaestrlen(p->p_aliases[numaliases++]) + 5;

		if (sb->protoent) {
			uae_FreeMem(ctx, sb->protoent, sb->protoentsize, sb->sysbase);
		}

		sb->protoent = uae_AllocMem(ctx, size, 0, sb->sysbase);

		if (!sb->protoent) {
			write_log(_T("BSDSOCK: WARNING - getprotobyname() ran out of Amiga memory (couldn't allocate %ld bytes):%d\n"),
				size, argsp->wscnt);
			bsdsocklib_seterrno(ctx, sb, 12);
			release_get_thread(tindex);
			return;
		}

		sb->protoentsize = size;

		aptr = sb->protoent + 16 + numaliases * 4;

		trap_put_long(ctx, sb->protoent + 4, sb->protoent + 12);
		trap_put_long(ctx, sb->protoent + 8, p->p_proto);

		for (i = 0; i < numaliases; i++)
			trap_put_long(ctx, sb->protoent + 12 + i * 4, addstr_ansi(ctx, &aptr, p->p_aliases[i]));
		trap_put_long(ctx, sb->protoent + 12 + numaliases * 4, 0);
		trap_put_long(ctx, sb->protoent, aptr);
		addstr_ansi(ctx, &aptr, p->p_name);
		BSDTRACE((_T("OK (%s, %d):%d\n"), p->p_name, p->p_proto, argsp->wscnt));
		bsdsocklib_seterrno(ctx, sb, 0);

	} else {
		BSDTRACE((_T("failed (%d):%d\n"), sb->sb_errno, argsp->wscnt));
	}

	xfree(name_rp);
	release_get_thread(tindex);
}

void host_getprotobynumber(TrapContext *ctx, SB, uae_u32 name)
{
	bsdsocklib_seterrno(ctx, sb, 1);
}

void host_getservbynameport(TrapContext *ctx, SB, uae_u32 nameport, uae_u32 proto, uae_u32 type)
{
	static int wscounter;
	SERVENT *s;
	int size, numaliases = 0;
	uae_u32 aptr;
	int i, tindex;
	struct threadargs args;
	volatile struct threadargs *argsp;

	memset(&args, 0, sizeof(args));
	argsp = &args;
	argsp->sb = sb;
	argsp->wscnt = ++wscounter;

	argsp->args1 = 2;
	argsp->args2 = nameport;
	argsp->args3 = proto;
	argsp->args4 = type;

	tindex = run_get_thread(ctx, sb, &args);
	if (tindex < 0)
		return;

	if (!sb->sb_errno) {
		s = (SERVENT *)argsp->buf;

		size = 20;
		if (s->s_name != NULL)
			size += uaestrlen(s->s_name) + 1;
		if (s->s_proto != NULL)
			size += uaestrlen(s->s_proto) + 1;

		if (s->s_aliases != NULL)
			while (s->s_aliases[numaliases])
				size += uaestrlen(s->s_aliases[numaliases++]) + 5;

		if (sb->servent) {
			uae_FreeMem(ctx, sb->servent, sb->serventsize, sb->sysbase);
		}

		sb->servent = uae_AllocMem(ctx, size, 0, sb->sysbase);

		if (!sb->servent) {
			write_log(_T("BSDSOCK: WARNING - getservby%s() ran out of Amiga memory (couldn't allocate %ld bytes):%d\n"), type ? _T("port") : _T("name"), size, argsp->wscnt);
			bsdsocklib_seterrno(ctx, sb, 12);
			release_get_thread(tindex);
			return;
		}

		sb->serventsize = size;

		aptr = sb->servent + 20 + numaliases * 4;

		trap_put_long(ctx, sb->servent + 4, sb->servent + 16);
		trap_put_long(ctx, sb->servent + 8, (unsigned short)htons(s->s_port));

		for (i = 0; i < numaliases; i++)
			trap_put_long(ctx, sb->servent + 16 + i * 4, addstr_ansi(ctx, &aptr, s->s_aliases[i]));
		trap_put_long(ctx, sb->servent + 16 + numaliases * 4, 0);
		trap_put_long(ctx, sb->servent, aptr);
		addstr_ansi(ctx, &aptr, s->s_name);
		trap_put_long(ctx, sb->servent + 12, aptr);
		addstr_ansi(ctx, &aptr, s->s_proto);

		BSDTRACE((_T("OK (%s, %d):%d\n"), s->s_name, (unsigned short)htons(s->s_port), argsp->wscnt));

		bsdsocklib_seterrno(ctx, sb, 0);

	} else {
		BSDTRACE((_T("failed (%d):%d\n"), sb->sb_errno, argsp->wscnt));
	}

	release_get_thread(tindex);
}

uae_u32 host_gethostname(TrapContext *ctx, uae_u32 name, uae_u32 namelen)
{
	if (!trap_valid_address(ctx, name, namelen))
		return -1;
	uae_char buf[256];
	trap_get_string(ctx, buf, name, sizeof buf);
	return gethostname(buf, namelen);
}

#endif /* BSDSOCKET */

#else /* !_WIN32 */
/* =========================================================================
 * POSIX implementation (Linux, macOS, FreeBSD, etc.) — original Amiberry code
 * ========================================================================= */

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <csignal>
#include <arpa/inet.h>
#include <unistd.h>
#endif /* _WIN32 */
#include <cstddef>
#include <cstring>
#include <vector>
#include <SDL_mutex.h>
#if defined(_WIN32)
#include <mutex>
static std::mutex bsdsock_mutex;
#elif defined(__linux__)
#include <pthread.h>
#elif defined(__APPLE__)
#include <mutex>
static std::mutex bsdsock_mutex;
#else
#include <mutex>
static std::mutex bsdsock_mutex;
#endif

#ifdef _WIN32
/* Winsock compatibility shims */
#define close_socket closesocket
#define SOCK_ERRNO WSAGetLastError()
#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif
static int winsock_initialized = 0;
static void ensure_winsock()
{
	if (!winsock_initialized) {
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		winsock_initialized = 1;
	}
}
/* pipe() replacement using loopback sockets for select()-compatibility */
static int socketpair_pipe(int fds[2])
{
	struct sockaddr_in addr;
	SOCKET listener, client, server;
	int addrlen = sizeof(addr);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == INVALID_SOCKET) return -1;
	if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) { closesocket(listener); return -1; }
	if (listen(listener, 1) < 0) { closesocket(listener); return -1; }
	if (getsockname(listener, (struct sockaddr*)&addr, &addrlen) < 0) { closesocket(listener); return -1; }
	client = socket(AF_INET, SOCK_STREAM, 0);
	if (client == INVALID_SOCKET) { closesocket(listener); return -1; }
	if (connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) { closesocket(client); closesocket(listener); return -1; }
	server = accept(listener, NULL, NULL);
	closesocket(listener);
	if (server == INVALID_SOCKET) { closesocket(client); return -1; }
	fds[0] = (int)server;
	fds[1] = (int)client;
	return 0;
}
#define pipe(fds) socketpair_pipe(fds)
/* On Windows, pipe fds are actually sockets, so use closesocket and send/recv */
#define close_pipe(fd) closesocket(fd)
#define write_pipe(fd, buf, len) send(fd, (const char*)(buf), (int)(len), 0)
#define read_pipe(fd, buf, len) recv(fd, (char*)(buf), (int)(len), 0)

/* POSIX errno values not available on Windows - map to Winsock equivalents */
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN WSAESHUTDOWN
#endif
#ifndef ETOOMANYREFS
#define ETOOMANYREFS WSAETOOMANYREFS
#endif

/* POSIX socket options not available on Windows */
#ifndef IPPROTO_ENCAP
#define IPPROTO_ENCAP 98
#endif
#ifndef IP_RECVOPTS
#define IP_RECVOPTS 6
#endif
#ifndef IP_RECVRETOPTS
#define IP_RECVRETOPTS 7
#endif
#ifndef IP_RETOPTS
#define IP_RETOPTS 8
#endif
#ifndef TCP_MAXSEG
#define TCP_MAXSEG 4
#endif

/* fcntl emulation for Winsock sockets.
   Only F_GETFL and F_SETFL with O_NONBLOCK are meaningfully supported.
   O_ASYNC is ignored (Windows uses WSAAsyncSelect/WSAEventSelect instead). */
#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x0004
#endif
#ifndef O_ASYNC
#define O_ASYNC 0x2000
#endif
static int fcntl(int fd, int cmd, ...)
{
	if (cmd == F_GETFL) {
		return 0; /* Cannot query flags on Windows; assume default */
	} else if (cmd == F_SETFL) {
		va_list ap;
		va_start(ap, cmd);
		int flags = va_arg(ap, int);
		va_end(ap);
		u_long mode = (flags & O_NONBLOCK) ? 1 : 0;
		return ioctlsocket((SOCKET)fd, FIONBIO, &mode);
	}
	return -1;
}
/* ssize_t is not defined by Winsock */
#ifndef _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif
#else
#define close_socket close
#define SOCK_ERRNO errno
#define ensure_winsock() ((void)0)
#define close_pipe(fd) close(fd)
#define write_pipe(fd, buf, len) write(fd, buf, len)
#define read_pipe(fd, buf, len) read(fd, buf, len)
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


#ifdef _WIN32
#define SETERRNO    bsdsocklib_seterrno (ctx, sb, mapErrno (SOCK_ERRNO))
#define SETHERRNO   bsdsocklib_setherrno (ctx, sb, SOCK_ERRNO)
#else
#define SETERRNO    bsdsocklib_seterrno (ctx, sb,mapErrno (errno))
#define SETHERRNO   bsdsocklib_setherrno (ctx, sb, h_errno)
#endif


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
 ** Socket Event Monitoring System
 ** Monitors sockets with SO_EVENTMASK set and posts Amiga signals when events occur
 **/

// Entry for a socket being monitored for events
struct socket_event_entry {
	struct socketbase* sb;
	int sd;                    // Amiga socket descriptor (0-based)
	SOCKET_TYPE s;             // Host socket
	int eventmask;             // REP_* flags to monitor
	bool connecting;           // True if connect() is in progress
	bool connected;            // True if socket is connected (or connectionless/listener)
	int fired_mask;            // Events that have fired and need re-enabling
};

// Event monitor thread state
struct event_monitor {
	uae_thread_id thread;      // Monitor thread
	SDL_mutex* mutex;          // Protects socket_list
	int wake_pipe[2];          // Pipe to wake thread on changes
	std::atomic<bool> running; // Thread running flag
	std::vector<socket_event_entry> socket_list;  // Sockets to monitor
};

static struct event_monitor* g_event_monitor = nullptr;

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

/**
 ** Socket Event Monitoring Functions
 **/

// Post an Amiga signal when a socket event occurs
static void post_socket_event(struct socketbase* sb, int sd, int event_type)
{
	if (!sb || sd < 0) return;
	
	// Set the appropriate SET_* flag in ftable
	sb->ftable[sd] |= (event_type << 8);

	// Signal the Amiga task
	addtosigqueue(sb, 1);
}

static int peek_socket(int s)
{
	char buf[1];
	int res;
	// Peek 1 byte without waiting
	res = recv(s, buf, 1, MSG_PEEK | MSG_DONTWAIT);
	if (res > 0) return 2; // Data available
	if (res == 0) return 1; // EOF (Ready to read 0 bytes)
	return 0; // Error/Spurious/WouldBlock
}

// Event monitor thread - monitors sockets and posts signals
static int event_monitor_thread(void* data)
{
	struct event_monitor* monitor = (struct event_monitor*)data;
	
	write_log("BSDSOCK: Event monitor thread started\n");
	
	while (monitor->running) {
		fd_set readfds, writefds, exceptfds;
		int maxfd = monitor->wake_pipe[0];
		struct timeval timeout;
		
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
		
		// Always monitor wake pipe
		FD_SET(monitor->wake_pipe[0], &readfds);
		
		// Lock mutex to build fd_sets from socket list
		SDL_LockMutex(monitor->mutex);
		
		if (!monitor->socket_list.empty()) {
			BSDTRACE((_T("BSDSOCK: Event monitor checking %d sockets\n"), (int)monitor->socket_list.size()));
		}
		
		for (const auto& entry : monitor->socket_list) {
			if (entry.s == INVALID_SOCKET) continue;
			
			// Skip sockets with no events to monitor
			if (entry.eventmask == 0) continue;
			
			if (entry.s > maxfd) {
				maxfd = entry.s;
			}
			
			// Add socket to appropriate fd_sets based on event mask
			// Filter out events that have already fired (one-shot) for default handling
			int active_mask = entry.eventmask & ~entry.fired_mask;

			// Use active_mask to respect One-Shot behavior (Wait for re-enablement via recv/accept)
			if (active_mask & (REP_READ | REP_ACCEPT)) {
				if (active_mask & REP_READ) {
					// Prevent premature monitoring of READ on connecting/disconnected sockets
					if (!entry.connecting && entry.connected) {
						FD_SET(entry.s, &readfds);
						// write_log("BSDSOCK: Adding socket %d to readfds (mask has REP_READ)\n", entry.sd);
					}
				} else {
					// REP_ACCEPT always monitored (if in active_mask)
					FD_SET(entry.s, &readfds);
					BSDTRACE((_T("BSDSOCK: Adding socket %d to readfds (mask has REP_ACCEPT)\n"), entry.sd));
				}
			}
			
			// REP_WRITE is treated as Level Triggered in select() but Edge Triggered/One-Shot for Amiga signals.
			// If connected and not connecting, we monitor for write if the event is active (not fired).
			// FIX: Also monitor if REP_CONNECT was requested, as implicit Writability expectation.
			if ((active_mask & (REP_WRITE | REP_CONNECT)) && entry.connected && !entry.connecting) {
				FD_SET(entry.s, &writefds);
				// logging noise reduced
			}

			// REP_CONNECT is One-Shot (handled via active_mask).
			// Only monitor if explicitly connecting.
			if ((active_mask & REP_CONNECT) && entry.connecting) {
				FD_SET(entry.s, &writefds);
				BSDTRACE((_T("BSDSOCK: Monitoring socket %d for connect completion (connecting=true)\n"), entry.sd));
			}

			if (active_mask & REP_OOB) {
				FD_SET(entry.s, &exceptfds);
			}
		}
		
		SDL_UnlockMutex(monitor->mutex);
		
		// Wait for events with 1 second timeout
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		
		int result = select(maxfd + 1, &readfds, &writefds, &exceptfds, &timeout);
		
		BSDTRACE((_T("BSDSOCK: select() returned %d\n"), result));
		
		if (result < 0) {
			if (errno == EINTR) continue;
			if (errno == EBADF) {
				BSDTRACE((_T("BSDSOCK: Event monitor select() got EBADF, rebuilding\n")));
				continue;
			}
			write_log("BSDSOCK: Event monitor select() error: %d\n", errno);
			SDL_Delay(100);
			continue;
		}
		
		if (result == 0) {
			// Timeout, just loop again
			continue;
		}
		
		// Check wake pipe
		if (FD_ISSET(monitor->wake_pipe[0], &readfds)) {
			char buf[256];
			read_pipe(monitor->wake_pipe[0], buf, sizeof(buf));
			// Socket list changed, loop again to rebuild fd_sets
			continue;
		}
		
		// Check sockets for events
		SDL_LockMutex(monitor->mutex);
		
		for (auto& entry : monitor->socket_list) {
			if (entry.s == INVALID_SOCKET) continue;
			
			int events = 0;
            // Add slight delay if we are spinning on Level Triggered events to prevent CPU hog
            // Only if we found something? No, loop level.
            // We rely on select() usage. If select returns immediately, we spin.
            // We cannot easily throttle here without affecting response time. 
            // Hoping App clears the signal buffers or handles it.
			
			if (FD_ISSET(entry.s, &readfds)) {
				// Filter "phantom" read events (where select says ready but peek returns error/block)
				if ((entry.eventmask & REP_READ) || (entry.eventmask & REP_CLOSE)) {
					int peek = peek_socket(entry.s);
					if (peek == 2) { // Data
						if ((entry.eventmask & REP_READ) && !(entry.fired_mask & REP_READ)) {
							events |= REP_READ;
						}
					} else if (peek == 1) { // EOF
						if ((entry.eventmask & REP_CLOSE) && !(entry.fired_mask & REP_CLOSE)) {
							events |= REP_CLOSE;
						}
						// EOF is also readable (read returns 0)
						if ((entry.eventmask & REP_READ) && !(entry.fired_mask & REP_READ)) {
							events |= REP_READ;
						}
					}
				}
				if ((entry.eventmask & REP_ACCEPT) && !(entry.fired_mask & REP_ACCEPT)) {
					events |= REP_ACCEPT;
				}
			}
			
			if (FD_ISSET(entry.s, &writefds)) {
				bool wrote = false;
				
				if (entry.connecting) {
					int error = 0;
					socklen_t len = sizeof(error);
					if (getsockopt(entry.s, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0 || error != 0) {
						// Connection failed
						write_log("BSDSOCK: Socket %d connect check failed (errno=%d), checking SO_ERROR\n", entry.sd, errno);
						// We don't set REP_ERROR here, maybe we should? But WinUAE usually handles it via generic error?
						// Actually, we should probably set REP_ERROR if the app asked for it.
						if (entry.eventmask & REP_ERROR) events |= REP_ERROR;
						entry.connecting = false;
					} else {
						entry.connecting = false;
						entry.connected = true;
						
						if ((entry.eventmask & REP_CONNECT) && !(entry.fired_mask & REP_CONNECT)) {
							events |= REP_CONNECT;
						}
						// Writable now
						if ((entry.eventmask & REP_WRITE) && !(entry.fired_mask & REP_WRITE)) {
							events |= REP_WRITE;
						}
                        // Do NOT set REP_READ here blindly. Let readfds handle it.
						BSDTRACE((_T("BSDSOCK: Socket %d CONNECT completed successfully\n"), entry.sd));
					}
					wrote = true;
				}
				
				// Standard Write Signaling
				if (entry.eventmask & REP_WRITE) {
					events |= REP_WRITE;
					wrote = true;
				}
				
				// Fallback: If App asked for REP_CONNECT but NOT REP_WRITE, and we are connected (writable).
				// We must signal REP_CONNECT (or WRITE) to wake it up.
				if (!wrote && (entry.eventmask & REP_CONNECT) && entry.connected) {
					events |= REP_WRITE | REP_CONNECT;
				}
			}
			
			if (FD_ISSET(entry.s, &exceptfds)) {
				if (entry.eventmask & REP_OOB) {
					events |= REP_OOB;
				}
			}
			
			// Post events to Amiga and clear them from the mask (one-shot delivery)
			if (events) {
				post_socket_event(entry.sb, entry.sd, events);
				
				// Mark these events as fired so we don't post them again until re-enabled
				// This implements the one-shot behavior required by Amiga apps
				entry.fired_mask |= events;
				
				// Do NOT clear them from eventmask, as that loses the user's request.
				// Do NOT update ftable here, post_socket_event handles the SET_ flags.
				
				BSDTRACE((_T("BSDSOCK: Fired events 0x%x for socket %d, fired_mask now 0x%x\n"),
				          events, entry.sd, entry.fired_mask));
			}
		}
		
		SDL_UnlockMutex(monitor->mutex);
		
		// Throttle the loop to prevent lock starvation of the Amiga Task
		// If we spin freely, addtosigqueue() locks can starve GetSocketEvents().
		SDL_Delay(10);
	}
	
	write_log("BSDSOCK: Event monitor thread exiting\n");
	return 0;
}

// Start the event monitor thread
static bool start_event_monitor()
{
	if (g_event_monitor) {
		return true; // Already running
	}
	
	g_event_monitor = new event_monitor();
	if (!g_event_monitor) {
		write_log("BSDSOCK: Failed to allocate event monitor\n");
		return false;
	}
	
	// Create wake pipe
	if (pipe(g_event_monitor->wake_pipe) < 0) {
		write_log("BSDSOCK: Failed to create wake pipe: %d\n", errno);
		delete g_event_monitor;
		g_event_monitor = nullptr;
		return false;
	}
	
	// Create mutex
	g_event_monitor->mutex = SDL_CreateMutex();
	if (!g_event_monitor->mutex) {
		write_log("BSDSOCK: Failed to create mutex\n");
		close_pipe(g_event_monitor->wake_pipe[0]);
		close_pipe(g_event_monitor->wake_pipe[1]);
		delete g_event_monitor;
		g_event_monitor = nullptr;
		return false;
	}
	
	// Initialize state
	g_event_monitor->running = true;
	g_event_monitor->socket_list.clear();
	
	// Start thread
	if (!uae_start_thread("bsdsock_event_monitor", event_monitor_thread, g_event_monitor, &g_event_monitor->thread)) {
		write_log("BSDSOCK: Failed to start event monitor thread\n");
		SDL_DestroyMutex(g_event_monitor->mutex);
		close_pipe(g_event_monitor->wake_pipe[0]);
		close_pipe(g_event_monitor->wake_pipe[1]);
		delete g_event_monitor;
		g_event_monitor = nullptr;
		return false;
	}
	
	write_log("BSDSOCK: Event monitor started\n");
	return true;
}

// Stop the event monitor thread
static void stop_event_monitor()
{
	if (!g_event_monitor) {
		return;
	}
	
	write_log("BSDSOCK: Stopping event monitor\n");
	
	// Signal thread to stop
	g_event_monitor->running = false;
	
	// Wake up the thread
	char wake = 1;
	write_pipe(g_event_monitor->wake_pipe[1], &wake, 1);
	
	// Wait for thread to exit
	uae_wait_thread(&g_event_monitor->thread);
	
	// Cleanup
	SDL_DestroyMutex(g_event_monitor->mutex);
	close_pipe(g_event_monitor->wake_pipe[0]);
	close_pipe(g_event_monitor->wake_pipe[1]);
	delete g_event_monitor;
	g_event_monitor = nullptr;
	
	write_log("BSDSOCK: Event monitor stopped\n");
}

// Register a socket for event monitoring
static void register_socket_events(struct socketbase* sb, int sd, SOCKET_TYPE s, int eventmask)
{
	if (!g_event_monitor) {
		if (!start_event_monitor()) {
			write_log("BSDSOCK: Failed to start event monitor for socket %d\n", sd);
			return;
		}
	}
	
	SDL_LockMutex(g_event_monitor->mutex);
	
	// Check if socket already registered
	bool found = false;
	for (auto& entry : g_event_monitor->socket_list) {
		if (entry.sb == sb && entry.sd == sd) {
			// Update existing entry
			entry.eventmask = eventmask;
			found = true;
			BSDTRACE((_T("BSDSOCK: Updated event mask 0x%x for socket %d\n"), eventmask, sd));
			break;
		}
	}
	
	if (!found) {
		// Add new entry
		socket_event_entry entry;
		entry.sb = sb;
		entry.sd = sd;
		entry.s = s;
		entry.sd = sd;
		entry.s = s;
		entry.eventmask = eventmask;
		entry.connecting = false;
		entry.connected = true; // Default to true (optimistic), disable if ENOTCONN seen
		entry.fired_mask = 0;
		g_event_monitor->socket_list.push_back(entry);
		
		BSDTRACE((_T("BSDSOCK: Registered socket %d (native %d) for event monitoring (mask 0x%x)\n"), sd, s, eventmask));
	}
	
	// Wake up monitor thread to rebuild fd_sets
	char wake = 1;
	write_pipe(g_event_monitor->wake_pipe[1], &wake, 1);
	
	SDL_UnlockMutex(g_event_monitor->mutex);
}

// Unregister a socket from event monitoring
static void unregister_socket_events(struct socketbase* sb, int sd)
{
	if (!g_event_monitor) {
		return;
	}
	
	SDL_LockMutex(g_event_monitor->mutex);
	
	// Remove socket from list
	auto it = g_event_monitor->socket_list.begin();
	while (it != g_event_monitor->socket_list.end()) {
		if (it->sb == sb && it->sd == sd) {
			it = g_event_monitor->socket_list.erase(it);
			BSDTRACE((_T("BSDSOCK: Unregistered socket %d from event monitoring\n"), sd));
		} else {
			++it;
		}
	}
	
	// Wake up monitor thread
	char wake = 1;
	write_pipe(g_event_monitor->wake_pipe[1], &wake, 1);
	
	SDL_UnlockMutex(g_event_monitor->mutex);
}

// Unregister all sockets for a socketbase (called during cleanup)
static void unregister_all_socket_events(struct socketbase* sb)
{
	if (!g_event_monitor) {
		return;
	}

	SDL_LockMutex(g_event_monitor->mutex);

	bool removed = false;
	auto it = g_event_monitor->socket_list.begin();
	while (it != g_event_monitor->socket_list.end()) {
		if (it->sb == sb) {
			it = g_event_monitor->socket_list.erase(it);
			removed = true;
		} else {
			++it;
		}
	}

	if (removed) {
		char wake = 1;
		write_pipe(g_event_monitor->wake_pipe[1], &wake, 1);
	}

	SDL_UnlockMutex(g_event_monitor->mutex);
}

// Set the connecting state for a socket
static void set_socket_connecting(struct socketbase* sb, int sd, bool connecting)
{
	if (!g_event_monitor) return;
	
	SDL_LockMutex(g_event_monitor->mutex);
	for (auto& entry : g_event_monitor->socket_list) {
		if (entry.sb == sb && entry.sd == sd) {
			entry.connecting = connecting;
			BSDTRACE((_T("BSDSOCK: Socket %d connecting state set to %d\n"), sd, connecting));
			break;
		}
	}
	// Wake up monitor to update handling
	if (g_event_monitor->wake_pipe[1] != -1) {
		char b = 1;
		write_pipe(g_event_monitor->wake_pipe[1], &b, 1);
	}
	SDL_UnlockMutex(g_event_monitor->mutex);
}

// Re-enable specific events for a socket (called by IO functions)
static void socket_reenable_events(struct socketbase* sb, int sd, int events)
{
	if (!g_event_monitor) return;
	
	SDL_LockMutex(g_event_monitor->mutex);
	for (auto& entry : g_event_monitor->socket_list) {
		if (entry.sb == sb && entry.sd == sd) {
			if (entry.fired_mask & events) {
				entry.fired_mask &= ~events;
				BSDTRACE((_T("BSDSOCK: Re-enabled events 0x%x for socket %d\n"), events, sd));
				// Wake up monitor to check this socket again
				if (g_event_monitor->wake_pipe[1] != -1) {
					char b = 1;
					write_pipe(g_event_monitor->wake_pipe[1], &b, 1);
				}
			}
			break;
		}
	}
	SDL_UnlockMutex(g_event_monitor->mutex);
}

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
    getsockopt(sb->s, SOL_SOCKET, SO_TYPE, (char*)&socktype, &optlen);
    int retries = (socktype == SOCK_RAW) ? 5 : 1;
    if (sb->from == 0) {
        ssize_t n;
        do {
            if (sb->s != -1 && socktype == SOCK_RAW) {
                write_log("[RAW RECV] fd=%d, buf=%p, len=%d, flags=0x%x\n", sb->s, sb->buf, sb->len, sb->flags);
            }
            n = recv(sb->s, (char*)sb->buf, sb->len, sb->flags /*| MSG_NOSIGNAL*/);
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
            n = recvfrom(sb->s, (char*)sb->buf, sb->len, sb->flags | MSG_NOSIGNAL, (struct sockaddr *)&addr, &l);
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
            getsockopt(sb->s, SOL_SOCKET, SO_TYPE, (char*)&socktype, &optlen);
            if (socktype == SOCK_RAW) {
                write_log("[RAW SEND] fd=%d, buf=%p, len=%d, flags=0x%x\n", sb->s, sb->buf, sb->len, sb->flags);
            }
        }
        n = send (sb->s, (const char*)sb->buf, sb->len, sb->flags | MSG_NOSIGNAL);
        return (int)n;
    } else {
        struct sockaddr_in addr{};
        int l = sizeof (struct sockaddr_in);
        ssize_t n;
        copysockaddr_a2n (&addr, sb->to, sb->tolen);
        if (sb->s != -1) {
            int socktype = 0;
            socklen_t optlen = sizeof(socktype);
            getsockopt(sb->s, SOL_SOCKET, SO_TYPE, (char*)&socktype, &optlen);
            if (socktype == SOCK_RAW) {
                write_log("[RAW SENDTO] fd=%d, buf=%p, len=%d, flags=0x%x\n", sb->s, sb->buf, sb->len, sb->flags);
            }
        }
        n = sendto (sb->s, (const char*)sb->buf, sb->len, sb->flags | MSG_NOSIGNAL, (struct sockaddr *)&addr, l);
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
		if (getsockopt (sb->s, SOL_SOCKET, SO_ERROR, (char*)&foo, &bar) == 0) {
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
#ifdef _WIN32
    flags = 0;
#else
    if ((flags = fcntl(sb->s, F_GETFL)) == -1)
        flags = 0;
#endif
    // Check if this is a raw socket
    if (getsockopt(sb->s, SOL_SOCKET, SO_TYPE, (char*)&socktype, &optlen) == 0 && socktype == SOCK_RAW) {
        is_raw = 1;
        // Save original timeout
        if (getsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, (char*)&orig_timeout, &tvlen) == 0) {
            timeout_set = 1;
        }
        // Set a 1 second timeout for raw sockets
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    }
#ifdef _WIN32
    nonblock = 0; /* Cannot query nonblock state on Windows; assume blocking */
#else
    nonblock = (flags & O_NONBLOCK);
#endif
    // Only set non-blocking for non-raw sockets
    if (!is_raw) {
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(sb->s, FIONBIO, &mode);
#else
        fcntl(sb->s, F_SETFL, flags | O_NONBLOCK);
#endif
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
#ifdef _WIN32
                    if (!is_raw) { u_long mode = 0; ioctlsocket(sb->s, FIONBIO, &mode); }
#else
                    if (!is_raw) fcntl(sb->s, F_SETFL, flags);
#endif
                    if (is_raw && timeout_set) setsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&orig_timeout, sizeof(orig_timeout));
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
#ifdef _WIN32
    if (!is_raw) { u_long mode = 0; ioctlsocket(sb->s, FIONBIO, &mode); }
#else
    if (!is_raw) fcntl(sb->s, F_SETFL, flags);
#endif
    if (is_raw && timeout_set) setsockopt(sb->s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&orig_timeout, sizeof(orig_timeout));
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
			if ((int)sb->resultval < 0) {
				SETERRNO;
			}
			break;

			/* @@@ Should check (from|to)len so it's 16.. */
		case 2:       /* Send[to] */
			sb->resultval = bsdthr_SendRecvAcceptConnect(bsdthr_Send_2, sb);
			if ((int)sb->resultval < 0) {
				SETERRNO;
			}
			break;

		case 3:       /* Recv[from] */
			sb->resultval = bsdthr_SendRecvAcceptConnect(bsdthr_Recv_2, sb);
			if ((int)sb->resultval < 0) {
				SETERRNO;
			}
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
			if ((int)sb->resultval < 0) {
				SETERRNO;
			}
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
			struct hostent* tmphostent = gethostbyaddr((const char*)get_real_address(sb->name), sb->a_addrlen, sb->flags);
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

	while ((num = read_pipe(sb->sockabort[0], &chr, sizeof(chr))) >= 0) {
		write_log("Sockabort got %d bytes\n", num);
	}
}

int init_socket_layer(void)
{
	int result = 0;
	ensure_winsock();

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

#ifdef _WIN32
	{
		u_long mode = 1;
		ioctlsocket(sb->sockabort[0], FIONBIO, &mode);
	}
#else
	if (fcntl (sb->sockabort[0], F_SETFL, O_NONBLOCK) < 0) {
		write_log ("Set nonblock failed %d\n", errno);
	}
#endif

	if (uae_sem_init (&sb->sem, 0, 0)) {
		write_log ("BSDSOCK: Failed to create semaphore.\n");
		close_pipe (sb->sockabort[0]);
		close_pipe (sb->sockabort[1]);
		return 0;
	}

	/* Alloc hostent buffer */
	sb->hostent = uae_AllocMem (ctx, 1024, 0, sb->sysbase);
	sb->hostentsize = 1024;

	/* @@@ The thread should be PTHREAD_CREATE_DETACHED */
	if (uae_start_thread ("bsdsocket", bsdlib_threadfunc, (void *)sb, &sb->thread) == BAD_THREAD) {
		write_log ("BSDSOCK: Failed to create thread.\n");
		uae_sem_destroy (&sb->sem);
		close_pipe (sb->sockabort[0]);
		close_pipe (sb->sockabort[1]);
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
		setsockopt (s, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
		close_socket (s);
	}
}

void host_sbcleanup (SB)
{
	int i;

	if (!sb) {
		return;
	}

	unregister_all_socket_events(sb);

	uae_thread_id thread = sb->thread;
	close_pipe (sb->sockabort[0]);
	close_pipe (sb->sockabort[1]);
	for (i = 0; i < sb->dtablesize; i++) {
		if (sb->dtable[i] != -1) {
			close_socket(sb->dtable[i]);
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
	stop_event_monitor();
}

void sockabort (SB)
{
	int chr = 1;
	write_log ("Sock abort!!\n");
	if (write_pipe (sb->sockabort[1], &chr, sizeof (chr)) != sizeof (chr)) {
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
				unregister_socket_events(sb, fd2 - 1);
				close_socket (s2);
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
        setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (const char*)&arg, sizeof(arg));
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
	
	// Implicitly re-enable REP_ACCEPT
	if (sb->resultval >= 0) {
		socket_reenable_events(sb, sd, REP_ACCEPT);
	}
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
			
			// Notify event monitor that connect is in progress
			// Note: sd was incremented at start of function, so we must pass sd-1 
			// to match the 0-based descriptor used in registration
			set_socket_connecting(sb, sd - 1, true);

			uae_sem_post (&sb->sem);

			WAITSIGNAL;

			// Implicitly			// Re-enable REP_CONNECT (and REP_WRITE as they are related on success)
			socket_reenable_events(sb, sd - 1, REP_CONNECT | REP_WRITE);
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
    
    write_log("BSDSOCK: host_sendto %d called\n", sd);

	uae_sem_post (&sb->sem);

	WAITSIGNAL;

	// Implicitly re-enable REP_WRITE
	socket_reenable_events(sb, sd - 1, REP_WRITE);

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
    
    write_log("BSDSOCK: host_recvfrom %d called\n", sd);

	uae_sem_post (&sb->sem);

	WAITSIGNAL;

	// Implicitly re-enable REP_READ and REP_OOB
	socket_reenable_events(sb, sd, REP_READ | REP_OOB);
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
	void* buf = NULL;
	struct linger sl;
	struct timeval timeout;

	if (s == INVALID_SOCKET) {
		sb->resultval = -1;
		bsdsocklib_seterrno(ctx, sb, 9); /* EBADF */
		return;
	}

	// Handle SO_EVENTMASK (0x2001) - Amiga-specific async event notification
	// Must be checked BEFORE mapsockoptname validation since it's not in the mapping table
	// Level 0xFFFF is Amiga's SOL_SOCKET value
	if (level == 0xFFFF && optname == 0x2001) {
		uae_u32 eventflags = 0;
		if (optval && len >= 4) {
			eventflags = get_long(optval);
		}
		
		// Fix for dynAMIte and other apps that rely on implicit Writability after Connect:
		if (eventflags & REP_CONNECT) {
			eventflags |= REP_WRITE;
			write_log("BSDSOCK: Force-enabled REP_WRITE for socket %d (requested mask 0x%x -> 0x%x)\n", sd, get_long(optval), eventflags);
		}
		
		BSDTRACE((_T("BSDSOCK: SO_EVENTMASK called for socket %d, eventflags=0x%x\n"), sd, eventflags));
		
		// Store event mask in ftable (using lower bits)
		sb->ftable[sd] = (sb->ftable[sd] & ~REP_ALL) | (eventflags & REP_ALL);
		
		// Register or unregister with event monitor
		if (eventflags & REP_ALL) {
			// Register socket for event monitoring
			register_socket_events(sb, sd, s, eventflags & REP_ALL);
		} else {
			// Unregister socket from event monitoring
			unregister_socket_events(sb, sd);
		}
		
		sb->resultval = 0;
		return;
	}

	// Now map the level and option name
	int nativelevel = mapsockoptlevel(level);
	int nativeoptname = mapsockoptname(nativelevel, optname);

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
		sb->resultval = setsockopt(s, nativelevel, nativeoptname, (const char*)&timeout, sizeof(timeout));
	}
	else if (nativeoptname == SO_LINGER) {
		sb->resultval = setsockopt(s, nativelevel, nativeoptname, (const char*)&sl, sizeof(sl));
	}
	else {
		sb->resultval = setsockopt(s, nativelevel, nativeoptname, (const char*)buf, len);
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

	// Handle SO_EVENTMASK (0x2001) - Amiga-specific, no host equivalent
	if (level == 0xFFFF && optname == 0x2001) {
		if (optval && optlen) {
			int mask = sb->ftable[sd] & REP_ALL;
			put_long(optval, mask);
			put_long(optlen, 4);
		}
		bsdsocklib_seterrno(ctx, sb, 0);
		sb->resultval = 0;
		return 0;
	}

	if (optlen) {
		len = get_long(optlen);
		buf = malloc(len);
		if (buf == NULL) {
			return -1;
		}
	}

	if (nativeoptname == SO_RCVTIMEO || nativeoptname == SO_SNDTIMEO) {
		r = getsockopt(s, nativelevel, nativeoptname, (char*)&timeout, &len);
	}
	else if (nativeoptname == SO_LINGER) {
		r = getsockopt(s, nativelevel, nativeoptname, (char*)&sl, &len);
	}
	else {
		r = getsockopt(s, nativelevel, nativeoptname, optval ? (char*)buf : NULL, optlen ? &len : NULL);
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

#ifdef _WIN32
	flags = 0; /* fcntl F_GETFL not available on Windows */
#else
	if ((flags = fcntl (sock, F_GETFL)) == -1) {
		SETERRNO;
		return -1;
	}
#endif

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
#ifdef _WIN32
		/* O_ASYNC / SIGIO not available on Windows */
		return 0;
#elif defined(O_ASYNC)
		r = fcntl (sock, F_SETFL, argval ? flags | O_ASYNC : flags & ~O_ASYNC);
		return r;
#   else
		/* O_ASYNC is only available on Linux and BSD systems */
		return fcntl (sock, F_GETFL);
#   endif

	case 0x8004667E: /* FIONBIO */
	{
#ifdef _WIN32
		u_long mode = argval ? 1 : 0;
		r = ioctlsocket(sock, FIONBIO, &mode);
#else
		r = fcntl (sock, F_SETFL, argval ?
			   flags | O_NONBLOCK : flags & ~O_NONBLOCK);
#endif
		if (argval) {
			write_log("nonblocking\n");
			sb->ftable[sd-1] &= ~SF_BLOCKING;
		} else {
			write_log("blocking\n");
			sb->ftable[sd-1] |= SF_BLOCKING;
		}
		return r;
	}

	case 0x4004667F: /* FIONREAD */
	{
#ifdef _WIN32
		u_long nbytes = 0;
		r = ioctlsocket(sock, FIONREAD, &nbytes);
#else
		int nbytes = 0;
		r = ioctl(sock, FIONREAD, &nbytes);
#endif

		if (r >= 0) {
			put_long (arg, nbytes);
			return 0;
		}
		break;
	}

#ifndef _WIN32
	// Interface discovery IOCTLs - not available on Windows (no ifreq/ifconf)
	case 0x80106921: /* SIOCGIFADDR */
	case 0x80106923: /* SIOCGIFDSTADDR */
	case 0x80106925: /* SIOCGIFBRDADDR */
	case 0x80106927: /* SIOCGIFNETMASK */
	case 0xc0206911: /* SIOCGIFFLAGS */
	{
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		// Read interface name from Amiga memory
		for (int i = 0; i < IFNAMSIZ - 1; i++) {
			ifr.ifr_name[i] = trap_get_byte(ctx, arg + i);
			if (ifr.ifr_name[i] == 0) break;
		}
		ifr.ifr_name[IFNAMSIZ - 1] = 0;
		r = ioctl(sock, request, &ifr);
		if (r >= 0) {
			// Write result back
			if (request == 0xc0206911) { // SIOCGIFFLAGS
				trap_put_word(ctx, arg + IFNAMSIZ, ifr.ifr_flags);
			} else { // Address IOCTLs
				copysockaddr_n2a(arg + IFNAMSIZ, (struct sockaddr_in*)&ifr.ifr_addr, 16);
			}
		}
		return r;
	}

	case 0xc0086924: /* SIOCGIFCONF */
	{
		struct ifconf ifc;
		char buf[1024];
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = buf;
		r = ioctl(sock, SIOCGIFCONF, &ifc);
		if (r >= 0) {
			// Write back the interface list
			trap_put_long(ctx, arg, ifc.ifc_len);
			uae_u32 bufptr = trap_get_long(ctx, arg + 4);
			if (bufptr) {
				for (int i = 0; i < ifc.ifc_len && i < (int)sizeof(buf);) {
					struct ifreq *ifr = (struct ifreq*)(buf + i);
					// Write interface name
					for (int j = 0; j < IFNAMSIZ; j++) {
						trap_put_byte(ctx, bufptr + i + j, ifr->ifr_name[j]);
					}
					// Write address
					copysockaddr_n2a(bufptr + i + IFNAMSIZ, (struct sockaddr_in*)&ifr->ifr_addr, 16);
					i += sizeof(struct ifreq);
				}
			}
		}
		return r;
	}
#endif /* !_WIN32 */

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
	
	// Unregister from event monitoring if registered
	unregister_socket_events(sb, sd);
	
	retval = close_socket (s);
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

	if (nfds == 0 && wssigs == 0 && timeout == 0) {
		/* Nothing to wait for — no sockets, no signals, no timeout */
		if (readfds)
			fd_zero(ctx, readfds, nfds);
		if (writefds)
			fd_zero(ctx, writefds, nfds);
		if (exceptfds)
			fd_zero(ctx, exceptfds, nfds);
		sb->resultval = 0;
		bsdsocklib_seterrno(ctx, sb, 0);
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
			sb->resultval = 0;
		}
		/*
		if (readfds)
			fd_zero(ctx, readfds, nfds);
		if (writefds)
			fd_zero(ctx, writefds, nfds);
		if (exceptfds)
			fd_zero(ctx, exceptfds, nfds);
		*/

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

#endif /* _WIN32 */
