package com.blitterstudio.amiberry.ui.screens.settings

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class InputTabOptionsTest {

	@Test
	fun `both joystick ports offer none`() {
		assertTrue(inputDeviceOptionIds(hasTouchScreen = true, portIndex = 0).contains("none"))
		assertTrue(inputDeviceOptionIds(hasTouchScreen = true, portIndex = 1).contains("none"))
	}

	@Test
	fun `port zero never offers on-screen joystick`() {
		val options = inputDeviceOptionIds(hasTouchScreen = true, portIndex = 0)

		assertFalse(options.contains("onscreen_joy"))
	}

	@Test
	fun `port one offers on-screen joystick on touch devices`() {
		val options = inputDeviceOptionIds(hasTouchScreen = true, portIndex = 1)

		assertTrue(options.contains("onscreen_joy"))
	}

	@Test
	fun `port one hides on-screen joystick without touchscreen`() {
		val options = inputDeviceOptionIds(hasTouchScreen = false, portIndex = 1)

		assertFalse(options.contains("onscreen_joy"))
	}
}
