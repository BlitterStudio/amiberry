#ifndef UAE_SLIRP_H
#define UAE_SLIRP_H

#include "uae/types.h"

#ifdef _WIN32
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int uae_slirp_init(void);
void uae_slirp_cleanup(void);
int uae_slirp_redir(int is_udp, int host_port, struct in_addr guest_addr,
					int guest_port);
bool uae_slirp_start (void);
void uae_slirp_end (void);
void uae_slirp_input(const uint8_t *pkt, int pkt_len);

void slirp_output(const uint8_t *pkt, int pkt_len);

#endif /* UAE_SLIRP_H */
