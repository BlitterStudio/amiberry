
#include "stdlib.h"
#include "stdint.h"

#include "dsp3210/DSP3210_emulation.h"

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "devices.h"
#include "uae.h"
#include "autoconf.h"
#include "debug.h"
#include "newcpu.h"
#include "threaddep/thread.h"
#include "dsp_glue.h"

#define DSP_INST_COUNT_WAIT 1000

int log_dsp = 0;
static volatile uae_u8 dsp_status;
bool is_dsp_installed;
static volatile int dsp_thread_running;
static uae_sem_t reset_sem, pause_sem;
static volatile uae_atomic dsp_int;
static volatile uae_atomic dsp_int_ext;
static int dsp_int0_delay, dsp_int1_delay;
static volatile bool dsp_running;
static volatile uae_u32 dsp_inst_cnt;
static bool dsp_paused;

static bool dsp_addr_check(uint32_t addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if ((ab->flags & ABFLAG_THREADSAFE) == 0 && ab != &dummy_bank) {
		write_log("DSP accesses non-threadsafe address %08x!\n", addr);
		return false;
	}
	return true;
}
static uint32_t safe_memory_read(uint32_t addr, int size)
{
	return 0;
}
static void safe_memory_write(uint32_t addr, uint32_t data, int size)
{
}

uint32_t DSP_get_long_ext(uint32_t addr)
{
	int32_t v;
	if (dsp_addr_check(addr)) {
		v = get_long(addr);
	} else {
		v = safe_memory_read(addr, 4);
	}
	v = do_byteswap_32(v);
	if (log_dsp) {
		write_log("DSP LGET %08x = %08x\n", addr, v);
	}
	return v;
}

uint16_t DSP_get_short_ext(uint32_t addr)
{
	uint16_t v;
	if (dsp_addr_check(addr)) {
		v = get_word(addr);
	} else {
		v = safe_memory_read(addr, 2);
	}
	v = do_byteswap_16(v);
	if (log_dsp) {
		write_log("DSP WGET %08x = %04x\n", addr, v & 0xffff);
	}
	return v;
}

unsigned char DSP_get_char_ext(uint32_t addr)
{
	unsigned char v;
	if (dsp_addr_check(addr)) {
		v = get_byte(addr);
	} else {
		v = safe_memory_read(addr, 1);
	}
	if (log_dsp) {
		write_log("DSP BGET %08x = %02x\n", addr, v & 0xff);
	}
	return v;
}

void DSP_set_long_ext(uint32_t addr, uint32_t v)
{
	v = do_byteswap_32(v);
	if (log_dsp) {
		write_log("DSP LPUT %08x = %08x\n", addr, v);
	}
	if (dsp_addr_check(addr)) {
		put_long(addr, v);
	} else {
		safe_memory_write(addr, v, 4);
	}
}

void DSP_set_short_ext(uint32_t addr, uint16_t v)
{
	v = do_byteswap_16(v);
	if (log_dsp) {
		write_log("DSP WPUT %08x = %04x\n", addr, v & 0xffff);
	}
	if (dsp_addr_check(addr)) {
		put_word(addr, v);
	} else {
		safe_memory_write(addr, v, 2);
	}
}

void DSP_set_char_ext(uint32_t addr, unsigned char v)
{
	if (log_dsp) {
		write_log("DSP BPUT %08x = %02x\n", addr, v & 0xff);
	}
	if (dsp_addr_check(addr)) {
		put_byte(addr, v);
	} else {
		safe_memory_write(addr, v, 1);
	}
}

static void dsp_status_intx(void)
{
	dsp_status &= ~(1 | 2 | 4 | 8);
	dsp_status |= dsp_int;
	dsp_status |= dsp_int_ext;
}

static void dsp_rethink(void)
{
	dsp_status_intx();
	if ((dsp_status & 0x20) && !(dsp_status & 8)) {
		if (log_dsp) {
			write_log("DSP INTREQ level 6\n");
		}
		INTREQ_0(0x8000 | 0x2000);
	}
	if ((dsp_status & 0x10) && !(dsp_status & 4)) {
		if (log_dsp) {
			write_log("DSP INTREQ level 2\n");
		}
		INTREQ_0(0x8000 | 0x0008);
	}
}

static void dsp_reset(void)
{
	dsp_int = 0x01 | 0x02;
	dsp_int_ext = 0x04 | 0x08;
}

static void cpu_wait(void)
{
	while (dsp_inst_cnt < DSP_INST_COUNT_WAIT && dsp_thread_running > 0) {
		x_do_cycles(CYCLE_UNIT * 8);
	}
}

// stop CPU for a while if DSP was not waiting for interrupt
static void dsp_do_int(int i)
{
	int mask = ~(1 << i);
	if (log_dsp) {
		write_log("DSP INT%d\n", i);
	}
	// stop CPU if DSP is not waiting for interrupt
	if (DSP_irsh_flag != 4) {
		dsp_inst_cnt = 0;
		atomic_and(&dsp_int, mask);
		cpu_wait();
	} else {
		atomic_and(&dsp_int, mask);
	}
}

void dsp_write(uae_u8 v)
{
	dsp_status_intx();
	if (log_dsp) {
		write_log("DSP write %02x (%02x) PC=%08x\n", v, dsp_status, M68K_GETPC);
	}
	if (!(dsp_status & 0x80) && (v & 0x80)) {
		dsp_status |= 0x80;
		write_log("DSP reset released\n");
		dsp_inst_cnt = 0;
		uae_sem_post(&reset_sem);
		cpu_wait();
		write_log("DSP reset delay end\n");
	} else if ((dsp_status & 0x80) && !(v & 0x80)) {
		write_log("DSP reset activated\n");
		dsp_status &= ~0x80;
		dsp_reset();
	}
	if ((dsp_status & 1) && !(v & 1)) {
		dsp_do_int(0);
	} else if (v & 1) {
		atomic_or(&dsp_int, 1);
	}
	if ((dsp_status & 2) && !(v & 2)) {
		dsp_do_int(1);
	} else if (v & 2) {
		atomic_or(&dsp_int, 2);
	}
	dsp_status = (v & (0x40 | 0x20 | 0x10)) | (dsp_status & 0x80);
	dsp_status_intx();
	if (log_dsp) {
		write_log("-> %02x\n", dsp_status);
	}
}

uae_u8 dsp_read(void)
{
	dsp_status_intx();
	uae_u8 v = dsp_status;
	if (log_dsp) {
		write_log("DSP read %02x\n", v);
	}
	return v;
}

void DSP_external_interrupt(int32_t level, int32_t state)
{
	int32_t mask = level ? 0x08 : 0x04;
	if (state) {
		if (log_dsp && !(dsp_status & mask)) {
			write_log("DSP interrupt %d = %d\n", level, state);
		}
		atomic_and(&dsp_int_ext, ~mask);
	} else {
		if (log_dsp && (dsp_status & mask)) {
			write_log("DSP interrupt %d = %d\n", level, state);
		}
		atomic_or(&dsp_int_ext, mask);
	}
	atomic_or(&uae_int_requested, 0x010000);
}

static int dsp_thread(void *v)
{
	dsp_thread_running = 1;
	DSP_init_emu();
	dsp_reset();
	dsp_int0_delay = 0;
	dsp_int1_delay = 0;

	while (dsp_thread_running > 0) {
		if (!(dsp_status & 0x80)) {
			uae_sem_wait(&reset_sem);
			dsp_reset();
			DSP_reset();
			dsp_inst_cnt = 0;
		}
		if (dsp_paused) {
			uae_sem_wait(&pause_sem);
			continue;
		}
		if (dsp_status & 0x80) {
			DSP_execute_insn();
			dsp_inst_cnt++;
			if (!(dsp_int & 1)) {
				dsp_int0_delay++;
				if (DSP_int0_masked()) {
					dsp_int0_delay = 0;
					if (log_dsp) {
						write_log("DSP interrupt 0 masked\n");
					}
				}
				if (dsp_int0_delay > 2) {
					if (DSP_int0()) {
						if (log_dsp) {
							write_log(_T("DSP interrupt 0 started\n"));
						}
					}
					atomic_or(&dsp_int, 1);
					dsp_int0_delay = 0;
				}
			}
			if (!(dsp_int & 2)) {
				dsp_int1_delay++;
				if (DSP_int1_masked()) {
					dsp_int1_delay = 0;
					if (log_dsp) {
						write_log("DSP interrupt 1 masked\n");
					}
				}
				if (dsp_int1_delay > 2) {
					if (DSP_int1()) {
						if (log_dsp) {
							write_log(_T("DSP interrupt 1 started\n"));
						}
					}
					atomic_or(&dsp_int, 2);
					dsp_int1_delay = 0;
				}
			}
		}
	}
	DSP_shutdown_emu();
	dsp_status = 0;
	dsp_thread_running = 0;
	return 0;
}

static void dsp_free(void)
{
	write_log("DSP free\n");
	if (dsp_thread_running > 0) {
		dsp_status &= ~0x80;
		dsp_thread_running = -1;
		uae_sem_post(&reset_sem);
		uae_sem_post(&pause_sem);
		while (dsp_thread_running) {
			sleep_millis(2);
		}
		uae_sem_destroy(&reset_sem);
	}
	is_dsp_installed = false;
}

bool dsp_init(struct autoconfig_info *aci)
{
	aci->start = 0xdd0000;
	aci->size = 0x10000;
	dsp_status = 0;
	dsp_reset();
	dsp_status_intx();
	if (aci->doinit) {
		is_dsp_installed = true;
		write_log("DSP init\n");
		device_add_rethink(dsp_rethink);
		device_add_exit(NULL, dsp_free);
		uae_sem_init(&reset_sem, 0, 0);
		uae_sem_init(&pause_sem, 0, 0);
		uae_start_thread(NULL, dsp_thread, NULL, NULL);
	}
	return true;
}

void dsp_pause(int pause)
{
	if (is_dsp_installed) {
		dsp_paused = pause;
		uae_sem_post(&pause_sem);
	}
}
