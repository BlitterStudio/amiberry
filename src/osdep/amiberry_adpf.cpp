#include "amiberry_adpf.h"

#ifdef __ANDROID__

#include <android/api-level.h>
#include <dlfcn.h>

#include "sysconfig.h"
#include "sysdeps.h" // write_log

// --- ADPF C API (android/performance_hint.h, API 31+) --------------------
// Opaque types + function pointers declared locally and resolved with dlsym, so
// the binary still loads on API 29-30 where these symbols do not exist.
typedef struct APerformanceHintManager APerformanceHintManager;
typedef struct APerformanceHintSession APerformanceHintSession;

typedef APerformanceHintManager* (*pfn_getManager)(void);
typedef APerformanceHintSession* (*pfn_createSession)(
	APerformanceHintManager*, const int32_t*, size_t, int64_t);
typedef int  (*pfn_updateTarget)(APerformanceHintSession*, int64_t);
typedef int  (*pfn_reportActual)(APerformanceHintSession*, int64_t);
typedef void (*pfn_closeSession)(APerformanceHintSession*);

static pfn_getManager    p_getManager;
static pfn_createSession p_createSession;
static pfn_updateTarget  p_updateTarget;
static pfn_reportActual  p_reportActual;
static pfn_closeSession  p_closeSession;

static APerformanceHintSession* g_session;
static int64_t g_last_target_ns;

static const int64_t ADPF_PROVISIONAL_TARGET_NS = 20000000; // 20 ms (PAL)

// Resolve the ADPF entry points once.  Returns true if all are available.
static bool resolve_api(void)
{
	if (p_getManager)
		return true; // already resolved
	if (android_get_device_api_level() < 31)
		return false;
	void* lib = dlopen("libandroid.so", RTLD_NOW);
	if (!lib)
		return false;
	p_getManager    = (pfn_getManager)    dlsym(lib, "APerformanceHint_getManager");
	p_createSession = (pfn_createSession) dlsym(lib, "APerformanceHint_createSession");
	p_updateTarget  = (pfn_updateTarget)  dlsym(lib, "APerformanceHint_updateTargetWorkDuration");
	p_reportActual  = (pfn_reportActual)  dlsym(lib, "APerformanceHint_reportActualWorkDuration");
	p_closeSession  = (pfn_closeSession)  dlsym(lib, "APerformanceHint_closeSession");
	if (!p_getManager || !p_createSession || !p_updateTarget || !p_reportActual || !p_closeSession) {
		p_getManager = nullptr; // leave unresolved so a later call can retry
		return false;
	}
	return true;
}

bool adpf_init(int32_t emu_tid)
{
	adpf_cleanup(); // idempotent: drop any prior session

	if (!resolve_api())
		return false;

	APerformanceHintManager* mgr = p_getManager();
	if (!mgr)
		return false;

	int32_t tids[1] = { emu_tid };
	g_session = p_createSession(mgr, tids, 1, ADPF_PROVISIONAL_TARGET_NS);
	if (!g_session) {
		write_log("ADPF: createSession failed (tid=%d)\n", emu_tid);
		return false;
	}
	g_last_target_ns = ADPF_PROVISIONAL_TARGET_NS;
	write_log("ADPF: hint session active, tid=%d, target=%.1fms\n",
		emu_tid, ADPF_PROVISIONAL_TARGET_NS / 1e6);
	return true;
}

void adpf_report_frame(int64_t actual_work_ns, int64_t target_ns)
{
	if (!g_session)
		return;
	if (actual_work_ns < 1)
		actual_work_ns = 1;
	if (target_ns < 1)
		target_ns = 1;
	if (target_ns != g_last_target_ns) {
		p_updateTarget(g_session, target_ns);
		g_last_target_ns = target_ns;
	}
	p_reportActual(g_session, actual_work_ns);
}

void adpf_cleanup(void)
{
	if (g_session && p_closeSession) {
		p_closeSession(g_session);
		g_session = nullptr;
	}
}

#else // !__ANDROID__

// Cross-platform no-op stubs so non-Android builds link cleanly.
bool adpf_init(int32_t) { return false; }
void adpf_report_frame(int64_t, int64_t) {}
void adpf_cleanup(void) {}

#endif // __ANDROID__
