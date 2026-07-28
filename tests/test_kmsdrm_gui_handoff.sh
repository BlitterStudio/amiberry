#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/gui/main_window.cpp"
platform_init_file="src/osdep/amiberry_platform_internal_host.h"
gfx_window_file="src/osdep/gfx_window.cpp"
opengl_renderer_file="src/osdep/opengl_renderer.cpp"
drawing_file="src/drawing.cpp"
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

if grep -E -q 'SDL_(SetHint|SetHintWithPriority)\([^;]*SDL_HINT_VIDEO_DOUBLE_BUFFER' "$platform_init_file"; then
	echo "Amiberry must not force SDL_HINT_VIDEO_DOUBLE_BUFFER" >&2
	exit 1
fi

if ! awk '
	/void OpenGLRenderer::update_vsync\(int monid\)/ { in_update_vsync = 1 }
	in_update_vsync && /if \(kmsdrm_detected\) \{/ { in_kmsdrm_policy = 1 }
	in_kmsdrm_policy && /interval = 1;/ { interval_one = 1 }
	in_kmsdrm_policy && /^	}/ {
		exit interval_one ? 0 : 1
	}
	END { if (!in_update_vsync) exit 1 }
' "$opengl_renderer_file"; then
	echo "KMSDRM must continue to force OpenGL swap interval 1" >&2
	exit 1
fi

if ! awk '
	/static bool amiberry_hw_vsync_pacing_ok\(void\)/ { in_pacing_policy = 1 }
	in_pacing_policy && /if \(kmsdrm_detected\)/ {
		kmsdrm_policy = 1
		next
	}
	kmsdrm_policy && /return false;/ { exit 0 }
	in_pacing_policy && /^}/ { exit 1 }
	END { if (!in_pacing_policy) exit 1 }
' "$drawing_file"; then
	echo "KMSDRM must continue to use software emulation pacing" >&2
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
	in_restore && /^}/ { exit context_current && interval_invalidated ? 0 : 1 }
	END { if (!in_restore) exit 1 }
' "$opengl_renderer_file"; then
	echo "OpenGL context restoration must continue to make the emulation context current and refresh its swap interval" >&2
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
