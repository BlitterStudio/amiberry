/*
 * UAE - The Un*x Amiga Emulator
 *
 * Not a parser, but parallel and serial emulation for Linux
 *
 * Copyright 2023 Dimitris Panokostas
 * Copyright 2010 Mustafa TUFAN
 */

#include "sysconfig.h"

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

#include "uae/socket.h"

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
	sp_port* port;
	int writeactive;

	volatile int threadactive;
	uae_sem_t change_sem, sync_sem, evtt;
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
	sd->port = NULL;
	sd->user = user;
}

int uaeser_query(void* vsd, uae_u16* status, uae_u32* pending)
{
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);
	uae_u16 s = 0;
	sp_signal signal;

	check(sp_get_signals(sd->port, &signal));
	s |= (signal & SP_SIG_CTS) ? 0 : (1 << 4);
	s |= (signal & SP_SIG_DSR) ? 0 : (1 << 7);
	s |= (signal & SP_SIG_RI) ? (1 << 2) : 0;
	*status = s;

	return 1;
}

int uaeser_break (void *vsd, int brklen)
{
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);
	if (!check(sp_start_break(sd->port)))
		return 0;
	Sleep(brklen / 1000);
	check(sp_end_break(sd->port));
	return 1;
}

int uaeser_setparams (void *vsd, int baud, int rbuffer, int bits, int sbits, int rtscts, int parity, uae_u32 xonxoff)
{
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);
	struct sp_port_config* initial_config;
	check(sp_new_config(&initial_config));
	if (!check(sp_new_config(&initial_config)))
	{
		sp_free_config(initial_config);
		return 5;
	}

	struct sp_port_config* new_config;
	check(sp_new_config(&new_config));
	check(sp_set_config_baudrate(new_config, baud));
	check(sp_set_config_bits(new_config, bits));
	check(sp_set_config_parity(new_config, parity == 0 ? SP_PARITY_NONE : (parity == 1 ? SP_PARITY_ODD : SP_PARITY_EVEN)));
	check(sp_set_config_stopbits(new_config, sbits));
	check(sp_set_config_dtr(new_config, SP_DTR_ON));
	if (rtscts)
	{
		check(sp_set_config_flowcontrol(new_config, SP_FLOWCONTROL_RTSCTS));
	}
	else
	{
		check(sp_set_config_flowcontrol(new_config, SP_FLOWCONTROL_NONE));
	}

	if (xonxoff & 1)
	{
		check(sp_set_config_flowcontrol(new_config, SP_FLOWCONTROL_XONXOFF));
	}

	if (!check(sp_set_config(sd->port, new_config)))
	{
		sp_free_config(initial_config);
		sp_free_config(new_config);
		return 5;
	}

	sp_free_config(initial_config);
	sp_free_config(new_config);
	return 0;
}

static int uaeser_trap_thread (void *arg)
{
	auto* sd = static_cast<struct uaeserialdata*>(arg);

	/* The set of events we will wait for. */
	struct sp_event_set* event_set;
	/* Allocate the event set. */
	check(sp_new_event_set(&event_set));

	uae_set_thread_priority(NULL, 1);
	sd->threadactive = 1;
	uae_sem_post(&sd->sync_sem);

	check(sp_add_port_events(event_set, sd->port, SP_EVENT_RX_READY));
	auto rx_ready = check(sp_wait(event_set, 1000)); //TODO set timeout?

	while (sd->threadactive == 1) {
		int sigmask = 0;
		uae_sem_wait(&sd->change_sem);
		if (rx_ready && check(sp_input_waiting(sd->port)))
			sigmask |= 1;
		else if (!sd->writeactive)
			sigmask |= 2;

		rx_ready = check(sp_wait(event_set, 1000)); // TODO set timeout?

		uaeser_signal (sd->user, sigmask);
		uae_sem_post(&sd->change_sem);
		uae_sem_wait (&sd->evtt);
	}
	/* Close ports and free resources. */
	sp_free_event_set(event_set);

	sd->threadactive = 0;
	uae_sem_post(&sd->sync_sem);
	return 0;
}

void uaeser_trigger (void *vsd)
{
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);
	uae_sem_post (&sd->evtt);
}

int uaeser_write (void *vsd, uae_u8 *data, uae_u32 len)
{
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
	if (!check(sp_nonblocking_write(sd->port, data, len)))
	{
		sd->writeactive = 1;
		return 1;
	}
	return 0;
}

int uaeser_read (void *vsd, uae_u8 *data, uae_u32 len)
{
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);

	if (!check(sp_nonblocking_read(sd->port, data, len)))
	{
		return 0;
	}
	return 1;
}

void uaeser_clearbuffers (void *vsd)
{
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);
	check(sp_flush(sd->port, SP_BUF_BOTH));
}

// TODO we probably can't open the same port if amiberry_serial has done so
// Check if we should share the same port between the two files
int uaeser_open (void *vsd, void *user, int unit)
{
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
	sd->user = user;

	check(sp_get_port_by_name(currprefs.sername, &sd->port));
	if (!check(sp_open(sd->port, SP_MODE_READ_WRITE)))
	{
		write_log("UAESER: '%s' failed to open\n", currprefs.sername);
		return 0;
	}

	check(sp_set_baudrate(sd->port, 9600));
	check(sp_set_bits(sd->port, 8));
	check(sp_set_parity(sd->port, SP_PARITY_NONE));
	check(sp_set_stopbits(sd->port, 8));
	check(sp_set_flowcontrol(sd->port, SP_FLOWCONTROL_NONE));

	uae_sem_init (&sd->sync_sem, 0, 0);
	uae_sem_init (&sd->change_sem, 0, 1);
	uae_sem_init (&sd->evtt, 0, 0);
	uae_start_thread (_T("uaeserial_amiberry"), uaeser_trap_thread, sd, NULL);
	uae_sem_wait (&sd->sync_sem);

	return 1;
}

void uaeser_close(void* vsd)
{
	auto* sd = static_cast<struct uaeserialdata*>(vsd);

	if (sd->threadactive) {
		sd->threadactive = -1;
		uae_sem_post(&sd->evtt);
		while (sd->threadactive)
			Sleep(10);
		uae_sem_destroy(&sd->evtt);
	}

	if (sd->port)
	{
		check(sp_close(sd->port));
		sp_free_port(sd->port);
	}

	uae_sem_destroy(&sd->sync_sem);
	uae_sem_destroy(&sd->change_sem);

	uaeser_initdata(sd, sd->user);
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
	return cnt;
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
	midi_out_ports.emplace_back(midi_emu_available(_T("MT-32")), "Munt MT-32", "Munt MT-32 (Missing ROMs)");
	total++;
	midi_out_ports.emplace_back(midi_emu_available(_T("CM-32L")), "Munt CM-32L", "Munt CM-32L (Missing ROMs)");
	total++;
#endif

	return total;
}