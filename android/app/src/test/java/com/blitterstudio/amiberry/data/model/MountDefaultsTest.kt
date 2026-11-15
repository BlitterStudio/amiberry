package com.blitterstudio.amiberry.data.model

import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import com.blitterstudio.amiberry.data.HardfileCreator
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import java.io.File
import java.io.RandomAccessFile
import java.util.zip.GZIPOutputStream

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
	fun `a dynamic VHD is probed through its block table`() {
		val vhd = HardfileCreator.create(File(folder.root, "disk.vhd"), 8L * 1024 * 1024, dynamicVhd = true)
		// Unallocated: the disk reads as zeros.
		assertEquals(MountDefaults.Geometry(32, 1, 2, 512), MountDefaults.hdfGeometry(vhd.path))
		RandomAccessFile(vhd, "rw").use { raf ->
			val blockStart = raf.length()
			raf.seek(1536) // the block table
			raf.writeInt((blockStart / 512).toInt())
			// The block: its sector bitmap padded to one sector, then the
			// disk's first sectors, with the RDB in sector 1.
			raf.seek(blockStart)
			raf.write(ByteArray(512) { 0xff.toByte() })
			raf.write(ByteArray(512))
			raf.write(rdsk("RDSK", 512))
			raf.write(ByteArray(512 * 1024 - 1024))
		}
		assertEquals(MountDefaults.Geometry(0, 0, 0, 512), MountDefaults.hdfGeometry(vhd.path))
	}

	@Test
	fun `a gzipped hdz image is probed unpacked`() {
		val plain = image("inner.hdf", 2, rdsk("RDSK", 512))
		val hdz = File(folder.root, "disk.hdz").also { out -> GZIPOutputStream(out.outputStream()).use { it.write(plain.readBytes()) } }
		assertEquals(MountDefaults.Geometry(0, 0, 0, 512), MountDefaults.hdfGeometry(hdz.path))
	}

	@Test
	fun `names drop what a mount line cannot carry`() {
		assertEquals("Work", MountNames.clean("Wo:r,k;\"", device = false))
		assertEquals("MyDisk", MountNames.clean("My Disk", device = true))
		assertEquals("My Disk", MountNames.clean("My Disk", device = false))
		assertFalse(MountNames.allowed(':', device = false))
	}

	/** The schema the launcher ships, with the real mount_controllers table. */
	private fun bundledSchema() = SettingsSchema.parse(org.json.JSONObject(File("src/main/settings/settings_schema.json").readText()))

	@Test
	fun `a new CD drive goes on SCSI when the machine has built-in SCSI`() {
		val schema = bundledSchema()
		val plain = EmulatorSettings()
		val a3000 = EmulatorSettings(options = mapOf("scsi_a3000" to "true"))
		assertEquals("ide0", MountDefaults.newMount(MountKind.Cd, "", plain, schema).controller)
		assertEquals("scsi0", MountDefaults.newMount(MountKind.Cd, "", a3000, schema).controller)
	}

	@Test
	fun `no ninth CD drive can be added`() {
		val schema = bundledSchema()
		var settings = EmulatorSettings()
		repeat(8) { settings = MountDefaults.added(settings, MountKind.Cd, "", schema) }
		assertEquals((0 until 8).toList(), settings.hardDrives.map { it.unit })
		assertFalse(MountDefaults.canAdd(MountKind.Cd, settings, schema))
		assertEquals(settings, MountDefaults.added(settings, MountKind.Cd, "", schema))
		// Tape drives have units of their own.
		assertTrue(MountDefaults.canAdd(MountKind.Tape, settings, schema))
	}

	@Test
	fun `CD drives fill IDE before moving to SCSI, one per unit`() {
		val schema = bundledSchema()
		var settings = EmulatorSettings()
		repeat(3) { settings = MountDefaults.added(settings, MountKind.Cd, "", schema) }
		// IDE (Auto) has a master and a slave; the third goes on SCSI (Auto).
		assertEquals(listOf("ide0", "ide1", "scsi0"), settings.hardDrives.map { it.controller })
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
	fun `picking an RDB image for a hardfile switches it to RDB mode`() {
		val blank = MountEntry(MountKind.Hardfile, "", "DH0", controller = "uae0")
		val picked = MountDefaults.withPath(blank, image("rdb.hdf", 0, rdsk("RDSK", 512)).path)
		assertEquals(listOf(0, 0, 0, 512), listOf(picked.sectors, picked.surfaces, picked.reserved, picked.blockSize))
		// A directory's path carries no geometry.
		val dir = MountEntry(MountKind.Directory, "/a", "DH1", "A")
		assertEquals(dir.copy(path = "/b"), MountDefaults.withPath(dir, "/b"))
	}

	@Test
	fun `volume names keep a folder's dots and drop an archive's extension`() {
		val dir = folder.newFolder("My.Games")
		val archive = folder.newFile("Work,Disk.lha")
		assertEquals("My.Games", MountDefaults.volumeName(dir.path))
		assertEquals("Work_Disk", MountDefaults.volumeName(archive.path))
	}
}
