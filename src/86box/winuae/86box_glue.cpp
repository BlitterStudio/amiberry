/*
* UAE - The Un*x Amiga Emulator
*
* Services the imported 86Box code expects from the 86Box application
* (logging, the device list, the platform layer and mouse input), and
* stubs for the 86Box subsystems WinUAE doesn't use.
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#if defined(__APPLE__) && defined(__aarch64__)
#include <pthread.h>
#endif

#include "86box_glue.h"

#include <86box/86box.h>
#include <86box/device.h>
#include <86box/timer.h>
#include <86box/plat.h>
#include <86box/path.h>
#include <86box/mem.h>
#include <86box/rom.h>
#include <86box/pci.h>
#include <86box/chipset.h>
#include <86box/mca.h>
#include <86box/mouse.h>
#include <86box/keyboard.h>
#include <86box/sound.h>
#include <86box/snd_opl.h>
#include <86box/snd_emu8k.h>
#include <86box/snd_azt2316a.h>
#include <86box/snd_azt2320.h>
#include <86box/midi.h>
#include <86box/gameport.h>
#include <86box/isapnp.h>
#include <86box/hdc.h>
#include <86box/hdc_ide.h>
#include <86box/fdd.h>
#include <86box/nvr.h>
#include <86box/cassette.h>
#include <86box/char.h>
#include <86box/serial.h>
#include <86box/i8018x.h>
#include <86box/video.h>
#include <86box/m_at_t3100e.h>
#include <86box/m_xt_xi8088.h>
#include <86box/m_ibm5140.h>
#include <86box/nv/vid_nv_rivatimer.h>
#include "86box/cpu/cpu.h"
#ifdef USE_DYNAREC
#include "86box/cpu/codegen_public.h"
#endif

#ifdef WITH_MIDI
#include "midi.h"
#endif

#ifdef DEBUGGER
extern void activate_debugger(void);
#endif

/* Logging */

void pclog(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	write_log("%s", buf);
}

void fatal(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	write_log("86BOXFATAL: %s", buf);
#ifdef DEBUGGER
	activate_debugger();
#endif
}

/* Configuration the 86Box application normally owns */

int cpu;
int cpu_use_dynarec;
int fpu_softfloat = 1;
int fpu_type;
int machine;
uint32_t mem_size;
int time_sync = TIME_SYNC_ENABLED;
int time_offset_years;
int video_fullscreen;
int fullscreen_ui_visible;
int dump_missing;
int kbd_req_capture;
/* WinUAE only passes mouse input on while the PC side has the mouse */
int mouse_capture = 1;
int mouse_automation;
int other_ide_present;
int inboard386_present;
int is_pcjr;
/* acpi.h declares it with C linkage but can't be included here */
extern "C" {
int acpi_rtc_status;
}
uint32_t amstrad_latch;
pc_cassette_t *cassette;
struct i8018x_pcb_t *i8018x_pcb_current;
fdd_drive_t drives[FDD_NUM];
char usr_path[1024];
__thread int is_cpu_thread;

int pci_flags;
uint32_t pci_base;
uint32_t pci_size;
int pci_burst_time;
int pci_nonburst_time;
int agp_burst_time;
int agp_nonburst_time;

int music_pos_global;
int sound_sample_rate = SOUND_FREQ;
void (*filter_pc_speaker)(int channel, double *buffer, void *priv);
void *filter_pc_speaker_p;

/* Device list. Configuration comes from WinUAE through
   device_get_config_int(), so there's no device context. */

#define MAX_DEVICES 64

struct added_device
{
	const device_t *dev;
	device_t *init_dev;
	void *priv;
	int inst;
};

static struct added_device devices[MAX_DEVICES];
static int serial_mouse_port;
/* The device whose init function is running */
static const device_t *device_current;

static void *device_add_common(const device_t *dev, void *priv, void *params, int inst)
{
	if (!dev) {
		return NULL;
	}
	for (int i = 0; i < MAX_DEVICES; i++) {
		struct added_device *ad = &devices[i];
		if (ad->dev) {
			continue;
		}
		ad->dev = dev;
		ad->inst = inst;
		ad->init_dev = NULL;
		if (params) {
			ad->init_dev = xmalloc(device_t, 1);
			memcpy(ad->init_dev, dev, sizeof(device_t));
			ad->init_dev->local |= (uintptr_t)params;
		}
		if (priv) {
			ad->priv = priv;
		} else if (dev->init) {
			const device_t *prev = device_current;
			device_current = ad->init_dev ? ad->init_dev : dev;
			ad->priv = dev->init(device_current);
			device_current = prev;
		} else {
			ad->priv = NULL;
		}
		return ad->priv;
	}
	write_log(_T("86Box: too many devices, can't add '%s'\n"), dev->name);
	return NULL;
}

void *device_add(const device_t *dev)
{
	return device_add_common(dev, NULL, NULL, 0);
}

void *device_add_params(const device_t *dev, void *params)
{
	return device_add_common(dev, NULL, params, 0);
}

void *device_add_inst(const device_t *dev, int inst)
{
	return device_add_common(dev, NULL, NULL, inst);
}

void *device_add_inst_params(const device_t *dev, int inst, void *params)
{
	return device_add_common(dev, NULL, params, inst);
}

void device_add_ex(const device_t *dev, void *priv)
{
	device_add_common(dev, priv, NULL, 0);
}

void *device_get_priv(const device_t *dev)
{
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i].dev == dev) {
			return devices[i].priv;
		}
	}
	return NULL;
}

static void device_remove(struct added_device *ad)
{
	if (ad->dev->close && ad->priv) {
		ad->dev->close(ad->priv);
	}
	xfree(ad->init_dev);
	memset(ad, 0, sizeof(struct added_device));
}

void device_close_by_flags(uint32_t match_flags)
{
	for (int i = MAX_DEVICES - 1; i >= 0; i--) {
		if (devices[i].dev && (devices[i].dev->flags & match_flags)) {
			device_remove(&devices[i]);
		}
	}
}

/* 86Box's timer list must never become empty (timer_remove_head() doesn't
   check for that). In 86Box the sound poll timer always runs, here a once
   a second timer does the same job. */
static pc_timer_t keepalive_timer;

static void keepalive_timer_poll(void *priv)
{
	timer_advance_u64(&keepalive_timer, TIMER_USEC * 1000000);
}

void x86pc_codegen_init(void)
{
#ifdef USE_DYNAREC
	static bool initialized;
	if (!cpu_use_dynarec || initialized) {
		return;
	}
	/* The code buffers are allocated once, codegen_reset() clears them */
	initialized = true;
#if defined(__APPLE__) && defined(__aarch64__)
	if (__builtin_available(macOS 11.0, *)) {
		pthread_jit_write_protect_np(0);
	}
#endif
	codegen_init();
#if defined(__APPLE__) && defined(__aarch64__)
	if (__builtin_available(macOS 11.0, *)) {
		pthread_jit_write_protect_np(1);
	}
#endif
	write_log(_T("86Box: x86 recompiler enabled\n"));
#endif
}

void x86pc_timer_init(void)
{
	timer_init();
	timer_add(&keepalive_timer, keepalive_timer_poll, NULL, 0);
	timer_set_delay_u64(&keepalive_timer, TIMER_USEC * 1000000);
}

void x86pc_close_devices(void)
{
	/* Unlink all timers first, as 86Box does on reset: the devices'
	   timers are in the structs closing them frees. */
	timer_close();
	for (int i = MAX_DEVICES - 1; i >= 0; i--) {
		if (devices[i].dev) {
			device_remove(&devices[i]);
		}
	}
}

void device_reset_all(uint32_t match_flags)
{
	for (int i = 0; i < MAX_DEVICES; i++) {
		const device_t *dev = devices[i].dev;
		if (dev && dev->reset && (dev->flags & match_flags)) {
			dev->reset(devices[i].priv);
		}
	}
}

void device_speed_changed(void)
{
	for (int i = 0; i < MAX_DEVICES; i++) {
		const device_t *dev = devices[i].dev;
		if (dev && dev->speed_changed) {
			dev->speed_changed(devices[i].priv);
		}
	}
}

void device_context(const device_t *dev)
{
}

void device_context_restore(void)
{
}

/* WinUAE sets the options it has (x86_device_config() in x86.cpp), the
   rest keep the default of the device's own configuration table. */
int device_get_config_int(const char *name)
{
	int value;
	if (device_current == &mouse_msserial_device && !strcmp(name, "port")) {
		return serial_mouse_port;
	}
	if (x86_device_config(device_current, name, &value)) {
		return value;
	}
	if (device_current && device_current->config) {
		for (const device_config_t *c = device_current->config; c->type != CONFIG_END && c->name; c++) {
			if (!strcmp(c->name, name)) {
				return c->default_int;
			}
		}
	}
	return 0;
}

int device_get_config_hex16(const char *name)
{
	return device_get_config_int(name);
}

int device_get_config_hex20(const char *name)
{
	return device_get_config_int(name);
}

const char *device_get_internal_name(const device_t *dev)
{
	return dev ? dev->internal_name : NULL;
}

/* Platform */

/* WinUAE loads and saves the files itself (CMOS, ROMs), 86Box code must not
   create files of its own (nvr_close() saves NVR files, for example). */
FILE *plat_fopen(const char *path, const char *mode)
{
	return NULL;
}

int plat_dir_check(char *path)
{
	return 0;
}

int plat_dir_create(char *path)
{
	return -1;
}

void path_slash(char *path)
{
	size_t len = strlen(path);
	if (len > 0 && path[len - 1] != '/' && path[len - 1] != '\\') {
		path[len] = '/';
		path[len + 1] = 0;
	}
}

void *plat_mmap(size_t size, uint8_t executable, uint8_t *large)
{
	if (large) {
		*large = 0;
	}
#ifdef _WIN32
	return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
#else
	int flags = MAP_ANON | MAP_PRIVATE;
#if defined(__APPLE__) && defined(MAP_JIT)
	if (executable) {
		flags |= MAP_JIT;
	}
#endif
	void *p = mmap(NULL, size, PROT_READ | PROT_WRITE | (executable ? PROT_EXEC : 0), flags, -1, 0);
	return p == MAP_FAILED ? NULL : p;
#endif
}

void plat_munmap(void *ptr, size_t size)
{
	if (!ptr) {
		return;
	}
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	munmap(ptr, size);
#endif
}

int rom_present(const char *fn)
{
	return 0;
}

FILE *rom_fopen(const char *fn, char *mode)
{
	return NULL;
}

/* Mouse: WinUAE feeds relative movement, the 86Box mouse devices take it
   out through the mouse_subtract_*() calls when they report. */

static double mouse_x, mouse_y;
static int mouse_z, mouse_w;
static int mouse_buttons, mouse_old_b, mouse_delta_b, mouse_nbut = 2;
static int (*mouse_dev_poll)(void *);
static void *mouse_priv;
static pc_timer_t mouse_timer;
static double mouse_sample_rate = 100.0;
int mouse_timed;

void mouse_process(void)
{
	if (mouse_dev_poll) {
		mouse_dev_poll(mouse_priv);
	}
}

static void mouse_timer_poll(void *priv)
{
	timer_on_auto(&mouse_timer, 1000000.0 / mouse_sample_rate);
	mouse_process();
}

void x86pc_mouse_input(int dx, int dy, int dz, int buttons)
{
	mouse_x += dx;
	mouse_y += dy;
	mouse_z += dz;
	mouse_buttons = buttons;
	if (!mouse_timed) {
		mouse_process();
	}
}

static void mouse_subtract(double *v, int *delta, int *o, int min, int max, int invert)
{
	double real = invert ? -*v : *v;
	int scaled = real >= 0 ? (int)floor(real) : (int)ceil(real);
	if (o) {
		*o = 1;
	}
	if (real > max) {
		*delta = max;
		real -= max;
	} else if (real < min) {
		*delta = min;
		real -= min;
	} else {
		*delta = scaled;
		real -= scaled;
		if (o) {
			*o = 0;
		}
	}
	*v = invert ? -real : real;
}

void mouse_subtract_coords(int *delta_x, int *delta_y, int *o_x, int *o_y, int min, int max, int invert, int abs)
{
	mouse_subtract(&mouse_x, delta_x, o_x, min, max, 0);
	mouse_subtract(&mouse_y, delta_y, o_y, min, max, invert);
}

static void mouse_subtract_wheel(int *wheel, int *delta, int min, int max, int invert)
{
	int real = invert ? -*wheel : *wheel;
	if (real > max) {
		*delta = max;
		real -= max;
	} else if (real < min) {
		*delta = min;
		real -= min;
	} else {
		*delta = real;
		real = 0;
	}
	*wheel = invert ? -real : real;
}

void mouse_subtract_z(int *delta_z, int min, int max, int invert)
{
	mouse_subtract_wheel(&mouse_z, delta_z, min, max, invert);
}

void mouse_subtract_w(int *delta_w, int min, int max, int invert)
{
	mouse_subtract_wheel(&mouse_w, delta_w, min, max, invert);
}

void mouse_clear_coords(void)
{
	mouse_x = mouse_y = 0;
	mouse_z = mouse_w = 0;
}

int mouse_state_changed(void)
{
	int b_mask = (1 << mouse_nbut) - 1;
	mouse_delta_b = mouse_buttons ^ mouse_old_b;
	mouse_old_b = mouse_buttons;
	return (int)floor(fabs(mouse_x)) || (int)floor(fabs(mouse_y)) ||
		(mouse_z != 0 && mouse_nbut >= 4) || (mouse_w != 0 && mouse_nbut >= 6) ||
		(mouse_delta_b & b_mask);
}

int mouse_mbut_changed(void)
{
	return !!(mouse_delta_b & 0x04);
}

int mouse_get_buttons_ex(void)
{
	return mouse_buttons;
}

void mouse_set_buttons(int buttons)
{
	if (mouse_nbut < buttons) {
		mouse_nbut = buttons;
	}
}

void mouse_set_poll(int (*f)(void *), void *priv)
{
	mouse_dev_poll = f;
	mouse_priv = priv;
}

void mouse_set_sample_rate(double new_rate)
{
	mouse_timed = new_rate > 0.0;
	if (!mouse_timer.callback) {
		timer_add(&mouse_timer, mouse_timer_poll, NULL, 0);
	}
	timer_stop(&mouse_timer);
	mouse_sample_rate = new_rate;
	if (mouse_timed) {
		timer_on_auto(&mouse_timer, 1000000.0 / mouse_sample_rate);
	}
}

/* FM synthesis: only the Nuked OPL2 and OPL3 cores are included. They run
   at 48kHz so that their output can be mixed with the other sound. */

uint8_t fm_driver_get_ex(int chip_id, fm_drv_t *drv, int is_48k)
{
	uintptr_t flags = FM_FORCE_48K | ((is_48k & 2) ? FM_CRYSTAL : 0);
	switch (chip_id)
	{
		case FM_YM3812:
		*drv = nuked_opl2_drv_48k;
		drv->priv = device_add_inst_params(&ym3812_nuked_opl2_device, 0, (void *)flags);
		return 1;
		case FM_YMF262:
		*drv = nuked_opl3_drv_48k;
		drv->priv = device_add_inst_params(&ymf262_nuked_opl3_device, 0, (void *)flags);
		return 1;
	}
	write_log(_T("86Box: FM chip %d not available\n"), chip_id);
	return 0;
}

uint8_t fm_driver_get(int chip_id, fm_drv_t *drv)
{
	return fm_driver_get_ex(chip_id, drv, 0);
}

uint8_t fm_driver_get_cs(int chip_id, fm_drv_t *drv)
{
	return fm_driver_get_ex(chip_id, drv, 2);
}

/* MIDI out of the MPU-401 goes to WinUAE's MIDI port */

static int midi_open;

void midi_raw_out_byte(uint8_t val)
{
#ifdef WITH_MIDI
	if (!midi_open) {
		midi_open = Midi_Open();
	}
	BYTE b = (BYTE)val;
	Midi_Parse(midi_output, &b);
#endif
}

void midi_raw_out_rt_byte(uint8_t val)
{
	midi_raw_out_byte(val);
}

void midi_clear_buffer(void)
{
}

void midi_reset(void)
{
#ifdef WITH_MIDI
	if (midi_open) {
		Midi_Close();
	}
#endif
	midi_open = 0;
}

void midi_in_handler(int set, void (*msg)(void *priv, uint8_t *msg, uint32_t len), int (*sysex)(void *priv, uint8_t *buffer, uint32_t len, int abort), int (*poll)(void *priv), void *priv)
{
}

/* Sound inputs and filters WinUAE doesn't have */

void sound_in_add_handler(void (*put_buffer)(int16_t *buffer, int len, void *priv), void *priv)
{
}

void sound_in_start_input(void)
{
}

void wavetable_add_handler(void (*get_buffer)(int32_t *buffer, uint16_t len, void *priv), void *priv)
{
}

void sound_set_cd_audio_filter(void (*filter)(int channel, double *buffer, void *priv), void *priv)
{
}

void sound_set_pc_speaker_filter(void (*filter)(int channel, double *buffer, void *priv), void *priv)
{
	filter_pc_speaker = filter;
	filter_pc_speaker_p = priv;
}

void sound_set_midi_filter(void (*filter)(int channel, double *buffer, void *priv), void *priv)
{
}

int al_capture_get_rate(void)
{
	return SOUND_FREQ;
}

/* Sound Blaster variants WinUAE doesn't offer: AWE32 EMU8000, Aztech, ESS,
   ISA PnP, IDE and the game port. */

void emu8k_init(emu8k_t *emu8k, uint16_t emu_addr, int onboard_ram)
{
}

void emu8k_close(emu8k_t *emu8k)
{
}

void emu8k_update(emu8k_t *emu8k)
{
}

void emu8k_change_addr(emu8k_t *emu8k, uint16_t emu_addr)
{
}

void emu8k_reset_buffer(emu8k_t *emu8k)
{
}

void azt2316a_enable_wss(uint8_t enable, void *priv)
{
}

void azt2320_enable_wss(uint8_t enable, void *priv)
{
}

void azt1605_update_mixer(void *priv)
{
}

void aztpr16_update_mixer(void *priv)
{
}

void aztpr16_wss_mode(uint8_t mode, void *priv)
{
}

void *isapnp_add_card(uint8_t *rom, uint16_t rom_size,
	void (*config_changed)(uint8_t ld, isapnp_device_config_t *config, void *priv),
	void (*csn_changed)(uint8_t csn, void *priv),
	uint8_t (*read_vendor_reg)(uint8_t ld, uint8_t reg, void *priv),
	void (*write_vendor_reg)(uint8_t ld, uint8_t reg, uint8_t val, void *priv),
	void *priv)
{
	return NULL;
}

uint8_t isapnp_read_reg(void *priv, uint8_t ldn, uint8_t reg)
{
	return 0xff;
}

void isapnp_write_reg(void *priv, uint8_t ldn, uint8_t reg, uint8_t val)
{
}

void ide_set_irq(int board, int irq)
{
}

void ide_set_base_addr(int board, int base, uint16_t port)
{
}

void ide_handlers(uint8_t board, int set)
{
}

void ide_pnp_config_changed(uint8_t ld, isapnp_device_config_t *config, void *priv)
{
}

void ide_pnp_config_changed_1addr(uint8_t ld, isapnp_device_config_t *config, void *priv)
{
}

uint32_t ide_boards_quaternary(const device_t *dev)
{
	return 0;
}

void *gameport_add(const device_t *gameport_type)
{
	return NULL;
}

void gameport_remap(void *priv, uint16_t address)
{
}

#define STUB_DEVICE(n, desc) const device_t n = { .name = desc, .internal_name = #n }

STUB_DEVICE(gameport_device, "Game port");
STUB_DEVICE(gameport_200_device, "Game port");
STUB_DEVICE(gameport_pnp_device, "Game port");
STUB_DEVICE(gameport_pnp_1io_device, "Game port");
STUB_DEVICE(ide_qua_pnp_device, "IDE");
STUB_DEVICE(keyboard_pc_xt_device, "PC/XT Keyboard");
STUB_DEVICE(mouse_upc_standalone_device, "UPC mouse");

/* Serial ports */

extern void serial_write(uint16_t addr, uint8_t val, void *priv);
void (*serial_irq_hook)(int irq, int set);

void *x86pc_add_serial(int port)
{
	com_ports[port].enabled = 1;
	serial_set_next_inst(port);
	return device_add_inst(&ns16550_device, port + 1);
}

uint8_t x86pc_serial_read(void *serial, uint16_t reg)
{
	return serial_read(reg, serial);
}

void x86pc_serial_write(void *serial, uint16_t reg, uint8_t val)
{
	serial_write(reg, val, serial);
}

void *x86pc_add_serial_mouse(int port)
{
	serial_mouse_port = port;
	return device_add(&mouse_msserial_device);
}

void x86pc_set_serial_irq_hook(void (*hook)(int irq, int set))
{
	serial_irq_hook = hook;
}

/* No serial port pass-through: the ports only talk to the emulated mice. */

const device_t *char_get_device(const int id)
{
	return NULL;
}

void *char_init(char_port_t *port, const device_t *device, int instance)
{
	return NULL;
}

/* No PCI or MCA on the bridgeboards */

uint8_t pci_read(uint16_t port, void *priv)
{
	return 0xff;
}

uint16_t pci_readw(uint16_t port, void *priv)
{
	return 0xffff;
}

uint32_t pci_readl(uint16_t port, void *priv)
{
	return 0xffffffff;
}

void pci_write(uint16_t port, uint8_t val, void *priv)
{
}

void pci_writew(uint16_t port, uint16_t val, void *priv)
{
}

void pci_writel(uint16_t port, uint32_t val, void *priv)
{
}

void pci_reset(void)
{
}

uint8_t mca_add(uint8_t (*read)(uint16_t port, void *priv), void (*write)(uint16_t port, uint8_t val, void *priv), uint8_t (*feedb)(void *priv), void (*reset)(void *priv), void *priv)
{
	return 0;
}

/* No local APIC or IOAPIC: the bridgeboard CPUs are 8088 to 486. The
   answers are the ones 86Box's own code gives without an APIC. */

void cpu_lapic_reset_all(void)
{
}

void cpu_lapic_ven_reset(void)
{
}

int cpu_apic_feature_enabled(void)
{
	return 0;
}

uint8_t cpu_lapic_index_source(void)
{
	return 0;
}

uint32_t cpu_cpuid_apic_id_ebx(void)
{
	return 0;
}

void lapic_set_index_source(uint8_t (*source)(void))
{
}

void lapic_index_board_reset(uint8_t index)
{
}

void ioapic_pic_intr(int active)
{
}

void ioapic_legacy_irq(uint16_t mask, int level, int set)
{
}

int ioapic_lint1_raise(void)
{
	return 0;
}

int ioapic_pic_fallback_allowed(void)
{
	return 1;
}

void ioapic_processor_reset(void)
{
}

uint8_t ioapic_irq_path_active;
uint32_t cpu_lapic_irr_live;
uint8_t cpu_lapic_extint_live[IOAPIC_STATE_MAX_LAPICS];

int ioapic_external_interrupt_pending(void)
{
	return 0;
}

int ioapic_external_interrupt_vector(void)
{
	return -1;
}

int cpu_lapic_highest_deliverable_irr_index(uint8_t lapic_index, int started)
{
	return -1;
}

uint64_t cpu_apic_base_msr_read_current(void)
{
	return 0;
}

void cpu_apic_base_msr_write_current(uint64_t val)
{
}

/* Hooks into machines and video cards that WinUAE doesn't emulate */

void startblit(void)
{
}

void endblit(void)
{
}

void video_update_timing(void)
{
}

int video_is_cga(void)
{
	return 0;
}

int video_is_mda(void)
{
	return 0;
}

unsigned video_get_wait_states(uint32_t address, int write, unsigned size, uint64_t cpu_cycle)
{
	return 0;
}

int fdd_get_flags(void *priv)
{
	return 0;
}

void pc_cas_set_out(pc_cassette_t *cas, unsigned char val)
{
}

void ps2_cache_clean(void)
{
}

void pcjr_waitstates(void *priv)
{
}

void fdc37mx0x_watchdog_reset_ext(int origin)
{
}

void rivatimer_init(void)
{
}

void ibm5140_clock_wake(void)
{
}

int i8018x_pcb_icu_int_type(void)
{
	return 0;
}

int i8018x_pcb_icu_pending(void)
{
	return 0;
}

uint8_t t3100e_config_get(void)
{
	return 0;
}

void t3100e_display_set(uint8_t value)
{
}

uint8_t t3100e_mono_get(void)
{
	return 0;
}

void t3100e_mono_set(uint8_t value)
{
}

void t3100e_notify_set(uint8_t value)
{
}

void t3100e_turbo_set(uint8_t value)
{
}

uint8_t xi8088_turbo_get(void)
{
	return 0;
}

void xi8088_turbo_set(uint8_t value)
{
}
