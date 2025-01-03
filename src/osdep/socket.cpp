#include "sysconfig.h"
#include "sysdeps.h"

#include "uae/socket.h"

#ifdef _WIN32
#include <Ws2tcpip.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#endif

#ifndef _WIN32
#define SOCKADDR_INET sockaddr_storage
#endif

#if SIZEOF_TCHAR == 1
#define ADDRINFOW struct addrinfo
#define PADDRINFOW struct addrinfo *
#define GetAddrInfoW getaddrinfo
#define FreeAddrInfoW freeaddrinfo
#endif

bool uae_socket_init()
{
	static bool initialized = false;
	static bool result = true;
	if (initialized) {
		return result;
	}
#ifdef _WIN32
	static WSADATA wsadata;
	if (WSAStartup(MAKEWORD (2, 2), &wsadata)) {
		write_log (_T("WSAStartup failed (error %d)\n"), WSAGetLastError());
		result = false;
	}
#endif
	initialized = true;
	return result;
}

int uae_socket_error()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

uae_socket uae_tcp_listen(const TCHAR *host, const TCHAR *port, int flags)
{
	uae_socket s = UAE_SOCKET_INVALID;

	if (!uae_socket_init()) {
		write_log(_T("TCP: Can't open %s port %s\n"), host, port);
		return UAE_SOCKET_INVALID;
	}

	write_log(_T("TCP: Open %s port %s\n"), host, port);

	ADDRINFOW hints{};
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	PADDRINFOW socketinfo;
	int err = GetAddrInfoW(host, port, &hints, &socketinfo);
	if (err < 0) {
		write_log(_T("TCP: getaddrinfo failed, %s:%s: %d\n"),
		          host, port, uae_socket_error());
		goto fail;
	}

	s = socket(socketinfo->ai_family, socketinfo->ai_socktype,
	           socketinfo->ai_protocol);
	if (s == UAE_SOCKET_INVALID) {
		write_log(_T("TCP: socket() failed, %s:%s: %d\n"),
		          host, port, uae_socket_error());
		goto fail;
	}

	if (flags & UAE_SOCKET_LINGER) {
		constexpr struct linger l = { 1, 1 };
		err = setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &l, sizeof(l));
		if (err < 0) {
			write_log(_T("TCP: setsockopt(SO_LINGER) failed, %s:%s: %d\n"),
			          host, port, uae_socket_error());
			goto fail;
		}
	}
	if (flags & UAE_SOCKET_REUSEADDR) {
		constexpr int o = 1;
		err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &o, sizeof(o));
		if (err < 0) {
			write_log(_T("TCP: setsockopt(SO_REUSEADDR) failed, %s:%s: %d\n"),
			          host, port, uae_socket_error());
			goto fail;
		}
	}

	err = ::bind(s, socketinfo->ai_addr, socketinfo->ai_addrlen);
	if (err < 0) {
		write_log(_T("TCP: bind() failed, %s:%s: %d\n"),
		          host, port, uae_socket_error());
		goto fail;
	}
	err = listen(s, 1);
	if (err < 0) {
		write_log(_T("TCP: listen() failed, %s:%s: %d\n"),
		          host, port, uae_socket_error());
		goto fail;
	}

	write_log(_T("TCP: Listening on %s port %s\n"), host, port);
	return s;
	fail:
	if (s != UAE_SOCKET_INVALID) {
		uae_socket_close(s);
	}
	write_log(_T("TCP: Failed to open %s port %s\n"), host, port);
	return UAE_SOCKET_INVALID;
}

uae_socket uae_tcp_listen_uri(
		const TCHAR *uri, const TCHAR *default_port, int flags)
{
	if (_tcsnicmp(uri, _T("tcp://"), 6) == 0) {
		uri += 6;
	} else if (_tcsnicmp(uri, _T("tcp:"), 4) == 0) {
		uri += 4;
	} else {
		return UAE_SOCKET_INVALID;
	}
	write_log(_T("TCP: Listen %s\n"), uri);

	TCHAR *name = _tcsdup(uri);
	TCHAR *port = nullptr;
	const TCHAR *p = _tcschr(uri, ':');
	if (p) {
		name[p - uri] = 0;
		port = my_strdup(p + 1);
		const TCHAR *p2 = _tcschr(port, '/');
		if (p2) {
			port[p2 - port] = 0;
		}
	}
	if (port && port[0] == 0) {
		xfree(port);
		port = nullptr;
	}
	if (!port) {
		port = _tcsdup(default_port);
	}
	uae_socket s = uae_tcp_listen(name, port, flags);
	xfree(name);
	xfree(port);
	return s;
}

uae_socket uae_socket_accept(uae_socket s)
{
	socklen_t sa_len = sizeof(SOCKADDR_INET);
	char socketaddr[sizeof(SOCKADDR_INET)];
	uae_socket result = accept(s, reinterpret_cast<struct sockaddr*>(socketaddr), &sa_len);
	return result;
}

int uae_socket_read(uae_socket s, void *buf, int count)
{
	return static_cast<int>(recv(s, (char*)buf, count, 0));
}

int uae_socket_write(uae_socket s, const void *buf, int count)
{
	return static_cast<int>(send(s, (const char*)buf, count, 0));
}

int uae_socket_select(
		uae_socket s, bool read, bool write, bool except, uae_u64 timeout)
{
	struct timeval tv{};
	tv.tv_sec = timeout / 1000000;
	tv.tv_usec = timeout % 1000000;
	fd_set readfds, writefds, exceptfds;
//#ifdef _WIN32
#if 0
	readfds.fd_array[0] = s;
	readfds.fd_count = 1;
	writefds.fd_array[0] = s;
	writefds.fd_count = 1;
	exceptfds.fd_array[0] = s;
	exceptfds.fd_count = 1;
#else
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	FD_ZERO(&writefds);
	FD_SET(s, &writefds);
	FD_ZERO(&exceptfds);
	FD_SET(s, &exceptfds);
#endif

	int num = select(
			s + 1, read ? &readfds : nullptr, write ? &writefds : nullptr,
			except ? &exceptfds : nullptr, &tv);
	if (num == 0) {
		// write_log("TCP: select %d result 0\n", s);
		return 0;
	} else if (num < 0) {
		// write_log("TCP: select %d error\n", s);
		return UAE_SELECT_ERROR;
	}
	int result = 0;
	if (FD_ISSET(s, &readfds)) {
		result |= UAE_SELECT_READ;
	}
	if (FD_ISSET(s, &writefds)) {
		result |= UAE_SELECT_WRITE;
	}
	if (FD_ISSET(s, &exceptfds)) {
		result |= UAE_SELECT_EXCEPT;
	}
	// write_log("TCP: select %d result %d\n", s, result);
	return result;
}

bool uae_socket_close(uae_socket s)
{
#ifdef _WIN32
	return closesocket(s) == 0;
#else
	return close(s) == 0;
#endif
}

const TCHAR *uae_uri_path(const TCHAR *p)
{
	p = _tcschr(p, ':');
	if (p) {
		p = _tcschr(p + 1, ':');
	}
	if (p) {
		p = _tcschr(p + 1, '/');
	}
	if (p) {
		return p;
	}
	return "";
}
