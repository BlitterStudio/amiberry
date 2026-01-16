#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "execlib.h"
#include "disk.h"
#include "rommgr.h"
#include "uae.h"
#include "threaddep/thread.h"
#include "fsdb.h"
#include "custom.h"
#include "consolehook.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern int consoleopen;

#define CONSOLEHOOK_DEBUG 0

static uaecptr beginio;

static uae_sem_t console_in_sem;
static uae_atomic console_in_init;
static uae_atomic console_thread_started;
static uae_atomic console_stop;

static unsigned char console_in_buf[8192];
static volatile uae_u32 console_in_rp;
static volatile uae_u32 console_in_wp;

static void console_in_init_once(void)
{
	if (console_in_init)
		return;
	console_in_init = 1;
	uae_sem_init(&console_in_sem, 0, 1);
	console_in_rp = console_in_wp = 0;
}

static uae_u32 console_in_avail_nolock(void)
{
	return (console_in_wp - console_in_rp) & (sizeof console_in_buf - 1);
}

static void console_in_push(unsigned char b)
{
	uae_sem_wait(&console_in_sem);
	uae_u32 next = (console_in_wp + 1) & (sizeof console_in_buf - 1);
	if (next != console_in_rp) {
		console_in_buf[console_in_wp] = b;
		console_in_wp = next;
	}
	uae_sem_post(&console_in_sem);
#if CONSOLEHOOK_DEBUG
	write_log(_T("[consolehook] push %02X avail=%u\n"), b, console_in_avail_nolock());
#endif
}

static uae_u32 console_in_pop(unsigned char *dst, uae_u32 maxlen)
{
	uae_u32 got = 0;
	uae_sem_wait(&console_in_sem);
	while (got < maxlen && console_in_rp != console_in_wp) {
		dst[got++] = console_in_buf[console_in_rp];
		console_in_rp = (console_in_rp + 1) & (sizeof console_in_buf - 1);
	}
	uae_sem_post(&console_in_sem);
	return got;
}

/* CMD_READ must not block: it is called on the emulation thread. */

void consolehook_shutdown(void)
{
	console_stop = 1;
}

void consolehook_config (struct uae_prefs *p, const TCHAR *custom_path)
{
	struct uaedev_config_info ci = { 0 };
	int roms[8];
	roms[0] = 11;
	roms[1] = 15;
	roms[2] = 276;
	roms[3] = 281;
	roms[4] = 286;
	roms[5] = 291;
	roms[6] = 304;
	roms[7] = -1;

	consoleopen = 0;
	default_prefs (p, true, 0);
	//p->headless = true;
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
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
#ifdef JIT
	p->cachesize = 16384;
#else
	p->cachesize = 0;
#endif
	p->chipmem.size = 0x200000;
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
	p->cs_compatible = CP_A1200;
	built_in_chipset_prefs (p);

	uci_set_defaults(&ci, false);
	if (custom_path && custom_path[0]) {
		// Use the custom path provided via command line
		my_canonicalize_path(custom_path, ci.rootdir, MAX_DPATH);
	} else {
		// Use current working directory as before
		TCHAR cwd[MAX_DPATH];
		cwd[0] = 0;
#ifdef _WIN32
		GetCurrentDirectory(MAX_DPATH, cwd);
#else
		getcwd(cwd, MAX_DPATH);
#endif
		if (cwd[0])
			my_canonicalize_path(cwd, ci.rootdir, MAX_DPATH);
		else
			_tcscpy(ci.rootdir, _T("."));
	}
	_tcscpy (ci.volname, _T("CLIBOOT"));
	_tcscpy (ci.devname, _T("DH0"));
	ci.bootpri = 15;
	ci.type = UAEDEV_DIR;
	add_filesys_config (p, -1, &ci);
}

static int console_thread (void *v)
{
	uae_set_thread_priority (NULL, 1);
	console_in_init_once();
	set_console_input_mode(0);
	for (;;) {
		if (console_stop)
			break;
		TCHAR wc = console_getch ();
		if (!wc)
			continue;
		char c = (char)wc;
		/* Normalize host console keys to what Amiga console.device expects. */
		if (c == '\r')
			c = '\n';
		console_in_push((unsigned char)c);
	}
	set_console_input_mode(1);
	console_thread_started = 0;
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
	open_console();
	console_in_init_once();
	if (!console_thread_started) {
		console_thread_started = 1;
		console_stop = 0;
		uae_start_thread (_T("consolereader"), console_thread, NULL, NULL);
	}
}

uaecptr consolehook_beginio(TrapContext *ctx, uaecptr request)
{
	uae_u32 io_data = trap_get_long(ctx, request + 40); // 0x28
	uae_u32 io_length = trap_get_long(ctx, request + 36); // 0x24
	(void)trap_get_long(ctx, request + 32); // io_actual (unused)
	(void)trap_get_long(ctx, request + 44); // io_offset (unused)
	uae_u16 cmd = trap_get_word(ctx, request + 28);

	if (cmd == CMD_WRITE) {
		open_console();
		int len = (int)io_length;
		if (len < 0)
			len = 0;
		if (len > 1024 * 1024)
			len = 1024 * 1024;
		if (len > 0 && io_data) {
			char *dbuf = xmalloc(char, len);
			if (!dbuf)
				return beginio;
			trap_get_bytes(ctx, dbuf, io_data, len);

			for (int i = 0; i < len; i++) {
				TCHAR ch = (TCHAR)dbuf[i];
				if (ch == '\n')
					console_out_f(_T("\r"));
				console_out_f(_T("%c"), ch);
			}
			console_flush();
			xfree(dbuf);
		}
	} else if (cmd == CMD_READ) {
		console_in_init_once();
		uae_u32 maxlen = io_length;
		if ((uae_s32)maxlen < 0)
			maxlen = 0;
		if (maxlen > 4096)
			maxlen = 4096;
		unsigned char tmp[4096];
		uae_u32 got = 0;
		if (maxlen && io_data) // Fix: check io_data (buf) is not NULL before use
			got = console_in_pop(tmp, maxlen);
#if CONSOLEHOOK_DEBUG
		write_log(_T("[consolehook] CMD_READ max=%u got=%u\n"), maxlen, got);
#endif
		if (got && io_data) // Fix: check io_data (buf) is not NULL before use
			trap_put_bytes(ctx, tmp, io_data, got);
		trap_put_long(ctx, request + 32, got); // io_actual
	}
	return beginio;
}
