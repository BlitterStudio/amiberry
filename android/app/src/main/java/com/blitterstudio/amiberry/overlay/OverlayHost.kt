package com.blitterstudio.amiberry.overlay

import android.app.Activity
import android.os.Bundle
import android.view.InputDevice
import android.view.KeyEvent
import android.view.View
import android.view.ViewGroup
import androidx.activity.OnBackPressedDispatcher
import androidx.activity.OnBackPressedDispatcherOwner
import androidx.activity.setViewTreeOnBackPressedDispatcherOwner
import androidx.compose.ui.platform.ComposeView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.LifecycleRegistry
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.ViewModelStore
import androidx.lifecycle.ViewModelStoreOwner
import androidx.lifecycle.setViewTreeLifecycleOwner
import androidx.lifecycle.setViewTreeViewModelStoreOwner
import androidx.savedstate.SavedStateRegistry
import androidx.savedstate.SavedStateRegistryController
import androidx.savedstate.SavedStateRegistryOwner
import androidx.savedstate.setViewTreeSavedStateRegistryOwner
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.ui.theme.AmiberryTheme
import org.libsdl.app.SDLActivity

/**
 * Hosts the in-emulation menu in AmiberryActivity, over SDL's surface in
 * the same window, so showing it neither pauses nor blocks SDL (the native
 * loop pauses the emulation). SDLActivity is a plain Activity, so this
 * provides the lifecycle, saved state, view models and Back dispatcher the
 * ComposeView needs, following the activity's lifecycle.
 *
 * Input: while the menu shows, keys go to it (it holds the Android focus);
 * key releases also go to SDL, so a button pressed to open the menu is not
 * left held there. The gamepad's B goes back, Start and Guide resume.
 */
class OverlayHost(
	private val activity: Activity,
	private val layout: ViewGroup,
	/** SDL's surface, which gets the focus back when the menu closes. */
	private val sdlSurface: View
) : LifecycleOwner, SavedStateRegistryOwner, ViewModelStoreOwner, OnBackPressedDispatcherOwner {
	private val lifecycleRegistry = LifecycleRegistry(this)
	private val savedState = SavedStateRegistryController.create(this)
	override val lifecycle: Lifecycle get() = lifecycleRegistry
	override val savedStateRegistry: SavedStateRegistry get() = savedState.savedStateRegistry
	override val viewModelStore = ViewModelStore()
	/** Back for Compose's own handlers (an open dropdown); the menu's pages come after. */
	override val onBackPressedDispatcher = OnBackPressedDispatcher()

	private val viewModel: OverlayViewModel by lazy {
		ViewModelProvider(
			this,
			ViewModelProvider.AndroidViewModelFactory.getInstance(activity.application)
		)[OverlayViewModel::class.java].also { it.hideView = ::hide }
	}
	private var view: ComposeView? = null

	val isShowing: Boolean
		get() = view?.visibility == View.VISIBLE

	fun onCreate(savedInstanceState: Bundle?) {
		savedState.performAttach()
		savedState.performRestore(savedInstanceState)
		lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_CREATE)
		// The launcher's choice: the menu, or the built-in GUI as before.
		OverlayNative.setEnabled(!AppPreferences.getInstance(activity).useClassicInGameMenu.value)
		// The menu leaves touch mode for its D-pad focus; SDL's surface gets the
		// focus back afterwards and must not draw the system's focus frame
		// around the emulation.
		sdlSurface.defaultFocusHighlightEnabled = false
	}

	fun onStart() = lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START)

	fun onResume() {
		lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_RESUME)
		// SDL's surface takes the focus as the activity resumes.
		if (isShowing) view?.post { takeFocus() }
	}

	fun onPause() = lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_PAUSE)

	fun onStop() = lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_STOP)

	fun onSaveInstanceState(outState: Bundle) = savedState.performSave(outState)

	fun onDestroy() {
		lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_DESTROY)
		viewModelStore.clear()
	}

	fun onWindowFocusChanged(hasFocus: Boolean) {
		if (hasFocus && isShowing) takeFocus()
	}

	/** The native loop started: show the menu. */
	fun show() {
		val menu = view ?: createView().also { view = it }
		if (menu.visibility == View.VISIBLE) return
		menu.visibility = View.VISIBLE
		menu.bringToFront()
		viewModel.shown()
		// Out of touch mode, where Compose's clickables cannot take focus:
		// the menu opens with Resume focused, ready for the D-pad or a gamepad.
		menu.requestFocusFromTouch()
	}

	/** The menu closed, or the native loop ended (a reset, quit or state load). */
	fun hide() {
		val menu = view ?: return
		if (menu.visibility != View.VISIBLE) return
		menu.visibility = View.GONE
		viewModel.hidden()
		sdlSurface.requestFocus()
	}

	/** Back while the menu shows: its previous page, or out of it. */
	fun handleBack(): Boolean {
		if (!isShowing) return false
		if (onBackPressedDispatcher.hasEnabledCallbacks()) onBackPressedDispatcher.onBackPressed() else viewModel.back()
		return true
	}

	/**
	 * A key while the menu shows; true when it was handled here, false for
	 * the view hierarchy (the menu) to handle it.
	 */
	fun onKeyEvent(event: KeyEvent): Boolean {
		val menu = view ?: return false
		if (!isShowing) return false
		// The release of a press SDL saw (the Start or F12 that opened the
		// menu) must reach it; SDL ignores releases of keys it has not seen.
		if (event.action == KeyEvent.ACTION_UP) SDLActivity.handleKeyEvent(sdlSurface, event.keyCode, event, null)
		if (!menu.hasFocus()) takeFocus()
		return when (event.keyCode) {
			KeyEvent.KEYCODE_BUTTON_B -> {
				if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) handleBack()
				true
			}
			KeyEvent.KEYCODE_BUTTON_START, KeyEvent.KEYCODE_BUTTON_MODE -> {
				if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) viewModel.resume()
				true
			}
			// A gamepad's A selects, as OK does on a remote.
			KeyEvent.KEYCODE_BUTTON_A -> menu.dispatchKeyEvent(
				KeyEvent(
					event.downTime, event.eventTime, event.action, KeyEvent.KEYCODE_DPAD_CENTER,
					event.repeatCount, event.metaState, event.deviceId, event.scanCode, event.flags,
					InputDevice.SOURCE_DPAD
				)
			).let { true }
			else -> false
		}
	}

	private fun takeFocus() {
		view?.requestFocus()
	}

	private fun createView(): ComposeView {
		// Compose looks the owners up from the window's root, as a
		// ComponentActivity sets them on its decor view.
		activity.window.decorView.let { root ->
			root.setViewTreeLifecycleOwner(this)
			root.setViewTreeSavedStateRegistryOwner(this)
			root.setViewTreeViewModelStoreOwner(this)
			root.setViewTreeOnBackPressedDispatcherOwner(this)
		}
		return ComposeView(activity).apply {
			visibility = View.GONE
			setContent {
				AmiberryTheme {
					OverlayScreen(viewModel)
				}
			}
			layout.addView(this, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
		}
	}
}
