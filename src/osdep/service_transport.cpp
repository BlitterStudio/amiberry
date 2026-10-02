/* Opaque guest-memory transport to the optional in-process renderer plugin.
 * Commands and rendering remain in the plugin; this file owns byte copies
 * and transfer bounds only. */
#include "sysdeps.h"
#include "uae.h"
#include "uae/dlopen.h"
#include "service_transport.h"
#include "traps.h"
#include "memory.h"
#include "picasso96.h"
#include "gfxboard.h"
#include <mutex>
#include <vector>
#include <new>

namespace {

typedef uint32_t (*FuncQuery)(void);
typedef uint32_t (*FuncCreate)(uint32_t width, uint32_t height);
typedef uint32_t (*FuncSubmit)(uint32_t handle, const uint8_t* packet_data, uint32_t packet_bytes);
typedef uint32_t (*FuncReadback)(uint32_t handle, uint8_t* dest_pixels, uint32_t pixel_bytes);
typedef uint32_t (*FuncRequest)(uint32_t handle, const uint8_t* request, uint32_t request_bytes, uint8_t* reply, uint32_t reply_bytes);
typedef void (*FuncDestroy)(uint32_t handle);
typedef void (*FuncReset)(void);

struct PluginBinding {
	UAE_DLHANDLE handle = nullptr;
	FuncQuery query = nullptr;
	FuncCreate create = nullptr;
	FuncSubmit submit = nullptr;
	FuncReadback readback = nullptr;
	FuncRequest request = nullptr;
	FuncDestroy destroy = nullptr;
	FuncReset reset = nullptr;
	bool loaded = false;
	bool tried = false;
};

// Indirect UAE-board mode runs traps on several worker threads. Loading, reset
// and every plugin call are serialized by g_plugin_mutex, which is never held
// across a trap accessor: those wait for the emulator thread, which may itself
// be waiting for the mutex in service_transport_reset(). Staging buffers are
// per thread, so a concurrent call cannot resize one that is in use.
static PluginBinding g_plugin;
static std::mutex g_plugin_mutex;
static thread_local std::vector<uint8_t> t_input, t_output;

// Caller holds g_plugin_mutex.
bool ensure_plugin_loaded()
{
	if (g_plugin.loaded)
		return true;
	if (g_plugin.tried)
		return false;

	g_plugin.tried = true;
	write_log(_T("SERVICE_TRANSPORT: Attempting to load minigl_plugin...\n"));
	g_plugin.handle = uae_dlopen_plugin(_T("minigl_plugin"));
	if (!g_plugin.handle) {
		g_plugin.handle = uae_dlopen_plugin(_T("libminigl_plugin"));
	}

	if (!g_plugin.handle) {
		write_log(_T("SERVICE_TRANSPORT: Failed to load minigl_plugin (not found in plugins dir).\n"));
		return false;
	}
	g_plugin.query = (FuncQuery)uae_dlsym(g_plugin.handle, "minigl_plugin_query");
	g_plugin.create = (FuncCreate)uae_dlsym(g_plugin.handle, "minigl_plugin_create");
	g_plugin.submit = (FuncSubmit)uae_dlsym(g_plugin.handle, "minigl_plugin_submit");
	g_plugin.readback = (FuncReadback)uae_dlsym(g_plugin.handle, "minigl_plugin_readback");
	g_plugin.request = (FuncRequest)uae_dlsym(g_plugin.handle, "minigl_plugin_request");
	g_plugin.destroy = (FuncDestroy)uae_dlsym(g_plugin.handle, "minigl_plugin_destroy");
	g_plugin.reset = (FuncReset)uae_dlsym(g_plugin.handle, "minigl_plugin_reset");

	if (!g_plugin.query || !g_plugin.create || !g_plugin.submit || !g_plugin.readback
		|| !g_plugin.request || !g_plugin.destroy || !g_plugin.reset) {
		write_log(_T("SERVICE_TRANSPORT: minigl_plugin missing required symbols!\n"));
		uae_dlclose(g_plugin.handle);
		g_plugin.handle = nullptr;
		return false;
	}
	write_log(_T("SERVICE_TRANSPORT: Successfully loaded minigl_plugin!\n"));

	g_plugin.loaded = true;
	return true;
}

// Runs a plugin call under g_plugin_mutex once the plugin is loaded. The
// function pointers never change after loading, so they are read unlocked.
template <typename Call>
uae_u32 plugin_call(Call call)
{
	const std::lock_guard<std::mutex> lock(g_plugin_mutex);
	return call();
}

} // anonymous namespace

uae_u32 service_transport_native_code_denied_result(uae_u32 operation)
{
	return operation == 0 ? 0 : SERVICE_TRANSPORT_ERROR_UNAVAILABLE;
}

void service_transport_reset()
{
	const std::lock_guard<std::mutex> lock(g_plugin_mutex);
	if (g_plugin.loaded)
		g_plugin.reset();
}

// Guest RAM banks are stored in Amiga byte order, so a range inside one
// directly addressable bank can be handed to the plugin in place. Everything
// else is staged through the trap accessors: custom/IO and ROM banks, and all
// guest memory in indirect UAE-board mode, where the trap runs on a worker
// thread and must reach guest memory through the emulator.
static bool direct_readable(uaecptr address, uae_u32 bytes)
{
	return bytes && !trap_is_indirect() && real_address_allowed()
		&& valid_address(address, bytes);
}

// In-place output goes only to memory known to accept raw host writes: RTG
// VRAM (commit_output() marks it dirty), banks that map writes directly, and
// ZZ9000 VRAM. Everything else, ROM and banks whose put handlers filter
// writes included, is staged and written through those handlers.
static bool direct_writable(uaecptr address, uae_u32 bytes)
{
	if (!direct_readable(address, bytes))
		return false;
	const addrbank& bank = get_mem_bank(address);
	if (bank.flags & ABFLAG_RTG)
		return true;
	if (bank.baseaddr_direct_r)
		return bank.baseaddr_direct_w != nullptr;
	return zz9000_host_writable(address, bytes);
}

static const uae_u8* guest_input(TrapContext* ctx, uaecptr address, uae_u32 bytes)
{
	if (direct_readable(address, bytes))
		return get_real_address(address);
	t_input.resize(bytes);
	trap_get_bytes(ctx, t_input.data(), address, static_cast<int>(bytes));
	return t_input.data();
}

static uae_u8* guest_output(uaecptr address, uae_u32 bytes)
{
	if (direct_writable(address, bytes))
		return get_real_address(address);
	t_output.resize(bytes);
	return t_output.data();
}

static void commit_output(TrapContext* ctx, uaecptr address, const uae_u8* data, uae_u32 bytes)
{
	if (data != t_output.data()) {
		picasso_mark_host_write(data, bytes); // written in place
		return;
	}
	// Not trap_put_bytes() in direct mode: it would memcpy into any
	// directly readable bank, ROM included.
	if (trap_is_indirect()) {
		trap_put_bytes(ctx, data, address, static_cast<int>(bytes));
		return;
	}
	for (uae_u32 i = 0; i < bytes; ++i)
		put_byte(address + i, data[i]);
}

uae_u32 service_transport_uaelib(TrapContext* ctx, uae_u32 operation, uae_u32 arg2,
	uae_u32 arg3, uae_u32 arg4, uae_u32 arg5)
{
	{
		const std::lock_guard<std::mutex> lock(g_plugin_mutex);
		if (!ensure_plugin_loaded())
			return operation == 0 ? 0 : SERVICE_TRANSPORT_ERROR_UNAVAILABLE;
		if (operation == 0)
			return g_plugin.query();
	}
	try {
		switch (operation) {
		case 1:
			return plugin_call([&] { return g_plugin.create(arg2, arg3); });
		case 2: {
			if (arg4 > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES)
				return SERVICE_TRANSPORT_ERROR_CAPACITY;
			if (arg4 > 0xffffffffu - arg3)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			const uae_u8* input = guest_input(ctx, arg3, arg4);
			return plugin_call([&] { return g_plugin.submit(arg2, input, arg4); });
		}
		case 3: {
			if (arg4 > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES)
				return SERVICE_TRANSPORT_ERROR_CAPACITY;
			if (arg4 > 0xffffffffu - arg3)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			uae_u8* pixels = guest_output(arg3, arg4);
			const auto result = plugin_call([&] { return g_plugin.readback(arg2, pixels, arg4); });
			if (result != 0)
				return result;
			commit_output(ctx, arg3, pixels, arg4);
			return 0;
		}
		case 4:
			plugin_call([&] { g_plugin.destroy(arg2); return 0u; });
			return 0;
		case 5:
		case 6: {
			if (arg5 > 0xfffffff8u)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			const uaecptr reply = trap_get_long(ctx, arg5);
			const uae_u32 reply_bytes = trap_get_long(ctx, arg5 + 4);
			if (arg4 > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES
				|| reply_bytes > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES)
				return SERVICE_TRANSPORT_ERROR_CAPACITY;
			if (arg4 > 0xffffffffu - arg3 || reply_bytes > 0xffffffffu - reply)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			const uae_u8* input = guest_input(ctx, arg3, arg4);
			uae_u8* output;
			if (operation == 6) {
				// In-place reply (e.g. a locked RTG bitmap): never stage, since
				// a staged copy-back would overwrite bytes the plugin skipped.
				if (!direct_writable(reply, reply_bytes))
					return SERVICE_TRANSPORT_ERROR_UNAVAILABLE;
				output = get_real_address(reply);
			} else {
				output = guest_output(reply, reply_bytes);
			}
			const auto result = plugin_call([&] {
				return g_plugin.request(arg2, input, arg4, output, reply_bytes);
			});
			if (result != 0)
				return result;
			commit_output(ctx, reply, output, reply_bytes);
			return 0;
		}
		default:
			return SERVICE_TRANSPORT_ERROR_INVALID;
		}
	} catch (const std::bad_alloc&) {
		return SERVICE_TRANSPORT_ERROR_CAPACITY;
	}
}
