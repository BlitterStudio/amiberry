package com.blitterstudio.amiberry.overlay

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class OverlayStatusTest {

	private fun fields(cdActivity: String, cdPath: String, vararg drives: Pair<Boolean, String>): Array<String> =
		arrayOf(cdActivity, cdPath) +
			drives.flatMap { (disabled, path) -> listOf(if (disabled) "1" else "0", path) }

	@Test
	fun `the snapshot parses into drives in order`() {
		val status = OverlayStatus.parse(
			fields("-1", "", false to "/f/a.adf", false to "", true to "", true to "")
		)!!
		assertEquals(listOf(0, 1), status.enabledDrives.map { it.drive })
		assertEquals("/f/a.adf", status.drives[0].path)
		assertFalse(status.hasCd)
	}

	@Test
	fun `a CD drive shows when the machine has one or a CD is inserted`() {
		val drives = arrayOf(false to "", true to "", true to "", true to "")
		assertTrue(OverlayStatus.parse(fields("0", "", *drives))!!.hasCd)
		assertTrue(OverlayStatus.parse(fields("-1", "/cd/x.iso", *drives))!!.hasCd)
	}

	@Test
	fun `a short or missing snapshot is rejected`() {
		assertNull(OverlayStatus.parse(arrayOf("-1", "")))
		assertNull(OverlayStatus.parse(null))
	}
}
