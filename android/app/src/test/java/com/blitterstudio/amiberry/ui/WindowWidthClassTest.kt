package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class WindowWidthClassTest {

	@Test
	fun `phones keep the single-pane layout upright and sideways`() {
		// Pixel 7: 412 x 915 dp upright, 915 x 412 dp sideways.
		assertEquals(WindowWidthClass.Compact, WindowWidthClass.of(widthDp = 412, heightDp = 915))
		assertEquals(WindowWidthClass.Compact, WindowWidthClass.of(widthDp = 915, heightDp = 412))
	}

	@Test
	fun `an upright tablet is medium width and a landscape tablet or TV is expanded`() {
		// Pixel Tablet: 800 x 1280 dp upright, 1280 x 800 dp in landscape; 1080p TV: 960 x 540 dp.
		assertEquals(WindowWidthClass.Medium, WindowWidthClass.of(widthDp = 800, heightDp = 1280))
		assertEquals(WindowWidthClass.Expanded, WindowWidthClass.of(widthDp = 1280, heightDp = 800))
		assertEquals(WindowWidthClass.Expanded, WindowWidthClass.of(widthDp = 960, heightDp = 540))
	}

	@Test
	fun `the side list shows from medium width and two full panes only when expanded`() {
		assertFalse(WindowWidthClass.Compact.showsSideList)
		assertTrue(WindowWidthClass.Medium.showsSideList)
		assertTrue(WindowWidthClass.Expanded.showsSideList)
		assertFalse(WindowWidthClass.Medium.showsTwoPanes)
		assertTrue(WindowWidthClass.Expanded.showsTwoPanes)
	}

	@Test
	fun `the navigation rail shows in landscape and on windows of medium width`() {
		assertFalse(WindowWidthClass.usesNavigationRail(widthDp = 412, landscape = false))
		assertTrue(WindowWidthClass.usesNavigationRail(widthDp = 915, landscape = true))
		assertTrue(WindowWidthClass.usesNavigationRail(widthDp = 800, landscape = false))
	}
}
