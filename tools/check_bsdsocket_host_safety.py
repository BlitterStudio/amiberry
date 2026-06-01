#!/usr/bin/env python3
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/osdep/bsdsocket_host.cpp"


def extract_body(text, signature):
	start = text.find(signature)
	if start == -1:
		raise AssertionError(f"missing {signature}")
	brace = text.find("{", start)
	if brace == -1:
		raise AssertionError(f"missing body for {signature}")
	depth = 0
	for pos in range(brace, len(text)):
		if text[pos] == "{":
			depth += 1
		elif text[pos] == "}":
			depth -= 1
			if depth == 0:
				return text[brace + 1:pos]
	raise AssertionError(f"unterminated body for {signature}")


def require(condition, message, failures):
	if not condition:
		failures.append(message)


def main():
	text = SOURCE.read_text()
	posix = text[text.index("#else /* !_WIN32 */"):]
	failures = []

	cleanup = extract_body(posix, "void host_sbcleanup (SB)")
	wait_pos = cleanup.find("uae_wait_thread")
	close_pipe_pos = cleanup.find("close_pipe")
	close_socket_pos = cleanup.find("close_socket")
	require(wait_pos != -1, "POSIX host_sbcleanup must join the worker thread", failures)
	require(close_pipe_pos != -1, "POSIX host_sbcleanup must close the abort pipe", failures)
	require(close_socket_pos != -1, "POSIX host_sbcleanup must close socket table entries", failures)
	require(wait_pos < close_pipe_pos, "POSIX host_sbcleanup must join before closing the abort pipe", failures)
	require(wait_pos < close_socket_pos, "POSIX host_sbcleanup must join before closing sockets", failures)
	require("uae_sem_destroy" in cleanup and cleanup.find("uae_sem_destroy") > wait_pos,
		"POSIX host_sbcleanup must destroy the semaphore after joining", failures)

	threadfunc = extract_body(posix, "static int bsdlib_threadfunc")
	case0 = threadfunc[threadfunc.find("case 0:"):threadfunc.find("case 1:")]
	require("uae_sem_destroy(&sb->sem)" not in case0,
		"socket worker must not destroy the semaphore it is waiting on", failures)

	post_event = extract_body(posix, "static void post_socket_event")
	require("sd >= sb->dtablesize" in post_event,
		"post_socket_event must bounds-check the Amiga descriptor", failures)

	setsockopt = extract_body(posix, "void host_setsockopt")
	require("setsockopt_argument_size" in setsockopt,
		"host_setsockopt must validate Amiga option buffer sizes", failures)
	require("optval == 0" in setsockopt,
		"host_setsockopt must reject null option buffers before mapping values", failures)
	require("len < minlen" in setsockopt or "len < (uae_u32)minlen" in setsockopt,
		"host_setsockopt must reject short option buffers before reading them", failures)

	getsockopt = extract_body(posix, "uae_u32 host_getsockopt")
	require("getsockopt_result_size" in getsockopt,
		"host_getsockopt must validate Amiga result buffer sizes", failures)
	require("nativeoptname == -1" in getsockopt,
		"host_getsockopt must reject unmapped options before calling the host", failures)
	require("len < minlen" in getsockopt,
		"host_getsockopt must reject short result buffers before writing them", failures)

	require("socket_fd_usable_for_select" in posix,
		"POSIX select paths must guard fds against FD_SETSIZE overflow", failures)

	if failures:
		for failure in failures:
			print(f"FAIL: {failure}")
		return 1

	print("bsdsocket host safety checks passed")
	return 0


if __name__ == "__main__":
	sys.exit(main())
