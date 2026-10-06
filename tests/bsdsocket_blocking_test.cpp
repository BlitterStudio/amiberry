#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "bsdsocket_blocking.h"

using namespace bsdsock_blocking;
using std::chrono::milliseconds;

static int failures;

static void expect(bool condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		failures++;
	}
}

struct tcp_pair {
	int client = -1;
	int server = -1;
};

/* A connected loopback TCP pair; nothing is ever sent unless a test does. */
static tcp_pair make_tcp_pair()
{
	tcp_pair pair;
	const int listener = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	socklen_t len = sizeof(addr);
	if (listener < 0 || bind(listener, (sockaddr *)&addr, sizeof(addr)) != 0 || listen(listener, 1) != 0
		|| getsockname(listener, (sockaddr *)&addr, &len) != 0) {
		perror("listener");
		exit(2);
	}
	pair.client = socket(AF_INET, SOCK_STREAM, 0);
	if (pair.client < 0 || connect(pair.client, (sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("connect");
		exit(2);
	}
	pair.server = accept(listener, nullptr, nullptr);
	if (pair.server < 0) {
		perror("accept");
		exit(2);
	}
	close(listener);
	return pair;
}

static void close_pair(const tcp_pair &pair)
{
	close(pair.client);
	close(pair.server);
}

static void set_timeout(int fd, int option, long sec, long usec)
{
	timeval tv{};
	tv.tv_sec = sec;
	tv.tv_usec = usec;
	if (setsockopt(fd, SOL_SOCKET, option, &tv, sizeof(tv)) != 0) {
		perror("setsockopt");
		exit(2);
	}
}

static long elapsed_ms(wait_clock::time_point since)
{
	return (long)std::chrono::duration_cast<milliseconds>(wait_clock::now() - since).count();
}

static void test_deadline_follows_the_direction_option()
{
	const tcp_pair pair = make_tcp_pair();
	const wait_clock::time_point start = wait_clock::now();
	wait_clock::time_point deadline;

	expect(!socket_deadline(pair.client, direction::receive, start, deadline),
		"a socket without SO_RCVTIMEO has no receive deadline");

	set_timeout(pair.client, SO_RCVTIMEO, 1, 500000);
	expect(socket_deadline(pair.client, direction::receive, start, deadline),
		"SO_RCVTIMEO gives recv a deadline");
	expect(deadline - start == milliseconds(1500), "the receive deadline is start plus SO_RCVTIMEO");
	expect(!socket_deadline(pair.client, direction::send, start, deadline),
		"SO_RCVTIMEO does not limit send");
	expect(!socket_deadline(pair.client, direction::none, start, deadline),
		"connect and accept ignore socket timeouts");

	set_timeout(pair.client, SO_SNDTIMEO, 0, 250000);
	expect(socket_deadline(pair.client, direction::send, start, deadline),
		"SO_SNDTIMEO gives send a deadline");
	expect(deadline - start == milliseconds(250), "the send deadline is start plus SO_SNDTIMEO");

	set_timeout(pair.client, SO_RCVTIMEO, 0, 0);
	expect(!socket_deadline(pair.client, direction::receive, start, deadline),
		"clearing SO_RCVTIMEO restores the unbounded wait");
	close_pair(pair);
}

static void test_silent_peer_times_out()
{
	const tcp_pair pair = make_tcp_pair();
	int abort_pipe[2];
	if (pipe(abort_pipe) != 0)
		exit(2);

	const wait_clock::time_point start = wait_clock::now();
	const wait_clock::time_point deadline = start + milliseconds(200);
	expect(wait_for_socket(pair.client, abort_pipe[0], true, &deadline) == wait_result::timed_out,
		"recv from a silent peer times out at the deadline");
	const long waited = elapsed_ms(start);
	expect(waited >= 190 && waited < 2000, "the receive wait lasts until the deadline");

	const wait_clock::time_point past = wait_clock::now() - milliseconds(10);
	expect(wait_for_socket(pair.client, abort_pipe[0], true, &past) == wait_result::timed_out,
		"an expired deadline times out without waiting");

	close(abort_pipe[0]);
	close(abort_pipe[1]);
	close_pair(pair);
}

static void test_ready_and_abort_win_over_the_deadline()
{
	const tcp_pair pair = make_tcp_pair();
	int abort_pipe[2];
	if (pipe(abort_pipe) != 0)
		exit(2);
	const wait_clock::time_point deadline = wait_clock::now() + milliseconds(5000);

	if (write(pair.server, "x", 1) != 1)
		exit(2);
	wait_clock::time_point start = wait_clock::now();
	expect(wait_for_socket(pair.client, abort_pipe[0], true, &deadline) == wait_result::ready,
		"pending data makes the socket ready before the deadline");
	expect(elapsed_ms(start) < 1000, "ready data returns without waiting for the deadline");

	char byte;
	if (read(pair.client, &byte, 1) != 1)
		exit(2);
	if (write(abort_pipe[1], "a", 1) != 1)
		exit(2);
	expect(wait_for_socket(pair.client, abort_pipe[0], true, nullptr) == wait_result::aborted,
		"the abort pipe ends an unbounded wait");
	if (read(abort_pipe[0], &byte, 1) != 1)
		exit(2);

	expect(wait_for_socket(pair.client, abort_pipe[0], false, nullptr) == wait_result::ready,
		"a connected socket with buffer space is writable");

	close(abort_pipe[0]);
	close(abort_pipe[1]);
	close_pair(pair);
}

static void test_full_send_buffer_times_out()
{
	const tcp_pair pair = make_tcp_pair();
	int abort_pipe[2];
	if (pipe(abort_pipe) != 0)
		exit(2);

	/* The peer never reads, so filling the buffer makes send block. */
	fcntl(pair.client, F_SETFL, fcntl(pair.client, F_GETFL) | O_NONBLOCK);
	static char chunk[65536];
	while (send(pair.client, chunk, sizeof(chunk), 0) > 0) {
	}

	const wait_clock::time_point start = wait_clock::now();
	const wait_clock::time_point deadline = start + milliseconds(150);
	expect(wait_for_socket(pair.client, abort_pipe[0], false, &deadline) == wait_result::timed_out,
		"send to a peer that stopped reading times out at the deadline");
	expect(elapsed_ms(start) >= 140, "the send wait lasts until the deadline");

	close(abort_pipe[0]);
	close(abort_pipe[1]);
	close_pair(pair);
}

int main()
{
	/* A regression here hangs in select(); fail instead of stalling the suite. */
	alarm(20);

	test_deadline_follows_the_direction_option();
	test_silent_peer_times_out();
	test_ready_and_abort_win_over_the_deadline();
	test_full_send_buffer_times_out();

	if (failures)
		return 1;
	printf("bsdsocket_blocking_test: all tests passed\n");
	return 0;
}
