// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, Vas Crabb
//============================================================
//
//  winsocket.cpp - Windows socket (TCP) access functions
//
//  Adapted from the POSIX socket implementation for Winsock2.
//  Unix domain sockets are not supported on Windows.
//
//============================================================

#include "winfile.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <memory>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>


namespace {

	char const* const winfile_socket_identifier = "socket.";

	// Helper: map WSA errors to std::error_condition
	std::error_condition wsa_error_to_error_condition(int wsaerr) noexcept
	{
		switch (wsaerr)
		{
		case WSAEWOULDBLOCK:    return std::errc::operation_would_block;
		case WSAEINPROGRESS:    return std::errc::operation_in_progress;
		case WSAECONNREFUSED:   return std::errc::connection_refused;
		case WSAECONNRESET:     return std::errc::connection_reset;
		case WSAECONNABORTED:   return std::errc::connection_aborted;
		case WSAETIMEDOUT:      return std::errc::timed_out;
		case WSAENETUNREACH:    return std::errc::network_unreachable;
		case WSAEHOSTUNREACH:   return std::errc::host_unreachable;
		case WSAEADDRINUSE:     return std::errc::address_in_use;
		case WSAEADDRNOTAVAIL:  return std::errc::address_not_available;
		case WSAEINVAL:         return std::errc::invalid_argument;
		case WSAENOBUFS:        return std::errc::no_buffer_space;
		case WSAEISCONN:        return std::errc::already_connected;
		case WSAENOTCONN:       return std::errc::not_connected;
		case WSAEACCES:         return std::errc::permission_denied;
		default:                return std::errc::io_error;
		}
	}

	std::error_condition last_wsa_error() noexcept
	{
		return wsa_error_to_error_condition(WSAGetLastError());
	}


	class win_osd_socket : public osd_file
	{
	public:
		win_osd_socket(win_osd_socket const&) = delete;
		win_osd_socket(win_osd_socket&&) = delete;
		win_osd_socket& operator=(win_osd_socket const&) = delete;
		win_osd_socket& operator=(win_osd_socket&&) = delete;

		win_osd_socket(SOCKET sock, bool listening) noexcept
			: m_sock(sock)
			, m_listening(listening)
		{
			assert(m_sock != INVALID_SOCKET);
		}

		virtual ~win_osd_socket()
		{
			closesocket(m_sock);
		}

		virtual std::error_condition read(void* buffer, std::uint64_t offset, std::uint32_t count, std::uint32_t& actual) noexcept override
		{
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(m_sock, &readfds);

			struct timeval timeout;
			timeout.tv_sec = timeout.tv_usec = 0;

			int const selresult = ::select(0, &readfds, nullptr, nullptr, &timeout);
			if (selresult < 0)
			{
				return last_wsa_error();
			}
			else if (FD_ISSET(m_sock, &readfds))
			{
				if (!m_listening)
				{
					// connected socket
					int const result = ::recv(m_sock, reinterpret_cast<char*>(buffer), count, 0);
					if (result < 0)
					{
						return last_wsa_error();
					}
					else
					{
						actual = std::uint32_t(result);
						return std::error_condition();
					}
				}
				else
				{
					// listening socket
					SOCKET const accepted = ::accept(m_sock, nullptr, nullptr);
					if (accepted == INVALID_SOCKET)
					{
						return last_wsa_error();
					}
					else
					{
						closesocket(m_sock);
						m_sock = accepted;
						m_listening = false;
						actual = 0;
						return std::error_condition();
					}
				}
			}
			else
			{
				// no data available
				actual = 0;
				return std::errc::operation_would_block;
			}
		}

		virtual std::error_condition write(void const* buffer, std::uint64_t offset, std::uint32_t count, std::uint32_t& actual) noexcept override
		{
			int const result = ::send(m_sock, reinterpret_cast<const char*>(buffer), count, 0);
			if (result < 0)
				return last_wsa_error();

			actual = std::uint32_t(result);
			return std::error_condition();
		}

		virtual std::error_condition truncate(std::uint64_t offset) noexcept override
		{
			// doesn't make sense on socket
			return std::errc::bad_file_descriptor;
		}

		virtual std::error_condition flush() noexcept override
		{
			// there's no simple way to flush buffers on a socket anyway
			return std::error_condition();
		}

	private:
		SOCKET  m_sock;
		bool    m_listening;
	};


	template <typename T>
	std::error_condition create_socket(T const& sa, SOCKET sock, std::uint32_t openflags, osd_file::ptr& file, std::uint64_t& filesize) noexcept
	{
		osd_file::ptr result;
		if (openflags & OPEN_FLAG_CREATE)
		{
			// listening socket support
			if (::bind(sock, reinterpret_cast<struct sockaddr const*>(&sa), sizeof(sa)) == SOCKET_ERROR)
			{
				std::error_condition binderr = last_wsa_error();
				closesocket(sock);
				return binderr;
			}

			if (::listen(sock, 1) == SOCKET_ERROR)
			{
				std::error_condition lstnerr = last_wsa_error();
				closesocket(sock);
				return lstnerr;
			}

			result.reset(new (std::nothrow) win_osd_socket(sock, true));
		}
		else
		{
			if (::connect(sock, reinterpret_cast<struct sockaddr const*>(&sa), sizeof(sa)) == SOCKET_ERROR)
			{
				std::error_condition connerr = last_wsa_error();
				closesocket(sock);
				return connerr;
			}
			result.reset(new (std::nothrow) win_osd_socket(sock, false));
		}

		if (!result)
		{
			closesocket(sock);
			return std::errc::not_enough_memory;
		}
		file = std::move(result);
		filesize = 0;
		return std::error_condition();
	}

} // anonymous namespace


bool win_check_socket_path(std::string const& path) noexcept
{
	if (strncmp(path.c_str(), winfile_socket_identifier, strlen(winfile_socket_identifier)) == 0 &&
		strchr(path.c_str(), ':') != nullptr) return true;
	return false;
}


std::error_condition win_open_socket(std::string const& path, std::uint32_t openflags, osd_file::ptr& file, std::uint64_t& filesize) noexcept
{
	// Initialize Winsock if not already done
	static bool wsa_initialized = false;
	if (!wsa_initialized)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			return std::errc::io_error;
		wsa_initialized = true;
	}

	char hostname[256];
	int port;
	std::sscanf(&path[strlen(winfile_socket_identifier)], "%255[^:]:%d", hostname, &port);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portstr[16];
	std::snprintf(portstr, sizeof(portstr), "%d", port);

	struct addrinfo* addrresult = nullptr;
	if (getaddrinfo(hostname, portstr, &hints, &addrresult) != 0)
		return std::errc::no_such_file_or_directory;

	SOCKET sock = ::socket(addrresult->ai_family, addrresult->ai_socktype, addrresult->ai_protocol);
	if (sock == INVALID_SOCKET)
	{
		std::error_condition sockerr = last_wsa_error();
		freeaddrinfo(addrresult);
		return sockerr;
	}

	int const flag = 1;
	if ((::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag)) == SOCKET_ERROR) ||
		(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag), sizeof(flag)) == SOCKET_ERROR))
	{
		std::error_condition sockopterr = last_wsa_error();
		closesocket(sock);
		freeaddrinfo(addrresult);
		return sockopterr;
	}

	// Copy the address before freeing
	struct sockaddr_in sai;
	memcpy(&sai, addrresult->ai_addr, sizeof(sai));
	freeaddrinfo(addrresult);

	return create_socket(sai, sock, openflags, file, filesize);
}
