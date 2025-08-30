/*
* UAE - The Un*x Amiga Emulator
*
* uaeserial.device
*
* Copyright 2004/2006 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"
#include "options.h"
#include "memory.h"
#include "traps.h"
#include "autoconf.h"
#include "execlib.h"
#include "native2amiga.h"
#include "uaeserial.h"
#include "serial.h"
#include "execio.h"

#define MAX_TOTAL_DEVICES 8

int log_uaeserial = 0;

#define SDCMD_QUERY	9
#define SDCMD_BREAK	10
#define SDCMD_SETPARAMS	11

#define SerErr_DevBusy	       1
#define SerErr_BaudMismatch    2
#define SerErr_BufErr	       4
#define SerErr_InvParam        5
#define SerErr_LineErr	       6
#define SerErr_ParityErr       9
#define SerErr_TimerErr       11
#define SerErr_BufOverflow    12
#define SerErr_NoDSR	      13
#define SerErr_DetectedBreak  15

#define SERB_XDISABLED	7	/* io_SerFlags xOn-xOff feature disabled bit */
#define SERF_XDISABLED	(1<<7)	/*    "     xOn-xOff feature disabled mask */
#define	SERB_EOFMODE	6	/*    "     EOF mode enabled bit */
#define	SERF_EOFMODE	(1<<6)	/*    "     EOF mode enabled mask */
#define	SERB_SHARED	5	/*    "     non-exclusive access bit */
#define	SERF_SHARED	(1<<5)	/*    "     non-exclusive access mask */
#define SERB_RAD_BOOGIE 4	/*    "     high-speed mode active bit */
#define SERF_RAD_BOOGIE (1<<4)	/*    "     high-speed mode active mask */
#define	SERB_QUEUEDBRK	3	/*    "     queue this Break ioRqst */
#define	SERF_QUEUEDBRK	(1<<3)	/*    "     queue this Break ioRqst */
#define	SERB_7WIRE	2	/*    "     RS232 7-wire protocol */
#define	SERF_7WIRE	(1<<2)	/*    "     RS232 7-wire protocol */
#define	SERB_PARTY_ODD	1	/*    "     parity feature enabled bit */
#define	SERF_PARTY_ODD	(1<<1)	/*    "     parity feature enabled mask */
#define	SERB_PARTY_ON	0	/*    "     parity-enabled bit */
#define	SERF_PARTY_ON	(1<<0)	/*    "     parity-enabled mask */

#define	IO_STATB_XOFFREAD 12	   /* io_Status receive currently xOFF'ed bit */
#define	IO_STATF_XOFFREAD (1<<12)  /*	 "     receive currently xOFF'ed mask */
#define	IO_STATB_XOFFWRITE 11	   /*	 "     transmit currently xOFF'ed bit */
#define	IO_STATF_XOFFWRITE (1<<11) /*	 "     transmit currently xOFF'ed mask */
#define	IO_STATB_READBREAK 10	   /*	 "     break was latest input bit */
#define	IO_STATF_READBREAK (1<<10) /*	 "     break was latest input mask */
#define	IO_STATB_WROTEBREAK 9	   /*	 "     break was latest output bit */
#define	IO_STATF_WROTEBREAK (1<<9) /*	 "     break was latest output mask */
#define	IO_STATB_OVERRUN 8	   /*	 "     status word RBF overrun bit */
#define	IO_STATF_OVERRUN (1<<8)	   /*	 "     status word RBF overrun mask */

#define io_CtlChar	0x30    /* ULONG control char's (order = xON,xOFF,INQ,ACK) */
#define io_RBufLen	0x34	/* ULONG length in bytes of serial port's read buffer */
#define io_ExtFlags	0x38	/* ULONG additional serial flags (see bitdefs below) */
#define io_Baud		0x3c	/* ULONG baud rate requested (true baud) */
#define io_BrkTime	0x40	/* ULONG duration of break signal in MICROseconds */
#define io_TermArray0	0x44	/* ULONG termination character array */
#define io_TermArray1	0x48	/* ULONG termination character array */
#define io_ReadLen	0x4c	/* UBYTE bits per read character (# of bits) */
#define io_WriteLen	0x4d	/* UBYTE bits per write character (# of bits) */
#define io_StopBits	0x4e	/* UBYTE stopbits for read (# of bits) */
#define io_SerFlags	0x4f	/* UBYTE see SerFlags bit definitions below  */
#define io_Status	0x50	/* UWORD */

#define SEXTF_MSPON 2
#define SEXTF_MARK 1

#define IOExtSerSize 82

/* status of serial port, as follows:
*		   BIT	ACTIVE	FUNCTION
*		    0	 ---	reserved
*		    1	 ---	reserved
*		    2	 high	Connected to parallel "select" on the A1000.
*				Connected to both the parallel "select" and
*				serial "ring indicator" pins on the A500
*				& A2000.  Take care when making cables.
*		    3	 low	Data Set Ready
*		    4	 low	Clear To Send
*		    5	 low	Carrier Detect
*		    6	 low	Ready To Send
*		    7	 low	Data Terminal Ready
*		    8	 high	read overrun
*		    9	 high	break sent
*		   10	 high	break received
*		   11	 high	transmit x-OFFed
*		   12	 high	receive x-OFFed
*		13-15		reserved
*/


struct asyncreq {
	struct asyncreq *next;
	uaecptr arequest;
	uae_u8 *request;
	int ready;
};

struct devstruct {
	int open;
	int unit;
	int uniq;
	int exclusive;

	struct asyncreq *ar;

	smp_comm_pipe requests;
	int thread_running;
	uae_sem_t sync_sem;

	void *sysdata;
};

static int uniq;
static uae_u32 nscmd_cmd;
static struct devstruct devst[MAX_TOTAL_DEVICES];
static uae_sem_t change_sem, async_sem, pipe_sem;

static const TCHAR *getdevname (void)
{
	return _T("uaeserial.device");
}

static void io_log (const TCHAR *msg, uae_u8 *request, uaecptr arequest)
{
	if (log_uaeserial)
		write_log (_T("%s: %08X %d %08X %d %d io_actual=%d io_error=%d\n"),
		msg, request, get_word_host(request + 28), get_long_host(request + 40),
			get_long_host(request + 36), get_long_host(request + 44),
			get_long_host(request + 32), get_byte_host(request + 31));
}

static struct devstruct *getdevstruct (int uniq)
{
	int i;
	for (i = 0; i < MAX_TOTAL_DEVICES; i++) {
		if (devst[i].uniq == uniq)
			return &devst[i];
	}
	return 0;
}

static int dev_thread (void *devs);
static int start_thread (struct devstruct *dev)
{
	init_comm_pipe (&dev->requests, 100, 1);
	uae_sem_init (&dev->sync_sem, 0, 0);
	uae_start_thread (_T("uaeserial"), dev_thread, dev, NULL);
	uae_sem_wait (&dev->sync_sem);
	return dev->thread_running;
}

static void dev_close_3 (struct devstruct *dev)
{
	uaeser_close (dev->sysdata);
	dev->open = 0;
	xfree (dev->sysdata);
	uae_sem_wait(&pipe_sem);
	write_comm_pipe_pvoid(&dev->requests, NULL, 0);
	write_comm_pipe_pvoid(&dev->requests, NULL, 0);
	write_comm_pipe_u32 (&dev->requests, 0, 1);
	uae_sem_post(&pipe_sem);
}

static uae_u32 REGPARAM2 dev_close (TrapContext *ctx)
{
	uae_u32 request = trap_get_areg(ctx, 1);
	struct devstruct *dev;

	dev = getdevstruct (trap_get_long(ctx, request + 24));
	if (!dev)
		return 0;
	if (log_uaeserial)
		write_log (_T("%s:%d close, req=%x\n"), getdevname(), dev->unit, request);
	dev_close_3 (dev);
	trap_put_long(ctx, request + 24, 0);
	trap_put_word(ctx, trap_get_areg(ctx, 6) + 32, trap_get_word(ctx, trap_get_areg(ctx, 6) + 32) - 1);
	return 0;
}

static void resetparams(TrapContext *ctx, struct devstruct *dev, uae_u8 *req)
{
	put_long_host(req + io_CtlChar, 0x00001311);
	put_long_host(req + io_RBufLen, 1024);
	put_long_host(req + io_ExtFlags, 0);
	put_long_host(req + io_Baud, 9600);
	put_long_host(req + io_BrkTime, 250000);
	put_long_host(req + io_TermArray0, 0);
	put_long_host(req + io_TermArray1, 0);
	put_byte_host(req + io_ReadLen, 8);
	put_byte_host(req + io_WriteLen, 8);
	put_byte_host(req + io_StopBits, 1);
	put_byte_host(req + io_SerFlags, get_byte_host(req + io_SerFlags) & (SERF_XDISABLED | SERF_SHARED | SERF_7WIRE));
	put_word_host(req + io_Status, 0);
}

static int setparams(TrapContext *ctx, struct devstruct *dev, uae_u8 *req)
{
	uae_u32 extFlags, serFlags;
	int rbuffer, baud, rbits, wbits, sbits, rtscts, parity, xonxoff, err;

	rbuffer = get_long_host(req + io_RBufLen);
	extFlags = get_long_host(req + io_ExtFlags);
	baud = get_long_host(req + io_Baud);
	serFlags = get_byte_host(req + io_SerFlags);
	xonxoff = (serFlags & SERF_XDISABLED) ? 0 : 1;
	if (xonxoff) {
		xonxoff |= (get_long_host(req + io_CtlChar) << 8) & 0x00ffff00;
	}
	rtscts = (serFlags & SERF_7WIRE) ? 1 : 0;
	parity = 0;
	if (extFlags & SEXTF_MSPON) {
		parity = (extFlags & SEXTF_MARK) ? 3 : 4;
		if (!(serFlags & SERF_PARTY_ON)) {
			put_byte_host(req + io_SerFlags, serFlags | SERF_PARTY_ON);
		}
	} else if (serFlags & SERF_PARTY_ON) {
		parity = (serFlags & SERF_PARTY_ODD) ? 1 : 2;
	}
	rbits = get_byte_host(req + io_ReadLen);
	wbits = get_byte_host(req + io_WriteLen);
	sbits = get_byte_host(req + io_StopBits);
	if ((rbits != 7 && rbits != 8) || (wbits != 7 && wbits != 8) || (sbits != 1 && sbits != 2) || rbits != wbits) {
		write_log (_T("UAESER: Read=%d, Write=%d, Stop=%d, not supported\n"), rbits, wbits, sbits);
		return 5;
	}
	write_log (_T("%s:%d BAUD=%d BUF=%d BITS=%d+%d RTSCTS=%d PAR=%d XO=%06X\n"),
		getdevname(), dev->unit,
		baud, rbuffer, rbits, sbits, rtscts, parity, xonxoff);
	err = uaeser_setparams (dev->sysdata, baud, rbuffer,
		rbits, sbits, rtscts, parity, xonxoff);
	if (err) {
		write_log (_T("->failed %d\n"), err);
		return err;
	}
	return 0;
}

static int openfail(TrapContext *ctx, uaecptr ioreq, int error)
{
	trap_put_long(ctx, ioreq + 20, -1);
	trap_put_byte(ctx, ioreq + 31, error);
	return (uae_u32)-1;
}

static uae_u32 REGPARAM2 dev_open (TrapContext *ctx)
{
	uaecptr ioreq = trap_get_areg(ctx, 1);
	uae_u32 unit = trap_get_dreg(ctx, 0);
	uae_u32 flags = trap_get_dreg(ctx, 1);
	struct devstruct *dev;
	int i, err;
	uae_u8 request[IOExtSerSize];

	trap_get_bytes(ctx, request, ioreq, IOExtSerSize);

	if (trap_get_word(ctx, ioreq + 0x12) < IOSTDREQ_SIZE)
		return openfail(ctx, ioreq, IOERR_BADLENGTH);
	for (i = 0; i < MAX_TOTAL_DEVICES; i++) {
		if (devst[i].open && devst[i].unit == unit && devst[i].exclusive)
			return openfail(ctx, ioreq, IOERR_UNITBUSY);
	}
	for (i = 0; i < MAX_TOTAL_DEVICES; i++) {
		if (!devst[i].open)
			break;
	}
	if (i == MAX_TOTAL_DEVICES)
		return openfail(ctx, ioreq, IOERR_OPENFAIL);
	dev = &devst[i];
	dev->sysdata = xcalloc (uae_u8, uaeser_getdatalength ());
	if (!uaeser_open (dev->sysdata, dev, unit)) {
		xfree (dev->sysdata);
		return openfail(ctx, ioreq, IOERR_OPENFAIL);
	}
	dev->unit = unit;
	dev->open = 1;
	dev->uniq = ++uniq;
	dev->exclusive = (trap_get_word(ctx, ioreq + io_SerFlags) & SERF_SHARED) ? 0 : 1;
	put_long_host(request + 24, dev->uniq);
	resetparams (ctx, dev, request);
	err = setparams (ctx, dev, request);
	if (err) {
		uaeser_close (dev->sysdata);
		dev->open = 0;
		xfree (dev->sysdata);
		return openfail(ctx, ioreq, err);
	}
	if (log_uaeserial)
		write_log (_T("%s:%d open ioreq=%08X\n"), getdevname(), unit, ioreq);
	start_thread (dev);

	trap_put_word(ctx, trap_get_areg(ctx, 6) + 32, trap_get_word(ctx, trap_get_areg(ctx, 6) + 32) + 1);
	put_byte_host(request + 31, 0);
	put_byte_host(request + 8, 7);
	trap_put_bytes(ctx, request + 8, ioreq + 8, IOExtSerSize - 8);
	return 0;
}

static uae_u32 REGPARAM2 dev_expunge (TrapContext *context)
{
	return 0;
}

static struct asyncreq *get_async_request (struct devstruct *dev, uaecptr arequest, int ready)
{
	struct asyncreq *ar;

	uae_sem_wait (&async_sem);
	ar = dev->ar;
	while (ar) {
		if (ar->arequest == arequest) {
			if (ready)
				ar->ready = 1;
			break;
		}
		ar = ar->next;
	}
	uae_sem_post (&async_sem);
	return ar;
}

static int add_async_request (struct devstruct *dev, uae_u8 *request, uaecptr arequest)
{
	struct asyncreq *ar, *ar2;

	if (log_uaeserial)
		write_log (_T("%s:%d async request %x added\n"), getdevname(), dev->unit, arequest);

	uae_sem_wait (&async_sem);
	ar = xcalloc (struct asyncreq, 1);
	ar->arequest = arequest;
	ar->request = request;
	if (!dev->ar) {
		dev->ar = ar;
	} else {
		ar2 = dev->ar;
		while (ar2->next)
			ar2 = ar2->next;
		ar2->next = ar;
	}
	uae_sem_post (&async_sem);
	return 1;
}

static int release_async_request (struct devstruct *dev, uaecptr arequest)
{
	struct asyncreq *ar, *prevar;

	uae_sem_wait (&async_sem);
	ar = dev->ar;
	prevar = NULL;
	while (ar) {
		if (ar->arequest == arequest) {
			if (prevar == NULL)
				dev->ar = ar->next;
			else
				prevar->next = ar->next;
			uae_sem_post (&async_sem);
			xfree(ar->request);
			xfree(ar);
			if (log_uaeserial)
				write_log (_T("%s:%d async request %x removed\n"), getdevname(), dev->unit, arequest);
			return 1;
		}
		prevar = ar;
		ar = ar->next;
	}
	uae_sem_post (&async_sem);
	write_log (_T("%s:%d async request %x not found for removal!\n"), getdevname(), dev->unit, arequest);
	return 0;
}

static void abort_async(TrapContext *ctx, struct devstruct *dev, uaecptr arequest)
{
	struct asyncreq *ar = get_async_request (dev, arequest, 1);
	if (!ar) {
		write_log (_T("%s:%d: abort async but no request %x found!\n"), getdevname(), dev->unit, arequest);
		return;
	}
	uae_u8 *request = ar->request;
	if (log_uaeserial)
		write_log (_T("%s:%d asyncronous request=%08X aborted\n"), getdevname(), dev->unit, arequest);
	put_byte_host(request + 31, IOERR_ABORTED);
	put_byte_host(request + 30, get_byte_host(request + 30) | 0x20);
	uae_sem_wait(&pipe_sem);
	write_comm_pipe_pvoid(&dev->requests, ctx, 0);
	write_comm_pipe_pvoid(&dev->requests, request, 0);
	write_comm_pipe_u32(&dev->requests, arequest, 1);
	uae_sem_post(&pipe_sem);
}

static bool eofmatch(uae_u8 v, uae_u32 term0, uae_u32 term1)
{
	return v == (term0 >> 24) || v == (term0 >> 16) || v == (term0 >> 8) || v == (term0 >> 0)
		|| v == (term1 >> 24) || v == (term1 >> 16) || v == (term1 >> 8) || v == (term1 >> 0);
}

void uaeser_signal (void *vdev, int sigmask)
{
	TrapContext *ctx = NULL;
	struct devstruct *dev = (struct devstruct*)vdev;
	struct asyncreq *ar;

	uae_sem_wait (&async_sem);
	ar = dev->ar;
	while (ar) {
		if (!ar->ready) {
			uaecptr arequest = ar->arequest;
			uae_u8 *request = ar->request;
			uae_u32 io_data = get_long_host(request + 40); // 0x28
			uae_u32 io_length = get_long_host(request + 36); // 0x24
			uae_u8 serFlags = get_byte_host(request + io_SerFlags);
			int command = get_word_host(request + 28);
			uae_u32 io_error = 0, io_actual = 0;
			int io_done = 0;
			uae_u32 term0 = 0, term1 = 0;

			switch (command)
			{
			case SDCMD_BREAK:
				if (ar == dev->ar) {
					uaeser_break (dev->sysdata,  get_long_host(request + io_BrkTime));
					io_done = 1;
				}
				break;
			case CMD_READ:
				if (sigmask & 1) {
					uae_u8 tmp[RTAREA_TRAP_DATA_EXTRA_SIZE];
					io_done = 1;
					if (serFlags & SERF_EOFMODE) {
						term0 = get_long_host(request + io_TermArray0);
						term1 = get_long_host(request + io_TermArray1);
					}
					while (io_length) {
						int size = 1;
						if (!(serFlags & SERF_EOFMODE)) {
							size = io_length > sizeof(tmp) ? sizeof(tmp) : io_length;
						}
						int status = uaeser_read(dev->sysdata, tmp, size);
						if (status > 0) {
							trap_put_bytes(ctx, tmp, io_data, size);
							io_actual += size;
							io_data += size;
							io_length -= size;
							if ((serFlags & SERF_EOFMODE) && eofmatch(tmp[0], term0, term1)) {
								io_length = 0;
							}
						} else if (status == 0) {
							if (io_actual == 0)
								io_done = 0;
							break;
						}
					}
				} else if (sigmask & 4) {
					io_done = 1;
					io_error = SerErr_DetectedBreak;
				}
				break;
			case CMD_WRITE:
				if (sigmask & 2) {
					uae_u8 tmp[RTAREA_TRAP_DATA_EXTRA_SIZE];
					bool done = false;
					if (serFlags & SERF_EOFMODE) {
						term0 = get_long_host(request + io_TermArray0);
						term1 = get_long_host(request + io_TermArray1);
					}
					while (io_length && !done) {
						uae_u32 size = 0;
						if (io_length == 0xffffffff) {
							while (size < sizeof(tmp)) {
								tmp[size] = trap_get_byte(ctx, io_data + size);
								if (!tmp[size]) {
									done = true;
									break;
								}
								if ((serFlags & SERF_EOFMODE) && eofmatch(tmp[size], term0, term1)) {
									done = true;
									break;
								}
								size++;
							}
							if (size > 0) {
								if (!uaeser_write(dev->sysdata, tmp, size)) {
									done = true;
									break;
								}
							}
							io_actual += size;
							io_data += size;
						} else {
							size = io_length > sizeof(tmp) ? sizeof(tmp) : io_length;
							trap_get_bytes(ctx, tmp, io_data, size);
							if (serFlags & SERF_EOFMODE) {
								for (int i = 0; i < size; i++) {
									if (eofmatch(tmp[i], term0, term1)) {
										size = i;
										io_length = size;
										break;
									}
								}
							}
							if (!uaeser_write(dev->sysdata, tmp, size)) {
								done = true;
								break;
							}
							io_actual += size;
							io_data += size;
							io_length -= size;
						}
					}
					io_done = 1;
				}
				break;
			default:
				write_log (_T("%s:%d incorrect async request %x (cmd=%d) signaled?!"), getdevname(), dev->unit, request, command);
				break;
			}

			if (io_done) {
				if (log_uaeserial)
					write_log (_T("%s:%d async request %x completed\n"), getdevname(), dev->unit, request);
				put_long_host(request + 32, io_actual);
				put_byte_host(request + 31, io_error);
				ar->ready = 1;
				uae_sem_wait(&pipe_sem);
				write_comm_pipe_pvoid(&dev->requests, ctx, 0);
				write_comm_pipe_pvoid(&dev->requests, request, 0);
				write_comm_pipe_u32 (&dev->requests, arequest, 1);
				uae_sem_post(&pipe_sem);
				break;
			}

		}
		ar = ar->next;
	}
	uae_sem_post (&async_sem);
}

static void cmd_reset(TrapContext *ctx, struct devstruct *dev, uae_u8 *req)
{
	while (dev->ar) {
		abort_async(ctx, dev, dev->ar->arequest);
	}
	put_long_host(req + io_RBufLen, 8192);
	put_long_host(req + io_ExtFlags, 0);
	put_long_host(req + io_Baud, 57600);
	put_long_host(req + io_BrkTime, 250000);
	put_long_host(req + io_TermArray0, 0);
	put_long_host(req + io_TermArray1, 0);
	put_long_host(req + io_ReadLen, 8);
	put_long_host(req + io_WriteLen, 8);
	put_long_host(req + io_StopBits, 1);
	put_long_host(req + io_SerFlags, SERF_XDISABLED);
	put_word_host(req + io_Status, 0);
}

static int dev_do_io(TrapContext *ctx, struct devstruct *dev, uae_u8 *request, uaecptr arequest, int quick)
{
	uae_u32 command;
	uae_u32 io_data = get_long_host(request + 40); // 0x28
	uae_u32 io_length = get_long_host(request + 36); // 0x24
	uae_u32 io_actual = get_long_host(request + 32); // 0x20
	uae_u32 io_offset = get_long_host(request + 44); // 0x2c
	uae_u32 io_error = 0;
	uae_u16 io_status;
	int async = 0;

	if (!dev)
		return 0;
	command = get_word_host(request + 28);
	io_log (_T("dev_io_START"), request, arequest);

	switch (command)
	{
	case SDCMD_QUERY:
		if (uaeser_query (dev->sysdata, &io_status, &io_actual))
			put_byte_host(request + io_Status, (uae_u8)io_status);
		else
			io_error = IOERR_BADADDRESS;
		break;
	case SDCMD_SETPARAMS:
		io_error = setparams(ctx, dev, request);
		break;
	case CMD_WRITE:
		async = 1;
		break;
	case CMD_READ:
		async = 1;
		break;
	case SDCMD_BREAK:
		if (get_byte_host(request + io_SerFlags) & SERF_QUEUEDBRK) {
			async = 1;
		} else {
			uaeser_break(dev->sysdata,  get_long_host(request + io_BrkTime));
		}
		break;
	case CMD_CLEAR:
		uaeser_clearbuffers(dev->sysdata);
		break;
	case CMD_RESET:
		cmd_reset(ctx, dev, request);
		break;
	case CMD_FLUSH:
	case CMD_START:
	case CMD_STOP:
		break;
	case NSCMD_DEVICEQUERY:
		trap_put_long(ctx, io_data + 0, 0);
		trap_put_long(ctx, io_data + 4, 16); /* size */
		trap_put_word(ctx, io_data + 8, NSDEVTYPE_SERIAL);
		trap_put_word(ctx, io_data + 10, 0);
		trap_put_long(ctx, io_data + 12, nscmd_cmd);
		io_actual = 16;
		break;
	default:
		io_error = IOERR_NOCMD;
		break;
	}
	put_long_host(request + 32, io_actual);
	put_byte_host(request + 31, io_error);
	io_log (_T("dev_io_END"), request, arequest);
	return async;
}

static int dev_canquick (struct devstruct *dev, uae_u8 *request)
{
	return 0;
}

static uae_u32 REGPARAM2 dev_beginio (TrapContext *ctx)
{
	uae_u8 err = 0;
	uae_u32 arequest = trap_get_areg(ctx, 1);
	uae_u8 *request = xmalloc(uae_u8, IOExtSerSize);

	trap_get_bytes(ctx, request, arequest, IOExtSerSize);

	uae_u8 flags = get_byte_host(request + 30);
	int command = get_word_host(request + 28);
	struct devstruct *dev = getdevstruct (get_long_host(request + 24));

	put_byte_host(request + 8, NT_MESSAGE);
	if (!dev) {
		err = 32;
		goto end;
	}
	put_byte_host(request + 31, 0);
	if ((flags & 1) && dev_canquick(dev, request)) {
		if (dev_do_io(ctx, dev, request, arequest, 1))
			write_log (_T("device %s:%d command %d bug with IO_QUICK\n"), getdevname(), dev->unit, command);
		err = get_byte_host(request + 31);
	} else {
		put_byte_host(request + 30, get_byte_host(request + 30) & ~1);
		trap_put_bytes(ctx, request + 8, arequest + 8, IOExtSerSize - 8);
		uae_sem_wait(&pipe_sem);
		trap_set_background(ctx);
		write_comm_pipe_pvoid(&dev->requests, ctx, 0);
		write_comm_pipe_pvoid(&dev->requests, request, 0);
		write_comm_pipe_u32(&dev->requests, arequest, 1);
		uae_sem_post(&pipe_sem);
		return 0;
	}
end:
	put_byte_host(request + 31, 32);
	trap_put_bytes(ctx, request + 8, arequest + 8, IOExtSerSize - 8);
	xfree(request);
	return err;
}

static int dev_thread (void *devs)
{
	struct devstruct *dev = (struct devstruct*)devs;

	uae_set_thread_priority (NULL, 1);
	dev->thread_running = 1;
	uae_sem_post (&dev->sync_sem);
	for (;;) {
		TrapContext *ctx = (TrapContext*)read_comm_pipe_pvoid_blocking(&dev->requests);
		uae_u8 *iobuf = (uae_u8*)read_comm_pipe_pvoid_blocking(&dev->requests);
		uaecptr request = (uaecptr)read_comm_pipe_u32_blocking (&dev->requests);
		uae_sem_wait (&change_sem);
		if (!request) {
			dev->thread_running = 0;
			uae_sem_post (&dev->sync_sem);
			uae_sem_post (&change_sem);
			return 0;
		} else if (get_async_request (dev, request, 1)) {
			uae_ReplyMsg (request);
			release_async_request (dev, request);
		} else if (dev_do_io(ctx, dev, iobuf, request, 0) == 0) {
			uae_ReplyMsg (request);
		} else {
			add_async_request (dev, iobuf, request);
			uaeser_trigger (dev->sysdata);
		}
		trap_background_set_complete(ctx);
		uae_sem_post (&change_sem);
	}
}

static uae_u32 REGPARAM2 dev_init (TrapContext *context)
{
	uae_u32 base = trap_get_dreg (context, 0);
	if (log_uaeserial)
		write_log (_T("%s init\n"), getdevname ());
	return base;
}

static uae_u32 REGPARAM2 dev_abortio(TrapContext *ctx)
{
	uae_u32 request = trap_get_areg(ctx, 1);
	struct devstruct *dev = getdevstruct(trap_get_long(ctx, request + 24));

	if (!dev) {
		trap_put_byte(ctx, request + 31, 32);
		return trap_get_byte(ctx, request + 31);
	}
	abort_async(ctx, dev, request);
	return 0;
}

static void dev_reset (void)
{
	int i;
	struct devstruct *dev;

	for (i = 0; i < MAX_TOTAL_DEVICES; i++) {
		dev = &devst[i];
		if (dev->open) {
			while (dev->ar)
				abort_async(NULL, dev, dev->ar->arequest);
			dev_close_3 (dev);
			uae_sem_wait (&dev->sync_sem);
		}
		memset (dev, 0, sizeof (struct devstruct));
	}
}

static uaecptr ROM_uaeserialdev_resname = 0,
	ROM_uaeserialdev_resid = 0,
	ROM_uaeserialdev_init = 0;

uaecptr uaeserialdev_startup(TrapContext *ctx, uaecptr resaddr)
{
	if (!currprefs.uaeserial)
		return resaddr;
	if (log_uaeserial)
		write_log (_T("uaeserialdev_startup(0x%x)\n"), resaddr);
	/* Build a struct Resident. This will set up and initialize
	* the serial.device */
	trap_put_word(ctx, resaddr + 0x0, 0x4AFC);
	trap_put_long(ctx, resaddr + 0x2, resaddr);
	trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
	if (kickstart_version >= 37) {
		trap_put_long(ctx, resaddr + 0xA, 0x84010300 | AFTERDOS_PRI); /* RTF_AUTOINIT, RT_VERSION NT_LIBRARY, RT_PRI */
	} else {
		trap_put_long(ctx, resaddr + 0xA, 0x81010305); /* RTF_AUTOINIT, RT_VERSION NT_LIBRARY, RT_PRI */
	}
	trap_put_long(ctx, resaddr + 0xE, ROM_uaeserialdev_resname);
	trap_put_long(ctx, resaddr + 0x12, ROM_uaeserialdev_resid);
	trap_put_long(ctx, resaddr + 0x16, ROM_uaeserialdev_init);
	resaddr += 0x1A;
	return resaddr;
}


void uaeserialdev_install (void)
{
	uae_u32 functable, datatable;
	uae_u32 initcode, openfunc, closefunc, expungefunc;
	uae_u32 beginiofunc, abortiofunc;

	if (!currprefs.uaeserial)
		return;

	ROM_uaeserialdev_resname = ds (_T("uaeserial.device"));
	ROM_uaeserialdev_resid = ds (_T("UAE serial.device 0.3"));

	/* initcode */
	initcode = here ();
	calltrap (deftrap (dev_init)); dw (RTS);

	/* Open */
	openfunc = here ();
	calltrap (deftrap (dev_open)); dw (RTS);

	/* Close */
	closefunc = here ();
	calltrap (deftrap (dev_close)); dw (RTS);

	/* Expunge */
	expungefunc = here ();
	calltrap (deftrap (dev_expunge)); dw (RTS);

	/* BeginIO */
	beginiofunc = here ();
	calltrap (deftrap (dev_beginio)); dw (RTS);

	/* AbortIO */
	abortiofunc = here ();
	calltrap (deftrap (dev_abortio)); dw (RTS);

	/* FuncTable */
	functable = here ();
	dl (openfunc); /* Open */
	dl (closefunc); /* Close */
	dl (expungefunc); /* Expunge */
	dl (EXPANSION_nullfunc); /* Null */
	dl (beginiofunc); /* BeginIO */
	dl (abortiofunc); /* AbortIO */
	dl (0xFFFFFFFFul); /* end of table */

	/* DataTable */
	datatable = here ();
	dw (0xE000); /* INITBYTE */
	dw (0x0008); /* LN_TYPE */
	dw (0x0300); /* NT_DEVICE */
	dw (0xC000); /* INITLONG */
	dw (0x000A); /* LN_NAME */
	dl (ROM_uaeserialdev_resname);
	dw (0xE000); /* INITBYTE */
	dw (0x000E); /* LIB_FLAGS */
	dw (0x0600); /* LIBF_SUMUSED | LIBF_CHANGED */
	dw (0xD000); /* INITWORD */
	dw (0x0014); /* LIB_VERSION */
	dw (0x0004); /* 0.4 */
	dw (0xD000); /* INITWORD */
	dw (0x0016); /* LIB_REVISION */
	dw (0x0000);
	dw (0xC000); /* INITLONG */
	dw (0x0018); /* LIB_IDSTRING */
	dl (ROM_uaeserialdev_resid);
	dw (0x0000); /* end of table */

	ROM_uaeserialdev_init = here ();
	dl (0x00000100); /* size of device base */
	dl (functable);
	dl (datatable);
	dl (initcode);

	nscmd_cmd = here ();
	dw (NSCMD_DEVICEQUERY);
	dw (CMD_RESET);
	dw (CMD_READ);
	dw (CMD_WRITE);
	dw (CMD_CLEAR);
	dw (CMD_START);
	dw (CMD_STOP);
	dw (CMD_FLUSH);
	dw (SDCMD_BREAK);
	dw (SDCMD_SETPARAMS);
	dw (SDCMD_QUERY);
	dw (0);
}

void uaeserialdev_start_threads (void)
{
	uae_sem_init(&change_sem, 0, 1);
	uae_sem_init(&async_sem, 0, 1);
	uae_sem_init(&pipe_sem, 0, 1);
}

void uaeserialdev_reset (void)
{
	if (!currprefs.uaeserial)
		return;
	dev_reset ();
}
