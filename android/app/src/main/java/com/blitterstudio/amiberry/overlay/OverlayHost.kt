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
		)[OverlayViewModel::class.java].also {
			it.hideView = ::hide
			it.dialogsChanged = ::dialogsChanged
		}
	}
	private var view: ComposeView? = null
	/** The native menu's loop runs (its show/hide commands from android_overlay.cpp). */
	private var menuShowing = false
	/** A native dialog (message box, gui_ask_disk's requester) is open. */
	private var dialogShowing = false
	/** The native on-screen keyboard's view; only as tall as the keyboard, docked at the bottom, so touches above it reach SDL. */
	private var keyboardView: ComposeView? = null
	/** The native on-screen keyboard shows (android_osk_toggle turned it on). */
	private var keyboardShowing = false

	val isShowing: Boolean
		get() = view?.visibility == View.VISIBLE || keyboardView?.visibility == View.VISIBLE

	fun onCreate(savedInstanceState: Bundle?) {
		savedState.performAttach()
		savedState.performRestore(savedInstanceState)
		lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_CREATE)
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

	/** The native loop started: show the menu, at [page] where a media or state hotkey asked for it. */
	fun show(page: Int = 0) {
		val menu = view ?: createView().also { view = it }
		menuShowing = true
		syncVisibility()
		viewModel.shown(page)
		// Out of touch mode, where Compose's clickables cannot take focus:
		// the menu opens with Resume focused, ready for the D-pad or a gamepad.
		menu.requestFocusFromTouch()
	}

	/** The native on-screen keyboard showed (android_osk_toggle): the emulation keeps running under it. */
	fun showKeyboard() {
		val keyboard = keyboardView ?: createKeyboardView().also { keyboardView = it }
		keyboardShowing = true
		if (keyboard.visibility != View.VISIBLE) {
			keyboard.visibility = View.VISIBLE
			keyboard.bringToFront()
		}
		viewModel.keyboardShown()
		// Out of touch mode, as the menu: the keys can take the D-pad focus.
		keyboard.requestFocusFromTouch()
	}

	/** The native side hid the keyboard (Back through SDL, the menu opening, the app backgrounding). */
	fun hideKeyboard() {
		if (!keyboardShowing) return
		keyboardShowing = false
		viewModel.keyboardHidden()
		keyboardView?.visibility = View.GONE
		if (!isShowing) sdlSurface.requestFocus()
	}

	/** The keyboard hid itself (its Hide key, Back, the B button): it released what it held on the way down. */
	private fun keyboardClosedByView() {
		if (!keyboardShowing) return
		keyboardShowing = false
		viewModel.hideKeyboard()
		keyboardView?.visibility = View.GONE
		if (!isShowing) sdlSurface.requestFocus()
	}

	/** The menu closed, or the native loop ended (a reset, quit or state load). */
	fun hide() {
		if (!menuShowing) return
		menuShowing = false
		viewModel.hidden()
		syncVisibility()
		if (!isShowing) sdlSurface.requestFocus()
	}

	/** gui_message's boxes arrived; they show one at a time over whatever runs. */
	fun showMessages() {
		val texts = OverlayNative.pendingMessages().toList()
		if (texts.isEmpty()) return
		val menu = view ?: createView().also { view = it }
		viewModel.messagesQueued(texts)
		dialogShowing = true
		syncVisibility()
		// The box takes the focus (modal over a running emulation, too).
		menu.requestFocusFromTouch()
	}

	/** gui_ask_disk's requester opened ([OverlayNative.dialogRequest]). */
	fun showRecoveryDialog() {
		val request = OverlayNative.dialogRequest()?.let(FloppyRecoveryRequest::parse) ?: return
		val menu = view ?: createView().also { view = it }
		viewModel.recoveryShown(request)
		dialogShowing = true
		syncVisibility()
		menu.requestFocusFromTouch()
	}

	/** The native side stopped waiting for the dialog (the emulation is quitting). */
	fun hideDialog() {
		if (!dialogShowing) return
		dialogShowing = false
		viewModel.recoveryHidden()
		syncVisibility()
		if (!isShowing) sdlSurface.requestFocus()
	}

	/**
	 * A dialog came or went ([OverlayViewModel.dialogsChanged]): the last
	 * one leaving takes the view with it — nothing of the overlay may keep
	 * the emulation's input, or its Back, after its dialogs are gone. The
	 * menu keeps the view up on its own show/hide commands.
	 */
	private fun dialogsChanged(showing: Boolean) {
		if (showing) {
			dialogShowing = true
		} else {
			if (!dialogShowing) return
			dialogShowing = false
		}
		syncVisibility()
		if (!isShowing) sdlSurface.requestFocus()
		else if (keyboardShowing && !menuShowing && !dialogShowing) keyboardView?.requestFocus()
	}

	private fun syncVisibility() {
		val menu = view ?: return
		if (menuShowing || dialogShowing) {
			if (menu.visibility != View.VISIBLE) {
				menu.visibility = View.VISIBLE
				menu.bringToFront()
			}
		} else if (menu.visibility == View.VISIBLE) {
			menu.visibility = View.GONE
		}
	}

	/** Back while an overlay shows: out of the keyboard, or the menu's previous page. */
	fun handleBack(): Boolean {
		if (!isShowing) return false
		if (keyboardShowing && !menuShowing && !dialogShowing) {
			keyboardClosedByView()
			return true
		}
		if (onBackPressedDispatcher.hasEnabledCallbacks()) onBackPressedDispatcher.onBackPressed() else viewModel.back()
		return true
	}

	/**
	 * A key while an overlay shows; true when it was handled here, false
	 * for the view hierarchy (the menu's or the keyboard's) to handle it.
	 */
	fun onKeyEvent(event: KeyEvent): Boolean {
		if (!isShowing) return false
		// The release of a press SDL saw (the Start or F12 that opened the
		// menu) must reach it; SDL ignores releases of keys it has not seen.
		if (event.action == KeyEvent.ACTION_UP) SDLActivity.handleKeyEvent(sdlSurface, event.keyCode, event, null)
		val target = keyTarget() ?: return false
		if (!target.hasFocus()) takeFocus()
		return when (event.keyCode) {
			KeyEvent.KEYCODE_BUTTON_B -> {
				if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) handleBack()
				true
			}
			KeyEvent.KEYCODE_BUTTON_START, KeyEvent.KEYCODE_BUTTON_MODE -> {
				if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) {
					// Back to the game from either: typing is done.
					if (keyboardShowing && !menuShowing) keyboardClosedByView() else viewModel.resume()
				}
				true
			}
			// A gamepad's A selects, as OK does on a remote.
			KeyEvent.KEYCODE_BUTTON_A -> target.dispatchKeyEvent(
				KeyEvent(
					event.downTime, event.eventTime, event.action, KeyEvent.KEYCODE_DPAD_CENTER,
					event.repeatCount, event.metaState, event.deviceId, event.scanCode, event.flags,
					InputDevice.SOURCE_DPAD
				)
			).let { true }
			else -> false
		}
	}

	/** The overlay view the keys belong to: a dialog or the menu over the keyboard. */
	private fun keyTarget(): View? = when {
		menuShowing || dialogShowing -> view
		keyboardShowing -> keyboardView
		else -> null
	}

	private fun takeFocus() {
		keyTarget()?.requestFocus()
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
					// The menu while its loop runs; otherwise only the native
					// dialogs (a message box or the recovery requester over a
					// running emulation) compose.
					if (viewModel.menuUp) OverlayScreen(viewModel) else OverlayDialogs(viewModel)
				}
			}
			layout.addView(this, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
		}
	}

	/** The keyboard's own view, docked at the bottom: what it does not cover keeps SDL's touches. */
	private fun createKeyboardView(): ComposeView {
		// The owners are the window's, as createView sets them for the menu.
		activity.window.decorView.let { root ->
			root.setViewTreeLifecycleOwner(this)
			root.setViewTreeSavedStateRegistryOwner(this)
			root.setViewTreeViewModelStoreOwner(this)
			root.setViewTreeOnBackPressedDispatcherOwner(this)
		}
		val keyboard = ComposeView(activity).apply {
			visibility = View.GONE
			setContent {
				AmiberryTheme {
					OverlayKeyboard(
						model = viewModel.keyboard,
						options = viewModel.keyboardOptions,
						onHide = ::keyboardClosedByView
					)
				}
			}
		}
		layout.addView(
			keyboard,
			android.widget.RelativeLayout.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			).apply { addRule(android.widget.RelativeLayout.ALIGN_PARENT_BOTTOM) }
		)
		return keyboard
	}
}
