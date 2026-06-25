#include "amiberry_adpf.h"

#ifdef __ANDROID__

#include <android/api-level.h>
#include <dlfcn.h>
#include <mutex>

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

// Guards the session and the tid table below: adpf_register_thread() may be
// called from both the main thread and the CPU thread, while adpf_report_frame()
// runs on the frame thread.  Contention only happens once, at CPU-thread
// startup; thereafter the lock is uncontended (taken once per frame).
static std::mutex g_lock;
static APerformanceHintManager* g_manager;
static APerformanceHintSession* g_session;
static int64_t g_last_target_ns;

#define ADPF_MAX_THREADS 8
static int32_t g_tids[ADPF_MAX_THREADS];
static int     g_ntids;

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

// Create a session covering every currently-registered tid.  Builds the new
// session before closing the old one, so a failed rebuild leaves the previous
// (working) session intact rather than dropping coverage.  Caller holds g_lock.
static bool rebuild_session_locked(void)
{
	if (g_ntids == 0)
		return false;
	if (!g_manager) {
		g_manager = p_getManager();
		if (!g_manager)
			return false;
	}
	APerformanceHintSession* ns =
		p_createSession(g_manager, g_tids, (size_t)g_ntids, ADPF_PROVISIONAL_TARGET_NS);
	if (!ns)
		return false;
	if (g_session)
		p_closeSession(g_session);
	g_session = ns;
	g_last_target_ns = ADPF_PROVISIONAL_TARGET_NS;
	return true;
}

bool adpf_register_thread(int32_t tid)
{
	std::lock_guard<std::mutex> guard(g_lock);

	if (!resolve_api())
		return false;

	// Already registered: report current coverage without rebuilding.
	for (int i = 0; i < g_ntids; i++) {
		if (g_tids[i] == tid)
			return g_session != nullptr;
	}
	if (g_ntids >= ADPF_MAX_THREADS)
		return g_session != nullptr;

	g_tids[g_ntids++] = tid;
	if (!rebuild_session_locked()) {
		write_log("ADPF: createSession failed (tid=%d)\n", tid);
		g_ntids--; // roll back so this thread can use the static fallback
		return false;
	}
	write_log("ADPF: hint session active, %d thread(s), tid=%d, target=%.1fms\n",
		g_ntids, tid, ADPF_PROVISIONAL_TARGET_NS / 1e6);
	return true;
}

void adpf_report_frame(int64_t actual_work_ns, int64_t target_ns)
{
	std::lock_guard<std::mutex> guard(g_lock);
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
	std::lock_guard<std::mutex> guard(g_lock);
	if (g_session && p_closeSession) {
		p_closeSession(g_session);
		g_session = nullptr;
	}
	g_ntids = 0;
}

#else // !__ANDROID__

// Cross-platform no-op stubs so non-Android builds link cleanly.
bool adpf_register_thread(int32_t) { return false; }
void adpf_report_frame(int64_t, int64_t) {}
void adpf_cleanup(void) {}

#endif // __ANDROID__
