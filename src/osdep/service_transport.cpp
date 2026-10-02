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
	std::vector<uint8_t> input, output;
};

static PluginBinding g_plugin;

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

} // anonymous namespace

uae_u32 service_transport_native_code_denied_result(uae_u32 operation)
{
	return operation == 0 ? 0 : SERVICE_TRANSPORT_ERROR_UNAVAILABLE;
}

void service_transport_reset()
{
	if (g_plugin.loaded && g_plugin.reset) {
		g_plugin.reset();
	}
}

// Guest RAM banks are stored in Amiga byte order, so a range inside one
// directly addressable bank can be handed to the plugin in place. Other banks
// (custom/IO) fall back to byte accessors through a staging buffer.
static const uae_u8* guest_input(uaecptr address, uae_u32 bytes)
{
	if (!bytes)
		return g_plugin.input.data();
	if (valid_address(address, bytes))
		return get_real_address(address);
	g_plugin.input.resize(bytes);
	for (uae_u32 i = 0; i < bytes; ++i)
		g_plugin.input[i] = get_byte(address + i);
	return g_plugin.input.data();
}

static uae_u8* guest_output(uaecptr address, uae_u32 bytes)
{
	if (bytes && valid_address(address, bytes))
		return get_real_address(address);
	g_plugin.output.resize(bytes);
	return g_plugin.output.data();
}

static void commit_output(uaecptr address, const uae_u8* data, uae_u32 bytes)
{
	if (data != g_plugin.output.data())
		return; // written in place
	for (uae_u32 i = 0; i < bytes; ++i)
		put_byte(address + i, data[i]);
}

uae_u32 service_transport_uaelib(TrapContext* ctx, uae_u32 operation, uae_u32 arg2,
	uae_u32 arg3, uae_u32 arg4, uae_u32 arg5)
{
	(void)ctx;
	if (operation == 0)
		return ensure_plugin_loaded() ? g_plugin.query() : 0;
	if (!ensure_plugin_loaded())
		return SERVICE_TRANSPORT_ERROR_UNAVAILABLE;
	try {
		switch (operation) {
		case 1:
			return g_plugin.create(arg2, arg3);
		case 2: {
			if (arg4 > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES)
				return SERVICE_TRANSPORT_ERROR_CAPACITY;
			if (arg4 > 0xffffffffu - arg3)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			return g_plugin.submit(arg2, guest_input(arg3, arg4), arg4);
		}
		case 3: {
			if (arg4 > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES)
				return SERVICE_TRANSPORT_ERROR_CAPACITY;
			if (arg4 > 0xffffffffu - arg3)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			uae_u8* pixels = guest_output(arg3, arg4);
			const auto result = g_plugin.readback(arg2, pixels, arg4);
			if (result != 0)
				return result;
			commit_output(arg3, pixels, arg4);
			return 0;
		}
		case 4:
			g_plugin.destroy(arg2);
			return 0;
		case 5:
		case 6: {
			if (arg5 > 0xfffffff8u)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			const uaecptr reply = get_long(arg5);
			const uae_u32 reply_bytes = get_long(arg5 + 4);
			if (arg4 > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES
				|| reply_bytes > SERVICE_TRANSPORT_MAX_TRANSFER_BYTES)
				return SERVICE_TRANSPORT_ERROR_CAPACITY;
			if (arg4 > 0xffffffffu - arg3 || reply_bytes > 0xffffffffu - reply)
				return SERVICE_TRANSPORT_ERROR_INVALID;
			const uae_u8* input = guest_input(arg3, arg4);
			uae_u8* output;
			if (operation == 6) {
				// In-place reply (e.g. a locked RTG bitmap): never stage, since
				// a staged copy-back would overwrite bytes the plugin skipped.
				if (!reply_bytes || !valid_address(reply, reply_bytes))
					return SERVICE_TRANSPORT_ERROR_UNAVAILABLE;
				output = get_real_address(reply);
			} else {
				output = guest_output(reply, reply_bytes);
			}
			const auto result = g_plugin.request(arg2, input, arg4, output, reply_bytes);
			if (result != 0)
				return result;
			if (operation == 6)
				picasso_mark_host_write(output, reply_bytes);
			else
				commit_output(reply, output, reply_bytes);
			return 0;
		}
		default:
			return SERVICE_TRANSPORT_ERROR_INVALID;
		}
	} catch (const std::bad_alloc&) {
		return SERVICE_TRANSPORT_ERROR_CAPACITY;
	}
}
