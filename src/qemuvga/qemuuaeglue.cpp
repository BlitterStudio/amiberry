
#include "sysconfig.h"
#include "sysdeps.h"

#include "qemuuaeglue.h"
#include "vga_int.h"


void memory_region_transaction_begin(void)
{
}
void memory_region_transaction_commit(void)
{
}
void memory_region_add_subregion(MemoryRegion *mr,
                                 hwaddr offset,
                                 MemoryRegion *subregion)
{
}
void memory_region_add_subregion_overlap(MemoryRegion *mr,
                                         hwaddr offset,
                                         MemoryRegion *subregion,
                                         unsigned priority)
{
}
void memory_region_del_subregion(MemoryRegion *mr,
                                 MemoryRegion *subregion)
{
}
void memory_region_destroy(MemoryRegion *mr)
{
}
void memory_region_set_log(MemoryRegion *mr, bool log, unsigned client)
{
}
void memory_region_init(MemoryRegion *mr,
                        const char *name,
                        uint64_t size)
{
}
void memory_region_set_flush_coalesced(MemoryRegion *mr)
{
}
uint64_t memory_region_size(MemoryRegion *mr)
{
	return 0;
}
void memory_region_sync_dirty_bitmap(MemoryRegion *mr)
{
}
void memory_region_set_coalescing(MemoryRegion *mr)
{
}

uint16_t le16_to_cpu(uint16_t v)
{
	return v;
}
uint32_t le32_to_cpu(uint32_t v)
{
	return v;
}

void graphic_hw_update(QemuConsole *con)
{
}
void qemu_console_copy(QemuConsole *con, int src_x, int src_y,
                       int dst_x, int dst_y, int w, int h)
{
}

void qemu_flush_coalesced_mmio_buffer(void)
{
}

QEMUClock *vm_clock;
vga_retrace_method vga_retrace_method_value = VGA_RETRACE_DUMB;

int64_t qemu_get_clock_ns(QEMUClock *clock)
{
	return 0;
}
int64_t qemu_get_clock_ms(QEMUClock *clock)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (uae_u64)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
int64_t get_ticks_per_sec(void)
{
    return 1000000;
}


void portio_list_init(PortioList *piolist,
                      const struct MemoryRegionPortio *callbacks,
                      void *opaque, const char *name)
{
}
void portio_list_destroy(PortioList *piolist)
{
}
void portio_list_add(PortioList *piolist,
                     struct MemoryRegion *address_space,
                     uint32_t addr)
{
}
void portio_list_del(PortioList *piolist)
{
}

void dpy_text_cursor(QemuConsole *con, int x, int y)
{
}
void dpy_text_update(QemuConsole *con, int x, int y, int w, int h)
{
}
void dpy_text_resize(QemuConsole *con, int w, int h)
{
}
void dpy_gfx_replace_surface(QemuConsole *con,
                             DisplaySurface *surface)
{
}
