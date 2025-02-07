/*
 * UAE - The Un*x Amiga Emulator
 *
 * Not a parser, but parallel and serial emulation for Linux
 *
 * Copyright 2023 Dimitris Panokostas
 * Copyright 2010 Mustafa TUFAN
 */

#include "sysconfig.h"

#include <cstring>
#include <cstdio>

#undef SERIAL_ENET

#include "config.h"
#include "sysdeps.h"
#include "options.h"
#include "events.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "autoconf.h"
#include "newcpu.h"
#include "traps.h"
#include "picasso96.h"
#include "threaddep/thread.h"
#include "serial.h"
#include "parser.h"
#include "ioport.h"
#include "parallel.h"
#include "cia.h"
#include "savestate.h"
#include "xwin.h"
#include "drawing.h"
#include "vpar.h"
#include "ahi_v1.h"

#ifdef POSIX_SERIAL
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#endif

#ifdef WITH_MIDI
#include "portmidi.h"
#endif
#ifdef WITH_MIDIEMU
#include "midiemu.h"
#endif

#include "uae/socket.h"

#ifdef USE_LIBSERIALPORT
#include <libserialport.h>
#endif

#if !defined B300 || !defined B1200 || !defined B2400 || !defined B4800 || !defined B9600
#undef POSIX_SERIAL
#endif
#if !defined B19200 || !defined B57600 || !defined B115200 || !defined B230400
#undef POSIX_SERIAL
#endif

#ifdef POSIX_SERIAL
struct termios tios;
#endif

#define MIN_PRTBYTES 10

#define PARALLEL_MODE_NONE 0
#define PARALLEL_MODE_TCP_PRINTER 1

int parallel_mode = 0;
static uae_socket parallel_tcp_listener = UAE_SOCKET_INVALID;
static uae_socket parallel_tcp = UAE_SOCKET_INVALID;

static bool parallel_tcp_connected(void)
{
	if (parallel_tcp_listener == UAE_SOCKET_INVALID) {
		return false;
	}
	if (parallel_tcp == UAE_SOCKET_INVALID) {
		if (uae_socket_select_read(parallel_tcp_listener)) {
			parallel_tcp = uae_socket_accept(parallel_tcp_listener);
			if (parallel_tcp != UAE_SOCKET_INVALID) {
				write_log(_T("TCP: Parallel connection accepted\n"));
			}
		}
	}
	return parallel_tcp != UAE_SOCKET_INVALID;
}

static void parallel_tcp_disconnect(void)
{
	if (parallel_tcp == UAE_SOCKET_INVALID) {
		return;
	}
	uae_socket_close(parallel_tcp);
	parallel_tcp = UAE_SOCKET_INVALID;
	write_log(_T("TCP: Parallel disconnect\n"));
}

static void parallel_tcp_open(const TCHAR *name)
{
	parallel_tcp_listener = uae_tcp_listen_uri(
			name, "1235", UAE_SOCKET_DEFAULT);
	if (parallel_tcp_listener != UAE_SOCKET_INVALID) {
		parallel_mode = PARALLEL_MODE_TCP_PRINTER;
		if (_tcsicmp(uae_uri_path(name), _T("/wait")) == 0) {
			while (parallel_tcp_connected() == false) {
				sleep_millis(1000);
				write_log(_T("TCP: Waiting for parallel connection...\n"));
			}
		}
	}
}

static void parallel_tcp_close(void)
{
	if (parallel_tcp_listener == UAE_SOCKET_INVALID) {
		return;
	}
	parallel_tcp_disconnect();
	uae_socket_close(parallel_tcp_listener);
	parallel_tcp_listener = UAE_SOCKET_INVALID;
	write_log("TCP: Parallel listener socket closed\n");
}

void parallel_ack(void)
{
	if (0) {
#ifdef WITH_VPAR
		} else if (vpar_enabled()) {
		/* Do nothing, acking is instead done via parallel_poll_ack. */
#endif
	} else {
		cia_parallelack();
	}
}

void parallel_poll_ack(void)
{
#ifdef WITH_VPAR
	vpar_update();
#endif
}

void parallel_exit(void)
{
	parallel_tcp_close();
#ifdef WITH_VPAR
	vpar_close();
#endif
}

int isprinter (void)
{
	if (parallel_mode == PARALLEL_MODE_TCP_PRINTER) {
		return 1;
	}
#ifdef WITH_VPAR
	if (par_fd >= 0) {
        return par_mode;
    }
#endif
	return 0;
}

void flushprinter (void)
{
	// not implemented
}

void doprinter (uae_u8 val)
{
	if (parallel_mode == PARALLEL_MODE_TCP_PRINTER) {
		if (parallel_tcp_connected()) {
			if (uae_socket_write(parallel_tcp, &val, 1) != 1) {
				parallel_tcp_disconnect ();
			}
		}
	}

#ifdef WITH_VPAR
	if (par_fd >= 0) {
        if (write(par_fd, &val, 1) != 1) {
            write_log("VPAR: Writing one byte failed\n");
        }
    }
#endif
}

struct uaeserialdata
{
#ifdef USE_LIBSERIALPORT
	sp_port* port;
#endif
	int writeactive;

	pthread_t thread;
	volatile int threadactive;
	uae_sem_t evtwce, evtt, evtw, change_sem, sync_sem;
	void *user;
};

int uaeser_getdatalength (void)
{
	return sizeof (uaeserialdata);
}

static void uaeser_initdata (void *vsd, void *user)
{
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
	memset(sd, 0, sizeof(uaeserialdata));
#ifdef USE_LIBSERIALPORT
	sd->port = NULL;
#endif
	sd->user = user;
}

int uaeser_query(void* vsd, uae_u16* status, uae_u32* pending)
{
#ifdef USE_LIBSERIALPORT
	struct uaeserialdata* sd = static_cast<struct uaeserialdata*>(vsd);
	int bytes_available;
	enum sp_signal signals;
	uae_u16 s = 0;

	if (!sd->port) {
		return 0;
	}

	bytes_available = sp_input_waiting(sd->port);
	if (bytes_available < 0) {
		write_log("Error checking input buffer: %s\n", sp_last_error_message());
		return 0;
	}
	*pending = bytes_available;

	if (status) {
		if (sp_get_signals(sd->port, &signals) != SP_OK) {
			write_log("Error getting serial port signals: %s\n", sp_last_error_message());
			return 0;
		}
		//s |= (signals & SP_SIG_BREAK) ? (1 << 10) : 0;
		s |= (signals & SP_SIG_CTS) ? 0 : (1 << 4);
		s |= (signals & SP_SIG_DSR) ? 0 : (1 << 7);
		s |= (signals & SP_SIG_RI) ? (1 << 2) : 0;
		*status = s;
	}
	return 1;
#else
	return 0;
#endif
}

int uaeser_break(void* vsd, int brklen) {
#ifdef USE_LIBSERIALPORT
	struct uaeserialdata* sd = static_cast<struct uaeserialdata*>(vsd);
	if (!sd->port) {
		return 0;
	}
	
	if (sp_end_break(sd->port) != SP_OK) {
		write_log("Error sending break signal: %s\n", sp_last_error_message());
		return 0;
	}
	Sleep(brklen / 1000);
	return 1;
#else
	return 0;
#endif
}

int uaeser_setparams(void* vsd, int baud, int rbuffer, int bits, int sbits, int rtscts, int parity, uae_u32 xonxoff)
{
#ifdef USE_LIBSERIALPORT
	struct sp_port* port = static_cast<struct sp_port*>(vsd);
	struct sp_port_config* config;
	sp_return result;

	result = sp_new_config(&config);
	if (result != SP_OK) {
		write_log("Failed to create port config: %s\n", sp_last_error_message());
		return 5;
	}

	result = sp_set_config_baudrate(config, baud);
	if (result != SP_OK) {
		write_log("Failed to set baud rate: %s\n", sp_last_error_message());
		sp_free_config(config);
		return 5;
	}

	result = sp_set_config_bits(config, bits);
	if (result != SP_OK) {
		write_log("Failed to set data bits: %s\n", sp_last_error_message());
		sp_free_config(config);
		return 5;
	}

	enum sp_parity sp_parity;
	switch (parity) {
	case 1:
		sp_parity = SP_PARITY_ODD;
		break;
	case 2:
		sp_parity = SP_PARITY_EVEN;
		break;
	case 3:
		sp_parity = SP_PARITY_MARK;
		break;
	case 4:
		sp_parity = SP_PARITY_SPACE;
		break;
	default:
		sp_parity = SP_PARITY_NONE;
		break;
	}

	result = sp_set_config_parity(config, sp_parity);
	if (result != SP_OK) {
		write_log("Failed to set parity: %s\n", sp_last_error_message());
		sp_free_config(config);
		return 5;
	}

	result = sp_set_config_stopbits(config, sbits);
	if (result != SP_OK) {
		write_log("Failed to set stop bits: %s\n", sp_last_error_message());
		sp_free_config(config);
		return 5;
	}

	enum sp_flowcontrol sp_flowcontrol = rtscts ? SP_FLOWCONTROL_RTSCTS : SP_FLOWCONTROL_NONE;
	result = sp_set_config_flowcontrol(config, sp_flowcontrol);
	if (result != SP_OK) {
		write_log("Failed to set flow control: %s\n", sp_last_error_message());
		sp_free_config(config);
		return 5;
	}

	enum sp_xonxoff sp_xonxoff = xonxoff ? SP_XONXOFF_INOUT : SP_XONXOFF_DISABLED;
	if (xonxoff & 1) {
		result = sp_set_config_xon_xoff(config, sp_xonxoff);
		if (result != SP_OK) {
			write_log("Failed to set XON/XOFF: %s\n", sp_last_error_message());
			sp_free_config(config);
			return 5;
		}
	}
	else {
		result = sp_set_config_xon_xoff(config, sp_xonxoff);
		if (result != SP_OK) {
			write_log("Failed to disable XON/XOFF: %s\n", sp_last_error_message());
			sp_free_config(config);
			return 5;
		}
	}

	result = sp_set_config(port, config);
	if (result != SP_OK) {
		write_log("Failed to apply port config: %s\n", sp_last_error_message());
		sp_free_config(config);
		return 5;
	}

	sp_free_config(config);
#endif
	return 0;
}

static void startwce(struct uaeserialdata* sd, struct sp_event_set* evtset)
{
#ifdef USE_LIBSERIALPORT
	uae_sem_post(&sd->evtwce);
	sp_wait(evtset, 0);
#endif
}

static int uaeser_trap_thread (void *arg)
{
#ifdef USE_LIBSERIALPORT
	struct uaeserialdata* sd = static_cast<struct uaeserialdata*>(arg);
	struct sp_event_set* event_set;
	enum sp_event evtmask = static_cast<enum sp_event>(SP_EVENT_RX_READY | SP_EVENT_TX_READY);

	uae_set_thread_priority(NULL, 1);
	sd->threadactive = 1;
	uae_sem_post(&sd->sync_sem);

	// Create an event set
	sp_new_event_set(&event_set);
	sp_add_port_events(event_set, sd->port, evtmask);

	startwce(sd, event_set);

	while (sd->threadactive == 1) {
		int sigmask = 0;
		uae_sem_wait(&sd->change_sem);
		if (uae_sem_trywait(&sd->evtwce) == 0) {
			if (sp_input_waiting(sd->port) > 0)
				sigmask |= 1;
			if ((sp_output_waiting(sd->port) == 0) && !sd->writeactive)
				sigmask |= 2;
			//if (evtmask & SP_EVENT_BREAK)
			//	sigmask |= 4;
			startwce(sd, event_set);
		}

		if (sd->writeactive) {
			int actual = 0;
			if (sp_output_waiting(sd->port) == 0) {
				sd->writeactive = 0;
				sigmask |= 2;
			}
		}

		if (!sd->writeactive)
			sigmask |= 2;

		uaeser_signal(sd->user, sigmask | 1);
		uae_sem_post(&sd->change_sem);
		usleep(1000); // Sleep for a short period to prevent busy-waiting
	}

	sd->threadactive = 0;
	uae_sem_post(&sd->sync_sem);
#endif
	return 0;
}

void uaeser_trigger (void *vsd)
{
#ifdef USE_LIBSERIALPORT
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);
	uae_sem_post (&sd->evtt);
#endif
}

int uaeser_write(void* vsd, uae_u8* data, uae_u32 len)
{
#ifdef USE_LIBSERIALPORT
	struct uaeserialdata* sd = static_cast<struct uaeserialdata*>(vsd);
	int ret = 1;
	enum sp_return result = sp_blocking_write(sd->port, data, len, 0);

	if (result < 0) {
		sd->writeactive = 1;
		if (result != SP_ERR_FAIL) {
			ret = 0;
			sd->writeactive = 0;
		}
	}
	uae_sem_post(&sd->evtt);
	return ret;
#else
	return 1;
#endif
}

int uaeser_read(void* vsd, uae_u8* data, uae_u32 len)
{
#ifdef USE_LIBSERIALPORT
	struct uaeserialdata* sd = static_cast<struct uaeserialdata*>(vsd);
	int ret = 1;
	int bytes_read = sp_blocking_read(sd->port, data, len, 0);

	if (bytes_read < 0) {
		if (bytes_read == SP_ERR_FAIL) {
			return -1; // Break condition
		}
		ret = 0;
	}
	else if (bytes_read < len) {
		return 0; // Not enough data available
	}

	uae_sem_post(&sd->evtt);
	return ret;
#else
	return 1;
#endif
}

void uaeser_clearbuffers (void *vsd)
{
#ifdef USE_LIBSERIALPORT
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);
	sp_flush(sd->port, SP_BUF_BOTH);
#endif
}

int uaeser_open (void *vsd, void *user, int unit)
{
#ifdef USE_LIBSERIALPORT
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
	char port_name[256];

	strncpy(port_name, currprefs.sername, sizeof(port_name) - 1);
	port_name[sizeof(port_name) - 1] = '\0'; // Ensure null-termination

	// Replace the last character with the value of unit
	size_t len = strlen(port_name);
	if (len > 0) {
		port_name[len - 1] = static_cast<char>('0' + unit); // Assuming unit is a single digit
	}

	sd->user = user;

	enum sp_return result = sp_get_port_by_name(port_name, &sd->port);
	if (result != SP_OK) {
		write_log("UAESER: Failed to find port '%s'\n", port_name);
		return 0;
	}

	result = sp_open(sd->port, SP_MODE_READ_WRITE);
	if (result != SP_OK) {
		write_log("UAESER: Failed to open port '%s'\n", port_name);
		return 0;
	}

	sp_set_baudrate(sd->port, 9600);
	sp_set_bits(sd->port, 8);
	sp_set_parity(sd->port, SP_PARITY_NONE);
	sp_set_stopbits(sd->port, 1);
	sp_set_flowcontrol(sd->port, SP_FLOWCONTROL_NONE);

	uae_sem_init (&sd->sync_sem, 0, 0);
	uae_sem_init (&sd->change_sem, 0, 1);
	uae_sem_init (&sd->evtt, 0, 0);
	uae_start_thread (_T("uaeserial_amiberry"), uaeser_trap_thread, sd, NULL);
	uae_sem_wait (&sd->sync_sem);
#endif
	return 1;
}

void uaeser_close(void* vsd)
{
#ifdef USE_LIBSERIALPORT
	auto* sd = static_cast<struct uaeserialdata*>(vsd);

	if (sd->threadactive) {
		sd->threadactive = -1;
		uae_sem_post(&sd->evtt);
		while (sd->threadactive)
			Sleep(10);
		uae_sem_destroy(&sd->evtt);
	}

	if (sd->port) {
		sp_close(sd->port);
		sp_free_port(sd->port);
		sd->port = NULL;
	}

	uae_sem_destroy(&sd->sync_sem);
	uae_sem_destroy(&sd->change_sem);

	uaeser_initdata(sd, sd->user);
#endif
}

void initparallel (void)
{
#ifdef AMIBERRY
	write_log("initparallel\n");
#endif
	if (_tcsnicmp(currprefs.prtname, "tcp:", 4) == 0) {
		parallel_tcp_open(currprefs.prtname);
	} else {
#ifdef WITH_VPAR
		vpar_open();
#endif
	}
	// We handle this in devices
#ifndef AMIBERRY
#ifdef AHI
	if (uae_boot_rom_type) {
#ifdef AMIBERRY
		write_log("installing ahi_winuae\n");
#endif
		uaecptr a = here (); //this install the ahisound
		org (rtarea_base + 0xFFC0);
		calltrap (deftrapres (ahi_demux, 0, _T("ahi_winuae")));
		dw (RTS);
		org (a);
#ifdef AHI_V2
		init_ahi_v2 ();
#endif
	}
#endif
#endif
}

int flashscreen = 0;

void doflashscreen (void)
{
#ifdef AMIBERRY

#else
	flashscreen = 10;
	init_colors ();
	picasso_refresh ();
	reset_drawing ();
	flush_screen (gfxvidinfo.outbuffer, 0, 0);
#endif
}

void hsyncstuff (void)
//only generate Interrupts when
//writebuffer is complete flushed
//check state of lwin rwin
{
	static int keycheck = 0;
// We handle this in devices instead
#ifndef AMIBERRY
#ifdef AHI
	{ //begin ahi_sound
		static int count;
		if (ahi_on) {
			count++;
			//15625/count freebuffer check
			if (count > ahi_pollrate) {
				ahi_updatesound (1);
				count = 0;
			}
		}
	} //end ahi_sound
#endif
#endif
#if 0 // Not implemented in Amiberry
#ifdef PARALLEL_PORT
	keycheck++;
	if(keycheck >= 1000)
	{
		if (prtopen)
			flushprtbuf ();
		{
			if (flashscreen > 0) {
				flashscreen--;
				if (flashscreen == 0) {
					init_colors(0);
					reset_drawing ();
					picasso_refresh(0);
					//flush_screen (gfxvidinfo.outbuffer, 0, 0);
				}
			}
		}
		keycheck = 0;
	}
	if (currprefs.parallel_autoflush_time && !currprefs.parallel_postscript_detection) {
		parflush++;
		if (parflush / ((currprefs.ntscmode ? MAXVPOS_NTSC : MAXVPOS_PAL) * MAXHPOS_PAL / maxhpos) >= currprefs.parallel_autoflush_time * 50) {
			flushprinter ();
			parflush = 0;
		}
	}
#endif
#endif
}

int enumserialports (void)
{
#ifdef USE_LIBSERIALPORT
	int cnt = 0;
	/* A pointer to a null-terminated array of pointers to
	* struct sp_port, which will contain the serial ports found.*/
	struct sp_port** port_list;

	write_log (_T("Serial port enumeration..\n"));
	/* Call sp_list_ports() to get the ports. The port_list
	* pointer will be updated to refer to the array created. */
	const enum sp_return result = sp_list_ports(&port_list);
	if (result != SP_OK)
	{
		write_log("sp_list_ports() failed!\n");
		return cnt;
	}
	else
	{
		serial_ports.clear();
		for (int i = 0; port_list[i] != nullptr; i++)
		{
			cnt++;
			const struct sp_port* port = port_list[i];

			/* Get the name of the port. */
			serial_ports.emplace_back(sp_get_port_name(port));
		}
		/* Free the array created by sp_list_ports(). */
		sp_free_port_list(port_list);
	}

	serial_ports.emplace_back(SERIAL_INTERNAL);
	cnt++;
	serial_ports.emplace_back(SERIAL_LOOPBACK);
	cnt++;
	serial_ports.emplace_back("TCP://0.0.0.0:1234");
	cnt++;
	serial_ports.emplace_back("TCP://0.0.0.0:1234/wait");
	cnt++;
	write_log(_T("Port enumeration end\n"));
	return cnt;
#else
	return 0;
#endif
}

int enummidiports (void)
{
	int total = 0;

#ifdef WITH_MIDI
	total = Pm_CountDevices();
	write_log(_T("MIDI: found devices: %d\n"), total);
	for(int i=0; i<total; i++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		write_log(_T("MIDI: %d: '%s', '%s' %s %s\n"),
		          i, info->interf, info->name,
		          info->input ? "IN" : "  ",
		          info->output ? "OUT" : "  ");

		if(info->input) {
			midi_in_ports.emplace_back(info->name);
		}
		if(info->output) {
			midi_out_ports.emplace_back(info->name);
		}
	}
#endif

#ifdef WITH_MIDIEMU
	midi_out_ports.emplace_back(midi_emu_available(_T("MT-32")) ? "Munt MT-32" : "Munt MT-32 (Missing ROMs)");
	total++;
	midi_out_ports.emplace_back(midi_emu_available(_T("CM-32L")) ? "Munt CM-32L" : "Munt CM-32L (Missing ROMs)");
	total++;
#endif

	return total;
}
