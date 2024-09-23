#include "sysconfig.h"
#include "sysdeps.h"

#include "uae/dlopen.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "uae/qemu.h"

UAE_DEFINE_IMPORT_FUNCTION(qemu_uae_version)
UAE_DEFINE_IMPORT_FUNCTION(qemu_uae_init)
UAE_DEFINE_IMPORT_FUNCTION(qemu_uae_start)

UAE_DEFINE_IMPORT_FUNCTION(qemu_uae_slirp_init)
UAE_DEFINE_IMPORT_FUNCTION(qemu_uae_slirp_input)

UAE_DEFINE_IMPORT_FUNCTION(qemu_uae_ppc_init)
UAE_DEFINE_IMPORT_FUNCTION(qemu_uae_ppc_in_cpu_thread)

static void init_ppc(UAE_DLHANDLE handle)
{
#ifdef WITH_PPC
	UAE_IMPORT_FUNCTION(handle, qemu_uae_ppc_init);
	UAE_IMPORT_FUNCTION(handle, qemu_uae_ppc_in_cpu_thread);

	if (qemu_uae_ppc_init) {
		const TCHAR *model;
		uint32_t hid1;
		uae_ppc_get_model(&model, &hid1);
		char *model_s = ua(model);
		qemu_uae_ppc_init(model_s, hid1);
		free(model_s);
	}
#endif
}

#ifdef WITH_QEMU_SLIRP
static void init_slirp(UAE_DLHANDLE handle)
{
	UAE_IMPORT_FUNCTION(handle, qemu_uae_slirp_init);
	UAE_IMPORT_FUNCTION(handle, qemu_uae_slirp_input);
	UAE_EXPORT_FUNCTION(handle, uae_slirp_output);

	if (qemu_uae_slirp_init) {
		qemu_uae_slirp_init();
	}
}
#endif

UAE_DLHANDLE uae_qemu_uae_init(void)
{
	static bool initialized;
	static UAE_DLHANDLE handle;
	if (initialized) {
		return handle;
	}
	initialized = true;

	handle = uae_dlopen_plugin(_T("qemu-uae"));
	if (!handle) {
		gui_message(_T("Error loading qemu-uae plugin\n"));
		return handle;
	}
	write_log(_T("Loaded qemu-uae library at %p\n"), handle);

	/* Check major version (=) and minor version (>=) */

	qemu_uae_version = (qemu_uae_version_function) uae_dlsym(
		handle, "qemu_uae_version");

	int major = 0, minor = 0, revision = 0;
	if (qemu_uae_version) {
		qemu_uae_version(&major, &minor, &revision);
	}
	if (major != QEMU_UAE_VERSION_MAJOR) {
		gui_message(
			_T("PPC: Wanted qemu-uae version %d.x (got %d.x)\n"),
			QEMU_UAE_VERSION_MAJOR, major);
		handle = NULL;
		return handle;
	}
	if (minor < QEMU_UAE_VERSION_MINOR) {
		gui_message(
			_T("PPC: Wanted qemu-uae version >= %d.%d (got %d.%d)\n"),
			QEMU_UAE_VERSION_MAJOR, QEMU_UAE_VERSION_MINOR,
			major, minor);
		handle = NULL;
		return handle;
	}

	/* Retrieve function pointers from library */

	UAE_IMPORT_FUNCTION(handle, qemu_uae_init);
	UAE_IMPORT_FUNCTION(handle, qemu_uae_start);

	if (qemu_uae_init) {
		qemu_uae_init();
	} else {
		gui_message(_T("qemu_uae_init not found"));
		handle = NULL;
		return handle;
	}

	init_ppc(handle);
#ifdef WITH_QEMU_SLIRP
	init_slirp(handle);
#endif

	if (qemu_uae_start) {
		qemu_uae_start();
	} else {
		gui_message(_T("qemu_uae_start not found"));
		handle = NULL;
		return handle;
	}

	return handle;
}
