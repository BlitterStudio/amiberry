/*
* UAE - The Un*x Amiga Emulator
*
* A2065 ZorroII Ethernet Card
*
* Copyright 2009 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef A2065

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "a2065.h"
#include "ethernet.h"
#include "crc32.h"
#include "savestate.h"
#include "autoconf.h"
#include "rommgr.h"
#include "debug.h"
#include "devices.h"
#include "threaddep/thread.h"

#define DUMPPACKET 0

#define MEM_MIN 0xffff
int log_a2065 = 0;
static int log_transmit = 1;
static int log_receive = 1;
int a2065_promiscuous = 0;

#define A2065_CHIP_OFFSET 0x4000
#define ARIADNE_CHIP_OFFSET 0x370
#define A2065_RAP (A2065_CHIP_OFFSET + 2)
#define A2065_RDP (A2065_CHIP_OFFSET)
#define ARIADNE_RAP (ARIADNE_CHIP_OFFSET + 2)
#define ARIADNE_RDP (ARIADNE_CHIP_OFFSET)
static uaecptr addr_rap, addr_rdp;
static uae_u16 rap_mask;

#define CHIP_SIZE 4
#define RAM_OFFSET 0x8000
#define RAM_SIZE 0x8000
#define RAM_MASK 0x7fff

static uae_u8 *boardram;
#define RAP_SIZE 128
static volatile uae_u16 csr[RAP_SIZE];
static int rap;
static int configured;
static int romtype;
static bool AM79C960;
static int abyteswap;
static uae_sem_t sync_sem;

static struct netdriverdata *td;
static void *sysdata;

static volatile int am_initialized;
static volatile int transmitnow;
static uae_u16 am_mode;
static uae_u64 am_ladrf;
static uae_u32 am_rdr, am_rdr_rlen, am_rdr_rdra;
static uae_u32 am_tdr, am_tdr_tlen, am_tdr_tdra;
static int tdr_offset, rdr_offset;
static int dbyteswap, prom, fakeprom;
static uae_u8 fakemac[6], realmac[6];
static const uae_u8 broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

#define CSR0_ERR 0x8000
#define CSR0_BABL 0x4000
#define CSR0_CERR 0x2000
#define CSR0_MISS 0x1000
#define CSR0_MERR 0x0800
#define CSR0_RINT 0x0400
#define CSR0_TINT 0x0200
#define CSR0_IDON 0x0100
#define CSR0_INTR 0x0080
#define CSR0_INEA 0x0040
#define CSR0_RXON 0x0020
#define CSR0_TXON 0x0010
#define CSR0_TDMD 0x0008
#define CSR0_STOP 0x0004
#define CSR0_STRT 0x0002
#define CSR0_INIT 0x0001

#define CSR3_BSWP 0x0004
#define CSR3_ACON 0x0002
#define CSR3_BCON 0x0001

#define MODE_PROM 0x8000
#define MODE_EMBA 0x0080
#define MODE_INTL 0x0040
#define MODE_DRTY 0x0020
#define MODE_COLL 0x0010
#define MODE_DTCR 0x0008
#define MODE_LOOP 0x0004
#define MODE_DTX  0x0002
#define MODE_DRX  0x0001

#define TX_OWN 0x8000
#define TX_ERR 0x4000
#define TX_ADD_FCS 0x2000
#define TX_MORE 0x1000
#define TX_ONE 0x0800
#define TX_DEF 0x0400
#define TX_STP 0x0200
#define TX_ENP 0x0100

#define TX_BUFF 0x8000
#define TX_UFLO 0x4000
#define TX_LCOL 0x1000
#define TX_LCAR 0x0800
#define TX_RTRY 0x0400

#define RX_OWN 0x8000
#define RX_ERR 0x4000
#define RX_FRAM 0x2000
#define RX_OFLO 0x1000
#define RX_CRC 0x0800
#define RX_BUFF 0x0400
#define RX_STP 0x0200
#define RX_ENP 0x0100

DECLARE_MEMORY_FUNCTIONS(a2065);

#if DUMPPACKET
static void dumppacket (const TCHAR *n, uae_u8 *packet, int len)
{
	int i;
	TCHAR buf[10000];

	for (i = 0; i < len; i++) {
		_stprintf (buf + i * 3, _T(".%02X"), packet[i]);
	}
	write_log (_T("%s %d: "), n, len);
	write_log (_T("%s"), buf);
	write_log (_T("\n\n"));
}
#endif

#define MAX_PACKET_SIZE 4000
static uae_u8 transmitbuffer[MAX_PACKET_SIZE];
static volatile int transmitlen;

static int dofakemac (uae_u8 *packet)
{
	if (!memcmp(fakemac, realmac, 6)) {
		return 1;
	}
	if (!memcmp (packet, fakemac, 6)) {
		memcpy (packet, realmac, 6);
		return 1;
	}
	if (!memcmp (packet, realmac, 6)) {
		memcpy (packet, fakemac, 6);
		return 1;
	}
	return 0;
}

// Replace card's MAC with real MAC and vice versa.
// We have to do this because drivers are hardcoded to
// Commodore's MAC address range.

static int mungepacket (uae_u8 *packet, int len)
{
	uae_u8 *data;
	uae_u16 type;
	int ret = 0;

	if (len < 20)
		return 0;
	if (!memcmp(fakemac, realmac, 6))
		return len;
#if DUMPPACKET
	dumppacket (_T("pre:"), packet, len);
#endif
	data = packet + 14;
	type = (packet[12] << 8) | packet[13];
	// switch destination mac
	ret |= dofakemac (packet);
	// switch source mac
	ret |= dofakemac (packet + 6);
	if (type == 0x0806) { // ARP?
		if (((data[0] << 8) | data[1]) == 1 && data[4] == 6) { // Ethernet and LEN=6?
			ret |= dofakemac (data + 8); // sender
			ret |= dofakemac (data + 8 + 6 + 4); // target
		}
	} else if (type == 0x0800) { // IPv4?
		int proto = data[9];
		int ihl = data[0] & 15;
		uae_u8 *ipv4 = data;

		data += ihl * 4;
		if (proto == 17) { // UDP?
			int udpcrc = 0;
			int sp = (data[0] << 8) | data[1];
			int dp = (data[2] << 8) | data[3];
			int len2 = (data[4] << 8) | data[5];
			if (sp == 67 || sp == 68 || dp == 67 || dp == 68)
				udpcrc |= dofakemac (data + 36); // DHCP CHADDR
			if (udpcrc && (data[6] || data[7])) {
				// fix UDP checksum
				int i;
				uae_u32 sum;
				data[6] = data[7] = 0;
				data[len2] = 0;
				sum = 0;
				for (i = 0; i < ((len2 + 1) & ~1); i += 2)
					sum += (data[i] << 8) | data[i + 1];
				sum += (ipv4[12] << 8) | ipv4[13];
				sum += (ipv4[14] << 8) | ipv4[15];
				sum += (ipv4[16] << 8) | ipv4[17];
				sum += (ipv4[18] << 8) | ipv4[19];
				sum += 17;
				sum += len2;
				while (sum >> 16)
					sum = (sum & 0xFFFF) + (sum >> 16);
				sum = ~sum;
				if (sum == 0)
					sum = 0xffff;
				data[6] = sum >> 8;
				data[7] = sum >> 0;	
				ret |= 1;
			}
			// this all just to translate single DHCP MAC..
		}
	}
#if DUMPPACKET
	dumppacket (_T("post:"), packet, len);
#endif
	return ret;
}

static void rethink_a2065(void)
{
	if (!configured)
		return;
	csr[0] &= ~CSR0_INTR;
	uae_u16 mask = csr[0];
	if (AM79C960)
		mask &= (~csr[3]) & (0x4000 | 0x1000 | 0x800 | 0x400 | 0x200 | 0x100);
	if (mask & (CSR0_BABL | CSR0_MISS | CSR0_MERR | CSR0_RINT | CSR0_TINT | CSR0_IDON))
		csr[0] |= CSR0_INTR;
	if ((csr[0] & (CSR0_INTR | CSR0_INEA)) == (CSR0_INTR | CSR0_INEA)) {
		safe_interrupt_set(IRQ_SOURCE_A2065, 0, false);
		if (log_a2065 > 2)
			write_log(_T("7990 +IRQ\n"));
	}
	if (log_a2065) {
		write_log(_T("7990 -IRQ\n"));
	}
}

static int mcfilter (const uae_u8 *data)
{
	if (am_ladrf == 0) // multicast filter completely disabled?
		return 0;
	return 1; // just allow everything
}

static uae_u8 get_ram_byte(uae_u32 offset)
{
	return boardram[offset & RAM_MASK];
}
static uae_u16 get_ram_word(uae_u32 offset)
{
	return (get_ram_byte(offset) << 8) | get_ram_byte(offset + 1);
}
static void put_ram_byte(uae_u32 offset, uae_u8 v)
{
	boardram[offset & RAM_MASK] = v;
}
static void put_ram_word(uae_u32 offset, uae_u16 v)
{
	put_ram_byte(offset, v >> 8);
	put_ram_byte(offset + 1, (uae_u8)v);
}

static void gotfunc2(void *devv, const uae_u8 *databuf, int len)
{
	int i;
	int size, insize, first;
	uae_u32 addr, off;
	uae_u8 *d;
	uae_u16 rmd0, rmd1, rmd2, rmd3;
	uae_u32 crc32;
	uae_u8 tmp[MAX_PACKET_SIZE], *data;
	const uae_u8 *dstmac, *srcmac;
	struct s2devstruct *dev = (struct s2devstruct*)devv;

	dstmac = databuf;
	srcmac = databuf + 6;

	if (log_a2065 > 1 && log_receive) {
		write_log (_T("7790<!DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X S=%d\n"),
			dstmac[0], dstmac[1], dstmac[2], dstmac[3], dstmac[4], dstmac[5],
			srcmac[6], srcmac[7], srcmac[8], srcmac[9], srcmac[10], srcmac[11],
			(databuf[12] << 8) | databuf[13], len);
	}

	if (!(csr[0] & CSR0_RXON)) // receiver off?
		return;
	if (len < 20) { // too short
		if (log_a2065)
			write_log (_T("7990: short frame, %d bytes\n"), len);
		return;
	}

	if ((dstmac[0] & 0x01) && memcmp (dstmac, broadcast, sizeof broadcast) != 0) {
		// multicast
		if (!mcfilter (dstmac)) {
			if (log_a2065 > 1)
				write_log (_T("mc filtered\n"));
			return;
		}
	} else {
		// !promiscuous and dst != me and dst != broadcast
		if (!prom && (memcmp (dstmac, realmac, sizeof realmac) != 0 && memcmp (dstmac, broadcast, sizeof broadcast) != 0)) {
			if (log_a2065 > 1)
				write_log (_T("not for me1\n"));
			return;
		}
	}

	// src and dst = me? right, better drop it.
	if (memcmp (dstmac, realmac, sizeof realmac) == 0 && memcmp (srcmac, realmac, sizeof realmac) == 0) {
		if (log_a2065 > 1)
			write_log (_T("not for me2\n"));
		return;
	}
	// dst = broadcast and src = me? no thanks.
	if (memcmp (dstmac, broadcast, sizeof broadcast) == 0 && memcmp (srcmac, realmac, sizeof realmac) == 0) {
		if (log_a2065 > 1)
			write_log (_T("not for me3\n"));
		return;
	}

	if (log_a2065 > 1) {
		if (!memcmp(dstmac, realmac, sizeof realmac)) {
			write_log(_T("DST = ME. SRC = %02X.%02X.%02X.%02X.%02X.%02X\n"), srcmac[6], srcmac[7], srcmac[8], srcmac[9], srcmac[10], srcmac[11]);
		}
	}

	memcpy (tmp, databuf, len);
#if 0
	FILE *f = fopen("s:\\d\\wireshark2.cap", "rb");
	fseek (f, 474, SEEK_SET);
	fread (tmp, 342, 1, f);
	fclose (f);
	realmac[0] = 0xc8;
	realmac[1] = 0x0a;
	realmac[2] = 0xa9;
	realmac[3] = 0x81;
	realmac[4] = 0xff;
	realmac[5] = 0x2f;
	fakemac[3] = realmac[3];
	fakemac[4] = realmac[4];
	fakemac[5] = realmac[5];
#endif
	d = tmp;
	dstmac = d;
	srcmac = d + 6;
	if (log_a2065 && log_receive) {
		if (memcmp (dstmac, realmac, sizeof realmac) == 0) {
			write_log (_T("7990<-DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X S=%d\n"),
				dstmac[0], dstmac[1], dstmac[2], dstmac[3], dstmac[4], dstmac[5],
				srcmac[6], srcmac[7], srcmac[8], srcmac[9], srcmac[10], srcmac[11],
				(d[12] << 8) | d[13], len);
		}
	}
	if (mungepacket (d, len)) {
		if (log_a2065 && log_receive) {
			write_log (_T("7990<*DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X S=%d\n"),
				dstmac[0], dstmac[1], dstmac[2], dstmac[3], dstmac[4], dstmac[5],
				srcmac[6], srcmac[7], srcmac[8], srcmac[9], srcmac[10], srcmac[11],
				(d[12] << 8) | d[13], len);
		}
	}

	// winpcap does not include checksum bytes
	if (!(csr[4] & 0x0400)) { // ASTRP_RCV
		crc32 = get_crc32 (d, len);
		d[len++] = crc32 >> 24;
		d[len++] = crc32 >> 16;
		d[len++] = crc32 >>  8;
		d[len++] = crc32 >>  0;
	}

	data = tmp;
	size = 0;
	insize = 0;
	first = 1;

	for (;;) {
		rdr_offset %= am_rdr_rlen;
		off = am_rdr_rdra + rdr_offset * 8;
		rmd0 = get_ram_word(off + 0);
		rmd1 = get_ram_word(off + 2);
		rmd2 = get_ram_word(off + 4);
		rmd3 = get_ram_word(off + 6);
		addr = rmd0 | ((rmd1 & 0xff) << 16);
		addr &= RAM_MASK;

		if (!(rmd1 & RX_OWN)) {
			write_log (_T("7990: RECEIVE BUFFER ERROR\n"));
			if (!first) {
				rmd1 |= RX_BUFF | RX_OFLO;
				csr[0] &= ~CSR0_RXON;
			} else {
				csr[0] |= CSR0_MISS;
			}
			put_ram_word(off + 2, rmd1);
			devices_rethink_all(rethink_a2065);
			return;
		}

		rmd1 &= ~RX_OWN;
		rdr_offset++;

		if (first) {
			rmd1 |= RX_STP;
			first = 0;
		}

		size = 65536 - rmd2;
		uae_u8 *pr = boardram + addr;
		for (i = 0; i < size && insize < len; i++, insize++) {
			pr[(i ^ abyteswap) & RAM_MASK] = data[insize];
		}
		if (insize >= len) {
			rmd1 |= RX_ENP;
			rmd3 = len;
		}

		put_ram_word(off + 2, rmd1);
		put_ram_word(off + 6, rmd3);

		if (insize >= len)
			break;
	}

	csr[0] |= CSR0_RINT;
	devices_rethink_all(rethink_a2065);
}

static void gotfunc(void *devv, const uae_u8 *databuf, int len)
{
	if (!am_initialized)
		return;
	if (!am_rdr_rlen)
		return;

	uae_sem_wait(&sync_sem);
	gotfunc2(devv, databuf, len);
	uae_sem_post(&sync_sem);
}

static int getfunc (void *devv, uae_u8 *d, int *len)
{
	struct s2devstruct *dev = (struct s2devstruct*)devv;

	if (!am_initialized)
		return 0;
	if (transmitlen <= 0)
		return 0;
	if (transmitlen > *len) {
		write_log (_T("7990: too large packet transmission attempt %d > %d\n"), transmitlen, *len);
		transmitlen = 0;
		return 0;
	}
	memcpy (d, transmitbuffer, transmitlen);
	*len = transmitlen;
	transmitlen = 0;
	transmitnow = 1;
	return 1;
}

static void do_transmit (void)
{
	int i;
	int size, outsize;
	int err, add_fcs;
	uae_u32 addr, bufaddr;
	uae_u16 tmd0, tmd1, tmd2, tmd3;
	uae_u32 off;

	err = 0;
	size = 0;
	outsize = 0;

	if (!am_tdr_tlen)
		return;

	tdr_offset %= am_tdr_tlen;
	bufaddr = am_tdr_tdra + tdr_offset * 8;
	off = bufaddr;
	tmd1 = get_ram_word(off + 2);
	if (!(tmd1 & TX_OWN) || !(tmd1 & TX_STP)) {
		tdr_offset++;
		return;
	}
	if (!(tmd1 & TX_ENP) && log_a2065 > 0)
		write_log (_T("7990: chained transmit!?\n"));

	add_fcs = tmd1 & TX_ADD_FCS;

	for (;;) {
		tdr_offset %= am_tdr_tlen;
		off = am_tdr_tdra + tdr_offset * 8;
		tmd0 = get_ram_word(off + 0);
		tmd1 = get_ram_word(off + 2);
		tmd2 = get_ram_word(off + 4);
		tmd3 = get_ram_word(off + 6);
		addr = tmd0 | ((tmd1 & 0xff) << 16);
		addr &= RAM_MASK;

		if (!(tmd1 & TX_OWN)) {
			tmd3 |= TX_BUFF | TX_UFLO;
			tmd1 |= TX_ERR;
			csr[0] &= ~CSR0_TXON;
			write_log (_T("7990: TRANSMIT OWN NOT SET\n"));
			err = 1;
		} else {
			tmd1 &= ~TX_OWN;
			size = 65536 - tmd2;
			if (size > MAX_PACKET_SIZE)
				size = MAX_PACKET_SIZE;
			uae_u8 *pm = boardram + addr;
			for (i = 0; i < size; i++) {
				transmitbuffer[outsize++] = pm[(i ^ abyteswap) & RAM_MASK];
			}
			if (size < 60 && (csr[4] & 0x0800)) { // APAD_XMT
				while (size < 60) {
					size++;
					transmitbuffer[outsize++] = 0;
				}	
			}
			tdr_offset++;
		}
		put_ram_word(off + 2, tmd1);
		put_ram_word(off + 6, tmd3);
		if ((tmd1 & TX_ENP) || err)
			break;
	}
	if (!err && outsize < 60) {
		tmd3 |= TX_BUFF | TX_UFLO;
		tmd1 |= TX_ERR;
		csr[0] &= ~CSR0_TXON;
		write_log (_T("7990: TRANSMIT UNDERFLOW %d\n"), outsize);
		err = 1;
		put_ram_word(off + 2, tmd1);
		put_ram_word(off + 6, tmd3);
	}

	if (!err) {
		uae_u8 *d = transmitbuffer;
		if ((am_mode & MODE_DTCR) && !add_fcs)
			outsize -= 4; // do not include checksum bytes
		if (log_a2065 && log_transmit) {
			write_log (_T("7990->DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X S=%d ADDR=%04X\n"),
				d[0], d[1], d[2], d[3], d[4], d[5],
				d[6], d[7], d[8], d[9], d[10], d[11],
				(d[12] << 8) | d[13], outsize, bufaddr);
		}
		transmitlen = outsize;
		if (mungepacket (d, transmitlen)) {
			if (log_a2065 && log_transmit) {
				write_log (_T("7990*>DST:%02X.%02X.%02X.%02X.%02X.%02X SRC:%02X.%02X.%02X.%02X.%02X.%02X E=%04X S=%d\n"),
					d[0], d[1], d[2], d[3], d[4], d[5],
					d[6], d[7], d[8], d[9], d[10], d[11],
					(d[12] << 8) | d[13], outsize);
			}
		}
		ethernet_trigger (td, sysdata);
	}
	csr[0] |= CSR0_TINT;
	devices_rethink_all(rethink_a2065);
}

static void check_transmit(bool tdmd)
{
	if (transmitlen > 0)
		return;
	if (!(csr[0] & CSR0_TXON))
		return;
	if (AM79C960 && !tdmd && (csr[4] & 0x1000)) // DPOLL
		return;
	transmitnow = 0;
	do_transmit ();
}

static void a2065_hsync_handler(void)
{
	static int cnt;

	cnt--;
	if (cnt < 0 || transmitnow) {
		check_transmit(false);
		cnt = 15;
	}
}

static void chip_init_mask(void)
{
	am_rdr_rdra &= RAM_MASK;
	am_tdr_tdra &= RAM_MASK;
	tdr_offset = rdr_offset = 0;
}

static void chip_init2(void)
{
	uae_u32 iaddr = ((csr[2] & 0xff) << 16) | csr[1];

	prom = (am_mode & MODE_PROM) ? 1 : 0;
	fakeprom = a2065_promiscuous ? 1 : 0;

	write_log(_T("7990: %04X %06X %d %d %d %d %06X %06X %02X:%02X:%02X:%02X:%02X:%02X\n"),
		am_mode, iaddr, prom, fakeprom, am_rdr_rlen, am_tdr_tlen, am_rdr_rdra, am_tdr_tdra,
		fakemac[0], fakemac[1], fakemac[2], fakemac[3], fakemac[4], fakemac[5]);

	chip_init_mask();

	ethernet_close(td, sysdata);
	if (td != NULL) {
		if (!sysdata)
			sysdata = xcalloc(uae_u8, ethernet_getdatalenght(td));
		if (!ethernet_open(td, sysdata, NULL, gotfunc, getfunc, prom || fakeprom, fakemac)) {
			write_log(_T("7990: failed to initialize winpcap driver\n"));
		}
	}

}

static void chip_init (void)
{
	uae_u32 iaddr = ((csr[2] & 0xff) << 16) | csr[1];
	int off = iaddr & RAM_MASK;

	write_log (_T("7990: Initialization block2:\n"));
	for (int i = 0; i < 24; i++)
		write_log (_T(".%02X"), get_ram_byte(off + i));
	write_log (_T("\n"));

	am_mode = get_ram_word(off + 0);
	am_ladrf = (((uae_u64)get_ram_word(off + 14)) << 48) | (((uae_u64)get_ram_word(off + 12)) << 32) | (((uae_u64)get_ram_word(off + 10)) << 16) | get_ram_word(off + 8);
	am_rdr = (get_ram_word(off + 18) << 16) | get_ram_word(off + 16);
	am_tdr = (get_ram_word(off + 22) << 16) | get_ram_word(off + 20);

	am_rdr_rlen = 1 << ((am_rdr >> 29) & 7);
	am_tdr_tlen = 1 << ((am_tdr >> 29) & 7);
	am_rdr_rdra = am_rdr & 0x00fffff8;
	am_tdr_tdra = am_tdr & 0x00fffff8;

	fakemac[0] = get_ram_byte(3);
	fakemac[1] = get_ram_byte(2);
	fakemac[2] = get_ram_byte(5);
	fakemac[3] = get_ram_byte(4);
	fakemac[4] = get_ram_byte(7);
	fakemac[5] = get_ram_byte(6);

	chip_init2();
}

static uae_u16 chip_wget (uaecptr addr)
{
	if (addr == addr_rap) {
		return rap;
	} else if (addr == addr_rdp) {

		if (rap >= RAP_SIZE)
			return 0;

		uae_u16 v = csr[rap];
		switch(rap)
		{
			case 0:
			if (v & (CSR0_BABL | CSR0_CERR | CSR0_MISS | CSR0_MERR))
				v |= CSR0_ERR;
			break;
			// chip id
			case 88:
			v = 1 << (28 - 16);
			break;
			case 89:
			v = 0x3003;
			break;
		}
		if (log_a2065 > 2)
			write_log (_T("7990_CHIPWGET: CSR%d=%04X PC=%08X\n"), rap, v, M68K_GETPC);
		return v;
	}
	return 0xffff;
}

static void chip_wput (uaecptr addr, uae_u16 v)
{

	if (addr == addr_rap) {

		rap = v & rap_mask;

	} else if (addr == addr_rdp) {

		if (rap >= RAP_SIZE)
			return;

		uae_u16 oreg = csr[rap];
		uae_u16 t;

		if (log_a2065 > 2)
			write_log (_T("7990_CHIPWPUT: CSR%d=%04X PC=%08X\n"), rap, v & 0xffff, M68K_GETPC);

		switch (rap)
		{
		case 0:
			csr[0] &= ~CSR0_INEA; csr[0] |= v & CSR0_INEA;
			// bit = 1 -> set, bit = 0 -> nop
			t = v & (CSR0_INIT | CSR0_STRT | CSR0_STOP | CSR0_TDMD);
			csr[0] |= t;
			// bit = 1 -> clear, bit = 0 -> nop
			t = v & (CSR0_IDON | CSR0_TINT | CSR0_RINT | CSR0_MERR | CSR0_MISS | CSR0_CERR | CSR0_BABL);
			csr[0] &= ~t;
			csr[0] &= ~CSR0_ERR;

			if ((csr[0] & CSR0_STOP) && !(oreg & CSR0_STOP)) {

				csr[0] = CSR0_STOP;
				if (log_a2065)
					write_log (_T("7990: STOP. %04X -> %04X -> %04X\n"), oreg, v, csr[0]);
				csr[3] = 0;
				dbyteswap = 0;

			} else if ((csr[0] & CSR0_STRT) && !(oreg & CSR0_STRT) && (oreg & (CSR0_STOP | CSR0_INIT))) {

				csr[0] &= ~CSR0_STOP;
				if (!(am_mode & MODE_DTX))
					csr[0] |= CSR0_TXON;
				if (!(am_mode & MODE_DRX))
					csr[0] |= CSR0_RXON;
				if ((csr[0] & CSR0_INIT) && !(oreg & CSR0_INIT)) {
					chip_init ();
					csr[0] |= CSR0_IDON;
					am_initialized = 1;
					if (log_a2065)
						write_log (_T("7990: INIT+START. %04X -> %04X -> %04X\n"), oreg, v, csr[0]);
				}
				if (log_a2065)
					write_log (_T("7990: START. %04X -> %04X -> %04X\n"), oreg, v, csr[0]);
			
			} else if ((csr[0] & CSR0_INIT) && !(oreg & CSR0_INIT) && (oreg & CSR0_STOP)) {

				chip_init ();
				csr[0] |= CSR0_IDON;
				csr[0] &= ~(CSR0_RXON | CSR0_TXON | CSR0_STOP);
				am_initialized = 1;
				csr[3] = 0;
				if (log_a2065)
					write_log (_T("7990: INIT. %04X -> %04X -> %04X\n"), oreg, v, csr[0]);
			}

			if ((csr[0] & CSR0_STRT)) {
				if (am_initialized) {
					if (csr[0] & CSR0_TDMD)
						check_transmit(true);
				} else if (AM79C960) {
					chip_init2();
					am_initialized = 1;
					if (csr[0] & CSR0_TDMD)
						check_transmit(true);
				}
			}
			csr[0] &= ~CSR0_TDMD;

			devices_rethink_all(rethink_a2065);
			break;
		case 1:
			if (csr[0] & 4) {
				csr[1] = v;
				csr[1] &= ~1;
			}
			break;
		case 2:
			if (csr[0] & 4) {
				csr[2] = v;
				csr[2] &= 0x00ff;
			}
			break;
		case 3:
			if ((csr[0] & 4) || AM79C960) {
				csr[3] = v;
				if (AM79C960)
					csr[3] &= 0x4000 | 0x1000 | 0x800 | 0x400 | 0x200 | 0x100 | 0x10 | 8;
				else
					csr[3] &= 7;
			}
			dbyteswap = 0;
			/*
			 * Some drivers set this but only work if no byteswapping
			 * is done. Weird..
			 * dbyteswap = (csr[3] & CSR3_BSWP) ? 1 : 0;
			*/
			break;

		// Am79C960 extra

		// interrupt masks
		case 4:
			v &= ~(0x80 | 0x40);
			break;
		// logical address filter
		case 8:
			am_ladrf &= 0x0000ffffffffffff;
			am_ladrf |= (uae_u64)v << 48;
			break;
		case 9:
			am_ladrf &= 0xffff0000ffffffff;
			am_ladrf |= (uae_u64)v << 32;
			break;
		case 10:
			am_ladrf &= 0xffffffff0000ffff;
			am_ladrf |= v << 16;
			break;
		case 11:
			am_ladrf &= 0xffffffffffff0000;
			am_ladrf |= v << 0;
			break;
		// physical address
		case 12:
			fakemac[1] = v >> 8;
			fakemac[0] = v >> 0;
			break;
		case 13:
			fakemac[3] = v >> 8;
			fakemac[2] = v >> 0;
			break;
		case 14:
			fakemac[5] = v >> 8;
			fakemac[4] = v >> 0;
			break;
		// mode
		case 15:
			am_mode = v;
			break;
		// receive descriptor ring pointer
		case 25:
			am_rdr_rdra &= 0x0000ffff;
			am_rdr_rdra |= v << 16;
			chip_init_mask();
			break;
		case 24:
			am_rdr_rdra &= 0xffff0000;
			am_rdr_rdra |= v;
			chip_init_mask();
			break;
		// transmit descriptor ring pointer
		case 31:
			am_tdr_tdra &= 0x0000ffff;
			am_tdr_tdra |= v << 16;
			chip_init_mask();
			break;
		case 30:
			am_tdr_tdra &= 0xffff0000;
			am_tdr_tdra |= v;
			chip_init_mask();
			break;

		// receive descriptor ring length
		case 76:
			am_rdr_rlen = (-v) & 0xffff;
			break;
		// transmit descriptor ring length
		case 78:
			am_tdr_tlen = (-v) & 0xffff;
			break;
		}
		if (rap >= 4)
			csr[rap] = v;
	}
}


static uae_u8 a2065_bget2 (uaecptr addr)
{
	uae_u8 v = 0;

	if (addr >= RAM_OFFSET) {
		v = boardram[(addr & RAM_MASK)];
	}
	return v;
}

static void a2065_bput2 (uaecptr addr, uae_u32 v)
{
	if (addr >= RAM_OFFSET) {
		boardram[(addr & RAM_MASK)] = v;
	}
}

static uae_u32 REGPARAM2 a2065_wget (uaecptr addr)
{
	uae_u16 v = 0;
	addr &= 65535;

	switch (romtype) {
		case ROMTYPE_A2065:
		if (addr == A2065_CHIP_OFFSET || addr == A2065_CHIP_OFFSET + 2) {
			v = chip_wget(addr);
		} else {
			v = a2065_bget2(addr + 0) << 8;
			v |= a2065_bget2(addr + 1);
		}
		break;
		case ROMTYPE_ARIADNE:
		if (addr == ARIADNE_CHIP_OFFSET || addr == ARIADNE_CHIP_OFFSET + 2) {
			v = chip_wget(addr);
		} else {
			v = a2065_bget2(addr + 0) << 8;
			v |= a2065_bget2(addr + 1);
		}
		v = (v << 8) | (v >> 8);
		break;
	}
	if (log_a2065 > 3 && addr < MEM_MIN)
		write_log (_T("7990_WGET: %08X -> %04X PC=%08X\n"), addr, v, M68K_GETPC);
	return v;
}

static uae_u32 REGPARAM2 a2065_lget (uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = a2065_wget (addr) << 16;
	v |= a2065_wget (addr + 2);
	return v;
}

static uae_u32 REGPARAM2 a2065_bget (uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = a2065_bget2 (addr ^ abyteswap);
	if (log_a2065 > 3 && addr < MEM_MIN)
		write_log (_T("7990_BGET: %08X -> %02X PC=%08X\n"), addr, v & 0xff, M68K_GETPC);
	return v;
}

static void REGPARAM2 a2065_wput (uaecptr addr, uae_u32 w)
{
	addr &= 65535;
	w &= 0xffff;
	switch (romtype) {
		case ROMTYPE_A2065:
		if (addr == A2065_CHIP_OFFSET || addr == A2065_CHIP_OFFSET + 2) {
			chip_wput(addr, w);
		} else {
			a2065_bput2(addr, w >> 8);
			a2065_bput2(addr + 1, w);
		}
		break;
		case ROMTYPE_ARIADNE:
		uae_u16 ww = (w >> 8) | (w << 8);
		if (addr == ARIADNE_CHIP_OFFSET || addr == ARIADNE_CHIP_OFFSET + 2) {
			chip_wput(addr, ww);
		} else {
			a2065_bput2(addr + 0, ww >> 8);
			a2065_bput2(addr + 1, ww);
		}
		break;
	}
	if (log_a2065 > 3 && addr < MEM_MIN)
		write_log (_T("7990_WPUT: %08X <- %04X PC=%08X\n"), addr, w, M68K_GETPC);
}

static void REGPARAM2 a2065_lput (uaecptr addr, uae_u32 l)
{
	addr &= 65535;
	a2065_wput (addr, l >> 16);
	a2065_wput (addr + 2, l);
}

uae_u8 *REGPARAM2 a2065_xlate(uaecptr addr)
{
	if ((addr & 65535) >= RAM_OFFSET)
		return &boardram[addr & RAM_MASK];
	return default_xlate(addr);
}

int REGPARAM2 a2065_check(uaecptr a, uae_u32 b)
{
	a &= 65535;
	return a >= RAM_OFFSET && a + b < 65536;
}

static addrbank a2065_bank = {
	a2065_lget, a2065_wget, a2065_bget,
	a2065_lput, a2065_wput, a2065_bput,
	a2065_xlate, a2065_check, NULL, _T("*"), _T("7990 Ethernet"),
	a2065_lgeti, a2065_wgeti,
	ABFLAG_IO | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};

static void REGPARAM2 a2065_bput (uaecptr addr, uae_u32 b)
{
	b &= 0xff;
	addr &= 65535;
	if (log_a2065 > 3 && addr < MEM_MIN)
		write_log (_T("7990_BPUT: %08X <- %02X PC=%08X\n"), addr, b & 0xff, M68K_GETPC);
	a2065_bput2 (addr ^ abyteswap, b);
}

static uae_u32 REGPARAM2 a2065_wgeti (uaecptr addr)
{
	uae_u32 v = 0xffff;
	addr &= 65535;
	return v;
}
static uae_u32 REGPARAM2 a2065_lgeti (uaecptr addr)
{
	uae_u32 v = 0xffff;
	addr &= 65535;
	v = (a2065_wgeti (addr) << 16) | a2065_wgeti (addr + 2);
	return v;
}

static void a2065_reset(int hardreset)
{
	if (!sync_sem) {
		return;
	}

	uae_sem_wait(&sync_sem);

	am_initialized = 0;

	ethernet_close(td, sysdata);

	for (int i = 0; i < RAP_SIZE; i++)
		csr[i] = 0;
	csr[0] = CSR0_STOP;
	csr[1] = csr[2] = csr[3] = 0;
	csr[4] = 0x0115;
	dbyteswap = 0;
	rap = 0;

	free_expansion_bank(&a2065_bank);
	boardram = NULL;
	xfree(sysdata);
	sysdata = NULL;
	td = NULL;

	uae_sem_post(&sync_sem);
}

static void a2065_free(void)
{
	a2065_reset(1);
}

static bool a2065_config (struct autoconfig_info *aci)
{
	uae_u8 maco[3];

	if (!sync_sem) {
		uae_sem_init(&sync_sem, 0, 1);
	}

	if (!aci) {
		return false;
	}

	device_add_reset(a2065_reset);

	if (aci->postinit) {
		configured = expamem_board_pointer >> 16;
		return true;
	}

	memcpy(aci->autoconfig_bytes, aci->ert->autoconfig, sizeof aci->autoconfig_bytes);
	romtype = aci->ert->romtype & ROMTYPE_MASK;
	switch (romtype)
	{
		case ROMTYPE_A2065:
		// 0x00 0x80 0x10 = Commodore MAC range, A2065 drivers expect it.
		maco[0] = 0x00;
		maco[1] = 0x80;
		maco[2] = 0x10;
		aci->label = _T("A2065");
		addr_rap = A2065_RAP;
		addr_rdp = A2065_RDP;
		rap_mask = 3;
		AM79C960 = false;
		abyteswap = 0;
		break;
		case ROMTYPE_ARIADNE:
		maco[0] = 0x00;
		maco[1] = 0x60;
		maco[2] = 0x30;
		aci->label = _T("ARIADNE");
		addr_rap = ARIADNE_RAP;
		addr_rdp = ARIADNE_RDP;
		rap_mask = 127;
		AM79C960 = true;
		abyteswap = 1;
		break;
		default:
		return false;
	}

	td = NULL;
	if (ethernet_enumerate (&td, romtype)) {
		if (!ethernet_getmac(realmac, aci->rc->configtext)) {
			memcpy (realmac, td->mac, sizeof realmac);
		}
		memcpy(realmac, maco, 3);
		if (aci->doinit)
			write_log (_T("7990: '%s' %02X:%02X:%02X:%02X:%02X:%02X\n"),
				td->name, realmac[0], realmac[1], realmac[2], realmac[3], realmac[4], realmac[5]);
	} else {
		memcpy(realmac, maco, 3);
		realmac[3] = 4;
		realmac[4] = 3;
		realmac[5] = 2;
		if (aci->doinit)
			write_log (_T("7990: Disconnected mode %02X:%02X:%02X:%02X:%02X:%02X\n"),
				realmac[0], realmac[1], realmac[2], realmac[3], realmac[4], realmac[5]);
	}

	aci->autoconfig_bytes[6] = realmac[2];
	aci->autoconfig_bytes[7] = realmac[3];
	aci->autoconfig_bytes[8] = realmac[4];
	aci->autoconfig_bytes[9] = realmac[5];

	memcpy(fakemac, maco, 3);
	fakemac[3] = realmac[3];
	fakemac[4] = realmac[4];
	fakemac[5] = realmac[5];

	aci->addrbank = &a2065_bank;
	aci->autoconfig_automatic = true;
	aci->autoconfigp = aci->autoconfig_bytes;
	if (!aci->doinit)
		return true;

	alloc_expansion_bank(&a2065_bank, aci);
	boardram = a2065_bank.baseaddr + RAM_OFFSET;

	device_add_hsync(a2065_hsync_handler);
	device_add_rethink(rethink_a2065);
	device_add_exit(a2065_free, NULL);

	return true;
}

uae_u8 *save_a2065 (size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak,*dst;

	if (!is_board_enabled(&currprefs, ROMTYPE_A2065, 0))
		return NULL;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = (uae_u8*)malloc (16);
	save_u32 (0);
	save_u8 (configured);
	for (int i = 0; i < 6; i++)
		save_u8 (realmac[i]);
	*len = dst - dstbak;
	return dstbak;
}
uae_u8 *restore_a2065 (uae_u8 *src)
{
	restore_u32 ();
	configured = restore_u8 ();
	for (int i = 0; i < 6; i++)
		realmac[i] = restore_u8 ();
	return src;
}

void restore_a2065_finish (void)
{
	if (configured)
		a2065_config(NULL);
}

bool a2065_init (struct autoconfig_info *aci)
{
	configured = 0;
	return a2065_config(aci);
}

bool ariadne_init(struct autoconfig_info *aci)
{
	configured = 0;
	return a2065_config(aci);
}

#endif /* A2065 */
