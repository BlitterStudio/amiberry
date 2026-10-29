package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.gestures.BringIntoViewSpec
import org.junit.Assert.assertEquals
import org.junit.Test

@OptIn(ExperimentalFoundationApi::class)
class ClearOfBottomBringIntoViewSpecTest {

	/** Stands in for the platform rule: scroll only what is outside the container. */
	private val platform = object : BringIntoViewSpec {
		override fun calculateScrollDistance(offset: Float, size: Float, containerSize: Float): Float = when {
			offset >= 0f && offset + size <= containerSize -> 0f
			offset < 0f -> offset
			else -> offset + size - containerSize
		}
	}
	private val spec = ClearOfBottomBringIntoViewSpec(bottomPx = 230f, platform = platform)

	@Test
	fun `a control above the Start button stays put`() {
		assertEquals(0f, spec.calculateScrollDistance(offset = 100f, size = 150f, containerSize = 2000f))
	}

	@Test
	fun `a control behind the Start button comes up clear of it`() {
		assertEquals(80f, spec.calculateScrollDistance(offset = 1700f, size = 150f, containerSize = 2000f))
	}

	@Test
	fun `a control above the page comes down to the top`() {
		assertEquals(-40f, spec.calculateScrollDistance(offset = -40f, size = 150f, containerSize = 2000f))
	}

	@Test
	fun `a scroller barely taller than the Start button keeps the platform rule`() {
		// A dropdown field's own text scroller: its cursor already shows, so nothing scrolls.
		assertEquals(0f, spec.calculateScrollDistance(offset = 42f, size = 63f, containerSize = 242f))
	}
}
