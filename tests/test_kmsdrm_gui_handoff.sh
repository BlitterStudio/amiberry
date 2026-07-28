#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/gui/main_window.cpp"
platform_init_file="src/osdep/amiberry_platform_internal_host.h"
gfx_window_file="src/osdep/gfx_window.cpp"
opengl_renderer_file="src/osdep/opengl_renderer.cpp"
drawing_file="src/drawing.cpp"
xwin_file="src/include/xwin.h"
display_panel_file="src/osdep/imgui/display.cpp"
gui_lifecycle_file="src/osdep/amiberry_gui.cpp"
handoff_guard='if (gui_running || !kmsdrm_detected) {'

handoff_guard_count=$(grep -F -c "$handoff_guard" "$source_file" || true)
if [ "$handoff_guard_count" -ne 1 ]; then
	echo "GUI backend dispatch must use one late KMSDRM handoff guard" >&2
	exit 1
fi

if ! awk '
	/void run_gui\(\)/ { in_run_gui = 1 }
	in_run_gui && /\/\/ Rendering/ { in_render_boundary = 1 }
	in_render_boundary && /ImGui::Render\(\);/ {
		imgui_rendered = 1
		next
	}
	in_render_boundary && /if \(gui_running \|\| !kmsdrm_detected\) \{/ {
		if (!imgui_rendered || guard_seen)
			exit 1
		guard_seen = 1
		guard_depth = depth
	}
	in_render_boundary {
		line = $0
		opens = gsub(/\{/, "", line)
		line = $0
		closes = gsub(/\}/, "", line)

		if (/vk->render_gui_frame\(ImGui::GetDrawData\(\)\)/ ||
			/glViewport\(0, 0,/ ||
			/glClearColor\(0.45f, 0.55f, 0.60f, 1.00f\)/ ||
			/glClear\(GL_COLOR_BUFFER_BIT\)/ ||
			/ImGui_ImplOpenGL3_RenderDrawData\(ImGui::GetDrawData\(\)\)/ ||
			/SDL_GL_SwapWindow\(mon->gui_window\)/ ||
			/SDL_SetRenderScale\(mon->gui_renderer, render_scale_x, render_scale_y\)/ ||
			/SDL_SetRenderDrawColor\(mon->gui_renderer,/ ||
			/SDL_RenderClear\(mon->gui_renderer\)/ ||
			/ImGui_ImplSDLRenderer3_RenderDrawData\(ImGui::GetDrawData\(\), mon->gui_renderer\)/ ||
			/SDL_RenderPresent\(mon->gui_renderer\)/) {
			if (!guard_seen || guard_closed || depth <= guard_depth)
				exit 1
			backend_dispatches++
		}

		depth += opens - closes
		if (guard_seen && !guard_closed && depth == guard_depth)
			guard_closed = 1
	}
	in_run_gui && /amiberry_gui_halt\(\);/ {
		exit imgui_rendered && guard_seen && guard_closed && backend_dispatches == 11 ? 0 : 1
	}
	END {
		if (!in_run_gui)
			exit 1
	}
' "$source_file"; then
	echo "ImGui frame completion must remain unconditional and the late guard must enclose every GUI backend dispatch" >&2
	exit 1
fi

if ! awk '
	/void run_gui\(\)/ { in_run_gui = 1 }
	in_run_gui && /if \(gui_running \|\| !kmsdrm_detected\) \{/ { guard_seen = 1 }
	in_run_gui && /gui_running = false;/ {
		exit_routes++
		if (guard_seen)
			exit 1
	}
	in_run_gui && /amiberry_gui_halt\(\);/ {
		exit exit_routes > 0 && guard_seen ? 0 : 1
	}
' "$source_file"; then
	echo "GUI exit routes must converge on the single guard after all exit decisions" >&2
	exit 1
fi

if ! awk '
	/void gui_restart\(\)/ { in_restart = 1 }
	in_restart && /gui_running = false;/ { restart_clears_gui = 1 }
	in_restart && /^}/ { exit restart_clears_gui ? 0 : 1 }
	END { if (!in_restart) exit 1 }
' "$source_file"; then
	echo "External GUI restart must continue to converge through gui_running" >&2
	exit 1
fi

dispatch_eligible()
{
	local gui_is_running=$1
	local kmsdrm_is_active=$2

	if (( gui_is_running || ! kmsdrm_is_active )); then
		echo 1
	else
		echo 0
	fi
}

if [ "$(dispatch_eligible 0 1)" -ne 0 ]; then
	echo "Terminal KMSDRM frames must skip GUI backend dispatch" >&2
	exit 1
fi

if [ "$(dispatch_eligible 1 1)" -ne 1 ]; then
	echo "Active KMSDRM GUI frames must remain eligible for backend dispatch" >&2
	exit 1
fi

if [ "$(dispatch_eligible 0 0)" -ne 1 ]; then
	echo "Non-KMSDRM terminal GUI frames must remain eligible for backend dispatch" >&2
	exit 1
fi

if ! awk '
	/static inline bool osdep_platform_init_sdl\(\)/ { in_init = 1 }
	in_init && /SDL_GetCurrentVideoDriver\(\)/ { driver_queried = 1 }
	in_init && /SDL_strcasecmp\(video_driver, "kmsdrm"\) == 0/ {
		if (!driver_queried)
			exit 1
		in_kmsdrm_policy = 1
	}
	in_kmsdrm_policy && /SDL_SetHintWithPriority\(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1", SDL_HINT_OVERRIDE\)/ {
		double_buffer_forced = 1
	}
	in_init && /return true;/ { exit double_buffer_forced ? 0 : 1 }
	END { if (!in_init) exit 1 }
' "$platform_init_file"; then
	echo "KMSDRM must force double-buffered presentation so submitted page flips drain before handoff" >&2
	exit 1
fi

if ! awk '
	/void OpenGLRenderer::update_vsync\(int monid\)/ { in_update_vsync = 1 }
	in_update_vsync && /if \(kmsdrm_detected\) \{/ { in_kmsdrm_policy = 1 }
	in_kmsdrm_policy && /interval = SDL_GetVersion\(\) < SDL_VERSIONNUM\(3, 4, 0\) \? 0 : 1;/ {
		versioned_interval = 1
	}
	in_kmsdrm_policy && /^	}/ {
		exit versioned_interval ? 0 : 1
	}
	END { if (!in_update_vsync) exit 1 }
' "$opengl_renderer_file"; then
	echo "KMSDRM must use interval 0 for SDL 3.2.x and retain interval 1 for SDL 3.4+" >&2
	exit 1
fi

if ! grep -F -q 'static std::atomic<bool> hw_vsync_cached_presentation_blocking{false};' "$drawing_file" ||
	! grep -F -q 'hw_vsync_cached_presentation_blocking.store(blocking, std::memory_order_relaxed);' "$drawing_file" ||
	! grep -F -q 'extern void amiberry_hw_vsync_pacing_set_blocking(bool blocking);' "$xwin_file" ||
	! grep -F -q 'void amiberry_hw_vsync_pacing_set_blocking(const bool) {}' "$drawing_file"; then
	echo "Hardware pacing must expose an atomic blocking-presentation capability with a Libretro stub" >&2
	exit 1
fi

if ! awk '
	/static bool amiberry_hw_vsync_pacing_ok\(void\)/ { in_pacing_policy = 1 }
	in_pacing_policy && /if \(!hw_vsync_cached_presentation_blocking.load\(std::memory_order_relaxed\)\)/ {
		blocking_gate = 1
		next
	}
	blocking_gate && /return false;/ { exit 0 }
	in_pacing_policy && /^}/ { exit 1 }
	END { if (!in_pacing_policy) exit 1 }
' "$drawing_file"; then
	echo "Hardware pacing must require the cached blocking-presentation capability" >&2
	exit 1
fi

if ! awk '
	/void OpenGLRenderer::update_vsync\(int monid\)/ { in_update_vsync = 1 }
	in_update_vsync && /if \(m_vsync.current_interval != interval\) \{/ { changing_interval = 1 }
	changing_interval && /amiberry_hw_vsync_pacing_set_blocking\(false\);/ {
		cleared_before_attempt = 1
		next
	}
	changing_interval && /if \(interval == ADAPTIVE_SWAP_INTERVAL\)/ {
		if (!cleared_before_attempt)
			exit 1
		adaptive_request = 1
		next
	}
	changing_interval && /else if \(SDL_GL_SetSwapInterval\(interval\)\)/ {
		nonadaptive_success = 1
		next
	}
	nonadaptive_success && /amiberry_hw_vsync_pacing_set_blocking\(interval > 0\);/ {
		positive_success_published = 1
		next
	}
	in_update_vsync && /m_vsync.current_interval = requested_interval;/ {
		exit cleared_before_attempt && adaptive_request && positive_success_published ? 0 : 1
	}
	in_update_vsync && /^}/ { exit 1 }
	END { if (!in_update_vsync) exit 1 }
' "$opengl_renderer_file"; then
	echo "OpenGL pacing capability must be false for failure/nonblocking/adaptive requests and true only after a positive interval succeeds" >&2
	exit 1
fi

if ! awk '
	/int isvsync_chipset\(void\)/ { in_chipset_policy = 1 }
	in_chipset_policy && /#if defined\(AMIBERRY\) && !defined\(LIBRETRO\) && defined\(USE_OPENGL\) && !defined\(USE_VULKAN\)/ {
		in_blocking_gl_policy = 1
		gl_guard_seen = 1
		next
	}
	in_chipset_policy && /if \(kmsdrm_detected && amiberry_hw_vsync_pacing_ok\(\)\)/ {
		if (!in_blocking_gl_policy)
			exit 1
		kmsdrm_match = 1
		next
	}
	kmsdrm_match && /return 1;/ {
		kmsdrm_hardware_pacing = 1
		next
	}
	in_blocking_gl_policy && /#endif/ {
		if (!kmsdrm_hardware_pacing)
			exit 1
		in_blocking_gl_policy = 0
		gl_guard_closed = 1
		next
	}
	in_chipset_policy && /if \(currprefs.gfx_apmode\[0\]\.gfx_vsync <= 0\)/ {
		exit gl_guard_seen && gl_guard_closed ? 0 : 1
	}
	in_chipset_policy && /^}/ { exit 1 }
	END { if (!in_chipset_policy) exit 1 }
' "$drawing_file"; then
	echo "Matched-refresh KMSDRM hardware pacing must be limited to OpenGL/GLES before the user VSync early return" >&2
	exit 1
fi

if ! grep -F -q 'success &= SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);' "$gfx_window_file"; then
	echo "The OpenGL context must continue to request double buffering" >&2
	exit 1
fi

if ! awk '
	/void amiberry_gui_halt\(\)/ { in_gui_halt = 1 }
	in_gui_halt && /gl_renderer->restore_emulation_context\(mon->amiga_window\);/ {
		context_restored = 1
	}
	in_gui_halt && /^}/ { exit context_restored ? 0 : 1 }
	END { if (!in_gui_halt) exit 1 }
' "$source_file"; then
	echo "GUI teardown must continue to restore the emulation context" >&2
	exit 1
fi

if ! awk '
	/void OpenGLRenderer::restore_emulation_context\(SDL_Window\* window\)/ { in_restore = 1 }
	in_restore && /SDL_GL_MakeCurrent\(window, m_gl_context\);/ { context_current = 1 }
	in_restore && /m_vsync.current_interval = INVALID_SWAP_INTERVAL;/ { interval_invalidated = 1 }
	in_restore && /amiberry_hw_vsync_pacing_set_blocking\(false\);/ { blocking_cleared = 1 }
	in_restore && /^}/ { exit context_current && interval_invalidated && blocking_cleared ? 0 : 1 }
	END { if (!in_restore) exit 1 }
' "$opengl_renderer_file"; then
	echo "OpenGL context restoration must make the context current and invalidate both swap interval and blocking capability" >&2
	exit 1
fi

for lifecycle_function in init_context destroy_context; do
	if ! awk -v function_name="$lifecycle_function" '
		$0 ~ "OpenGLRenderer::" function_name "\\(" { in_lifecycle = 1 }
		in_lifecycle && /amiberry_hw_vsync_pacing_set_blocking\(false\);/ { exit 0 }
		in_lifecycle && /^}/ { exit 1 }
		END { if (!in_lifecycle) exit 1 }
	' "$opengl_renderer_file"; then
		echo "OpenGL $lifecycle_function must clear the blocking-presentation capability" >&2
		exit 1
	fi
done

if ! grep -F -q 'Legacy KMSDRM uses software timing with drained presentation' "$display_panel_file" ||
	! grep -F -q 'blocking presentation is used for pacing only when console and emulated refresh match' "$display_panel_file" ||
	! grep -F -q 'VSync controls, refresh switching, and Adaptive/VRR modes are not available' "$display_panel_file"; then
	echo "KMSDRM help must describe legacy software pacing, matched-refresh hardware pacing, and unavailable controls" >&2
	exit 1
fi

if ! awk '
	/void gui_display\(int shortcut\)/ { in_gui_display = 1 }
	in_gui_display && /if \(no_wm_detected && amiga_surface != nullptr\)/ {
		in_shared_window_refresh = 1
		next
	}
	in_shared_window_refresh && /target_graphics_buffer_update\(mon->monitor_id, true\);/ {
		forced_refresh = 1
	}
	in_shared_window_refresh && /^	}/ { exit forced_refresh ? 0 : 1 }
	in_gui_display && /^}/ { exit 1 }
	END { if (!in_gui_display) exit 1 }
' "$gui_lifecycle_file"; then
	echo "Shared-window GUI resume must continue to force a graphics-buffer refresh" >&2
	exit 1
fi
