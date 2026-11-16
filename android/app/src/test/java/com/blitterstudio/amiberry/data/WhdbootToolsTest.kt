package com.blitterstudio.amiberry.data

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class WhdbootToolsTest {
	@Test
	fun `status parser treats a short native response as missing`() {
		val status = parseWhdbootStatus(arrayOf("1", "0"))

		assertTrue(status.whdloadInstalled)
		assertFalse(status.jstInstalled)
		assertFalse(status.rtbFilesInstalled)
		assertFalse(status.librariesInstalled)
		assertFalse(status.downloadsAvailable)
	}

	@Test
	fun `selected JST must be installed before a game launches`() {
		val status = parseWhdbootStatus(arrayOf("1", "0", "1", "1", "1")).copy(useJst = true)

		assertFalse(status.readyForSelectedTool)
	}
}
