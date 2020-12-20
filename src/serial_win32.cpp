/*
* UAE - The Un*x Amiga Emulator
*
*  Serial Line Emulation
*
* (c) 1996, 1997 Stefan Reinauer <stepan@linux.de>
* (c) 1997 Christian Schmitt <schmitt@freiburg.linux.de>
*
*/

#include "sysconfig.h"
#ifdef SERIAL_ENET
#include "enet/enet.h"
#endif
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "cia.h"
#include "serial.h"

#include "parser.h"

#define SERIALLOGGING 0
#define SERIALDEBUG 0 /* 0, 1, 2 3 */
#define SERIALHSDEBUG 0
#define SERIAL_HSYNC_BEFORE_OVERFLOW 200
#define SERIAL_BREAK_DELAY (20 * maxvpos)
#define SERIAL_BREAK_TRANSMIT_DELAY 4

#ifndef AMIBERRY
#define SERIAL_MAP
#endif

#ifdef SERIAL_MAP
#define SERMAP_SIZE 256
struct sermap_buffer
{
	volatile ULONG version;
	volatile uae_u32 active_read;
	volatile uae_u32 active_write;
	volatile uae_u32 read_offset;
	volatile uae_u32 write_offset;
	volatile uae_u16 data[SERMAP_SIZE];
};
static struct sermap_buffer *sermap1, *sermap2;
static HANDLE sermap_handle;
static uae_u8 *sermap_data;
static bool sermap_master;
static bool sermap_enabled;

#define SER_MEMORY_MAPPING _T("WinUAE_Serial")

static void shmem_serial_send(uae_u16 data)
{
	uae_u32 v;

	sermap1->active_write = true;
	if (!sermap1->active_read)
		return;
	v = sermap1->write_offset;
	if (((v + 1) & (SERMAP_SIZE - 1)) == sermap1->read_offset) {
		write_log(_T("Shared serial port memory overflow!\n"));
		return;
	}
	sermap1->data[v] = data;
	v++;
	v &= (SERMAP_SIZE - 1);
	sermap1->write_offset = v;
}
static uae_u16 shmem_serial_receive(void)
{
	uae_u32 v;
	uae_u16 data;
	sermap2->active_read = true;
	if (!sermap2->active_write)
		return 0xffff;
	v = sermap2->read_offset;
	if (v == sermap2->write_offset)
		return 0xffff;
	data = sermap2->data[v];
	v++;
	v &= (SERMAP_SIZE - 1);
	sermap2->read_offset = v;
	return data;
}

static void sermap_deactivate(void)
{
	sermap_enabled = false;
	if (sermap1) {
		sermap1->active_write = 0;
		sermap1->write_offset = sermap1->read_offset;
	}
	if (sermap2) {
		sermap2->active_read = 0;
		sermap2->read_offset = sermap2->write_offset;
	}
}

int shmem_serial_state(void)
{
	if (!sermap_handle)
		return 0;
	if (sermap_master)
		return 1;
	return 2;
}

void shmem_serial_delete(void)
{
	sermap_deactivate();
	sermap_master = false;
	if (sermap_data)
		UnmapViewOfFile(sermap_data);
	if (sermap_handle)
		CloseHandle(sermap_handle);
	sermap_data = NULL;
	sermap_handle = NULL;
	sermap1 = sermap2 = NULL;
}

bool shmem_serial_create(void)
{
	shmem_serial_delete();
	sermap_handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SER_MEMORY_MAPPING);
	if (!sermap_handle) {
		sermap_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(struct sermap_buffer) * 2, SER_MEMORY_MAPPING);
		if (!sermap_handle) {
			write_log(_T("Failed to create shared serial port memory: %d\n"), GetLastError());
			return false;
		}
		sermap_master = true;
		write_log(_T("Created internal serial port shared memory\n"));
	} else {
		write_log(_T("Found already existing serial port shared memory\n"));
	}
	sermap_data = (uae_u8*)MapViewOfFile(sermap_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct sermap_buffer) * 2);
	if (!sermap_data) {
		write_log(_T("Shared serial port memory MapViewOfFile() failed: %d\n"), GetLastError());
		return false;
	}
	if (sermap_master) {
		sermap1 = (struct sermap_buffer*)sermap_data;
		sermap2 = (struct sermap_buffer*)(sermap_data + sizeof(struct sermap_buffer));
		sermap1->version = version;
		sermap2->version = version;
	} else {
		sermap2 = (struct sermap_buffer*)sermap_data;
		sermap1 = (struct sermap_buffer*)(sermap_data + sizeof(struct sermap_buffer));
		if (sermap2->version != version || sermap1->version != version) {
			write_log(_T("Shared serial port memory version mismatch %08x != %08x\n"), sermap1->version, version);
			shmem_serial_delete();
			return false;
		}
	}
	return true;
}

#endif

static int data_in_serdat; /* new data written to SERDAT */
static int data_in_serdatr; /* new data received */
static int data_in_sershift; /* data transferred from SERDAT to shift register */
static int break_in_serdatr; /* break state */
static int break_delay;
static uae_u16 serdatshift; /* serial shift register */
static uae_u16 serdatshift_masked; /* stop bit masked */
static int ovrun;
static int dtr;
static int serial_period_hsyncs, serial_period_hsync_counter;
static int ninebit;
static int lastbitcycle_active_hsyncs;
static bool gotlogwrite;
static unsigned int lastbitcycle;
static int serial_recv_previous, serial_send_previous;
static int serdatr_last_got;
int serdev;
int seriallog = 0, log_sercon = 0;
int serial_enet;
static bool seriallog_lf;
extern int consoleopen;

void serial_open (void);
void serial_close (void);

uae_u16 serper, serdat, serdatr;

static const int allowed_baudrates[] =
{ 0, 110, 300, 600, 1200, 2400, 4800, 9600, 14400,
19200, 31400, 38400, 57600, 115200, 128000, 256000, -1 };

void SERPER (uae_u16 w)
{
	int baud = 0, i, per;
	static int warned;

	if (serper == w)  /* don't set baudrate if it's already ok */
		return;

	ninebit = 0;
	serper = w;
	if (w & 0x8000)
		ninebit = 1;
	w &= 0x7fff;

	if (w < 13)
		w = 13;

	per = w;
	if (per == 0)
		per = 1;
	per = 3546895 / (per + 1);
	if (per <= 0)
		per = 1;
	i = 0;
	while (allowed_baudrates[i] >= 0 && per > allowed_baudrates[i] * 100 / 97)
		i++;
	baud = allowed_baudrates[i];

	serial_period_hsyncs = (((serper & 0x7fff) + 1) * (1 + 8 + ninebit + 1 - 1)) / maxhpos;
	if (serial_period_hsyncs <= 0)
		serial_period_hsyncs = 1;

#if SERIALLOGGING > 0
	serial_period_hsyncs = 1;
	seriallog = 1;
#endif
	if (log_sercon) {
		serial_period_hsyncs = 1;
		seriallog = log_sercon;
		seriallog_lf = true;
		write_log(_T("\n"));
	}

	serial_period_hsync_counter = 0;

	write_log (_T("SERIAL: period=%d, baud=%d, hsyncs=%d, bits=%d, PC=%x\n"), w, baud, serial_period_hsyncs, ninebit ? 9 : 8, M68K_GETPC);

	if (ninebit)
		baud *= 2;
	if (currprefs.serial_direct) {
		if (baud != 31400 && baud < 115200)
			baud = 115200;
		serial_period_hsyncs = 1;
	}
	serial_recv_previous = -1;
	serial_send_previous = -1;
#ifdef SERIAL_PORT
	setbaud (baud);
#endif
}

static TCHAR docharlog(int v)
{
	v &= 0xff;
	if (v >= 32 && v < 127)
		return v;
	if (v == 10)
		return 10;
	return '.';
}

static TCHAR dochar(int v)
{
	v &= 0xff;
	if (v >= 32 && v < 127)
		return v;
	return '.';
}

static bool canreceive(void)
{
	if (!data_in_serdatr)
		return true;
	if (currprefs.serial_direct)
		return false;
	if (currprefs.cpu_cycle_exact)
		return true;
	if (serdatr_last_got > SERIAL_HSYNC_BEFORE_OVERFLOW) {
#if SERIALDEBUG > 0
		write_log(_T("SERIAL: OVERRUN\n"));
#endif
		flushser();
		ovrun = true;
		data_in_serdatr = 0;
		serdatr_last_got = 0;
		return true;
	}
	return false;
}

static void checkreceive_enet (void)
{
#ifdef SERIAL_ENET
	uae_u16 recdata;

	if (!canreceive())
		return;
	if (!enet_readseravail ())
		return;
	if (!enet_readser (&recdata))
		return;
	serdatr = recdata & 0x1ff;
	if (recdata & 0x200)
		serdatr |= 0x200;
	else
		serdatr |= 0x100;
	data_in_serdatr = 1;
	serdatr_last_got = 0;
	serial_check_irq ();
#if SERIALDEBUG > 2
	write_log (_T("SERIAL: received %02X (%c)\n"), serdatr & 0xff, dochar (serdatr));
#endif
#endif
}

static void checkreceive_serial (void)
{
#ifdef SERIAL_PORT
	static int ninebitdata;
	int recdata;

	if (!canreceive())
		return;

	if (ninebit) {
		bool breakcond;
		int status = readseravail(&breakcond);
		if (break_in_serdatr == -1 || break_in_serdatr > 0) {
			serial_recv_previous = 0;
			serdatr = 0;
			if (break_in_serdatr < 0) {
				break_in_serdatr = SERIAL_BREAK_DELAY;
				break_delay = SERIAL_BREAK_TRANSMIT_DELAY;
			}
			if (break_in_serdatr == 1) {
				serdatr |= 0x200;
				break_in_serdatr = 0;
			}
			break_delay--;
			if (break_delay && break_in_serdatr) {
				return;
			}
			break_delay = SERIAL_BREAK_TRANSMIT_DELAY;
		} else {
			if (breakcond && !break_in_serdatr) {
				break_in_serdatr = -1;
				break_in_serdatr -= status;
				if (break_in_serdatr == -1)
					return;
			}
			if (status <= 0) {
				return;
			}
			for (;;) {
				status = readser(&recdata);
				if (!status)
					return;
				if (break_in_serdatr > 0) {
					break_in_serdatr = 0;
				}
				if (ninebitdata) {
					serdatr = (ninebitdata & 1) << 8;
					serdatr |= recdata;
					serdatr |= 0x200;
					ninebitdata = 0;
					if (break_in_serdatr < -1) {
						break_in_serdatr++;
					}
					break;
				} else {
					ninebitdata = recdata;
					if ((ninebitdata & ~1) != 0xa8) {
						write_log(_T("SERIAL: 9-bit serial emulation sync lost, %02X != %02X\n"), ninebitdata & ~1, 0xa8);
						ninebitdata = 0;
						return;
					}
					continue;
				}
			}
		}
	} else {
		bool breakcond;
		int status = readseravail(&breakcond);
		if (break_in_serdatr == -1 || break_in_serdatr > 0) {
			// break: stop bit is zero
			// Paula for some reason keeps receiving zeros continuously in break condition.
			serial_recv_previous = 0;
			serdatr = 0;
			if (break_in_serdatr < 0) {
				break_in_serdatr = SERIAL_BREAK_DELAY;
				break_delay = SERIAL_BREAK_TRANSMIT_DELAY;
#if SERIALDEBUG
				write_log(_T("SERIAL: BREAK START\n"));
#endif
			}
			// serial.device requires valid serial word before it finally returns break error
			if (break_in_serdatr == 1) {
				serdatr |= 0x100;
#if SERIALDEBUG
				write_log(_T("SERIAL: BREAK COMPLETE\n"));
#endif
				break_in_serdatr = 0;
			}
			break_delay--;
			if (break_delay && break_in_serdatr) {
				return;
			}
			break_delay = SERIAL_BREAK_TRANSMIT_DELAY;
		} else {
			if (breakcond && !break_in_serdatr) {
				break_in_serdatr = -1;
#if SERIALDEBUG
				write_log(_T("SERIAL: BREAK DETECT (%d buffered)\n"), status);
#endif
				break_in_serdatr -= status;
				if (break_in_serdatr == -1)
					return;
			}
			if (status <= 0) {
				return;
			}
			if (!readser(&recdata))
				return;
			if (break_in_serdatr > 0) {
#if SERIALDEBUG
				write_log(_T("SERIAL: BREAK END\n"));
#endif
				break_in_serdatr = 0;
			}
			if (currprefs.serial_crlf) {
				if (recdata == 0 || (serial_recv_previous == 13 && recdata == 10)) {
					//write_log(_T(" [%02X] "), (uae_u8)recdata);
					serial_recv_previous = -1;
					return;
				}
			}
			//write_log(_T(" %02X "), (uae_u8)recdata);
			serial_recv_previous = recdata;
			serdatr = recdata;
			serdatr |= 0x0100;
			if (break_in_serdatr < -1) {
				break_in_serdatr++;
			}
		}
	}

	data_in_serdatr = 1;
	serdatr_last_got = 0;
	serial_check_irq();
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: received %02X (%c)\n"), serdatr & 0xff, dochar(serdatr));
#endif
#endif
}


static void serdatcopy(void);

static void checksend(void)
{
	if (data_in_sershift != 1 && data_in_sershift != 2)
		return;

#ifdef ARCADIA
	if (alg_flag || currprefs.genlock_image >= 7) {
		ld_serial_read(serdatshift);
	}
#endif
#if 0
	if (cubo_enabled) {
		touch_serial_read(serdatshift);
	}
#endif
#ifdef SERIAL_MAP
	if (sermap_data && sermap_enabled)
		shmem_serial_send(serdatshift);
#endif
#ifdef SERIAL_ENET
	if (serial_enet) {
		enet_writeser(serdatshift);
	}
#endif
#ifdef SERIAL_PORT
	if (ninebit) {
		if (!checkserwrite(2)) {
			data_in_sershift = 2;
			return;
		}
		writeser(((serdatshift >> 8) & 1) | 0xa8);
		writeser(serdatshift_masked);
	} else {
		if (currprefs.serial_crlf) {
			if (serdatshift_masked == 10 && serial_send_previous != 13) {
				if (!checkserwrite(2)) {
					data_in_sershift = 2;
					return;
				}
				writeser(13);
			}
		}
		if (!checkserwrite(1)) {
			data_in_sershift = 2;
			return;
		}
		writeser(serdatshift_masked);
		serial_send_previous = serdatshift_masked;
	}
#endif
	if (serial_period_hsyncs <= 1 || data_in_sershift == 2) {
		data_in_sershift = 0;
		serdatcopy();
	} else {
		data_in_sershift = 3;
	}
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: send %04X (%c)\n"), serdatshift, dochar(serdatshift));
#endif
}

static bool checkshiftempty(void)
{
	writeser_flush();
	checksend();
	if (data_in_sershift == 3) {
		data_in_sershift = 0;
		serdatcopy();
		return true;
	}
	return false;
}

static void sersend_ce(uae_u32 v)
{
	if (checkshiftempty()) {
		lastbitcycle = get_cycles() + ((serper & 0x7fff) + 1) * CYCLE_UNIT;
		lastbitcycle_active_hsyncs = ((serper & 0x7fff) + 1) / maxhpos + 2;
	} else if (data_in_sershift == 1 || data_in_sershift == 2) {
		event2_newevent_x_replace(maxhpos, 0, sersend_ce);
	}
}

static void serdatcopy(void)
{
	int bits;

	if (data_in_sershift || !data_in_serdat)
		return;
	serdatshift = serdat;
	bits = 8;
	if ((serdatshift & 0xff80) == 0x80) {
		bits = 7;
	}
	serdatshift_masked = serdatshift & ((1 << bits) - 1);
	data_in_sershift = 1;
	data_in_serdat = 0;
#if 0
	if (seriallog > 0 || (consoleopen && seriallog < 0)) {
		gotlogwrite = true;
		if (seriallog_lf && seriallog > 2) {
			TCHAR *ts = write_log_get_ts();
			if (ts)
				write_logx(_T("%s:"), ts);
			seriallog_lf = false;
		}
		TCHAR ch = docharlog(serdatshift_masked);
		write_logx(_T("%c"), ch);
		if (ch == 10)
			seriallog_lf = true;
	}

	if (serper == 372) {
		if (enforcermode & 2) {
			console_out_f(_T("%c"), docharlog(serdatshift_masked));
		}
	}
#endif
	// if someone uses serial port as some kind of timer..
	if (currprefs.cpu_memory_cycle_exact) {
		int per;

		bits = 16 + 1;
		for (int i = 15; i >= 0; i--) {
			if (serdatshift & (1 << i))
				break;
			bits--;
		}
		// assuming when last bit goes out, transmit buffer
		// becomes empty, not when last bit has finished
		// transmitting.
		per = ((serper & 0x7fff) + 1) * (bits - 1);
		if (lastbitcycle_active_hsyncs) {
			// if last bit still transmitting, add remaining time.
			int extraper = lastbitcycle - get_cycles();
			if (extraper > 0)
				per += extraper / CYCLE_UNIT;
		}
		if (per < 4)
			per = 4;
		event2_newevent_x_replace(per, 0, sersend_ce);
	}

	INTREQ(0x8000 | 0x0001);
	checksend();
}

void serial_hsynchandler (void)
{
#ifdef AHI
	hsyncstuff();
#endif
#ifdef ARCADIA
	if ((alg_flag || currprefs.genlock_image >= 7) && !data_in_serdatr) {
		int ch = ld_serial_write();
		if (ch >= 0) {
			serdatr = ch | 0x100;
			data_in_serdatr = 1;
			serdatr_last_got = 0;
			serial_check_irq ();
		}
	}
#endif
#if 0
	if (cubo_enabled && !data_in_serdatr) {
		int ch = touch_serial_write();
		if (ch >= 0) {
			serdatr = ch | 0x100;
			data_in_serdatr = 1;
			serdatr_last_got = 0;
			serial_check_irq();
		}
	}
	if (seriallog > 1 && !data_in_serdatr && gotlogwrite) {
		int ch = read_log();
		if (ch > 0) {
			serdatr = ch | 0x100;
			data_in_serdatr = 1;
			serial_check_irq ();
		}
	}
#endif
	if (lastbitcycle_active_hsyncs > 0)
		lastbitcycle_active_hsyncs--;
#ifdef SERIAL_MAP
	if (sermap2 && sermap_enabled && !data_in_serdatr) {
		uae_u16 v = shmem_serial_receive();
		if (v != 0xffff) {
			serdatr = v;
			data_in_serdatr = 1;
			serial_check_irq();
		}
	}
#endif
	if (data_in_serdatr)
		serdatr_last_got++;
	if (serial_period_hsyncs == 0)
		return;
	serial_period_hsync_counter++;
	if (serial_period_hsyncs == 1 || (serial_period_hsync_counter % (serial_period_hsyncs - 1)) == 0) {
		checkreceive_serial();
		checkreceive_enet();
		checkshiftempty();
	} else if ((serial_period_hsync_counter % serial_period_hsyncs) == 0 && !currprefs.cpu_cycle_exact) {
		checkshiftempty();
	}
	if (break_in_serdatr > 1) {
		break_in_serdatr--;
		if (break_in_serdatr == 1) {
#if SERIALDEBUG
			write_log(_T("SERIAL: BREAK TIMEOUT\n"));
#endif
			flushser();
		}
	}
}

void SERDAT (uae_u16 w)
{
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: SERDAT write 0x%04x (%c) PC=%x\n"), w, dochar(w), M68K_GETPC);
#endif

	serdatcopy();

	serdat = w;

	if (!w) {
#if SERIALDEBUG > 1
		write_log (_T("SERIAL: zero serial word written?! PC=%x\n"), M68K_GETPC);
#endif
		return;
	}

#if SERIALDEBUG > 1
	if (data_in_serdat) {
		write_log (_T("SERIAL: program wrote to SERDAT but old byte wasn't fetched yet\n"));
	}
#endif

	data_in_serdat = 1;
	serdatcopy();
}

uae_u16 SERDATR(void)
{
	serdatr &= 0x03ff;
	if (!data_in_serdat) {
		serdatr |= 0x2000;
	}
	if (!data_in_sershift) {
		serdatr |= 0x1000;
	}
	if (data_in_serdatr) {
		serdatr |= 0x4000;
	}
	if (ovrun) {
		serdatr |= 0x8000;
	}
	if (break_in_serdatr <= 0) {
		serdatr |= 0x0800;
	}
#if SERIALDEBUG > 2
	write_log (_T("SERIAL: read 0x%04x (%c) %x\n"), serdatr, dochar (serdatr), M68K_GETPC);
#endif
	data_in_serdatr = 0;
	return serdatr;
}

void serial_rbf_clear(void)
{
	ovrun = 0;
}

void serial_check_irq (void)
{
	// Data in receive buffer
	if (data_in_serdatr)
		INTREQ_0(0x8000 | 0x0800);
}

void serial_dtr_on (void)
{
#if SERIALHSDEBUG > 0
	write_log ( "SERIAL: DTR on\n" );
#endif
	dtr = 1;
	if (currprefs.serial_demand)
		serial_open ();
#ifdef SERIAL_PORT
	setserstat(TIOCM_DTR, dtr);
#endif
}

void serial_dtr_off (void)
{
#if SERIALHSDEBUG > 0
	write_log ( "SERIAL: DTR off\n" );
#endif
	dtr = 0;
#ifdef SERIAL_PORT
	if (currprefs.serial_demand)
		serial_close ();
	setserstat(TIOCM_DTR, dtr);
#endif
}

void serial_flush_buffer (void)
{
}

static uae_u8 oldserbits;

static void serial_status_debug(const TCHAR *s)
{
#if SERIALHSDEBUG > 1
	write_log (_T("%s: DTR=%d RTS=%d CD=%d CTS=%d DSR=%d\n"), s,
		(oldserbits & 0x80) ? 0 : 1, (oldserbits & 0x40) ? 0 : 1,
		(oldserbits & 0x20) ? 0 : 1, (oldserbits & 0x10) ? 0 : 1, (oldserbits & 0x08) ? 0 : 1);
#endif
}

uae_u8 serial_readstatus (uae_u8 dir)
{
	int status = 0;
	uae_u8 serbits = oldserbits;

#ifdef SERIAL_PORT
	getserstat (&status);
#endif
	if (!(status & TIOCM_CAR)) {
		if (!(serbits & 0x20)) {
			serbits |= 0x20;
#if SERIALHSDEBUG > 0
			write_log ( "SERIAL: CD off\n" );
#endif
		}
	} else {
		if (serbits & 0x20) {
			serbits &= ~0x20;
#if SERIALHSDEBUG > 0
			write_log ( "SERIAL: CD on\n" );
#endif
		}
	}

	if (!(status & TIOCM_DSR)) {
		if (!(serbits & 0x08)) {
			serbits |= 0x08;
#if SERIALHSDEBUG > 0
			write_log ( "SERIAL: DSR off\n" );
#endif
		}
	} else {
		if (serbits & 0x08) {
			serbits &= ~0x08;
#if SERIALHSDEBUG > 0
			write_log ( "SERIAL: DSR on\n" );
#endif
		}
	}

	if (!(status & TIOCM_CTS)) {
		if (!(serbits & 0x10)) {
			serbits |= 0x10;
#if SERIALHSDEBUG > 0
			write_log ( "SERIAL: CTS off\n" );
#endif
		}
	} else {
		if (serbits & 0x10) {
			serbits &= ~0x10;
#if SERIALHSDEBUG > 0
			write_log ( "SERIAL: CTS on\n" );
#endif
		}
	}

	serbits &= 0x08 | 0x10 | 0x20;
	oldserbits &= ~(0x08 | 0x10 | 0x20);
	oldserbits |= serbits;

	serial_status_debug (_T("read"));

	return oldserbits;
}

uae_u8 serial_writestatus (uae_u8 newstate, uae_u8 dir)
{
	static int logcnt = 10;

#ifdef SERIAL_PORT
	if (((oldserbits ^ newstate) & 0x80) && (dir & 0x80)) {
		if (newstate & 0x80)
			serial_dtr_off();
		else
			serial_dtr_on();
	}

	if (!currprefs.serial_hwctsrts && (dir & 0x40)) {
		if ((oldserbits ^ newstate) & 0x40) {
			if (newstate & 0x40) {
				setserstat (TIOCM_RTS, 0);
#if SERIALHSDEBUG > 0
				write_log (_T("SERIAL: RTS cleared\n"));
#endif
			} else {
				setserstat (TIOCM_RTS, 1);
#if SERIALHSDEBUG > 0
				write_log (_T("SERIAL: RTS set\n"));
#endif
			}
		}
	}

#if 0 /* CIA io-pins can be read even when set to output.. */
	if ((newstate & 0x20) != (oldserbits & 0x20) && (dir & 0x20))
		write_log (_T("SERIAL: warning, program tries to use CD as an output!\n"));
	if ((newstate & 0x10) != (oldserbits & 0x10) && (dir & 0x10))
		write_log (_T("SERIAL: warning, program tries to use CTS as an output!\n"));
	if ((newstate & 0x08) != (oldserbits & 0x08) && (dir & 0x08))
		write_log (_T("SERIAL: warning, program tries to use DSR as an output!\n"));
#endif

	if (logcnt > 0) {
		if (((newstate ^ oldserbits) & 0x40) && !(dir & 0x40)) {
			write_log (_T("SERIAL: warning, program tries to use RTS as an input! PC=%x\n"), M68K_GETPC);
			logcnt--;
		}
		if (((newstate ^ oldserbits) & 0x80) && !(dir & 0x80)) {
			write_log (_T("SERIAL: warning, program tries to use DTR as an input! PC=%x\n"), M68K_GETPC);
			logcnt--;
		}
	}

#endif

	oldserbits &= ~(0x80 | 0x40);
	newstate &= 0x80 | 0x40;
	oldserbits |= newstate;
	serial_status_debug (_T("write"));

	return oldserbits;
}

static int enet_is (TCHAR *name)
{
	return !_tcsnicmp (name, _T("ENET:"), 5);
}

void serial_open (void)
{
#ifdef SERIAL_PORT
	if (serdev)
		return;
	serper = 0;
	if (0) {
#ifdef SERIAL_ENET
	} else if (enet_is (currprefs.sername)) {
		enet_open (currprefs.sername);
#endif
#ifdef SERIAL_MAP
	} else if (!_tcsicmp(currprefs.sername, SERIAL_INTERNAL)) {
		sermap_enabled = true;
#endif
	} else {
		if(!openser (currprefs.sername)) {
			write_log (_T("SERIAL: Could not open device %s\n"), currprefs.sername);
			return;
		}
	}
	serdev = 1;
#endif
}

void serial_close (void)
{
#ifdef SERIAL_PORT
	closeser ();
#ifdef SERIAL_ENET
	enet_close ();
#endif
	serdev = 0;
#ifdef SERIAL_MAP
	sermap_deactivate();
#endif
#endif
}

void serial_init (void)
{
#ifdef SERIAL_PORT
	if (!currprefs.use_serial)
		return;
	if (!currprefs.serial_demand)
		serial_open ();
#endif
}

void serial_exit (void)
{
#ifdef SERIAL_PORT
	serial_close ();	/* serial_close can always be called because it	*/
#endif
	dtr = 0;		/* just closes *opened* filehandles which is ok	*/
	oldserbits = 0;	/* when exiting.				*/
}

void serial_uartbreak (int v)
{
#ifdef SERIAL_PORT
	serialuartbreak (v);
#endif
}

#ifdef SERIAL_ENET
static ENetHost *enethost, *enetclient;
static ENetPeer *enetpeer, *enet;
static int enetmode;
static uae_u16 enet_receive[256];
static int enet_receive_off_w, enet_receive_off_r;

static void enet_service (int serveronly)
{
	ENetEvent evt;
	ENetAddress address;
	int got;

	if (enetmode == 0)
		return;

	got = 1;
	while (got) {
		got = 0;
		if (enetmode > 0) {
			while (enet_host_service (enethost, &evt, 0)) {
				got = 1;
				switch (evt.type)
				{
					case ENET_EVENT_TYPE_CONNECT:
						address = evt.peer->address;
						write_log (_T("ENET_SERVER: connect from %d.%d.%d.%d:%u\n"),
							(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff,
							address.port);
						evt.peer->data = 0;
					break;
					case ENET_EVENT_TYPE_RECEIVE:
					{
						uae_u8 *p = evt.packet->data;
						int len = evt.packet->dataLength;
						if (len == 6 && !memcmp (p, "UAE_", 4)) {
							if (((enet_receive_off_w + 1) & 0xff) != enet_receive_off_r) {
								enet_receive[enet_receive_off_w++] = (p[4] << 8) | p[5];
							}
						}

						enet_packet_destroy (evt.packet);
					}
					break;
					case ENET_EVENT_TYPE_DISCONNECT:
						address = evt.peer->address;
						write_log (_T("ENET_SERVER: disconnect from %d.%d.%d.%d:%u\n"),
							(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff,
							address.port);
					break;
				}
			}
		}
		if (!serveronly) {
			while (enet_host_service (enetclient, &evt, 0)) {
				got = 1;
				switch (evt.type)
				{
					default:
					write_log (_T("ENET_CLIENT: %d\n"), evt.type);
					break;
				}
			}
		}
	}
}

static void enet_disconnect (ENetPeer *peer)
{
	ENetEvent evt;
	int cnt = 30;

	if (!peer)
		return;

	write_log (_T("ENET_CLIENT: disconnecting..\n"));
	enet_peer_disconnect (peer, 0);
	while (cnt-- > 0) {
		enet_service (1);
		while (enet_host_service (enetclient, &evt, 100) > 0)
		{
			switch (evt.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy (evt.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				write_log (_T("ENET_CLIENT: disconnection succeeded\n"));
				enetpeer = NULL;
				return;
			}
		}
	}
	write_log (_T("ENET_CLIENT: disconnection forced\n"));
	enet_peer_reset (enetpeer);
	enetpeer = NULL;
}

void enet_close (void)
{
	enet_disconnect (enetpeer);
	if (enetclient)
		enet_host_destroy (enetclient);
	enetclient = NULL;
	if (enethost)
		enet_host_destroy (enethost);
	enethost = NULL;
	serial_enet = 0;
	enetmode = 0;
}

int enet_open (TCHAR *name)
{
	ENetAddress address;
	ENetPacket *p;
	static int initialized;
	uae_u8 data[16];
	int cnt;

	if (!initialized) {
		int err = enet_initialize ();
		if (err) {
			write_log (_T("ENET: initialization failed: %d\n"), err);
			return 0;
		}
		initialized = 1;
	}

	enet_close ();
	enetmode = 0;
	if (!_tcsnicmp (name, _T("ENET:H"), 6)) {
		address.host = ENET_HOST_ANY;
		address.port = 1234;
		enethost = enet_host_create (&address, 2, 0, 0);
		if (enethost == NULL) {
			write_log (_T("ENET_SERVER: enet_host_create(server) failed\n"));
			enet_close ();
			return 0;
		}
		write_log (_T("ENET_SERVER: server created\n"));
		enetmode = 1;
	} else {
		enetmode = -1;
	}
	enetclient = enet_host_create (NULL, 1, 0, 0);
	if (enetclient == NULL) {
		write_log (_T("ENET_CLIENT: enet_host_create(client) failed\n"));
		enet_close ();
		return 0;
	}
	write_log (_T("ENET_CLIENT: client created\n"));
	enet_address_set_host (&address, enetmode > 0 ? "127.0.0.1" : "192.168.0.10");
	address.port = 1234;
	enetpeer = enet_host_connect (enetclient, &address, 2);
	if (enetpeer == NULL) {
		write_log (_T("ENET_CLIENT: connection to host %d.%d.%d.%d:%d failed\n"),
			(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff, address.port);
		enet_host_destroy (enetclient);
		enetclient = NULL;
	}
	write_log (_T("ENET_CLIENT: connecting to %d.%d.%d.%d:%d...\n"),
		(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff, address.port);
	cnt = 10 * 5;
	while (cnt-- > 0) {
		ENetEvent evt;
		enet_service (0);
		if (enet_host_service (enetclient, &evt, 100) > 0) {
			if (evt.type == ENET_EVENT_TYPE_CONNECT)
				break;
		}
	}
	if (cnt <= 0) {
		write_log (_T("ENET_CLIENT: connection failed, no response in 5 seconds\n"));
		enet_close ();
		return 0;
	}
	memcpy (data, "UAE_HELLO", 10);
	p = enet_packet_create (data, sizeof data, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send (enetpeer, 0, p);
	enet_host_flush (enetclient);
	write_log (_T("ENET: connected\n"));
	serial_enet = 1;
	return 1;
}

void enet_writeser (uae_u16 w)
{
	ENetPacket *p;
	uae_u8 data[16];

	memcpy (data, "UAE_", 4);
	data[4] = w >> 8;
	data[5] = w >> 0;
	write_log (_T("W=%04X "), w);
	p = enet_packet_create (data, 6, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send (enetpeer, 0, p);
	enet_host_flush (enetclient);
}

int enet_readseravail (void)
{
	enet_service (0);
	return enet_receive_off_r != enet_receive_off_w;
}

int enet_readser (uae_u16 *data)
{
	if (enet_receive_off_r == enet_receive_off_w)
		return 0;
	*data = enet_receive[enet_receive_off_r++];
	write_log (_T("R=%04X "), *data);
	enet_receive_off_r &= 0xff;
	return 1;
}
#endif
