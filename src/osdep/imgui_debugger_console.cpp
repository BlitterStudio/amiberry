#include "imgui_debugger_console.h"

#if !defined(AMIBERRY_MACOS) && !defined(__ANDROID__) && !defined(AMIBERRY_IOS)

#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "amiberry_gfx.h"
#include "debug.h"
namespace
{
	constexpr int kDebuggerWindowWidth = 980;
	constexpr int kDebuggerWindowHeight = 620;
	constexpr std::size_t kMaxTranscriptLines = 1000;
	constexpr std::size_t kInputBufferSize = 1024;

	struct DebuggerWindowState
	{
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		ImGuiContext* context = nullptr;
		std::thread::id owner_thread{};
		std::mutex mutex;
		std::deque<std::string> input_lines;
		std::vector<std::string> history;
		std::string transcript;
		bool closed = false;
		bool pending_autoscroll = true;
		bool focus_input = false;
		int history_index = -1;
		char input_buffer[kInputBufferSize]{};
	};

	DebuggerWindowState g_debugger_window;

	bool is_visible()
	{
		return g_debugger_window.window && (SDL_GetWindowFlags(g_debugger_window.window) & SDL_WINDOW_HIDDEN) == 0;
	}

	void trim_transcript_locked()
	{
		std::size_t line_count = static_cast<std::size_t>(
			std::count(g_debugger_window.transcript.begin(), g_debugger_window.transcript.end(), '\n'));
		while (line_count > kMaxTranscriptLines) {
			const std::size_t trim_pos = g_debugger_window.transcript.find('\n');
			if (trim_pos == std::string::npos) {
				g_debugger_window.transcript.clear();
				return;
			}
			g_debugger_window.transcript.erase(0, trim_pos + 1);
			line_count--;
		}
	}

	void append_transcript_locked(const char* text)
	{
		if (!text || !text[0]) {
			return;
		}
		g_debugger_window.transcript.append(text);
		trim_transcript_locked();
		g_debugger_window.pending_autoscroll = true;
	}

	void queue_line(const std::string& line)
	{
		std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
		g_debugger_window.input_lines.push_back(line);
		g_debugger_window.closed = false;
	}

	void request_window_close()
	{
		if (g_debugger_window.window) {
			SDL_HideWindow(g_debugger_window.window);
		}
		{
			std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
			g_debugger_window.input_lines.clear();
		}
		queue_line("x");
	}

	void request_emulator_quit()
	{
		if (g_debugger_window.window) {
			SDL_HideWindow(g_debugger_window.window);
		}
		{
			std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
			g_debugger_window.input_lines.clear();
		}
		queue_line("q");
	}

	bool should_exit_wait()
	{
		std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
		return g_debugger_window.closed || !debugger_active;
	}

	bool try_dequeue_line(char* out, const int maxlen, int* out_len)
	{
		std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
		if (g_debugger_window.input_lines.empty()) {
			return false;
		}

		std::string line = std::move(g_debugger_window.input_lines.front());
		g_debugger_window.input_lines.pop_front();

		const int len = std::min(maxlen - 1, static_cast<int>(line.size()));
		std::memcpy(out, line.c_str(), len);
		out[len] = 0;
		*out_len = len;
		return true;
	}

	int input_history_callback(ImGuiInputTextCallbackData* data)
	{
		auto* state = static_cast<DebuggerWindowState*>(data->UserData);
		if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory) {
			return 0;
		}

		const int previous_index = state->history_index;
		if (data->EventKey == ImGuiKey_UpArrow) {
			if (state->history.empty()) {
				return 0;
			}
			if (state->history_index < 0) {
				state->history_index = static_cast<int>(state->history.size()) - 1;
			} else if (state->history_index > 0) {
				state->history_index--;
			}
		} else if (data->EventKey == ImGuiKey_DownArrow) {
			if (state->history_index >= 0) {
				if (state->history_index < static_cast<int>(state->history.size()) - 1) {
					state->history_index++;
				} else {
					state->history_index = -1;
				}
			}
		}

		if (previous_index == state->history_index) {
			return 0;
		}

		data->DeleteChars(0, data->BufTextLen);
		if (state->history_index >= 0) {
			const std::string& entry = state->history[static_cast<std::size_t>(state->history_index)];
			data->InsertChars(0, entry.c_str());
		}
		return 0;
	}

	void submit_input()
	{
		std::string command;
		{
			std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
			command = g_debugger_window.input_buffer;
			if (!command.empty()) {
				if (g_debugger_window.history.empty()
					|| g_debugger_window.history.back() != command) {
					g_debugger_window.history.push_back(command);
				}
				append_transcript_locked((command + "\n").c_str());
			}
			g_debugger_window.history_index = -1;
			g_debugger_window.input_buffer[0] = 0;
			g_debugger_window.focus_input = true;
		}
		queue_line(command);
	}

	void apply_style()
	{
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.FrameRounding = 5.0f;
		style.GrabRounding = 5.0f;
		style.ScrollbarRounding = 6.0f;
		style.WindowPadding = ImVec2(14.0f, 14.0f);
		style.FramePadding = ImVec2(10.0f, 7.0f);
		style.ItemSpacing = ImVec2(10.0f, 8.0f);
		style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);

		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.10f, 0.11f, 1.0f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.0f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.19f, 1.0f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.25f, 1.0f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.23f, 0.25f, 0.29f, 1.0f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.27f, 0.36f, 1.0f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.34f, 0.45f, 1.0f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.24f, 0.33f, 1.0f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.27f, 0.36f, 1.0f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.34f, 0.45f, 1.0f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.24f, 0.33f, 1.0f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.28f, 0.31f, 0.35f, 1.0f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.31f, 0.35f, 1.0f);
	}

	bool ensure_window()
	{
		if (g_debugger_window.window) {
			return true;
		}

		Uint32 window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
#if !defined(__ANDROID__)
		window_flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
		g_debugger_window.window = SDL_CreateWindow("Amiberry Debugger",
			kDebuggerWindowWidth, kDebuggerWindowHeight, window_flags);
		if (!g_debugger_window.window) {
			write_log("imgui_debugger_console: SDL_CreateWindow failed: %s\n", SDL_GetError());
			return false;
		}

		SDL_SetWindowMinimumSize(g_debugger_window.window, 720, 420);
		g_debugger_window.renderer = SDL_CreateRenderer(g_debugger_window.window, NULL);
		if (!g_debugger_window.renderer) {
			write_log("imgui_debugger_console: SDL_CreateRenderer failed: %s\n", SDL_GetError());
			SDL_DestroyWindow(g_debugger_window.window);
			g_debugger_window.window = nullptr;
			return false;
		}
		SDL_SetRenderVSync(g_debugger_window.renderer, 1);

		g_debugger_window.context = ImGui::CreateContext();
		ImGui::SetCurrentContext(g_debugger_window.context);

		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;

		apply_style();
		ImGui_ImplSDL3_InitForSDLRenderer(g_debugger_window.window, g_debugger_window.renderer);
		ImGui_ImplSDLRenderer3_Init(g_debugger_window.renderer);

		g_debugger_window.owner_thread = std::this_thread::get_id();
		return true;
	}

	void begin_frame()
	{
		ImGui::SetCurrentContext(g_debugger_window.context);
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
	}

	void end_frame()
	{
		ImGui::Render();
		SDL_SetRenderDrawColor(g_debugger_window.renderer, 18, 20, 22, 255);
		SDL_RenderClear(g_debugger_window.renderer);
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_debugger_window.renderer);
		SDL_RenderPresent(g_debugger_window.renderer);
	}

	void render_window()
	{
		if (!g_debugger_window.context || !is_visible()) {
			return;
		}

		begin_frame();

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);

		const ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_MenuBar;

		bool clear_requested = false;
		bool copy_requested = false;
		bool follow_output = false;
		bool transcript_focused = false;
		bool input_focused = false;
		std::string transcript_snapshot;
		bool pending_autoscroll = false;
		bool focus_input = false;
		{
			std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
			transcript_snapshot = g_debugger_window.transcript;
			pending_autoscroll = g_debugger_window.pending_autoscroll;
			focus_input = g_debugger_window.focus_input;
		}

		ImGui::Begin("DebuggerRoot", nullptr, window_flags);
		if (ImGui::BeginMenuBar()) {
			if (ImGui::Button("Copy All")) {
				copy_requested = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear")) {
				clear_requested = true;
			}
			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
			ImGui::TextDisabled("Debugger paused");
			ImGui::EndMenuBar();
		}

		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_C)) {
			copy_requested = true;
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_L)) {
			clear_requested = true;
		}

		const float footer_height = ImGui::GetFrameHeightWithSpacing() * 2.0f + ImGui::GetStyle().FramePadding.y;
		if (ImGui::BeginChild("Transcript", ImVec2(0.0f, -footer_height),
			ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
			follow_output = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 2.0f;
			ImGui::TextUnformatted(transcript_snapshot.c_str());
			if (pending_autoscroll && follow_output) {
				ImGui::SetScrollHereY(1.0f);
			}
			if (ImGui::BeginPopupContextWindow("DebuggerTranscriptMenu")) {
				if (ImGui::MenuItem("Copy All", "Ctrl+Shift+C")) {
					copy_requested = true;
				}
				if (ImGui::MenuItem("Clear", "Ctrl+L")) {
					clear_requested = true;
				}
				ImGui::EndPopup();
			}
			transcript_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		}
		ImGui::EndChild();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Command");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-110.0f);
		if (focus_input) {
			ImGui::SetKeyboardFocusHere();
		}
		const ImGuiInputTextFlags input_flags =
			ImGuiInputTextFlags_EnterReturnsTrue
			| ImGuiInputTextFlags_CallbackHistory;
		if (ImGui::InputTextWithHint("##DebuggerCommand",
			"Enter debugger command and press Return",
			g_debugger_window.input_buffer,
			IM_ARRAYSIZE(g_debugger_window.input_buffer),
			input_flags,
			input_history_callback,
			&g_debugger_window)) {
			submit_input();
		}
		input_focused = ImGui::IsItemActive();
		ImGui::SameLine();
		if (ImGui::Button("Send")) {
			submit_input();
		}

		ImGui::Spacing();
		ImGui::TextDisabled("Up/Down recalls history. Ctrl+Shift+C copies transcript. Ctrl+L clears.");
		ImGui::End();

		{
			std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
			if (pending_autoscroll) {
				g_debugger_window.pending_autoscroll = false;
			}
			if (focus_input) {
				g_debugger_window.focus_input = false;
			}
		}

		if (copy_requested && !transcript_snapshot.empty()) {
			ImGui::SetClipboardText(transcript_snapshot.c_str());
		}
		if (clear_requested) {
			std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
			g_debugger_window.transcript.clear();
			g_debugger_window.pending_autoscroll = true;
		}
		if (transcript_focused && !input_focused && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C)) {
			ImGui::SetClipboardText(transcript_snapshot.c_str());
		}

		end_frame();
	}

	SDL_WindowID event_window_id(const SDL_Event& event)
	{
		if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
			return event.window.windowID;
		switch (event.type) {
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				return event.key.windowID;
			case SDL_EVENT_TEXT_EDITING:
				return event.edit.windowID;
			case SDL_EVENT_TEXT_INPUT:
				return event.text.windowID;
			case SDL_EVENT_MOUSE_MOTION:
				return event.motion.windowID;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
				return event.button.windowID;
			case SDL_EVENT_MOUSE_WHEEL:
				return event.wheel.windowID;
			case SDL_EVENT_FINGER_DOWN:
			case SDL_EVENT_FINGER_UP:
			case SDL_EVENT_FINGER_MOTION:
				return event.tfinger.windowID;
			default:
				return 0;
		}
	}

	void pump_events()
	{
		if (!g_debugger_window.context || !is_visible()) {
			return;
		}

		const SDL_WindowID our_window_id = SDL_GetWindowID(g_debugger_window.window);
		// Defer events that don't belong to the debugger window so the main emulator
		// loop still sees them when the breakpoint resumes. SDL_PushEvent inside the
		// SDL_PollEvent loop would just round-trip them back to us, so collect first
		// and re-push after draining.
		std::vector<SDL_Event> deferred;
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui::SetCurrentContext(g_debugger_window.context);
			ImGui_ImplSDL3_ProcessEvent(&event);

			if (event.type == SDL_EVENT_QUIT) {
				request_emulator_quit();
				continue;
			}
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
				if (event.window.windowID == our_window_id) {
					request_window_close();
				} else {
					deferred.push_back(event);
				}
				continue;
			}

			const SDL_WindowID target = event_window_id(event);
			if (target == our_window_id)
				continue;
			// Either targeted at another window or has no window association
			// (controller / audio device / etc.). Re-post so the emulator sees it.
			deferred.push_back(event);
		}

		for (auto& e : deferred) {
			SDL_PushEvent(&e);
		}

		render_window();
	}
}

bool imgui_debugger_console_supported()
{
	const char* driver = SDL_GetCurrentVideoDriver();
	if (!driver) {
		return false;
	}
	if (no_wm_detected) {
		return false;
	}
	if (strcmpi(driver, "KMSDRM") == 0 || strcmpi(driver, "dummy") == 0) {
		return false;
	}
	return true;
}

void imgui_debugger_console_open()
{
	if (!ensure_window()) {
		return;
	}

	const bool was_visible = is_visible();
	{
		std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
		g_debugger_window.input_lines.clear();
		g_debugger_window.closed = false;
		g_debugger_window.focus_input = true;
		g_debugger_window.history_index = -1;
		g_debugger_window.input_buffer[0] = 0;
		if (!was_visible) {
			g_debugger_window.transcript.clear();
			g_debugger_window.pending_autoscroll = true;
		}
	}

	SDL_ShowWindow(g_debugger_window.window);
	SDL_RaiseWindow(g_debugger_window.window);
	render_window();
}

void imgui_debugger_console_close()
{
	{
		std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
		g_debugger_window.input_lines.clear();
		g_debugger_window.closed = true;
	}
	if (g_debugger_window.window) {
		SDL_HideWindow(g_debugger_window.window);
	}
}

void imgui_debugger_console_activate()
{
	if (!g_debugger_window.window) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
		g_debugger_window.focus_input = true;
	}
	SDL_ShowWindow(g_debugger_window.window);
	SDL_RaiseWindow(g_debugger_window.window);
	render_window();
}

void imgui_debugger_console_write(const char* text)
{
	if (!text || !text[0]) {
		return;
	}
	std::thread::id owner;
	{
		std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
		append_transcript_locked(text);
		owner = g_debugger_window.owner_thread;
	}
	// SDL window queries are only safe on the thread that created the window;
	// only the owner thread is allowed to inspect window flags or render.
	// Writes from other threads are still visible: the input wait loop calls
	// pump_events()->render_window() every ~10ms and picks up the new transcript
	// snapshot under the mutex.
	if (std::this_thread::get_id() == owner && is_visible()) {
		render_window();
	}
}

bool imgui_debugger_console_has_input()
{
	std::lock_guard<std::mutex> lock(g_debugger_window.mutex);
	return !g_debugger_window.input_lines.empty();
}

char imgui_debugger_console_getch()
{
	return 0;
}

int imgui_debugger_console_get(char* out, const int maxlen)
{
	if (!out || maxlen <= 0) {
		return -1;
	}

	for (;;) {
		int len = 0;
		if (try_dequeue_line(out, maxlen, &len)) {
			return len;
		}
		if (should_exit_wait()) {
			return -1;
		}
		pump_events();
		SDL_Delay(10);
	}
}

#endif
