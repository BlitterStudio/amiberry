/*
* UAE - The Un*x Amiga Emulator
*
* The PCs inside the bridgeboards and ATonce, described the way 86Box
* describes its machines and CPUs. The CPU timings are 86Box's own.
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "86box_glue.h"

#include <86box/86box.h>
#include <86box/device.h>
#include <86box/timer.h>
#include <86box/mem.h>
#include <86box/pic.h>
#include <86box/pit.h>
#include <86box/keyboard.h>
#include <86box/nvr.h>
#include <86box/machine.h>
#include <86box/plat.h>
#include "86box/cpu/cpu.h"

extern uint8_t x86_get_jumpers(void);

FPU fpus_none[] = {
	{ .name = "None", .internal_name = "none", .type = FPU_NONE },
	{ .name = NULL, .internal_name = NULL, .type = 0 }
};
FPU fpus_8088[] = {
	{ .name = "None", .internal_name = "none", .type = FPU_NONE },
	{ .name = "8087", .internal_name = "8087", .type = FPU_8087 },
	{ .name = NULL, .internal_name = NULL, .type = 0 }
};
FPU fpus_80286[] = {
	{ .name = "None", .internal_name = "none", .type = FPU_NONE },
	{ .name = "287", .internal_name = "287", .type = FPU_287 },
	{ .name = "287XL", .internal_name = "287xl", .type = FPU_287XL },
	{ .name = NULL, .internal_name = NULL, .type = 0 }
};
FPU fpus_80386[] = {
	{ .name = "None", .internal_name = "none", .type = FPU_NONE },
	{ .name = "387", .internal_name = "387", .type = FPU_387 },
	{ .name = NULL, .internal_name = NULL, .type = 0 }
};
FPU fpus_486sx[] = {
	{ .name = "None", .internal_name = "none", .type = FPU_NONE },
	{ .name = "487SX", .internal_name = "487sx", .type = FPU_487SX },
	{ .name = NULL, .internal_name = NULL, .type = 0 }
};
FPU fpus_internal[] = {
	{ .name = "Internal", .internal_name = "internal", .type = FPU_INTERNAL },
	{ .name = NULL, .internal_name = NULL, .type = 0 }
};

#define CPU_ENTRY(n, t, f, speed, mul, edx, flags, mr, mw, cr, cw, atdiv) \
	{ .name = n, .cpu_type = t, .fpus = f, .rspeed = speed, .multi = mul, .voltage = 5000, \
	  .edx_reset = edx, .cpuid_model = 0, .cyrix_id = 0, .cpu_flags = flags, \
	  .mem_read_cycles = mr, .mem_write_cycles = mw, .cache_read_cycles = cr, .cache_write_cycles = cw, \
	  .atclk_div = atdiv }
#define CPU_END { .name = "", .cpu_type = 0 }

/* Not const: the speed throttle adjusts the clock of the selected CPU */
static CPU cpus_8088[] = {
	CPU_ENTRY("4.77", CPU_8088, fpus_8088, 4772728, 1, 0, 0, 0, 0, 0, 0, 1),
	CPU_ENTRY("7.16", CPU_8088, fpus_8088, 7159092, 1, 0, 0, 0, 0, 0, 0, 1),
	CPU_ENTRY("8", CPU_8088, fpus_8088, 8000000, 1, 0, 0, 0, 0, 0, 0, 1),
	CPU_ENTRY("10", CPU_8088, fpus_8088, 10000000, 1, 0, 0, 0, 0, 0, 0, 1),
	CPU_ENTRY("12", CPU_8088, fpus_8088, 12000000, 1, 0, 0, 0, 0, 0, 0, 1),
	CPU_ENTRY("16", CPU_8088, fpus_8088, 16000000, 1, 0, 0, 0, 0, 0, 0, 1),
	CPU_END
};
static CPU cpus_286[] = {
	CPU_ENTRY("6", CPU_286, fpus_80286, 6000000, 1, 0, 0, 2, 2, 2, 2, 1),
	CPU_ENTRY("8", CPU_286, fpus_80286, 8000000, 1, 0, 0, 2, 2, 2, 2, 1),
	CPU_ENTRY("10", CPU_286, fpus_80286, 10000000, 1, 0, 0, 2, 2, 2, 2, 1),
	CPU_ENTRY("12", CPU_286, fpus_80286, 12500000, 1, 0, 0, 3, 3, 3, 3, 2),
	CPU_ENTRY("16", CPU_286, fpus_80286, 16000000, 1, 0, 0, 3, 3, 3, 3, 2),
	CPU_ENTRY("20", CPU_286, fpus_80286, 20000000, 1, 0, 0, 4, 4, 4, 4, 3),
	CPU_ENTRY("25", CPU_286, fpus_80286, 25000000, 1, 0, 0, 4, 4, 4, 4, 3),
	CPU_END
};
static CPU cpus_i386sx[] = {
	CPU_ENTRY("16", CPU_386SX, fpus_80386, 16000000, 1, 0x2308, CPU_SUPPORTS_DYNAREC, 3, 3, 3, 3, 2),
	CPU_ENTRY("20", CPU_386SX, fpus_80386, 20000000, 1, 0x2308, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3),
	CPU_ENTRY("25", CPU_386SX, fpus_80386, 25000000, 1, 0x2308, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3),
	CPU_ENTRY("33", CPU_386SX, fpus_80386, 33333333, 1, 0x2308, CPU_SUPPORTS_DYNAREC, 6, 6, 3, 3, 4),
	CPU_ENTRY("40", CPU_386SX, fpus_80386, 40000000, 1, 0x2308, CPU_SUPPORTS_DYNAREC, 7, 7, 3, 3, 5),
	CPU_END
};
static CPU cpus_i386dx[] = {
	CPU_ENTRY("16", CPU_386DX, fpus_80386, 16000000, 1, 0x0308, CPU_SUPPORTS_DYNAREC, 3, 3, 2, 2, 2),
	CPU_ENTRY("20", CPU_386DX, fpus_80386, 20000000, 1, 0x0308, CPU_SUPPORTS_DYNAREC, 4, 4, 2, 2, 3),
	CPU_ENTRY("25", CPU_386DX, fpus_80386, 25000000, 1, 0x0308, CPU_SUPPORTS_DYNAREC, 4, 4, 2, 2, 3),
	CPU_ENTRY("33", CPU_386DX, fpus_80386, 33333333, 1, 0x0308, CPU_SUPPORTS_DYNAREC, 6, 6, 2, 2, 4),
	CPU_ENTRY("40", CPU_386DX, fpus_80386, 40000000, 1, 0x0308, CPU_SUPPORTS_DYNAREC, 7, 7, 2, 2, 5),
	CPU_END
};
static CPU cpus_i486sx[] = {
	CPU_ENTRY("16", CPU_i486SX, fpus_486sx, 16000000, 1, 0x420, CPU_SUPPORTS_DYNAREC, 3, 3, 3, 3, 2),
	CPU_ENTRY("20", CPU_i486SX, fpus_486sx, 20000000, 1, 0x420, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3),
	CPU_ENTRY("25", CPU_i486SX, fpus_486sx, 25000000, 1, 0x422, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3),
	CPU_ENTRY("33", CPU_i486SX, fpus_486sx, 33333333, 1, 0x422, CPU_SUPPORTS_DYNAREC, 6, 6, 3, 3, 4),
	CPU_END
};
static CPU cpus_i486dx[] = {
	CPU_ENTRY("25", CPU_i486DX, fpus_internal, 25000000, 1, 0x404, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3),
	CPU_ENTRY("33", CPU_i486DX, fpus_internal, 33333333, 1, 0x404, CPU_SUPPORTS_DYNAREC, 6, 6, 3, 3, 4),
	CPU_ENTRY("50", CPU_i486DX, fpus_internal, 50000000, 1, 0x411, CPU_SUPPORTS_DYNAREC, 8, 8, 4, 4, 6),
	CPU_END
};
static CPU cpus_i486dx2[] = {
	CPU_ENTRY("40", CPU_i486DX, fpus_internal, 40000000, 2, 0x430, CPU_SUPPORTS_DYNAREC, 7, 7, 6, 6, 5),
	CPU_ENTRY("50", CPU_i486DX, fpus_internal, 50000000, 2, 0x433, CPU_SUPPORTS_DYNAREC, 8, 8, 6, 6, 6),
	CPU_ENTRY("66", CPU_i486DX, fpus_internal, 66666666, 2, 0x433, CPU_SUPPORTS_DYNAREC, 12, 12, 6, 6, 8),
	CPU_END
};

#define FAMILY(p, n, c) { .package = p, .manufacturer = "Intel", .name = n, .internal_name = n, .cpus = c }

const cpu_family_t cpu_families[] = {
	FAMILY(CPU_PKG_8088, "8088", cpus_8088),
	FAMILY(CPU_PKG_286, "286", cpus_286),
	FAMILY(CPU_PKG_386SX, "i386SX", cpus_i386sx),
	FAMILY(CPU_PKG_386DX, "i386DX", cpus_i386dx),
	FAMILY(CPU_PKG_SOCKET1, "i486SX", cpus_i486sx),
	FAMILY(CPU_PKG_SOCKET1, "i486DX", cpus_i486dx),
	FAMILY(CPU_PKG_SOCKET1, "i486DX2", cpus_i486dx2),
	{ .package = 0, .manufacturer = "", .name = "", .internal_name = "", .cpus = NULL }
};

/* The families each PC model offers, the first one is its default */
static const int model_families[][8] = {
	{ 0, -1 },
	{ 1, -1 },
	{ 2, 3, 4, 5, 6, -1 }
};

static uint8_t bridge_p1_handler(void)
{
	return x86_get_jumpers();
}

#define MACHINE(n, iname, t, pkg, bus, ramax, nvrm, kbc, nvr, nvrparams) { \
	.name = n, .internal_name = iname, .type = t, .chipset = MACHINE_CHIPSET_DISCRETE, \
	.init = NULL, .p1_handler = bridge_p1_handler, .gpio_handler = NULL, \
	.available_flag = (uintptr_t) MACHINE_AVAILABLE, .gpio_acpi_handler = NULL, \
	.cpu = { .package = pkg, .block = CPU_BLOCK_NONE, .min_bus = 0, .max_bus = 0, \
		.min_voltage = 0, .max_voltage = 0, .min_multi = 0, .max_multi = 0 }, \
	.cpu_io_cycles = 0, .bus_flags = bus, .flags = MACHINE_FLAGS_NONE, \
	.ram = { .min = 128, .max = ramax, .step = 128 }, .ram_granularity = 0, \
	.nvrmask = nvrm, .display_aspect_x = 0, .display_aspect_y = 0, \
	.jumpered_ecp_dma = 0, .default_jumpered_ecp_dma = -1, \
	.kbc_device = kbc, .kbc_params = KBC_VEN_GENERIC, \
	.nvr_device = nvr, .nvr_params = (uintptr_t) (nvrparams), \
	.sio_device = NULL, .sio_params = 0, .kbc_p1 = 0xff, .gpio = 0xffffffff, .gpio_acpi = 0xffffffff }

const machine_t machines[] = {
	MACHINE("Bridgeboard XT", "bridgeboard_xt", MACHINE_TYPE_8088, CPU_PKG_8088, MACHINE_PC, 640, 0, NULL, NULL, 0),
	MACHINE("Bridgeboard 286", "bridgeboard_286", MACHINE_TYPE_286, CPU_PKG_286, MACHINE_AT, 16384, 127, &kbc_at_device, &nvr_at_device, NVR_AT),
	MACHINE("Bridgeboard 386SX", "bridgeboard_386sx", MACHINE_TYPE_386SX, CPU_PKG_386SX | CPU_PKG_386DX | CPU_PKG_SOCKET1, MACHINE_AT, 16384, 255, &kbc_at_device, &nvr_at_device, NVR_AT),
	{ .name = NULL }
};

static int cpu_family, cpu_index, cpu_throttle;
static bool ps2_kbc;
static int cpu_base_speed;

int machine_has_bus(int m, uintptr_t bus_flags)
{
	return (machines[m].bus_flags & bus_flags) ? 1 : 0;
}

static uintptr_t machine_flags(int m)
{
	return machines[m].flags | (ps2_kbc ? MACHINE_PS2_KBC : 0);
}

int machine_has_flags(int m, uintptr_t flags)
{
	return (machine_flags(m) & flags) ? 1 : 0;
}

uintptr_t machine_has_flags_64(int m, uintptr_t flags)
{
	return machine_flags(m) & flags;
}

void x86pc_set_ps2(bool ps2)
{
	ps2_kbc = ps2;
}

int machine_has_flags_ex(uintptr_t flags)
{
	return machine_has_flags(machine, flags);
}

const char *machine_get_internal_name(void)
{
	return machines[machine].internal_name;
}

const char *machine_get_nvr_name(void)
{
	return machines[machine].internal_name;
}

uint8_t machine_get_p1(uint8_t kbc_p1)
{
	uint8_t ret = 0xff;
	if (machines[machine].p1_handler) {
		ret = machines[machine].p1_handler();
	}
	return ret & kbc_p1;
}

void pit_irq0_timer(int new_out, int old_out, void *priv)
{
	if (new_out != old_out) {
		// Track the PIT output wire so PIC initialization sees it held high.
		uint8_t irq_state = old_out != 0;
		picint_common(1, PIC_IRQ_LEVEL, new_out != 0, &irq_state);
	}
}

void pc_speed_changed(void)
{
	if (cpu_s) {
		if (cpu_s->cpu_type >= CPU_286) {
			pit_set_clock(cpu_s->rspeed);
		} else {
			pit_set_clock((uint32_t)14318184.0);
		}
	}
	device_speed_changed();
}

/* Machine init functions some 86Box devices check for special cases. None
   of them is the machine WinUAE runs. */
#define NOT_THIS_MACHINE(n) int n(const machine_t *model) { return 0; }

NOT_THIS_MACHINE(machine_at_acera1g_init)
NOT_THIS_MACHINE(machine_at_advantage40xxd_init)
NOT_THIS_MACHINE(machine_at_bx6_init)
NOT_THIS_MACHINE(machine_at_cobalt3k_init)
NOT_THIS_MACHINE(machine_at_nupro592_init)
NOT_THIS_MACHINE(machine_at_pb410a_init)
NOT_THIS_MACHINE(machine_at_pl4600c_init)
NOT_THIS_MACHINE(machine_at_sb486pv_init)
NOT_THIS_MACHINE(machine_at_spc7700plw_init)
NOT_THIS_MACHINE(machine_at_spitfire_init)
NOT_THIS_MACHINE(machine_at_tandy_vis_init)
NOT_THIS_MACHINE(machine_at_wd76c10_init)
NOT_THIS_MACHINE(machine_xt_ibm5535_init)
NOT_THIS_MACHINE(machine_xt_ibm5550_init)
NOT_THIS_MACHINE(machine_xt_xi8088_init)

static CPU *selected_cpu(void)
{
	return (CPU *)&cpu_families[cpu_family].cpus[cpu_index];
}

static void apply_throttle(void)
{
	CPU *c = selected_cpu();
	c->rspeed = cpu_base_speed;
	if (cpu_throttle > 0) {
		c->rspeed = cpu_base_speed + (int32_t)((int64_t)cpu_base_speed * cpu_throttle / 1000);
	}
}

static void select_cpu(int family, int index)
{
	if (cpu_base_speed) {
		/* restore the clock of the previously selected CPU */
		selected_cpu()->rspeed = cpu_base_speed;
	}
	cpu_family = family;
	cpu_index = index;
	cpu_base_speed = selected_cpu()->rspeed;
	cpu_f = (cpu_family_t *)&cpu_families[cpu_family];
	cpu = cpu_index;
#ifdef USE_DYNAREC
	/* 86Box recompiles 386 and later CPUs (the 386s use its 486 timing) */
	cpu_use_dynarec = (selected_cpu()->cpu_flags & CPU_SUPPORTS_DYNAREC) != 0;
#endif
	apply_throttle();
}

void x86pc_set_machine(int type, int index)
{
	machine = type;
	select_cpu(model_families[type][0], index);
}

int x86pc_select_cpu(const char *name, int fpu)
{
	int ret = -1;
	for (int i = 0; model_families[machine][i] >= 0; i++) {
		const cpu_family_t *f = &cpu_families[model_families[machine][i]];
		for (int j = 0; f->cpus[j].cpu_type; j++) {
			const CPU *c = &f->cpus[j];
			char cpuname[64];
			snprintf(cpuname, sizeof cpuname, "%s/%s", f->name, c->name);
			if (!strstr(name, cpuname)) {
				continue;
			}
			select_cpu(model_families[machine][i], j);
			if (c->cpu_type == CPU_i486SX) {
				ret = FPU_NONE;
			} else if (c->cpu_type == CPU_i486DX) {
				ret = FPU_INTERNAL;
			} else if (fpu == FPU_NONE) {
				ret = FPU_NONE;
			} else {
				ret = c->fpus[1].type;
			}
			write_log(_T("CPU override = %s\n"), cpuname);
		}
	}
	return ret;
}

void x86pc_set_speed_throttle(int throttle)
{
	cpu_throttle = throttle;
	apply_throttle();
	cpu_set();
	pc_speed_changed();
}

void x86pc_set_turbo(int turbo)
{
	cpu_dynamic_switch(turbo ? cpu_index : 0);
}

int x86pc_get_cpu_speed(void)
{
	return cpu_s ? cpu_s->rspeed : 0;
}

void x86pc_timers_init(void)
{
	x86pc_close_devices();
	x86pc_set_machine(X86PC_AT286, 1);
	cpu_set();
	pc_speed_changed();
	x86pc_timer_init();
}

void x86pc_timers_run(int n)
{
	tsc += n;
	if (TIMER_VAL_LESS_THAN_VAL(timer_target, tsc)) {
		timer_process();
	}
}

void x86pc_mem_free(void)
{
	mem_close();
	plat_munmap(ram, (size_t)mem_size * 1024 + 16);
	ram = NULL;
}
