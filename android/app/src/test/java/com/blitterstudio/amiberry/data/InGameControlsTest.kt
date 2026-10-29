package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class InGameControlsTest {

	private val touch = EmulatorSettings(joyport1 = "onscreen_joy", onScreenJoystick = true, onScreenKeyboardButton = true)

	@Test
	fun `turning the joystick off in game frees port 2 as the launcher's switch does`() {
		val adopted = InGameControls.applyTo(touch, mapOf(InGameControls.ON_SCREEN_JOYSTICK to false), touchControls = true)
		assertFalse(adopted.onScreenJoystick)
		assertEquals("joy1", adopted.joyport1)
	}

	@Test
	fun `turning the joystick on in game puts it in port 2`() {
		val tv = EmulatorSettings(joyport1 = "joy1", onScreenJoystick = false)
		val adopted = InGameControls.applyTo(tv, mapOf(InGameControls.ON_SCREEN_JOYSTICK to true), touchControls = false)
		assertTrue(adopted.onScreenJoystick)
		assertEquals("onscreen_joy", adopted.joyport1)
	}

	@Test
	fun `the keyboard off in game is off for touch and gamepad alike`() {
		val adopted = InGameControls.applyTo(touch, mapOf(InGameControls.ON_SCREEN_KEYBOARD to false), touchControls = true)
		assertFalse(adopted.onScreenKeyboardButton)
		assertEquals("", adopted.onScreenKeyboardToggle)
		assertFalse(adopted.onScreenKeyboardAvailable)
	}

	@Test
	fun `the keyboard on in game brings back its gamepad toggle, and its touch button only on touch devices`() {
		val off = EmulatorSettings(onScreenKeyboardButton = false, onScreenKeyboardToggle = "")
		val phone = InGameControls.applyTo(off, mapOf(InGameControls.ON_SCREEN_KEYBOARD to true), touchControls = true)
		assertTrue(phone.onScreenKeyboardButton)
		assertNull(phone.onScreenKeyboardToggle)
		val tv = InGameControls.applyTo(off, mapOf(InGameControls.ON_SCREEN_KEYBOARD to true), touchControls = false)
		assertFalse(tv.onScreenKeyboardButton)
		assertTrue(tv.onScreenKeyboardAvailable)
	}

	@Test
	fun `nothing recorded changes nothing`() {
		assertEquals(touch, InGameControls.applyTo(touch, emptyMap(), touchControls = true))
	}

	@Test
	fun `a TV starts without touch overlays, a touch device with them`() {
		val tv = AndroidControlSettings.defaults(isTv = true)
		assertFalse(tv.onScreenJoystick)
		assertFalse(tv.onScreenKeyboardButton)
		assertEquals("joy1", tv.joyport1)
		val phone = AndroidControlSettings.defaults(isTv = false)
		assertTrue(phone.onScreenJoystick)
		assertTrue(phone.onScreenKeyboardButton)
		assertEquals("onscreen_joy", phone.joyport1)
	}
}
