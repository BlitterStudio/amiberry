package com.blitterstudio.amiberry.ui.components.settings

import org.junit.Assert.assertEquals
import org.junit.Test

class LevelStopTest {

	private val percent = 0f..100f

	@Test
	fun `a press moves one step from a stop`() {
		assertEquals(45f, nextLevelStop(40f, 5f, up = true, range = percent), 0f)
		assertEquals(35f, nextLevelStop(40f, 5f, up = false, range = percent), 0f)
	}

	@Test
	fun `a press from an in-between value lands on the next stop that way`() {
		assertEquals(40f, nextLevelStop(37f, 5f, up = true, range = percent), 0f)
		assertEquals(35f, nextLevelStop(37f, 5f, up = false, range = percent), 0f)
	}

	@Test
	fun `a value a rounding error off a stop still moves a whole step`() {
		assertEquals(45f, nextLevelStop(39.99999f, 5f, up = true, range = percent), 0f)
		assertEquals(35f, nextLevelStop(40.00001f, 5f, up = false, range = percent), 0f)
	}

	@Test
	fun `the ends hold`() {
		assertEquals(100f, nextLevelStop(100f, 5f, up = true, range = percent), 0f)
		assertEquals(0f, nextLevelStop(0f, 5f, up = false, range = percent), 0f)
		assertEquals(100f, nextLevelStop(98f, 5f, up = true, range = percent), 0f)
	}
}
