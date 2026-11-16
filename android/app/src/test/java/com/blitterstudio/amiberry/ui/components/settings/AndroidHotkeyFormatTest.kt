package com.blitterstudio.amiberry.ui.components.settings

import android.view.KeyEvent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class AndroidHotkeyFormatTest {
	@Test
	fun `formats adb and physical keyboard keys in native hotkey spelling`() {
		assertEquals("F12", AndroidHotkeyFormat.format(KeyEvent.KEYCODE_F12, 0, allowModifiers = true))
		assertEquals(
			"LCtrl+LShift+A",
			AndroidHotkeyFormat.format(
				KeyEvent.KEYCODE_A,
				KeyEvent.META_CTRL_LEFT_ON or KeyEvent.META_SHIFT_LEFT_ON,
				allowModifiers = true
			)
		)
		assertEquals("LeftArrow", AndroidHotkeyFormat.format(KeyEvent.KEYCODE_DPAD_LEFT, 0, allowModifiers = true))
	}

	@Test
	fun `amiga key bindings ignore held modifiers and unsupported keys`() {
		assertEquals(
			"A",
			AndroidHotkeyFormat.format(KeyEvent.KEYCODE_A, KeyEvent.META_CTRL_LEFT_ON, allowModifiers = false)
		)
		assertNull(AndroidHotkeyFormat.format(KeyEvent.KEYCODE_VOLUME_UP, 0, allowModifiers = true))
	}

	@Test
	fun `unsupported release cannot complete an empty modifier recording`() {
		assertNull(AndroidHotkeyFormat.releasedModifier(KeyEvent.KEYCODE_VOLUME_UP, null))
		assertEquals("LCtrl", AndroidHotkeyFormat.releasedModifier(KeyEvent.KEYCODE_CTRL_LEFT, "LCtrl"))
	}
}
