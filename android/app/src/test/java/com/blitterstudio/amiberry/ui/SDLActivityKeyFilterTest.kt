package com.blitterstudio.amiberry.ui

import android.view.InputDevice
import android.view.KeyEvent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.libsdl.app.SDLActivity

/**
 * JVM tests for the Amiberry-local ChromeOS key-event filters in the SDL Java shim.
 *
 * The filters are static pure functions of primitive ints because android.jar test stubs
 * (returnDefaultValues = true) cannot behaviorally simulate KeyEvent objects. Android
 * source/keycode constants are compile-time constants, so they are safe to reference here.
 */
class SDLActivityKeyFilterTest {
	private val evdevKeyYPosition = 21
	private val evdevKeyZPosition = 44
	private val evdevEnterPosition = 28

	private val keyboardSources = InputDevice.SOURCE_KEYBOARD
	private val touchpadSources = InputDevice.SOURCE_TOUCHPAD
	private val mouseRelativeSources = InputDevice.SOURCE_MOUSE_RELATIVE
	private val plainMouseSources = InputDevice.SOURCE_MOUSE
	private val comboKeyboardTouchpadSources = InputDevice.SOURCE_KEYBOARD or InputDevice.SOURCE_MOUSE

	@Test
	fun syntheticTouchpadTapConfirmIsSuppressed() {
		// ChromeOS synthesizes an ENTER-like tap confirm with no physical scan code.
		assertTrue(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_ENTER, 0, touchpadSources, false
			)
		)
		assertTrue(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_NUMPAD_ENTER, 0, touchpadSources, false
			)
		)
		// A touchpad or mouse-relative source is a tap confirm even if a scan code leaks through.
		assertTrue(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_ENTER, evdevEnterPosition, touchpadSources, false
			)
		)
		assertTrue(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_ENTER, 0, mouseRelativeSources, false
			)
		)
	}

	@Test
	fun realEnterFromKeyboardPrimaryDevicePassesThrough() {
		assertFalse(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_ENTER, evdevEnterPosition, keyboardSources, true
			)
		)
	}

	@Test
	fun realEnterFromComboKeyboardTouchpadDevicePassesThrough() {
		// Regression guard for the fork's source-mask predicate, which swallowed real Enter
		// keys from keyboard+touchpad combo accessories (Logitech K400 class: their touchpad
		// half reports plain SOURCE_MOUSE, not SOURCE_TOUCHPAD/SOURCE_MOUSE_RELATIVE).
		assertFalse(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_ENTER, evdevEnterPosition, comboKeyboardTouchpadSources, true
			)
		)
		// Even a scan-code-less Enter from a keyboard-primary device is left alone.
		assertFalse(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_ENTER, 0, comboKeyboardTouchpadSources, true
			)
		)
	}

	@Test
	fun scanCodeZeroEnterFromNonKeyboardDeviceIsSuppressed() {
		// Synthetic confirm that lost its touchpad source: no scan code, no keyboard either.
		assertTrue(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_ENTER, 0, plainMouseSources, false
			)
		)
	}

	@Test
	fun dpadCenterIsNeverSuppressed() {
		// TV remotes send DPAD_CENTER as real navigation; it is excluded from the confirm set.
		assertFalse(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_DPAD_CENTER, 0, touchpadSources, false
			)
		)
		assertFalse(
			SDLActivity.isSyntheticTouchpadConfirm(
				KeyEvent.KEYCODE_DPAD_CENTER, 0, InputDevice.SOURCE_DPAD, false
			)
		)
	}

	@Test
	fun qwertzPhysicalPositionsNormalizeToQwertyKeycodes() {
		// On a QWERTZ host the layout-shifted keyCode is fed for these two slots; the filters
		// pin them to the physical QWERTY position so the AmigaOS keymap can do the layout.
		assertEquals(
			KeyEvent.KEYCODE_Y,
			SDLActivity.normalizePhysicalKeyboardKeyCode(
				KeyEvent.KEYCODE_Z, evdevKeyYPosition, keyboardSources, false
			)
		)
		assertEquals(
			KeyEvent.KEYCODE_Z,
			SDLActivity.normalizePhysicalKeyboardKeyCode(
				KeyEvent.KEYCODE_Y, evdevKeyZPosition, keyboardSources, false
			)
		)
	}

	@Test
	fun otherPhysicalPositionsPassThroughUnchanged() {
		assertEquals(
			KeyEvent.KEYCODE_A,
			SDLActivity.normalizePhysicalKeyboardKeyCode(
				KeyEvent.KEYCODE_A, 30, keyboardSources, false
			)
		)
		assertEquals(
			KeyEvent.KEYCODE_ENTER,
			SDLActivity.normalizePhysicalKeyboardKeyCode(
				KeyEvent.KEYCODE_ENTER, evdevEnterPosition, keyboardSources, false
			)
		)
	}

	@Test
	fun joystickAndNonKeyboardSourcesBypassNormalization() {
		// Controller buttons keep their controller meaning.
		assertEquals(
			KeyEvent.KEYCODE_BUTTON_A,
			SDLActivity.normalizePhysicalKeyboardKeyCode(
				KeyEvent.KEYCODE_BUTTON_A, evdevKeyYPosition, InputDevice.SOURCE_JOYSTICK, true
			)
		)
		// Touchpad/mouse-source events are not layout-normalized.
		assertEquals(
			KeyEvent.KEYCODE_ENTER,
			SDLActivity.normalizePhysicalKeyboardKeyCode(
				KeyEvent.KEYCODE_ENTER, evdevKeyZPosition, touchpadSources, false
			)
		)
		assertEquals(
			KeyEvent.KEYCODE_ENTER,
			SDLActivity.normalizePhysicalKeyboardKeyCode(
				KeyEvent.KEYCODE_ENTER, evdevKeyZPosition, plainMouseSources, false
			)
		)
	}
}
