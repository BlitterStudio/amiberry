/*
* UAE - The Un*x Amiga Emulator
*
* Interface between WinUAE and the imported 86Box code.
*/

#ifndef WINUAE_86BOX_GLUE_H
#define WINUAE_86BOX_GLUE_H

#include "86box_msvc.h"

#include <stdint.h>

/* WinUAE's options.h has its own config_changed, 86Box's isn't used */
#define config_changed x86box_config_changed

/* PC models the bridgeboards and ATonce are built as */
enum {
	X86PC_XT,		/* 8088 PC/XT (A1060, A2088, A2088T) */
	X86PC_AT286,	/* 286 AT (A2286, ATonce) */
	X86PC_AT386SX	/* 386SX AT (A2386SX) */
};

/* Select the PC model and its default CPU, cpu_index into that model's
   CPU list. */
void x86pc_set_machine(int type, int cpu_index);
/* Select a CPU of the current model by the name WinUAE shows for it
   ("i386SX/25", ...), returns the FPU type that goes with the CPU or -1
   if no CPU has that name. */
int x86pc_select_cpu(const char *name, int fpu_type);
/* Speed adjustment in 1/1000 steps, as set by x86_speed_throttle. */
void x86pc_set_speed_throttle(int throttle);
/* Switch between the base (4.77MHz/6MHz/...) and the selected speed. */
void x86pc_set_turbo(int turbo);
/* Clock of the running CPU in Hz. */
int x86pc_get_cpu_speed(void);
/* Free the PC's RAM. */
void x86pc_mem_free(void);

/* Report the keyboard controller as PS/2 capable (PS/2 mouse configured). */
void x86pc_set_ps2(bool ps2);

/* IRQ 0 from PIT counter 0 */
void pit_irq0_timer(int new_out, int old_out, void *priv);

/* Start 86Box's timers. */
void x86pc_timer_init(void);

/* Set up the x86 recompiler if the selected CPU uses it, after mem_init(). */
void x86pc_codegen_init(void);

/* Remove and close all devices added through device_add(). */
void x86pc_close_devices(void);

/* WinUAE's settings for 86Box device options, in x86.cpp. Returns false
   for options that keep the device's default. dev is the device whose
   init function is running, or NULL. */
struct _device_;
bool x86_device_config(const struct _device_ *dev, const char *name, int *value);

/* 16550 UART on COM port 'port' (0 = COM1), returns its serial_t. */
void *x86pc_add_serial(int port);
uint8_t x86pc_serial_read(void *serial, uint16_t reg);
void x86pc_serial_write(void *serial, uint16_t reg, uint8_t val);
/* Microsoft serial mouse on COM port 'port'. */
void *x86pc_add_serial_mouse(int port);
/* Send the UART interrupts to WinUAE instead of the PC's PIC (DraCo). */
void x86pc_set_serial_irq_hook(void (*hook)(int irq, int set));

/* Run the 86Box timers without an x86 CPU (DraCo), cycles at the CPU clock
   x86pc_get_cpu_speed() returns. */
void x86pc_timers_init(void);
void x86pc_timers_run(int n);

/* Mouse movement and buttons for the 86Box serial and PS/2 mice. */
void x86pc_mouse_input(int dx, int dy, int dz, int buttons);

#endif /* WINUAE_86BOX_GLUE_H */
