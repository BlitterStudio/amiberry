/*
 * UAE - The Un*x Amiga Emulator
 *
 * CPU Threading - Synchronization and Performance Analysis Implementation
 *
 * (c) 2026 Amiberry Development Team
 */

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "cpu_thread.h"
#include "memory.h"
#include "events.h"
#include "newcpu.h"

/* Global diagnostics structure */
cpu_thread_diagnostics cpu_thread_diag = {0};

/* Lock-free IO operation queue */
static cpu_io_operation io_queue[CPU_THREAD_IO_QUEUE_SIZE];
static volatile uae_atomic io_queue_write_pos = 0;
static volatile uae_atomic io_queue_read_pos = 0;

/* Phase 2: Register batch queue for chipset/CIA operations
 * Allows CPU thread to queue register operations instead of stalling
 */
#define CPU_THREAD_REGISTER_BATCH_SIZE 64

typedef struct {
	uaecptr address;
	uae_u32 value;
	int size;        // 1, 2, or 4 bytes
	int is_write;    // 1 = write, 0 = read
} cpu_register_op;

static cpu_register_op register_batch[CPU_THREAD_REGISTER_BATCH_SIZE];
static volatile uae_atomic register_batch_count = 0;

void cpu_thread_diag_init(void)
{
	memset(&cpu_thread_diag, 0, sizeof(cpu_thread_diag));
	cpu_thread_diag.enable_diagnostics = 1;
	cpu_thread_diag.queue_strategy = 1;  // Default: immediate processing
}

void cpu_thread_diag_reset(void)
{
	atomic_set(&cpu_thread_diag.stall_count, 0);
	atomic_set(&cpu_thread_diag.stall_cycles_lost, 0);
	atomic_set(&cpu_thread_diag.io_queue_ops, 0);
	atomic_set(&cpu_thread_diag.io_queue_flushes, 0);
	atomic_set(&cpu_thread_diag.sync_point_count, 0);
	atomic_set(&cpu_thread_diag.longest_stall, 0);
	atomic_set(&cpu_thread_diag.is_stalled, 0);
	cpu_thread_diag.stall_start_cycle = 0;

	atomic_set(&io_queue_write_pos, 0);
	atomic_set(&io_queue_read_pos, 0);
}

void cpu_thread_diag_log_stall(evt_t duration)
{
	if (!cpu_thread_diag.enable_diagnostics)
		return;

	atomic_inc(&cpu_thread_diag.stall_count);
	atomic_add(&cpu_thread_diag.stall_cycles_lost, duration);

	evt_t longest = atomic_read(&cpu_thread_diag.longest_stall);
	while (duration > longest) {
		if (atomic_cas(&cpu_thread_diag.longest_stall, longest, duration)) {
			break;
		}
		longest = atomic_read(&cpu_thread_diag.longest_stall);
	}
}

void cpu_thread_diag_log_io_op(cpu_io_op_type op)
{
	if (!cpu_thread_diag.enable_diagnostics)
		return;
	atomic_inc(&cpu_thread_diag.io_queue_ops);
}

void cpu_thread_diag_print_stats(void)
{
	if (!cpu_thread_diag.enable_diagnostics)
		return;

	uae_u32 stalls = atomic_read(&cpu_thread_diag.stall_count);
	uae_u32 cycles = atomic_read(&cpu_thread_diag.stall_cycles_lost);
	uae_u32 ops = atomic_read(&cpu_thread_diag.io_queue_ops);
	uae_u32 syncs = atomic_read(&cpu_thread_diag.sync_point_count);
	uae_u32 longest = atomic_read(&cpu_thread_diag.longest_stall);

	write_log(_T("\n"));
	write_log(_T("================= CPU THREAD DIAGNOSTICS REPORT =================\n"));
	write_log(_T("Configuration:\n"));
	write_log(_T("  CPU Threading Enabled: %s\n"), currprefs.cpu_thread ? _T("Yes") : _T("No"));
	write_log(_T("  CPU Model: %d\n"), currprefs.cpu_model);
	write_log(_T("  Cycle Exact: %s\n"), currprefs.cpu_cycle_exact ? _T("Yes") : _T("No"));
	write_log(_T("  Memory Cycle Exact: %s\n"), currprefs.cpu_memory_cycle_exact ? _T("Yes") : _T("No"));
	write_log(_T("\n"));
	write_log(_T("Synchronization Stalls:\n"));
	write_log(_T("  Total stalls: %u\n"), stalls);
	write_log(_T("  Total cycles lost to stalls: %u\n"), cycles);
	if (stalls > 0) {
		write_log(_T("  Average cycles/stall: %.2f\n"), (float)cycles / stalls);
	}
	write_log(_T("  Longest single stall: %u cycles\n"), longest);
	write_log(_T("\n"));
	write_log(_T("IO Queue Operations:\n"));
	write_log(_T("  Operations queued: %u\n"), ops);
	write_log(_T("  Queue flushes: %u\n"), atomic_read(&cpu_thread_diag.io_queue_flushes));
	write_log(_T("\n"));
	write_log(_T("Synchronization Points:\n"));
	write_log(_T("  Total sync points: %u\n"), syncs);
	write_log(_T("\n"));

	if (stalls > 0) {
		write_log(_T("ANALYSIS:\n"));
		if (cycles > (stalls * 1000)) {
			write_log(_T("  WARNING: Significant cycle loss due to thread synchronization.\n"));
			write_log(_T("  Consider increasing CPU_THREAD_IO_QUEUE_SIZE or using batched IO processing.\n"));
		}
		if (longest > 5000) {
			write_log(_T("  WARNING: Very long stall detected (%u cycles).\n"), longest);
			write_log(_T("  This may indicate main thread contention or IO bottleneck.\n"));
		}
	} else {
		write_log(_T("  No synchronization stalls detected - CPU threading working well.\n"));
	}
	write_log(_T("================= END DIAGNOSTICS REPORT =================\n"));
	write_log(_T("\n"));
}

/* Lock-free queue operations using atomic CAS */
int cpu_thread_queue_io_op(cpu_io_operation *op)
{
	if (!currprefs.cpu_thread || is_mainthread()) {
		return 0;  // No queueing needed if on main thread
	}

	uae_u32 write_pos = atomic_read(&io_queue_write_pos);
	uae_u32 next_pos = (write_pos + 1) % CPU_THREAD_IO_QUEUE_SIZE;
	uae_u32 read_pos = atomic_read(&io_queue_read_pos);

	/* Check if queue is full */
	if (next_pos == read_pos) {
		cpu_thread_diag_log_stall(0);  // Queue full, count as stall
		return 0;
	}

	io_queue[write_pos] = *op;
	op->cpu_cycle = get_cycles();

	/* CAS to update write position atomically */
	if (atomic_cas(&io_queue_write_pos, write_pos, next_pos)) {
		cpu_thread_diag_log_io_op(op->op_type);
		return 1;
	}

	return 0;
}

int cpu_thread_flush_io_queue(void)
{
	int ops_processed = 0;

	while (atomic_read(&io_queue_read_pos) != atomic_read(&io_queue_write_pos)) {
		cpu_io_operation *op = cpu_thread_dequeue_io_op();
		if (!op) break;

		/* Process operation - placeholder for actual implementation */
		ops_processed++;
	}

	if (ops_processed > 0) {
		atomic_inc(&cpu_thread_diag.io_queue_flushes);
	}

	return ops_processed;
}

cpu_io_operation* cpu_thread_peek_io_queue(void)
{
	uae_u32 read_pos = atomic_read(&io_queue_read_pos);
	uae_u32 write_pos = atomic_read(&io_queue_write_pos);

	if (read_pos == write_pos) {
		return NULL;
	}

	return &io_queue[read_pos];
}

cpu_io_operation* cpu_thread_dequeue_io_op(void)
{
	uae_u32 read_pos = atomic_read(&io_queue_read_pos);
	uae_u32 write_pos = atomic_read(&io_queue_write_pos);

	if (read_pos == write_pos) {
		return NULL;
	}

	cpu_io_operation *op = &io_queue[read_pos];
	uae_u32 next_pos = (read_pos + 1) % CPU_THREAD_IO_QUEUE_SIZE;

	if (atomic_cas(&io_queue_read_pos, read_pos, next_pos)) {
		return op;
	}

	return NULL;
}

void cpu_thread_sync_point(int reason)
{
	if (!currprefs.cpu_thread) {
		return;
	}

	atomic_inc(&cpu_thread_diag.sync_point_count);

	/* Main thread should process queued IO operations here */
	if (is_mainthread()) {
		cpu_thread_flush_io_queue();
	}
}

void cpu_thread_stall_start(void)
{
	if (!cpu_thread_diag.enable_diagnostics) {
		return;
	}

	atomic_set(&cpu_thread_diag.is_stalled, 1);
	cpu_thread_diag.stall_start_cycle = get_cycles();
}

void cpu_thread_stall_end(evt_t duration)
{
	if (!cpu_thread_diag.enable_diagnostics) {
		return;
	}

	atomic_set(&cpu_thread_diag.is_stalled, 0);
	cpu_thread_diag_log_stall(duration);
}

/* Phase 2: Register batching implementation
 * Queues chipset/CIA register operations to reduce stalls
 */

int cpu_thread_register_batch_full(void)
{
	/* Check if batch buffer is full */
	return atomic_read(&register_batch_count) >= CPU_THREAD_REGISTER_BATCH_SIZE;
}

int cpu_thread_queue_register_op(uaecptr addr, uae_u32 value, int size, int is_write)
{
	if (!currprefs.cpu_thread || is_mainthread()) {
		return 0;  /* Not applicable for main thread or threading disabled */
	}

	/* Check current batch count */
	uae_u32 count = atomic_read(&register_batch_count);
	if (count >= CPU_THREAD_REGISTER_BATCH_SIZE) {
		/* Batch is full - signal to main thread that flush needed */
		return 0;
	}

	/* Queue operation in batch */
	register_batch[count].address = addr;
	register_batch[count].value = value;
	register_batch[count].size = size;
	register_batch[count].is_write = is_write;

	/* Increment batch count atomically */
	atomic_inc(&register_batch_count);
	atomic_inc(&cpu_thread_diag.batch_ops);

	return 1;  /* Successfully queued */
}

void cpu_thread_flush_register_batch(void)
{
	uae_u32 count = atomic_read(&register_batch_count);

	if (count == 0) {
		return;  /* Nothing to flush */
	}

	/* In a full implementation, this would process all queued operations
	 * For now, just reset the counter
	 * TODO: Implement actual operation processing in main thread
	 */

	atomic_set(&register_batch_count, 0);
	atomic_inc(&cpu_thread_diag.batch_flushes);
}

