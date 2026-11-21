/* Native Windows host. WinUI runs out of process so the emulator keeps its
 * llvm-mingw toolchain. Only this thread dispatches settings/media calls. */
#include "sysconfig.h"
#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <sddl.h>
#include "sysdeps.h"
#include "native_ui_host.h"
#include "settings_bridge.h"
#include "options.h"
#include "uae.h"
#include "target.h"
#include "perf_monitor.h"
#include "gui/gui_handling.h"
#include "amiberry_actions_host.h"
#include "settings/amiberry_actions.h"
#include "settings/amiberry_settings.h"
#ifdef USE_IPC_SOCKET
#include "amiberry_ipc.h"
#endif
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

extern int emulating;

namespace {
using json = nlohmann::json;
bool settings_visible = false;
bool keyboard_visible = false;
unsigned settings_epoch = 0;
unsigned library_epoch = 0;
unsigned devices_epoch = 0;
HWND emulation_window = nullptr;
std::mutex deferred_mutex;
std::vector<std::string> deferred_messages;

// Display and dialog text from the core: UTF-8, except the configuration and
// media paths that older builds stored in the ANSI code page. Invalid bytes
// become U+FFFD instead of throwing out of a message box.
std::wstring wide(const std::string& text)
{
	if (text.empty()) return {};
	const std::string utf8 = windows_native_core_to_utf8(text);
	const int count = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
	if (!count) throw std::runtime_error("The text could not be converted to UTF-16.");
	std::wstring result(count, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), count);
	return result;
}

std::string utf8(const wchar_t* text)
{
	const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, nullptr, 0, nullptr, nullptr);
	if (count <= 0) throw std::runtime_error("The selected path cannot be represented as UTF-8.");
	std::string result(count, '\0');
	WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, result.data(), count, nullptr, nullptr);
	result.pop_back();
	return result;
}

// The unpredictable name is not an access control boundary: only the current
// user can connect, and remote clients are rejected independently.
PSECURITY_DESCRIPTOR pipe_security()
{
	HANDLE token = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
		throw std::runtime_error("Cannot read the Windows user token.");
	DWORD size = 0;
	GetTokenInformation(token, TokenUser, nullptr, 0, &size);
	std::vector<unsigned char> storage(size);
	const bool read = GetTokenInformation(token, TokenUser, storage.data(), size, &size) != FALSE;
	CloseHandle(token);
	if (!read) throw std::runtime_error("Cannot read the Windows user identity.");
	LPWSTR sid = nullptr;
	if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(storage.data())->User.Sid, &sid))
		throw std::runtime_error("Cannot identify the Windows pipe owner.");
	const std::wstring sddl = L"D:P(A;;GA;;;" + std::wstring(sid) + L")";
	LocalFree(sid);
	PSECURITY_DESCRIPTOR security = nullptr;
	if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &security, nullptr))
		throw std::runtime_error("Cannot secure the native UI connection.");
	return security;
}

class frontend_connection {
	HANDLE pipe = INVALID_HANDLE_VALUE;
	HANDLE process = nullptr;
	OVERLAPPED connect_io{}, read_io{}, write_io{};
	bool connected = false, connecting = false, reading = false, writing = false;
	bool started = false, failed = false;
	std::array<char, 16384> buffer{};
	std::string input, output;
	size_t written = 0;
	static constexpr size_t max_request = 4 * 1024 * 1024;

	void disconnected()
	{
		if (failed) return;
		failed = true;
		settings_visible = false;
		keyboard_visible = false;
		// Closing the Library owns the app lifetime. No detached emulator is
		// left running after its only native controller has gone away.
		uae_quit();
	}

	bool finish(OVERLAPPED& io, DWORD& transferred)
	{
		if (GetOverlappedResult(pipe, &io, &transferred, FALSE)) return true;
		if (GetLastError() != ERROR_IO_INCOMPLETE) disconnected();
		return false;
	}

	void begin_read()
	{
		ResetEvent(read_io.hEvent);
		DWORD count = 0;
		if (ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &count, &read_io)) {
			if (!count) { disconnected(); return; }
			input.append(buffer.data(), count);
		} else if (GetLastError() == ERROR_IO_PENDING) {
			reading = true;
		} else disconnected();
	}

	void write_response()
	{
		if (writing) {
			DWORD count = 0;
			if (!finish(write_io, count)) return;
			writing = false;
			if (!count) { disconnected(); return; }
			written += count;
		}
		if (written == output.size()) {
			output.clear();
			written = 0;
			return;
		}
		ResetEvent(write_io.hEvent);
		DWORD count = 0;
		if (WriteFile(pipe, output.data() + written, static_cast<DWORD>(output.size() - written), &count, &write_io)) {
			if (!count) { disconnected(); return; }
			written += count;
		} else if (GetLastError() == ERROR_IO_PENDING) writing = true;
		else disconnected();
	}

	json dispatch(const json& request)
	{
		const std::string op = request.at("op").get<std::string>();
		if (op == "start") {
			const int apply = request.value("apply", 0);
			if (windows_native_dispatch({{"op", "whdboot_status"}}).value("downloading", false))
				throw std::runtime_error("Wait for the WHDBoot download to finish, or cancel it before starting emulation.");
			if (apply < 0 || apply > 2) throw std::runtime_error("Unknown apply class.");
			if (emulating) {
				const auto id = amiberry_actions_apply_settings(static_cast<amiberry_apply_class>(apply));
				if (!id) throw std::runtime_error("The emulator could not accept the settings action.");
				amiberry_actions_run_pending();
				std::array<char, AMIBERRY_ACTIONS_PATH_MAX> error{};
				if (amiberry_actions_poll(id, error.data(), error.size()) != AMIBERRY_ACTION_DONE)
					throw std::runtime_error(error[0] ? error.data() : "The settings could not be applied.");
			}
			settings_visible = false;
			return nullptr;
		}
		if (op == "show") {
			if (emulating && !amiberry_actions_open_settings())
				throw std::runtime_error("The emulator could not open settings.");
			return nullptr;
		}
		if (op == "keyboard_toggle" || op == "keyboard_hide") {
			if (op == "keyboard_toggle") native_host_toggle_keyboard();
			else native_host_hide_keyboard();
			return nullptr;
		}
		json data = windows_native_dispatch(request);
		if (op == "status") {
			data["settingsOpen"] = settings_visible;
			data["settingsEpoch"] = settings_epoch;
			data["keyboardVisible"] = keyboard_visible;
			data["libraryEpoch"] = library_epoch;
			data["devicesEpoch"] = devices_epoch;
			data["keyboardOptions"] = {
				{"enabled", currprefs.vkbd_enabled}, {"numpad", currprefs.vkbd_numpad},
				{"language", currprefs.vkbd_language},
				{"opacity", currprefs.vkbd_transparency > 0 ? std::min(currprefs.vkbd_transparency, 100) / 100.0 : 0.85},
				{"capsLatches", currprefs.keyboard_mode > KB_UAE}};
		}
		return data;
	}

public:
	~frontend_connection()
	{
		if (pipe != INVALID_HANDLE_VALUE) {
			CancelIoEx(pipe, nullptr);
			DWORD ignored;
			if (connecting) GetOverlappedResult(pipe, &connect_io, &ignored, TRUE);
			if (reading) GetOverlappedResult(pipe, &read_io, &ignored, TRUE);
			if (writing) GetOverlappedResult(pipe, &write_io, &ignored, TRUE);
			CloseHandle(pipe);
		}
		if (process) CloseHandle(process);
		if (connect_io.hEvent) CloseHandle(connect_io.hEvent);
		if (read_io.hEvent) CloseHandle(read_io.hEvent);
		if (write_io.hEvent) CloseHandle(write_io.hEvent);
	}

	bool active() const { return started && !failed; }

	void start()
	{
		if (started) return;
		started = true;
		try {
			GUID nonce;
			if (FAILED(CoCreateGuid(&nonce))) throw std::runtime_error("Cannot create the native UI connection ID.");
			wchar_t id[40];
			StringFromGUID2(nonce, id, 40);
			const std::wstring name = L"amiberry-native-" + std::to_wstring(GetCurrentProcessId()) + L"-" + id;
			const std::wstring full_name = L"\\\\.\\pipe\\" + name;
			PSECURITY_DESCRIPTOR security = pipe_security();
			SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), security, FALSE};
			pipe = CreateNamedPipeW(full_name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
				1, 65536, 65536, 0, &attributes);
			LocalFree(security);
			if (pipe == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create the native UI pipe.");
			connect_io.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
			read_io.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
			write_io.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
			if (!connect_io.hEvent || !read_io.hEvent || !write_io.hEvent)
				throw std::runtime_error("Cannot create native UI I/O events.");
			if (ConnectNamedPipe(pipe, &connect_io)) connected = true;
			else {
				const DWORD error = GetLastError();
				if (error == ERROR_IO_PENDING) connecting = true;
				else if (error == ERROR_PIPE_CONNECTED) connected = true;
				else throw std::runtime_error("Cannot listen for the native UI.");
			}
			std::vector<wchar_t> executable(32768);
			const DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
			if (!length || length >= executable.size()) throw std::runtime_error("Cannot locate the native UI executable.");
			std::wstring directory(executable.data(), length);
			directory.resize(directory.find_last_of(L"\\/"));
			const std::wstring ui = directory + L"\\native-ui\\Amiberry.Windows.exe";
			std::wstring command = L"\"" + ui + L"\" --pipe \"" + name + L"\"";
			STARTUPINFOW startup{sizeof(STARTUPINFOW)};
			PROCESS_INFORMATION child{};
			if (!CreateProcessW(ui.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, directory.c_str(), &startup, &child))
				throw std::runtime_error("Cannot launch native-ui/Amiberry.Windows.exe. Install the complete native Windows build.");
			CloseHandle(child.hThread);
			process = child.hProcess;
		} catch (const std::exception& error) {
			native_host_message("Amiberry native UI", error.what());
			disconnected();
		}
	}

	void service()
	{
		if (!started || failed) return;
		if (process && WaitForSingleObject(process, 0) == WAIT_OBJECT_0) { disconnected(); return; }
		if (connecting) {
			DWORD count;
			if (!finish(connect_io, count)) return;
			connecting = false;
			connected = true;
		}
		if (!connected) return;
		// Bounded work per frame; no allocations or parsing when idle.
		for (int pass = 0; pass != 4 && !failed; ++pass) {
			if (!output.empty()) {
				write_response();
				if (!output.empty()) return;
			}
			if (reading) {
				DWORD count = 0;
				if (!finish(read_io, count)) return;
				reading = false;
				if (!count) { disconnected(); return; }
				input.append(buffer.data(), count);
			}
			if (input.size() > max_request) { disconnected(); return; }
			const size_t newline = input.find('\n');
			if (newline == std::string::npos) { begin_read(); continue; }
			try {
				const json request = json::parse(input.begin(), input.begin() + newline);
				output = json{{"ok", true}, {"data", dispatch(request)}}.dump();
			} catch (const std::exception& error) {
				// Strict: a string that is still not UTF-8 is reported, never rewritten.
				try {
					output = json{{"ok", false}, {"error", windows_native_core_to_utf8(error.what())}}.dump();
				} catch (const json::exception&) {
					output = json{{"ok", false}, {"error", "The native UI reply contained text that is not valid UTF-8."}}.dump();
				}
			}
			input.erase(0, newline + 1);
			output += '\n';
			written = 0;
		}
	}
};

frontend_connection& connection()
{
	static frontend_connection instance;
	return instance;
}

constexpr UINT menu_settings = 0xA100, menu_pause = 0xA110, menu_library = 0xA120, menu_keyboard = 0xA130;
LRESULT CALLBACK emulation_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR)
{
	if (message == WM_CLOSE) {
		if (MessageBoxW(window, L"Stop emulation and return to the Library?", L"Amiberry", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
			++library_epoch;
			amiberry_actions_stop();
		}
		return 0;
	}
	if (message == WM_SYSCOMMAND) {
		switch (wparam & 0xfff0) {
		case menu_settings: amiberry_actions_open_settings(); return 0;
		case menu_pause: {
			amiberry_actions_status status{};
			amiberry_actions_get_status(&status);
			if (status.paused) amiberry_actions_resume(); else amiberry_actions_pause();
			return 0;
		}
		case menu_library: ++library_epoch; amiberry_actions_stop(); return 0;
		case menu_keyboard: native_host_toggle_keyboard(); return 0;
		}
	}
	if (message == WM_NCDESTROY) {
		RemoveWindowSubclass(window, emulation_proc, 1);
		if (emulation_window == window) emulation_window = nullptr;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

void startup_notices()
{
	native_host_show_deferred_messages();
	std::string title, message;
	if (consume_startup_migration_notice(title, message)) native_host_message(title.c_str(), message.c_str());
	if (should_show_legacy_cleanup_prompt()) {
		const bool trash = legacy_cleanup_uses_trash();
		std::string failure;
		for (;;) {
			std::string body = "Your active paths already use the new locations. These leftover items can be ";
			body += trash ? "moved to the Recycle Bin:" : "archived:";
			for (const auto& item : get_legacy_cleanup_prompt_items()) body += "\n- " + item;
			body += failure;
			const std::wstring content = wide(body);
			const TASKDIALOG_BUTTON buttons[] = {
				{100, trash ? L"Move to Recycle Bin" : L"Archive old items"},
				{101, L"Keep for now"}, {102, L"Don't ask again"}};
			TASKDIALOGCONFIG dialog{};
			dialog.cbSize = sizeof(dialog);
			dialog.hwndParent = emulation_window;
			dialog.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SIZE_TO_CONTENT;
			dialog.pszWindowTitle = L"Amiberry";
			dialog.pszMainInstruction = L"Clean up files from the previous layout?";
			dialog.pszContent = content.c_str();
			dialog.cButtons = 3;
			dialog.pButtons = buttons;
			dialog.nDefaultButton = 101;
			int answer = 101;
			if (FAILED(TaskDialogIndirect(&dialog, &answer, nullptr, nullptr))) break;
			if (answer == 102) { dismiss_legacy_cleanup_prompt(); break; }
			if (answer != 100) { postpone_legacy_cleanup_prompt(); break; }
			std::vector<std::string> failed;
			if (cleanup_legacy_items(failed)) break;
			failure = "\n\nThese items could not be moved and were left in place:";
			for (const auto& item : failed) failure += "\n- " + item;
		}
	}
	if (perf_monitor_warning_pending()) {
		const int answer = MessageBoxW(emulation_window,
			L"The current settings run below full speed. Enable resolution autoswitch to reduce rendering cost without changing emulation accuracy?",
			L"Amiberry performance", MB_YESNO | MB_ICONWARNING);
		if (answer == IDYES) changed_prefs.gfx_autoresolution = 1;
		perf_monitor_clear_warning();
	}
}
} // namespace

void windows_native_service()
{
	// Some layer operations drain actions synchronously. They must not
	// recursively read a second request over the first request's reply.
	static bool servicing = false;
	if (servicing) return;
	struct guard { bool& value; guard(bool& v) : value(v) { value = true; } ~guard() { value = false; } } busy(servicing);
	connection().service();
}

void native_host_install_menus()
{
	SetCurrentProcessExplicitAppUserModelID(L"BlitterStudio.Amiberry.Native");
	connection().start();
}

void native_host_run_settings()
{
	settings_visible = true;
	++settings_epoch;
	startup_notices();
	while (settings_visible && connection().active()) {
		const unsigned long before = amiberry_actions_emulation_requests();
#ifdef USE_IPC_SOCKET
		Amiberry::IPC::IPCHandle();
#endif
		amiberry_actions_run_pending();
		// Servicing the pipe can close the settings (Start/Resume) or end the connection; leave before the SDL
		// wait below, which would otherwise swallow the focus and key events that belong to the resumed machine.
		if (amiberry_actions_emulation_requests() != before || !settings_visible || !connection().active()) break;
		native_host_show_deferred_messages();
		SDL_Event event;
		if (SDL_WaitEventTimeout(&event, 8)) {
			do {
				if (event.type == SDL_EVENT_QUIT) { uae_quit(); settings_visible = false; }
				else if (event.type == SDL_EVENT_JOYSTICK_ADDED || event.type == SDL_EVENT_JOYSTICK_REMOVED) {
					handle_joy_device_event(event.jdevice.which, event.type == SDL_EVENT_JOYSTICK_REMOVED);
					amiberry_settings_refresh_choices();
					++devices_epoch;
				}
			} while (SDL_PollEvent(&event));
		}
	}
	settings_visible = false;
	amiberry_actions_run_pending();
}

bool native_host_settings_open() { return settings_visible; }

void native_host_emulation_window_created(SDL_Window* window)
{
	emulation_window = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	if (!emulation_window) return;
	if (!SetWindowSubclass(emulation_window, emulation_proc, 1, 0)) {
		native_host_message_deferred("Could not attach the native Windows emulation commands.");
		return;
	}
	HMENU menu = GetSystemMenu(emulation_window, FALSE);
	AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(menu, MF_STRING, menu_settings, L"Machine settings\tF12");
	AppendMenuW(menu, MF_STRING, menu_pause, L"Pause / Resume");
	AppendMenuW(menu, MF_STRING, menu_keyboard, L"Amiga keyboard");
	AppendMenuW(menu, MF_STRING, menu_library, L"Return to Library");
}

void native_host_toggle_keyboard()
{
	if (!emulating || !currprefs.vkbd_enabled) return;
	keyboard_visible = !keyboard_visible;
	if (keyboard_visible)
		setmouseinactive_running(0);
}
void native_host_hide_keyboard() { keyboard_visible = false; }
// Desktop builds have no lifecycle autosave (the Mac host is the same).
void native_host_request_autosave() {}
void native_host_service_autosave() {}
std::string native_host_autosave_path() { return {}; }
native_host_between_frames::native_host_between_frames() = default;
native_host_between_frames::~native_host_between_frames() = default;

bool native_host_choose_file(native_host_file kind, const std::string& title,
	const std::string& start_path, std::string& chosen)
{
	const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	IFileDialog* dialog = nullptr;
	const bool save = kind == native_host_file::save_state;
	HRESULT result = CoCreateInstance(save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (SUCCEEDED(result)) {
		DWORD options;
		dialog->GetOptions(&options);
		dialog->SetOptions(options | FOS_FORCEFILESYSTEM | (save ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST));
		const std::wstring caption = wide(title);
		dialog->SetTitle(caption.c_str());
		const COMDLG_FILTERSPEC floppy[] = {{L"Amiga disk images", L"*.adf;*.adz;*.dms;*.ipf;*.scp;*.fdi;*.zip;*.7z"}, {L"All files", L"*.*"}};
		const COMDLG_FILTERSPEC cd[] = {{L"CD images", L"*.cue;*.iso;*.ccd;*.mds;*.chd;*.nrg"}, {L"All files", L"*.*"}};
		const COMDLG_FILTERSPEC state[] = {{L"Amiberry save states", L"*.uss"}, {L"All files", L"*.*"}};
		dialog->SetFileTypes(2, kind == native_host_file::floppy ? floppy : kind == native_host_file::cd ? cd : state);
		if (save) dialog->SetDefaultExtension(L"uss");
		if (!start_path.empty()) {
			std::wstring path = wide(start_path);
			IShellItem* folder = nullptr;
			if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES || !(GetFileAttributesW(path.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) {
				const size_t slash = path.find_last_of(L"\\/");
				if (slash != std::wstring::npos) path.resize(slash);
			}
			if (SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
				dialog->SetFolder(folder);
				folder->Release();
			}
		}
		result = dialog->Show(emulation_window);
		if (SUCCEEDED(result)) {
			IShellItem* item = nullptr;
			result = dialog->GetResult(&item);
			if (SUCCEEDED(result)) {
				PWSTR path = nullptr;
				result = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
				if (SUCCEEDED(result)) { chosen = utf8(path); CoTaskMemFree(path); }
				item->Release();
			}
		}
		dialog->Release();
	}
	if (SUCCEEDED(initialized)) CoUninitialize();
	if (FAILED(result) && result != HRESULT_FROM_WIN32(ERROR_CANCELLED))
		native_host_message("File picker", "Windows could not open the requested file picker.");
	return SUCCEEDED(result) && !chosen.empty();
}

void native_host_message(const char* title, const char* message)
{
	MessageBoxW(emulation_window, wide(message ? message : "").c_str(), wide(title ? title : "Amiberry").c_str(), MB_OK | MB_ICONINFORMATION);
}

void native_host_message_deferred(const char* message)
{
	const std::string text = message ? message : "";
	std::lock_guard<std::mutex> lock(deferred_mutex);
	for (const auto& existing : deferred_messages) if (existing == text) return;
	if (deferred_messages.size() < 8) deferred_messages.push_back(text);
}

void native_host_show_deferred_messages()
{
	std::vector<std::string> messages;
	{ std::lock_guard<std::mutex> lock(deferred_mutex); messages.swap(deferred_messages); }
	for (const auto& message : messages) native_host_message("Amiberry", message.c_str());
}
