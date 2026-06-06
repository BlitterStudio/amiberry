package com.blitterstudio.amiberry.ui.screens.settings

import org.junit.Assert.assertEquals
import org.junit.Test

class MemoryAmountDisplayTest {

	@Test
	fun `memory display preserves fractional megabytes`() {
		assertEquals(MemoryAmountDisplay.FractionalMegabytes(tenths = 15), memoryAmountDisplayValue(1536))
		assertEquals(MemoryAmountDisplay.FractionalMegabytes(tenths = 18), memoryAmountDisplayValue(1792))
	}

	@Test
	fun `memory display keeps none kilobyte and whole megabyte values`() {
		assertEquals(MemoryAmountDisplay.None, memoryAmountDisplayValue(0))
		assertEquals(MemoryAmountDisplay.Kilobytes(512), memoryAmountDisplayValue(512))
		assertEquals(MemoryAmountDisplay.WholeMegabytes(1), memoryAmountDisplayValue(1024))
		assertEquals(MemoryAmountDisplay.WholeMegabytes(2), memoryAmountDisplayValue(2048))
	}
}
