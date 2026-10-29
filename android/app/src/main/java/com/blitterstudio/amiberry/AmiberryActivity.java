package com.blitterstudio.amiberry;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import com.blitterstudio.amiberry.overlay.OverlayHost;
import org.libsdl.app.SDLActivity;

/**
 * Subclass of SDLActivity that reads command-line arguments from the launching Intent.
 * The native UI passes arguments via the "SDL_ARGS" String[] extra, which becomes
 * argv for the C++ main() function.
 *
 * Also forces fullscreen immersive mode so the system bars don't overlap the
 * emulation display.
 *
 * Back-button handling: legacy Back and Android 13+ predictive Back both use
 * dispatchBackToSdl().  While the in-game menu shows, Back is the menu's;
 * otherwise it synthesizes KEYCODE_BACK for SDL.  The native side receives
 * SDL_SCANCODE_AC_BACK, closes the on-screen keyboard first, then uses Back
 * as the GUI shortcut, which opens the in-game menu (OverlayHost) over a
 * running emulation.
 */
public class AmiberryActivity extends SDLActivity {
	// Messages from src/osdep/android_overlay.cpp (SDL_SendAndroidMessage).
	private static final int COMMAND_SHOW_OVERLAY = COMMAND_USER + 1;
	private static final int COMMAND_HIDE_OVERLAY = COMMAND_USER + 2;

	private OnBackInvokedCallback backCallback;
	private OverlayHost overlay;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		// Without its libraries SDLActivity shows an error and has no layout.
		if (!mBrokenLibraries && mLayout != null) {
			overlay = new OverlayHost(this, mLayout, mSurface);
			overlay.onCreate(savedInstanceState);
		}
		registerBackCallback();
		enterImmersiveMode();
	}

	@Override
	protected void onStart() {
		super.onStart();
		if (overlay != null) {
			overlay.onStart();
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		if (overlay != null) {
			overlay.onResume();
		}
	}

	@Override
	protected void onPause() {
		if (overlay != null) {
			overlay.onPause();
		}
		super.onPause();
	}

	@Override
	protected void onStop() {
		if (overlay != null) {
			overlay.onStop();
		}
		super.onStop();
	}

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		if (overlay != null) {
			overlay.onSaveInstanceState(outState);
		}
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus) {
			enterImmersiveMode();
		}
		if (overlay != null) {
			overlay.onWindowFocusChanged(hasFocus);
		}
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		if (overlay != null && overlay.isShowing() && overlay.onKeyEvent(event)) {
			return true;
		}
		return super.dispatchKeyEvent(event);
	}

	@Override
	protected boolean onUnhandledMessage(int command, Object param) {
		if (overlay == null) {
			return super.onUnhandledMessage(command, param);
		}
		if (command == COMMAND_SHOW_OVERLAY) {
			overlay.show();
			return true;
		}
		if (command == COMMAND_HIDE_OVERLAY) {
			overlay.hide();
			return true;
		}
		return super.onUnhandledMessage(command, param);
	}

	@Override
	@SuppressWarnings("deprecation")
	@android.annotation.SuppressLint("GestureBackNavigation")
	public void onBackPressed() {
		dispatchBackToSdl();
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
		// Default: open ImGui GUI but still rescan ROMs after Android recreates this activity.
		return new String[] {"--rescan-roms"};
	}

	@Override
	protected void onDestroy() {
		final boolean finishing = isFinishing();
		unregisterBackCallback();
		if (overlay != null) {
			overlay.onDestroy();
		}
		// Note: SDL3's SDLActivity calls System.exit(0) after native
		// main() returns, which kills the process without reaching
		// onDestroy().  Clean-exit markers are therefore written from
		// native code in target_quit() instead.  This block is kept
		// as a best-effort fallback for edge cases where onDestroy
		// does run.
		if (finishing) {
			com.blitterstudio.amiberry.data.EmulatorLauncher.INSTANCE.writeCleanExitMarker(this);
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

	private void registerBackCallback() {
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU || backCallback != null) {
			return;
		}

		backCallback = this::dispatchBackToSdl;
		getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
			OnBackInvokedDispatcher.PRIORITY_DEFAULT,
			backCallback);
	}

	private void unregisterBackCallback() {
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU || backCallback == null) {
			return;
		}

		getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(backCallback);
		backCallback = null;
	}

	private void dispatchBackToSdl() {
		// The in-game menu shows over a paused emulation: Back is its own.
		if (overlay != null && overlay.handleBack()) {
			return;
		}
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_BACK);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_BACK);
	}
}
