 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Hardfile emulation
  *
  * Copyright 1995 Bernd Schmidt
  *           2002 Toni Wilen (scsi emulation, 64-bit support)
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "td-sdl/thread.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "disk.h"
#include "autoconf.h"
#include "traps.h"
#include "filesys.h"
#include "execlib.h"
#include "native2amiga.h"
#include "gui.h"
#include "uae.h"
#include "execio.h"

#define MAX_ASYNC_REQUESTS 50
#define ASYNC_REQUEST_NONE 0
#define ASYNC_REQUEST_TEMP 1
#define ASYNC_REQUEST_CHANGEINT 10

struct hardfileprivdata {
    volatile uaecptr d_request[MAX_ASYNC_REQUESTS];
    volatile int d_request_type[MAX_ASYNC_REQUESTS];
    volatile uae_u32 d_request_data[MAX_ASYNC_REQUESTS];
    smp_comm_pipe requests;
    int thread_running;
    uae_sem_t sync_sem;
    uaecptr base;
    int changenum;
};

static uae_sem_t change_sem = 0;

static struct hardfileprivdata hardfpd[MAX_FILESYSTEM_UNITS];

static uae_u32 nscmd_cmd;

static uae_u64 cmd_readx (struct hardfiledata *hfd, uae_u8 *dataptr, uae_u64 offset, uae_u64 len)
{
    gui_hd_led (1);
    write_log ("cmd_read: %p %04x-%08x (%d) %08x (%d)\n", 
  dataptr, (uae_u32)(offset >> 32), (uae_u32)offset, (uae_u32)(offset / hfd->blocksize), (uae_u32)len, (uae_u32)(len / hfd->blocksize));
    fseek (hfd->fd, offset, SEEK_SET);
    return fread (dataptr, 1, len, hfd->fd);
}
static uae_u64 cmd_read (struct hardfiledata *hfd, uaecptr dataptr, uae_u64 offset, uae_u64 len)
{
    addrbank *bank_data = &get_mem_bank (dataptr);
    if (!bank_data || !bank_data->check (dataptr, len))
	return 0;
    return cmd_readx (hfd, bank_data->xlateaddr (dataptr), offset, len);
}
static uae_u64 cmd_writex (struct hardfiledata *hfd, uae_u8 *dataptr, uae_u64 offset, uae_u64 len)
{
    gui_hd_led (2);
    write_log ("cmd_write: %p %04x-%08x %08x\n", dataptr, (uae_u32)(offset >> 32), (uae_u32)offset, (uae_u32)(offset / hfd->blocksize), (uae_u32)len, (uae_u32)(len / hfd->blocksize));
    fseek (hfd->fd, offset, SEEK_SET);
    return fwrite (dataptr, 1, len, hfd->fd);
}

static uae_u64 cmd_write (struct hardfiledata *hfd, uaecptr dataptr, uae_u64 offset, uae_u64 len)
{
    addrbank *bank_data = &get_mem_bank (dataptr);
    if (!bank_data || !bank_data->check (dataptr, len))
	return 0;
    return cmd_writex (hfd, bank_data->xlateaddr (dataptr), offset, len);
}

static int nodisk (struct hardfiledata *hfd)
{
    if (hfd->drive_empty)
	return 1;
    return 0;
}

void hardfile_do_disk_change (struct uaedev_config_info *uci, int insert)
{
    int fsid = uci->configoffset;
    int j;
    int newstate = insert ? 0 : 1;
    struct hardfiledata *hfd;

    hfd = get_hardfile_data (fsid);
    if (!hfd)
	return;
    uae_sem_wait (&change_sem);
    hardfpd[fsid].changenum++;
    write_log("uaehf.device:%d media status=%d\n", fsid, insert);
    hfd->drive_empty = newstate;
    j = 0;
    while (j < MAX_ASYNC_REQUESTS) {
	if (hardfpd[fsid].d_request_type[j] == ASYNC_REQUEST_CHANGEINT) {
	    uae_Cause (hardfpd[fsid].d_request_data[j]);
	}
	j++;
    }
    uae_sem_post (&change_sem);
}

static int add_async_request (struct hardfileprivdata *hfpd, uaecptr request, int type, uae_u32 data)
{
    int i;

    i = 0;
    while (i < MAX_ASYNC_REQUESTS) {
	if (hfpd->d_request[i] == request) {
	    hfpd->d_request_type[i] = type;
	    hfpd->d_request_data[i] = data;
	    write_log ("old async request %p (%d) added\n", request, type);
	    return 0;
	}
	i++;
    }
    i = 0;
    while (i < MAX_ASYNC_REQUESTS) {
	if (hfpd->d_request[i] == 0) {
	    hfpd->d_request[i] = request;
	    hfpd->d_request_type[i] = type;
	    hfpd->d_request_data[i] = data;
	    write_log ("async request %p (%d) added (total=%d)\n", request, type, i);
	    return 0;
	}
	i++;
    }
    write_log ("async request overflow %p!\n", request);
    return -1;
}

static int release_async_request (struct hardfileprivdata *hfpd, uaecptr request)
{
    int i = 0;

    while (i < MAX_ASYNC_REQUESTS) {
	if (hfpd->d_request[i] == request) {
	    int type = hfpd->d_request_type[i];
	    hfpd->d_request[i] = 0;
	    hfpd->d_request_data[i] = 0;
	    hfpd->d_request_type[i] = 0;
	    write_log ("async request %p removed\n", request);
	    return type;
	}
	i++;
    }
    write_log ("tried to remove non-existing request %p\n", request);
    return -1;
}

static void abort_async (struct hardfileprivdata *hfpd, uaecptr request, int errcode, int type)
{
    int i;
    write_log ("aborting async request %p\n", request);
    i = 0;
    while (i < MAX_ASYNC_REQUESTS) {
	if (hfpd->d_request[i] == request && hfpd->d_request_type[i] == ASYNC_REQUEST_TEMP) {
	    /* ASYNC_REQUEST_TEMP = request is processing */
	    sleep_millis (1);
	    i = 0;
	    continue;
	}
	i++;
    }
    i = release_async_request (hfpd, request);
    if (i >= 0)
	write_log ("asyncronous request=%08X aborted, error=%d\n", request, errcode);
}

static void *hardfile_thread (void *devs);
static int start_thread (TrapContext *context, int unit)
{
    struct hardfileprivdata *hfpd = &hardfpd[unit];

    if (hfpd->thread_running)
        return 1;
    memset (hfpd, 0, sizeof (struct hardfileprivdata));
    hfpd->base = m68k_areg(&context->regs, 6);
    init_comm_pipe (&hfpd->requests, 100, 1);
    uae_sem_init (&hfpd->sync_sem, 0, 0);
    uae_start_thread ("hardfile", hardfile_thread, hfpd, NULL);
    uae_sem_wait (&hfpd->sync_sem);
    return hfpd->thread_running;
}

static int mangleunit (int unit)
{
    if (unit <= 99)
	return unit;
    if (unit == 100)
	return 8;
    if (unit == 110)
	return 9;
    return -1;
}

static uae_u32 REGPARAM2 hardfile_open (TrapContext *context)
{
  uaecptr ioreq = m68k_areg(&context->regs, 1); /* IOReq */
  int unit = mangleunit(m68k_dreg (&context->regs, 0));
  struct hardfileprivdata *hfpd = &hardfpd[unit];
  int err = IOERR_OPENFAIL;
  int size = get_word (ioreq + 0x12);

  /* boot device port size == 0!? KS 1.x size = 12??? */
  if (size >= IOSTDREQ_SIZE || size == 0 || kickstart_version == 0xffff || kickstart_version < 39) {
    /* Check unit number */
    if (unit >= 0) {
    	struct hardfiledata *hfd = get_hardfile_data (unit);
      if (hfd && hfd->fd != 0 && start_thread (context, unit)) {
  	    put_word (hfpd->base + 32, get_word (hfpd->base + 32) + 1);
  	    put_long (ioreq + 24, unit); /* io_Unit */
  	    put_byte (ioreq + 31, 0); /* io_Error */
  	    put_byte (ioreq + 8, 7); /* ln_type = NT_REPLYMSG */
        write_log ("hardfile_open, unit %d (%d), OK\n", unit, m68k_dreg (&context->regs, 0));
      	return 0;
      }
    }
    if (unit < 1000 || is_hardfile(unit) == FILESYS_VIRTUAL)
    	err = 50; /* HFERR_NoBoard */
  } else {
  	err = IOERR_BADLENGTH;
  }
  write_log ("hardfile_open, unit %d (%d), ERR=%d\n", unit, m68k_dreg (&context->regs, 0), err);
  put_long (ioreq + 20, (uae_u32)err);
  put_byte (ioreq + 31, (uae_u8)err);
  return (uae_u32)err;
}

static uae_u32 REGPARAM2 hardfile_close (TrapContext *context)
{
    uaecptr request = m68k_areg(&context->regs, 1); /* IOReq */
    int unit = mangleunit (get_long (request + 24));
    struct hardfileprivdata *hfpd = &hardfpd[unit];

    if (!hfpd) 
      return 0;
    put_word (hfpd->base + 32, get_word (hfpd->base + 32) - 1);
    if (get_word(hfpd->base + 32) == 0)
	    write_comm_pipe_u32 (&hfpd->requests, 0, 1);
    return 0;
}

static uae_u32 REGPARAM2 hardfile_expunge (TrapContext *context)
{
    return 0; /* Simply ignore this one... */
}

static void outofbounds (int cmd, uae_u64 offset, uae_u64 len, uae_u64 max)
{
    write_log ("UAEHF: cmd %d: out of bounds, %08X-%08X + %08X-%08X > %08X-%08X\n", cmd,
	(uae_u32)(offset >> 32),(uae_u32)offset,(uae_u32)(len >> 32),(uae_u32)len,
	(uae_u32)(max >> 32),(uae_u32)max);
}
static void unaligned (int cmd, uae_u64 offset, uae_u64 len, int blocksize)
{
    write_log ("UAEHF: cmd %d: unaligned access, %08X-%08X, %08X-%08X, %08X\n", cmd,
	(uae_u32)(offset >> 32),(uae_u32)offset,(uae_u32)(len >> 32),(uae_u32)len,
	blocksize);
}

static uae_u32 hardfile_do_io (struct hardfiledata *hfd, struct hardfileprivdata *hfpd, uaecptr request)
{
    uae_u32 dataptr, offset, actual = 0, cmd;
    uae_u64 offset64;
    int unit = get_long (request + 24);
    uae_u32 error = 0, len;
    int async = 0;
    int bmask = hfd->blocksize - 1;

    cmd = get_word (request + 28); /* io_Command */
    dataptr = get_long (request + 40);
    switch (cmd)
    {
	case CMD_READ:
	if (nodisk (hfd))
	    goto no_disk;
	offset = get_long (request + 44);
	len = get_long (request + 36); /* io_Length */
	if ((offset & bmask) || dataptr == 0) {
	    unaligned (cmd, offset, len, hfd->blocksize);
	    goto bad_command;
  }
	if (len & bmask) {
	    unaligned (cmd, offset, len, hfd->blocksize);
	    goto bad_len;
	}
	if (len + offset > hfd->size) {
	    outofbounds (cmd, offset, len, hfd->size);
	    goto bad_len;
  }
	actual = (uae_u32)cmd_read (hfd, dataptr, offset, len);
	break;

	case TD_READ64:
	case NSCMD_TD_READ64:
	if (nodisk (hfd))
	    goto no_disk;
	offset64 = get_long (request + 44) | ((uae_u64)get_long (request + 32) << 32);
	len = get_long (request + 36); /* io_Length */
	if ((offset64 & bmask) || dataptr == 0) {
	    unaligned (cmd, offset64, len, hfd->blocksize);
	    goto bad_command;
	}
	if (len & bmask) {
	    unaligned (cmd, offset64, len, hfd->blocksize);
	    goto bad_len;
  }
	if (len + offset64 > hfd->size) {
	    outofbounds (cmd, offset64, len, hfd->size);
    	    goto bad_len;
  }
	actual = (uae_u32)cmd_read (hfd, dataptr, offset64, len);
	break;

	case CMD_WRITE:
	case CMD_FORMAT: /* Format */
	if (nodisk (hfd))
	    goto no_disk;
	if (hfd->readonly) {
	    error = 28; /* write protect */
	} else {
	    offset = get_long (request + 44);
	    len = get_long (request + 36); /* io_Length */
	    if ((offset & bmask) || dataptr == 0) {
	      unaligned (cmd, offset, len, hfd->blocksize);
		    goto bad_command;
	    }
	    if (len & bmask) {
		unaligned (cmd, offset, len, hfd->blocksize);
		goto bad_len;
      }
	    if (len + offset > hfd->size) {
	      outofbounds (cmd, offset, len, hfd->size);
		goto bad_len;
      }
	    actual = (uae_u32)cmd_write (hfd, dataptr, offset, len);
	}
	break;

	case TD_WRITE64:
	case TD_FORMAT64:
	case NSCMD_TD_WRITE64:
	case NSCMD_TD_FORMAT64:
	if (nodisk (hfd))
	    goto no_disk;
	if (hfd->readonly) {
	    error = 28; /* write protect */
	} else {
	    offset64 = get_long (request + 44) | ((uae_u64)get_long (request + 32) << 32);
	    len = get_long (request + 36); /* io_Length */
	    if ((offset64 & bmask) || dataptr == 0) {
	      unaligned (cmd, offset64, len, hfd->blocksize);
		    goto bad_command;
	    }
	    if (len & bmask) {
		unaligned (cmd, offset64, len, hfd->blocksize);
		goto bad_len;
      }
	    if (len + offset64 > hfd->size) {
	      outofbounds (cmd, offset64, len, hfd->size);
		goto bad_len;
      }
	    put_long (request + 32, (uae_u32)cmd_write (hfd, dataptr, offset64, len));
	}
	break;

	bad_command:
	error = IOERR_BADADDRESS;
	break;
	bad_len:
	error = IOERR_BADLENGTH;
	no_disk:
	error = 29; /* no disk */
	break;

	case NSCMD_DEVICEQUERY:
	    put_long (dataptr + 0, 0);
	    put_long (dataptr + 4, 16); /* size */
	    put_word (dataptr + 8, NSDEVTYPE_TRACKDISK);
	    put_word (dataptr + 10, 0);
	    put_long (dataptr + 12, nscmd_cmd);
	    actual = 16;
	break;

	case CMD_GETDRIVETYPE:
	    actual = DRIVE_NEWSTYLE;
	    break;

	case CMD_GETNUMTRACKS:
	    write_log ("CMD_GETNUMTRACKS - shouldn't happen\n");
	    actual = 0;
	    break;

	case CMD_PROTSTATUS:
	    if (hfd->readonly)
		actual = -1;
	    else
		actual = 0;
	break;

	case CMD_CHANGESTATE:
	    actual = hfd->drive_empty ? 1 :0;
	break;

	/* Some commands that just do nothing and return zero */
	case CMD_UPDATE:
	case CMD_CLEAR:
	case CMD_MOTOR:
	case CMD_SEEK:
	case TD_SEEK64:
	case NSCMD_TD_SEEK64:
	break;

	case CMD_REMOVE:
	break;

	case CMD_CHANGENUM:
	    actual = hfpd->changenum;
	break;

	case CMD_ADDCHANGEINT:
	error = add_async_request (hfpd, request, ASYNC_REQUEST_CHANGEINT, get_long (request + 40));
	if (!error)
	    async = 1;
	break;
	case CMD_REMCHANGEINT:
	release_async_request (hfpd, request);
	break;
 
	case HD_SCSICMD: /* SCSI */
//	    if (hfd->nrcyls == 0) {
//		    error = handle_scsi (request, hfd);
//	    } else { /* we don't want users trashing their "partition" hardfiles with hdtoolbox */
		    error = IOERR_NOCMD;
//		write_log ("UAEHF: HD_SCSICMD tried on regular HDF, unit %d\n", unit);
//	    }
	break;

	default:
	    /* Command not understood. */
	    error = IOERR_NOCMD;
	break;
    }
    put_long (request + 32, actual);
    put_byte (request + 31, error);

    write_log ("hf: unit=%d, request=%p, cmd=%d offset=%u len=%d, actual=%d error%=%d\n", unit, request,
	get_word(request + 28), get_long (request + 44), get_long (request + 36), actual, error);
 
    return async;
}

static uae_u32 REGPARAM2 hardfile_abortio (TrapContext *context)
{
    uae_u32 request = m68k_areg(&context->regs, 1);
    int unit = mangleunit (get_long (request + 24));
    struct hardfiledata *hfd = get_hardfile_data (unit);
    struct hardfileprivdata *hfpd = &hardfpd[unit];

    write_log ("uaehf.device abortio ");
    start_thread(context, unit);
    if (!hfd || !hfpd || !hfpd->thread_running) {
	put_byte (request + 31, 32);
	write_log ("error\n");
	return get_byte (request + 31);
    }
    put_byte (request + 31, -2);
    write_log ("unit=%d, request=%08X\n",  unit, request);
    abort_async (hfpd, request, -2, 0);
    return 0;
}

static int hardfile_can_quick (uae_u32 command)
{
    switch (command)
    {
	case CMD_RESET:
	case CMD_STOP:
	case CMD_START:
	case CMD_CHANGESTATE:
	case CMD_PROTSTATUS:
	case CMD_MOTOR:
	case CMD_GETDRIVETYPE:
	case CMD_GETNUMTRACKS:
	case CMD_GETGEOMETRY:
	case NSCMD_DEVICEQUERY:
	return 1;
    }
    return 0;
}

static int hardfile_canquick (struct hardfiledata *hfd, uaecptr request)
{
    uae_u32 command = get_word (request + 28);
    return hardfile_can_quick (command);
}

static uae_u32 REGPARAM2 hardfile_beginio (TrapContext *context)
{
    uae_u32 request = m68k_areg(&context->regs, 1);
    uae_u8 flags = get_byte (request + 30);
    int cmd = get_word (request + 28);
    int unit = mangleunit (get_long (request + 24));
    struct hardfiledata *hfd = get_hardfile_data (unit);
    struct hardfileprivdata *hfpd = &hardfpd[unit];

    put_byte (request + 8, NT_MESSAGE);
    start_thread(context, unit);
    if (!hfd || !hfpd || !hfpd->thread_running) {
	put_byte (request + 31, 32);
	return get_byte (request + 31);
    }
    put_byte (request + 31, 0);
    if ((flags & 1) && hardfile_canquick (hfd, request)) {
        write_log ("hf quickio unit=%d request=%p cmd=%d\n", unit, request, cmd);
	if (hardfile_do_io (hfd, hfpd, request))
	    write_log ("uaehf.device cmd %d bug with IO_QUICK\n", cmd);
	return get_byte (request + 31);
    } else {
        write_log ("hf asyncio unit=%d request=%p cmd=%d\n", unit, request, cmd);
        add_async_request (hfpd, request, ASYNC_REQUEST_TEMP, 0);
        put_byte (request + 30, get_byte (request + 30) & ~1);
	write_comm_pipe_u32 (&hfpd->requests, request, 1);
 	return 0;
    }
}

static void *hardfile_thread (void *devs)
{
    struct hardfileprivdata *hfpd = (struct hardfileprivdata *)devs;

    uae_set_thread_priority (2);
    hfpd->thread_running = 1;
    uae_sem_post (&hfpd->sync_sem);
    for (;;) {
	uaecptr request = (uaecptr)read_comm_pipe_u32_blocking (&hfpd->requests);
	uae_sem_wait (&change_sem);
        if (!request) {
	    hfpd->thread_running = 0;
	    uae_sem_post (&hfpd->sync_sem);
	    uae_sem_post (&change_sem);
      write_log("hardfile_thread: leave\n");
	    return 0;
	} else if (hardfile_do_io (get_hardfile_data (hfpd - &hardfpd[0]), hfpd, request) == 0) {
            put_byte (request + 30, get_byte (request + 30) & ~1);
	    release_async_request (hfpd, request);
            uae_ReplyMsg (request);
	} else {
	    write_log ("async request %08X\n", request);
	}
	uae_sem_post (&change_sem);
    }
  write_log("hardfile_thread: exit\n");
}

void hardfile_reset (void)
{
  int i, j;
  struct hardfileprivdata *hfpd;

  for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
	  hfpd = &hardfpd[i];
	  if (hfpd->base && valid_address(hfpd->base, 36) && get_word(hfpd->base + 32) > 0) {
	    for (j = 0; j < MAX_ASYNC_REQUESTS; j++) {
	      uaecptr request;
		    if ((request = hfpd->d_request[j]))
		      abort_async (hfpd, request, 0, 0);
	    }
	  }
    if(hfpd->thread_running)
    {
      uae_sem_wait (&change_sem);
      write_comm_pipe_u32(&hfpd->requests, 0, 1);
      uae_sem_post (&change_sem);
      uae_sem_wait (&hfpd->sync_sem);
      uae_sem_destroy (&hfpd->sync_sem);
      destroy_comm_pipe (&hfpd->requests);
    }
	  memset (hfpd, 0, sizeof (struct hardfileprivdata));
  }
}

void hardfile_install (void)
{
    uae_u32 functable, datatable;
    uae_u32 initcode, openfunc, closefunc, expungefunc;
    uae_u32 beginiofunc, abortiofunc;
    if(change_sem != 0)
      uae_sem_destroy(&change_sem);
    change_sem = 0;
    uae_sem_init (&change_sem, 0, 1);

    ROM_hardfile_resname = ds ("uaehf.device");
    ROM_hardfile_resid = ds ("UAE hardfile.device 0.2");
    nscmd_cmd = here ();
    dw (NSCMD_DEVICEQUERY);
    dw (CMD_RESET);
    dw (CMD_READ);
    dw (CMD_WRITE);
    dw (CMD_UPDATE);
    dw (CMD_CLEAR);
    dw (CMD_START);
    dw (CMD_STOP);
    dw (CMD_FLUSH);
    dw (CMD_MOTOR);
    dw (CMD_SEEK);
    dw (CMD_FORMAT);
    dw (CMD_REMOVE);
    dw (CMD_CHANGENUM);
    dw (CMD_CHANGESTATE);
    dw (CMD_PROTSTATUS);
    dw (CMD_GETDRIVETYPE);
    dw (CMD_GETGEOMETRY);
    dw (CMD_ADDCHANGEINT);
    dw (CMD_REMCHANGEINT);
    dw (HD_SCSICMD);
    dw (NSCMD_TD_READ64);
    dw (NSCMD_TD_WRITE64);
    dw (NSCMD_TD_SEEK64);
    dw (NSCMD_TD_FORMAT64);
    dw (0);

    /* initcode */
    initcode = filesys_initcode;

    /* Open */
    openfunc = here ();
    calltrap (deftrap (hardfile_open)); dw (RTS);

    /* Close */
    closefunc = here ();
    calltrap (deftrap (hardfile_close)); dw (RTS);

    /* Expunge */
    expungefunc = here ();
    calltrap (deftrap (hardfile_expunge)); dw (RTS);

    /* BeginIO */
    beginiofunc = here ();
    calltrap (deftrap (hardfile_beginio));
    dw (RTS);

    /* AbortIO */
    abortiofunc = here ();
    calltrap (deftrap (hardfile_abortio)); dw (RTS);

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
    dl (ROM_hardfile_resname);
    dw (0xE000); /* INITBYTE */
    dw (0x000E); /* LIB_FLAGS */
    dw (0x0600); /* LIBF_SUMUSED | LIBF_CHANGED */
    dw (0xD000); /* INITWORD */
    dw (0x0014); /* LIB_VERSION */
    dw (0x0004); /* 0.4 */
    dw (0xD000);
    dw (0x0016); /* LIB_REVISION */
    dw (0x0000);
    dw (0xC000);
    dw (0x0018); /* LIB_IDSTRING */
    dl (ROM_hardfile_resid);
    dw (0x0000); /* end of table */

    ROM_hardfile_init = here ();
    dl (0x00000100); /* ??? */
    dl (functable);
    dl (datatable);
    dl (initcode);
}
