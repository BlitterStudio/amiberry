#include "sysconfig.h"
#include "sysdeps.h"

#include "devices.h"

#include "options.h"
#include "threaddep/thread.h"
#include "memory.h"
#include "audio.h"
#ifdef GFXBOARD
#include "gfxboard.h"
#endif
#include "scsi.h"
#include "scsidev.h"
#include "sana2.h"
#include "clipboard.h"
#include "cpuboard.h"
#ifdef WITH_SNDBOARD
#include "sndboard.h"
#endif
#include "statusline.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#ifdef CD32
#include "cd32_fmv.h"
#include "akiko.h"
#endif
#include "disk.h"
#include "cia.h"
#include "inputdevice.h"
#include "blkdev.h"
#include "parallel.h"
#include "autoconf.h"
#include "sampler.h"
#include "newcpu.h"
#include "blitter.h"
#include "xwin.h"
#include "custom.h"
#ifdef SERIAL_PORT
#include "serial.h"
#endif
#include "bsdsocket.h"
#include "uaeserial.h"
#include "uaeresource.h"
#include "native2amiga.h"
#include "dongle.h"
#include "gensound.h"
#include "gui.h"
#include "savestate.h"
#include "uaeexe.h"
#ifdef WITH_UAENATIVE
#include "uaenative.h"
#endif
#include "tabletlibrary.h"
#ifdef WITH_LUA
#include "luascript.h"
#endif
#ifdef DRIVESOUND
#include "driveclick.h"
#endif
#ifdef WITH_X86
#include "x86.h"
#endif
#include "ethernet.h"
#include "drawing.h"
#ifdef AVIOUTPUT
#include "videograb.h"
#endif
#ifdef AHI
#include "ahi_v1.h"
#endif
#include "rommgr.h"
#include "newcpu.h"
#ifdef WITH_MIDIEMU
#include "midiemu.h"
#endif
#ifdef RETROPLATFORM
#include "rp.h"
#endif
#ifdef WITH_DRACO
#include "draco.h"
#endif
#ifdef WITH_DSP
#include "dsp3210/dsp_glue.h"
#endif
#include "keyboard_mcu.h"

#define MAX_DEVICE_ITEMS 64

static void add_device_item(DEVICE_VOID *pp, int *cnt, DEVICE_VOID p)
{
	for (int i = 0; i < *cnt; i++) {
		if (pp[i] == p)
			return;
	}
	if (*cnt >= MAX_DEVICE_ITEMS) {
		return;
	}
	pp[(*cnt)++] = p;
}
static void execute_device_items(DEVICE_VOID *pp, int cnt)
{
	for (int i = 0; i < cnt; i++) {
		pp[i]();
	}
}

static int device_configs_cnt;
static DEVICE_VOID device_configs[MAX_DEVICE_ITEMS];
static int device_vsync_pre_cnt;
static DEVICE_VOID device_vsyncs_pre[MAX_DEVICE_ITEMS];
static int device_vsync_post_cnt;
static DEVICE_VOID device_vsyncs_post[MAX_DEVICE_ITEMS];
static int device_hsync_cnt;
static DEVICE_VOID device_hsyncs[MAX_DEVICE_ITEMS];
static int device_rethink_cnt;
static DEVICE_VOID device_rethinks[MAX_DEVICE_ITEMS];
static int device_leave_cnt;
static DEVICE_VOID device_leaves[MAX_DEVICE_ITEMS];
static int device_leave_early_cnt;
static DEVICE_VOID device_leaves_early[MAX_DEVICE_ITEMS];
static int device_resets_cnt;
static DEVICE_INT device_resets[MAX_DEVICE_ITEMS];
static bool device_reset_done[MAX_DEVICE_ITEMS];

static void reset_device_items(void)
{
	device_configs_cnt = 0;
	device_vsync_pre_cnt = 0;
	device_vsync_post_cnt = 0;
	device_hsync_cnt = 0;
	device_rethink_cnt = 0;
	device_resets_cnt = 0;
	device_leave_cnt = 0;
	device_leave_early_cnt = 0;
	memset(device_reset_done, 0, sizeof(device_reset_done));
}

void device_add_vsync_pre(DEVICE_VOID p)
{
	add_device_item(device_vsyncs_pre, &device_vsync_pre_cnt, p);
}
void device_add_vsync_post(DEVICE_VOID p)
{
	add_device_item(device_vsyncs_post, &device_vsync_post_cnt, p);
}
void device_add_hsync(DEVICE_VOID p)
{
	add_device_item(device_hsyncs, &device_hsync_cnt, p);
}
void device_add_rethink(DEVICE_VOID p)
{
	add_device_item(device_rethinks, &device_rethink_cnt, p);
}
void device_add_check_config(DEVICE_VOID p)
{
	add_device_item(device_configs, &device_configs_cnt, p);
}
void device_add_exit(DEVICE_VOID p, DEVICE_VOID p2)
{
	if (p != NULL) {
		add_device_item(device_leaves, &device_leave_cnt, p);
	}
	if (p2 != NULL) {
		add_device_item(device_leaves_early, &device_leave_early_cnt, p2);
	}
}
void device_add_reset(DEVICE_INT p)
{
	for (int i = 0; i < device_resets_cnt; i++) {
		if (device_resets[i] == p)
			return;
	}
	device_resets[device_resets_cnt++] = p;
}
void device_add_reset_imm(DEVICE_INT p)
{
	for (int i = 0; i < device_resets_cnt; i++) {
		if (device_resets[i] == p)
			return;
	}
	device_reset_done[device_resets_cnt] = true;
	device_resets[device_resets_cnt++] = p;
	p(1);
}

void device_check_config(void)
{
	execute_device_items(device_configs, device_configs_cnt);

	check_prefs_changed_cd();
	check_prefs_changed_audio();
	check_prefs_changed_custom();
	check_prefs_changed_cpu();
	check_prefs_picasso();
}

void devices_reset_ext(int hardreset)
{
	for (int i = 0; i < device_resets_cnt; i++) {
		if (!device_reset_done[i]) {
			device_resets[i](hardreset);
			device_reset_done[i] = true;
		}
	}
}

void devices_reset(int hardreset)
{
	memset(device_reset_done, 0, sizeof(device_reset_done));
	// must be first
	init_eventtab();
	init_shm();
	memory_reset();
#ifdef AUTOCONFIG
	rtarea_reset();
#endif
	DISK_reset();
	CIA_reset();
	a1000_reset();
#ifdef JIT
	compemu_reset();
#endif
#ifdef WITH_PPC
	uae_ppc_reset(is_hardreset());
#endif
	native2amiga_reset();
#ifdef SCSIEMU
	scsi_reset();
	scsidev_reset();
	scsidev_start_threads();
#endif
#ifdef GFXBOARD
	gfxboard_reset ();
#endif
#ifdef DRIVESOUND
	driveclick_reset();
#endif
	ethernet_reset();
	reset_traps();
#ifdef FILESYS
	filesys_prepare_reset();
	filesys_reset();
#endif
#if defined (BSDSOCKET)
	bsdlib_reset();
#endif
#ifdef FILESYS
	filesys_start_threads();
	hardfile_reset();
#endif
#ifdef UAESERIAL
	uaeserialdev_reset();
	uaeserialdev_start_threads();
#endif
#if defined (PARALLEL_PORT)
	initparallel();
#endif
#if defined (AHI)
	init_ahi();
#endif
	dongle_reset();
	sampler_init();
	device_func_reset();
#ifdef RETROPLATFORM
	rp_reset();
#endif
	keymcu_reset();
	keymcu2_reset();
	keymcu3_reset();
	uae_int_requested = 0;
}

void devices_vsync_pre(void)
{
	audio_vsync();
	blkdev_vsync();
	CIA_vsync_prehandler();
	inputdevice_vsync();
	filesys_vsync();
	sampler_vsync();
	clipboard_vsync();
	statusline_vsync();

	execute_device_items(device_vsyncs_pre, device_vsync_pre_cnt);
}

void devices_vsync_post(void)
{
	execute_device_items(device_vsyncs_post, device_vsync_post_cnt);
}

void devices_hsync(void)
{
	DISK_hsync();
	audio_hsync();
	CIA_hsync_prehandler();

	decide_blitter(-1);
#ifdef AHI
	ahi_hsync();
#endif
#ifdef SERIAL_PORT
	serial_hsynchandler();
#endif

	execute_device_items(device_hsyncs, device_hsync_cnt);
}

void devices_rethink_all(void func(void))
{
	func();
}

void devices_rethink(void)
{
	rethink_cias ();

	execute_device_items(device_rethinks, device_rethink_cnt);

	rethink_uae_int();
	/* cpuboard_rethink must be last */
	cpuboard_rethink();
}

void devices_update_sound(float clk, float syncadjust)
{
	update_sound (clk);
#ifdef WITH_SNDBOARD
	update_sndboard_sound (clk / syncadjust);
#endif
	update_cda_sound(clk / syncadjust);
#ifdef WITH_X86
	x86_update_sound(clk / syncadjust);
#endif
#ifdef WITH_MIDIEMU
	midi_update_sound(clk / syncadjust);
#endif
}

void devices_update_sync(float svpos, float syncadjust)
{
#ifdef CD32
#ifdef USE_LIBMPEG2
	cd32_fmv_set_sync(svpos, syncadjust);
#endif
#endif
}

void virtualdevice_free(void)
{
#ifdef WITH_PPC
	// must be first
	uae_ppc_free();
#endif

	execute_device_items(device_leaves_early, device_leave_early_cnt);

	reset_traps();
	free_traps();
#ifdef FILESYS
	filesys_cleanup();
#endif
#ifdef BSDSOCKET
	bsdlib_reset();
#endif
	sampler_free();
	inputdevice_close();
	DISK_free();
	//dump_counts();
#ifdef SERIAL_PORT
	serial_exit();
#endif
#ifdef AUTOCONFIG
	expansion_cleanup();
#endif
	device_func_free();
#ifdef WITH_LUA
	uae_lua_free();
#endif
#ifdef GFXBOARD
	gfxboard_free();
#endif
	savestate_free();
	memory_cleanup();
	free_shm();
	cfgfile_addcfgparam(0);
#ifdef DRIVESOUND
	driveclick_free();
#endif
	ethernet_enumerate_free();
	rtarea_free();
#ifdef WITH_DRACO
	draco_free();
#endif
	keymcu_free();
	keymcu2_free();
	keymcu3_free();
	execute_device_items(device_leaves, device_leave_cnt);
}

void do_leave_program (void)
{
	virtualdevice_free();
	graphics_leave();
	close_sound();
	if (! no_gui)
		gui_exit();

	//machdep_free();
}

void virtualdevice_init (void)
{
	reset_device_items();

#ifdef CD32
	akiko_init();
#endif
#ifdef AUTOCONFIG
	rtarea_setup ();
#endif
#ifdef FILESYS
	rtarea_init ();
	uaeres_install ();
	hardfile_install ();
#endif
#ifdef SCSIEMU
	scsi_reset ();
	scsidev_install ();
#endif
#ifdef SANA2
	netdev_install ();
#endif
#ifdef UAESERIAL
	uaeserialdev_install ();
#endif
#ifdef AUTOCONFIG
	expansion_init ();
	emulib_install ();
	uaeexe_install ();
#endif
#ifdef FILESYS
	filesys_install ();
#endif
#if defined (BSDSOCKET)
	bsdlib_install ();
#endif
#ifdef WITH_UAENATIVE
	uaenative_install ();
#endif
#ifdef WITH_TABLETLIBRARY
	tabletlib_install ();
#endif
#ifdef WITH_DRACO
	draco_init();
#endif
	keymcu_init();
	keymcu2_init();
	keymcu3_init();
}

void devices_restore_start(void)
{
	restore_audio_start();
	restore_cia_start();
	restore_blkdev_start();
	restore_blitter_start();
	restore_custom_start();
	changed_prefs.bogomem.size = 0;
	changed_prefs.chipmem.size = 0;
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		changed_prefs.fastmem[i].size = 0;
		changed_prefs.z3fastmem[i].size = 0;
	}
	changed_prefs.mbresmem_low.size = 0;
	changed_prefs.mbresmem_high.size = 0;
	restore_expansion_boards(NULL);
}

void devices_syncchange(void)
{
#ifdef WITH_X86
	x86_bridge_sync_change();
#endif
}

void devices_pause(void)
{
#ifdef WITH_PPC
	uae_ppc_pause(1);
#endif
#ifdef WITH_DSP
	dsp_pause(1);
#endif
	blkdev_entergui();
#ifdef RETROPLATFORM
	rp_pause(1);
#endif
#ifdef AVIOUTPUT
	pausevideograb(1);
#endif
	ethernet_pause(1);
}

void devices_unpause(void)
{
	blkdev_exitgui();
#ifdef RETROPLATFORM
	rp_pause(0);
#endif
#ifdef WITH_PPC
	uae_ppc_pause(0);
#endif
#ifdef WITH_DSP
	dsp_pause(0);
#endif
#ifdef AVIOUTPUT
	pausevideograb(0);
#endif
	ethernet_pause(0);
}

void devices_unsafeperiod(void)
{
	clipboard_unsafeperiod();
}
