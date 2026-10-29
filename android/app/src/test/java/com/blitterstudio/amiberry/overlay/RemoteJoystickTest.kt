package com.blitterstudio.amiberry.overlay

import android.view.InputDevice
import android.view.KeyEvent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class RemoteJoystickTest {

	@Test
	fun `a remote is no gamepad, a gamepad or joystick is`() {
		assertFalse(RemoteJoystick.isGamepad(InputDevice.SOURCE_KEYBOARD or InputDevice.SOURCE_DPAD))
		assertTrue(RemoteJoystick.isGamepad(InputDevice.SOURCE_GAMEPAD or InputDevice.SOURCE_DPAD))
		assertTrue(RemoteJoystick.isGamepad(InputDevice.SOURCE_JOYSTICK))
	}

	@Test
	fun `the D-pad moves the joystick and OK fires, other keys stay keys`() {
		assertEquals(OverlayNative.REMOTE_UP, RemoteJoystick.button(KeyEvent.KEYCODE_DPAD_UP))
		assertEquals(OverlayNative.REMOTE_DOWN, RemoteJoystick.button(KeyEvent.KEYCODE_DPAD_DOWN))
		assertEquals(OverlayNative.REMOTE_LEFT, RemoteJoystick.button(KeyEvent.KEYCODE_DPAD_LEFT))
		assertEquals(OverlayNative.REMOTE_RIGHT, RemoteJoystick.button(KeyEvent.KEYCODE_DPAD_RIGHT))
		assertEquals(OverlayNative.REMOTE_FIRE, RemoteJoystick.button(KeyEvent.KEYCODE_DPAD_CENTER))
		assertEquals(0, RemoteJoystick.button(KeyEvent.KEYCODE_BACK))
		assertEquals(0, RemoteJoystick.button(KeyEvent.KEYCODE_ENTER))
	}

	@Test
	fun `the remote hint shows once and stands for the menu hint`() {
		assertEquals(EmulationHints.REMOTE, EmulationHints.next(remoteActive = true, seen = emptySet()))
		assertNull(EmulationHints.next(remoteActive = true, seen = setOf(EmulationHints.REMOTE)))
		assertNull(EmulationHints.next(remoteActive = false, seen = setOf(EmulationHints.REMOTE)))
		assertEquals(EmulationHints.MENU, EmulationHints.next(remoteActive = false, seen = emptySet()))
		assertNull(EmulationHints.next(remoteActive = false, seen = setOf(EmulationHints.MENU)))
		// A gamepad unplugged later on a TV still explains the remote once.
		assertEquals(EmulationHints.REMOTE, EmulationHints.next(remoteActive = true, seen = setOf(EmulationHints.MENU)))
	}

	@Test
	fun `swapping ports exchanges both devices, or nothing while one is unknown`() {
		val settings = mapOf(
			OverlaySettings.PORT_1 to SettingState("mouse", true),
			OverlaySettings.PORT_2 to SettingState("joy0", true)
		)
		assertEquals(
			mapOf(OverlaySettings.PORT_1 to "joy0", OverlaySettings.PORT_2 to "mouse"),
			OverlaySettings.swappedPorts(settings)
		)
		assertNull(OverlaySettings.swappedPorts(settings - OverlaySettings.PORT_2))
	}
}
