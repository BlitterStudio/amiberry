#include "sysconfig.h"
#include "sysdeps.h"

#include "uae/dlopen.h"
#ifdef WITH_QEMU_PPC
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

template<typename T>
static bool require_qemu_symbol(UAE_DLHANDLE handle, T& function, const char *name)
{
	function = (T)uae_dlsym(handle, name);
	if (!function) {
		write_log("qemu-uae: missing required symbol: %s\n", name);
		return false;
	}
	write_log("Imported %s\n", name);
	return true;
}

static bool require_qemu_symbol(UAE_DLHANDLE handle, const char *name)
{
	if (!uae_dlsym(handle, name)) {
		write_log("qemu-uae: missing required symbol: %s\n", name);
		return false;
	}
	write_log("Imported %s\n", name);
	return true;
}

#define REQUIRE_QEMU_SYMBOL(handle, function_name) require_qemu_symbol(handle, function_name, #function_name)

static bool init_ppc(UAE_DLHANDLE handle)
{
#ifdef WITH_QEMU_PPC
	if (!REQUIRE_QEMU_SYMBOL(handle, qemu_uae_ppc_init) ||
		!REQUIRE_QEMU_SYMBOL(handle, qemu_uae_ppc_in_cpu_thread)) {
		return false;
	}
	if (!require_qemu_symbol(handle, "ppc_cpu_init") ||
		!require_qemu_symbol(handle, "qemu_uae_ppc_external_interrupt") ||
		!require_qemu_symbol(handle, "ppc_cpu_map_memory") ||
		!require_qemu_symbol(handle, "ppc_cpu_run_continuous") ||
		!require_qemu_symbol(handle, "ppc_cpu_set_state") ||
		!require_qemu_symbol(handle, "ppc_cpu_reset") ||
		!require_qemu_symbol(handle, "qemu_uae_lock")) {
		return false;
	}

	const TCHAR *model;
	uint32_t hid1;
	uae_ppc_get_model(&model, &hid1);
	char *model_s = ua(model);
	bool initialized = qemu_uae_ppc_init(model_s, hid1);
	free(model_s);
	if (!initialized) {
		write_log(_T("PPC: qemu_uae_ppc_init failed for model %s\n"), model);
		return false;
	}
#endif
	return true;
}

#ifdef WITH_QEMU_SLIRP
static bool init_slirp(UAE_DLHANDLE handle)
{
	if (!REQUIRE_QEMU_SYMBOL(handle, qemu_uae_slirp_init) ||
		!REQUIRE_QEMU_SYMBOL(handle, qemu_uae_slirp_input)) {
		return false;
	}
	UAE_EXPORT_FUNCTION(handle, uae_slirp_output);

	qemu_uae_slirp_init();
	return true;
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

#ifdef MACOS_APP_STORE
	write_log(_T("PPC emulation via qemu-uae plugin is not available in App Store builds\n"));
	return handle;
#endif

	handle = uae_dlopen_plugin(_T("qemu-uae"));
	if (!handle) {
		gui_message(_T("Error loading qemu-uae plugin\n"));
		return handle;
	}
	write_log(_T("Loaded qemu-uae library at %p\n"), handle);

	/* Check major version (=) and minor version (>=) */

	if (!REQUIRE_QEMU_SYMBOL(handle, qemu_uae_version)) {
		gui_message(_T("qemu_uae_version not found"));
		handle = NULL;
		return handle;
	}

	int major = 0, minor = 0, revision = 0;
	qemu_uae_version(&major, &minor, &revision);
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

	if (!REQUIRE_QEMU_SYMBOL(handle, qemu_uae_init)) {
		gui_message(_T("qemu_uae_init not found"));
		handle = NULL;
		return handle;
	}
	qemu_uae_init();

	if (!init_ppc(handle)) {
		gui_message(_T("qemu-uae PPC initialization failed"));
		handle = NULL;
		return handle;
	}
#ifdef WITH_QEMU_SLIRP
	if (!init_slirp(handle)) {
		gui_message(_T("qemu-uae SLIRP initialization failed"));
		handle = NULL;
		return handle;
	}
#endif

	if (!REQUIRE_QEMU_SYMBOL(handle, qemu_uae_start)) {
		gui_message(_T("qemu_uae_start not found"));
		handle = NULL;
		return handle;
	}
	qemu_uae_start();

	return handle;
}
