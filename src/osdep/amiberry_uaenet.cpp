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
static struct uaenet_data **uaenet_data;
static int uaenet_count;
static uae_sem_t queue_available;

// Adds packet to outgoing queue
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
    uae_sem_post(&queue_available);
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
        // Check if there are any packets to process
        if (uaenet_checkpacket(ud))
            continue;
        
        // Wait for more packets
        uae_sem_wait(&queue_available);
        
        // Check if we should exit
        if (!ud->active2)
            break;
        
        // Process packets from pcap
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
    
    uae_sem_post(&queue_available);
    
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
    
    // Allocate new uaenet_data if needed
    if (!uaenet_data) {
        uaenet_count = 10;
        uaenet_data = xcalloc(struct uaenet_data*, uaenet_count);
        if (!uaenet_data)
            return 0;
        uae_sem_init(&queue_available, 0, 0);
    }
    
    write_log(_T("UAENET: Opening '%s'\n"), ndd->name);
    
    ud->mtu = MAX_MTU;
    ud->promiscuous = promiscuous;
    strncpy(ud->name, ndd->name, MAX_DPATH - 1);
    ud->name[MAX_DPATH - 1] = '\0';
    
    if (mac)
        memcpy(ud->mac_addr, mac, 6);
    
    uae_sem_init(&ud->change_sem, 0, 1);
    uae_sem_init(&ud->sync_sem, 0, 0);
    uae_sem_init(&ud->queue_sem, 0, 1);
    
    // Open the device
    ud->handle = pcap_open_live(ndd->name, 65536, promiscuous, 1, ud->errbuf);
    if (!ud->handle) {
        write_log(_T("UAENET: Failed to open device: %s\n"), ud->errbuf);
        uaenet_close_driver_internal(ud);
        return 0;
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
    
    int i;
    for (i = 0; i < uaenet_count; i++) {
        if (uaenet_data && uaenet_data[i] == ud) {
            uaenet_close_driver_internal(ud);
            uaenet_data[i] = NULL;
            
            // Free any remaining queued packets
            struct uaenet_queue *q = ud->first;
            while (q) {
                struct uaenet_queue *next = q->next;
                xfree(q->data);
                xfree(q);
                q = next;
            }
            
            xfree(ud);
            break;
        }
    }
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

// Trigger packet processing
void uaenet_trigger(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud || !ud->active)
        return;
    
    // Signal that there might be data to process
    uae_sem_post(&queue_available);
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
    ethernet_paused = pause;
}

// Reset ethernet subsystem
void ethernet_reset(void)
{
    ethernet_paused = 0;
}
