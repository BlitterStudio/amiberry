
#include "sysconfig.h"
#ifdef _WIN32
#include "Winsock2.h"
#endif
#include "sysdeps.h"

#ifdef WITH_SLIRP

#include "options.h"
#include "uae/slirp.h"
#include "uae.h"

#ifdef WITH_BUILTIN_SLIRP
#include "slirp/slirp.h"
#include "slirp/libslirp.h"
#include "threaddep/thread.h"
#endif

#ifdef WITH_QEMU_SLIRP
#include "uae/dlopen.h"
#include "uae/ppc.h"
#include "uae/qemu.h"
#endif

/* Implementation enumeration must correspond to slirp_implementations in
 * cfgfile.cpp. */
enum Implementation {
	AUTO_IMPLEMENTATION = 0,
	NO_IMPLEMENTATION,
	BUILTIN_IMPLEMENTATION,
	QEMU_IMPLEMENTATION,
};

static Implementation impl;

static Implementation check_conf(Implementation check)
{
	int conf = AUTO_IMPLEMENTATION;
	if (conf == AUTO_IMPLEMENTATION || conf == check) {
		return check;
	}
	return AUTO_IMPLEMENTATION;
}

int uae_slirp_init(void)
{
#if defined(WITH_QEMU_SLIRP)
	if (impl == AUTO_IMPLEMENTATION) {
		impl = check_conf(QEMU_IMPLEMENTATION);
	}
#endif
#if defined(WITH_BUILTIN_SLIRP)
	if (impl == AUTO_IMPLEMENTATION) {
		impl = check_conf(BUILTIN_IMPLEMENTATION);
	}
#endif
	if (impl == AUTO_IMPLEMENTATION) {
		impl = NO_IMPLEMENTATION;
	}

#ifdef WITH_QEMU_SLIRP
	if (impl == QEMU_IMPLEMENTATION) {
		return uae_qemu_uae_init() == NULL;
	}
#endif
#ifdef WITH_BUILTIN_SLIRP
	if (impl == BUILTIN_IMPLEMENTATION) {
		return slirp_init();
	}
#endif
	return -1;
}

void uae_slirp_cleanup(void)
{
#ifdef WITH_QEMU_SLIRP
	if (impl == QEMU_IMPLEMENTATION) {
		UAE_LOG_STUB("");
		return;
	}
#endif
#ifdef WITH_BUILTIN_SLIRP
	if (impl == BUILTIN_IMPLEMENTATION) {
		slirp_cleanup();
		return;
	}
#endif
}

void uae_slirp_input(const uint8_t *pkt, int pkt_len)
{
#ifdef WITH_QEMU_SLIRP
	if (impl == QEMU_IMPLEMENTATION) {
		if (qemu_uae_slirp_input) {
			qemu_uae_slirp_input(pkt, pkt_len);
		}
		return;
	}
#endif
#ifdef WITH_BUILTIN_SLIRP
	if (impl == BUILTIN_IMPLEMENTATION) {
		slirp_input(pkt, pkt_len);
		return;
	}
#endif
}

void uae_slirp_output(const uint8_t *pkt, int pkt_len)
{
#if 0
	write_log(_T("uae_slirp_output pkt_len %d\n"), pkt_len);
#endif
	slirp_output(pkt, pkt_len);
}

int uae_slirp_redir(int is_udp, int host_port, struct in_addr guest_addr,
		    int guest_port)
{
#ifdef WITH_QEMU_SLIRP
	if (impl == QEMU_IMPLEMENTATION) {
		UAE_LOG_STUB("");
		return 0;
	}
#endif
#ifdef WITH_BUILTIN_SLIRP
	if (impl == BUILTIN_IMPLEMENTATION) {
		return slirp_redir(is_udp, host_port, guest_addr, guest_port);
	}
#endif
	return 0;
}

#ifdef WITH_BUILTIN_SLIRP

static volatile int slirp_thread_active;
static uae_thread_id slirp_tid;
extern uae_sem_t slirp_sem2;

static void slirp_receive_func(void *arg)
{
	slirp_thread_active = 1;
	while (slirp_thread_active) {
		// Wait for packets to arrive
		fd_set rfds, wfds, xfds;
		INT_PTR nfds;
		int ret, timeout;

		// ... in the output queue
		nfds = -1;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&xfds);
		uae_sem_wait (&slirp_sem2);
		timeout = slirp_select_fill(&nfds, &rfds, &wfds, &xfds);
		uae_sem_post (&slirp_sem2);
		if (nfds < 0) {
			/* Windows does not honour the timeout if there is not
			   descriptor to wait for */
			sleep_millis (timeout / 1000);
			ret = 0;
		} else {
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = timeout;
			ret = select(0, &rfds, &wfds, &xfds, &tv);
			if (ret == SOCKET_ERROR) {
				write_log(_T("SLIRP socket ERR=%d\n"), WSAGetLastError());
			}
		}
		if (ret >= 0) {
			uae_sem_wait (&slirp_sem2);
			slirp_select_poll(&rfds, &wfds, &xfds);
			uae_sem_post (&slirp_sem2);
		}
	}
	slirp_thread_active = -1;
}

int slirp_can_output(void)
{
	return 1;
}

#endif

bool uae_slirp_start (void)
{
#ifdef WITH_QEMU_SLIRP
	if (impl == QEMU_IMPLEMENTATION) {
		UAE_LOG_STUB("");
		return true;
	}
#endif
#ifdef WITH_BUILTIN_SLIRP
	if (impl == BUILTIN_IMPLEMENTATION) {
		uae_slirp_end ();
		uae_start_thread(_T("slirp-receive"), slirp_receive_func, NULL,
						 &slirp_tid);
		return true;
	}
#endif
	return false;
}

void uae_slirp_end (void)
{
#ifdef WITH_QEMU_SLIRP
	if (impl == QEMU_IMPLEMENTATION) {
		UAE_LOG_STUB("");
		return;
	}
#endif
#ifdef WITH_BUILTIN_SLIRP
	if (impl == BUILTIN_IMPLEMENTATION) {
		if (slirp_thread_active > 0) {
			slirp_thread_active = 0;
			while (slirp_thread_active == 0) {
				sleep_millis (10);
			}
			uae_end_thread (&slirp_tid);
		}
		slirp_thread_active = 0;
		return;
	}
#endif
}

#endif /* WITH_SLIRP */
