/* WinUAE loopback GDB server. No worker thread accesses emulated state. */
#include "sysconfig.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif
#include "sysdeps.h"
#include "options.h"
#include "gdb_server.h"
#include "gdb_protocol.h"
#include "uae.h"
#include "xwin.h"
#include "sounddep/sound.h"
#include <memory>

#ifdef DEBUGGER
namespace {
#ifdef _WIN32
using Socket = SOCKET;
const Socket invalid_socket = INVALID_SOCKET;
void close_socket(Socket s) { closesocket(s); }
bool would_block() { return WSAGetLastError() == WSAEWOULDBLOCK; }
bool nonblocking(Socket s) { u_long one = 1; return ioctlsocket(s, FIONBIO, &one) == 0; }
#else
using Socket = int;
const Socket invalid_socket = -1;
void close_socket(Socket s) { close(s); }
bool would_block() { return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR; }
bool nonblocking(Socket s)
{
	int flags = fcntl(s, F_GETFL, 0);
	return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
}
#endif
Socket listener = invalid_socket, client = invalid_socket;
std::unique_ptr<winuae_gdb::Session> session;
std::string pending;
bool polling, sockets_started;
int configured_port;

void disconnect_client()
{
	if (client != invalid_socket) close_socket(client);
	client = invalid_socket;
	if (session) debug_gdb_target().detach();
	session.reset();
	pending.clear();
}

void listen_local(int port)
{
#ifdef _WIN32
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data)) return;
#endif
	sockets_started = true;
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == invalid_socket) return;
	int one = 1;
#ifdef _WIN32
	setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&one), sizeof(one));
#else
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#endif
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(static_cast<unsigned short>(port));
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (!nonblocking(listener) || bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) || listen(listener, 1)) {
		close_socket(listener);
		listener = invalid_socket;
		write_log(_T("WinUAE GDB: cannot listen on 127.0.0.1:%d\n"), port);
		return;
	}
	write_log(_T("WinUAE GDB: listening on 127.0.0.1:%d\n"), port);
}
}

bool gdb_server_connected() { return session != nullptr; }

void gdb_server_close()
{
	disconnect_client();
	if (listener != invalid_socket) close_socket(listener);
	listener = invalid_socket;
#ifdef _WIN32
	if (sockets_started) WSACleanup();
#endif
	sockets_started = false;
	configured_port = 0;
}

void gdb_server_poll()
{
	if (polling) return;
	int port = (currprefs.debugging_features & DEBUG_FEATURE_GDBSERVER) ? currprefs.gdb_port : 0;
	if (quit_program) port = 0;
	if (port != configured_port) {
		gdb_server_close();
		configured_port = port;
		if (port > 0 && port <= 65535) listen_local(port);
	}
	if (listener == invalid_socket) return;
	polling = true;
	if (client == invalid_socket) {
		client = accept(listener, nullptr, nullptr);
		if (client != invalid_socket) {
			if (!nonblocking(client)) { close_socket(client); client = invalid_socket; }
			else {
#ifdef SO_NOSIGPIPE
				int one = 1;
				setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
				session.reset(new winuae_gdb::Session(debug_gdb_target()));
				debug_gdb_target().interrupt();
			}
		}
	}
	if (session) {
		char buffer[4096];
		// Limit work per event pump so a busy client cannot starve the UI.
		for (int i = 0; i < 16 && session && !session->finished(); ++i) {
			int count = static_cast<int>(recv(client, buffer, sizeof(buffer), 0));
			if (count > 0) session->receive(buffer, count);
			else {
				if (!count || !would_block()) disconnect_client();
				break;
			}
		}
	}
	if (session) {
		pending += session->take_output();
		if (pending.size() > winuae_gdb::Session::packet_size * 4) disconnect_client();
	}
	if (session && !pending.empty()) {
		int flags = 0;
#ifdef MSG_NOSIGNAL
		flags = MSG_NOSIGNAL;
#endif
		int count = static_cast<int>(send(client, pending.data(), static_cast<int>(pending.size()), flags));
		if (count > 0) pending.erase(0, count);
		else if (!count || !would_block()) disconnect_client();
	}
	if (session && session->finished() && pending.empty()) disconnect_client();
	polling = false;
}

bool gdb_server_stop(const char* reason)
{
	if (!session) return false;
	session->stop(reason);
	pause_sound();
	while (session && session->stopped() && !quit_program) {
		gdb_server_poll();
		handle_msgpump(false);
		sleep_millis(1);
	}
	resume_sound();
	return true;
}
#endif
