package com.blitterstudio.amiberry;

import android.content.Intent;
import android.os.Bundle;
import android.view.WindowManager;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import org.libsdl.app.SDLActivity;

/**
 * Subclass of SDLActivity that reads command-line arguments from the launching Intent.
 * The native UI passes arguments via the "SDL_ARGS" String[] extra, which becomes
 * argv for the C++ main() function.
 *
 * Also forces fullscreen immersive mode so the system bars don't overlap the
 * emulation display.
 *
 * Back-button handling: SDL3's trapping mechanism (SDL_HINT_ANDROID_TRAP_BACK_BUTTON=1)
 * intercepts KEYCODE_BACK in dispatchKeyEvent() and delivers it as SDL_SCANCODE_AC_BACK.
 * The native side maps this to AKS_ENTERGUI, which opens the ImGui GUI — consistent
 * with all other platforms.  No Java-side back handling is needed.
 */
public class AmiberryActivity extends SDLActivity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		enterImmersiveMode();
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus) {
			enterImmersiveMode();
		}
	}

	@Override
	protected String[] getLibraries() {
		return new String[] {
			// SDL3 and SDL3_image are statically linked into libamiberry.so,
			// so only load the final shared library.
			"amiberry"
		};
	}

	@Override
	protected String[] getArguments() {
		Intent intent = getIntent();
		if (intent != null) {
			String[] args = intent.getStringArrayExtra("SDL_ARGS");
			if (args != null) {
				return args;
			}
		}
		// Default: no arguments, opens ImGui GUI
		return new String[0];
	}

	@Override
	protected void onDestroy() {
		final boolean finishing = isFinishing();
		// Note: SDL3's SDLActivity calls System.exit(0) after native
		// main() returns, which kills the process without reaching
		// onDestroy().  Clean-exit markers are therefore written from
		// native code in target_quit() instead.  This block is kept
		// as a best-effort fallback for edge cases where onDestroy
		// does run.
		if (finishing) {
			com.blitterstudio.amiberry.data.EmulatorLauncher.INSTANCE.writeCleanExitMarker(this);
			com.blitterstudio.amiberry.data.EmulatorLauncher.INSTANCE.clearSessionMarker(this);
		}
		super.onDestroy();
		// Amiberry runs in a dedicated :sdl process; terminate it when this
		// activity is finished so the next launch always starts from clean state.
		if (finishing) {
			android.os.Process.killProcess(android.os.Process.myPid());
		}
	}

	private void enterImmersiveMode() {
		WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
		WindowInsetsControllerCompat controller =
			WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
		controller.hide(WindowInsetsCompat.Type.statusBars() | WindowInsetsCompat.Type.navigationBars());
		controller.setSystemBarsBehavior(
			WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}
}
