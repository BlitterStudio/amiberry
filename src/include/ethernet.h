#ifndef UAE_ETHERNET_H
#define UAE_ETHERNET_H

#include "uae/types.h"

#define UAENET_NONE 0
#define UAENET_SLIRP 1
#define UAENET_SLIRP_INBOUND 2
#define UAENET_PCAP 3

struct netdriverdata
{
	int type;
	const TCHAR *name;
	const TCHAR *desc;
	int mtu;
	uae_u8 mac[6];
	uae_u8 originalmac[6];
	int active;
};


typedef void (ethernet_gotfunc)(void *dev, const uae_u8 *data, int len);
typedef int (ethernet_getfunc)(void *dev, uae_u8 *d, int *len);

extern bool ethernet_enumerate (struct netdriverdata **, int romtype);
extern void ethernet_enumerate_free (void);
extern void ethernet_close_driver (struct netdriverdata *ndd);

extern int ethernet_getdatalenght (struct netdriverdata *ndd);
extern int ethernet_open (struct netdriverdata *ndd, void*, void*, ethernet_gotfunc*, ethernet_getfunc*, int, const uae_u8 *mac);
extern void ethernet_close (struct netdriverdata *ndd, void*);
extern void ethernet_trigger (struct netdriverdata *ndd, void*);

extern bool ariadne2_init(struct autoconfig_info *aci);
extern bool hydra_init(struct autoconfig_info *aci);
extern bool lanrover_init(struct autoconfig_info *aci);
extern bool xsurf_init(struct autoconfig_info *aci);
extern bool xsurf100_init(struct autoconfig_info *aci);

void ethernet_updateselection(void);
uae_u32 ethernet_getselection(const TCHAR*);
const TCHAR *ethernet_getselectionname(uae_u32 settings);
bool ethernet_getmac(uae_u8 *m, const TCHAR *mac);

void ethernet_pause(int);
void ethernet_reset(void);

#endif /* UAE_ETHERNET_H */
