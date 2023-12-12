
#include "sysconfig.h"
#include "sysdeps.h"

#include "ethernet.h"
#ifdef _WIN32
#include "win32_uaenet.h"
#endif
#include "threaddep/thread.h"
#include "options.h"
#include "traps.h"
#include "sana2.h"
#include "uae/slirp.h"
#include "gui.h"
#include "rommgr.h"

#ifndef HAVE_INET_ATON
static int inet_aton(const char *cp, struct in_addr *ia)
{
	uint32_t addr = inet_addr(cp);
	if (addr == 0xffffffff)
		return 0;
	ia->s_addr = addr;
	return 1;
}
#endif

struct ethernet_data
{
	ethernet_gotfunc *gotfunc;
	ethernet_getfunc *getfunc;
	void *userdata;
};

#define SLIRP_PORT_OFFSET 0

static const int slirp_ports[] = { 21, 22, 23, 80, 0 };

static struct ethernet_data *slirp_data;
static bool slirp_inited;
uae_sem_t slirp_sem1, slirp_sem2;
static int netmode;

static struct netdriverdata slirpd =
{
	UAENET_SLIRP,
	_T("slirp"), _T("SLIRP User Mode NAT"),
	1500,
	{ 0x00, 0x00, 0x00, 50, 51, 52 },
	{ 0x00, 0x00, 0x00, 50, 51, 52 },
	1
};
static struct netdriverdata slirpd2 =
{
	UAENET_SLIRP_INBOUND,
	_T("slirp_inbound"), _T("SLIRP + Open ports (21-23,80)"),
	1500,
	{ 0x00, 0x00, 0x00, 50, 51, 52 },
	{ 0x00, 0x00, 0x00, 50, 51, 52 },
	1
};

void slirp_output (const uint8_t *pkt, int pkt_len)
{
	if (!slirp_data)
		return;
	gui_flicker_led(LED_NET, 0, gui_data.net | 1);
	uae_sem_wait (&slirp_sem1);
	slirp_data->gotfunc (slirp_data->userdata, pkt, pkt_len);
	uae_sem_post (&slirp_sem1);
}

void ethernet_trigger (struct netdriverdata *ndd, void *vsd)
{
	if (!ndd)
		return;
	gui_flicker_led(LED_NET, 0, gui_data.net | 2);
	switch (ndd->type)
	{
#ifdef WITH_SLIRP
		case UAENET_SLIRP:
		case UAENET_SLIRP_INBOUND:
		{
			struct ethernet_data *ed = (struct ethernet_data*)vsd;
			if (slirp_data) {
				uae_u8 pkt[4000];
				int len = sizeof pkt;
				int v;
				uae_sem_wait (&slirp_sem1);
				v = slirp_data->getfunc(ed->userdata, pkt, &len);
				uae_sem_post (&slirp_sem1);
				if (v) {
					uae_sem_wait (&slirp_sem2);
					uae_slirp_input(pkt, len);
					uae_sem_post (&slirp_sem2);
				}
			}
		}
		return;
#endif
#ifdef WITH_UAENET_PCAP
		case UAENET_PCAP:
		uaenet_trigger (vsd);
		return;
#endif
	}
}

int ethernet_open (struct netdriverdata *ndd, void *vsd, void *user, ethernet_gotfunc *gotfunc, ethernet_getfunc *getfunc, int promiscuous, const uae_u8 *mac)
{
	switch (ndd->type)
	{
#ifdef WITH_SLIRP
		case UAENET_SLIRP:
		case UAENET_SLIRP_INBOUND:
		{
			struct ethernet_data *ed = (struct ethernet_data*)vsd;
			ed->gotfunc = gotfunc;
			ed->getfunc = getfunc;
			ed->userdata = user;
			slirp_data = ed;
			uae_sem_init (&slirp_sem1, 0, 1);
			uae_sem_init (&slirp_sem2, 0, 1);
			uae_slirp_init();
			for (int i = 0; i < MAX_SLIRP_REDIRS; i++) {
				struct slirp_redir *sr = &currprefs.slirp_redirs[i];
				if (sr->proto) {
					struct in_addr a;
					if (sr->srcport == 0) {
					    inet_aton("10.0.2.15", &a);
						uae_slirp_redir (0, sr->dstport, a, sr->dstport);
					} else {
#ifdef HAVE_STRUCT_IN_ADDR_S_UN
						a.S_un.S_addr = sr->addr;
#else
						a.s_addr = sr->addr;
#endif
						uae_slirp_redir (sr->proto == 1 ? 0 : 1, sr->dstport, a, sr->srcport);
					}
				}
			}
			if (ndd->type == UAENET_SLIRP_INBOUND) {
				struct in_addr a;
			    inet_aton("10.0.2.15", &a);
				for (int i = 0; slirp_ports[i]; i++) {
					int port = slirp_ports[i];
					int j;
					for (j = 0; j < MAX_SLIRP_REDIRS; j++) {
						struct slirp_redir *sr = &currprefs.slirp_redirs[j];
						if (sr->proto && sr->dstport == port)
							break;
					}
					if (j == MAX_SLIRP_REDIRS)
						uae_slirp_redir (0, port + SLIRP_PORT_OFFSET, a, port);
				}
			}
			netmode = ndd->type;
			uae_slirp_start ();
		}
		return 1;
#endif
#ifdef WITH_UAENET_PCAP
		case UAENET_PCAP:
		if (uaenet_open (vsd, ndd, user, gotfunc, getfunc, promiscuous, mac)) {
			netmode = ndd->type;
			return 1;
		}
		return 0;
#endif
	}
	return 0;
}

void ethernet_close (struct netdriverdata *ndd, void *vsd)
{
	if (!ndd)
		return;
	switch (ndd->type)
	{
#ifdef WITH_SLIRP
		case UAENET_SLIRP:
		case UAENET_SLIRP_INBOUND:
		if (slirp_data) {
			slirp_data = NULL;
			uae_slirp_end ();
			uae_slirp_cleanup ();
			uae_sem_destroy (&slirp_sem1);
			uae_sem_destroy (&slirp_sem2);
		}
		return;
#endif
#ifdef WITH_UAENET_PCAP
		case UAENET_PCAP:
		return uaenet_close (vsd);
#endif
	}
}

void ethernet_enumerate_free (void)
{
#ifdef WITH_UAENET_PCAP
	uaenet_enumerate_free ();
#endif
}

bool ethernet_enumerate (struct netdriverdata **nddp, int romtype)
{
	int j;
	struct netdriverdata *nd;
	const TCHAR *name = NULL;
	
	if (romtype) {
		struct romconfig *rc = get_device_romconfig(&currprefs, romtype, 0);
		name = ethernet_getselectionname(rc ? rc->device_settings : 0);
	}

	gui_flicker_led(LED_NET, 0, 0);
	if (name) {
		netmode = 0;
		*nddp = NULL;
		if (!_tcsicmp (slirpd.name, name))
			*nddp = &slirpd;
		if (!_tcsicmp (slirpd2.name, name))
			*nddp = &slirpd2;
#ifdef WITH_UAENET_PCAP
		if (*nddp == NULL)
			*nddp = uaenet_enumerate (name);
#endif
		if (*nddp) {
			netmode = (*nddp)->type;
			return true;
		}
		return false;
	}
	j = 0;
	nddp[j++] = &slirpd;
	nddp[j++] = &slirpd2;
#ifdef WITH_UAENET_PCAP
	nd = uaenet_enumerate (NULL);
	if (nd) {
		int last = MAX_TOTAL_NET_DEVICES - 1 - j;
		for (int i = 0; i < last; i++) {
			if (nd[i].active)
				nddp[j++] = &nd[i];
		}
	}
#endif
	nddp[j] = NULL;
	return true;
}

void ethernet_close_driver (struct netdriverdata *ndd)
{
	switch (ndd->type)
	{
		case UAENET_SLIRP:
		case UAENET_SLIRP_INBOUND:
		return;
#ifdef WITH_UAENET_PCAP
		case UAENET_PCAP:
		return uaenet_close_driver (ndd);
#endif
	}
	netmode = 0;
}

int ethernet_getdatalenght (struct netdriverdata *ndd)
{
	switch (ndd->type)
	{
		case UAENET_SLIRP:
		case UAENET_SLIRP_INBOUND:
		return sizeof (struct ethernet_data);
#ifdef WITH_UAENET_PCAP
		case UAENET_PCAP:
		return uaenet_getdatalenght ();
#endif
	}
	return 0;
}

bool ethernet_getmac(uae_u8 *m, const TCHAR *mac)
{
	if (!mac)
		return false;
	if (_tcslen(mac) != 3 * 5 + 2)
		return false;
	for (int i = 0; i < 6; i++) {
		TCHAR *endptr;
		if (mac[0] == 0 || mac[1] == 0)
			return false;
		if (i < 5 && (mac[2] != '.' && mac[2] != ':'))
			return false;
		uae_u8 v = (uae_u8)_tcstol(mac, &endptr, 16);
		mac += 3;
		m[i] = v;
	}
	return true;
}
