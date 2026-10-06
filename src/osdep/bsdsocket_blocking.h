#pragma once

/*
 * Waiting helpers for the POSIX bsdsocket emulation's blocking calls.
 *
 * bsdthr_blockingstuff() runs every blocking Amiga call on a non-blocking
 * host socket and waits in select(), so the kernel never applies the
 * SO_RCVTIMEO/SO_SNDTIMEO the guest stored on the host socket. These helpers
 * apply those timeouts the way BSD stacks (and Roadshow) do: recv() obeys
 * SO_RCVTIMEO, send() obeys SO_SNDTIMEO, and connect()/accept() wait without
 * a limit. A zero timeout means wait forever.
 */

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <cerrno>
#include <chrono>

namespace bsdsock_blocking {

using wait_clock = std::chrono::steady_clock;

enum class direction { none, receive, send };

enum class wait_result { ready, aborted, timed_out, error };

/* Returns true and sets deadline when fd carries a non-zero timeout for dir. */
inline bool socket_deadline(int fd, direction dir, wait_clock::time_point start, wait_clock::time_point& deadline)
{
	if (dir == direction::none)
		return false;
	timeval tv{};
	socklen_t len = sizeof(tv);
	const int option = dir == direction::receive ? SO_RCVTIMEO : SO_SNDTIMEO;
	if (getsockopt(fd, SOL_SOCKET, option, &tv, &len) != 0)
		return false;
	if (tv.tv_sec <= 0 && tv.tv_usec <= 0)
		return false;
	deadline = start + std::chrono::seconds(tv.tv_sec) + std::chrono::microseconds(tv.tv_usec);
	return true;
}

/*
 * Waits until fd is readable (want_read) or writable, abort_fd is readable,
 * or the deadline passes. Without a deadline the wait is unbounded. Signal
 * interruptions are retried with the time that is left. On error, errno holds
 * select()'s error.
 */
inline wait_result wait_for_socket(int fd, int abort_fd, bool want_read, const wait_clock::time_point* deadline)
{
	const int maxfd = fd > abort_fd ? fd : abort_fd;
	for (;;) {
		fd_set readset, writeset;
		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		if (want_read)
			FD_SET(fd, &readset);
		else
			FD_SET(fd, &writeset);
		FD_SET(abort_fd, &readset);

		timeval tv{};
		timeval* timeout = nullptr;
		if (deadline) {
			auto left = std::chrono::duration_cast<std::chrono::microseconds>(*deadline - wait_clock::now());
			if (left.count() < 0)
				left = std::chrono::microseconds(0);
			tv.tv_sec = static_cast<decltype(tv.tv_sec)>(left.count() / 1000000);
			tv.tv_usec = static_cast<decltype(tv.tv_usec)>(left.count() % 1000000);
			timeout = &tv;
		}

		const int num = select(maxfd + 1, &readset, &writeset, nullptr, timeout);
		if (num < 0) {
			if (errno == EINTR)
				continue;
			return wait_result::error;
		}
		if (num == 0)
			return wait_result::timed_out;
		if (FD_ISSET(abort_fd, &readset))
			return wait_result::aborted;
		return wait_result::ready;
	}
}

} // namespace bsdsock_blocking
