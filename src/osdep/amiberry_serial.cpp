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

static uae_u8 oldserbits;
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

void serial_open(void);
void serial_close(void);

uae_u16 serper, serdat, serdatr;

static const int allowed_baudrates[] =
{ 0, 110, 300, 600, 1200, 2400, 4800, 9600, 14400,
19200, 31400, 38400, 57600, 115200, 128000, 256000, -1 };

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
	}
	else {
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

	data_in_serdatr = 1;
	serdatr_last_got = 0;
	serial_check_irq();
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

#ifdef SERIAL_PORT
	if (ninebit) {
		if (!checkserwrite(2)) {
			data_in_sershift = 2;
			return;
		}
		writeser(((serdatshift >> 8) & 1) | 0xa8);
		writeser(serdatshift_masked);
	}
	else {
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
	}
	else {
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
	serdatshift_masked = serdatshift & ((1 << bits) - 1);
	data_in_sershift = 1;
	data_in_serdat = 0;

	// if someone uses serial port as some kind of timer..
	if (currprefs.cpu_memory_cycle_exact) {
		bits = 16 + 1;
		for (int i = 15; i >= 0; i--) {
			if (serdatshift & (1 << i))
				break;
			bits--;
		}
		// assuming when last bit goes out, transmit buffer
		// becomes empty, not when last bit has finished
		// transmitting.
		int per = ((serper & 0x7fff) + 1) * (bits - 1);
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

void serial_hsynchandler()
{
#ifdef AHI
	extern void hsyncstuff(void);
	hsyncstuff();
#endif

	if (lastbitcycle_active_hsyncs > 0)
		lastbitcycle_active_hsyncs--;

	if (data_in_serdatr)
		serdatr_last_got++;
	if (serial_period_hsyncs == 0)
		return;
	serial_period_hsync_counter++;
	if (serial_period_hsyncs == 1 || (serial_period_hsync_counter % (serial_period_hsyncs - 1)) == 0) {
		checkreceive_serial();
		//checkreceive_enet();
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
	if (serper == w)  /* don't set baudrate if it's already ok */
		return;

	ninebit = 0;
	serper = w;
	if (w & 0x8000)
		ninebit = 1;
	w &= 0x7fff;

	if (w < 13)
		w = 13;

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

	write_log(_T("SERIAL: period=%d, baud=%d, hsyncs=%d, bits=%d, PC=%x\n"), w, baud, serial_period_hsyncs, ninebit ? 9 : 8, M68K_GETPC);

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
}

void SERDAT(uae_u16 w)
{
	serdatcopy();

	serdat = w;

	if (!w) {
		return;
	}

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

	data_in_serdatr = 0;
	return serdatr;
}

void serial_rbf_clear(void)
{
	ovrun = 0;
}

void serial_check_irq(void)
{
	// Data in receive buffer
	if (data_in_serdatr)
		INTREQ_0(0x8000 | 0x0800);
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

uae_u8 serial_readstatus(uae_u8 dir)
{
	sp_signal signal;
	uae_u8 serbits = oldserbits;

#ifdef SERIAL_PORT
	/* Read the current config from the port into that configuration. */
	check(sp_get_signals(port, &signal));
#endif
	if (!(signal & SP_SIG_DCD)) {
		if (!(serbits & 0x20)) {
			serbits |= 0x20;
		}
	}
	else {
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

	serbits &= 0x08 | 0x10 | 0x20;
	oldserbits &= ~(0x08 | 0x10 | 0x20);
	oldserbits |= serbits;

	return oldserbits;
}

uae_u8 serial_writestatus(uae_u8 newstate, uae_u8 dir)
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
				check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));
			}
			else {
				check(sp_set_flowcontrol(port, SP_FLOWCONTROL_RTSCTS));
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
	if (serdev)
		return;
#ifdef SERIAL_PORT
	/* Call sp_get_port_by_name() to find the port. The port
	* pointer will be updated to refer to the port found. */
	check(sp_get_port_by_name(currprefs.sername, &port));
	check(sp_open(port, SP_MODE_READ_WRITE));

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
	
#endif

	serdev = 1;
}

void serial_close()
{
#ifdef SERIAL_PORT
	if (serdev)
	{
		check(sp_close(port));
		sp_free_port(port);
	}
#endif
	serdev = 0;
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


// TODO These should be moved somewhere more relevant
int flashscreen = 0;

void doflashscreen(void)
{
	flashscreen = 10;
	init_colors(0);
	picasso_refresh(0);
	reset_drawing();
}

void hsyncstuff(void)
{
#ifdef AHI
	{
		if (ahi_on) {
			static int count;
			count++;
			//15625/count freebuffer check
			if (count > ahi_pollrate) {
				ahi_updatesound(1);
				count = 0;
			}
		}
	}
#endif
}