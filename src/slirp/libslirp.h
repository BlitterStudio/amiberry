#ifndef _LIBSLIRP_H
#define _LIBSLIRP_H

#ifdef _WIN32
#include <winsock2.h>
int inet_aton(const char *cp, struct in_addr *ia);
#else
#include <sys/select.h>
#include <arpa/inet.h>
#endif

int slirp_init(void);
void slirp_cleanup(void);

int slirp_select_fill(int *pnfds, 
					  fd_set *readfds, fd_set *writefds, fd_set *xfds);

void slirp_select_poll(fd_set *readfds, fd_set *writefds, fd_set *xfds);

void slirp_input(const uint8 *pkt, int pkt_len);

/* you must provide the following functions: */
int slirp_can_output(void);
void slirp_output(const uint8 *pkt, int pkt_len);

int slirp_redir(int is_udp, int host_port, 
                struct in_addr guest_addr, int guest_port);
int slirp_add_exec(int do_pty, const char *args, int addr_low_byte, 
                   int guest_port);

extern char slirp_hostname[33];

#endif
