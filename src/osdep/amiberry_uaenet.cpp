/*
* UAE - The Un*x Amiga Emulator
*
* Amiberry uaenet emulation (libpcap-based, Linux/macOS)
*
* Copyright 2025 Dimitris Panokostas
*/

#include "sysconfig.h"
#include "sysdeps.h"
#include "ethernet.h"

#ifdef WITH_UAENET_PCAP
#include <pcap.h>

#include "options.h"
#include "sana2.h"
#include "threaddep/thread.h"

#define MAX_PSIZE 1600
#define MAX_MTU 1500


// Forward declarations
static int uaenet_worker_thread(void *arg);
static void uaenet_close_driver_internal(struct uaenet_data *ud);

struct uaenet_queue {
    uae_u8 *data;
    int len;
    struct uaenet_queue *next;
};

struct uaenet_data {
    int slirp;
    int promiscuous;
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    bool active;
    bool active2;
    uae_thread_id tid;
    ethernet_gotfunc *gotfunc;
    ethernet_getfunc *getfunc;
    void *userdata;
    void *user_cb_data;
    int mtu;
    int packetsinbuffer;
    bool prehandler;
    struct uaenet_queue *first, *last;
    bool closed;
    uae_sem_t change_sem;
    uae_sem_t sync_sem;
    uae_sem_t queue_sem;
    char name[MAX_DPATH];
    uae_u8 mac_addr[6];
};

int log_ethernet;
static int enumerated;
static int ethernet_paused;

// Adds received packet to incoming queue (called from worker thread)
static void uaenet_queue(struct uaenet_data *ud, const uae_u8 *data, int len)
{
    struct uaenet_queue *q;

    if (!ud || len <= 0 || len > MAX_PSIZE)
        return;

    if (ud->packetsinbuffer > 10)
        return;

    uae_sem_wait(&ud->queue_sem);
    
    q = xcalloc(struct uaenet_queue, 1);
    q->data = xmalloc(uae_u8, len);
    memcpy(q->data, data, len);
    q->len = len;
    q->next = NULL;
    if (!ud->first) {
        ud->first = q;
        ud->last = q;
    } else {
        ud->last->next = q;
        ud->last = q;
    }
    ud->packetsinbuffer++;
    uae_sem_post(&ud->queue_sem);
}

// Process packets in queue
static int uaenet_checkpacket(struct uaenet_data *ud)
{
    struct uaenet_queue *q;
    
    if (!ud || !ud->first)
        return 0;
    
    uae_sem_wait(&ud->queue_sem);
    q = ud->first;
    if (!q) {
        uae_sem_post(&ud->queue_sem);
        return 0;
    }
    ud->first = ud->first->next;
    if (!ud->first)
        ud->last = NULL;
    uae_sem_post(&ud->queue_sem);
    
    if (ud->active && ud->active2 && ud->gotfunc) {
        ud->gotfunc(ud->userdata, q->data, q->len);
    }
    
    xfree(q->data);
    xfree(q);
    ud->packetsinbuffer--;
    return 1;
}

// Thread to handle network traffic
static int uaenet_worker_thread(void *arg)
{
    struct uaenet_data *ud = (struct uaenet_data*)arg;
    struct pcap_pkthdr *header;
    const u_char *pkt_data;

    ud->active2 = true;
    uae_sem_post(&ud->sync_sem);

    while (ud->active2) {
        // NOTE: received packet delivery (uaenet_checkpacket) is NOT done here.
        // It is done on the emulation thread via uaenet_receive_poll() to avoid
        // concurrent access to Am7990 chip state (csr[], boardram, interrupts).

        // Poll pcap for new packets (uses timeout from pcap_open_live)
        if (ud->handle) {
            int res = pcap_next_ex(ud->handle, &header, &pkt_data);
            if (res == 1 && header && pkt_data) {
                uaenet_queue(ud, pkt_data, header->caplen);
            }
        }
    }

    ud->active2 = false;
    return 0;
}

// Internal close function
static void uaenet_close_driver_internal(struct uaenet_data *ud)
{
    if (!ud)
        return;
    
    ud->active = false;
    ud->active2 = false;

    if (ud->tid) {
        write_log(_T("UAENET: Waiting for thread to terminate..\n"));
        uae_wait_thread(&ud->tid);
        ud->tid = 0;
        write_log(_T("UAENET: Thread terminated\n"));
    }
    
    if (ud->handle) {
        pcap_close(ud->handle);
        ud->handle = NULL;
    }
}

static struct netdriverdata nd[MAX_TOTAL_NET_DEVICES + 1];

// Enumerate network devices
struct netdriverdata *uaenet_enumerate(const TCHAR *name)
{
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];
    memset(nd, 0, sizeof(nd));
    
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        write_log(_T("UAENET: Error in pcap_findalldevs: %s\n"), errbuf);
        return NULL;
    }
    
    int j = 0;
    for (pcap_if_t *d = alldevs; d && j < MAX_TOTAL_NET_DEVICES; d = d->next) {
        if (name != NULL && name[0] != '\0') {
            if (_tcsicmp(name, d->name) != 0)
                continue;
        }
        
        TCHAR *n2 = my_strdup(d->name);
        if (d->description)
            nd[j].desc = my_strdup(d->description);
        else
            nd[j].desc = my_strdup(d->name);
        
        nd[j].name = n2;
        nd[j].type = UAENET_PCAP;
        nd[j].active = 1;
        nd[j].driverdata = nullptr; // Initialize driverdata to nullptr
        j++;
        
        if (name != NULL)
            break;
    }
    
    pcap_freealldevs(alldevs);
    enumerated = 1;
    return nd;
}

// Version that matches the signature in ethernet.cpp
void uaenet_close_driver(struct netdriverdata *ndd)
{
    write_log(_T("UAENET: close_driver called for %s\n"), ndd->name);
    if (ndd && ndd->driverdata) {
        uaenet_close_driver_internal((struct uaenet_data*)ndd->driverdata);
        ndd->driverdata = nullptr;
    }
}

// Open a network device
int uaenet_open(void *vsd, struct netdriverdata *ndd, void *userdata, ethernet_gotfunc *gotfunc, ethernet_getfunc *getfunc, int promiscuous, const uae_u8 *mac)
{
    if (!enumerated)
        return 0;

    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud)
        return 0;

    // If already open (re-init), shut down existing thread and handle first
    if (ud->active || ud->active2 || ud->handle) {
        write_log(_T("UAENET: Re-opening '%s', shutting down existing state\n"), ndd->name);
        ud->active = false;
        ud->active2 = false;
        if (ud->tid) {
            uae_wait_thread(&ud->tid);
            ud->tid = 0;
        }
        if (ud->handle) {
            pcap_close(ud->handle);
            ud->handle = NULL;
        }
        // Flush any queued packets
        struct uaenet_queue *q = ud->first;
        while (q) {
            struct uaenet_queue *next = q->next;
            xfree(q->data);
            xfree(q);
            q = next;
        }
        ud->first = ud->last = NULL;
        ud->packetsinbuffer = 0;
    }

    write_log(_T("UAENET: Opening '%s'\n"), ndd->name);

    ud->mtu = MAX_MTU;
    ud->promiscuous = promiscuous;
    strncpy(ud->name, ndd->name, MAX_DPATH - 1);
    ud->name[MAX_DPATH - 1] = '\0';

    if (mac)
        memcpy(ud->mac_addr, mac, 6);

    // Destroy existing semaphores before reinitializing to prevent value
    // inflation — uae_sem_init() on an existing semaphore calls SDL_SemPost()
    // instead of recreating, which inflates mutex values and breaks synchronization.
    if (ud->change_sem) uae_sem_destroy(&ud->change_sem);
    if (ud->sync_sem) uae_sem_destroy(&ud->sync_sem);
    if (ud->queue_sem) uae_sem_destroy(&ud->queue_sem);
    uae_sem_init(&ud->change_sem, 0, 1);
    uae_sem_init(&ud->sync_sem, 0, 0);
    uae_sem_init(&ud->queue_sem, 0, 1);
    // Open the device
    // Always use promiscuous mode: on Linux bridge interfaces, the host
    // interface (br0) has a different MAC than the emulated device. Without
    // promiscuous mode, the kernel only delivers packets addressed to br0's
    // own MAC — packets to the Amiga's MAC are silently discarded before
    // reaching the pcap socket. The BPF filter already limits capture to
    // relevant packets (to our MAC, broadcast, multicast).
    ud->handle = pcap_open_live(ndd->name, 65536, 1, 1, ud->errbuf);
    if (!ud->handle) {
        write_log(_T("UAENET: Failed to open device: %s\n"), ud->errbuf);
        uaenet_close_driver_internal(ud);
        return 0;
    }
    
    // BPF filter: accept packets destined to the Amiga's MAC, broadcast, or
    // multicast — but reject packets sourced from the Amiga's own MAC.
    // The source exclusion prevents transmit loopback (packets we send via
    // pcap_sendpacket being immediately recaptured by pcap_next_ex).
    // We do NOT use pcap_setdirection(PCAP_D_IN) because on Linux bridge
    // interfaces, broadcast/multicast packets may be classified as "outgoing"
    // by the kernel, causing them to be silently dropped.
    if (mac) {
        struct bpf_program fp;
        char filter[256];
        snprintf(filter, sizeof(filter),
            "(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast or ether multicast)"
            " and not ether src %02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        write_log(_T("UAENET: BPF filter: %s\n"), filter);
        if (pcap_compile(ud->handle, &fp, filter, 1, PCAP_NETMASK_UNKNOWN) == 0) {
            if (pcap_setfilter(ud->handle, &fp) != 0) {
                write_log(_T("UAENET: pcap_setfilter failed: %s (non-fatal)\n"),
                          pcap_geterr(ud->handle));
            }
            pcap_freecode(&fp);
        } else {
            write_log(_T("UAENET: pcap_compile failed: %s (non-fatal)\n"),
                      pcap_geterr(ud->handle));
        }
    }

    // Store the pointer to uaenet_data in the netdriverdata for later cleanup
    ndd->driverdata = ud;

    ud->gotfunc = gotfunc;
    ud->getfunc = getfunc;
    ud->userdata = userdata;
    ud->active = true;

    // Start worker thread
    uae_start_thread(_T("uaenet_pcap"), uaenet_worker_thread, ud, &ud->tid);
    uae_sem_wait(&ud->sync_sem);
    
    write_log(_T("UAENET: '%s' open successful\n"), ndd->name);
    
    return 1;
}

// Send a packet
int uaenet_send(void *vsd, const void *data, int len)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud || !ud->handle || !ud->active)
        return 0;
    
    return pcap_sendpacket(ud->handle, (const u_char *)data, len) == 0;
}

// Close device
void uaenet_close(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud)
        return;

    uaenet_close_driver_internal(ud);

    // Free any remaining queued packets
    struct uaenet_queue *q = ud->first;
    while (q) {
        struct uaenet_queue *next = q->next;
        xfree(q->data);
        xfree(q);
        q = next;
    }
    ud->first = ud->last = NULL;
    ud->packetsinbuffer = 0;
    // Note: do NOT xfree(ud) — the caller (a2065.cpp) owns the sysdata buffer
}

// Get MAC address of device
uae_u8 *uaenet_getmac(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud)
        return NULL;
    
    // If we have a stored MAC address, use it
    if (ud->mac_addr[0] || ud->mac_addr[1] || ud->mac_addr[2] || 
        ud->mac_addr[3] || ud->mac_addr[4] || ud->mac_addr[5]) {
        return ud->mac_addr;
    }
    
    // Otherwise return a default MAC
    static uae_u8 mac[6] = { 0x00, 0x80, 0xD3, 0x54, 0x26, 0x1A };
    return mac;
}

// Get MTU of device
int uaenet_getmtu(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud)
        return MAX_MTU;
    
    return ud->mtu;
}

// Deliver queued received packets to the emulation — must be called from the emulation thread.
void uaenet_receive_poll(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud || !ud->active)
        return;
    while (uaenet_checkpacket(ud))
        ;
}

// Get number of bytes pending
int uaenet_getbytespending(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud)
        return 0;
    
    // Count bytes in all pending packets
    int bytes = 0;
    struct uaenet_queue *q = ud->first;
    while (q) {
        bytes += q->len;
        q = q->next;
    }
    return bytes;
}

// Trigger packet transmission — called from ethernet_trigger() on the emulation thread
// when the A2065 has a packet to send. Retrieves the packet via getfunc and sends
// it directly via pcap_sendpacket.
void uaenet_trigger(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud || !ud->active || !ud->handle)
        return;

    if (ud->getfunc) {
        uae_u8 pkt[4000];
        int len = sizeof pkt;
        if (ud->getfunc(ud->userdata, pkt, &len)) {
            pcap_sendpacket(ud->handle, (const u_char *)pkt, len);
        }
    }
}

// Free memory allocated during enumeration
void uaenet_enumerate_free()
{
#if defined(WITH_UAENET_PCAP)
    extern struct netdriverdata *uaenet_enumerate(const TCHAR *name);
    // The static array is only accessible in uaenet_enumerate, so we must duplicate the static definition here
    extern struct netdriverdata nd[MAX_TOTAL_NET_DEVICES + 1];
    for (int i = 0; i < MAX_TOTAL_NET_DEVICES + 1; ++i) {
        if (nd[i].name) {
            xfree((void*)nd[i].name);
            nd[i].name = nullptr;
        }
        if (nd[i].desc) {
            xfree((void*)nd[i].desc);
            nd[i].desc = nullptr;
        }
    }
    enumerated = 0;
#endif
}

// Return the size of the uaenet_data structure
int uaenet_getdatalenght(void)
{
    return sizeof(struct uaenet_data);
}

#endif // WITH_UAENET_PCAP

// Pause ethernet operations
void ethernet_pause(int pause)
{
#ifdef WITH_UAENET_PCAP
    ethernet_paused = pause;
#endif
}

// Reset ethernet subsystem
void ethernet_reset(void)
{
#ifdef WITH_UAENET_PCAP
    ethernet_paused = 0;
#endif
}
