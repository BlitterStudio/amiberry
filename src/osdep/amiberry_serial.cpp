/*
 * Amiberry
 *
 * Serial port implementation, using libserialport
 *
 * Copyright (c) 2023 Dimitris Panokostas <midwan@gmail.com>
 *
 */

#include "sysconfig.h"
#ifdef SERIAL_ENET
#include "enet/enet.h"
#endif
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "serial.h"
#ifdef ENFORCER
#include "enforcer.h"
#endif
#ifdef ARCADIA
#include "arcadia.h"
#endif
#ifdef PARALLEL_PORT
#include "parallel.h"
#endif
#ifdef RETROPLATFORM
#include "rp.h"
#endif
#include "parser.h"

#ifdef WITH_MIDI
#include "midi.h"
#endif
#ifdef WITH_MIDIEMU
#include "midiemu.h"
#endif

#ifdef USE_LIBSERIALPORT
#include <libserialport.h>
#endif
#include <netinet/tcp.h>
#include <sys/mman.h>

#define SERIALLOGGING 0
#define SERIALDEBUG 0 /* 0, 1, 2 3 */
#define SERIALHSDEBUG 0
#define SERIAL_HSYNC_BEFORE_OVERFLOW 200
#define SERIAL_BREAK_DELAY (20 * maxvpos)
#define SERIAL_BREAK_TRANSMIT_DELAY 4


#define SERIAL_MAP

#ifdef SERIAL_MAP
#define SERMAP_SIZE 256
struct sermap_buffer
{
	volatile unsigned long version;
	volatile uae_u32 active_read;
	volatile uae_u32 active_write;
	volatile uae_u32 read_offset;
	volatile uae_u32 write_offset;
	volatile uae_u32 data[SERMAP_SIZE];
};
static sermap_buffer* sermap1, * sermap2;
static void* sermap_handle;
static uae_u8* sermap_data;
static bool sermap_master;
static bool sermap_enabled;
static uae_u32 sermap_flags;
static bool serloop_enabled;
static bool serempty_enabled;
static bool serxdevice_enabled;
static uae_u8 serstatus;
static bool ser_accurate;
static bool safe_receive;
static uae_u16* receive_buf;
static bool sticky_receive_interrupt;
static int receive_buf_size, receive_buf_count;
static void flushqueue(void);

#define SER_MEMORY_MAPPING _T("Amiberry_Serial")

static void shmem_serial_send(uae_u32 data)
{
	sermap1->active_write = true;
	if (!sermap1->active_read) {
		return;
	}
	uae_u32 v = sermap1->write_offset;
	if (((v + 1) & (SERMAP_SIZE - 1)) == sermap1->read_offset) {
		write_log(_T("Shared serial port memory overflow!\n"));
		return;
	}
	sermap1->data[v] = data;
	v++;
	v &= (SERMAP_SIZE - 1);
	sermap1->write_offset = v;
}
static uae_u32 shmem_serial_receive()
{
	sermap2->active_read = true;
	if (!sermap2->active_write) {
		return 0xffffffff;
	}
	uae_u32 v = sermap2->read_offset;
	if (v == sermap2->write_offset) {
		return 0xffffffff;
	}
	const uae_u32 data = sermap2->data[v];
	v++;
	v &= (SERMAP_SIZE - 1);
	sermap2->read_offset = v;
	return data;
}

static void sermap_deactivate()
{
	sermap_enabled = false;
	sermap_flags = 0;
	if (sermap1) {
		sermap1->active_write = 0;
		sermap1->write_offset = sermap1->read_offset;
	}
	if (sermap2) {
		sermap2->active_read = 0;
		sermap2->read_offset = sermap2->write_offset;
	}
}

int shmem_serial_state()
{
	if (!sermap_handle)
		return 0;
	if (sermap_master)
		return 1;
	return 2;
}

void shmem_serial_delete()
{
	sermap_deactivate();
	sermap_master = false;
	if (sermap_data) {
		munmap(sermap_data, sizeof(sermap_buffer) * 2);
	}
	if (sermap_handle) {
		shm_unlink(SER_MEMORY_MAPPING);
	}
	sermap_data = nullptr;
	sermap_handle = nullptr;
	sermap1 = sermap2 = nullptr;
}


bool shmem_serial_create()
{
	shmem_serial_delete();

	int fd = shm_open(SER_MEMORY_MAPPING, O_RDWR, 0666);
	if (fd == -1) {
		fd = shm_open(SER_MEMORY_MAPPING, O_CREAT | O_RDWR, 0666);
		if (fd == -1) {
			write_log("Failed to create shared serial port memory");
			return false;
		}
		sermap_master = true;
		write_log("Created internal serial port shared memory\n");
	}
	else {
		write_log("Found already existing serial port shared memory\n");
	}

	if (ftruncate(fd, sizeof(sermap_buffer) * 2) == -1) {
		write_log("Failed to set size of shared memory");
		close(fd);
		return false;
	}

	sermap_data = static_cast<uae_u8*>(mmap(nullptr, sizeof(sermap_buffer) * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
	if (sermap_data == MAP_FAILED) {
		write_log("Shared serial port memory mmap() failed");
		close(fd);
		return false;
	}

	close(fd);

	if (sermap_master) {
		sermap1 = reinterpret_cast<sermap_buffer*>(sermap_data);
		sermap2 = reinterpret_cast<sermap_buffer*>(sermap_data + sizeof(sermap_buffer));
		sermap1->version = version;
		sermap2->version = version;
	}
	else {
		sermap2 = reinterpret_cast<sermap_buffer*>(sermap_data);
		sermap1 = reinterpret_cast<sermap_buffer*>(sermap_data + sizeof(sermap_buffer));
		if (sermap2->version != version || sermap1->version != version) {
			write_log("Shared serial port memory version mismatch %08x != %08x\n", sermap1->version, version);
			shmem_serial_delete();
			return false;
		}
	}

	return true;
}

#endif

#ifdef USE_LIBSERIALPORT
/* A pointer to a struct sp_port, which will refer to
 * the port found. */
sp_port* port;
#endif

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
static int serial_period_hsyncs;
static int serial_period_transmit_ccks, serial_period_receive_ccks;
static int serial_period_transmit_cck_counter, serial_period_receive_cck_counter;
static int ninebit;
static int lastbitcycle_active_hsyncs;
static bool gotlogwrite;
static evt_t lastbitcycle;
static int serial_recv_previous, serial_send_previous;
static int serdatr_last_got;
int serdev;
int seriallog = 0, log_sercon = 0;
int serial_enet;
static bool seriallog_lf;
extern int consoleopen;

void serial_open ();
void serial_close ();

uae_u16 serper, serdat, serdatr;
static bool serper_set = false;

static constexpr int allowed_baudrates[] =
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

#include "uae/socket.h"

static SOCKET serialsocket = UAE_SOCKET_INVALID;
static SOCKET serialconn = UAE_SOCKET_INVALID;
static BOOL tcpserial;

static bool tcp_is_connected ()
{
#ifdef SERIAL_ENET
	if (serialsocket == UAE_SOCKET_INVALID) {
		return false;
	}
	if (serialconn == UAE_SOCKET_INVALID) {
		if (uae_socket_select_read(serialsocket)) {
			serialconn = uae_socket_accept(serialsocket);
			if (serialconn != UAE_SOCKET_INVALID) {
				write_log(_T("TCP: Serial connection accepted\n"));
				int opt = 1;
				setsockopt(serialsocket, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(int));
			}
		}
	}
	return serialconn != UAE_SOCKET_INVALID;
#else
	return false;
#endif
}

static void tcp_disconnect ()
{
#ifdef SERIAL_ENET
	if (serialconn == UAE_SOCKET_INVALID) {
		return;
	}
	uae_socket_close(serialconn);
	serialconn = UAE_SOCKET_INVALID;
	write_log(_T("TCP: Serial disconnect\n"));
#endif
}

static void closetcp ()
{
#ifdef SERIAL_ENET
	if (serialconn != UAE_SOCKET_INVALID) {
		uae_socket_close(serialconn);
		serialconn = UAE_SOCKET_INVALID;
	}
	if (serialsocket != UAE_SOCKET_INVALID) {
		uae_socket_close(serialsocket);
		serialsocket = UAE_SOCKET_INVALID;
	}
	// WSACleanup ();
#endif
}

static int opentcp (const TCHAR *sername)
{
#ifdef SERIAL_ENET
	serialsocket = uae_tcp_listen_uri(sername, "1234", UAE_SOCKET_DEFAULT);
	if (serialsocket == UAE_SOCKET_INVALID) {
		return 0;
	}
	if (_tcsicmp(uae_uri_path(sername), _T("/wait")) == 0) {
		while (tcp_is_connected() == false) {
			Sleep(1000);
			write_log(_T("TCP: Waiting for serial connection...\n"));
		}
	}
	tcpserial = TRUE;
	return 1;
#else
	return 0;
#endif
}

int openser (const TCHAR *sername)
{
#ifdef USE_LIBSERIALPORT
	if (!_tcsnicmp(sername, _T("TCP:"), 4)) {
		return opentcp(sername);
	}

	if (sp_get_port_by_name(sername, &port) != SP_OK) {
		write_log("Error finding serial port %s\n", sername);
		return 0;
	}

	if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
		write_log("Error opening serial port %s\n", sername);
		sp_free_port(port);
		port = nullptr;
		return 0;
	}

	sp_set_baudrate(port, 9600);
	sp_set_bits(port, 8);
	sp_set_parity(port, SP_PARITY_NONE);
	sp_set_stopbits(port, 1);
	sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
#endif
	return 1;
}

void closeser ()
{
	if (tcpserial) {
		closetcp();
		tcpserial = FALSE;
	}
#ifdef WITH_MIDI
	if (midi_ready) {
		midi_close();
		midi_ready = 0;
		//need for camd Midi Stuff(it close midi and reopen it but serial.c think the baudrate
		//is the same and do not open midi), so setting serper to different value helps
		extern uae_u16 serper;
		serper = 0x30;
	}
#endif
#ifdef WITH_MIDIEMU
	if (midi_emu) {
		midi_emu_close();
	}
#endif
#ifdef USE_LIBSERIALPORT
	if (port != nullptr) {
		sp_close(port);
		sp_free_port(port);
		port = nullptr;
	}
#endif
}

#define SERDATR_IRQACK 2
#define SERDATR_READ 1

static void serial_rx_irq()
{
	int delay = 9;
	// Data in receive buffer
	data_in_serdatr = SERDATR_READ | SERDATR_IRQACK;
	data_in_serdatr_evt = get_cycles() + delay * CYCLE_UNIT;
	if (ser_accurate) {
		INTREQ_INT(11, delay);
	} else {
		INTREQ_INT(11, 0);
	}
	serdatr_last_got = 0;
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

static void receive_next_buffered()
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

void serial_rethink()
{
	if (data_in_serdatr == SERDATR_READ) {
		int sdr = 1;
		if (serloop_enabled) {
			sdr = 0;
		}
		if (serxdevice_enabled) {
			sdr = 1;
		}
		if (safe_receive) {
			sdr = 1;
		}
		if (ser_accurate && get_cycles() <= data_in_serdatr_evt) {
			sdr = 0;
		}
		// We get here if program cleared RBF bit but didn't yet read SERDATR
		// Without this hack data can be lost when using fast emulation modes
		// and physical serial port or internally emulated serial devices.
		if (sdr && (intena & (1 << 11)) && (intena & (1 << 14)) && !(intreq & (1 << 11))) {
			INTREQ_INT(11, 0);
		}
	}
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
	if (tcpserial) {
		if (tcp_is_connected()) {
			char buf[1];
			buf[0] = 0;
			int err = uae_socket_read(serialconn, buf, 1);
			if (err == 1) {
				*buffer = buf[0];
				//write_log(_T(" %02X "), buf[0]);
				return 1;
			} else {
				tcp_disconnect();
			}
		}
		return 0;
#ifdef WITH_MIDI
	} else if (midi_ready) {
		uae_u8 ch;
		int res = midi_recv_byte(&ch);
		*buffer = ch;
		return res;
#endif
	} else {
		if (!currprefs.use_serial)
			return 0;
		if (dataininput > dataininputcnt) {
			*buffer = inputbuffer[dataininputcnt++];
			return 1;
		}
		dataininput = 0;
		dataininputcnt = 0;
#ifdef USE_LIBSERIALPORT
		if (port != nullptr) {
			int bytes_available = sp_input_waiting(port);
			if (bytes_available < 0) {
				write_log("Error checking input buffer: %s\n", sp_last_error_message());
				return 0;
			}

			if (bytes_available > 0) {
				int len = bytes_available;
				if (len > sizeof(inputbuffer))
					len = sizeof(inputbuffer);

				int bytes_read = sp_nonblocking_read(port, inputbuffer, len);
				if (bytes_read < 0) {
					write_log("Error reading from serial port: %s\n", sp_last_error_message());
					return 0;
				}

				dataininput = bytes_read;
				dataininputcnt = 0;
				if (bytes_read == 0)
					return 0;
				return readser(buffer);
			}
		}
#endif
		return 0;
	}
}

void flushser()
{
#ifdef USE_LIBSERIALPORT
	if (port) {
		sp_flush(port, SP_BUF_INPUT);
	}
	else {
		while (readseravail(nullptr)) {
			int data;
			if (readser(&data) <= 0)
				break;
		}
	}
#endif
}

int readseravail(bool* breakcond)
{
	if (breakcond)
		*breakcond = false;
	if (tcpserial) {
		if (tcp_is_connected()) {
			int err = uae_socket_select_read(serialconn);
			if (err == UAE_SELECT_ERROR) {
				tcp_disconnect();
				return 0;
			}
			if (err > 0)
				return 1;
		}
		return 0;
#ifdef WITH_MIDI
	} else if (midi_ready) {
		if (midi_has_byte())
			return 1;
#endif
	} else {
		if (!currprefs.use_serial)
			return 0;
		if (dataininput > dataininputcnt)
			return 1;
#ifdef USE_LIBSERIALPORT
		if (port) {
			if (breakcond && breakpending) {
				*breakcond = true;
				breakpending = false;
			}
			int bytes_available = sp_input_waiting(port);
			if (bytes_available < 0) {
				write_log("Error checking input buffer: %s\n", sp_last_error_message());
				return 0;
			}
			return bytes_available;
		}
#endif
	}
	return 0;
}

static void maybesetoverrun(void)
{
	if ((data_in_serdatr & SERDATR_IRQACK) && serdatr_last_got > SERIAL_HSYNC_BEFORE_OVERFLOW) {
#if SERIALDEBUG > 0
		write_log(_T("SERIAL: OVERRUN\n"));
#endif
		flushser();
		ovrun = true;
		sticky_receive_interrupt = false;
	}
	serdatr_last_got = 0;
}

static bool canreceive()
{
	// don't replace data in SERDATR until interrupt is cleared and SERDATR is read in safe receive mode
	if (safe_receive) {
		if (data_in_serdatr) {
			return false;
		}
	}
	if (!(data_in_serdatr & SERDATR_IRQACK)) {
		return true;
	}
	if (currprefs.serial_direct) {
		return false;
	}
	if (currprefs.cpu_cycle_exact) {
		return true;
	}
	if (serdatr_last_got > SERIAL_HSYNC_BEFORE_OVERFLOW) {
		return true;
	}
	return false;
}

static void checkreceive_enet()
{
#ifdef SERIAL_ENET
	uae_u16 recdata;

	if (!canreceive())
		return;
	if (!enet_readseravail())
		return;
	maybesetoverrun();
	if (!enet_readser(&recdata))
		return;
	serdatr = recdata & 0x1ff;
	if (recdata & 0x200)
		serdatr |= 0x200;
	else
		serdatr |= 0x100;
	serial_rx_irq();
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: received %02X (%c)\n"), serdatr & 0xff, dochar(serdatr));
#endif
#endif
}

static void checkreceive_serial ()
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

	serial_rx_irq();
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: received %02X (%c)\n"), serdatr & 0xff, dochar(serdatr));
#endif
#endif
}

static void outser()
{
	if (datainoutput <= 0)
		return;
#ifdef USE_LIBSERIALPORT
	memcpy(outputbufferout, outputbuffer, datainoutput);
	int bytes_written = sp_blocking_write(port, outputbufferout, datainoutput, 1000);
	if (bytes_written < 0) {
		write_log("Failed to write to serial port: %d\n", bytes_written);
	}
	else if (bytes_written != datainoutput) {
		write_log("Only wrote %d of %d bytes!\n", bytes_written, datainoutput);
	}
#endif
	datainoutput = 0;
}

void writeser_flush()
{
	outser();
}

void writeser(int c)
{
	if (tcpserial) {
		if (tcp_is_connected()) {
			char buf[1];
			buf[0] = static_cast<char>(c);
			if (uae_socket_write(serialconn, buf, 1) != 1) {
				tcp_disconnect();
			}
		}
#ifdef WITH_MIDIEMU
	} else if (midi_emu) {
		auto b = static_cast<uae_u8>(c);
		midi_emu_parse(&b, 1);
#endif
#ifdef WITH_MIDI
	} else if (midi_ready) {
		midi_send_byte(static_cast<uint8_t>(c));
#endif
	} else {
#ifdef USE_LIBSERIALPORT
		if (!port || !currprefs.use_serial)
			return;
		if (datainoutput + 1 < sizeof(outputbuffer)) {
			outputbuffer[datainoutput++] = c;
		} else {
			write_log(_T("serial output buffer overflow, data will be lost\n"));
			datainoutput = 0;
		}
		outser();
#endif
	}
}

int checkserwrite(int spaceneeded)
{
#ifdef USE_LIBSERIALPORT
	if (!port || !currprefs.use_serial)
		return 1;
#endif
#ifdef WITH_MIDI
	if (midi_ready) {
		return 1;
	}
#endif
	outser();
	if (datainoutput + spaceneeded >= sizeof(outputbuffer))
		return 0;

	return 1;
}

static void serdatcopy();

static void checksend()
{
	if (data_in_sershift != 1 && data_in_sershift != 2) {
		return;
	}
	if (serloop_enabled) {
		return;
	}
#ifdef ARCADIA
	if (alg_flag || currprefs.genlock_image >= 7) {
		ld_serial_read(serdatshift);
	}
#endif
	if (cubo_enabled) {
		touch_serial_read(serdatshift);
	}
#ifdef RETROPLATFORM
	if (rp_ismodem()) {
		rp_writemodem((uae_u8)serdatshift_masked);
	}
#endif
	if (serempty_enabled && !serxdevice_enabled) {
		goto end;
	}
#ifdef SERIAL_MAP
	if (sermap_data && sermap_enabled) {
		shmem_serial_send(serdatshift);
	}
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
end:
	if (serial_period_transmit_ccks <= maxhpos || data_in_sershift == 2) {
		data_in_sershift = 0;
		serdatcopy();
	} else {
		data_in_sershift = 3;
	}
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: send %04X (%c)\n"), serdatshift, dochar(serdatshift));
#endif
}

static bool checkshiftempty()
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
	} else if (data_in_sershift == 1 || data_in_sershift == 2) {
		event2_newevent_x_replace(maxhpos, 0, sersend_ce);
	}
}

static void serdatcopy()
{
	if (data_in_sershift || !data_in_serdat)
		return;
	serdatshift = serdat;
	int sper = (serper & 0x7fff) + 1;
	int bits = 8;
	if ((serdatshift & 0xff80) == 0x80) {
		bits = 7;
	}
	serdatshift_start = get_cycles();
	serdatshift_masked = serdatshift & ((1 << bits) - 1);
	data_in_serdat = 0;

	if (seriallog > 0 || (consoleopen && seriallog < 0)) {
		gotlogwrite = true;
		if (seriallog_lf && seriallog > 2) {
			TCHAR* ts = write_log_get_ts();
			if (ts)
				write_log(_T("%s:"), ts);
			seriallog_lf = false;
		}
		const TCHAR ch = docharlog(serdatshift_masked);
		write_log(_T("%c"), ch);
		if (ch == 10)
			seriallog_lf = true;
	}

	if (serper == 372) {
#ifdef ENFORCER
		if (enforcermode & 2) {
			console_out_f(_T("%c"), docharlog(serdatshift_masked));
		}
#endif
	}

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

	data_in_serdat_delay = get_cycles() + CYCLE_UNIT;

	// if someone uses serial port as some kind of timer...
	if (ser_accurate) {
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
			int extraper = static_cast<int>((lastbitcycle - get_cycles()) / CYCLE_UNIT);
			per += extraper;
		}

		event2_newevent_x_replace(per, 0, sersend_ce);

		checksend();
	}
	else {

		if (serloop_enabled) {
			sersend_serloop(0);
		} else {
			checksend();
			sersend_end(0);
		}
	}

}

void serial_hsynchandler ()
{
	// We handle this in ahi_hsync() instead
#ifndef AMIBERRY
#ifdef AHI
	extern void hsyncstuff(void);
	hsyncstuff();
#endif
#endif
	bool can = canreceive();

#ifdef ARCADIA
	if (alg_flag || currprefs.genlock_image >= 7) {
		if (can) {
			const int ch = ld_serial_write();
			if (ch >= 0) {
				serdatr = ch | 0x100;
				serial_rx_irq();
			}
		}
	}
#endif
	if (cubo_enabled) {
		if (can) {
			int ch = touch_serial_write();
			if (ch >= 0) {
				serdatr = ch | 0x100;
				serial_rx_irq();
			}
		}
	}
	if (seriallog > 1 && !data_in_serdatr && gotlogwrite) {
		const int ch = read_log();
		if (ch > 0) {
			serdatr = ch | 0x100;
			serial_rx_irq();
		}
	}
	if (lastbitcycle_active_hsyncs > 0)
		lastbitcycle_active_hsyncs--;
#ifdef SERIAL_MAP
	if (sermap2 && sermap_enabled) {
		if (can) {
			for (;;) {
				uae_u32 v = shmem_serial_receive();
				if (v == 0xffffffff) {
					break;
				}
				if (!(v & 0xffff0000)) {
					serdatr = static_cast<uae_u16>(v);
					serial_rx_irq();
					break;
				} else if ((v & 0x80000000) == 0x80000000) {
					sermap_flags &= 0x0fff0000;
					sermap_flags |= v & 0xffff;
				} else if ((v & 0x40000000) == 0x40000000) {
					if (v & (0x10000 | 0x20000)) {
						sermap_flags &= ~(0x10000 | 0x20000);
						sermap_flags |= v & (0x10000 | 0x20000);
						break;
					}
				}
			}
		}
		// break on
		if (sermap_flags & 0x20000) {
			break_in_serdatr = maxvpos;
		}
		if (break_in_serdatr) {
			serdatr = 0;
			if (break_delay == 0) {
				serial_rx_irq();
				break_delay = SERIAL_BREAK_TRANSMIT_DELAY;
			}
			if (break_delay > 0) {
				break_delay--;
			}
		}
		// break off
		if (break_in_serdatr == 1) {
			break_in_serdatr = 0;
			break_delay = 0;
			serdatr |= 0x100;
			serial_rx_irq();
		}
	}
#endif
	if (data_in_serdatr & SERDATR_IRQACK) {
		serdatr_last_got++;
	}
	if (!ser_accurate) {
		serial_period_transmit_cck_counter += maxhpos;
		if (serial_period_transmit_cck_counter >= serial_period_transmit_ccks && serial_period_transmit_ccks) {
			serial_period_transmit_cck_counter %= serial_period_transmit_ccks;
			serial_period_transmit_cck_counter += maxhpos - 1;
			checkshiftempty();
		}
	} else {
		serial_period_transmit_cck_counter = 0;
	}
	serial_period_receive_cck_counter += maxhpos;
	if (serial_period_receive_cck_counter >= serial_period_receive_ccks && serial_period_receive_ccks) {
		serial_period_receive_cck_counter %= serial_period_receive_ccks;
		serial_period_receive_cck_counter += maxhpos - 1;
		receive_next_buffered();
		checkreceive_serial();
		checkreceive_enet();
	}


	if (break_in_serdatr > 1) {
		break_in_serdatr--;
		if (break_in_serdatr == 1) {
			flushser();
		}
	}
}

void setserstat(int mask, int onoff)
{
#ifdef USE_LIBSERIALPORT
	if (mask & SP_FLOWCONTROL_DTRDSR)
	{
		if (currprefs.use_serial && port != nullptr)
		{
			check(sp_set_flowcontrol(port, onoff ? SP_FLOWCONTROL_DTRDSR : SP_FLOWCONTROL_NONE));
			check(sp_set_dtr(port, onoff ? SP_DTR_ON : SP_DTR_OFF));
		}
	}
	if (!currprefs.serial_hwctsrts)
	{
		if (mask & SP_FLOWCONTROL_RTSCTS)
		{
			if (currprefs.use_serial && port != nullptr)
			{
				check(sp_set_flowcontrol(port, onoff ? SP_FLOWCONTROL_RTSCTS : SP_FLOWCONTROL_NONE));
				check(sp_set_rts(port, onoff ? SP_RTS_ON : SP_RTS_OFF));
			}
		}
	}
#endif
}

int setbaud(int baud, int org_baud)
{
#ifdef WITH_MIDI
	if (org_baud == 31400 && _tcscmp(currprefs.midioutdev, "none") != 0) {
		/* MIDI baud-rate */
#ifdef WITH_MIDIEMU
		if (currprefs.midioutdev[0]) {
			if (!_tcsncmp(currprefs.midioutdev, _T("Munt "), 5)) {
				midi_emu_open(currprefs.midioutdev);
				return 1;
			}
		}
#endif
		if (currprefs.midioutdev[0] && !midi_ready) {
			/* try to open midi devices */
			if (midi_open()) {
				midi_ready = 1;
			}
		}
	} else {
		if (midi_ready) {
			midi_close();
			midi_ready = 0;
		}
#ifdef WITH_MIDIEMU
		if (midi_emu) {
			midi_emu_close();
		}
#endif
	}
#endif
	if (!currprefs.use_serial)
		return 1;
#ifdef USE_LIBSERIALPORT
	if (port != nullptr)
	{
		check(sp_set_baudrate(port, baud));
	}
#endif
	return 1;
}

void SERPER(uae_u16 w)
{
	int baud = 0, mbaud = 0;
	int oldper = serper;

	if (serper == w && serper_set)  /* don't set baudrate if it's already ok */
		return;

	safe_receive = false;
	serper_set = true;
	ninebit = 0;
	serper = w;
	if (w & 0x8000)
		ninebit = 1;
	w &= 0x7fff;

	int per = w;
	if (per == 0)
		per = 1;
	per = (currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL) / (per + 1);
	if (per <= 0)
		per = 1;
	int i = 0;
	while (allowed_baudrates[i] >= 0 && per > allowed_baudrates[i] * 100 / 97)
		i++;
	baud = allowed_baudrates[i];
	if (baud <= 0) {
		baud = allowed_baudrates[1];
	}
	mbaud = baud;

	serial_period_transmit_ccks = ((serper & 0x7fff) + 1) * (1 + 8 + ninebit + 1 - 1);
	serial_period_receive_ccks = serial_period_transmit_ccks / 4;

#if SERIALLOGGING > 0
	serial_period_hsyncs = 1;
	seriallog = 1;
	serial_period_transmit_ccks = maxhpos;
#endif

	if (log_sercon > 0) {
		serial_period_hsyncs = 1;
		seriallog = log_sercon;
		seriallog_lf = true;
		write_log(_T("\n"));
	}

	if (!serloop_enabled || seriallog > 0) {
		write_log(_T("SERIAL: period=%d/%d, baud=%d, hsyncs=%d, bits=%d, PC=%x\n"), w, serial_period_transmit_ccks, baud, serial_period_transmit_ccks / maxhpos, ninebit ? 9 : 8, M68K_GETPC);
	}

	if (ninebit) {
		baud *= 2;
	}
	if (currprefs.serial_direct) {
		if (baud != 31400 && baud < 115200) {
			baud = 115200;
		}
		serial_period_transmit_ccks = maxhpos;
		serial_period_receive_ccks = maxhpos;
		safe_receive = true;
	}
	if (sermap_enabled || serxdevice_enabled) {
		safe_receive = true;
	}

	serial_recv_previous = -1;
	serial_send_previous = -1;
#ifdef SERIAL_PORT
	setbaud(baud, mbaud);
#endif

	flushqueue();

	// mid transmit period change
	if ((serloop_enabled || serempty_enabled) && ser_accurate) {
		evt_t c = get_cycles();
		evt_t n = serper_tx_evt + serper_tx_cycles * CYCLE_UNIT;
		if (n > c) {
			const int cycles_transmitted = static_cast<int>((c - serper_tx_evt) / CYCLE_UNIT);
			serper_tx_cycles -= cycles_transmitted;
			if (serper_tx_cycles >= 0) {
				const int serper_tx_cycles_mod = serper_tx_cycles % serper_tx_per;
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

static void flushqueue(void)
{
#ifdef SERIAL_MAP
	if (sermap2 && sermap_enabled) {
		while (shmem_serial_receive() != 0xffffffff);
	}
#endif
	for (;;) {
		bool breakcond;
		if (!readseravail(&breakcond)) {
			break;
		}
	}
}

static void SERDAT_send(uae_u32 v)
{
	auto w = static_cast<uae_u16>(v);
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: SERDAT write 0x%04x (%c) PC=%x\n"), w, dochar(w), M68K_GETPC);
#endif

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

	} else {
		serdatcopy();

		serdat = w;

		if (!w) {
#if SERIALDEBUG > 1
			write_log(_T("SERIAL: zero serial word written?! PC=%x\n"), M68K_GETPC);
#endif
			return;
		}

#if SERIALDEBUG > 1
		if (data_in_serdat) {
			write_log(_T("SERIAL: program wrote to SERDAT but old byte wasn't fetched yet\n"));
		}
#endif

		data_in_serdat = 1;
		serdatcopy();
	}
}

uae_u16 SERDATR()
{
	serdatr &= 0x03ff;
	if (!data_in_serdat && (!ser_accurate || (ser_accurate && get_cycles() >= data_in_serdat_delay))) {
		serdatr |= 0x2000; // TBE (Transmit buffer empty)
	}
	if (!data_in_sershift && (serdatr & 0x2000)) {
		serdatr |= 0x1000; // TSRE (Transmit shift register empty)
	}
	if (data_in_serdatr & SERDATR_IRQACK) {
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
		} else if (diff > 0) {
			const int bit = static_cast<int>(c - serdatshift_start) / per;
			if (bit > 0 && !(serdatshift & (1 << (bit - 1)))) {
				serdatr |= 0x0800;
			}
		} else {
			if (break_in_serdatr <= 0) {
				serdatr |= 0x0800;
			}
		}
	} else {
		if (break_in_serdatr <= 0) {
			serdatr |= 0x0800; // RXD
		}
	}
#if SERIALDEBUG > 2
	write_log(_T("SERIAL: read 0x%04x (%c) %x\n"), serdatr, dochar(serdatr), M68K_GETPC);
#endif
	data_in_serdatr &= ~SERDATR_READ;
	if (!data_in_serdatr) {
		// interrupt was previously cleared but SERDATR was not read.
		// Clear it now when SERDATR was read.
		INTREQ_f(1 << 11);
	}
	return serdatr;
}

void SERDAT(uae_u16 w)
{
	if (ser_accurate) {
		event2_newevent_xx(-1, 1 * CYCLE_UNIT, w, SERDAT_send);
	} else {
		SERDAT_send(w);
	}
}

void serial_rbf_change(bool set)
{
	ovrun = set;
	if (!set) {
		data_in_serdatr &= ~SERDATR_IRQACK;
	}
}

void serial_dtr_on()
{
#if SERIALHSDEBUG > 0
	write_log("SERIAL: DTR on\n");
#endif
#ifdef RETROPLATFORM
	if (rp_ismodem()) {
		rp_modemstate(1);
	}
#endif
	dtr = 1;
#ifdef SERIAL_PORT
	if (currprefs.serial_demand)
		serial_open();
#ifdef USE_LIBSERIALPORT
	setserstat(SP_FLOWCONTROL_DTRDSR, dtr);
#endif
#endif
}

void serial_dtr_off()
{
#if SERIALHSDEBUG > 0
	write_log("SERIAL: DTR off\n");
#endif
#ifdef RETROPLATFORM
	if (rp_ismodem()) {
		rp_modemstate(0);
	}
#endif
	dtr = 0;
#ifdef SERIAL_PORT
	if (currprefs.serial_demand)
		serial_close();
#ifdef USE_LIBSERIALPORT
	setserstat(SP_FLOWCONTROL_DTRDSR, dtr);
#endif
#endif
}

void serial_flush_buffer ()
{
}

static uae_u8 oldserbits;

static void serial_status_debug(const TCHAR* s)
{
#if SERIALHSDEBUG > 1
	write_log(_T("%s: DTR=%d RTS=%d CD=%d CTS=%d DSR=%d\n"), s,
		(oldserbits & 0x80) ? 0 : 1, (oldserbits & 0x40) ? 0 : 1,
		(oldserbits & 0x20) ? 0 : 1, (oldserbits & 0x10) ? 0 : 1, (oldserbits & 0x08) ? 0 : 1);
#endif
}

uae_u8 serial_readstatus(uae_u8 v, uae_u8 dir)
{
#ifdef USE_LIBSERIALPORT
	sp_signal signal;
	uae_u8 serbits = oldserbits;

	if (serloop_enabled) {
		if (serstatus & 0x80) { // DTR -> DSR + CD
			signal = SP_SIG_DSR;
		}
		if (serstatus & 0x10) { // RTS -> CTS
			signal = SP_SIG_CTS;
		}
#ifdef RETROPLATFORM
	} else if (rp_ismodem()) {
		bool dsr, cd, cts, ri;
		rp_readmodemstatus(&dsr, &cd, &cts, &ri);
		if (dsr) {
			signal |= SP_SIG_DSR;
		}
		if (cd) {
			signal |= SP_SIG_DCD;
		}
		if (cts) {
			signal |= SP_SIG_CTS;
		}
		if (ri) {
			signal |= SP_SIG_RI;
		}
#endif
#ifdef SERIAL_MAP
	} else if (sermap_enabled) {
		if (sermap_flags & 1) {
			signal = SP_SIG_DSR;
		}
		if (sermap_flags & 2) {
			signal = SP_SIG_DCD;
		}
		if (sermap_flags & 4) {
			signal = SP_SIG_CTS;
		}
#endif
	} else if (currprefs.use_serial) {
#ifdef SERIAL_PORT
		/* Read the current config from the port into that configuration. */
		if (port != nullptr)
			check(sp_get_signals(port, &signal));
#endif
	}
	else {
		return v;
	}

	if (currprefs.serial_rtsctsdtrdtecd) {
		if (!(signal & SP_SIG_DCD)) {
			if (!(serbits & 0x20)) {
				serbits |= 0x20;
#if SERIALHSDEBUG > 0
				write_log("SERIAL: CD off\n");
#endif
			}
		}
		else {
			if (serbits & 0x20) {
				serbits &= ~0x20;
#if SERIALHSDEBUG > 0
				write_log("SERIAL: CD on\n");
#endif
			}
		}

		if (!(signal & SP_SIG_DSR)) {
			if (!(serbits & 0x08)) {
				serbits |= 0x08;
#if SERIALHSDEBUG > 0
				write_log("SERIAL: DSR off\n");
#endif
			}
		}
		else {
			if (serbits & 0x08) {
				serbits &= ~0x08;
#if SERIALHSDEBUG > 0
				write_log("SERIAL: DSR on\n");
#endif
			}
		}

		if (!(signal & SP_SIG_CTS)) {
			if (!(serbits & 0x10)) {
				serbits |= 0x10;
#if SERIALHSDEBUG > 0
				write_log("SERIAL: CTS off\n");
#endif
			}
		}
		else {
			if (serbits & 0x10) {
				serbits &= ~0x10;
#if SERIALHSDEBUG > 0
				write_log("SERIAL: CTS on\n");
#endif
			}
		}
	}

	// SEL == RI
	if (isprinter()) {
		serbits |= 0x04;
		serbits |= v & 0x04;
		if (currprefs.serial_ri && (signal & SP_SIG_RI)) {
			serbits &= ~0x04;
		}
	} else if (currprefs.serial_ri) {
		serbits |= 0x04;
		if (signal & SP_SIG_RI) {
			serbits &= ~0x04;
		}
	} else {
		serbits &= ~0x04;
		serbits |= v & 0x04;
	}

	serbits &= 0x04 | 0x08 | 0x10 | 0x20;
	oldserbits &= ~(0x04 | 0x08 | 0x10 | 0x20);
	oldserbits |= serbits;

	serial_status_debug(_T("read"));

	v = (v & (0x80 | 0x40 | 0x02 | 0x01)) | serbits;
	return v;
#else
	return 0;
#endif
}

uae_u8 serial_writestatus(uae_u8 newstate, uae_u8 dir)
{
	static int logcnt = 10;

	serstatus = newstate & dir;

#ifdef SERIAL_PORT
	if (currprefs.use_serial) {
		if (currprefs.serial_rtsctsdtrdtecd && ((oldserbits ^ newstate) & 0x80) && (dir & 0x80)) {
			if (newstate & 0x80)
				serial_dtr_off();
			else
				serial_dtr_on();
		}
#ifdef USE_LIBSERIALPORT
		if (!currprefs.serial_hwctsrts && currprefs.serial_rtsctsdtrdtecd && (dir & 0x40)) {
			if ((oldserbits ^ newstate) & 0x40) {
				if (newstate & 0x40) {
					setserstat(SP_FLOWCONTROL_RTSCTS, 0);
#if SERIALHSDEBUG > 0
					write_log(_T("SERIAL: RTS cleared\n"));
#endif
				} else {
					setserstat(SP_FLOWCONTROL_RTSCTS, 1);
#if SERIALHSDEBUG > 0
					write_log(_T("SERIAL: RTS set\n"));
#endif
				}
			}
		}
#endif
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
#ifdef RETROPLATFORM
	if (rp_ismodem()) {
		if ((oldserbits & (0x80 | 0x40)) != (newstate & (0x80 | 0x40))) {
			rp_writemodemstatus(
				(newstate & 0x40) == 0, (oldserbits & 0x40) != (newstate & 0x40),
				(newstate & 0x80) == 0, (oldserbits & 0x80) != (newstate & 0x80));
		}
	}
#endif
#endif

#ifdef SERIAL_MAP
	if (sermap_data && sermap_enabled) {
		uae_u32 flags = 0x80000000;
		bool changed = false;
		if (currprefs.serial_rtsctsdtrdtecd && ((oldserbits ^ newstate) & 0x80) && (dir & 0x80)) {
			if (!(newstate & 0x80)) {
				flags |= 1; // DSR
				flags |= 2; // CD
			}
			changed = true;
		}
		if (!currprefs.serial_hwctsrts && currprefs.serial_rtsctsdtrdtecd && (dir & 0x40)) {
			if ((oldserbits ^ newstate) & 0x40) {
				if (!(newstate & 0x40)) {
					flags |= 4; // RTS
				}
				changed = true;
			}
		}
		if (changed) {
			shmem_serial_send(flags);
		}
	}
#endif

	oldserbits &= ~(0x80 | 0x40);
	newstate &= 0x80 | 0x40;
	oldserbits |= newstate;

	serial_status_debug(_T("write"));

	return oldserbits;
}

static int enet_is(const TCHAR* name)
{
	return !_tcsnicmp(name, _T("ENET:"), 5);
}

void serial_open()
{
#ifdef SERIAL_PORT
	if (serdev)
		return;
	serper = 0;
	if (false) {
#ifdef SERIAL_ENET
	}
	else if (enet_is(currprefs.sername)) {
		enet_open(currprefs.sername);
#endif
#ifdef SERIAL_MAP
	}
	else if (!_tcsicmp(currprefs.sername, SERIAL_INTERNAL)) {
		sermap_enabled = true;
#endif
	}
	else if (!_tcsicmp(currprefs.sername, SERIAL_LOOPBACK)) {
		serloop_enabled = true;
	}
	else if (!currprefs.sername[0] && !currprefs.midioutdev[0] && !currprefs.midiindev[0]) {
		serempty_enabled = true;
	}
	else {
		if (currprefs.sername[0] && !openser(currprefs.sername)) {
			write_log(_T("SERIAL: Could not open device %s\n"), currprefs.sername);
			return;
		}
	}

	if (
#ifdef ARCADIA
		alg_flag ||
#endif
		currprefs.genlock_image >= 7 || cubo_enabled 
#ifdef SERIAL_MAP
		|| sermap_enabled
#endif
#ifdef RETROPLATFORM		
		|| rp_ismodem()
#endif
		) {
		serxdevice_enabled = true;
	}
	serdev = 1;
	ser_accurate = currprefs.cpu_memory_cycle_exact || (currprefs.cpu_model <= 68020 && currprefs.cpu_compatible && currprefs.m68k_speed == 0);
#endif
}

void serial_close()
{
#ifdef SERIAL_PORT
	closeser();
#ifdef SERIAL_ENET
	enet_close();
#endif
	serdev = 0;
#ifdef SERIAL_MAP
	sermap_deactivate();
#endif
#endif
#ifdef RETROPLATFORM
	if (rp_ismodem()) {
		rp_modemstate(0);
	}
#endif
	if (receive_buf) {
		xfree(receive_buf);
		receive_buf = nullptr;
	}
	receive_buf_size = 0;
	receive_buf_count = 0;

	serloop_enabled = false;
	serempty_enabled = false;
	serxdevice_enabled = false;
	serper_set = false;
	ser_accurate = false;
	data_in_serdat_delay = 0;
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

void serial_exit ()
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

void serial_uartbreak (int v)
{
#ifdef SERIAL_MAP
	if (sermap_data && sermap_enabled) {
		shmem_serial_send(0x40000000 | (v ? 0x20000 : 0x10000));
	}
#endif
#ifdef USE_LIBSERIALPORT
	if (!port || !currprefs.use_serial)
		return;
#endif
#ifdef SERIAL_PORT
#ifdef USE_LIBSERIALPORT
	if (v)
	{
		check(sp_start_break(port));
	}
	else
	{
		check(sp_end_break(port));
	}
#endif
#endif
}

#ifdef USE_LIBSERIALPORT
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
#endif

#ifdef SERIAL_ENET
static ENetHost* enethost, * enetclient;
static ENetPeer* enetpeer, * enet;
static int enetmode;
static uae_u16 enet_receive[256];
static int enet_receive_off_w, enet_receive_off_r;

static void enet_service(int serveronly)
{
	ENetEvent evt;
	ENetAddress address;

	if (enetmode == 0)
		return;

	int got = 1;
	while (got) {
		got = 0;
		if (enetmode > 0) {
			while (enet_host_service(enethost, &evt, 0)) {
				got = 1;
				switch (evt.type)
				{
				case ENET_EVENT_TYPE_CONNECT:
					address = evt.peer->address;
					write_log(_T("ENET_SERVER: connect from %d.%d.%d.%d:%u\n"),
						(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff,
						address.port);
					evt.peer->data = nullptr;
					break;
				case ENET_EVENT_TYPE_RECEIVE:
				{
					uae_u8* p = evt.packet->data;
					size_t len = evt.packet->dataLength;
					if (len == 6 && !memcmp(p, "UAE_", 4)) {
						if (((enet_receive_off_w + 1) & 0xff) != enet_receive_off_r) {
							enet_receive[enet_receive_off_w++] = (p[4] << 8) | p[5];
						}
					}

					enet_packet_destroy(evt.packet);
				}
				break;
				case ENET_EVENT_TYPE_DISCONNECT:
					address = evt.peer->address;
					write_log(_T("ENET_SERVER: disconnect from %d.%d.%d.%d:%u\n"),
						(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff,
						address.port);
					break;
				default: break;
				}
			}
		}
		if (!serveronly) {
			while (enet_host_service(enetclient, &evt, 0)) {
				got = 1;
				switch (evt.type)
				{
				default:
				case 0:
					write_log(_T("ENET_CLIENT: %d\n"), evt.type);
					break;
				}
			}
		}
	}
}

static void enet_disconnect(ENetPeer* peer)
{
	ENetEvent evt;
	int cnt = 30;

	if (!peer)
		return;

	write_log(_T("ENET_CLIENT: disconnecting..\n"));
	enet_peer_disconnect(peer, 0);
	while (cnt-- > 0) {
		enet_service(1);
		while (enet_host_service(enetclient, &evt, 100) > 0)
		{
			switch (evt.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(evt.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				write_log(_T("ENET_CLIENT: disconnection succeeded\n"));
				enetpeer = nullptr;
				return;
			default: break;
			}
		}
	}
	write_log(_T("ENET_CLIENT: disconnection forced\n"));
	enet_peer_reset(enetpeer);
	enetpeer = nullptr;
}

void enet_close()
{
	enet_disconnect(enetpeer);
	if (enetclient)
		enet_host_destroy(enetclient);
	enetclient = nullptr;
	if (enethost)
		enet_host_destroy(enethost);
	enethost = nullptr;
	serial_enet = 0;
	enetmode = 0;
}

int enet_open(const TCHAR* name)
{
	ENetAddress address;
	static int initialized;
	uae_u8 data[16];

	if (!initialized) {
		const int err = enet_initialize();
		if (err) {
			write_log(_T("ENET: initialization failed: %d\n"), err);
			return 0;
		}
		initialized = 1;
	}

	enet_close();
	enetmode = 0;
	if (!_tcsnicmp(name, _T("ENET:H"), 6)) {
		address.host = ENET_HOST_ANY;
		address.port = 1234;
		enethost = enet_host_create(&address, 2, 0, 0, 0);
		if (enethost == nullptr) {
			write_log(_T("ENET_SERVER: enet_host_create(server) failed\n"));
			enet_close();
			return 0;
		}
		write_log(_T("ENET_SERVER: server created\n"));
		enetmode = 1;
	}
	else {
		enetmode = -1;
	}
	enetclient = enet_host_create(nullptr, 1, 0, 0, 0);
	if (enetclient == nullptr) {
		write_log(_T("ENET_CLIENT: enet_host_create(client) failed\n"));
		enet_close();
		return 0;
	}
	write_log(_T("ENET_CLIENT: client created\n"));
	enet_address_set_host(&address, enetmode > 0 ? "127.0.0.1" : "192.168.0.10");
	address.port = 1234;
	enetpeer = enet_host_connect(enetclient, &address, 2, 0);
	if (enetpeer == nullptr) {
		write_log(_T("ENET_CLIENT: connection to host %d.%d.%d.%d:%d failed\n"),
			(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff, address.port);
		enet_host_destroy(enetclient);
		enetclient = nullptr;
	}
	write_log(_T("ENET_CLIENT: connecting to %d.%d.%d.%d:%d...\n"),
		(address.host >> 0) & 0xff, (address.host >> 8) & 0xff, (address.host >> 16) & 0xff, (address.host >> 24) & 0xff, address.port);
	int cnt = 10 * 5;
	while (cnt-- > 0) {
		ENetEvent evt;
		enet_service(0);
		if (enet_host_service(enetclient, &evt, 100) > 0) {
			if (evt.type == ENET_EVENT_TYPE_CONNECT)
				break;
		}
	}
	if (cnt <= 0) {
		write_log(_T("ENET_CLIENT: connection failed, no response in 5 seconds\n"));
		enet_close();
		return 0;
	}
	memcpy(data, "UAE_HELLO", 10);
	ENetPacket* p = enet_packet_create(data, sizeof data, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(enetpeer, 0, p);
	enet_host_flush(enetclient);
	write_log(_T("ENET: connected\n"));
	serial_enet = 1;
	return 1;
}

void enet_writeser(uae_u16 w)
{
	uae_u8 data[16];

	memcpy(data, "UAE_", 4);
	data[4] = w >> 8;
	data[5] = w >> 0;
	write_log(_T("W=%04X "), w);
	ENetPacket* p = enet_packet_create(data, 6, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(enetpeer, 0, p);
	enet_host_flush(enetclient);
}

int enet_readseravail()
{
	enet_service(0);
	return enet_receive_off_r != enet_receive_off_w;
}

int enet_readser(uae_u16* data)
{
	if (enet_receive_off_r == enet_receive_off_w)
		return 0;
	*data = enet_receive[enet_receive_off_r++];
	write_log(_T("R=%04X "), *data);
	enet_receive_off_r &= 0xff;
	return 1;
}
#endif
