package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class HardfileCreatorTest {
	@get:Rule val folder = TemporaryFolder()

	@Test
	fun `raw hardfile is sparse requested size`() {
		val file = HardfileCreator.create(File(folder.root, "disk.hdf"), 8L * 1024 * 1024, dynamicVhd = false)
		assertEquals(8L * 1024 * 1024, file.length())
	}

	@Test
	fun `dynamic VHD has native footer header and unallocated BAT`() {
		val size = 8L * 1024 * 1024
		val file = HardfileCreator.create(File(folder.root, "disk.vhd"), size, dynamicVhd = true)
		val bytes = file.readBytes()
		val batSize = 512
		assertEquals(512 + 1024 + batSize + 512, bytes.size)
		assertEquals("conectix", bytes.decodeToString(0, 8))
		assertEquals("cxsparse", bytes.decodeToString(512, 520))
		assertEquals("conectix", bytes.decodeToString(bytes.size - 512, bytes.size - 504))
		assertEquals(16, unsigned(bytes, 512 + 28))
		assertTrue(bytes.sliceArray(1536 until 1536 + 64).all { it == 0xff.toByte() })
		assertFalse(File(folder.root, "disk.vhd").let { runCatching { HardfileCreator.create(it, size, true) }.isSuccess })
	}

	private fun unsigned(bytes: ByteArray, offset: Int): Int =
		((bytes[offset].toInt() and 0xff) shl 24) or ((bytes[offset + 1].toInt() and 0xff) shl 16) or
			((bytes[offset + 2].toInt() and 0xff) shl 8) or (bytes[offset + 3].toInt() and 0xff)
}
