/*
 * UAE - The Un*x Amiga Emulator
 *
 * Virtual parallel port protocol (vpar) for Amiga Emulators
 *
 * Written by Christian Vogelgsang <chris@vogelgsang.org>
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "vpar.h"
#include "cia.h"
#include "options.h"
#include "threaddep/thread.h"

// #define DEBUG_PAR

#ifdef WITH_VPAR

/* Virtual parallel port protocol aka "vpar"
 *
 * Always sends/receives 2 bytes: one control byte, one data byte
 *
 * Send to UAE the following control byte:
 *     0x01 0 = BUSY line value
 *     0x02 1 = POUT line value
 *     0x04 2 = SELECT line value
 *     0x08 3 = trigger ACK (and IRQ if enabled)
 *
 *     0x10 4 = set following data byte
 *     0x20 5 = set line value as given in bits 0..2
 *     0x40 6 = set line bits given in bits 0..2
 *     0x80 7 = clr line bits given in bits 0..2
 *
 * Receive from UAE the following control byte:
 *     0x01 0 = BUSY line set
 *     0x02 1 = POUT line set
 *     0x04 2 = SELECT line set
 *     0x08 3 = STROBE was triggered
 *     0x10 4 = is a reply to a read request (otherwise emu change)
 *     0x20 5 = n/a
 *     0x40 6 = emulator is starting (first msg of session)
 *     0x80 7 = emulator is shutting down (last msg of session)
 *
 * Note: sending a 00,00 pair returns the current state pair.
 */

#define VPAR_STROBE     0x08
#define VPAR_REPLY      0x10
#define VPAR_INIT       0x40
#define VPAR_EXIT       0x80

/* Virtual parallel stream state */
static uae_u8 pctl;
static uae_u8 pdat;
static uae_u8 last_pctl;
static uae_u8 last_pdat;

int par_fd = -1;
int par_mode = PAR_MODE_OFF;
static int vpar_debug = 0;
static int vpar_init_done = 0;
static uae_sem_t vpar_sem;

static char buf[80];
static char buf2[32];

static int ack_flag;
static uint64_t ts_req;

static const char *decode_ctl(const uae_u8 ctl, const char *txt)
{
	int busy = (ctl & 1) == 1;
	int pout = (ctl & 2) == 2;
	int select = (ctl & 4) == 4;

	char *ptr = buf;
	if (busy) {
		strcpy(ptr, "BUSY ");
	} else {
		strcpy(ptr, "busy ");
	}
	ptr+=5;
	if (pout) {
		strcpy(ptr, "POUT ");
	} else {
		strcpy(ptr, "pout ");
	}
	ptr+=5;
	if (select) {
		strcpy(ptr, "SELECT ");
	} else {
		strcpy(ptr, "select ");
	}
	ptr+=7;
	if (txt[0]) {
		int ack = (ctl & 8) == 8;
		if (ack) {
			strcpy(ptr, txt);
			ptr += strlen(txt);
		}
	}
	*ptr = '\0';
	return buf;
}

static const char *get_ts()
{
	struct timeval tv{};
	gettimeofday(&tv, nullptr);
	_sntprintf(buf2, sizeof buf2, "%8ld.%06ld",tv.tv_sec,tv.tv_usec);
	return buf2;
}

static uint64_t get_ts_uint64()
{
	struct timeval tv{};
	gettimeofday(&tv, nullptr);
	return tv.tv_sec * 1000000UL + tv.tv_usec;
}

static int vpar_low_write(uae_u8 data[2])
{
	int rem = 2;
	int off = 0;
	while (rem > 0) {
		int num = static_cast<int>(write(par_fd, data + off, rem));
		if (num < 0) {
			if (errno != EAGAIN) {
				if (vpar_debug) {
					printf("tx: ERROR(-1): %d rem %d\n", num, rem);
				}
				close(par_fd);
				par_fd = -1;
				return -1; /* failed */
			}
		}
		else if (num == 0) {
			if (vpar_debug) {
				printf("tx: ERROR(0): %d rem %d\n", num, rem);
			}
			close(par_fd);
			par_fd = -1;
			return -1; /* failed */
		}
		else {
			rem -= num;
			off += num;
		}
	}
	return 0; /* ok */
}

static int vpar_low_read(uae_u8 data[2])
{
	fd_set fds, fde;
	FD_ZERO(&fds);
	FD_SET(par_fd, &fds);
	FD_ZERO(&fde);
	FD_SET(par_fd, &fde);
	int num_ready = select (FD_SETSIZE, &fds, nullptr, &fde, nullptr);
	if (num_ready > 0) {
		if (FD_ISSET(par_fd, &fde)) {
			if (vpar_debug) {
				printf("rx: fd ERROR\n");
			}
			close(par_fd);
			par_fd = -1;
			return -1; /* failed */
		}
		if (FD_ISSET(par_fd, &fds)) {
			/* read 2 bytes command */
			int rem = 2;
			int off = 0;
			while (rem > 0) {
				int n = read(par_fd, data+off, rem);
				if (n<0) {
					if (errno != EAGAIN) {
						if (vpar_debug) {
							printf("rx: ERROR(-1): %d rem: %d\n", n, rem);
						}
						close(par_fd);
						par_fd = -1;
						return -1; /* failed */
					}
				}
				else if (n==0) {
					if (vpar_debug) {
						printf("rx: ERROR(0): %d rem: %d\n", n, rem);
					}
					close(par_fd);
					par_fd = -1;
					return -1; /* failed */
				}
				else {
					rem -= n;
					off += n;
				}
			}
			return 0; /* ok */
		}
	}
	return 1; /* delayed */
}

static void vpar_write_state(int force_flags)
{
	if (par_fd == -1) {
		return;
	}

	/* only write if value changed */
	if (force_flags || (last_pctl != pctl) || (last_pdat != pdat)) {
		uae_u8 data[2] = { pctl, pdat };
		if (force_flags) {
			data[0] |= force_flags;
		}

		/* try to write out value */
		int res = vpar_low_write(data);
		if (res == 0) {
			last_pctl = pctl;
			last_pdat = pdat;
			if (vpar_debug) {
				const char *what = force_flags ? "TX" : "tx";
				printf("%s %s: [%02x %02x] ctl=%02x  (%02x)  %s\n",
				       get_ts(), what, data[0], data[1], pctl, pdat,
				       decode_ctl(data[0],"strobe"));
			}
		}
	}
}

static int vpar_read_state(const uae_u8 data[2])
{
	int ack = 0;
	uae_u8 cmd = data[0];

	// is an update
	if (cmd != 0) {
		uae_u8 bits = cmd & 7;

		// update pdat value
		if (cmd & 0x10) {
			pdat = data[1];
		}

		// absolute set line bits
		if (cmd & 0x20) {
			pctl = bits;
		}
			// set line bits
		else if (cmd & 0x40) {
			pctl |= bits;
		}
			// clear line bits
		else if (cmd & 0x80) {
			pctl &= ~bits;
		}

		// set ack flag
		if (cmd & 8) {
			ack = 1;
			pctl |= 8;
		}

		if (vpar_debug) {
			printf("%s rx: [%02x %02x] ctl=%02x  (%02x)  %s\n",
			       get_ts(), data[0], data[1], pctl, pdat, decode_ctl(pctl,"ack"));
		}
	} else {
		if (vpar_debug) {
			printf("%s rx: [00 xx] ctl=%02x  (%02x)  %s\n",
			       get_ts(), pctl, pdat, decode_ctl(pctl,"ack"));
		}
	}

	// is ACK bit set?
	return ack;
}

static void vpar_init()
{
	/* write initial state with init flag set */
	vpar_write_state(VPAR_INIT);
}

static void vpar_exit()
{
	/* write final state with exit flag set */
	vpar_write_state(VPAR_EXIT);
}

void vpar_update()
{
	// report
	if (ack_flag) {
		if (vpar_debug) {
			uae_u64 delta = get_ts_uint64() - ts_req;
			printf("%s th: ACK done. delta=%llu\n",get_ts(), delta);
		}
		ack_flag = 0;
		cia_parallelack();
	}
}

static int vpar_thread(void *)
{
	if (vpar_debug) {
		printf("th: enter\n");
	}
	while (par_fd != -1) {
		/* block until we got data */
		uae_u8 data[2];
		int res = vpar_low_read(data);
		if (res == 0) {
			uae_sem_wait(&vpar_sem);
			int do_ack = vpar_read_state(data);
			/* ack value -> force write */
			vpar_write_state(VPAR_REPLY);
			uae_sem_post(&vpar_sem);
			if (do_ack) {
				if (vpar_debug) {
					ts_req = get_ts_uint64();
					printf("%s th: ACK req\n",get_ts());
				}
				ack_flag = 1;
				pctl &= ~8; // clear ack
			}
		}
	}
	if (vpar_debug) {
		printf("th: leave\n");
	}
	vpar_init_done = 0;
	return 0;
}

void vpar_open()
{
	/* is a printer file given? */
	char *name = strdup(currprefs.prtname);
	if (name[0]) {
		/* is a mode given with "mode:/file/path" ? */
		char *colptr = strchr(name,':');
		char *file_name = name;
		int oflag = 0;
		if (colptr) {
			*colptr = 0;
			/* raw mode: expect an existing socat stream */
			if (strcmp(name,"raw")==0) {
				par_mode = PAR_MODE_RAW;
			}
				/* printer mode: allow to create new file */
			else if (strcmp(name,"prt")==0) {
				par_mode = PAR_MODE_PRT;
				oflag = O_CREAT;
			}
				/* unknown mode */
			else {
				write_log("invalid parallel mode: '%s'\n", name);
				par_mode = PAR_MODE_PRT;
			}
			file_name = colptr+1;
		} else {
			par_mode = PAR_MODE_PRT;
		}
		/* enable debug output */
		vpar_debug = (getenv("VPAR_DEBUG") != nullptr);
		/* open parallel control file */
		if (par_fd == -1) {
			par_fd = open(file_name, O_RDWR | O_NONBLOCK | O_BINARY | oflag);
			write_log("parallel: open file='%s' mode=%d -> fd=%d\n", file_name, par_mode, par_fd);
		}
		/* start vpar reader thread */
		if (!vpar_init_done) {
			uae_sem_init(&vpar_sem, 0, 1);
			uae_start_thread (_T("parser_ack"), vpar_thread, nullptr, nullptr);
			vpar_init_done = 1;
		}
		/* init vpar */
		if (par_fd >= 0) {
			if (vpar_debug) {
				puts("*** vpar init");
			}
			vpar_init();
		}
	} else {
		par_mode = PAR_MODE_OFF;
	}
	free(name);
}

void vpar_close()
{
	/* close parallel control file */
	if (par_fd >= 0) {
		/* exit vpar */
		if (vpar_debug) {
			puts("*** vpar exit");
		}
		vpar_exit();

		write_log("parallel: close fd=%d\n", par_fd);
		close(par_fd);
		par_fd = -1;
	}
}

int vpar_direct_write_status(uae_u8 v, uae_u8 dir)
{
	uae_u8 pdir = dir & 7;

#ifdef DEBUG_PAR
	uae_u8 val = v & pdir;
    printf("%s wr: ctl=%02x dir=%02x %s\n", get_ts(), val, pdir, decode_ctl(val ,NULL));
#endif

	// update pctl
	uae_sem_wait(&vpar_sem);
	pctl = (v & pdir) | (pctl & ~pdir);
	vpar_write_state(0);
	uae_sem_post(&vpar_sem);
	return 0;
}

int vpar_direct_read_status(uae_u8 *vp)
{
	uae_sem_wait(&vpar_sem);
	*vp = pctl;
	uae_sem_post(&vpar_sem);

#ifdef DEBUG_PAR
	static uae_u8 last_v = 0;
    if (*vp != last_v) {
        last_v = *vp;
        printf("%s RD: ctl=%02x\n", get_ts(), last_v);
    }
#endif
	return 0;
}

int vpar_direct_write_data(uae_u8 v, uae_u8 dir)
{
#ifdef DEBUG_PAR
	uae_u8 val = v & dir;
    printf("%s wr: dat=%02x dir=%02x\n", get_ts(), val, dir);
#endif

	uae_sem_wait(&vpar_sem);
	pdat = (v & dir) | (pdat & ~dir);
	vpar_write_state(VPAR_STROBE); /* write with strobe (0x08) */
	uae_sem_post(&vpar_sem);
	return 0;
}

int vpar_direct_read_data(uae_u8 *v)
{
	uae_sem_wait(&vpar_sem);
	*v = pdat;
	uae_sem_post(&vpar_sem);

#ifdef DEBUG_PAR
	static uae_u8 last_v = 0;
    if (*v != last_v) {
        last_v = *v;
        printf("%s RD: dat=%02x\n", get_ts(), last_v);
    }
#endif
	return 0;
}

#endif /* WITH_VPAR */
