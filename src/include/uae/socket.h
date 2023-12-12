#ifndef UAE_SOCKET_H
#define UAE_SOCKET_H

#include "uae/types.h"
#ifdef _WIN32
#include "Winsock2.h"
#endif

#ifdef _WIN32
typedef SOCKET uae_socket;
#define UAE_SOCKET_INVALID INVALID_SOCKET
#else
typedef int uae_socket;
#define UAE_SOCKET_INVALID -1
#endif

bool uae_socket_init(void);
uae_socket uae_socket_accept(uae_socket s);
int uae_socket_read(uae_socket s, void *buf, int count);
int uae_socket_write(uae_socket s, const void *buf, int count);
int uae_socket_select(
        uae_socket s, bool read, bool write, bool except, uae_u64 timeout);
bool uae_socket_close(uae_socket s);
int uae_socket_error(void);

uae_socket uae_tcp_listen(const TCHAR *host, const TCHAR *port, int flags);
uae_socket uae_tcp_listen_uri(
        const TCHAR *uri, const TCHAR *default_port, int flags);

const TCHAR *uae_uri_path(const TCHAR *p);

#define UAE_SOCKET_LINGER 1
#define UAE_SOCKET_REUSEADDR 2
#define UAE_SOCKET_DEFAULT (UAE_SOCKET_LINGER | UAE_SOCKET_REUSEADDR)

#define UAE_SELECT_READ 1
#define UAE_SELECT_WRITE 2
#define UAE_SELECT_EXCEPT 4
#define UAE_SELECT_ERROR 8

static inline int uae_socket_select_read(uae_socket s)
{
    return uae_socket_select(s, true, false, false, 0);
}

#endif /* UAE_SOCKET_H */
