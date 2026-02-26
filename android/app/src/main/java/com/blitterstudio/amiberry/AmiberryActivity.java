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
import org.libsdl.app.SDLActivity;

/**
 * Subclass of SDLActivity that reads command-line arguments from the launching Intent.
 * The native UI passes arguments via the "SDL_ARGS" String[] extra, which becomes
 * argv for the C++ main() function.
 *
 * Also forces fullscreen immersive mode so the system bars don't overlap the
 * emulation display.
 */
public class AmiberryActivity extends SDLActivity {

	private OnBackInvokedCallback backCallback;

	// ── SDL-version-dependent constants ──────────────────────────────────
	// These are the touch-points that may need updating when migrating
	// from SDL2 to SDL3.  As of SDL 3.5.0 the JNI signatures and hint
	// name are identical to SDL2, so no changes are needed yet.
	//
	// SDL2 → SDL3 checklist:
	//   • HINT_TRAP_BACK  – same name in SDL3 ("SDL_ANDROID_TRAP_BACK_BUTTON")
	//   • sendBackToSDL() – uses onNativeKeyDown/Up, same JNI sig in SDL3
	//   • isBackTrapped() – uses nativeGetHintBoolean, same JNI sig in SDL3
	//   • If SDL3 adds its own OnBackInvokedCallback, remove registerBackHandler()
	//     and the manifest attribute android:enableOnBackInvokedCallback.
	// ─────────────────────────────────────────────────────────────────────
	private static final String HINT_TRAP_BACK = "SDL_ANDROID_TRAP_BACK_BUTTON";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		enterImmersiveMode();
		registerBackHandler();
	}

	@Override
	protected void onNewIntent(Intent intent) {
		super.onNewIntent(intent);
		setIntent(intent);
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus) {
			enterImmersiveMode();
		}
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

	// ── Back-button handling ─────────────────────────────────────────────
	// SDL2/SDL3's SDLActivity.onBackPressed() is deprecated since API 33.
	// On API 34+ with gesture navigation the system may never call it,
	// causing the activity to finish instead of opening the ImGui GUI.
	//
	// We register an OnBackInvokedCallback that replicates the trap logic
	// and injects the key event into SDL ourselves.  The manifest also
	// sets android:enableOnBackInvokedCallback="true" on this activity so
	// the legacy onBackPressed() path is fully bypassed on API 33+.
	//
	// On API ≤ 32 the attribute is ignored and SDLActivity.onBackPressed()
	// handles back as before.
	// ─────────────────────────────────────────────────────────────────────

	/**
	 * Query SDL's hint to check whether the native side wants the back
	 * button trapped (i.e. delivered as a key event rather than finishing).
	 *
	 * SDL2/SDL3: same JNI method, same hint name.
	 */
	private boolean isBackTrapped() {
		return SDLActivity.nativeGetHintBoolean(HINT_TRAP_BACK, false);
	}

	/**
	 * Inject a KEYCODE_BACK down+up pair into SDL's native event queue.
	 * The native side maps this to SDL_SCANCODE_AC_BACK → AKS_ENTERGUI,
	 * which opens or closes the ImGui settings panel.
	 *
	 * SDL2/SDL3: same JNI method signatures.
	 */
	private void sendBackToSDL() {
		SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_BACK);
		SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_BACK);
	}

	private void registerBackHandler() {
		if (Build.VERSION.SDK_INT >= 33) {
			backCallback = () -> {
				if (isBackTrapped()) {
					sendBackToSDL();
				} else {
					finish();
				}
			};
			getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
				OnBackInvokedDispatcher.PRIORITY_DEFAULT,
				backCallback
			);
		}
	}

	@Override
	protected void onDestroy() {
		if (Build.VERSION.SDK_INT >= 33 && backCallback != null) {
			getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(backCallback);
			backCallback = null;
		}
		super.onDestroy();
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
