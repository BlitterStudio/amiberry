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

#include "sysdeps.h"
#include "options.h"
#include "threaddep/thread.h"
#include "serial.h"
#include "parser.h"

#include <libserialport.h>

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
