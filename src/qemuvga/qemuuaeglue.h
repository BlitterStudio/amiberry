#include "uae/inline.h"
#include "uae/likely.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sysconfig.h"

#ifdef DEBUGGER
extern void activate_debugger(void);
#endif

//#define DEBUG_VGA_REG
//#define DEBUG_VGA

extern void write_log (const char *, ...);

#ifndef glue
#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)	tostring(s)
#define tostring(s)	#s
#endif

#ifndef AMIBERRY
typedef int ssize_t;
#endif

#ifdef _MSC_VER
#include <windows.h>
#define container_of(address, type, field) ((type *)( \
        (PCHAR)(address) - \
        (ULONG_PTR)(&((type *)0)->field)))

#define snprintf c99_snprintf
inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}
inline int c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}


#else
#ifndef container_of
#define container_of(ptr, type, member) ({                      \
        const typeof(((type *) 0)->member) *__mptr = (ptr);     \
        (type *) ((char *) __mptr - offsetof(type, member));})
#endif
#endif

#ifndef ABS
#define ABS(x) abs(x)
#endif

#ifdef USE_GLIB
#include <glib.h>
#else
#define g_free free
#define g_malloc malloc
#define g_new(type, num) ((type*)calloc(sizeof(type),num))
#endif

enum device_endian {
    DEVICE_NATIVE_ENDIAN,
    DEVICE_BIG_ENDIAN,
    DEVICE_LITTLE_ENDIAN,
};
enum vga_retrace_method {
    VGA_RETRACE_DUMB,
    VGA_RETRACE_PRECISE
};
extern vga_retrace_method vga_retrace_method_value;

typedef uint32_t QEMUClock;
extern QEMUClock *vm_clock;
int64_t qemu_get_clock_ns(QEMUClock *clock);
int64_t qemu_get_clock_ms(QEMUClock *clock);
int64_t get_ticks_per_sec(void);

#define isa_mem_base 0

#define QemuConsole void
#define console_ch_t uint8_t
typedef struct GraphicHwOps {
    void (*invalidate)(void *opaque);
    void (*gfx_update)(void *opaque);
    void (*text_update)(void *opaque, console_ch_t *text);
    void (*update_interval)(void *opaque, uint64_t interval);
} GraphicHwOps;

#define VMStateDescription uint32_t
#define hwaddr uint32_t
#define ram_addr_t uint32_t

typedef struct DisplaySurface {
	void *data;
} DisplaySurface;

uint16_t le16_to_cpu(uint16_t v);
uint32_t le32_to_cpu(uint32_t v);

#define le_bswap(v, size) (v)
#define cpu_to_le16(x) (x)

STATIC_INLINE void stl_he_p(void *ptr, uint32_t v)
{
	memcpy(ptr, &v, sizeof(v));
}
STATIC_INLINE void stl_le_p(void *ptr, uint32_t v)
{
	stl_he_p(ptr, le_bswap(v, 32));
}
STATIC_INLINE int ldl_he_p(const void *ptr)
{
	int32_t r;
	memcpy(&r, ptr, sizeof(r));
	return r;
}
STATIC_INLINE int ldl_le_p(const void *ptr)
{
	return le_bswap(ldl_he_p(ptr), 32);
}
STATIC_INLINE void cpu_to_32wu(uint32_t *p, uint32_t v)
{
	stl_le_p(p, v);
}

void graphic_hw_update(QemuConsole *con);
void qemu_console_copy(QemuConsole *con, int src_x, int src_y,
                       int dst_x, int dst_y, int w, int h);
void qemu_console_resize(QemuConsole *con, int width, int height);
DisplaySurface *qemu_console_surface(QemuConsole *con);
DisplaySurface* qemu_create_displaysurface_from(int width, int height, int bpp,
                                                int linesize, uint8_t *data,
                                                bool byteswap);
int surface_stride(DisplaySurface *s);
uint8_t *surface_data(DisplaySurface *s);
int is_surface_bgr(DisplaySurface *surface);

static inline int is_buffer_shared(DisplaySurface *surface)
{
    return 0;
}

void dpy_gfx_update(QemuConsole *con, int x, int y, int w, int h);
void dpy_text_cursor(QemuConsole *con, int x, int y);
void dpy_text_update(QemuConsole *con, int x, int y, int w, int h);
void dpy_text_resize(QemuConsole *con, int w, int h);
void dpy_gfx_replace_surface(QemuConsole *con,
                             DisplaySurface *surface);

static inline void console_write_ch(console_ch_t *dest, uint32_t ch)
{
    if (!(ch & 0xff))
        ch |= ' ';
    *dest = ch;
}

void qemu_flush_coalesced_mmio_buffer(void);

int surface_bits_per_pixel(DisplaySurface *s);
int surface_bytes_per_pixel(DisplaySurface *s);

typedef struct PortioList {
    const struct MemoryRegionPortio *ports;
    struct MemoryRegion *address_space;
    unsigned nr;
    struct MemoryRegion **regions;
    struct MemoryRegion **aliases;
    void *opaque;
    const char *name;
} PortioList;

void portio_list_init(PortioList *piolist,
                      const struct MemoryRegionPortio *callbacks,
                      void *opaque, const char *name);
void portio_list_destroy(PortioList *piolist);
void portio_list_add(PortioList *piolist,
                     struct MemoryRegion *address_space,
                     uint32_t addr);
void portio_list_del(PortioList *piolist);


typedef struct IORangeOps IORangeOps;

typedef struct IORange {
    const IORangeOps *ops;
    uint64_t base;
    uint64_t len;
} IORange;

struct IORangeOps {
    void (*read)(IORange *iorange, uint64_t offset, unsigned width,
                 uint64_t *data);
    void (*write)(IORange *iorange, uint64_t offset, unsigned width,
                  uint64_t data);
    void (*destructor)(IORange *iorange);
};


typedef void (IOPortWriteFunc)(void *opaque, uint32_t address, uint32_t data);
typedef uint32_t (IOPortReadFunc)(void *opaque, uint32_t address);

typedef void CPUWriteMemoryFunc(void *opaque, hwaddr addr, uint32_t value);
typedef uint32_t CPUReadMemoryFunc(void *opaque, hwaddr addr);

#include "qemumemory.h"
#include "pixel_ops.h"


static inline uint32_t lduw_raw(void *p)
{
	return ((uint32_t*)p)[0];
}

typedef void QEMUResetHandler(void *opaque);
void qemu_register_reset(QEMUResetHandler *func, void *opaque);

#include "vga_int.h"

// ID
#define CIRRUS_ID_CLGD5422  (0x23<<2)
#define CIRRUS_ID_CLGD5426  (0x24<<2)
#define CIRRUS_ID_CLGD5424  (0x25<<2)
#define CIRRUS_ID_CLGD5428  (0x26<<2)
#define CIRRUS_ID_CLGD5429  (0x27<<2)
#define CIRRUS_ID_CLGD5430  (0x28<<2)
#define CIRRUS_ID_CLGD5434  (0x2A<<2)
#define CIRRUS_ID_CLGD5436  (0x2B<<2)
#define CIRRUS_ID_CLGD5446  (0x2E<<2)

typedef struct CirrusVGAState CirrusVGAState;

typedef void (*cirrus_bitblt_rop_t) (CirrusVGAState *s,
					uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
					const uint8_t *src, uint32_t srcaddr, uint32_t srcmask,
				    int dstpitch, int srcpitch,
				    int bltwidth, int bltheight);
typedef void (*cirrus_fill_t)(CirrusVGAState *s,
					uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
					int dst_pitch, int width, int height);

struct CirrusVGAState {
    VGACommonState vga;

    MemoryRegion cirrus_vga_io;
    MemoryRegion cirrus_linear_io;
    MemoryRegion cirrus_linear_bitblt_io;
    MemoryRegion cirrus_mmio_io;
    MemoryRegion pci_bar;
    bool linear_vram;  /* vga.vram mapped over cirrus_linear_io */
    MemoryRegion low_mem_container; /* container for 0xa0000-0xc0000 */
    MemoryRegion low_mem;           /* always mapped, overridden by: */
    MemoryRegion cirrus_bank[2];    /*   aliases at 0xa0000-0xb0000  */
    uint32_t cirrus_addr_mask;
    uint32_t linear_mmio_mask;
    uint8_t cirrus_shadow_gr0;
    uint8_t cirrus_shadow_gr1;
    uint8_t cirrus_hidden_dac_lockindex;
    uint8_t cirrus_hidden_dac_data;
    uint32_t cirrus_bank_base[2];
    uint32_t cirrus_bank_limit[2];
    uint8_t cirrus_hidden_palette[48];
    int32_t hw_cursor_x;
    int32_t hw_cursor_y;
    int cirrus_blt_pixelwidth;
    int cirrus_blt_width;
    int cirrus_blt_height;
    int cirrus_blt_dstpitch;
    int cirrus_blt_srcpitch;
    uint32_t cirrus_blt_fgcol;
    uint32_t cirrus_blt_bgcol;
    uint32_t cirrus_blt_dstaddr;
    uint32_t cirrus_blt_srcaddr;
    uint8_t cirrus_blt_mode;
    uint8_t cirrus_blt_modeext;
    cirrus_bitblt_rop_t cirrus_rop;
#define CIRRUS_BLTBUFSIZE (2048 * 4) /* one line width */
    uint8_t cirrus_bltbuf[CIRRUS_BLTBUFSIZE];
    uint8_t *cirrus_srcptr;
    uint8_t *cirrus_srcptr_end;
    uint32_t cirrus_srccounter;
    /* hwcursor display state */
    int last_hw_cursor_size;
    int last_hw_cursor_x;
    int last_hw_cursor_y;
    int last_hw_cursor_y_start;
    int last_hw_cursor_y_end;
    int real_vram_size; /* XXX: suppress that */
	int total_vram_size;
    int device_id;
    int bustype;
	int valid_memory_config;
	bool x86vga;
};

void cirrus_init_common(CirrusVGAState * s, int device_id, int is_pci,
                               MemoryRegion *system_memory,
                               MemoryRegion *system_io, int vramlimit, bool x86vga);

struct DeviceState
{
	void *lsistate;
};

#define QEMUFile void*
#define PCIDevice void
typedef unsigned long dma_addr_t;
#define PCI_DEVICE(s) ((void*)(s->bus.privdata))
#define DMA_ADDR_FMT "%08x"

void pci710_set_irq(PCIDevice *pci_dev, int level);
void lsi710_scsi_init(DeviceState *dev);
void lsi710_scsi_reset(DeviceState *dev, void*);

void pci_set_irq(PCIDevice *pci_dev, int level);
void lsi_scsi_init(DeviceState *dev);
void lsi_scsi_reset(DeviceState *dev, void*);

static inline int32_t sextract32(uint32_t value, int start, int length)
{
//    assert(start >= 0 && length > 0 && length <= 32 - start);
    /* Note that this implementation relies on right shift of signed
     * integers being an arithmetic shift.
     */
    return ((int32_t)(value << (32 - length - start))) >> (32 - length);
}
static inline uint32_t deposit32(uint32_t value, int start, int length,
                                 uint32_t fieldval)
{
    uint32_t mask;
//    assert(start >= 0 && length > 0 && length <= 32 - start);
    mask = (~0U >> (32 - length)) << start;
    return (value & ~mask) | ((fieldval << start) & mask);
}

STATIC_INLINE uint32_t cpu_to_le32(uint32_t t)
{
	return ((t >> 24) & 0x000000ff) | ((t >> 8) & 0x0000ff00) | ((t << 8) & 0x00ff0000) | ((t << 24) & 0xff000000);
}

typedef enum {
    DMA_DIRECTION_TO_DEVICE = 0,
    DMA_DIRECTION_FROM_DEVICE = 1,
} DMADirection;

int pci710_dma_rw(PCIDevice *dev, dma_addr_t addr, void *buf, dma_addr_t len, DMADirection dir);

static inline int pci710_dma_read(PCIDevice *dev, dma_addr_t addr,
                               void *buf, dma_addr_t len)
{
    return pci710_dma_rw(dev, addr, buf, len, DMA_DIRECTION_TO_DEVICE);
}
static inline int pci710_dma_write(PCIDevice *dev, dma_addr_t addr,
                                const void *buf, dma_addr_t len)
{
    return pci710_dma_rw(dev, addr, (void *) buf, len, DMA_DIRECTION_FROM_DEVICE);
}

int pci_dma_rw(PCIDevice *dev, dma_addr_t addr, void *buf, dma_addr_t len, DMADirection dir);

static inline int pci_dma_read(PCIDevice *dev, dma_addr_t addr,
	void *buf, dma_addr_t len)
{
	return pci_dma_rw(dev, addr, buf, len, DMA_DIRECTION_TO_DEVICE);
}
static inline int pci_dma_write(PCIDevice *dev, dma_addr_t addr,
	const void *buf, dma_addr_t len)
{
	return pci_dma_rw(dev, addr, (void *)buf, len, DMA_DIRECTION_FROM_DEVICE);
}

struct BusState {
    //Object obj;
    DeviceState *parent;
    const char *name;
    int allow_hotplug;
    int max_index;
//    QTAILQ_HEAD(ChildrenHead, BusChild) children;
//    QLIST_ENTRY(BusState) sibling;
};


extern void lsi710_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
extern uint64_t lsi710_mmio_read(void *opaque, hwaddr addr, unsigned size);

extern void lsi_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
extern uint64_t lsi_mmio_read(void *opaque, hwaddr addr, unsigned size);

// ESP

typedef void *qemu_irq;
typedef void* SysBusDevice;

