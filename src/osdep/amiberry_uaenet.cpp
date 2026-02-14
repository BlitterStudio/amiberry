/*
* UAE - The Un*x Amiga Emulator
*
* Amiberry uaenet emulation (libpcap and TAP backends, Linux/macOS)
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
#ifdef WITH_UAENET_TAP
#include <dirent.h>
#include <sys/stat.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/if_tun.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <poll.h>
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
#ifdef WITH_UAENET_TAP
static int uaenet_tap_worker_thread(void *arg);
static bool is_bridge_device(const char *name);
static int uaenet_tap_create(struct uaenet_data *ud, const char *bridge_name, const uae_u8 *mac);
static int uaenet_tap_open_existing(struct uaenet_data *ud, const char *tap_name, const uae_u8 *mac);
static void uaenet_tap_destroy(struct uaenet_data *ud);
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
    int backend_type;        // UAENET_PCAP or UAENET_TAP (actual backend in use)
#ifdef WITH_UAENET_PCAP
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
#endif
#ifdef WITH_UAENET_TAP
    int tap_fd;
    char tap_ifname[IFNAMSIZ];
    char bridge_name[IFNAMSIZ];
    bool owns_tap;           // true = Option A (we created it), false = Option B
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
// Shared by PCAP and TAP enumerations — both are called together via
// ethernet_enumerate() and both free functions reset it. Gates uaenet_open().
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
    ud->packetsinbuffer--;
    uae_sem_post(&ud->queue_sem);

    if (ud->active && ud->active2 && ud->gotfunc) {
        ud->gotfunc(ud->userdata, q->data, q->len);
    }

    xfree(q->data);
    xfree(q);
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

#ifdef WITH_UAENET_TAP
    if (ud->backend_type == UAENET_TAP && ud->tap_fd >= 0) {
        uaenet_tap_destroy(ud);
    }
#endif
#ifdef WITH_UAENET_PCAP
    if (ud->handle) {
        pcap_close(ud->handle);
        ud->handle = NULL;
    }
#endif
    ud->backend_type = 0;
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

    // If already open (re-init), shut down existing thread and handle first.
    // Use backend_type (not tap_fd) to detect stale TAP state, because tap_fd
    // starts as 0 from xcalloc (a valid fd number) and must be initialized to -1
    // AFTER this guard to avoid clobbering the old value before cleanup.
    if (ud->active || ud->active2 || ud->backend_type != 0
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
#ifdef WITH_UAENET_TAP
        if (ud->backend_type == UAENET_TAP && ud->tap_fd >= 0)
            uaenet_tap_destroy(ud);
#endif
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
        ud->backend_type = 0;
    }

    // Initialize tap_fd for fresh start. Must be AFTER the re-init guard so
    // stale fds from a previous session can be detected and cleaned up above.
    // xcalloc zeros tap_fd to 0, which is a valid fd (stdin), so -1 is required.
#ifdef WITH_UAENET_TAP
    ud->tap_fd = -1;
#endif

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

    int target_backend = ndd->type;

#ifdef WITH_UAENET_TAP
    if (target_backend == UAENET_TAP) {
        int ok;
        if (is_bridge_device(ndd->name)) {
            ok = uaenet_tap_create(ud, ndd->name, mac);
        } else {
            ok = uaenet_tap_open_existing(ud, ndd->name, mac);
        }
        if (ok) {
            ud->backend_type = UAENET_TAP;
        } else {
#ifdef WITH_UAENET_PCAP
            write_log(_T("UAENET: TAP failed for '%s', falling back to pcap\n"), ndd->name);
            target_backend = UAENET_PCAP;
#else
            write_log(_T("UAENET: TAP failed for '%s' and no pcap fallback available\n"), ndd->name);
#endif
        }
    }
#endif

#ifdef WITH_UAENET_PCAP
    if (target_backend == UAENET_PCAP) {
        // Always use promiscuous mode: on Linux bridge interfaces, the host
        // interface (br0) has a different MAC than the emulated device. Without
        // promiscuous mode, the kernel only delivers packets addressed to br0's
        // own MAC — packets to the Amiga's MAC are silently discarded before
        // reaching the pcap socket. The BPF filter already limits capture to
        // relevant packets (to our MAC, broadcast, multicast).
        ud->handle = pcap_open_live(ndd->name, 65536, 1, 10, ud->errbuf);
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
        ud->backend_type = UAENET_PCAP;
    }
#endif

    if (ud->backend_type == 0) {
        write_log(_T("UAENET: No backend available for '%s'\n"), ndd->name);
        uaenet_close_driver_internal(ud);
        return 0;
    }

    // Store the pointer to uaenet_data in the netdriverdata for later cleanup
    ndd->driverdata = ud;

    ud->gotfunc = gotfunc;
    ud->getfunc = getfunc;
    ud->userdata = userdata;
    ud->active = true;

    // Start appropriate worker thread
#ifdef WITH_UAENET_TAP
    if (ud->backend_type == UAENET_TAP) {
        uae_start_thread(_T("uaenet_tap"), uaenet_tap_worker_thread, ud, &ud->tid);
        uae_sem_wait(&ud->sync_sem);
    }
#endif
#ifdef WITH_UAENET_PCAP
    if (ud->backend_type == UAENET_PCAP) {
        uae_start_thread(_T("uaenet_pcap"), uaenet_worker_thread, ud, &ud->tid);
        uae_sem_wait(&ud->sync_sem);
    }
#endif

    write_log(_T("UAENET: '%s' open successful\n"), ndd->name);

    return 1;
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
    uae_sem_wait(&ud->queue_sem);
    struct uaenet_queue *q = ud->first;
    while (q) {
        bytes += q->len;
        q = q->next;
    }
    uae_sem_post(&ud->queue_sem);
    return bytes;
}

// Trigger packet transmission — called from ethernet_trigger() on the emulation thread
// when the A2065 has a packet to send. Retrieves the packet via getfunc and dispatches
// to the active backend (TAP write or pcap_sendpacket).
void uaenet_trigger(void *vsd)
{
    struct uaenet_data *ud = (struct uaenet_data*)vsd;
    if (!ud || !ud->active)
        return;
#ifdef WITH_UAENET_PCAP
    if (ud->backend_type == UAENET_PCAP && !ud->handle)
        return;
#endif
#ifdef WITH_UAENET_TAP
    if (ud->backend_type == UAENET_TAP && ud->tap_fd < 0)
        return;
#endif

    if (ud->getfunc) {
        uae_u8 pkt[4000];
        int len = sizeof pkt;
        if (ud->getfunc(ud->userdata, pkt, &len)) {
#ifdef WITH_UAENET_TAP
            if (ud->backend_type == UAENET_TAP) {
                ssize_t ret = write(ud->tap_fd, pkt, len);
                if (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                    write_log(_T("UAENET: TAP write error: %s\n"), strerror(errno));
            }
#endif
#ifdef WITH_UAENET_PCAP
            if (ud->backend_type == UAENET_PCAP) {
                pcap_sendpacket(ud->handle, (const u_char *)pkt, len);
            }
#endif
        }
    }
}

// Free memory allocated during enumeration
void uaenet_enumerate_free()
{
#if defined(WITH_UAENET_PCAP)
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

#ifdef WITH_UAENET_TAP
// --- TAP backend enumeration ---

static struct netdriverdata tap_nd[MAX_TOTAL_NET_DEVICES + 1];
static int tap_nd_count = 0;

static bool is_bridge_device(const char *name)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/bridge", name);
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool is_tap_device(const char *name)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/tun_flags", name);
    FILE *f = fopen(path, "r");
    if (!f)
        return false;
    unsigned int flags = 0;
    if (fscanf(f, "%x", &flags) != 1)
        flags = 0;
    fclose(f);
    return (flags & 0x0002) != 0;  // IFF_TAP
}

static long read_sysfs_long(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    long val = 0;
    if (fscanf(f, "%ld", &val) != 1)
        val = -1;
    fclose(f);
    return val;
}

void uaenet_tap_enumerate_free(void)
{
    for (int i = 0; i < tap_nd_count; i++) {
        xfree((void *)tap_nd[i].name);
        xfree((void *)tap_nd[i].desc);
    }
    memset(tap_nd, 0, sizeof(tap_nd));
    tap_nd_count = 0;
    enumerated = 0;
}

struct netdriverdata *uaenet_tap_enumerate(const TCHAR *name)
{
    // If already enumerated and doing a single-device lookup, search the existing
    // array. Re-enumerating would free the tap_nd[] name/desc strings, invalidating
    // pointers cached in the global ndd[] array from target_ethernet_enumerate().
    if (name != NULL && name[0] != '\0' && tap_nd_count > 0) {
        for (int i = 0; i < tap_nd_count; i++) {
            if (tap_nd[i].active && tap_nd[i].name && !_tcsicmp(name, tap_nd[i].name))
                return &tap_nd[i];
        }
        return NULL;  // not found — don't return a wrong device
    }

    uaenet_tap_enumerate_free();

    DIR *dir = opendir("/sys/class/net");
    if (!dir) {
        write_log(_T("UAENET: TAP enumerate: cannot open /sys/class/net\n"));
        enumerated = 1;
        return tap_nd;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && tap_nd_count < MAX_TOTAL_NET_DEVICES) {
        if (ent->d_name[0] == '.')
            continue;

        bool bridge = is_bridge_device(ent->d_name);
        bool tap = is_tap_device(ent->d_name);
        if (!bridge && !tap)
            continue;

        // Single-device lookup: skip non-matching entries
        if (name != NULL && name[0] != '\0') {
            if (_tcsicmp(name, ent->d_name) != 0)
                continue;
        }

        int j = tap_nd_count;
        tap_nd[j].name = my_strdup(ent->d_name);
        if (bridge) {
            char desc[256];
            snprintf(desc, sizeof(desc), "TAP bridge: %s", ent->d_name);
            tap_nd[j].desc = my_strdup(desc);
        } else {
            char desc[256];
            snprintf(desc, sizeof(desc), "TAP device: %s", ent->d_name);
            tap_nd[j].desc = my_strdup(desc);
        }

        tap_nd[j].type = UAENET_TAP;
        tap_nd[j].active = 1;
        tap_nd[j].driverdata = nullptr;

        // Read MTU from sysfs
        char mtu_path[256];
        snprintf(mtu_path, sizeof(mtu_path), "/sys/class/net/%s/mtu", ent->d_name);
        long mtu = read_sysfs_long(mtu_path);
        tap_nd[j].mtu = (mtu > 0) ? (int)mtu : 1500;

        tap_nd_count++;
        if (name != NULL)
            break;
    }

    closedir(dir);
    enumerated = 1;
    return tap_nd;
}

// --- TAP backend open/close/worker ---

// Option A: Create a new TAP device and add it to the specified bridge.
// Returns 1 on success, 0 on failure. On failure, partially-created state is cleaned up.
static int uaenet_tap_create(struct uaenet_data *ud, const char *bridge_name, const uae_u8 *mac)
{
    int fd = open("/dev/net/tun", O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        write_log(_T("UAENET: TAP: cannot open /dev/net/tun: %s\n"), strerror(errno));
        return 0;
    }

    struct ifreq ifr = {};
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    // "amiberry%d" is a template — the kernel's tun driver assigns the next
    // available number (amiberry0, amiberry1, ...) and writes the actual name
    // back into ifr.ifr_name.
    strncpy(ifr.ifr_name, "amiberry%d", IFNAMSIZ - 1);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        write_log(_T("UAENET: TAP: TUNSETIFF failed: %s\n"), strerror(errno));
        close(fd);
        return 0;
    }

    write_log(_T("UAENET: TAP: created interface '%s'\n"), ifr.ifr_name);

    // Store state early so cleanup can find it
    ud->tap_fd = fd;
    strncpy(ud->tap_ifname, ifr.ifr_name, IFNAMSIZ - 1);
    ud->tap_ifname[IFNAMSIZ - 1] = '\0';
    strncpy(ud->bridge_name, bridge_name, IFNAMSIZ - 1);
    ud->bridge_name[IFNAMSIZ - 1] = '\0';
    ud->owns_tap = true;

    // Declare variables used after goto targets up front — C++ forbids
    // jumping over initializations.
    char stp_path[256];
    long stp_state;
    struct ifreq ifr_br;
    struct ifreq ifr_up;

    // Control socket for ioctls (SIOCBRADDIF, SIOCSIFFLAGS)
    int ctl = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctl < 0) {
        write_log(_T("UAENET: TAP: cannot create control socket: %s\n"), strerror(errno));
        goto fail_fd;
    }

    // Do NOT set the TAP interface MAC to the Amiga MAC. When a bridge port's
    // link-layer address matches, the bridge creates a "permanent local" FDB entry
    // and delivers frames to that MAC to the bridge's own network stack instead of
    // forwarding them to the port. Leave the TAP with its default random MAC and
    // let the bridge learn the Amiga MAC from outgoing traffic.

    // Check bridge STP state and warn if enabled
    snprintf(stp_path, sizeof(stp_path), "/sys/class/net/%s/bridge/stp_state", bridge_name);
    stp_state = read_sysfs_long(stp_path);
    if (stp_state > 0) {
        char fwd_path[256];
        snprintf(fwd_path, sizeof(fwd_path), "/sys/class/net/%s/bridge/forward_delay", bridge_name);
        long fwd_delay_jiffies = read_sysfs_long(fwd_path);
        // sysfs forward_delay is in jiffies (centiseconds); STP transitions
        // through listening and learning states, so total delay is 2x forward_delay.
        long total_delay = (fwd_delay_jiffies > 0) ? (fwd_delay_jiffies * 2) / 100 : 30;
        write_log(_T("UAENET: WARNING: STP is enabled on bridge '%s'. The TAP port will take\n"
                      "  approximately %ld seconds to start forwarding traffic. To avoid this delay,\n"
                      "  pre-create the TAP device before starting Amiberry:\n"
                      "    ip tuntap add dev tap0 mode tap user amiberry\n"
                      "    ip link set tap0 master %s\n"
                      "    ip link set tap0 up\n"
                      "  Then configure: a2065=tap0\n"),
                  bridge_name, total_delay, bridge_name);
    }

    // Add TAP to bridge via SIOCBRADDIF
    memset(&ifr_br, 0, sizeof(ifr_br));
    strncpy(ifr_br.ifr_name, bridge_name, IFNAMSIZ - 1);
    ifr_br.ifr_ifindex = if_nametoindex(ud->tap_ifname);
    if (ifr_br.ifr_ifindex == 0) {
        write_log(_T("UAENET: TAP: if_nametoindex failed for '%s': %s\n"),
                  ud->tap_ifname, strerror(errno));
        goto fail_ctl;
    }
    if (ioctl(ctl, SIOCBRADDIF, &ifr_br) < 0) {
        write_log(_T("UAENET: TAP: SIOCBRADDIF failed (add '%s' to '%s'): %s\n"),
                  ud->tap_ifname, bridge_name, strerror(errno));
        goto fail_ctl;
    }
    write_log(_T("UAENET: TAP: added '%s' to bridge '%s'\n"), ud->tap_ifname, bridge_name);

    // Bring TAP interface up
    memset(&ifr_up, 0, sizeof(ifr_up));
    strncpy(ifr_up.ifr_name, ud->tap_ifname, IFNAMSIZ - 1);
    if (ioctl(ctl, SIOCGIFFLAGS, &ifr_up) < 0) {
        write_log(_T("UAENET: TAP: SIOCGIFFLAGS failed for '%s': %s\n"),
                  ud->tap_ifname, strerror(errno));
        // Non-fatal: try to set UP anyway
    }
    ifr_up.ifr_flags |= IFF_UP;
    if (ioctl(ctl, SIOCSIFFLAGS, &ifr_up) < 0) {
        write_log(_T("UAENET: TAP: SIOCSIFFLAGS (UP) failed for '%s': %s\n"),
                  ud->tap_ifname, strerror(errno));
        // Remove from bridge before failing
        ioctl(ctl, SIOCBRDELIF, &ifr_br);
        goto fail_ctl;
    }

    close(ctl);
    write_log(_T("UAENET: TAP: Opening '%s' via TAP backend (%s -> %s)\n"),
              bridge_name, ud->tap_ifname, bridge_name);
    return 1;

fail_ctl:
    close(ctl);
fail_fd:
    close(fd);
    ud->tap_fd = -1;
    ud->owns_tap = false;
    ud->tap_ifname[0] = '\0';
    ud->bridge_name[0] = '\0';
    return 0;
}

// Option B: Open an existing (pre-created) TAP device by name.
// Returns 1 on success, 0 on failure.
static int uaenet_tap_open_existing(struct uaenet_data *ud, const char *tap_name, const uae_u8 *mac)
{
    int fd = open("/dev/net/tun", O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        write_log(_T("UAENET: TAP: cannot open /dev/net/tun: %s\n"), strerror(errno));
        return 0;
    }

    struct ifreq ifr = {};
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, tap_name, IFNAMSIZ - 1);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        write_log(_T("UAENET: TAP: TUNSETIFF failed for '%s': %s\n"), tap_name, strerror(errno));
        close(fd);
        return 0;
    }

    ud->tap_fd = fd;
    strncpy(ud->tap_ifname, ifr.ifr_name, IFNAMSIZ - 1);
    ud->tap_ifname[IFNAMSIZ - 1] = '\0';
    ud->bridge_name[0] = '\0';
    ud->owns_tap = false;

    // Do NOT set the TAP interface MAC — same reason as uaenet_tap_create().
    // The bridge learns the Amiga MAC from traffic; setting it on the interface
    // creates a "permanent local" FDB entry that prevents forwarding.

    write_log(_T("UAENET: TAP: Opening '%s' via TAP backend (pre-created)\n"), tap_name);
    return 1;
}

// Destroy a TAP device. For Option A (owns_tap): bring down, remove from bridge,
// close fd (destroys device). For Option B (!owns_tap): just close fd.
static void uaenet_tap_destroy(struct uaenet_data *ud)
{
    if (!ud || ud->tap_fd < 0)
        return;

    if (ud->owns_tap) {
        int ctl = socket(AF_INET, SOCK_DGRAM, 0);
        if (ctl >= 0) {
            // Bring interface down
            struct ifreq ifr_down = {};
            strncpy(ifr_down.ifr_name, ud->tap_ifname, IFNAMSIZ - 1);
            if (ioctl(ctl, SIOCGIFFLAGS, &ifr_down) == 0) {
                ifr_down.ifr_flags &= ~IFF_UP;
                ioctl(ctl, SIOCSIFFLAGS, &ifr_down);
            }

            // Remove from bridge
            if (ud->bridge_name[0]) {
                struct ifreq ifr_br = {};
                strncpy(ifr_br.ifr_name, ud->bridge_name, IFNAMSIZ - 1);
                ifr_br.ifr_ifindex = if_nametoindex(ud->tap_ifname);
                if (ifr_br.ifr_ifindex > 0)
                    ioctl(ctl, SIOCBRDELIF, &ifr_br);
            }
            close(ctl);
        }
        write_log(_T("UAENET: TAP: destroyed '%s'\n"), ud->tap_ifname);
    } else {
        write_log(_T("UAENET: TAP: closed '%s' (pre-created, not destroying)\n"), ud->tap_ifname);
    }

    close(ud->tap_fd);
    ud->tap_fd = -1;
}

// TAP worker thread — reads packets from the TAP fd and queues them for the
// emulation thread. No GRO segmentation needed: the kernel delivers individual
// frames (segmentation happens before the tap fd).
static int uaenet_tap_worker_thread(void *arg)
{
    struct uaenet_data *ud = (struct uaenet_data *)arg;
    ud->active2 = true;
    uae_sem_post(&ud->sync_sem);

    uae_u8 buf[MAX_FRAME_LEN + 4];

    while (ud->active2) {
        struct pollfd pfd = { .fd = ud->tap_fd, .events = POLLIN };
        int ret = poll(&pfd, 1, 10);  // 10ms timeout — balances idle CPU vs latency
        if (ret > 0) {
            if (pfd.revents & (POLLERR | POLLHUP)) {
                write_log(_T("UAENET: TAP: poll error/hangup on '%s', stopping worker\n"),
                          ud->tap_ifname);
                break;
            }
            if (pfd.revents & POLLIN) {
                int len = read(ud->tap_fd, buf, sizeof(buf));
                if (len > 0) {
                    uaenet_queue_one(ud, buf, len);
                }
                // EAGAIN/EWOULDBLOCK from O_NONBLOCK is harmless — just retry next poll
            }
        }
    }

    ud->active2 = false;
    return 0;
}
#endif

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
