

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "execlib.h"
#include "disk.h"
#include "rommgr.h"
#include "uae.h"
#include "threaddep/thread.h"
#include "keybuf.h"

#include "consolehook.h"

static uaecptr beginio;

void consolehook_config (struct uae_prefs *p)
{
	struct uaedev_config_info ci = { 0 };
	int roms[] = { 15, 31, 16, 46, -1 };

	default_prefs (p, true, 0);
	//p->headless = 1;
	p->produce_sound = 0;
	p->gfx_resolution = 0;
	p->gfx_vresolution = 0;
	p->gfx_iscanlines = 0;
	p->gfx_pscanlines = 0;
	p->gfx_framerate = 10;
	p->immediate_blits = 1;
	p->collision_level = 0;
	configure_rom (p, roms, 0);
	p->cpu_model = 68020;
	p->fpu_model = 68882;
	p->m68k_speed = -1;
	p->cachesize = 8192;
	p->cpu_compatible = 0;
	p->address_space_24 = 0;
	p->chipmem.size = 0x00200000;
	p->fastmem[0].size = 0x00800000;
	p->bogomem.size = 0;
	p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->floppy_speed = 0;
	p->start_gui = 0;
	p->gfx_monitor[0].gfx_size_win.width = 320;
	p->gfx_monitor[0].gfx_size_win.height = 256;
	p->turbo_emulation = 0;
	//p->automount_drives = 2;
	//p->automount_cddrives = 2;

	_tcscpy (ci.rootdir, _T("."));
	_tcscpy (ci.volname, _T("CLIBOOT"));
	_tcscpy (ci.devname, _T("DH0"));
	ci.bootpri = 15;
	ci.type = UAEDEV_DIR;
	add_filesys_config (p, -1, &ci);
}

static int console_thread (void *v)
{
	uae_set_thread_priority (NULL, 1);
	for (;;) {
		TCHAR wc = console_getch ();
		char c[2];

		c[0] = 0;
		c[1] = 0;
		ua_copy (c, 1, &wc);
		record_key_direct((0x10 << 1) | 0, false);
		record_key_direct((0x10 << 1) | 1, false);
	}
	return 0;
}

int consolehook_activate (void)
{
	return console_emulation;
}

void consolehook_ret(TrapContext *ctx, uaecptr condev, uaecptr oldbeginio)
{
	beginio = oldbeginio;
	write_log (_T("console.device at %08X\n"), condev);

	uae_start_thread (_T("consolereader"), console_thread, NULL, NULL);
}

uaecptr consolehook_beginio(TrapContext *ctx, uaecptr request)
{
	uae_u32 io_data = trap_get_long(ctx, request + 40); // 0x28
	uae_u32 io_length = trap_get_long(ctx, request + 36); // 0x24
	uae_u32 io_actual = trap_get_long(ctx, request + 32); // 0x20
	uae_u32 io_offset = trap_get_long(ctx, request + 44); // 0x2c
	uae_u16 cmd = trap_get_word(ctx, request + 28);

	if (cmd == CMD_WRITE) {
		int len = io_length;
		char *dbuf;
		TCHAR *buf;
		if (len == -1) {
			dbuf = xmalloc(char, MAX_DPATH);
			trap_get_string(ctx, dbuf, io_data, MAX_DPATH);
			len = uaestrlen(dbuf);
		} else {
			dbuf = xmalloc(char, len);
			trap_get_bytes(ctx, dbuf, io_data, len);
		}
		buf = xmalloc(TCHAR, len + 1);
		au_copy(buf, len, dbuf);
		buf[len] = 0;
		f_out(stdout, _T("%s"), buf);
		xfree(buf);
		xfree(dbuf);
	} else if (cmd == CMD_READ) {

		write_log (_T("%08x: CMD=%d LEN=%d OFF=%d ACT=%d\n"), request, cmd, io_length, io_offset, io_actual);

	}
	return beginio;
}
