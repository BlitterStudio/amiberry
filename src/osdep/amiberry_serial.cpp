/*
 * Amiberry
 *
 * Serial port implementation, using libserialport
 *
 * (c) 2023 Dimitris Panokostas <midwan@gmail.com>
 *
 */

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "picasso96.h"
#include "serial.h"
#ifdef AHI
#include "ahi_v1.h"
#endif
#include "xwin.h"
#include "drawing.h"

#include <libserialport.h>

/* A pointer to a struct sp_port, which will refer to
 * the port found. */
sp_port* port;

/* Helper function for error handling. */
extern int check(sp_return result);

static bool serloop_enabled;
static bool serempty_enabled;
static bool serxdevice_enabled;
static uae_u8 serstatus;
static bool ser_accurate;
static uae_u16* receive_buf;
static int receive_buf_size, receive_buf_count;

static bool sermap_enabled;
static int data_in_serdat; /* new data written to SERDAT */
static evt_t data_in_serdat_delay;
static evt_t serper_tx_evt;
static int serper_tx_per, serper_tx_bits, serper_tx_cycles;
static int data_in_serdatr; /* new data received */
static evt_t data_in_serdatr_evt;
static int data_in_sershift; /* data transferred from SERDAT to shift register */
static int break_in_serdatr; /* break state */
static int break_delay;
static uae_u16 serdatshift; /* serial shift register */
static int serdatshift_bits;
static uae_u16 serdatshift_masked; /* stop bit masked */
static evt_t serdatshift_start;
static int ovrun;
static int dtr;
static int serial_period_hsyncs, serial_period_hsync_counter;
static int ninebit;
static int lastbitcycle_active_hsyncs;
static unsigned int lastbitcycle;
static int serial_recv_previous, serial_send_previous;
static int serdatr_last_got;
int serdev;
int seriallog = 0, log_sercon = 0;
int serial_enet;
static bool seriallog_lf;
extern int consoleopen;

void serial_open(void);
void serial_close(void);

uae_u16 serper, serdat, serdatr;
static bool serper_set = false;

static const int allowed_baudrates[] =
{
	0, 110, 300, 600, 1200, 2400, 4800, 9600, 14400,
	19200, 31400, 38400, 57600, 115200, 128000, 256000, -1
};

#define SERIAL_INTERNAL _T("INTERNAL_SERIAL")
#define SERIAL_LOOPBACK _T("LOOPBACK_SERIAL")

#define SERIAL_HSYNC_BEFORE_OVERFLOW 200
#define SERIAL_BREAK_DELAY (20 * maxvpos)
#define SERIAL_BREAK_TRANSMIT_DELAY 4
#define SERIAL_WRITE_BUFFER 100
#define SERIAL_READ_BUFFER 100
static uae_u8 outputbuffer[SERIAL_WRITE_BUFFER];
static uae_u8 outputbufferout[SERIAL_WRITE_BUFFER];
static uae_u8 inputbuffer[SERIAL_READ_BUFFER];
static int datainoutput;
static int dataininput, dataininputcnt;
static bool breakpending = false;

/* We'll allow a 1 second timeout for send and receive. */
unsigned int timeout = 1000;

int openser (const TCHAR *sername)
{
	/* Call sp_get_port_by_name() to find the port. The port
	* pointer will be updated to refer to the port found. */
	check(sp_get_port_by_name(sername, &port));
	int result = check(sp_open(port, SP_MODE_READ_WRITE));

	check(sp_set_baudrate(port, 9600));
	check(sp_set_bits(port, 8));
	check(sp_set_parity(port, SP_PARITY_NONE));
	check(sp_set_stopbits(port, currprefs.serial_stopbits));
	if (currprefs.serial_hwctsrts)
	{
		check(sp_set_flowcontrol(port, SP_FLOWCONTROL_RTSCTS));
	}
	else
	{
		check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));
	}
	return result;
}

void closeser ()
{
	if (serdev)
	{
		check(sp_close(port));
		sp_free_port(port);
	}
}

static void serial_rx_irq(void)
{
	int delay = 9;
	// Data in receive buffer
	if (!data_in_serdatr) {
		data_in_serdatr = 1;
		data_in_serdatr_evt = get_cycles() + delay * CYCLE_UNIT;
	}
	if (ser_accurate) {
		INTREQ_INT(11, delay);
	}
	else {
		INTREQ_INT(11, 0);
	}
}

bool serreceive_external(uae_u16 v)
{
	if (data_in_serdatr) {
		if (!receive_buf) {
			receive_buf_size = 200;
			receive_buf = xcalloc(uae_u16, receive_buf_size);
			if (!receive_buf) {
				return false;
			}
		}
		if (receive_buf_count >= receive_buf_size) {
			return false;
		}
		receive_buf[receive_buf_count++] = v;
		return true;
	}
	serdatr = v;
	serial_rx_irq();
	return true;
}

static void receive_next_buffered(void)
{
	if (receive_buf && receive_buf_count > 0 && !(intreq & (1 << 11))) {
		uae_u16 v = receive_buf[0];
		receive_buf_count--;
		if (receive_buf_count > 0) {
			memmove(receive_buf, receive_buf + 1, receive_buf_count * sizeof(uae_u16));
		}
		serreceive_external(v);
	}
}

void serial_rethink(void)
{
	if (data_in_serdatr) {
		int sdr = data_in_serdatr;
		if (ser_accurate && get_cycles() > data_in_serdatr_evt) {
			sdr = 0;
		}

		// RBF bit is not "sticky" but without it data can be lost when using fast emulation modes
		// and physical serial port or internally emulated serial devices.
		if (sdr) {
			INTREQ_INT(11, 0);
		}
	}
	receive_next_buffered();
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

int readser(int* buffer)
{
	if (!currprefs.use_serial)
		return 0;
	if (dataininput > dataininputcnt) {
		*buffer = inputbuffer[dataininputcnt++];
		return 1;
	}
	dataininput = 0;
	dataininputcnt = 0;
	if (serdev) {
		const int bytes = check(sp_input_waiting(port));
		if (bytes) {
			int len = bytes;
			if (len > sizeof(inputbuffer))
				len = sizeof(inputbuffer);
			dataininput = check(sp_blocking_read(port, inputbuffer, len, timeout));
			dataininputcnt = 0;
			if(!dataininput)
			{
				return 0;
			}
			return readser(buffer);
		}
	}
	return 0;
}

void flushser(void)
{
	if (serdev) {
		check(sp_flush(port, SP_BUF_BOTH));
	}
}

int readseravail(bool* breakcond)
{
	if (breakcond)
		*breakcond = false;

	if (!currprefs.use_serial)
		return 0;
	if (dataininput > dataininputcnt)
		return 1;
	if (serdev) {
		if (breakcond && breakpending) {
			*breakcond = true;
			breakpending = false;
		}
		const int bytes = check(sp_input_waiting(port));
		if (bytes > 0)
			return bytes;
	}
	return 0;
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
		flushser();
		ovrun = true;
		data_in_serdatr = 0;
		serdatr_last_got = 0;
		return true;
	}
	return false;
}

static void checkreceive_serial(void)
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
		}
		else {
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
				}
				ninebitdata = recdata;
				if ((ninebitdata & ~1) != 0xa8) {
					write_log(_T("SERIAL: 9-bit serial emulation sync lost, %02X != %02X\n"), ninebitdata & ~1, 0xa8);
					ninebitdata = 0;
					return;
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
			}
			// serial.device requires valid serial word before it finally returns break error
			if (break_in_serdatr == 1) {
				serdatr |= 0x100;
				break_in_serdatr = 0;
			}
			break_delay--;
			if (break_delay && break_in_serdatr) {
				return;
			}
			break_delay = SERIAL_BREAK_TRANSMIT_DELAY;
		}
		else {
			if (breakcond && !break_in_serdatr) {
				break_in_serdatr = -1;
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
				break_in_serdatr = 0;
			}
			if (currprefs.serial_crlf) {
				if (recdata == 0 || (serial_recv_previous == 13 && recdata == 10)) {
					serial_recv_previous = -1;
					return;
				}
			}
			serial_recv_previous = recdata;
			serdatr = recdata;
			serdatr |= 0x0100;
			if (break_in_serdatr < -1) {
				break_in_serdatr++;
			}
		}
	}

	//data_in_serdatr = 1;
	serdatr_last_got = 0;
	serial_rx_irq();
#endif
}

static void outser(void)
{
	if (datainoutput <= 0)
		return;

	memcpy(outputbufferout, outputbuffer, datainoutput);
	check(sp_nonblocking_write(port, outputbufferout, datainoutput));
	datainoutput = 0;
}

void writeser_flush(void)
{
	outser();
}

void writeser(int c)
{
	if (!serdev || !currprefs.use_serial)
		return;
	if (datainoutput + 1 < sizeof(outputbuffer)) {
		outputbuffer[datainoutput++] = c;
	}
	else {
		write_log(_T("serial output buffer overflow, data will be lost\n"));
		datainoutput = 0;
	}
	outser();
}

int checkserwrite(int spaceneeded)
{
	if (!serdev || !currprefs.use_serial)
		return 1;

	outser();
	if (datainoutput + spaceneeded >= sizeof(outputbuffer))
		return 0;

	return 1;
}

static void serdatcopy(void);

static void checksend(void)
{
	if (data_in_sershift != 1 && data_in_sershift != 2)
		return;

	if (serloop_enabled) {
		return;
	}

	if (serempty_enabled && !serxdevice_enabled) {
		return;
	}

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

static void sersend_end(uae_u32 v)
{
	data_in_sershift = 0;
	serdatcopy();
	data_in_serdat = 0;
	data_in_serdat_delay = get_cycles() + CYCLE_UNIT;
}

static void sersend_serloop(uae_u32 v)
{
	serdatr = serdatshift;
	serial_rx_irq();
	event2_newevent_xx(-1, v * CYCLE_UNIT, 0, sersend_end);
}

static void sersend_ce(uae_u32 v)
{
	if (checkshiftempty()) {
		lastbitcycle = get_cycles() + ((serper & 0x7fff) + 1) * CYCLE_UNIT;
		lastbitcycle_active_hsyncs = ((serper & 0x7fff) + 1) / maxhpos + 2;
	}
	else if (data_in_sershift == 1 || data_in_sershift == 2) {
		event2_newevent_x_replace(maxhpos, 0, sersend_ce);
	}
}

static void serdatcopy(void)
{
	if (data_in_sershift || !data_in_serdat)
		return;
	serdatshift = serdat;
	int bits = 8;
	if ((serdatshift & 0xff80) == 0x80) {
		bits = 7;
	}
	serdatshift_start = get_cycles();
	serdatshift_masked = serdatshift & ((1 << bits) - 1);
	data_in_serdat = 0;

	if (data_in_sershift == 0) {
		INTREQ_INT(0, 1);
	}
	data_in_sershift = 1;

	serdatshift_bits = 16;
	for (int i = 15; i >= 0; i--) {
		if (serdatshift & (1 << i))
			break;
		serdatshift_bits--;
	}

	// if someone uses serial port as some kind of timer..
	if (ser_accurate) {
		int sper = (serper & 0x7fff) + 1;
		int per = sper * (serdatshift_bits + 1);

		serper_tx_evt = get_cycles();
		serper_tx_per = sper;
		serper_tx_bits = serdatshift_bits + 1;
		serper_tx_cycles = per;

		// not connected, emulate only TX
		if (serempty_enabled && !serxdevice_enabled) {
			event2_newevent_xx(-1, per * CYCLE_UNIT, 0, sersend_end);
			return;
		}

		// loopback receive complete
		if (serloop_enabled) {
			int recper = sper * (ninebit ? 10 : 9) + (sper - 1) / 2;
			int perdiff = per - recper;
			if (perdiff < 0) {
				perdiff = 0;
			}

			// TX to RX delay (1488/1489 chip delays)
			recper += 3;

			event2_newevent_xx(-1, recper * CYCLE_UNIT, perdiff, sersend_serloop);
			return;
		}

		if (lastbitcycle_active_hsyncs) {
			// if last bit still transmitting, add remaining time.
			int extraper = (int)((lastbitcycle - get_cycles()) / CYCLE_UNIT);
			per += extraper;
		}

		event2_newevent_x_replace(per, 0, sersend_ce);
	}

	checksend();
}

void serial_hsynchandler()
{
	if (lastbitcycle_active_hsyncs > 0)
		lastbitcycle_active_hsyncs--;

	if (data_in_serdatr)
		serdatr_last_got++;
	if (serial_period_hsyncs == 0)
		return;
	serial_period_hsync_counter++;
	if (serial_period_hsyncs == 1 || (serial_period_hsync_counter % (serial_period_hsyncs - 1)) == 0) {
		checkreceive_serial();
		checkshiftempty();
	}
	else if ((serial_period_hsync_counter % serial_period_hsyncs) == 0 && !currprefs.cpu_cycle_exact) {
		checkshiftempty();
	}
	if (break_in_serdatr > 1) {
		break_in_serdatr--;
		if (break_in_serdatr == 1) {
			flushser();
		}
	}
}

void SERPER(uae_u16 w)
{
	int oldper = serper;

	if (serper == w && serper_set)  /* don't set baudrate if it's already ok */
		return;

	serper_set = true;
	ninebit = 0;
	serper = w;
	if (w & 0x8000)
		ninebit = 1;
	w &= 0x7fff;

	int per = w;
	if (per == 0)
		per = 1;
	per = 3546895 / (per + 1);
	if (per <= 0)
		per = 1;
	int i = 0;
	while (allowed_baudrates[i] >= 0 && per > allowed_baudrates[i] * 100 / 97)
		i++;
	int baud = allowed_baudrates[i];
	if (baud <= 0) {
		baud = allowed_baudrates[1];
	}

	serial_period_hsyncs = (((serper & 0x7fff) + 1) * (1 + 8 + ninebit + 1 - 1)) / maxhpos;
	if (serial_period_hsyncs <= 0)
		serial_period_hsyncs = 1;

	if (log_sercon) {
		serial_period_hsyncs = 1;
		seriallog = log_sercon;
		seriallog_lf = true;
		write_log(_T("\n"));
	}

	serial_period_hsync_counter = 0;

	if (!serloop_enabled || seriallog > 0) {
		write_log(_T("SERIAL: period=%d, baud=%d, hsyncs=%d, bits=%d, PC=%x\n"), w, baud, serial_period_hsyncs, ninebit ? 9 : 8, M68K_GETPC);
	}

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
	check(sp_set_baudrate(port, baud));
#endif

	// mid transmit period change
	if ((serloop_enabled || serempty_enabled) && ser_accurate) {
		evt_t c = get_cycles();
		evt_t n = serper_tx_evt + serper_tx_cycles * CYCLE_UNIT;
		if (n > c) {
			int cycles_transmitted = (int)((c - serper_tx_evt) / CYCLE_UNIT);
			serper_tx_cycles -= cycles_transmitted;
			if (serper_tx_cycles >= 0) {
				int serper_tx_cycles_mod = serper_tx_cycles % serper_tx_per;
				serper_tx_cycles /= serper_tx_per;
				serper_tx_per = (serper & 0x7fff) + 1;
				serper_tx_cycles *= serper_tx_per;
				serper_tx_cycles += serper_tx_cycles_mod;
				serper_tx_evt = c;
				event2_newevent_x_replace_exists(serper_tx_cycles, 0, sersend_end);
			}
		}
	}
}

static void SERDAT_send(uae_u32 v)
{
	uae_u16 w = (uae_u16)v;

	if (ser_accurate) {
		serdat = w;
		data_in_serdat = 1;
		if (!data_in_sershift) {
			if (serloop_enabled || serempty_enabled || serxdevice_enabled) {
				data_in_sershift = -1;
				INTREQ_INT(0, 1);
			}
			serdatcopy();
		}

	}
	else {
		serdatcopy();

		serdat = w;

		if (!w) {
			return;
		}

		data_in_serdat = 1;
		serdatcopy();
	}
}

uae_u16 SERDATR(void)
{
	serdatr &= 0x03ff;
	if (!data_in_serdat || (ser_accurate && get_cycles() >= data_in_serdat_delay)) {
		serdatr |= 0x2000; // TBE (Transmit buffer empty)
	}
	if (!data_in_sershift && (serdatr & 0x2000)) {
		serdatr |= 0x1000; // TSRE (Transmit shift register empty)
	}
	if (data_in_serdatr) {
		serdatr |= 0x4000; // RBF (Receive buffer full)
	}
	if (ovrun) {
		serdatr |= 0x8000; // OVRUN (Receiver overrun)
	}
	if (serloop_enabled) {
		int per = ((serper & 0x7fff) + 1) * CYCLE_UNIT;
		evt_t sper = per * serdatshift_bits;
		evt_t c = get_cycles();
		evt_t diff = serdatshift_start + sper - c;
		if (break_in_serdatr < 0) {
			serdatr |= 0x0800;
		}
		else if (diff > 0) {
			int bit = (int)(c - serdatshift_start) / per;
			if (bit > 0 && !(serdatshift & (1 << (bit - 1)))) {
				serdatr |= 0x0800;
			}
		}
		else {
			if (break_in_serdatr <= 0) {
				serdatr |= 0x0800;
			}
		}
	}
	else {
		if (break_in_serdatr <= 0) {
			serdatr |= 0x0800; // RXD
		}
	}

	data_in_serdatr = 0;
	receive_next_buffered();
	return serdatr;
}

void SERDAT(uae_u16 w)
{
	if (ser_accurate) {
		event2_newevent_xx(-1, 1 * CYCLE_UNIT, w, SERDAT_send);
	}
	else {
		SERDAT_send(w);
	}
}

void serial_rbf_clear(void)
{
	ovrun = 0;
}

void serial_dtr_on(void)
{
	dtr = 1;
#ifdef SERIAL_PORT
	if (currprefs.serial_demand)
		serial_open();
	check(sp_set_flowcontrol(port, SP_FLOWCONTROL_DTRDSR));
#endif
}

void serial_dtr_off(void)
{
	dtr = 0;
#ifdef SERIAL_PORT
	if (currprefs.serial_demand)
		serial_close();
	check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));
#endif
}

void serial_flush_buffer(void)
{
}

static uae_u8 oldserbits;

uae_u8 serial_readstatus(uae_u8 v, uae_u8 dir)
{
	sp_signal signal;
	uae_u8 serbits = oldserbits;

	//if (serloop_enabled) {
	//	if (serstatus & 0x80) { // DTR -> DSR + CD
	//		signal |= SP_SIG_DSR | SP_SIG_DCD;
	//	}
	//	if (serstatus & 0x10) { // RTS -> CTS
	//		signal |= SP_SIG_CTS;
	//	}
	//}
	//else 
		if (currprefs.use_serial) {
#ifdef SERIAL_PORT
		/* Read the current config from the port into that configuration. */
		check(sp_get_signals(port, &signal));
#endif
	} else {
		return v;
	}

	if (!(signal & SP_SIG_DCD)) {
		if (!(serbits & 0x20)) {
			serbits |= 0x20;
		}
	} else {
		if (serbits & 0x20) {
			serbits &= ~0x20;
		}
	}

	if (!(signal & SP_SIG_DSR)) {
		if (!(serbits & 0x08)) {
			serbits |= 0x08;
		}
	}
	else {
		if (serbits & 0x08) {
			serbits &= ~0x08;
		}
	}

	if (!(signal & SP_SIG_CTS)) {
		if (!(serbits & 0x10)) {
			serbits |= 0x10;
		}
	}
	else {
		if (serbits & 0x10) {
			serbits &= ~0x10;
		}
	}

	// SEL == RI
	if (1) {
		serbits |= 0x04;
	} else {
		serbits &= ~0x04;
		serbits |= v & 0x04;
	}

	if (signal & SP_SIG_RI) {
		serbits &= ~0x04;
	}

	serbits &= 0x08 | 0x10 | 0x20;
	oldserbits &= ~(0x08 | 0x10 | 0x20);
	oldserbits |= serbits;

	v = (v & (0x80 | 0x40 | 0x02 | 0x01)) | serbits;
	return v;
}

uae_u8 serial_writestatus(uae_u8 newstate, uae_u8 dir)
{
	static int logcnt = 10;

	serstatus = newstate & dir;

#ifdef SERIAL_PORT
	if (currprefs.use_serial) {
		if (((oldserbits ^ newstate) & 0x80) && (dir & 0x80)) {
			if (newstate & 0x80)
				serial_dtr_off();
			else
				serial_dtr_on();
		}

		if (!currprefs.serial_hwctsrts && (dir & 0x40)) {
			if ((oldserbits ^ newstate) & 0x40) {
				if (newstate & 0x40) {
					check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));
				}
				else {
					check(sp_set_flowcontrol(port, SP_FLOWCONTROL_RTSCTS));
				}
			}
		}
	}

	if (logcnt > 0) {
		if (((newstate ^ oldserbits) & 0x40) && !(dir & 0x40)) {
			write_log(_T("SERIAL: warning, program tries to use RTS as an input! PC=%x\n"), M68K_GETPC);
			logcnt--;
		}
		if (((newstate ^ oldserbits) & 0x80) && !(dir & 0x80)) {
			write_log(_T("SERIAL: warning, program tries to use DTR as an input! PC=%x\n"), M68K_GETPC);
			logcnt--;
		}
	}

#endif

	oldserbits &= ~(0x80 | 0x40);
	newstate &= 0x80 | 0x40;
	oldserbits |= newstate;

	return oldserbits;
}

void serial_open()
{
#ifdef SERIAL_PORT
	if (serdev)
		return;
	serper = 0;

	if (!_tcsicmp(currprefs.sername, SERIAL_INTERNAL)) {
		sermap_enabled = true;
	} else if (!_tcsicmp(currprefs.sername, SERIAL_LOOPBACK)) {
		serloop_enabled = true;
	} else {
		if (currprefs.sername[0] && !openser(currprefs.sername)) {
			write_log(_T("SERIAL: Could not open device %s\n"), currprefs.sername);
			return;
		}
	}
	serdev = 1;
	ser_accurate = currprefs.cpu_memory_cycle_exact || (currprefs.cpu_model <= 68020 && currprefs.cpu_compatible && currprefs.m68k_speed == 0);
#endif
}

void serial_close()
{
#ifdef SERIAL_PORT
	closeser();
#endif
	serdev = 0;

	if (receive_buf) {
		xfree(receive_buf);
		receive_buf = NULL;
	}
	receive_buf_size = 0;
	receive_buf_count = 0;

	serloop_enabled = false;
	serempty_enabled = false;
	serxdevice_enabled = false;
	serper_set = false;
	ser_accurate = false;
}

void serial_init()
{
#ifdef SERIAL_PORT
	if (!currprefs.use_serial)
		return;
	if (!currprefs.serial_demand)
		serial_open();
#endif
}

void serial_exit()
{
#ifdef SERIAL_PORT
	serial_close();
#endif
	dtr = 0;
	oldserbits = 0;
	data_in_serdat = 0;
	data_in_serdatr = 0;
	data_in_sershift = 0;
	break_in_serdatr = 0;
	ovrun = 0;
	serdatr_last_got = 0;
}

void serial_uartbreak(int v)
{
	if (!serdev || !currprefs.use_serial)
		return;
#ifdef SERIAL_PORT
	if (v)
	{
		check(sp_start_break(port));
	}
	else
	{
		check(sp_end_break(port));
	}
#endif
}

/* Helper function for error handling. */
int check(const sp_return result)
{
	char* error_message;

	switch (result) {
	case SP_ERR_ARG:
		write_log("Error: Invalid argument.\n");
		return SP_ERR_ARG;
	case SP_ERR_FAIL:
		error_message = sp_last_error_message();
		write_log("Error: Failed: %s\n", error_message);
		sp_free_error_message(error_message);
		return SP_ERR_FAIL;
	case SP_ERR_SUPP:
		write_log("Error: Not supported.\n");
		return SP_ERR_SUPP;
	case SP_ERR_MEM:
		write_log("Error: Couldn't allocate memory.\n");
		return SP_ERR_MEM;
	case SP_OK:
	default:
		return result;
	}
}


// TODO This should be moved somewhere more relevant (PARALLEL support)
int flashscreen = 0;
