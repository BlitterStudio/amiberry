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

#if defined(WITH_UAENET_PCAP) || defined(WITH_UAENET_TAP)
#ifdef WITH_UAENET_PCAP
#include <pcap.h>
#endif

#include "options.h"
#include "sana2.h"
#include "threaddep/thread.h"

#define MAX_MTU 1500
#ifndef ETH_HLEN
#define ETH_HLEN 14
#endif
#define MAX_FRAME_LEN 1514    // Standard Ethernet frame without FCS

#ifdef WITH_UAENET_PCAP
#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif
#define IP_PROTO_TCP 6
#define MAX_GRO_SIZE 65536    // Max coalesced packet size from GRO/virtio
#endif

// Forward declarations
#ifdef WITH_UAENET_PCAP
static int uaenet_worker_thread(void *arg);
#endif
static void uaenet_close_driver_internal(struct uaenet_data *ud);

struct uaenet_queue {
    uae_u8 *data;
    int len;
    struct uaenet_queue *next;
};

struct uaenet_data {
    int slirp;
    int promiscuous;
#ifdef WITH_UAENET_PCAP
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
#endif
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

#ifdef WITH_UAENET_PCAP
// Internet checksum (RFC 1071)
static uae_u16 compute_ip_checksum(const uae_u8 *data, int len)
{
    uae_u32 sum = 0;
    for (int i = 0; i < len - 1; i += 2)
        sum += (data[i] << 8) | data[i + 1];
    if (len & 1)
        sum += data[len - 1] << 8;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return (uae_u16)(~sum & 0xffff);
}

// TCP checksum with pseudo-header
static uae_u16 compute_tcp_checksum(const uae_u8 *ip_hdr, int ip_hdr_len,
    const uae_u8 *tcp_data, int tcp_len)
{
    uae_u32 sum = 0;
    // Pseudo-header: src IP, dst IP, zero+protocol, TCP length
    sum += (ip_hdr[12] << 8) | ip_hdr[13];
    sum += (ip_hdr[14] << 8) | ip_hdr[15];
    sum += (ip_hdr[16] << 8) | ip_hdr[17];
    sum += (ip_hdr[18] << 8) | ip_hdr[19];
    sum += IP_PROTO_TCP;
    sum += tcp_len;
    // TCP header + data
    for (int i = 0; i < tcp_len - 1; i += 2)
        sum += (tcp_data[i] << 8) | tcp_data[i + 1];
    if (tcp_len & 1)
        sum += tcp_data[tcp_len - 1] << 8;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return (uae_u16)(~sum & 0xffff);
}
#endif

// Insert a single packet into the incoming queue (called from worker thread)
static void uaenet_queue_one(struct uaenet_data *ud, const uae_u8 *data, int len)
{
    struct uaenet_queue *q;

    if (!ud || len <= 0 || len > MAX_FRAME_LEN)
        return;

    if (ud->packetsinbuffer > 50)
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

#ifdef WITH_UAENET_PCAP
// Segment GRO/GSO-coalesced IPv4/TCP packets into individual Ethernet frames.
// Linux GRO, GSO, and virtio NICs can deliver TCP segments coalesced into packets
// far larger than the Ethernet MTU. The A2065 emulation expects standard-sized
// Ethernet frames, so we must split them back into individual segments here.
static void uaenet_queue(struct uaenet_data *ud, const uae_u8 *data, int len)
{
    if (!ud || len <= 0 || len > MAX_GRO_SIZE)
        return;

    // Fast path: normal-sized frame, no segmentation needed
    if (len <= MAX_FRAME_LEN) {
        uaenet_queue_one(ud, data, len);
        return;
    }

    // Oversized packet — check if it's IPv4/TCP that we can segment
    if (len < ETH_HLEN + 20) // minimum Ethernet + IP header
        return;

    uae_u16 ethertype = (data[12] << 8) | data[13];
    if (ethertype != ETH_P_IP) {
        write_log(_T("UAENET: dropping oversized non-IPv4 packet (%d bytes, ethertype 0x%04x)\n"), len, ethertype);
        return;
    }

    const uae_u8 *ip_hdr = data + ETH_HLEN;
    int ip_version = (ip_hdr[0] >> 4) & 0xf;
    int ip_hdr_len = (ip_hdr[0] & 0xf) * 4;
    int ip_protocol = ip_hdr[9];

    if (ip_version != 4 || ip_hdr_len < 20 || ip_protocol != IP_PROTO_TCP) {
        write_log(_T("UAENET: dropping oversized non-TCP packet (%d bytes, proto %d)\n"), len, ip_protocol);
        return;
    }

    if (len < ETH_HLEN + ip_hdr_len + 20) // minimum TCP header
        return;

    const uae_u8 *tcp_hdr = ip_hdr + ip_hdr_len;
    int tcp_hdr_len = ((tcp_hdr[12] >> 4) & 0xf) * 4;
    if (tcp_hdr_len < 20)
        return;
    int total_hdr_len = ETH_HLEN + ip_hdr_len + tcp_hdr_len;
    int payload_len = len - total_hdr_len;
    int mss = MAX_MTU - ip_hdr_len - tcp_hdr_len;
    if (mss <= 0)
        return;

    uae_u32 orig_seq = ((uae_u32)tcp_hdr[4] << 24) | ((uae_u32)tcp_hdr[5] << 16) |
                        ((uae_u32)tcp_hdr[6] << 8) | tcp_hdr[7];
    uae_u16 orig_ip_id = (ip_hdr[4] << 8) | ip_hdr[5];
    uae_u8 orig_tcp_flags = tcp_hdr[13];
    const uae_u8 *payload = data + total_hdr_len;
    int offset = 0;
    int seg_num = 0;

    while (offset < payload_len) {
        int seg_payload = payload_len - offset;
        if (seg_payload > mss)
            seg_payload = mss;
        bool last_segment = (offset + seg_payload >= payload_len);

        uae_u8 seg[MAX_FRAME_LEN + 4];

        // Ethernet header — unchanged
        memcpy(seg, data, ETH_HLEN);

        // IP header — update length, ID, checksum
        memcpy(seg + ETH_HLEN, ip_hdr, ip_hdr_len);
        uae_u8 *sip = seg + ETH_HLEN;
        uae_u16 ip_total = ip_hdr_len + tcp_hdr_len + seg_payload;
        sip[2] = (ip_total >> 8) & 0xff;
        sip[3] = ip_total & 0xff;
        uae_u16 new_ip_id = orig_ip_id + seg_num;
        sip[4] = (new_ip_id >> 8) & 0xff;
        sip[5] = new_ip_id & 0xff;
        sip[10] = 0;
        sip[11] = 0;
        uae_u16 ip_ck = compute_ip_checksum(sip, ip_hdr_len);
        sip[10] = (ip_ck >> 8) & 0xff;
        sip[11] = ip_ck & 0xff;

        // TCP header — update seq, flags, checksum
        memcpy(seg + ETH_HLEN + ip_hdr_len, tcp_hdr, tcp_hdr_len);
        uae_u8 *stcp = seg + ETH_HLEN + ip_hdr_len;
        uae_u32 new_seq = orig_seq + offset;
        stcp[4] = (new_seq >> 24) & 0xff;
        stcp[5] = (new_seq >> 16) & 0xff;
        stcp[6] = (new_seq >> 8) & 0xff;
        stcp[7] = new_seq & 0xff;
        // FIN and PSH only on last segment
        if (!last_segment)
            stcp[13] = orig_tcp_flags & ~0x09; // clear FIN(0x01) and PSH(0x08)
        else
            stcp[13] = orig_tcp_flags;
        // Clear checksum, copy payload, recompute
        stcp[16] = 0;
        stcp[17] = 0;
        memcpy(seg + total_hdr_len, payload + offset, seg_payload);
        int tcp_total = tcp_hdr_len + seg_payload;
        uae_u16 tcp_ck = compute_tcp_checksum(sip, ip_hdr_len, stcp, tcp_total);
        stcp[16] = (tcp_ck >> 8) & 0xff;
        stcp[17] = tcp_ck & 0xff;

        uaenet_queue_one(ud, seg, ETH_HLEN + ip_total);

        offset += seg_payload;
        seg_num++;
    }
}
#endif

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

#ifdef WITH_UAENET_PCAP
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
#endif

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

#ifdef WITH_UAENET_PCAP
    if (ud->handle) {
        pcap_close(ud->handle);
        ud->handle = NULL;
    }
#endif
}

static struct netdriverdata nd[MAX_TOTAL_NET_DEVICES + 1];

#ifdef WITH_UAENET_PCAP
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
#endif

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
    if (ud->active || ud->active2
#ifdef WITH_UAENET_PCAP
        || ud->handle
#endif
    ) {
        write_log(_T("UAENET: Re-opening '%s', shutting down existing state\n"), ndd->name);
        ud->active = false;
        ud->active2 = false;
        if (ud->tid) {
            uae_wait_thread(&ud->tid);
            ud->tid = 0;
        }
#ifdef WITH_UAENET_PCAP
        if (ud->handle) {
            pcap_close(ud->handle);
            ud->handle = NULL;
        }
#endif
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

#ifdef WITH_UAENET_PCAP
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
#endif

    // Store the pointer to uaenet_data in the netdriverdata for later cleanup
    ndd->driverdata = ud;

    ud->gotfunc = gotfunc;
    ud->getfunc = getfunc;
    ud->userdata = userdata;
    ud->active = true;

#ifdef WITH_UAENET_PCAP
    // Start worker thread
    uae_start_thread(_T("uaenet_pcap"), uaenet_worker_thread, ud, &ud->tid);
    uae_sem_wait(&ud->sync_sem);
#endif

    write_log(_T("UAENET: '%s' open successful\n"), ndd->name);

    return 1;
}

// Send a packet
int uaenet_send(void *vsd, const void *data, int len)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud || !ud->active)
        return 0;
#ifdef WITH_UAENET_PCAP
    if (!ud->handle)
        return 0;
    return pcap_sendpacket(ud->handle, (const u_char *)data, len) == 0;
#else
    return 0;
#endif
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
    if (!ud || !ud->active)
        return;
#ifdef WITH_UAENET_PCAP
    if (!ud->handle)
        return;
#endif

    if (ud->getfunc) {
        uae_u8 pkt[4000];
        int len = sizeof pkt;
        if (ud->getfunc(ud->userdata, pkt, &len)) {
#ifdef WITH_UAENET_PCAP
            pcap_sendpacket(ud->handle, (const u_char *)pkt, len);
#endif
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

#endif // defined(WITH_UAENET_PCAP) || defined(WITH_UAENET_TAP)

// Pause ethernet operations
void ethernet_pause(int pause)
{
#if defined(WITH_UAENET_PCAP) || defined(WITH_UAENET_TAP)
    ethernet_paused = pause;
#endif
}

// Reset ethernet subsystem
void ethernet_reset(void)
{
#if defined(WITH_UAENET_PCAP) || defined(WITH_UAENET_TAP)
    ethernet_paused = 0;
#endif
}
