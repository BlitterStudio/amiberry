/*
 * The Android app's in-emulation menu (see android_overlay.h): the loop
 * that runs while the Compose overlay shows, and the JNI the overlay calls
 * (com.blitterstudio.amiberry.overlay.OverlayNative). The JNI is a thin
 * layer over the runtime actions and the settings layer: actions are posted
 * from any thread; the settings session and the prefs are read and written
 * on the emulator thread only, which the calls reach with
 * SDL_RunOnMainThread from the overlay's background threads (never from the
 * Android UI thread, which must not wait for the emulator).
 */
#include "sysconfig.h"
#include "sysdeps.h"

#include "android_overlay.h"

#include "options.h"
#include "uae.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "target.h"
#include "imgui_osk.h"
#include "gui/gui_handling.h"
#include "amiberry_actions_host.h"
#include "settings/amiberry_actions.h"
#include "settings/amiberry_settings.h"
#include "custom.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_system.h>
#include <jni.h>

#include <atomic>
#include <memory>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

extern int gui_active;

namespace {

// Messages to AmiberryActivity.onUnhandledMessage (SDLActivity's
// COMMAND_USER range starts at 0x8000).
constexpr Uint32 command_show_overlay = 0x8001;
constexpr Uint32 command_hide_overlay = 0x8002;

// OverlayNative.CLOSE_*: how the overlay asked the loop to end.
constexpr int close_none = -1;
constexpr int close_resume = 0;
constexpr int close_advanced = 1;
constexpr int close_keyboard = 2;

std::atomic<bool> enabled{true};
std::atomic<int> close_request{close_none};
// Show keyboard was chosen; android_overlay_service() shows it.
std::atomic<bool> keyboard_requested{false};

// The TV remote as the port 2 joystick: OverlayNative.REMOTE_* bits.
constexpr unsigned remote_up = 1;
constexpr unsigned remote_down = 2;
constexpr unsigned remote_left = 4;
constexpr unsigned remote_right = 8;
constexpr unsigned remote_fire = 16;
constexpr unsigned remote_all = remote_up | remote_down | remote_left | remote_right | remote_fire;
std::atomic<bool> remote_joystick{false};
// Held now, and pressed since the last pump (a tap may end before it).
std::atomic<unsigned> remote_held{0};
std::atomic<unsigned> remote_taps{0};
// Emulator thread: what the joystick shows, and taps kept until the next frame.
unsigned remote_applied = 0;
unsigned remote_latched = 0;
uae_u32 remote_latched_frame = 0;

bool is_window_size_event(const SDL_Event& event)
{
	return event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
}

// A call handed to the emulator thread. The caller may stop waiting (the
// emulator thread is blocked while the app is in the background); the
// callback then finds no work and only drops its reference, so it never
// touches the caller's stack frame.
struct emulator_call {
	std::mutex lock;
	std::condition_variable finished;
	std::function<void()> work;
	bool done = false;
};

constexpr auto emulator_call_timeout = std::chrono::seconds(30);

// Runs work on the emulator thread and waits for it, at most
// emulator_call_timeout; false when it did not run. The loop below (and
// the emulation's handle_events) pumps SDL events, which runs it.
bool on_emulator_thread(std::function<void()> work)
{
	auto call = std::make_shared<emulator_call>();
	call->work = std::move(work);
	auto* holder = new std::shared_ptr<emulator_call>(call);
	if (!SDL_RunOnMainThread([](void* data) {
			const std::unique_ptr<std::shared_ptr<emulator_call>> held(static_cast<std::shared_ptr<emulator_call>*>(data));
			emulator_call& c = **held;
			std::lock_guard<std::mutex> guard(c.lock);
			if (c.work)
				c.work();
			c.done = true;
			c.finished.notify_all();
		}, holder, false)) {
		delete holder;
		return false;
	}
	std::unique_lock<std::mutex> guard(call->lock);
	if (call->finished.wait_for(guard, emulator_call_timeout, [&] { return call->done; }))
		return true;
	// Too late: the work (and what it captured by reference) is dropped.
	call->work = nullptr;
	return false;
}

// Java strings carry UTF-16; paths and labels are UTF-8. The modified
// UTF-8 of GetStringUTFChars/NewStringUTF differs for characters outside
// the BMP, so both directions go through String's own UTF-8 conversion.
std::string from_java(JNIEnv* env, jstring text)
{
	if (!text)
		return {};
	jclass string_class = env->FindClass("java/lang/String");
	jmethodID get_bytes = env->GetMethodID(string_class, "getBytes", "(Ljava/lang/String;)[B");
	jstring charset = env->NewStringUTF("UTF-8");
	auto bytes = static_cast<jbyteArray>(env->CallObjectMethod(text, get_bytes, charset));
	std::string result;
	if (bytes) {
		const jsize length = env->GetArrayLength(bytes);
		result.resize(static_cast<size_t>(length));
		env->GetByteArrayRegion(bytes, 0, length, reinterpret_cast<jbyte*>(result.data()));
		env->DeleteLocalRef(bytes);
	}
	env->DeleteLocalRef(charset);
	env->DeleteLocalRef(string_class);
	return result;
}

jstring to_java(JNIEnv* env, const std::string& text)
{
	jclass string_class = env->FindClass("java/lang/String");
	jmethodID constructor = env->GetMethodID(string_class, "<init>", "([BLjava/lang/String;)V");
	jbyteArray bytes = env->NewByteArray(static_cast<jsize>(text.size()));
	env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(text.size()), reinterpret_cast<const jbyte*>(text.data()));
	jstring charset = env->NewStringUTF("UTF-8");
	auto result = static_cast<jstring>(env->NewObject(string_class, constructor, bytes, charset));
	env->DeleteLocalRef(charset);
	env->DeleteLocalRef(bytes);
	env->DeleteLocalRef(string_class);
	return result;
}

jobjectArray to_java(JNIEnv* env, const std::vector<std::string>& texts)
{
	jclass string_class = env->FindClass("java/lang/String");
	jobjectArray result = env->NewObjectArray(static_cast<jsize>(texts.size()), string_class, nullptr);
	for (size_t i = 0; i < texts.size(); i++) {
		jstring text = to_java(env, texts[i]);
		env->SetObjectArrayElement(result, static_cast<jsize>(i), text);
		env->DeleteLocalRef(text);
	}
	env->DeleteLocalRef(string_class);
	return result;
}

std::vector<std::string> from_java(JNIEnv* env, jobjectArray texts)
{
	std::vector<std::string> result;
	if (!texts)
		return result;
	const jsize count = env->GetArrayLength(texts);
	for (jsize i = 0; i < count; i++) {
		auto text = static_cast<jstring>(env->GetObjectArrayElement(texts, i));
		result.push_back(from_java(env, text));
		env->DeleteLocalRef(text);
	}
	return result;
}

std::string setting_value(const amiberry_settings_session* session, const std::string& id, bool& found)
{
	char value[AMIBERRY_SETTINGS_VALUE_MAX] = {};
	found = amiberry_settings_get(session, id.c_str(), value, sizeof value) == AMIBERRY_SETTINGS_OK;
	return found ? value : "";
}

} // namespace

bool android_overlay_enabled()
{
	return enabled.load();
}

android_overlay_result android_overlay_run()
{
	close_request.store(close_none);
	SDL_SendAndroidMessage(command_show_overlay, 0);

	// The emulation handles a rotation once it runs again: the last size
	// change waits for it.
	SDL_Event resize{};
	bool resized = false;
	int result = close_none;
	while (result == close_none) {
		// As the built-in GUI's loop: a reset, quit or state load the menu
		// posted is carried out by the emulation loop, so the menu closes.
		const unsigned long requests_before = amiberry_actions_emulation_requests();
		amiberry_actions_run_pending();
		if (amiberry_actions_emulation_requests() != requests_before) {
			result = close_resume;
			break;
		}
		result = close_request.load();
		if (result != close_none)
			break;

		// Waiting also runs the overlay's SDL_RunOnMainThread calls. Input
		// is the overlay's while it shows (it has the Android focus); what
		// still reaches SDL is dropped, as under the built-in GUI.
		SDL_Event event;
		if (!SDL_WaitEventTimeout(&event, 16))
			continue;
		do {
			if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_TERMINATING) {
				uae_quit();
				result = close_resume;
			} else if (event.type == SDL_EVENT_JOYSTICK_ADDED || event.type == SDL_EVENT_JOYSTICK_REMOVED) {
				// As run_gui: controllers plugged in meanwhile are picked up.
				handle_joy_device_event(event.jdevice.which, event.type == SDL_EVENT_JOYSTICK_REMOVED);
			} else if (is_window_size_event(event)) {
				resize = event;
				resized = true;
			}
		} while (SDL_PollEvent(&event));
	}

	// What the overlay posted as it closed (Pause, Quit) runs now, so the
	// emulation loop sees it before it resumes.
	amiberry_actions_run_pending();
	SDL_SendAndroidMessage(command_hide_overlay, 0);
	if (resized)
		SDL_PushEvent(&resize);
	if (result == close_keyboard)
		keyboard_requested.store(true);
	return result == close_advanced ? android_overlay_result::advanced : android_overlay_result::resume;
}

void android_overlay_service()
{
	if (!keyboard_requested.load() || pause_emulation)
		return;
	// The keyboard switch the menu may just have turned on reaches the
	// running configuration at a frame's configuration check; wait for it.
	if (!currprefs.vkbd_enabled) {
		if (!changed_prefs.vkbd_enabled)
			keyboard_requested.store(false);
		return;
	}
	keyboard_requested.store(false);
	if (!imgui_osk_is_active())
		inputdevice_add_inputcode(AKS_OSK, 1, nullptr);
}

void android_remote_joystick_pump()
{
	const unsigned taps = remote_taps.exchange(0);
	if (taps) {
		remote_latched |= taps;
		remote_latched_frame = vsync_counter;
	} else if (remote_latched && vsync_counter != remote_latched_frame) {
		remote_latched = 0;
	}
	const unsigned wanted = remote_joystick.load() ? (remote_held.load() | remote_latched) : 0;
	const unsigned changed = wanted ^ remote_applied;
	if (!changed)
		return;
	static constexpr struct { unsigned bit; int event; } events[] = {
		{ remote_up, INPUTEVENT_JOY2_UP },
		{ remote_down, INPUTEVENT_JOY2_DOWN },
		{ remote_left, INPUTEVENT_JOY2_LEFT },
		{ remote_right, INPUTEVENT_JOY2_RIGHT },
		{ remote_fire, INPUTEVENT_JOY2_FIRE_BUTTON },
	};
	for (const auto& e : events) {
		if (changed & e.bit)
			send_input_event(e.event, (wanted & e.bit) ? 1 : 0, 1, 0);
	}
	remote_applied = wanted;
}

#define OVERLAY_JNI(name) Java_com_blitterstudio_amiberry_overlay_OverlayNative_##name

extern "C" {

JNIEXPORT void JNICALL OVERLAY_JNI(setEnabled)(JNIEnv*, jclass, jboolean on)
{
	enabled.store(on == JNI_TRUE);
}

JNIEXPORT void JNICALL OVERLAY_JNI(close)(JNIEnv*, jclass, jint how)
{
	if (how == close_resume || how == close_advanced || how == close_keyboard)
		close_request.store(how);
}

// A TV with no gamepad: the remote's D-pad and OK drive port 2. Any thread.
JNIEXPORT void JNICALL OVERLAY_JNI(setRemoteJoystick)(JNIEnv*, jclass, jboolean on)
{
	remote_joystick.store(on == JNI_TRUE);
	if (on != JNI_TRUE) {
		// The pump releases what the joystick still shows.
		remote_held.store(0);
		remote_taps.store(0);
	}
}

// A remote key (one REMOTE_* bit) while remote_joystick is on: true when it
// moves the joystick. A press is not taken while the built-in GUI is open,
// where the D-pad moves its focus; a release always is, so none sticks.
JNIEXPORT jboolean JNICALL OVERLAY_JNI(remoteKey)(JNIEnv*, jclass, jint button, jboolean down)
{
	const unsigned bit = static_cast<unsigned>(button) & remote_all;
	if (!bit)
		return JNI_FALSE;
	if (down != JNI_TRUE) {
		remote_held.fetch_and(~bit);
		return JNI_TRUE;
	}
	if (!remote_joystick.load() || gui_active > 0)
		return JNI_FALSE;
	remote_held.fetch_or(bit);
	remote_taps.fetch_or(bit);
	return JNI_TRUE;
}

// Runtime actions: any thread. The id is 0 when the action was rejected.

JNIEXPORT jlong JNICALL OVERLAY_JNI(insertFloppy)(JNIEnv* env, jclass, jint drive, jstring path)
{
	return static_cast<jlong>(amiberry_actions_insert_floppy(drive, from_java(env, path).c_str()));
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(ejectFloppy)(JNIEnv*, jclass, jint drive)
{
	return static_cast<jlong>(amiberry_actions_eject_floppy(drive));
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(insertCd)(JNIEnv* env, jclass, jstring path)
{
	return static_cast<jlong>(amiberry_actions_insert_cd(from_java(env, path).c_str()));
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(ejectCd)(JNIEnv*, jclass)
{
	return static_cast<jlong>(amiberry_actions_eject_cd());
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(insertSwapperEntry)(JNIEnv*, jclass, jint index, jint drive)
{
	return static_cast<jlong>(amiberry_actions_insert_swapper_entry(index, drive));
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(reset)(JNIEnv*, jclass, jboolean hard)
{
	return static_cast<jlong>(amiberry_actions_reset(hard == JNI_TRUE, false));
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(pause)(JNIEnv*, jclass)
{
	return static_cast<jlong>(amiberry_actions_pause());
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(quit)(JNIEnv*, jclass)
{
	return static_cast<jlong>(amiberry_actions_quit());
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(saveState)(JNIEnv* env, jclass, jstring path)
{
	return static_cast<jlong>(amiberry_actions_save_state(from_java(env, path).c_str()));
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(loadState)(JNIEnv* env, jclass, jstring path)
{
	return static_cast<jlong>(amiberry_actions_load_state(from_java(env, path).c_str()));
}

JNIEXPORT jlong JNICALL OVERLAY_JNI(screenshot)(JNIEnv* env, jclass, jstring path)
{
	return static_cast<jlong>(amiberry_actions_screenshot(from_java(env, path).c_str()));
}

// OverlayNative.RESULT_*: amiberry_action_result.
JNIEXPORT jint JNICALL OVERLAY_JNI(poll)(JNIEnv*, jclass, jlong id)
{
	return static_cast<jint>(amiberry_actions_poll(static_cast<amiberry_action_id>(id), nullptr, 0));
}

// What the menu shows of the status snapshot (the media), in the order
// OverlayStatus.parse reads it.
JNIEXPORT jobjectArray JNICALL OVERLAY_JNI(status)(JNIEnv* env, jclass)
{
	// About 20 KB of paths: the heap, not a JNI thread's stack.
	const auto status = std::make_unique<amiberry_actions_status>();
	amiberry_actions_get_status(status.get());
	std::vector<std::string> fields = {
		std::to_string(status->cd_activity),
		status->cd_path,
	};
	for (const auto& drive : status->floppy) {
		fields.emplace_back(drive.disabled ? "1" : "0");
		fields.emplace_back(drive.media_path);
	}
	return to_java(env, fields);
}

JNIEXPORT jobjectArray JNICALL OVERLAY_JNI(swapperEntries)(JNIEnv* env, jclass)
{
	std::vector<std::string> entries;
	char path[AMIBERRY_ACTIONS_PATH_MAX];
	for (int i = 0; i < AMIBERRY_ACTIONS_SWAPPER_ENTRIES; i++) {
		amiberry_actions_get_swapper_entry(i, path, sizeof path);
		entries.emplace_back(path);
	}
	return to_java(env, entries);
}

// Emulator-thread calls. Each returns null when the emulator thread could
// not be reached.

// For each id the pending value and whether the layer offers the option
// now ("1"/"0"); an unknown id reads as an unavailable empty value.
JNIEXPORT jobjectArray JNICALL OVERLAY_JNI(readSettings)(JNIEnv* env, jclass, jobjectArray ids)
{
	const std::vector<std::string> wanted = from_java(env, ids);
	std::vector<std::string> fields;
	const bool ran = on_emulator_thread([&] {
		amiberry_settings_session* session = amiberry_settings_session_open();
		if (!session)
			return;
		for (const auto& id : wanted) {
			bool found = false;
			fields.push_back(setting_value(session, id, found));
			fields.emplace_back(found && amiberry_settings_is_available(session, id.c_str()) ? "1" : "0");
		}
		amiberry_settings_session_close(session);
	});
	if (!ran || fields.size() != wanted.size() * 2)
		return nullptr;
	return to_java(env, fields);
}

// The choices the layer offers for a CHOICE option now, as value and label
// pairs (the shaders folder's presets, for amiberry.shader).
JNIEXPORT jobjectArray JNICALL OVERLAY_JNI(settingChoices)(JNIEnv* env, jclass, jstring id)
{
	const std::string option = from_java(env, id);
	std::vector<std::string> fields;
	const bool ran = on_emulator_thread([&] {
		amiberry_settings_session* session = amiberry_settings_session_open();
		if (!session)
			return;
		const amiberry_setting_choice* choices = nullptr;
		const size_t count = amiberry_settings_choices(session, option.c_str(), &choices);
		for (size_t i = 0; i < count && choices; i++) {
			if (!amiberry_settings_choice_is_available(session, option.c_str(), choices[i].value))
				continue;
			fields.emplace_back(choices[i].value ? choices[i].value : "");
			fields.emplace_back(choices[i].label ? choices[i].label : "");
		}
		amiberry_settings_session_close(session);
	});
	return ran ? to_java(env, fields) : nullptr;
}

// Sets options as the settings editors do (one session, committed together)
// and has the running emulation pick them up; together, so swapping the
// port devices is not refused halfway. The apply class committed, or -1
// when a value was refused or the emulator thread could not be reached.
JNIEXPORT jint JNICALL OVERLAY_JNI(changeSettings)(JNIEnv* env, jclass, jobjectArray ids, jobjectArray values)
{
	const std::vector<std::string> options = from_java(env, ids);
	const std::vector<std::string> texts = from_java(env, values);
	if (options.empty() || options.size() != texts.size())
		return -1;
	int result = -1;
	on_emulator_thread([&] {
		amiberry_settings_session* session = amiberry_settings_session_open();
		if (!session)
			return;
		bool accepted = true;
		for (size_t i = 0; i < options.size() && accepted; i++)
			accepted = amiberry_settings_set(session, options[i].c_str(), texts[i].c_str()) == AMIBERRY_SETTINGS_OK;
		if (accepted) {
			bool changed = false;
			const amiberry_apply_class apply = amiberry_settings_commit(session, &changed);
			if (changed)
				amiberry_actions_apply_settings(apply);
			result = apply;
		}
		amiberry_settings_session_close(session);
	});
	return result;
}

// What the built-in GUI names state slots after (gui_update): the state and
// screenshot folders, DF0's image, the CD image, the WHDLoad game, the last
// configuration, and the machine's description for a session with none of
// them. StateSlots builds the slot paths from these.
JNIEXPORT jobjectArray JNICALL OVERLAY_JNI(stateNameSources)(JNIEnv* env, jclass)
{
	std::vector<std::string> fields;
	const bool ran = on_emulator_thread([&] {
		fields = {
			get_savestate_path(),
			get_screenshot_path(),
			currprefs.floppyslots[0].df,
			currprefs.cdslots[0].inuse ? currprefs.cdslots[0].name : "",
			whdload_prefs.whdload_filename,
			last_active_config,
			currprefs.description,
		};
	});
	return ran ? to_java(env, fields) : nullptr;
}

} // extern "C"
