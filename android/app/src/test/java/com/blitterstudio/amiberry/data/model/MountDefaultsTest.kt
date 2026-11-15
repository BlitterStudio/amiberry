package com.blitterstudio.amiberry.data.model

import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class MountDefaultsTest {
	@get:Rule val folder = TemporaryFolder()

	private fun image(name: String, block: Int, header: ByteArray): File =
		File(folder.root, name).also { file ->
			val bytes = ByteArray(32 * 512)
			header.copyInto(bytes, block * 512)
			file.writeBytes(bytes)
		}

	private fun rdsk(tag: String, blockSize: Int): ByteArray = ByteArray(512).also { b ->
		tag.encodeToByteArray().copyInto(b)
		b[16] = (blockSize ushr 24).toByte(); b[17] = (blockSize ushr 16).toByte()
		b[18] = (blockSize ushr 8).toByte(); b[19] = blockSize.toByte()
	}

	@Test
	fun `an RDB in the first 16 blocks selects RDB mode with its block size`() {
		val geometry = MountDefaults.hdfGeometry(image("rdb.hdf", 1, rdsk("RDSK", 1024)).path)
		assertEquals(MountDefaults.Geometry(0, 0, 0, 1024), geometry)
	}

	@Test
	fun `an image without an RDB gets the plain partition geometry`() {
		assertEquals(MountDefaults.Geometry(32, 1, 2, 512), MountDefaults.hdfGeometry(image("plain.hdf", 0, ByteArray(0)).path))
	}

	@Test
	fun `an RDB past block 15 is not looked for`() {
		assertEquals(MountDefaults.Geometry(32, 1, 2, 512), MountDefaults.hdfGeometry(image("late.hdf", 16, rdsk("RDSK", 512)).path))
	}

	@Test
	fun `RDSK must be followed by a zero size high part`() {
		val header = rdsk("RDSK", 512).also { it[4] = 1 }
		assertEquals(MountDefaults.Geometry(32, 1, 2, 512), MountDefaults.hdfGeometry(image("text.hdf", 0, header).path))
	}

	@Test
	fun `an A2090 BABE block is RDB mode with 512 byte blocks`() {
		val header = ByteArray(512).also { it[0] = 0xba.toByte(); it[1] = 0xbe.toByte(); it[18] = 8 }
		assertEquals(MountDefaults.Geometry(0, 0, 0, 512), MountDefaults.hdfGeometry(image("babe.hdf", 2, header).path))
	}

	@Test
	fun `volume names keep a folder's dots and drop an archive's extension`() {
		val dir = folder.newFolder("My.Games")
		val archive = folder.newFile("Work,Disk.lha")
		assertEquals("My.Games", MountDefaults.volumeName(dir.path))
		assertEquals("Work_Disk", MountDefaults.volumeName(archive.path))
	}
}
