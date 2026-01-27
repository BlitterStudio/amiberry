/*
 * UAE - The Un*x Amiga Emulator
 *
 * CPU Threading - Synchronization and Performance Analysis
 *
 * (c) 2026 Amiberry Development Team
 */

#ifndef UAE_CPU_THREAD_H
#define UAE_CPU_THREAD_H

#include "uae/types.h"

#ifdef WITH_THREADED_CPU
/* CPU Thread IO Operation Queue
 * Lock-free ring buffer for high-priority IO operations that need immediate main-thread processing
 */

#define CPU_THREAD_IO_QUEUE_SIZE 256

typedef enum {
	CPU_IO_INTERRUPT,        // Interrupt trigger (highest priority)
	CPU_IO_CHIPSET_WRITE,    // Chipset register write
	CPU_IO_CHIPSET_READ,     // Chipset register read
	CPU_IO_CIA_WRITE,        // CIA register write
	CPU_IO_CIA_READ,         // CIA register read
	CPU_IO_CUSTOM,           // Custom device operation
} cpu_io_op_type;

typedef struct {
	cpu_io_op_type op_type;
	uaecptr address;
	uae_u32 data;
	int size;  // 1, 2, 4 bytes
	evt_t cpu_cycle;  // CPU cycle when operation was queued
} cpu_io_operation;

/* Thread diagnostics and profiling */
typedef struct {
	// Counters
	volatile uae_atomic stall_count;          // Number of times CPU thread stalled on IO
	volatile uae_atomic stall_cycles_lost;     // Cycles wasted on synchronization
	volatile uae_atomic io_queue_ops;          // Operations queued
	volatile uae_atomic io_queue_flushes;      // Times queue was flushed
	volatile uae_atomic sync_point_count;      // Number of re-sync operations
	volatile uae_atomic longest_stall;         // Longest single stall in cycles

	// Batching metrics
	volatile uae_atomic batch_ops;             // Operations batched
	volatile uae_atomic batch_flushes;         // Batch flush count

	// State tracking
	volatile uae_atomic is_stalled;            // Currently stalled?
	evt_t stall_start_cycle;                   // When current stall started

	// Configuration
	int enable_diagnostics;
	int queue_strategy;  // 0 = none, 1 = immediate, 2 = batched
} cpu_thread_diagnostics;

extern cpu_thread_diagnostics cpu_thread_diag;

/* Function prototypes */
void cpu_thread_diag_init(void);
void cpu_thread_diag_reset(void);
void cpu_thread_diag_log_stall(evt_t duration);
void cpu_thread_diag_log_io_op(cpu_io_op_type op);
void cpu_thread_diag_print_stats(void);

/* IO Queue operations */
int cpu_thread_queue_io_op(cpu_io_operation *op);
int cpu_thread_flush_io_queue(void);
cpu_io_operation* cpu_thread_peek_io_queue(void);
cpu_io_operation* cpu_thread_dequeue_io_op(void);

/* Synchronization utilities */
#define CPU_THREAD_SYNC_POINT_VBLANK   0x01
#define CPU_THREAD_SYNC_POINT_CYCLE_N  0x02
#define CPU_THREAD_SYNC_POINT_IO_OP    0x04

void cpu_thread_sync_point(int reason);
void cpu_thread_stall_start(void);
void cpu_thread_stall_end(evt_t duration);

/* Phase 2: Register batching functions */
int cpu_thread_queue_register_op(uaecptr addr, uae_u32 value, int size, int is_write);
void cpu_thread_flush_register_batch(void);
int cpu_thread_register_batch_full(void);

#endif /* UAE_CPU_THREAD_H */

#endif