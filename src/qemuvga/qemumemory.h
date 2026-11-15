/*
 * Memory region interface for the QEMU-derived devices.
 *
 * The VGA, Cirrus Logic, SCSI, network and sound chip code in qemuvga/
 * describes its registers and memory as memory regions. WinUAE has no
 * QEMU memory map behind them: qemuuaeglue.cpp and gfxboard.cpp implement
 * these functions and route the accesses to the Amiga side.
 *
 * qemuuaeglue.h includes this header after it has defined hwaddr,
 * enum device_endian and the IOPort function types.
 *
 * Copyright (C) 2026 Stefan Reinauer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef QMEMORY_H
#define QMEMORY_H

#include <stdint.h>

/* Dirty tracking client used for the VGA frame buffer. */
#define DIRTY_MEMORY_VGA 0

/*
 * Register access callbacks of a device. The devices fill these in
 * positionally, so the order of the fields is part of the interface.
 */
typedef struct MemoryRegionOps {
	uint64_t (*read)(void *opaque, hwaddr addr, unsigned size);
	void (*write)(void *opaque, hwaddr addr, uint64_t data, unsigned size);
	enum device_endian endianness;
	/* Accesses the device accepts. */
	struct {
		unsigned min_access_size;
		unsigned max_access_size;
		bool unaligned;
		bool (*accepts)(void *opaque, hwaddr addr, unsigned size, bool is_write);
	} valid;
	/* Accesses the callbacks implement. */
	struct {
		unsigned min_access_size;
		unsigned max_access_size;
		bool unaligned;
	} impl;
} MemoryRegionOps;

typedef struct MemoryRegion {
	void *opaque;
	void *data;
} MemoryRegion;

/* One entry of a legacy I/O port table; PORTIO_END_OF_LIST() ends it. */
typedef struct MemoryRegionPortio {
	uint32_t offset;
	uint32_t len;
	unsigned size;
	IOPortReadFunc *read;
	IOPortWriteFunc *write;
} MemoryRegionPortio;

#define PORTIO_END_OF_LIST() { }

void memory_region_init(MemoryRegion *mr, const char *name, uint64_t size);
void memory_region_init_io(MemoryRegion *mr, const MemoryRegionOps *ops, void *opaque, const char *name, uint64_t size);
void memory_region_init_ram(MemoryRegion *mr, const char *name, uint64_t size);
void memory_region_init_alias(MemoryRegion *mr, const char *name, MemoryRegion *orig, hwaddr offset, uint64_t size);
void memory_region_destroy(MemoryRegion *mr);

uint64_t memory_region_size(MemoryRegion *mr);
void *memory_region_get_ram_ptr(MemoryRegion *mr);

void memory_region_set_log(MemoryRegion *mr, bool log, unsigned client);
bool memory_region_get_dirty(MemoryRegion *mr, hwaddr addr, hwaddr size, unsigned client);
void memory_region_sync_dirty_bitmap(MemoryRegion *mr);
void memory_region_reset_dirty(MemoryRegion *mr, hwaddr addr, hwaddr size, unsigned client);
/* Mark frame buffer writes for the Amiga display update (WinUAE). */
void linear_memory_region_set_dirty(MemoryRegion *mr, hwaddr addr, hwaddr size);
void vga_memory_region_set_dirty(MemoryRegion *mr, hwaddr addr, hwaddr size);

void memory_region_set_coalescing(MemoryRegion *mr);
void memory_region_set_flush_coalesced(MemoryRegion *mr);

void memory_region_add_subregion(MemoryRegion *mr, hwaddr offset, MemoryRegion *subregion);
void memory_region_add_subregion_overlap(MemoryRegion *mr, hwaddr offset, MemoryRegion *subregion, unsigned priority);
void memory_region_del_subregion(MemoryRegion *mr, MemoryRegion *subregion);
void memory_region_set_enabled(MemoryRegion *mr, bool enabled);
void memory_region_set_alias_offset(MemoryRegion *mr, hwaddr offset);

void memory_region_transaction_begin(void);
void memory_region_transaction_commit(void);

#endif /* QMEMORY_H */
