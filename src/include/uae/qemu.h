#ifndef UAE_QEMU_H
#define UAE_QEMU_H

/* This file is intended to be included by external libraries as well,
 * so don't pull in too much UAE-specific stuff. */

#include "uae/types.h"
#include "uae/api.h"

/* The qemu-uae major version must match this */
#define QEMU_UAE_VERSION_MAJOR 3

/* The qemu-uae minor version must be at least this */
#define QEMU_UAE_VERSION_MINOR 8

UAE_DECLARE_IMPORT_FUNCTION(
	void, qemu_uae_version, int *major, int *minor, int *revision)
UAE_DECLARE_IMPORT_FUNCTION(
	void, qemu_uae_init, void)
UAE_DECLARE_IMPORT_FUNCTION(
	void, qemu_uae_start, void)
UAE_DECLARE_IMPORT_FUNCTION(
	int, qemu_uae_lock, int)

UAE_DECLARE_IMPORT_FUNCTION(
	void, qemu_uae_slirp_init, void)
UAE_DECLARE_IMPORT_FUNCTION(
	void, qemu_uae_slirp_input, const uint8_t *pkt, int pkt_len)

UAE_DECLARE_EXPORT_FUNCTION(
	void, uae_slirp_output, const uint8_t *pkt, int pkt_len)

UAE_DECLARE_IMPORT_FUNCTION(
	bool, qemu_uae_ppc_init, const char* model, uint32_t hid1)
UAE_DECLARE_IMPORT_FUNCTION(
	bool, qemu_uae_ppc_in_cpu_thread, void)
UAE_DECLARE_IMPORT_FUNCTION(
	void, qemu_uae_ppc_external_interrupt, bool)

#define QEMU_UAE_LOCK_TRYLOCK 1
#define QEMU_UAE_LOCK_TRYLOCK_CANCEL 2
#define QEMU_UAE_LOCK_ACQUIRE 3
#define QEMU_UAE_LOCK_RELEASE 4

#ifdef UAE

#include "uae/dlopen.h"

UAE_DLHANDLE uae_qemu_uae_init(void);

#endif /* UAE */

#if 0
#ifdef UAE
typedef void (QEMUCALL *qemu_uae_version_function)(int *major, int *minor,
												   int *revision);
extern qemu_uae_version_function qemu_uae_version;
#else
void qemu_uae_version(int *major, int *minor, int *revision);
#endif

#ifdef UAE
typedef void (QEMUCALL *qemu_uae_init_function)(void);
extern qemu_uae_init_function qemu_uae_init;
#else
void qemu_uae_init(void);
#endif
#endif

#endif /* UAE_QEMU_H */
