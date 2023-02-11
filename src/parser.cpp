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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#ifndef __MACH__
#include <sys/epoll.h>
#include <linux/serial.h>
#endif
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
	serial_icounter_struct serial_counter;
#endif
	uae_sem_t write_start, write_done;
};

int uaeser_getdatalength (void)
{
	return sizeof (uaeserialdata);
}

static void uaeser_initdata (void *vsd, void *user)
{
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
	memset(sd, 0, sizeof(uaeserialdata));
	sd->user = user;
}

int uaeser_query (void *vsd, uae_u16 *status, uae_u32 *pending)
{
#ifndef __MACH__
	const auto sd = static_cast<struct uaeserialdata*>(vsd);
	serial_icounter_struct new_serial_counter{};
	uae_u16 s = 0;
	int stat = 0;
	int nread = 0;

	memset(&new_serial_counter, 0, sizeof(serial_icounter_struct));
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
	const auto* sd = static_cast<struct uaeserialdata*>(vsd);

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
	termios newtio{};
	auto sd = static_cast<struct uaeserialdata*>(vsd);
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
	auto* sd = static_cast<struct uaeserialdata*>(arg);
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

		timeval tv;
		fd_set fd;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fd);
		FD_SET(sd->fd, &fd);
		int num_ready = select (FD_SETSIZE, &fd, NULL, NULL, &tv);
		if( num_ready > 0 )
				sigmask |= 1;

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
	auto*sd = static_cast<struct uaeserialdata*>(vsd);
	uae_sem_post (&sd->evtt);
#endif
}

int uaeser_write (void *vsd, uae_u8 *data, uae_u32 len)
{
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
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
	auto* sd = static_cast<struct uaeserialdata*>(vsd);

	timeval tv{};
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
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
	tcflush(sd->fd, TCOFLUSH);
#endif
}

static int serial_read_thread(void* data)
{
#ifndef __MACH__ // disabled for MacOS for now, it doesn't support epoll
	auto* sd = static_cast<struct uaeserialdata*>(data);
	int efd, n, available_bytes, bytes_read;
	epoll_event event{};
	char buffer[256];

	event.data.fd = sd->fd;
	event.events = EPOLLIN | EPOLLET;

	efd = epoll_create1(0);
	epoll_ctl(efd, EPOLL_CTL_ADD, sd->fd, &event);

	/* Buffer where events are returned */
	auto* events = static_cast<epoll_event*>(calloc(64, sizeof event));

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
	auto* sd = static_cast<struct uaeserialdata*>(data);
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
	auto* sd = static_cast<struct uaeserialdata*>(vsd);
	termios newtio{};
	serial_icounter_struct serial_counter{};
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
	auto* sd = static_cast<struct uaeserialdata*>(vsd);

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
