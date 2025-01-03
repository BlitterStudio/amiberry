/*
* UAE - The Un*x Amiga Emulator
*
* SanaII emulation
*
* partially based on code from 3c589 PCMCIA driver by Neil Cafferke
*
* Copyright 2007 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "execlib.h"
#include "native2amiga.h"
#include "blkdev.h"
#include "uae.h"
#include "sana2.h"
#if defined(_WIN32) && defined(WITH_UAENET_PCAP)
#include "win32_uaenet.h"
#else
#include "ethernet.h"
#endif
#include "execio.h"
#include "debug.h"
#include "devices.h"

#ifdef SANA2

static void uaenet_gotdata (void *dev, const uae_u8 *data, int len);
static int uaenet_getdata (void *dev, uae_u8 *d, int *len);

#define SANA2NAME _T("uaenet.device")

#define MAX_ASYNC_REQUESTS 200
#define MAX_OPEN_DEVICES 20

#define DELAYED_DROPPED_PACKET_FRAMES 100

#define S2_START                (CMD_NONSTD)	// 9
#define S2_DEVICEQUERY          (S2_START+ 0)	// 9
#define S2_GETSTATIONADDRESS    (S2_START+ 1)	// 10
#define S2_CONFIGINTERFACE      (S2_START+ 2)	// 11
#define S2_ADDMULTICASTADDRESS  (S2_START+ 5)	// 14
#define S2_DELMULTICASTADDRESS  (S2_START+ 6)	// 15
#define S2_MULTICAST            (S2_START+ 7)	// 16
#define S2_BROADCAST            (S2_START+ 8)	// 17
#define S2_TRACKTYPE            (S2_START+ 9)	// 18
#define S2_UNTRACKTYPE          (S2_START+10)	// 19
#define S2_GETTYPESTATS         (S2_START+11)	// 20
#define S2_GETSPECIALSTATS      (S2_START+12)	// 21
#define S2_GETGLOBALSTATS       (S2_START+13)	// 22
#define S2_ONEVENT              (S2_START+14)	// 23
#define S2_READORPHAN           (S2_START+15)	// 24
#define S2_ONLINE               (S2_START+16)	// 25
#define S2_OFFLINE              (S2_START+17)	// 26
#define S2_ADDMULTICASTADDRESSES  0xC000
#define S2_DELMULTICASTADDRESSES  0xC001
#define S2_GETPEERADDRESS         0xC002
#define S2_GETDNSADDRESS          0xC003
#define S2_GETEXTENDEDGLOBALSTATS 0xC004
#define S2_CONNECT                0xC005
#define S2_DISCONNECT             0xC006
#define S2_SAMPLE_THROUGHPUT      0xC007

#define S2WireType_Ethernet             1
#define S2WireType_IEEE802              6

#define SANA2_MAX_ADDR_BITS     (128)
#define SANA2_MAX_ADDR_BYTES    ((SANA2_MAX_ADDR_BITS+7)/8)
#define ADDR_SIZE 6
#define ETH_HEADER_SIZE (ADDR_SIZE + ADDR_SIZE + 2)

#define S2ERR_NO_ERROR          0       /* peachy-keen                  */
#define S2ERR_NO_RESOURCES      1       /* resource allocation failure  */
#define S2ERR_BAD_ARGUMENT      3       /* garbage somewhere            */
#define S2ERR_BAD_STATE         4       /* inappropriate state          */
#define S2ERR_BAD_ADDRESS       5       /* who?                         */
#define S2ERR_MTU_EXCEEDED      6       /* too much to chew             */
#define S2ERR_NOT_SUPPORTED     8       /* hardware can't support cmd   */
#define S2ERR_SOFTWARE          9       /* software error detected      */
#define S2ERR_OUTOFSERVICE      10      /* driver is OFFLINE            */
#define S2ERR_TX_FAILURE        11      /* Transmission attempt failed  */

#define S2WERR_GENERIC_ERROR    0       /* no specific info available   */
#define S2WERR_NOT_CONFIGURED   1       /* unit not configured          */
#define S2WERR_UNIT_ONLINE      2       /* unit is currently online     */
#define S2WERR_UNIT_OFFLINE     3       /* unit is currently offline    */
#define S2WERR_ALREADY_TRACKED  4       /* protocol already tracked     */
#define S2WERR_NOT_TRACKED      5       /* protocol not tracked         */
#define S2WERR_BUFF_ERROR       6       /* buff mgt func returned error */
#define S2WERR_SRC_ADDRESS      7       /* source address problem       */
#define S2WERR_DST_ADDRESS      8       /* destination address problem  */
#define S2WERR_BAD_BROADCAST    9       /* broadcast address problem    */
#define S2WERR_BAD_MULTICAST    10      /* multicast address problem    */
#define S2WERR_MULTICAST_FULL   11      /* multicast address list full  */
#define S2WERR_BAD_EVENT        12      /* unsupported event class      */
#define S2WERR_BAD_STATDATA     13      /* statdata failed sanity check */
#define S2WERR_IS_CONFIGURED    15      /* attempt to config twice      */
#define S2WERR_NULL_POINTER     16      /* null pointer detected        */
#define S2WERR_TOO_MANY_RETIRES 17      /* tx failed - too many retries */
#define S2WERR_RCVREL_HDW_ERR   18      /* Driver fixable HW error      */

#define S2EVENT_ERROR           (1L<<0) /* error catch all              */
#define S2EVENT_TX              (1L<<1) /* transmitter error catch all  */
#define S2EVENT_RX              (1L<<2) /* receiver error catch all     */
#define S2EVENT_ONLINE          (1L<<3) /* unit is in service           */
#define S2EVENT_OFFLINE         (1L<<4) /* unit is not in service       */
#define S2EVENT_BUFF            (1L<<5) /* buff mgt function error      */
#define S2EVENT_HARDWARE        (1L<<6) /* hardware error catch all     */
#define S2EVENT_SOFTWARE        (1L<<7) /* software error catch all     */

#define KNOWN_EVENTS (S2EVENT_ERROR|S2EVENT_TX|S2EVENT_RX|S2EVENT_ONLINE|\
	S2EVENT_OFFLINE|S2EVENT_BUFF|S2EVENT_HARDWARE|S2EVENT_SOFTWARE)

#define SANA2OPB_MINE   0
#define SANA2OPF_MINE   (1<<SANA2OPB_MINE)
#define SANA2OPB_PROM   1
#define SANA2OPF_PROM   (1<<SANA2OPB_PROM)
#define SANA2IOB_RAW    7 
#define SANA2IOF_RAW    (1<<SANA2IOB_RAW)
#define SANA2IOB_BCAST  6
#define SANA2IOF_BCAST  (1<<SANA2IOB_BCAST)
#define SANA2IOB_MCAST  5
#define SANA2IOF_MCAST  (1<<SANA2IOB_MCAST)

#define S2_Dummy                static_cast<uint32_t>(TAG_USER + 0xB0000)
#define S2_CopyToBuff           (S2_Dummy + 1)
#define S2_CopyFromBuff         (S2_Dummy + 2)
#define S2_PacketFilter         (S2_Dummy + 3)
#define S2_CopyToBuff16         (S2_Dummy + 4)
#define S2_CopyFromBuff16       (S2_Dummy + 5)
#define S2_CopyToBuff32         (S2_Dummy + 6)
#define S2_CopyFromBuff32       (S2_Dummy + 7)
#define S2_DMACopyToBuff32      (S2_Dummy + 8)
#define S2_DMACopyFromBuff32    (S2_Dummy + 9)
#define S2_DMACopyToBuff64		(S2_Dummy + 10)
#define S2_DMACopyFromBuff64	(S2_Dummy + 11)
#define S2_Log					(S2_Dummy + 12)

#define SANA2_IOREQSIZE (32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4 + 4 + 4 + 4)

struct s2packet {
	struct s2packet *next;
	uae_u8 *data;
	int len;
	int drop_start;
	int drop_count;
};

static int uaenet_int_late;

static uaecptr timerdevname;

static uaecptr ROM_netdev_resname = 0,
	ROM_netdev_resid = 0,
	ROM_netdev_init = 0;

static const TCHAR *getdevname (void)
{
	return _T("uaenet.device");
}

struct asyncreq {
	struct asyncreq *next;
	uaecptr arequest;
	uae_u8 *request;
	struct s2packet *s2p;
	int ready;
};

struct mcast {
	struct mcast *next;
	uae_u64 start;
	uae_u64 end;
	int cnt;
};

struct s2devstruct {
	int unit, opencnt, exclusive, promiscuous;
	struct asyncreq *ar;
	struct asyncreq *s2p;
	struct mcast *mc;
	smp_comm_pipe requests;
	int thread_running;
	uae_sem_t sync_sem;
	void *sysdata;
	uae_u32 packetsreceived;
	uae_u32 packetssent;
	uae_u32 baddata;
	uae_u32 overruns;
	uae_u32 unknowntypesreceived;
	uae_u32 reconfigurations;
	uae_u32 online_micro;
	uae_u32 online_secs;
	int configured;
	int adapter;
	int online;
	struct netdriverdata *td;
	struct s2packet *readqueue;
	uae_u8 mac[ADDR_SIZE];
	int flush_timeout;
	int flush_timeout_cnt;
	TrapContext *ctx;
};

#define FLUSH_TIMEOUT 20

struct priv_s2devstruct {
	int inuse;
	int unit;
	int flags; /* OpenDevice() */
	int promiscuous;

	uae_u8 tracks[65536];
	int trackcnt;
	uae_u32 packetsreceived;
	uae_u32 packetssent;
	uae_u32 bytessent;
	uae_u32 bytesreceived;
	uae_u32 packetsdropped;

	uaecptr copytobuff;
	uaecptr copyfrombuff;
	uaecptr packetfilter;
	uaecptr tempbuf;

	uaecptr timerbase;

	struct netdriverdata *td;

	int tmp;
};

static struct netdriverdata *td[MAX_TOTAL_NET_DEVICES + 1];
static struct s2devstruct devst[MAX_TOTAL_NET_DEVICES];
static struct priv_s2devstruct pdevst[MAX_OPEN_DEVICES];
static uae_u32 nscmd_cmd;
static uae_sem_t change_sem, async_sem, pipe_sem;

static struct s2devstruct *gets2devstruct (int unit)
{
	if (unit >= MAX_TOTAL_NET_DEVICES || unit < 0)
		return 0;
	return &devst[unit];
}

static struct priv_s2devstruct *getps2devstruct(TrapContext *ctx, uae_u8 *iobuf, uaecptr request)
{
	int idx;
	
	if (iobuf) {
		idx = get_long_host(iobuf + 24);
	} else {
		idx = trap_get_long(ctx, request + 24);
	}
	if (idx < 0 || idx >= MAX_OPEN_DEVICES || pdevst[idx].inuse == 0) {
		write_log (_T("%s: corrupt iorequest %08X %d\n"), SANA2NAME, request, idx);
		return 0;
	}
	return &pdevst[idx];
}

static int dev_thread (void *devs);
static int start_thread (struct s2devstruct *dev)
{
	if (dev->thread_running)
		return 1;
	init_comm_pipe (&dev->requests, 100, 1);
	uae_sem_init(&dev->sync_sem, 0, 0);
	uae_start_thread (SANA2NAME, dev_thread, dev, NULL);
	uae_sem_wait (&dev->sync_sem);
	return dev->thread_running;
}

static uae_u32 REGPARAM2 dev_close_2 (TrapContext *ctx)
{
	uae_u32 request = trap_get_areg(ctx, 1);
	struct priv_s2devstruct *pdev = getps2devstruct(ctx, NULL, request);
	struct s2devstruct *dev;

	if (!pdev) {
		write_log (_T("%s close with unknown request %08X!?\n"), SANA2NAME, request);
		return 0;
	}
	dev = gets2devstruct (pdev->unit);
	if (!dev) {
		write_log (_T("%s:%d close with unknown request %08X!?\n"), SANA2NAME, pdev->unit, request);
		return 0;
	}
	if (log_net)
		write_log (_T("%s:%d close, open=%d req=%08X\n"), SANA2NAME, pdev->unit, dev->opencnt, request);
	trap_put_long(ctx, request + 24, 0);
	dev->opencnt--;
	pdev->inuse = 0;
	if (!dev->opencnt) {
		dev->exclusive = 0;
		if (pdev->tempbuf) {
			trap_call_add_areg(ctx, 1, pdev->tempbuf);
			trap_call_add_dreg(ctx, 0, pdev->td->mtu + ETH_HEADER_SIZE + 2);
			trap_call_lib(ctx, trap_get_long(ctx, 4), -0xD2); /* FreeMem */
			pdev->tempbuf = 0;
		}
		ethernet_close (pdev->td, dev->sysdata);
		xfree (dev->sysdata);
		dev->sysdata = NULL;
		uae_sem_wait(&pipe_sem);
		write_comm_pipe_pvoid(&dev->requests, NULL, 0);
		write_comm_pipe_pvoid(&dev->requests, NULL, 0);
		write_comm_pipe_u32 (&dev->requests, 0, 1);
		uae_sem_post(&pipe_sem);
		write_log (_T("%s: opencnt == 0, all instances closed\n"), SANA2NAME);
	}
	trap_put_word(ctx, trap_get_areg(ctx, 6) + 32, trap_get_word(ctx, trap_get_areg(ctx, 6) + 32) - 1);
	return 0;
}

static uae_u32 REGPARAM2 dev_close (TrapContext *ctx)
{
	return dev_close_2(ctx);
}
static uae_u32 REGPARAM2 diskdev_close (TrapContext *ctx)
{
	return dev_close_2(ctx);
}

static int openfail (TrapContext *ctx, uaecptr ioreq, int error)
{
	trap_put_long(ctx, ioreq + 20, -1);
	trap_put_byte(ctx, ioreq + 31, error);
	trap_put_long(ctx, ioreq + 32, 0); /* io_device */
	if (log_net)
		write_log (_T("-> failed with error %d\n"), error);
	return (uae_u32)-1;
}

static uaecptr uaenet_worker;
static int uaenet_signal_state;

static void uaenet_signal_done(int cmd)
{
	uaenet_signal_state = 1;
}

static void uaenet_int(void)
{
	if (uaenet_worker && uaenet_signal_state) {
		uaenet_signal_state = 0;
		uae_Signal_with_Func(uaenet_worker, 0x100, uaenet_signal_done);
	}
}

static uae_u32 REGPARAM2 uaenet_int_handler (TrapContext *ctx);
static int irq_init;
static int initint (TrapContext *ctx)
{
	uae_u32 tmp1, tmp2;
	uaecptr p;

	if (irq_init)
		return 1;

	trap_call_add_dreg(ctx, 0, 92 + 4096);
	trap_call_add_dreg(ctx, 1, 65536 + 1);
	p = trap_call_lib(ctx, trap_get_long(ctx, 4), -0xC6); /* AllocMem */
	if (!p)
		return 0;

	tmp1 = ds(_T("uaenet worker"));
	trap_put_byte(ctx, p + 8, 1); // NT_TASK
	trap_put_byte(ctx, p + 9, 20); // Priority
	trap_put_long(ctx, p + 10, tmp1); // name
	trap_put_long(ctx, p + 58, p + 92); // SPLower
	trap_put_long(ctx, p + 62, p + 92 + 4096); // SPUpper
	trap_put_long(ctx, p + 54, p + 92 + 4096); // SPReg

	tmp1 = here ();
	dl(0x2c780004); // move.l 4.w,a6
	dw(0x203c);		// move.l #$100,d0
	dl(0x00000100);
	dl(0x4eaefec2); // jsr -$13e(a6)
	tmp2 = here();
	calltrap (deftrap2 (uaenet_int_handler, TRAPFLAG_EXTRA_STACK, _T("uaenet_int_handler")));
	dw(0x4a80); // tst.l d0
	dw(0x6600 | (tmp2 - here() - 2) & 0xff); // bne.s tmp2
	dw(0x6000 | (tmp1 - here() - 2) & 0xff); // bra.s tmp1

	trap_call_add_areg(ctx, 1, p);
	trap_call_add_areg(ctx, 2, tmp1);
	trap_call_add_areg(ctx, 3, 0);
	uaenet_worker = trap_call_lib(ctx, trap_get_long(ctx, 4), -0x11a); // AddTask

	irq_init = 1;
	return 1;
}

static uae_u32 REGPARAM2 dev_open_2 (TrapContext *ctx)
{
	uaecptr ioreq = trap_get_areg(ctx, 1);
	uae_u32 unit = trap_get_dreg(ctx, 0);
	uae_u32 flags = trap_get_dreg(ctx, 1);
	uaecptr buffermgmt;
	struct s2devstruct *dev = gets2devstruct (unit);
	struct priv_s2devstruct *pdev = 0;
	int i;
	uaecptr tagp, tagpnext;

	if (!dev)
		return openfail(ctx, ioreq, IOERR_OPENFAIL);
	if (!initint(ctx))
		return openfail(ctx, ioreq, IOERR_SELFTEST);
	if (log_net)
		write_log (_T("opening %s:%d opencnt=%d ioreq=%08X\n"), SANA2NAME, unit, dev->opencnt, ioreq);
	if (trap_get_word(ctx, ioreq + 0x12) < IOSTDREQ_SIZE)
		return openfail(ctx, ioreq, IOERR_BADLENGTH);
	if ((flags & SANA2OPF_PROM) && dev->opencnt > 0)
		return openfail(ctx, ioreq, IOERR_UNITBUSY);

	for (i = 0; i < MAX_OPEN_DEVICES; i++) {
		pdev = &pdevst[i];
		if (pdev->inuse == 0)
			break;
	}
	if (i == MAX_OPEN_DEVICES)
		return openfail(ctx, ioreq, IOERR_UNITBUSY);

	trap_put_longt(ctx, ioreq + 24, pdev - pdevst);
	pdev->unit = unit;
	pdev->flags = flags;
	pdev->inuse = 1;
	pdev->td = td ? td[unit] : NULL;
	pdev->promiscuous = (flags & SANA2OPF_PROM) ? 1 : 0;

	if (pdev->td == NULL || pdev->td->active == 0)
		return openfail(ctx, ioreq, IOERR_OPENFAIL);

	if (dev->opencnt == 0) {
		dev->unit = unit;
		dev->sysdata = xcalloc (uae_u8, ethernet_getdatalenght (pdev->td));
		if (!ethernet_open (pdev->td, dev->sysdata, dev, uaenet_gotdata, uaenet_getdata, pdev->promiscuous, NULL)) {
			xfree (dev->sysdata);
			dev->sysdata = NULL;
			return openfail(ctx, ioreq, IOERR_OPENFAIL);
		}
		write_log (_T("%s: initializing unit %d\n"), getdevname (), unit);
		dev->td = pdev->td;
		dev->adapter = pdev->td ? pdev->td->active : 0;
		if (dev->adapter) {
			dev->online = 1;
			dev->configured = 1;
		}
		start_thread (dev);
	}

	if (kickstart_version >= 36) {
		trap_call_add_areg(ctx, 0, trap_get_long(ctx, 4) + 350);
		trap_call_add_areg(ctx, 1, timerdevname);
		pdev->timerbase = trap_call_lib(ctx, trap_get_long(ctx, 4), -0x114); /* FindName('timer.device') */
	}

	pdev->copyfrombuff = pdev->copytobuff = pdev->packetfilter = 0;
	pdev->tempbuf = 0;
	if (trap_get_word(ctx, ioreq + 0x12) >= SANA2_IOREQSIZE) {
		buffermgmt = trap_get_long(ctx, ioreq + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4 + 4 + 4);
		tagpnext = buffermgmt;
		while (tagpnext) {
			uae_u32 tag = trap_get_long(ctx, tagpnext);
			uae_u32 val = trap_get_long(ctx, tagpnext + 4);
			if (log_net)
				write_log(_T("%s:%d TAG %08x %08x\n"), getdevname(), unit, tag, val);
			tagp = tagpnext;
			tagpnext += 8;
			switch (tag)
			{
			case TAG_DONE:
				tagpnext = 0;
				break;
			case TAG_IGNORE:
				break;
			case TAG_MORE:
				tagpnext = val;
				break;
			case TAG_SKIP:
				tagpnext = tagp + val * 8;
				break;
			case S2_CopyToBuff:
				pdev->copytobuff = val;
				break;
			case S2_CopyFromBuff:
				pdev->copyfrombuff = val;
				break;
			case S2_PacketFilter:
				pdev->packetfilter = val;
				break;
			}
		}
		trap_call_add_dreg(ctx, 0, dev->td->mtu + ETH_HEADER_SIZE + 2);
		trap_call_add_dreg(ctx, 1, 65536 + 1);
		pdev->tempbuf = trap_call_lib(ctx, trap_get_long(ctx, 4), -0xC6); /* AllocMem */
		if (log_net) {
			write_log(_T("%s:%d CTB=%08x CFB=%08x PF=%08x TEMP=%08x\n"),
				getdevname(), unit, pdev->copytobuff, pdev->copyfrombuff, pdev->packetfilter, pdev->tempbuf);
		}
		if (!pdev->tempbuf) {
			if (dev->opencnt == 0) {
				ethernet_close (pdev->td, dev->sysdata);
				xfree (dev->sysdata);
				dev->sysdata = NULL;
			}
			return openfail(ctx, ioreq, S2ERR_BAD_ARGUMENT);
		}
		/* buffermanagement */
		trap_put_long(ctx, ioreq + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4 + 4 + 4, pdev->tempbuf);
	}
	dev->exclusive = (flags & SANA2OPF_MINE) != 0;
	dev->opencnt++;
	trap_put_word(ctx, trap_get_areg(ctx, 6) + 32, trap_get_word(ctx, trap_get_areg(ctx, 6) + 32) + 1);
	trap_put_byte(ctx, ioreq + 31, 0);
	trap_put_byte(ctx, ioreq + 8, 7); // NT_REPLYMSG
	return 0;
}

static uae_u32 REGPARAM2 dev_open (TrapContext *context)
{
	return dev_open_2 (context);
}
static uae_u32 REGPARAM2 dev_expunge (TrapContext *context)
{
	return 0;
}
static uae_u32 REGPARAM2 diskdev_expunge (TrapContext *context)
{
	return 0;
}

static void freepacket (struct s2packet *s2p)
{
	xfree (s2p->data);
	xfree (s2p);
}

static void add_async_packet (struct s2devstruct *dev, struct s2packet *s2p, uae_u8 *request, uaecptr arequest)
{
	struct asyncreq *ar, *ar2;

	ar = xcalloc (struct asyncreq, 1);
	ar->s2p = s2p;
	if (!dev->s2p) {
		dev->s2p = ar;
	} else {
		ar2 = dev->s2p;
		while (ar2->next)
			ar2 = ar2->next;
		ar2->next = ar;
	}
	ar->arequest = arequest;
	ar->request = request;
}

static void rem_async_packet (struct s2devstruct *dev, uaecptr arequest)
{
	struct asyncreq *ar, *prevar;

	uae_sem_wait (&async_sem);
	ar = dev->s2p;
	prevar = NULL;
	while (ar) {
		if (ar->arequest == arequest) {
			if (prevar == NULL)
				dev->s2p = ar->next;
			else
				prevar->next = ar->next;
			uae_sem_post (&async_sem);
			freepacket (ar->s2p);
			xfree(ar->request);
			xfree(ar);
			return;
		}
		prevar = ar;
		ar = ar->next;
	}
	uae_sem_post (&async_sem);
}

static struct asyncreq *get_async_request (struct s2devstruct *dev, uaecptr arequest, int ready)
{
	struct asyncreq *ar;
	int ret = 0;

	uae_sem_wait (&async_sem);
	ar = dev->ar;
	while (ar) {
		if (ar->arequest == arequest) {
			if (ready)
				ar->ready = 1;
			break;
		}
		ar = ar->next;
	}
	uae_sem_post (&async_sem);
	return ar;
}

static int add_async_request (struct s2devstruct *dev, uae_u8 *request, uaecptr arequest)
{
	struct asyncreq *ar, *ar2;

	if (log_net)
		write_log (_T("%s:%d async request %x added\n"), getdevname(), dev->unit, arequest);

	uae_sem_wait (&async_sem);
	ar = xcalloc (struct asyncreq, 1);
	ar->arequest = arequest;
	ar->request = request;
	if (!dev->ar) {
		dev->ar = ar;
	} else {
		ar2 = dev->ar;
		while (ar2->next)
			ar2 = ar2->next;
		ar2->next = ar;
	}
	uae_sem_post (&async_sem);
	return 1;
}

static int release_async_request (struct s2devstruct *dev, uaecptr arequest)
{
	struct asyncreq *ar, *prevar;

	uae_sem_wait (&async_sem);
	ar = dev->ar;
	prevar = NULL;
	while (ar) {
		if (ar->arequest == arequest) {
			if (prevar == NULL)
				dev->ar = ar->next;
			else
				prevar->next = ar->next;
			uae_sem_post (&async_sem);
			xfree(ar);
			if (log_net)
				write_log (_T("%s:%d async request %x removed\n"), getdevname(), dev->unit, arequest);
			return 1;
		}
		prevar = ar;
		ar = ar->next;
	}
	uae_sem_post (&async_sem);
	write_log (_T("%s:%d async request %x not found for removal!\n"), getdevname(), dev->unit, arequest);
	return 0;
}

static void do_abort_async (TrapContext *ctx, struct s2devstruct *dev, uae_u8 *request, uaecptr arequest)
{
	put_byte_host(request + 30, get_byte_host(request + 30) | 0x20);
	put_byte_host(request + 31, IOERR_ABORTED);
	put_long_host(request + 32, S2WERR_GENERIC_ERROR);
	uae_sem_wait(&pipe_sem);
	trap_set_background(ctx);
	write_comm_pipe_pvoid(&dev->requests, ctx, 0);
	write_comm_pipe_pvoid(&dev->requests, request, 0);
	write_comm_pipe_u32(&dev->requests, arequest, 1);
	uae_sem_post(&pipe_sem);
}

static void abort_async(TrapContext *ctx, struct s2devstruct *dev, uaecptr arequest)
{
	struct asyncreq *ar = get_async_request (dev, arequest, 1);
	if (!ar) {
		write_log (_T("%s:%d: abort async but no request %x found!\n"), getdevname(), dev->unit, arequest);
		return;
	}
	if (log_net)
		write_log (_T("%s:%d asyncronous request=%08X aborted\n"), getdevname(), dev->unit, arequest);
	do_abort_async(ctx, dev, ar->request, arequest);
}

static void signalasync(TrapContext *ctx, struct s2devstruct *dev, struct asyncreq *ar, int actual, int err)
{
	uaecptr arequest = ar->arequest;
	uae_u8 *request = ar->request;
	int command = get_word_host(request + 28);
	if (log_net)
		write_log (_T("%s:%d CMD=%d async request %x completed\n"), getdevname(), dev->unit, command, arequest);
	put_long_host(request + 32, actual);
	put_byte_host(request + 31, err);
	ar->ready = 1;
	uae_sem_wait(&pipe_sem);
	trap_set_background(ctx);
	write_comm_pipe_pvoid(&dev->requests, ctx, 0);
	write_comm_pipe_pvoid(&dev->requests, request, 0);
	write_comm_pipe_u32(&dev->requests, arequest, 1);
	uae_sem_post(&pipe_sem);
}

static uae_u32 copytobuff (TrapContext *ctx, uaecptr from, uaecptr to, uae_u32 len, uaecptr func)
{
	trap_call_add_areg(ctx, 0, to);
	trap_call_add_areg(ctx, 1, from);
	trap_call_add_dreg(ctx, 0, len);
	return trap_call_func(ctx, func);
}
static uae_u32 copyfrombuff (TrapContext *ctx, uaecptr from, uaecptr to, uae_u32 len, uaecptr func)
{
	trap_call_add_areg(ctx, 0, to);
	trap_call_add_areg(ctx, 1, from);
	trap_call_add_dreg(ctx, 0, len);
	return trap_call_func(ctx, func);
}
static uae_u32 packetfilter (TrapContext *ctx, uaecptr hook, uaecptr ios2, uaecptr data)
{
	uae_u32 v;

	trap_call_add_areg(ctx, 0, hook);
	trap_call_add_areg(ctx, 2, ios2);
	trap_call_add_areg(ctx, 1, data);
	v = trap_call_func(ctx, trap_get_long(ctx, hook + 8));
	return v;
}

static int isbroadcast (const uae_u8 *d)
{
	if (d[0] == 0xff && d[1] == 0xff && d[2] == 0xff &&
		d[3] == 0xff && d[4] == 0xff && d[5] == 0xff)
		return 1;
	return 0;
}
static int ismulticast (const uae_u8 *d)
{
	if (isbroadcast (d))
		return 0;
	if (d[0] & 1)
		return 1;
	return 0;
}

static uae_u64 addrto64 (const uae_u8 *d)
{
	int i;
	uae_u64 addr = 0;
	for (i = 0; i < ADDR_SIZE; i++) {
		addr <<= 8;
		addr |= d[i];
	}
	return addr;
}
static uae_u64 amigaaddrto64(uae_u8 *d)
{
	int i;
	uae_u64 addr = 0;
	for (i = 0; i < ADDR_SIZE; i++) {
		addr <<= 8;
		addr |= get_byte_host(d + i);
	}
	return addr;
}

static void addmulticastaddresses (struct s2devstruct *dev, uae_u64 start, uae_u64 end)
{
	struct mcast *mc, *mc2;

	if (!end)
		end = start;
	mc = dev->mc;
	while (mc) {
		if (start == mc->start && end == mc->end) {
			mc->cnt++;
			return;
		}
		mc = mc->next;
	}
	mc = xcalloc (struct mcast, 1);
	mc->start = start;
	mc->end = end;
	mc->cnt = 1;
	if (!dev->mc) {
		dev->mc = mc;
	} else {
		mc2 = dev->mc;
		while (mc2->next)
			mc2 = mc2->next;
		mc2->next = mc;
	}
}
static int delmulticastaddresses (struct s2devstruct *dev, uae_u64 start, uae_u64 end)
{
	struct mcast *mc, *prevmc;

	if (!end)
		end = start;
	mc = dev->mc;
	prevmc = NULL;
	while (mc) {
		if (start == mc->start && end == mc->end) {
			mc->cnt--;
			if (mc->cnt > 0)
				return 1;
			if (prevmc == NULL)
				dev->mc = mc->next;
			else
				prevmc->next = mc->next;
			xfree (mc);
			return 1;
		}
		prevmc = mc;
		mc = mc->next;
	}
	return 0;
}

static struct s2packet *createreadpacket (struct s2devstruct *dev, const uae_u8 *d, int len)
{
	struct s2packet *s2p = xcalloc (struct s2packet, 1);
	s2p->data = xmalloc (uae_u8, dev->td->mtu + ETH_HEADER_SIZE + 2);
	memcpy(s2p->data, d, len);
	s2p->len = len;
	return s2p;
}

static const TCHAR *dumphead(uae_u8 *d, int len)
{
	static TCHAR dumptxt[256];
	uae_u16 type = (d[2 * ADDR_SIZE] << 8) | d[2 * ADDR_SIZE + 1];
	_sntprintf(dumptxt, sizeof dumptxt, _T("DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X L=%d"),
		d[0], d[1], d[2], d[3], d[4], d[5],
		d[6], d[7], d[8], d[9], d[10], d[11],
		type, len);
	return dumptxt;
}

static int handleread (TrapContext *ctx, struct priv_s2devstruct *pdev, struct s2devstruct *dev, uae_u8 *request, uaecptr arequest, uae_u8 *d, int len, int cmd)
{
	uae_u8 flags = get_byte_host(request + 30);
	uaecptr data = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4);
	uae_u8 *srcaddr = request + 32 + 4 + 4;
	uae_u8 *dstaddr = request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES;
	uae_u16 type = (d[2 * ADDR_SIZE] << 8) | d[2 * ADDR_SIZE + 1];
	uae_u32 v = 0;
	uaecptr data2;

	trap_put_bytes(ctx, d, pdev->tempbuf, len);
	memcpy(dstaddr, d, ADDR_SIZE);
	memcpy(srcaddr, d + ADDR_SIZE, ADDR_SIZE);

	put_long_host(request + 32 + 4, type);
	if (pdev->tracks[type]) {
		pdev->bytesreceived += len;
		pdev->packetsreceived++;
	}

	flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
	if (isbroadcast (d))
		flags |= SANA2IOF_BCAST;
	else if (ismulticast (d))
		flags |= SANA2IOF_MCAST;
	put_byte_host(request + 30, flags);

	data2 = pdev->tempbuf;
	if (!(flags & SANA2IOF_RAW)) {
		len -= ETH_HEADER_SIZE;
		data2 += ETH_HEADER_SIZE;
	}
	put_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2, len);

	/* drop if CMD_READ and multicast with unknown address */
	if (cmd == CMD_READ && ismulticast(dstaddr)) {
		uae_u64 mac64 = addrto64(dstaddr);
		/* multicast */
		struct mcast *mc = dev->mc;
		while (mc) {
			if (mac64 >= mc->start && mac64 <= mc->end)
				break;
			mc = mc->next;
		}
		if (!mc) {
			if (log_net)
				write_log(_T("-> %s multicast filter rejected, CMD_READ, REQ=%08X LEN=%d\n"), dumphead(d, len), arequest, len);
			return 0;
		}
	}

	if (cmd == CMD_READ) {
		if (pdev->packetfilter && packetfilter (ctx, pdev->packetfilter, arequest, data2) == 0) {
			if (log_net)
				write_log(_T("-> %s PacketFilter() rejected, CMD_READ, REQ=%08X LEN=%d\n"), dumphead(d, len), arequest, len);
			return 0;
		}
	}

	if (!copytobuff (ctx, data2, data, len, pdev->copytobuff)) {
		put_long_host(request + 32, S2WERR_BUFF_ERROR);
		put_byte_host(request + 31, S2ERR_NO_RESOURCES);
		if (log_net)
			write_log(_T("-> %s CopyToBuff() rejected, CMD_READ, REQ=%08X LEN=%d\n"), dumphead(d, len), arequest, len);
	}
	return 1;
}

static void uaenet_gotdata (void *devv, const uae_u8 *d, int len)
{
	uae_u16 type;
	struct s2packet *s2p;
	struct s2devstruct *dev = (struct s2devstruct*)devv;

	if (!dev->online)
		return;
	/* drop if bogus size */
	if (len < 20  || len >= dev->td->mtu + ETH_HEADER_SIZE + 2) {
		if (log_net)
			write_log(_T("<- invalid packet size %d\n"), len);
		return;
	}
	/* drop if dst == broadcast and src == me */
	if (isbroadcast (d) && !memcmp (d + 6, dev->td->mac, ADDR_SIZE))
		return;
	/* drop if not promiscuous and dst != broadcast and dst != me */
	if (!dev->promiscuous && !isbroadcast (d) && memcmp (d, dev->td->mac, ADDR_SIZE))
		return;

	type = (d[12] << 8) | d[13];
	s2p = createreadpacket (dev, d, len);
	if (log_net)
		write_log (_T("<-DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X L=%d P=%p\n"),
			d[0], d[1], d[2], d[3], d[4], d[5],
			d[6], d[7], d[8], d[9], d[10], d[11],
			type, len, s2p);
	uae_sem_wait (&async_sem);
	if (!dev->readqueue) {
		dev->readqueue = s2p;
	} else {
		struct s2packet *s2p2 = dev->readqueue;
		while (s2p2->next)
			s2p2 = s2p2->next;
		s2p2->next = s2p;
	}
	uaenet_int();
	uae_sem_post (&async_sem);
}

static struct s2packet *createwritepacket(TrapContext *ctx, uae_u8 *request, uaecptr arequest)
{
	struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);
	uae_u8 flags = get_byte_host(request + 30);
	uae_u32 datalength = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2);
	uaecptr data = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4);
	uae_u8 *srcaddr = request + 32 + 4 + 4;
	uae_u8 *dstaddr = request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES;
	uae_u16 packettype = get_long_host(request + 32 + 4);
	struct s2packet *s2p;

	if (!pdev) {
		if (log_net)
			write_log(_T("-> createwritepacket() without device, REQ=%08X LEN=%d\n"), arequest, datalength);
		return NULL;
	}
	if (!copyfrombuff (ctx, data, pdev->tempbuf, datalength, pdev->copyfrombuff)) {
		if (log_net)
			write_log(_T("-> CopyFromBuff() rejected, CMD_READ, REQ=%08X LEN=%d\n"), arequest, datalength);
		return NULL;
	}
	s2p = xcalloc (struct s2packet, 1);
	s2p->data = xmalloc (uae_u8, pdev->td->mtu + ETH_HEADER_SIZE + 2);
	if (flags & SANA2IOF_RAW) {
		trap_get_bytes(ctx, s2p->data, pdev->tempbuf, datalength);
		packettype = (s2p->data[2 * ADDR_SIZE + 0] << 8) | (s2p->data[2 * ADDR_SIZE + 1]);
		s2p->len = datalength;
	} else {
		trap_get_bytes(ctx, s2p->data + ETH_HEADER_SIZE, pdev->tempbuf, datalength);
		memcpy(s2p->data + ADDR_SIZE, pdev->td->mac, ADDR_SIZE);
		memcpy(s2p->data, dstaddr, ADDR_SIZE);
		s2p->data[2 * ADDR_SIZE + 0] = packettype >> 8;
		s2p->data[2 * ADDR_SIZE + 1] = (uae_u8)packettype;
		s2p->len = datalength + ETH_HEADER_SIZE;
	}
	if (pdev->tracks[packettype]) {
		pdev->packetssent++;
		pdev->bytessent += datalength;
	}
	return s2p;
}

static int uaenet_getdata(void *devv, uae_u8 *d, int *len)
{
	int gotit;
	struct asyncreq *ar;
	struct s2devstruct *dev = (struct s2devstruct*)devv;

	uae_sem_wait (&async_sem);
	ar = dev->ar;
	gotit = 0;
	while (ar && !gotit) {
		if (!ar->ready) {
			uaecptr arequest = ar->arequest;
			uae_u8 *request = ar->request;
			int command = get_word_host(request + 28);
			uae_u32 packettype = get_long_host(request + 32 + 4);
			if (command == CMD_WRITE || command == S2_BROADCAST || command == S2_MULTICAST) {
				struct priv_s2devstruct *pdev = getps2devstruct(dev->ctx, request, arequest);
				struct asyncreq *ars2p = dev->s2p;
				while (ars2p) {
					if (ars2p->arequest == arequest) {
						*len = ars2p->s2p->len;
						memcpy(d, ars2p->s2p->data, *len);
						if (log_net)
							write_log (_T("->DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X S=%d\n"),
								d[0], d[1], d[2], d[3], d[4], d[5],
								d[6], d[7], d[8], d[9], d[10], d[11],
								packettype, *len);
						gotit = 1;
						dev->packetssent++;
						signalasync(dev->ctx, dev, ar, *len, 0);
						break;
					}
					ars2p = ars2p->next;
				}
			}
		}
		ar = ar->next;
	}
	uae_sem_post (&async_sem);
	return gotit;
}

static void checkevents(TrapContext *ctx, struct s2devstruct *dev, int mask, int sem)
{
	struct asyncreq *ar;

	if (sem)
		uae_sem_wait (&async_sem);
	ar = dev->ar;
	while (ar) {
		if (!ar->ready) {
			uaecptr arequest = ar->arequest;
			uae_u8 *request = ar->request;
			int command = get_word_host(request + 28);
			uae_u32 cmask = get_long_host(request + 32);
			if (command == S2_ONEVENT && (mask & cmask))
				signalasync(ctx, dev, ar, 0, 0);
		}
		ar = ar->next;
	}
	if (sem)
		uae_sem_post (&async_sem);
}

static int checksize(uae_u8 *request, struct s2devstruct *dev)
{
	uae_u32 datalength = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2);

	if (datalength > dev->td->mtu)
		return 0;
	return 1;
}

static void flush(TrapContext *ctx, struct priv_s2devstruct *pdev)
{
	struct asyncreq *ar;
	struct s2devstruct *dev;

	dev = gets2devstruct (pdev->unit);
	ar = dev->ar;
	while (ar) {
		if (!ar->ready && getps2devstruct(ctx, ar->request, ar->arequest) == pdev) {
			ar->ready = 1;
			do_abort_async(ctx, dev, ar->request, ar->arequest);
		}
		ar = ar->next;
	}
}

static int dev_do_io_2 (TrapContext *ctx, struct s2devstruct *dev, uae_u8 *request, uaecptr arequest, int quick)
{
	struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);

	uae_u8 flags = get_byte_host(request + 30);
	uae_u32 command = get_word_host(request + 28);
	uae_u32 packettype = get_long_host(request + 32 + 4);
	uaecptr data = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4);
	uae_u32 datalength = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2);
	uae_u8 *srcaddr = request + 32 + 4 + 4;
	uae_u8 *dstaddr = request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES;
	uaecptr statdata = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4 + 4);
	uaecptr buffermgmt = get_long_host(request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES * 2 + 4 + 4 + 4);
	uae_u32 io_error = 0;
	uae_u32 wire_error = 0;
	int i;
	int async = 0;

	if (log_net)
		write_log (_T("S2: IO=%08x %p C=%02d T=%04X S=%02X%02X%02X%02X%02X%02X D=%02X%02X%02X%02X%02X%02X L=%d D=%08X SD=%08X BM=%08X\n"),
			arequest, request, command, packettype,
			get_byte_host(srcaddr + 0), get_byte_host(srcaddr + 1), get_byte_host(srcaddr + 2), get_byte_host(srcaddr + 3), get_byte_host(srcaddr + 4), get_byte_host(srcaddr + 5),
			get_byte_host(dstaddr + 0), get_byte_host(dstaddr + 1), get_byte_host(dstaddr + 2), get_byte_host(dstaddr + 3), get_byte_host(dstaddr + 4), get_byte_host(dstaddr + 5),
			datalength, data, statdata, buffermgmt);

	if (command == CMD_READ || command == S2_READORPHAN || command == CMD_WRITE || command == S2_BROADCAST || command == S2_MULTICAST) {
		if (!pdev->copyfrombuff || !pdev->copytobuff) {
			io_error = S2ERR_BAD_ARGUMENT;
			wire_error = S2WERR_BUFF_ERROR;
			goto end;
		}
	}
	if (statdata == 0 && (command == S2_DEVICEQUERY || command == S2_GETTYPESTATS || command == S2_GETGLOBALSTATS || command == S2_GETSPECIALSTATS)) {
		io_error = S2ERR_BAD_ARGUMENT;
		wire_error = S2WERR_NULL_POINTER;
		goto end;
	}

	switch (command)
	{
	case CMD_READ:
		if (!dev->online)
			goto offline;
		async = 1;
		break;

	case S2_READORPHAN:
		if (!dev->online)
			goto offline;
		async = 1;
		break;

	case S2_BROADCAST:
	case CMD_WRITE:
		if (!dev->online)
			goto offline;
		if (!checksize(request, dev))
			goto toobig;
		async = 1;
		break;

	case S2_MULTICAST:
		if (!dev->online)
			goto offline;
		if ((get_byte_host(dstaddr + 0) & 1) == 0) {
			io_error = S2ERR_BAD_ADDRESS;
			wire_error = S2WERR_BAD_MULTICAST;
			goto end;
		}
		if (!checksize(request, dev))
			goto toobig;
		async = 1;
		break;

	case CMD_FLUSH:
		dev->flush_timeout_cnt = 0;
		dev->flush_timeout = FLUSH_TIMEOUT;
		if (log_net)
			write_log (_T("CMD_FLUSH started %08x\n"), request);
		uae_sem_wait (&async_sem);
		flush(ctx, pdev);
		uae_sem_post (&async_sem);
		async = 1;
		uaenet_int();
		break;

	case S2_ADDMULTICASTADDRESS:
		addmulticastaddresses (dev, amigaaddrto64(srcaddr), 0);
		break;
	case S2_DELMULTICASTADDRESS:
		if (!delmulticastaddresses (dev, amigaaddrto64(srcaddr), 0)) {
			io_error = S2ERR_BAD_STATE;
			wire_error = S2WERR_BAD_MULTICAST;
		}
		break;
	case S2_ADDMULTICASTADDRESSES:
		addmulticastaddresses(dev, amigaaddrto64(srcaddr), amigaaddrto64(dstaddr));
		break;
	case S2_DELMULTICASTADDRESSES:
		if (!delmulticastaddresses(dev, amigaaddrto64(srcaddr), amigaaddrto64(dstaddr))) {
			io_error = S2ERR_BAD_STATE;
			wire_error = S2WERR_BAD_MULTICAST;
		}
		break;

	case S2_DEVICEQUERY:
		{
			uae_u8 statbuf[34];
			int size = trap_get_long(ctx, statdata);
			if (size < 8 || size >= 0x10000) {
				io_error = S2ERR_BAD_ARGUMENT;
				wire_error = S2WERR_BAD_STATDATA;
				goto end;
			}
			if (size > 34)
				size = 34;
			put_long_host(statbuf + 4 , size);
			put_long_host(statbuf + 8, 0);
			put_long_host(statbuf + 12, 0);
			put_word_host(statbuf + 16, ADDR_SIZE * 8);
			put_long_host(statbuf + 18, dev->td->mtu);
			put_long_host(statbuf + 22, 10000000);
			put_long_host(statbuf + 26, S2WireType_Ethernet);
			put_long_host(statbuf + 30, 1514);
			trap_put_bytes(ctx, statbuf + 4, statdata + 4, size - 4);
		}
		break;

	case S2_GETTYPESTATS:
		if (pdev->trackcnt) {
			trap_put_long(ctx, statdata + 0, pdev->packetssent);
			trap_put_long(ctx, statdata + 4, pdev->packetsreceived);
			trap_put_long(ctx, statdata + 8, pdev->bytessent);
			trap_put_long(ctx, statdata + 12, pdev->bytesreceived);
			trap_put_long(ctx, statdata + 16, pdev->packetsdropped);
		} else {
			io_error = S2ERR_BAD_STATE;
			wire_error = S2WERR_NOT_TRACKED;
		}
		break;

	case S2_GETGLOBALSTATS:
		trap_put_long(ctx, statdata + 0, dev->packetsreceived);
		trap_put_long(ctx, statdata + 4, dev->packetssent);
		trap_put_long(ctx, statdata + 8, dev->baddata);
		trap_put_long(ctx, statdata + 12, dev->overruns);
		trap_put_long(ctx, statdata + 16, 0);
		trap_put_long(ctx, statdata + 20, dev->unknowntypesreceived);
		trap_put_long(ctx, statdata + 24, dev->reconfigurations);
		trap_put_long(ctx, statdata + 28, dev->online_secs);
		trap_put_long(ctx, statdata + 32, dev->online_micro);
		break;

	case S2_GETSPECIALSTATS:
		trap_put_long(ctx, statdata + 1, 0);
		break;

	case S2_GETSTATIONADDRESS:
		for (i = 0; i < ADDR_SIZE; i++) {
			put_byte_host(srcaddr + i, dev->td->mac[i]);
			put_byte_host(dstaddr + i, dev->td->mac[i]);
		}
		break;

	case S2_CONFIGINTERFACE:
		if (0 && dev->configured) {
			io_error = S2ERR_BAD_STATE;
			wire_error = S2WERR_IS_CONFIGURED;
		} else {
			dev->configured = TRUE;
		}
		break;

	case S2_ONLINE:
		if (!dev->configured) {
			io_error = S2ERR_BAD_STATE;
			wire_error = S2WERR_NOT_CONFIGURED;
		}
		if (!dev->adapter) {
			io_error = S2ERR_OUTOFSERVICE;
			wire_error = S2WERR_RCVREL_HDW_ERR;
		}
		if (!io_error) {
			uaenet_int();
			async = 1;
		}
		break;

	case S2_TRACKTYPE:
		if (packettype <= 65535) {
			if (pdev->tracks[packettype]) {
				io_error = S2ERR_BAD_STATE;
				wire_error = S2WERR_ALREADY_TRACKED;
			} else {
				pdev->tracks[packettype] = 1;
				pdev->trackcnt++;
			}
		} else {
			io_error = S2ERR_BAD_ARGUMENT;
		}
		break;
	case S2_UNTRACKTYPE:
		if (packettype <= 65535) {
			if (!pdev->tracks[packettype]) {
				io_error = S2ERR_BAD_STATE;
				wire_error = S2WERR_NOT_TRACKED;
			} else {
				pdev->tracks[packettype] = 0;
				pdev->trackcnt--;
			}
		} else {
			io_error = S2ERR_BAD_ARGUMENT;
		}
		break;

	case S2_OFFLINE:
		if (dev->online) {
			dev->online = 0;
			checkevents(ctx, dev, S2EVENT_OFFLINE, 1);
		}
		break;

	case S2_ONEVENT:
		{
			uae_u32 events;
			uae_u32 wanted_events = get_long_host(request + 32);
			if (wanted_events & ~KNOWN_EVENTS) {
				io_error = S2ERR_NOT_SUPPORTED;
				events = S2WERR_BAD_EVENT;
			} else {
				if (dev->online)
					events = S2EVENT_ONLINE;
				else
					events = S2EVENT_OFFLINE;
				events &= wanted_events;
				if (events) {
					wire_error = events;
				} else {
					async = 1;
				}
			}
		}
		break;

	default:
		io_error = IOERR_NOCMD;
		break;

offline:
		io_error = S2ERR_OUTOFSERVICE;
		wire_error = S2WERR_UNIT_OFFLINE;
		break;
toobig:
		io_error = S2ERR_MTU_EXCEEDED;
		wire_error = S2WERR_GENERIC_ERROR;
		break;

	}
end:
	if (log_net && (io_error || wire_error))
		write_log (_T("-> ERR %d (%d)\n"), io_error, wire_error);
	put_long_host(request + 32, wire_error);
	put_byte_host(request + 31, io_error);
	return async;
}

static int dev_do_io (TrapContext *ctx, struct s2devstruct *dev, uae_u8 *request, uaecptr arequest, int quick)
{
	struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);
	uae_u32 command = get_word_host(request + 28);
	uae_u16 len = get_word_host(request + 0x12);

	put_byte_host(request + 31, 0);
	if (!pdev) {
		write_log (_T("%s unknown iorequest %08x\n"), getdevname (), request);
		return 0;
	}
	if (command == NSCMD_DEVICEQUERY) {
		uae_u32 data = get_long_host(request + 40); /* io_data */
		trap_put_long(ctx, data + 0, 0);
		trap_put_long(ctx, data + 4, 16); /* size */
		trap_put_word(ctx, data + 8, 7); /* NSDEVTYPE_SANA2 */
		trap_put_word(ctx, data + 10, 0);
		trap_put_long(ctx, data + 12, nscmd_cmd);
		put_long_host(request + 32, 16); /* io_actual */
		return 0;
	} else if (len < SANA2_IOREQSIZE) {
		put_byte_host(request + 31, IOERR_BADLENGTH);
		return 0;
	}
	return dev_do_io_2(ctx, dev, request, arequest, quick);
}

static int dev_can_quick (uae_u32 command)
{
	switch (command)
	{
		case NSCMD_DEVICEQUERY:
		case S2_DEVICEQUERY:
		case S2_CONFIGINTERFACE:
		case S2_GETSTATIONADDRESS:
		case S2_TRACKTYPE:
		case S2_UNTRACKTYPE:
		case S2_ADDMULTICASTADDRESS:
		case S2_ADDMULTICASTADDRESSES:
		case S2_DELMULTICASTADDRESS:
		case S2_DELMULTICASTADDRESSES:
		case S2_GETTYPESTATS:
		case S2_GETGLOBALSTATS:
		case S2_GETSPECIALSTATS:
		return 1;
	}
	return 0;
}

static int dev_canquick (struct s2devstruct *dev, uae_u8 *request)
{
	uae_u32 command = get_word_host(request + 28);
	return dev_can_quick (command);
}

static void storerequest(TrapContext *ctx, uae_u8 *src, uaecptr dst)
{
	uae_u32 cmd = get_word_host(src + 28);
	uae_u16 len = get_word_host(src + 18);

	if (len < 8)
		return;
	// IORequest ln_Type needs to be copied
	len -= 8;
	src += 8;
	dst += 8;
	if (cmd == S2_BROADCAST || cmd == S2_MULTICAST || cmd == S2_READORPHAN || cmd == CMD_WRITE || cmd == CMD_READ || cmd == S2_GETSTATIONADDRESS) {
		// Copy:
		// IORequest
		// io_Flags, ioError
		// ios2_WireError, ios2_PacketType
		// ios2_SrcAddr, ios2_DstAddr
		// ios2_DataLength
		if (len > SANA2_IOREQSIZE - (8 + 3 * 4))
			len = SANA2_IOREQSIZE - (8 + 3 * 4);
		trap_put_bytes(ctx, src, dst, len);
	} else {
		// Copy:
		// IORequest
		// io_Flags, ioError
		// ios2_WireError
		trap_put_bytes(ctx, src, dst, (32 - 8) + 1 + 1 + 4);
	}
}

static uae_u32 REGPARAM2 dev_beginio (TrapContext *ctx)
{
	uae_u32 err = 0;
	uae_u32 arequest = trap_get_areg(ctx, 1);
	uae_u8 *request = xmalloc(uae_u8, SANA2_IOREQSIZE);
	trap_get_bytes(ctx, request, arequest, SANA2_IOREQSIZE);
	struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);

	uae_u8 flags = get_byte_host(request + 30);
	int command = get_word_host(request + 28);
	struct s2devstruct *dev;

	put_byte_host(request + 31, 0);

	put_byte_host(request + 8, NT_MESSAGE);
	if (!pdev) {
		write_log (_T("%s unknown iorequest (1) %08x\n"), getdevname (), request);
		err = 32;
		goto err;
	}
	dev = gets2devstruct (pdev->unit);
	if (!dev) {
		write_log (_T("%s unknown iorequest (2) %08x\n"), getdevname (), request);
		err =  32;
		goto err;
	}

	if ((flags & 1) && dev_canquick (dev, request)) {
		if (dev_do_io(ctx, dev, request, arequest, 1))
			write_log (_T("%s: command %d bug with IO_QUICK\n"), SANA2NAME, command);
		err = get_byte_host(request + 31);
	} else {
		if (command == CMD_WRITE || command == S2_BROADCAST || command == S2_MULTICAST) {
			struct s2packet *s2p;
			if (!pdev->copyfrombuff || !pdev->copytobuff) {
				put_long_host(request + 32, S2ERR_BAD_ARGUMENT);
				put_byte_host(request + 31, S2WERR_BUFF_ERROR);
			} else {
				if (command == S2_BROADCAST) {
					uae_u8 *dstaddr = request + 32 + 4 + 4 + SANA2_MAX_ADDR_BYTES;
					put_byte_host(dstaddr + 0, 0xff);
					put_byte_host(dstaddr + 1, 0xff);
					put_byte_host(dstaddr + 2, 0xff);
					put_byte_host(dstaddr + 3, 0xff);
					put_byte_host(dstaddr + 4, 0xff);
					put_byte_host(dstaddr + 5, 0xff);
				}
				s2p = createwritepacket(ctx, request, arequest);
				if (s2p) {
					uae_sem_wait (&async_sem);
					add_async_packet (dev, s2p, request, arequest);
					uae_sem_post (&async_sem);
				}
				if (!s2p) {
					put_long_host(request + 32, S2WERR_BUFF_ERROR);
					put_byte_host(request + 31, S2ERR_NO_RESOURCES);
				}
			}
		}
		put_byte_host(request + 30, get_byte_host(request + 30) & ~1);
		storerequest(ctx, request, arequest);
		uae_sem_wait(&pipe_sem);
		trap_set_background(ctx);
		write_comm_pipe_pvoid(&dev->requests, ctx, 0);
		write_comm_pipe_pvoid(&dev->requests, request, 0);
		write_comm_pipe_u32(&dev->requests, arequest, 1);
		uae_sem_post(&pipe_sem);
		return 0;
	}
err:
	storerequest(ctx, request, arequest);
	xfree(request);
	return err;
}

static int dev_thread (void *devs)
{
	struct s2devstruct *dev = (struct s2devstruct*)devs;

	uae_set_thread_priority (NULL, 1);
	dev->thread_running = 1;
	uae_sem_post (&dev->sync_sem);
	for (;;) {
		TrapContext *ctx = (TrapContext*)read_comm_pipe_pvoid_blocking(&dev->requests);
		uae_u8 *iobuf = (uae_u8*)read_comm_pipe_pvoid_blocking(&dev->requests);
		uaecptr request = (uaecptr)read_comm_pipe_u32_blocking (&dev->requests);
		uae_sem_wait (&change_sem);
		if (!request) {
			dev->thread_running = 0;
			uae_sem_post (&dev->sync_sem);
			uae_sem_post (&change_sem);
			write_log (_T("%s: dev_thread killed\n"), getdevname ());
			return 0;
		}
		struct priv_s2devstruct *pdev = getps2devstruct(ctx, iobuf, request);
		asyncreq *ar = get_async_request (dev, request, 1);
		if (ar) {
			storerequest(ctx, ar->request, request);
			uae_ReplyMsg (request);
			release_async_request (dev, request);
			rem_async_packet (dev, request);
		} else if (dev_do_io(ctx, dev, iobuf, request, 0) == 0) {
			storerequest(ctx, iobuf, request);
			uae_ReplyMsg (request);
			rem_async_packet (dev, request);
		} else {
			add_async_request (dev, iobuf, request);
			dev->ctx = ctx;
			ethernet_trigger (pdev->td, dev->sysdata);
			dev->ctx = NULL;
			iobuf = NULL;
		}
		trap_background_set_complete(ctx);
		uae_sem_post (&change_sem);
	}
	return 0;
}

static uae_u32 REGPARAM2 dev_init_2 (TrapContext *ctx)
{
	uae_u32 base = trap_get_dreg(ctx, 0);
	if (log_net)
		write_log (_T("%s init\n"), SANA2NAME);
	return base;
}

static uae_u32 REGPARAM2 dev_init (TrapContext *ctx)
{
	return dev_init_2 (ctx);
}

static uae_u32 REGPARAM2 dev_abortio (TrapContext *ctx)
{
	uae_u8 err = 0;
	uae_u32 request = trap_get_areg(ctx, 1);
	struct priv_s2devstruct *pdev = getps2devstruct(ctx, NULL, request);
	struct s2devstruct *dev;

	if (!pdev) {
		write_log (_T("%s abortio but no request %08x found!\n"), getdevname(), request);
		err = 32;
		trap_put_byte(ctx, request + 31, err);
		return err;
	}
	dev = gets2devstruct (pdev->unit);
	if (!dev) {
		write_log (_T("%s (%d) abortio but no request %08x found!\n"), getdevname(), pdev->unit, request);
		err = 32;
		trap_put_byte(ctx, request + 31, err);
		return err;
	}
	if (log_net)
		write_log (_T("%s:%d abortio %08x\n"), getdevname(), dev->unit, request);
	abort_async(ctx, dev, request);
	return 0;
}

static void resetpackettimer(struct s2packet *p)
{
	while (p) {
		p->drop_start = 0;
		p->drop_count = 0;
		p = p->next;
	}
}

static int uaenet_int_handler2(TrapContext *ctx)
{
	int i, j;
	struct asyncreq *ar;
	bool irq = false;

	for (i = 0; i < MAX_OPEN_DEVICES; i++) {
		pdevst[i].tmp = 0;
	}

	for (i = 0; i < MAX_TOTAL_NET_DEVICES; i++) {
		struct s2devstruct *dev = &devst[i];
		struct s2packet *p;
		if (dev->online) {
			while (dev->readqueue) {
				uae_u16 type;
				p = dev->readqueue;
				type = (p->data[2 * ADDR_SIZE] << 8) | p->data[2 * ADDR_SIZE + 1];
				ar = dev->ar;
				while (ar) {
					if (!ar->ready) {
						uaecptr arequest = ar->arequest;
						uae_u8 *request = ar->request;
						int command = get_word_host(request + 28);
						if (command == CMD_READ) {
							uae_u32 packettype = get_long_host(request + 32 + 4);
							if (packettype == type || (packettype <= 1500 && type <= 1500)) {
								struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);
								if (pdev && pdev->tmp == 0) {
									if (handleread (ctx, pdev, dev, request, arequest, p->data, p->len, command)) {
										if (log_net) {
											write_log (_T("-> %s Accepted, CMD_READ, REQ=%08X LEN=%d D=%d\n"), dumphead(p->data, p->len), arequest, p->len, p->drop_start ? 1 : 0);
										}
										ar->ready = 1;
										uae_sem_wait(&pipe_sem);
										trap_set_background(ctx);
										write_comm_pipe_pvoid(&dev->requests, ctx, 0);
										write_comm_pipe_pvoid(&dev->requests, request, 0);
										write_comm_pipe_u32 (&dev->requests, arequest, 1);
										uae_sem_post(&pipe_sem);
										dev->packetsreceived++;
										pdev->tmp = 1;
										dev->readqueue = dev->readqueue->next;
										resetpackettimer(dev->readqueue);
										freepacket(p);
										return -1;
									} else {
										pdev->tmp = -1;
									}
								}
							}
						}
					}
					ar = ar->next;
				}
				ar = dev->ar;
				while (ar) {
					if (!ar->ready) {
						uaecptr arequest = ar->arequest;
						uae_u8 *request = ar->request;
						int command = get_word_host(request + 28);
						if (command == S2_READORPHAN) {
							struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);
							if (pdev && pdev->tmp <= 0) {
								if (log_net)
									write_log (_T("-> %s Accepted, S2_READORPHAN, REQ=%08X D=%d\n"), dumphead(p->data, p->len), arequest, p->drop_start ? 1 : 0);
								handleread (ctx, pdev, dev, request, arequest, p->data, p->len, command);
								ar->ready = 1;
								uae_sem_wait(&pipe_sem);
								trap_set_background(ctx);
								write_comm_pipe_pvoid(&dev->requests, ctx, 0);
								write_comm_pipe_pvoid(&dev->requests, request, 0);
								write_comm_pipe_u32 (&dev->requests, arequest, 1);
								uae_sem_post(&pipe_sem);
								dev->packetsreceived++;
								dev->unknowntypesreceived++;
								pdev->tmp = 1;
								dev->readqueue = dev->readqueue->next;
								resetpackettimer(dev->readqueue);
								freepacket(p);
								return -1;
							}
						}
					}
					ar = ar->next;
				}
				if (p->drop_start == 0 || p->drop_count - p->drop_start < DELAYED_DROPPED_PACKET_FRAMES) {
					// we got packet but there was no readers, lets wait a bit before dropping it.
					if (log_net && p->drop_start == 0) {
						write_log(_T("-> %s No readers, queued for dropping\n"), dumphead(p->data, p->len));
					}
					while (p) {
						if (p->drop_start == 0)
							p->drop_start = vsync_counter;
						p->drop_count = vsync_counter;
						p = p->next;
					}
					break;
				}
				if (log_net) {
					write_log (_T("-> %s packet dropped, CNT=%d/%d:%d\n"), dumphead(p->data, p->len), p->drop_start, p->drop_count, p->drop_count - p->drop_start);
				}
				for (j = 0; j < MAX_OPEN_DEVICES; j++) {
					if (pdevst[j].unit == dev->unit) {
						if (pdevst[j].tracks[type])
							pdevst[j].packetsdropped++;
					}
				}
				dev->readqueue = dev->readqueue->next;
				freepacket(p);
			}
		} else {
			while (dev->readqueue) {
				p = dev->readqueue;
				dev->readqueue = dev->readqueue->next;
				freepacket (p);
			}
		}

		ar = dev->ar;
		while (ar) {
			if (!ar->ready) {
				uaecptr arequest = ar->arequest;
				uae_u8 *request = ar->request;
				int command = get_word_host(request + 28);
				if (command == S2_ONLINE) {
					struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);
					dev->packetsreceived = 0;
					dev->packetssent = 0;
					dev->baddata = 0;
					dev->overruns = 0;
					dev->unknowntypesreceived = 0;
					dev->reconfigurations = 0;
					if (pdev && pdev->timerbase) {
						trap_call_add_areg(ctx, 0, pdev->tempbuf);
						trap_call_lib(ctx, pdev->timerbase, -0x42); /* GetSysTime() */
					} else {
						trap_put_long(ctx, pdev->tempbuf + 0, 0);
						trap_put_long(ctx, pdev->tempbuf + 4, 0);
					}
					dev->online_secs = trap_get_long(ctx, pdev->tempbuf + 0);
					dev->online_micro = trap_get_long(ctx, pdev->tempbuf + 4);
					checkevents(ctx, dev, S2EVENT_ONLINE, 0);
					dev->online = 1;
					ar->ready = 1;
					uae_sem_wait(&pipe_sem);
					trap_set_background(ctx);
					write_comm_pipe_pvoid(&dev->requests, ctx, 0);
					write_comm_pipe_pvoid(&dev->requests, request, 0);
					write_comm_pipe_u32(&dev->requests, arequest, 1);
					uae_sem_post(&pipe_sem);
					return -1;
				} else if (command == CMD_FLUSH) {
					/* do not reply CMD_FLUSH until all other requests are gone */
					if (dev->ar->next == NULL) {
						if (log_net)
							write_log (_T("CMD_FLUSH replied %08x\n"), arequest);
						ar->ready = 1;
						uae_sem_wait(&pipe_sem);
						trap_set_background(ctx);
						write_comm_pipe_pvoid(&dev->requests, ctx, 0);
						write_comm_pipe_pvoid(&dev->requests, request, 0);
						write_comm_pipe_u32 (&dev->requests, arequest, 1);
						uae_sem_post(&pipe_sem);
						return -1;
					} else {
						struct priv_s2devstruct *pdev = getps2devstruct(ctx, request, arequest);
						if (pdev) {
							dev->flush_timeout--;
							if (dev->flush_timeout <= 0) {
								dev->flush_timeout = FLUSH_TIMEOUT;
								if (dev->flush_timeout_cnt > 1)
									write_log (_T("WARNING: %s:%d CMD_FLUSH possibly frozen..\n"), getdevname(), pdev->unit);
								dev->flush_timeout_cnt++;
								flush(ctx, pdev);
							}
						}
					}
				}
			}
			ar = ar->next;
		}
	}
	if (uaenet_int_late)
		irq = true;
	uaenet_int_late = 0;
	return irq ? 1 : 0;
}

static uae_u32 REGPARAM2 uaenet_int_handler(TrapContext *ctx)
{
	for (;;) {
		if (uae_sem_trywait(&async_sem)) {
			uaenet_int_late = 1;
			return 1;
		}
		int r = uaenet_int_handler2(ctx);
		uae_sem_post(&async_sem);
		if (r <= 0)
			return r;
	}
}

static void uaenet_vsync(void)
{
	if (!irq_init)
		return;
	if (uae_sem_trywait(&async_sem))
		return;
	bool pending = false;
	for (int i = 0; i < MAX_TOTAL_NET_DEVICES; i++) {
		struct s2devstruct *dev = &devst[i];
		if (dev->online) {
			if(dev->readqueue)
				pending = true;
		}
		if (dev->ar) {
			pending = true;
		}
	}
	if (uaenet_int_late || pending)
		uaenet_int();
	uaenet_int_late = 0;
	uae_sem_post(&async_sem);
}

static void dev_reset (void)
{
	write_log (_T("%s reset\n"), getdevname());
	for (int i = 0; i < MAX_TOTAL_NET_DEVICES; i++) {
		if (td[i] && td[i]->active) {
			write_log(_T("- %d: %s (%s)\n"), i, td[i]->desc, td[i]->name);
		}
	}
	for (int i = 0; i < MAX_TOTAL_NET_DEVICES; i++) {
		struct s2devstruct *dev = &devst[i];
		if (dev->opencnt) {
			struct asyncreq *ar = dev->ar;
			while (ar) {
				if (!ar->ready) {
					dev->ar->ready = 1;
				}
				ar = ar->next;
			}
			uae_sem_wait(&pipe_sem);
			write_comm_pipe_pvoid(&dev->requests, NULL, 0);
			write_comm_pipe_pvoid(&dev->requests, NULL, 0);
			write_comm_pipe_u32 (&dev->requests, 0, 1);
			uae_sem_post(&pipe_sem);
			uae_sem_wait (&dev->sync_sem);
			if (dev->td && dev->sysdata) {
				ethernet_close(dev->td, dev->sysdata);
				dev->td = NULL;
			}
			xfree(dev->sysdata);
			dev->sysdata = NULL;
		}
		while (dev->mc) {
			delmulticastaddresses(dev, dev->mc->start, dev->mc->end);
		}
		memset (dev, 0, sizeof (struct s2devstruct));
	}
	for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
		memset(&pdevst[i], 0, sizeof(struct priv_s2devstruct));
	}
	irq_init = 0;

}

static void netdev_start_threads(void)
{
	if (!currprefs.sana2)
		return;
	if (log_net)
		write_log(_T("netdev_start_threads()\n"));
	uae_sem_init(&change_sem, 0, 1);
	uae_sem_init(&pipe_sem, 0, 1);
	uae_sem_init(&async_sem, 0, 1);
}

static void netdev_reset(int hardreset)
{
	uaenet_signal_state = 1;
	netdev_start_threads();
	if (!currprefs.sana2)
		return;
	dev_reset();
}

uaecptr netdev_startup(TrapContext *ctx, uaecptr resaddr)
{
	if (!currprefs.sana2)
		return resaddr;
	if (log_net)
		write_log (_T("netdev_startup(0x%x)\n"), resaddr);
	/* Build a struct Resident. This will set up and initialize
	* the uaenet.device */
	trap_put_word(ctx, resaddr + 0x0, 0x4AFC);
	trap_put_long(ctx, resaddr + 0x2, resaddr);
	trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
	if (kickstart_version >= 37) {
		trap_put_long(ctx, resaddr + 0xA, 0x84010300 | AFTERDOS_PRI); /* RTF_AUTOINIT|RTF_AFTERDOS; Version 1; NT_DEVICE; pri */
	} else {
		trap_put_long(ctx, resaddr + 0xA, 0x81010305); /* RTF_AUTOINIT|RTF_COLDSTART; Version 1; NT_DEVICE; pri 05 */
	}
	trap_put_long(ctx, resaddr + 0xE, ROM_netdev_resname);
	trap_put_long(ctx, resaddr + 0x12, ROM_netdev_resid);
	trap_put_long(ctx, resaddr + 0x16, ROM_netdev_init); /* calls netdev_init */
	resaddr += 0x1A;
	return resaddr;
}

void netdev_install (void)
{
	uae_u32 functable, datatable;
	uae_u32 initcode, openfunc, closefunc, expungefunc;
	uae_u32 beginiofunc, abortiofunc;

	if (!currprefs.sana2)
		return;
	if (log_net)
		write_log (_T("netdev_install(): 0x%x\n"), here ());

	ethernet_enumerate_free ();
	ethernet_enumerate (td, 0);

	ROM_netdev_resname = ds (getdevname());
	ROM_netdev_resid = ds (_T("UAE net.device 0.2"));
	timerdevname = ds (_T("timer.device"));

	/* initcode */
	initcode = here ();
	calltrap (deftrap2 (dev_init, TRAPFLAG_EXTRA_STACK, _T("uaenet.init"))); dw (RTS);

	/* Open */
	openfunc = here ();
	calltrap (deftrap2 (dev_open, TRAPFLAG_EXTRA_STACK, _T("uaenet.open"))); dw (RTS);

	/* Close */
	closefunc = here ();
	calltrap (deftrap2 (dev_close, TRAPFLAG_EXTRA_STACK, _T("uaenet.close"))); dw (RTS);

	/* Expunge */
	expungefunc = here ();
	calltrap (deftrap2 (dev_expunge, TRAPFLAG_EXTRA_STACK, _T("uaenet.expunge"))); dw (RTS);

	/* BeginIO */
	beginiofunc = here ();
	calltrap (deftrap2 (dev_beginio, TRAPFLAG_EXTRA_STACK, _T("uaenet.beginio"))); dw (RTS);

	/* AbortIO */
	abortiofunc = here ();
	calltrap (deftrap2 (dev_abortio, TRAPFLAG_EXTRA_STACK, _T("uaenet.abortio"))); dw (RTS);

	/* FuncTable */
	functable = here ();
	dl (openfunc); /* Open */
	dl (closefunc); /* Close */
	dl (expungefunc); /* Expunge */
	dl (EXPANSION_nullfunc); /* Null */
	dl (beginiofunc); /* BeginIO */
	dl (abortiofunc); /* AbortIO */
	dl (0xFFFFFFFFul); /* end of table */

	/* DataTable */
	datatable = here ();
	dw (0xE000); /* INITBYTE */
	dw (0x0008); /* LN_TYPE */
	dw (0x0300); /* NT_DEVICE */
	dw (0xC000); /* INITLONG */
	dw (0x000A); /* LN_NAME */
	dl (ROM_netdev_resname);
	dw (0xE000); /* INITBYTE */
	dw (0x000E); /* LIB_FLAGS */
	dw (0x0600); /* LIBF_SUMUSED | LIBF_CHANGED */
	dw (0xD000); /* INITWORD */
	dw (0x0014); /* LIB_VERSION */
	dw (0x0004); /* 0.4 */
	dw (0xD000); /* INITWORD */
	dw (0x0016); /* LIB_REVISION */
	dw (0x0000); /* end of table already ??? */
	dw (0xC000); /* INITLONG */
	dw (0x0018); /* LIB_IDSTRING */
	dl (ROM_netdev_resid);
	dw (0x0000); /* end of table */

	ROM_netdev_init = here ();
	dl (0x00000100); /* size of device base */
	dl (functable);
	dl (datatable);
	dl (initcode);

	nscmd_cmd = here ();
	dw (CMD_READ);
	dw (CMD_WRITE);
	dw (CMD_FLUSH);
	dw (S2_DEVICEQUERY);
	dw (S2_GETSTATIONADDRESS);
	dw (S2_CONFIGINTERFACE);
	dw (S2_ADDMULTICASTADDRESS);
	dw (S2_DELMULTICASTADDRESS);
	dw (S2_MULTICAST);
	dw (S2_BROADCAST);
	dw (S2_TRACKTYPE);
	dw (S2_UNTRACKTYPE);
	dw (S2_GETTYPESTATS);
	dw (S2_GETSPECIALSTATS);
	dw (S2_GETGLOBALSTATS);
	dw (S2_ONEVENT);
	dw (S2_READORPHAN);
	dw (S2_ONLINE);
	dw (S2_OFFLINE);
	dw (S2_ADDMULTICASTADDRESSES);
	dw (S2_DELMULTICASTADDRESSES);
	dw (NSCMD_DEVICEQUERY);
	dw (0);

	device_add_vsync_pre(uaenet_vsync);
	device_add_reset(netdev_reset);
}

#endif /* SANA2 */
