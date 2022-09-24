/*
 * UAE - The Un*x Amiga Emulator
 *
 * Not a parser, but parallel and serial emulation for Linux
 *
 * Copyright 2010 Mustafa TUFAN
 */

#include "sysconfig.h"

#undef SERIAL_ENET

#ifdef _WIN32
#include <Ws2tcpip.h>
#include <windows.h>
#include <winspool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <setupapi.h>
#include <Ntddpar.h>
#endif

#include "config.h"
#include "sysdeps.h"
#include "options.h"
#include "gensound.h"
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
#ifdef AHI
#include "ahi_v1.h"
#endif
#include "cia.h"
#include "savestate.h"
#include "xwin.h"
#include "drawing.h"

#define POSIX_SERIAL

#ifdef POSIX_SERIAL
#include <termios.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#ifndef __MACH__
#include <sys/epoll.h>
#include <linux/serial.h>
#endif
#endif

#if !defined B300 || !defined B1200 || !defined B2400 || !defined B4800 || !defined B9600
#undef POSIX_SERIAL
#endif
#if !defined B19200 || !defined B57600 || !defined B115200 || !defined B230400
#undef POSIX_SERIAL
#endif

#ifdef POSIX_SERIAL
static int brk_cond_count = 0;
static bool breakpending = false;
#endif

#define MIN_PRTBYTES 10

#define PARALLEL_MODE_NONE 0
#define PARALLEL_MODE_TCP_PRINTER 1

#if 0
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
	uae_log("TCP: Parallel listener socket closed\n");
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

static void freepsbuffers (void)
{
}

static int openprinter_ps (void)
{
	return 0;
}

static void *prt_thread (void *p)
{
}
#endif

static int doflushprinter (void)
{
	return 0;
}

#if 0
static void openprinter (void);

static void flushprtbuf (void)
{
}

void finishjob (void)
{
	flushprtbuf ();
}

static void DoSomeWeirdPrintingStuff (uae_char val)
{
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

int isprinteropen (void)
{
	return 0;
}

int load_ghostscript (void)
{
	STUB("");
}

void unload_ghostscript (void)
{
	STUB("");
}

static void openprinter (void)
{
	STUB("");
}
#endif

void flushprinter (void)
{
	if (!doflushprinter())
		return;
}

#if 0
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
#endif

struct uaeserialdata
{
	int fd;
	int writeactive;
	void *readdata, *writedata;
	int readatasize, writedatasize;
	volatile int threadactive;
	uae_sem_t change_sem, sync_sem, evtt;
	void *user;
#ifndef __MACH__
	struct serial_icounter_struct serial_counter;
#endif
	uae_sem_t write_start, write_done;
};

int uaeser_getdatalength (void)
{
	return sizeof (struct uaeserialdata);
}

static void uaeser_initdata (void *vsd, void *user)
{
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;
	memset(sd, 0, sizeof(struct uaeserialdata));
	sd->user = user;
}

int uaeser_query (void *vsd, uae_u16 *status, uae_u32 *pending)
{
#ifndef __MACH__
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;
	struct serial_icounter_struct new_serial_counter;
	uae_u16 s = 0;
	int stat = 0;
	int nread = 0;

	memset(&new_serial_counter, 0, sizeof(struct serial_icounter_struct));
	if (ioctl(sd->fd, TIOCGICOUNT, &new_serial_counter) > 0) {
		if (new_serial_counter.brk > sd->serial_counter.brk)
			s |= (1 << 10);
		if (new_serial_counter.buf_overrun != sd->serial_counter.brk)
			s |= (1 << 8);
	}
	else
		return 0;

	/* read control signals */
	if (ioctl(sd->fd, TIOCMGET, &stat) > 0) {
		if (!(stat & TIOCM_CTS))
			s |= (1 << 4);
		if (!(stat & TIOCM_DSR))
			s |= (1 << 7);
		if (stat & TIOCM_RI)
			s |= (1 << 2);
	}
	else
		return 0;

	*status = s;

	ioctl(sd->fd, FIONREAD, &nread);
	*pending = nread;
#endif
	return 1;
}

int uaeser_break (void *vsd, int brklen)
{
#ifndef __MACH__
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;

	if (ioctl(sd->fd, TIOCSBRK) < 0)
		return 0;

	usleep(brklen);
	ioctl(sd->fd, TIOCCBRK);
#endif
	return 1;
}

int uaeser_setparams (void *vsd, int baud, int rbuffer, int bits, int sbits, int rtscts, int parity, uae_u32 xonxoff)
{
#ifndef __MACH__
	struct termios newtio;
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;
	int pspeed = 0;
	int pbits = 0;

	switch (baud) {
	case 300: pspeed = B300; break;
	case 1200: pspeed = B1200; break;
	case 2400: pspeed = B2400; break;
	case 4800: pspeed = B4800; break;
	case 9600: pspeed = B9600; break;
	case 19200: pspeed = B19200; break;
	case 38400: pspeed = B38400; break;
	case 57600: pspeed = B57600; break;
	case 115200: pspeed = B115200; break;
	case 230400: pspeed = B230400; break;
	default:
		write_log("serial: unsupported baudrate %ld\n", baud);
		return 0;
	}

	switch (bits) {
	case 5: pbits = CS5; break;
	case 6: pbits = CS6; break;
	case 7: pbits = CS7; break;
	case 8: pbits = CS8; break;
	default:
		write_log("serial: unsupported bits %d\n", bits);
		return 0;
	}

	newtio.c_cflag = pspeed | pbits | CLOCAL | CREAD;

	switch (sbits) {
	case 1: break;
	case 2: newtio.c_cflag |= CSTOPB; break;
	default:
		write_log("serial: unsupported stop bits %d\n", sbits);
		return 0;
	}

	if (rtscts)
		newtio.c_cflag |= CRTSCTS;
	else
		newtio.c_cflag &= ~CRTSCTS;

	if (xonxoff & 1)
		newtio.c_iflag |= (IXON | IXOFF | IXANY);
	else
		newtio.c_iflag &= ~(IXON | IXOFF | IXANY);

	if (parity) {
		newtio.c_iflag &= ~IGNPAR;
		newtio.c_cflag = PARENB;
		if (parity == 1)
			newtio.c_cflag |= PARODD;
		else
			newtio.c_cflag &= ~PARODD;
	}
	else {
		newtio.c_iflag |= IGNPAR;
	}

	newtio.c_oflag = 0;

	/* Equivielnt of cfmakeraw() */
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;

	/* Set new settings */
	if (tcsetattr(sd->fd, TCSANOW, &newtio) < 0)
		return 1;

	/*
	 * TODO
	 * What to do with rbuffer?
	 */
#endif
	/* Return logic is reversed in this function! */
	return 0;
}

static int uaeser_trap_thread (void *arg)
{
#ifndef __MACH__
	struct uaeserialdata* sd = (struct uaeserialdata*)arg;
	//HANDLE handles[4];
	int cnt;
	//DWORD evtmask, actual;
	
	uae_set_thread_priority(NULL, 1);
	sd->threadactive = 1;
	uae_sem_post(&sd->sync_sem);
	//startwce(sd, &evtmask);
	while (sd->threadactive == 1) {
		int sigmask = 0;
		uae_sem_wait(&sd->change_sem);

		struct timeval tv;
		fd_set fd;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fd);
		FD_SET(sd->fd, &fd);
		int num_ready = select (FD_SETSIZE, &fd, NULL, NULL, &tv);
		if( num_ready > 0 )
				sigmask |= 1;

		/*if (WaitForSingleObject(sd->evtwce, 0) == WAIT_OBJECT_0) {
			if (evtmask & EV_RXCHAR)
				sigmask |= 1;
			if ((evtmask & EV_TXEMPTY) && !sd->writeactive)
				sigmask |= 2;
			if (evtmask & EV_BREAK)
				sigmask |= 4;
			startwce(sd, &evtmask);
		}
		cnt = 0;
		handles[cnt++] = sd->evtt;
		handles[cnt++] = sd->evtwce;
		if (sd->writeactive) {
			if (GetOverlappedResult (sd->hCom, &sd->olw, &actual, FALSE)) {
				sd->writeactive = 0;
				sigmask |= 2;
			} else {
				handles[cnt++] = sd->evtw;
			}
		}
		*/
		if (!sd->writeactive)
			sigmask |= 2;

		uaeser_signal (sd->user, sigmask);
		uae_sem_post(&sd->change_sem);
		//WaitForMultipleObjects(cnt, handles, FALSE, INFINITE);
		uae_sem_wait (&sd->evtt);
	}
	sd->threadactive = 0;
	uae_sem_post(&sd->sync_sem);
#endif
	return(0);
}

void uaeser_trigger (void *vsd)
{
#ifndef __MACH__
	struct uaeserialdata *sd = (struct uaeserialdata*)vsd;
	uae_sem_post (&sd->evtt);
#endif
}

int uaeser_write (void *vsd, uae_u8 *data, uae_u32 len)
{
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;
	int ret = 1;
#ifndef __MACH__

	if (sd->writeactive==0)
	{

		sd->writedata=data;
		sd->writedatasize=len;
		uae_sem_post (&sd->write_start);
		//uae_sem_wait (&sd->write_done);
	}

	//uae_sem_post (&sd->evtt);
#endif
	return ret;
}

int uaeser_read (void *vsd, uae_u8 *data, uae_u32 len)
{
#ifndef __MACH__

	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;

	struct timeval tv;
	fd_set fd;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fd);
	FD_SET(sd->fd, &fd);
	int num_ready = select (FD_SETSIZE, &fd, NULL, NULL, &tv);

	if (len>num_ready)
		return 0;

	if (read(sd->fd, data, len) != len)
		return 0;

	//uae_sem_post (&sd->evtt);
#endif
	return 1;
}

void uaeser_clearbuffers (void *vsd)
{
#ifndef __MACH__
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;
	tcflush(sd->fd, TCOFLUSH);
#endif
}

static int serial_read_thread(void* data)
{
#ifndef __MACH__ // disabled for MacOS for now, it doesn't support epoll
	struct uaeserialdata* sd = (struct uaeserialdata*)data;
	int efd, n, available_bytes, bytes_read;
	struct epoll_event event;
	struct epoll_event* events;
	char buffer[256];

	event.data.fd = sd->fd;
	event.events = EPOLLIN | EPOLLET;

	efd = epoll_create1(0);
	epoll_ctl(efd, EPOLL_CTL_ADD, sd->fd, &event);

	/* Buffer where events are returned */
	events = (struct epoll_event*)calloc(64, sizeof event);

	while (1)
	{
		n = epoll_wait(efd, events, 64, 5000);
		if (n > 0)
		{
			ioctl(sd->fd, FIONREAD, &available_bytes);
			if (available_bytes > 0)
			{
				uae_sem_post(&sd->evtt);
				printf("Data available: %d bytes\n", available_bytes);
			}
		}
	}

	free(events);
#endif
	return(0);
}

static int serial_write_thread(void* data)
{
#ifndef __MACH__
	struct uaeserialdata* sd = (struct uaeserialdata*)data;
	int total_written = 0, written = 0;

	while (1)
	{
		uae_sem_wait(&sd->write_start);

		sd->writeactive = 1;

		do {
			written = write(sd->fd, sd->writedata, sd->writedatasize);
			if (written < 0)
				break;

			total_written += written;
		} 		while (total_written < sd->writedatasize);

		tcdrain(sd->fd);

		sd->writeactive = 0;
		written = 0;
		total_written = 0;

		uae_sem_post(&sd->evtt);
	}
#endif
	return 0;
}

int uaeser_open (void *vsd, void *user, int unit)
{
#ifndef __MACH__
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;
	struct termios newtio;
	struct serial_icounter_struct serial_counter;
	char ser[128];

	snprintf (ser, 64, "/dev/ttyUSB%d", unit);

	sd->fd = open(ser, O_RDWR | O_NONBLOCK | O_BINARY, 0);

	newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;

	/* Disable HW flow control. */
	newtio.c_cflag &= ~CRTSCTS;

	/* Disable flow control. */
	newtio.c_iflag &= ~(IXON | IXOFF | IXANY);

	/* No parity. */
	newtio.c_iflag |= IGNPAR;
	newtio.c_oflag = 0;

	/* Equivielnt of cfmakeraw() */
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;

	/* Set new settings */
	tcsetattr(sd->fd, TCSANOW, &newtio);

	/* Initialise the io_status condition count */
	ioctl(sd->fd, TIOCGICOUNT, &serial_counter);

	if (sd->fd < 0)
		return 0;

	sd->user = user;

	uae_sem_init (&sd->sync_sem, 0, 0);
	uae_sem_init (&sd->change_sem, 0, 1);
	uae_sem_init (&sd->evtt, 0, 0);
	uae_start_thread (_T("uaeserial_amiberry"), uaeser_trap_thread, sd, NULL);
	uae_start_thread (_T("uaeserial_readthread"), serial_read_thread, sd, NULL);
	uae_start_thread (_T("uaeserial_writethread"), serial_write_thread, sd, NULL);
	uae_sem_wait (&sd->sync_sem);


	uae_sem_init (&sd->write_start, 0, 0);
	uae_sem_init (&sd->write_done, 0, 0);
#endif
	return 1;
}

void uaeser_close(void* vsd)
{
#ifndef __MACH__
	struct uaeserialdata* sd = (struct uaeserialdata*)vsd;

	if (sd->threadactive) {
		sd->threadactive = -1;
		uae_sem_post(&sd->evtt);
		while (sd->threadactive)
			sleep(10);
		uae_sem_destroy(&sd->evtt);
	}

	if (sd->fd >= 0)
		close(sd->fd);

	uae_sem_destroy(&sd->sync_sem);
	uae_sem_destroy(&sd->change_sem);

	uae_sem_destroy(&sd->write_start);
	uae_sem_destroy(&sd->write_done);

	uaeser_initdata(sd, sd->user);
#endif
}



#ifdef AMIBERRY
/* No MIDI support. */
#else
#define WITH_MIDI
#endif

#ifdef _WIN32
static HANDLE hCom = INVALID_HANDLE_VALUE;
static DCB dcb;
static HANDLE writeevent, readevent;
#else
static int ser_fd = -1;
#endif
#define SERIAL_WRITE_BUFFER 100
#define SERIAL_READ_BUFFER 100
static uae_u8 outputbuffer[SERIAL_WRITE_BUFFER];
static uae_u8 outputbufferout[SERIAL_WRITE_BUFFER];
static uae_u8 inputbuffer[SERIAL_READ_BUFFER];
static int datainoutput;
static int dataininput, dataininputcnt;
#ifdef _WIN32
static OVERLAPPED writeol, readol;
#endif
static int writepending;

#ifndef _WIN32
#define BOOL bool
#endif

#if SERIAL_ENET
static SOCKET serialsocket = UAE_SOCKET_INVALID;
static SOCKET serialconn = UAE_SOCKET_INVALID;
static BOOL tcpserial;

static bool tcp_is_connected (void)
{
	if (serialsocket == UAE_SOCKET_INVALID) {
		return false;
	}
	if (serialconn == UAE_SOCKET_INVALID) {
		if (uae_socket_select_read(serialsocket)) {
			serialconn = uae_socket_accept(serialsocket);
			if (serialconn != UAE_SOCKET_INVALID) {
				write_log(_T("TCP: Serial connection accepted\n"));
			}
		}
	}
	return serialconn != UAE_SOCKET_INVALID;
}

static void tcp_disconnect (void)
{
	if (serialconn == UAE_SOCKET_INVALID) {
		return;
	}
	uae_socket_close(serialconn);
	serialconn = UAE_SOCKET_INVALID;
	write_log(_T("TCP: Serial disconnect\n"));
}

static void closetcp (void)
{
	if (serialconn != UAE_SOCKET_INVALID) {
		uae_socket_close(serialconn);
		serialconn = UAE_SOCKET_INVALID;
	}
	if (serialsocket != UAE_SOCKET_INVALID) {
		uae_socket_close(serialsocket);
		serialsocket = UAE_SOCKET_INVALID;
	}
	// WSACleanup ();
}

static int opentcp (const TCHAR *sername)
{
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
}
#endif

int openser (const TCHAR *sername)
{
	struct termios newtio;
#ifndef __MACH__
	struct serial_icounter_struct serial_counter;
#else
	char fd,retcode,flags;
#endif

#if SERIAL_ENET
	if (_tcsnicmp(sername, _T("tcp:"), 4) == 0) {
		return opentcp(sername);
	}
#endif

#ifdef POSIX_SERIAL
	ser_fd = open (currprefs.sername, O_RDWR|O_NONBLOCK|O_BINARY, 0);

	newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;

	/* Disable HW flow control. */
	newtio.c_cflag &= ~CRTSCTS;

	/* Disable flow control. */
	newtio.c_iflag &= ~(IXON | IXOFF | IXANY);

	/* No parity. */
	newtio.c_iflag |= IGNPAR;
	newtio.c_oflag = 0;

	/* Equivielnt of cfmakeraw() */
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;

	/* Set new settings */
	tcsetattr (ser_fd, TCSANOW, &newtio);

	/* Initialise the break condition count */
// TIOCGICOUNT is not available on OS x
#ifndef __MACH__
	memset (&serial_counter, 0, sizeof(serial_counter));
	if (ioctl(ser_fd, TIOCGICOUNT, &serial_counter) >= 0)
		brk_cond_count = serial_counter.brk;
#endif
	write_log("serial: open '%s' -> fd=%d\n", sername, ser_fd);
	return (ser_fd >= 0);
#else
	return 0;
#endif
}

static bool valid_serial_handle(void)
{
#ifdef _WIN32
	return hCom != INVALID_HANDLE_VALUE;
#else
	return ser_fd > 0;

#endif
}

void closeser (void)
{
#if SERIAL_ENET
	if (tcpserial) {
		closetcp ();
		tcpserial = FALSE;
	}
#endif
#ifdef POSIX_SERIAL
	write_log("serial: close fd=%d\n", ser_fd);
	if (valid_serial_handle()) {
#ifdef _WIN32
		CloseHandle (hCom);
		hCom = INVALID_HANDLE_VALUE;
#else
		close(ser_fd);
		ser_fd = 0;
#endif
	}
#endif
}

#ifdef _WIN32
static void outser (void)
{
	DWORD actual;
	if (datainoutput > 0 && WaitForSingleObject (writeevent, 0) == WAIT_OBJECT_0 ) {
		memcpy (outputbufferout, outputbuffer, datainoutput);
		WriteFile (hCom, outputbufferout, datainoutput, &actual, &writeol);
		datainoutput = 0;
	}
}
#endif

void writeser_flush(void)
{

}

void writeser (int c)
{
#if SERIAL_ENET
	if (tcpserial) {
		if (tcp_is_connected ()) {
			char buf[1];
			buf[0] = (char)c;
			if (uae_socket_write(serialconn, buf, 1) != 1) {
				tcp_disconnect ();
			}
		}
	} else {
#endif
		if (!valid_serial_handle() || !currprefs.use_serial)
			return;
#ifdef _WIN32
		if (datainoutput + 1 < sizeof (outputbuffer)) {
			outputbuffer[datainoutput++] = c;
		} else {
			write_log (_T("serial output buffer overflow, data will be lost\n"));
			datainoutput = 0;
		}
		outser ();
#else
		char b = (char)c;
		if (write(ser_fd, &b, 1) != 1) {
			write_log("WARNING: writeser - 1 byte was not written (errno %d)\n",
				  errno);
		}
#endif
#if SERIAL_ENET
	}
#endif
}

int checkserwrite(int spaceneeded)
{
	if (!valid_serial_handle() || !currprefs.use_serial)
		return 1;
#ifdef WITH_MIDI
	if (midi_ready) {
		return 1;
	}
#endif
#ifdef _WIN32
	outser ();
	if (datainoutput + spaceneeded >= sizeof (outputbuffer)) {
		return 0;
	}
#else
	/* we assume that we can write always */
#endif
	return 1;
}

void flushser(void)
{
	while (readseravail(NULL) > 0) {
		int data;
		if (!readser(&data))
			break;
	}
}

int readseravail (bool *breakcond)
{
	if (breakcond)
		*breakcond = false;
#if SERIAL_ENET
	if (tcpserial) {
		if (tcp_is_connected ()) {
			int err = uae_socket_select_read(serialconn);
			if (err == UAE_SELECT_ERROR) {
				tcp_disconnect ();
				return 0;
			}
			if (err > 0)
				return 1;
		}
		return 0;
	} else {
#endif
		if (!currprefs.use_serial)
			return 0;
#ifdef _WIN32
		if (dataininput > dataininputcnt)
			return 1;
		if (hCom != INVALID_HANDLE_VALUE)  {
			COMSTAT ComStat;
			DWORD dwErrorFlags;
			ClearCommError (hCom, &dwErrorFlags, &ComStat);
			if (ComStat.cbInQue > 0)
				return 1;
		}
#else
		/* device is closed */
		if (ser_fd < 0) {
			return 0;
		}
// TIOCGICOUNT is not available on OS x
#ifndef __MACH__
		struct serial_icounter_struct serial_counter;
		memset (&serial_counter, 0, sizeof(serial_counter));
		if (ioctl (ser_fd, TIOCGICOUNT, &serial_counter) >= 0)
			if (breakcond && ((brk_cond_count < serial_counter.brk) || breakpending)) {
				brk_cond_count = serial_counter.brk;
				*breakcond = true;
				breakpending = false;
			}
#else
	breakpending = false;
#endif

		/* poll if read data is available */
		struct timeval tv;
		fd_set fd;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fd);
		FD_SET(ser_fd, &fd);
		int num_ready = select (FD_SETSIZE, &fd, NULL, NULL, &tv);
		return (num_ready == 1);
#endif
#if SERIAL_ENET
	}
#endif
	return 0;
}

int readser (int *buffer)
{
#if SERIAL_ENET
	if (tcpserial) {
		if (tcp_is_connected ()) {
			char buf[1];
			buf[0] = 0;
			int err = uae_socket_read(serialconn, buf, 1);
			if (err == 1) {
				*buffer = buf[0];
				//write_log(_T(" %02X "), buf[0]);
				return 1;
			} else {
				tcp_disconnect ();
			}
		}
		return 0;
	} else {
#endif
		if (!currprefs.use_serial)
			return 0;
#ifdef _WIN32
		if (dataininput > dataininputcnt) {
			*buffer = inputbuffer[dataininputcnt++];
			return 1;
		}
		dataininput = 0;
		dataininputcnt = 0;
		if (hCom != INVALID_HANDLE_VALUE)  {
			COMSTAT ComStat;
			DWORD dwErrorFlags;
			DWORD actual;

			/* only try to read number of bytes in queue */
			ClearCommError (hCom, &dwErrorFlags, &ComStat);
			if (ComStat.cbInQue)  {
				int len = ComStat.cbInQue;
				if (len > sizeof (inputbuffer))
					len = sizeof (inputbuffer);
				if (!ReadFile (hCom, inputbuffer, len, &actual, &readol))  {
					if (GetLastError() == ERROR_IO_PENDING)
						WaitForSingleObject (&readol, INFINITE);
					else
						return 0;
				}
				dataininput = actual;
				dataininputcnt = 0;
				if (actual == 0)
					return 0;
				return readser (buffer);
			}
		}
#else
		if (ser_fd < 0) {
			return 0;
		}

		char b;
// TIOCGICOUNT is not available on OS X
#ifndef __MACH__
		struct serial_icounter_struct serial_counter;
		memset (&serial_counter, 0, sizeof(serial_counter));
		if (ioctl (ser_fd, TIOCGICOUNT, &serial_counter) >= 0)
			if (brk_cond_count<serial_counter.brk) {
				brk_cond_count = serial_counter.brk;
				breakpending = true;
			}
#else
	breakpending=false;
#endif

		int num = read(ser_fd, &b, 1);
		if (num == 1) {
			*buffer = b;
			return 1;
		} else {
			return 0;
		}
#endif
#ifdef SERIAL_ENET
	}
#endif
	return 0;
}

void serialuartbreak (int v)
{
	if (!valid_serial_handle() || !currprefs.use_serial)
		return;

#ifdef _WIN32
	if (v)
		EscapeCommFunction (hCom, SETBREAK);
	else
		EscapeCommFunction (hCom, CLRBREAK);
#else
	if (v) {
		/* in posix serial calls we can't fulfill this function interface
		   completely: as we are not able to toggle the break mode with "v".
		   We simply trigger a default break here if v is enabled... */
		if (tcsendbreak(ser_fd, 0) < 0) {
			write_log("serial: TCSENDBREAK failed\n");
		}
	}
#endif
}

void getserstat (int *pstatus)
{
	if (!valid_serial_handle() || !currprefs.use_serial)
		return;

	int status = 0;
	*pstatus = 0;

#ifdef _WIN32
	DWORD stat;
	GetCommModemStatus (hCom, &stat);
	if (stat & MS_CTS_ON)
		status |= TIOCM_CTS;
	if (stat & MS_RLSD_ON)
		status |= TIOCM_CAR;
	if (stat & MS_DSR_ON)
		status |= TIOCM_DSR;
	if (stat & MS_RING_ON)
		status |= TIOCM_RI;
#else
	int stat;
	/* read control signals */
	if (ioctl (ser_fd, TIOCMGET, &stat) < 0) {
		write_log ("serial: ioctl TIOCMGET failed\n");
		*pstatus = TIOCM_CTS | TIOCM_CAR | TIOCM_DSR;
		return;
	}
	if (stat & TIOCM_CTS)
		status |= TIOCM_CTS;
	if (stat & TIOCM_CAR)
		status |= TIOCM_CAR;
	if (stat & TIOCM_DSR)
		status |= TIOCM_DSR;
	if (stat & TIOCM_RI)
		status |= TIOCM_RI;
#endif
	*pstatus = status;
}

void setserstat (int mask, int onoff)
{
	if (!valid_serial_handle() || !currprefs.use_serial)
		return;

#ifdef POSIX_SERIAL
	int status = 0;
	/* read control signals */
	if (ioctl(ser_fd, TIOCMGET, &status) < 0) {
		write_log ("serial: ioctl TIOCMGET failed\n");
		return;
	}
#endif

	if (mask & TIOCM_DTR) {
#ifdef _WIN32
		EscapeCommFunction (hCom, onoff ? SETDTR : CLRDTR);
#else
		if (onoff) {
			status |= TIOCM_DTR;
		} else {
			status &= ~TIOCM_DTR;
		}
#endif
	}
	if (!currprefs.serial_hwctsrts) {
		if (mask & TIOCM_RTS) {
#ifdef _WIN32
			EscapeCommFunction (hCom, onoff ? SETRTS : CLRRTS);
#else
			if (onoff) {
				status |= TIOCM_RTS;
			} else {
				status &= ~TIOCM_RTS;
			}
#endif
		}
	}

#ifdef POSIX_SERIAL
	/* write control signals */
	if (ioctl(ser_fd, TIOCMSET, &status) < 0) {
		write_log ("serial: ioctl TIOCMSET failed\n");
	}
#endif
}

int setbaud (long baud)
{
	if (!currprefs.use_serial) {
		return 1;
	}

#ifdef POSIX_SERIAL
	int pspeed;
	struct termios tios;

	/* device not open? */
	if (ser_fd < 0) {
		return 0;
	}

	/* map to terminal baud rate constant */
	write_log ("serial: setbaud: %ld\n", baud);
	switch (baud) {
	case 300: pspeed=B300; break;
	case 1200: pspeed=B1200; break;
	case 2400: pspeed=B2400; break;
	case 4800: pspeed=B4800; break;
	case 9600: pspeed=B9600; break;
	case 19200: pspeed=B19200; break;
	case 38400: pspeed=B38400; break;
	case 57600: pspeed=B57600; break;
	case 115200: pspeed=B115200; break;
	case 230400: pspeed=B230400; break;
	default:
		write_log ("serial: unsupported baudrate %ld\n", baud);
		return 0;
	}

	/* Only access hardware when we own it */
	if (tcgetattr (ser_fd, &tios) < 0) {
		write_log ("serial: TCGETATTR failed\n");
		return 0;
	}

	if (cfsetispeed (&tios, pspeed) < 0) { /* set serial input speed */
		write_log ("serial: CFSETISPEED (%ld bps) failed\n", baud);
		return 0;
	}
	if (cfsetospeed (&tios, pspeed) < 0) { /* set serial output speed */
		write_log ("serial: CFSETOSPEED (%ld bps) failed\n", baud);
		return 0;
	}

	if (tcsetattr (ser_fd, TCSADRAIN, &tios) < 0) {
		write_log ("serial: TCSETATTR failed\n");
		return 0;
	}
#endif
	return 1;
}

void initparallel (void)
{
#if SERIAL_ENET
	if (_tcsnicmp(currprefs.prtname, "tcp:", 4) == 0) {
		parallel_tcp_open(currprefs.prtname);
	} else {
#endif
#ifdef WITH_VPAR
		vpar_open();
#endif
#ifdef SERIAL_ENET
	}
#endif

#ifdef AHI
	if (uae_boot_rom_type) {
		uaecptr a = here (); //this installs the ahisound
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
	flashscreen = 10;
	init_colors (0);
	picasso_refresh (0);
	reset_drawing ();
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

#ifdef AMIBERRY
/* DISABLED FOR NOW */
#else
#ifdef PARALLEL_PORT
	keycheck++;
	if (keycheck >= 1000)
	{
		if (prtopen)
			flushprtbuf ();
		{
			if (flashscreen > 0) {
				flashscreen--;
				if (flashscreen == 0) {
					init_colors ();
					reset_drawing ();
					picasso_refresh ();
					flush_screen (gfxvidinfo.outbuffer, 0, 0);
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
#if 0
static int enumports_2 (struct serparportinfo **pi, int cnt, bool parport)
{
	STUB("");
	return cnt;
}

int enumserialports (void)
{
	STUB("");
	return 0;
}

int enummidiports (void)
{
	STUB("");
	return 0;
}

void sernametodev (TCHAR *sername)
{
	int i;

	for (i = 0; i < MAX_SERPAR_PORTS && comports[i]; i++) {
		if (!_tcscmp(sername, comports[i]->cfgname)) {
			_tcscpy(sername, comports[i]->dev);
			return;
		}
	}
	if (!_tcsncmp(sername, _T("TCP:"), 4))
		return;
	sername[0] = 0;
}

void serdevtoname (TCHAR *sername)
{
	int i;
	if (!_tcsncmp(sername, _T("TCP:"), 4))
		return;
	for (i = 0; i < MAX_SERPAR_PORTS && comports[i]; i++) {
		if (!_tcscmp(sername, comports[i]->dev)) {
			_tcscpy(sername, comports[i]->cfgname);
			return;
		}
	}
	sername[0] = 0;
}
#endif
