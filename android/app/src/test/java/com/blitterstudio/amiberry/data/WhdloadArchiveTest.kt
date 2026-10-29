package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.ByteArrayOutputStream
import java.io.File

class WhdloadArchiveTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `level 0 archive lists members and finds the slave`() {
		val archive = lha(LhaMember("Game\\Game.Slave", "slave", level = 0), LhaMember("Game\\ReadMe", "readme", level = 0))

		assertEquals(listOf("Game\\Game.Slave", "Game\\ReadMe"), LhaHeaders.memberNames(archive.inputStream()))
		assertEquals(true, WhdloadArchive.hasSlave(write("game.lha", archive)))
	}

	@Test
	fun `level 1 archive joins extended directory headers to names`() {
		val archive = lha(
			LhaMember("ReadMe", "readme", level = 1, directory = "Game"),
			LhaMember("game.slave", "slave", level = 1, directory = "Game\u00ffdata")
		)

		assertEquals(listOf("Game/ReadMe", "Game/data/game.slave"), LhaHeaders.memberNames(archive.inputStream()))
		assertEquals(true, WhdloadArchive.hasSlave(write("game.lzh", archive)))
	}

	@Test
	fun `level 2 archive reads names from extended headers`() {
		val archive = lha(
			LhaMember("intro.dat", "data".repeat(100), level = 2, directory = "Game"),
			LhaMember("Game.SLAVE", "slave", level = 2, directory = "Game")
		)

		assertEquals(listOf("Game/intro.dat", "Game/Game.SLAVE"), LhaHeaders.memberNames(archive.inputStream()))
		assertEquals(true, WhdloadArchive.hasSlave(write("game.lha", archive)))
	}

	@Test
	fun `archive without a slave is not a WHDLoad game`() {
		val archive = lha(
			LhaMember("dynAMIte/dynAMIte", "exe", level = 2),
			LhaMember("dynAMIte/Samples/duell.snd", "snd", level = 1)
		)

		assertEquals(false, WhdloadArchive.hasSlave(write("dynamite.lha", archive)))
	}

	@Test
	fun `unreadable or unsupported archives are unknown`() {
		val archive = lha(LhaMember("Game.slave", "slave".repeat(50), level = 2))

		assertNull(WhdloadArchive.hasSlave(write("cut.lha", archive.copyOf(15))))
		assertNull(WhdloadArchive.hasSlave(write("notes.lha", "not an archive at all".toByteArray())))
		assertNull(WhdloadArchive.hasSlave(write("empty.lha", ByteArray(0))))
		assertNull(WhdloadArchive.hasSlave(write("game.lzx", archive)))
	}

	private fun write(name: String, bytes: ByteArray): File =
		tempDir.newFile(name).apply { writeBytes(bytes) }

	private data class LhaMember(val name: String, val content: String, val level: Int, val directory: String? = null)

	/** A stored (-lh0-) archive in the given header levels, as lha/Lhasa writes them. */
	private fun lha(vararg members: LhaMember): ByteArray {
		val out = ByteArrayOutputStream()
		for (member in members) {
			val data = member.content.toByteArray(Charsets.ISO_8859_1)
			val name = member.name.toByteArray(Charsets.ISO_8859_1)
			when (member.level) {
				0, 1 -> {
					val extended = ByteArrayOutputStream()
					member.directory?.let { dir ->
						val dirBytes = dir.toByteArray(Charsets.ISO_8859_1)
						extended.write(0x02)
						extended.write(dirBytes)
						extended.u16(0)
					}
					val ext = extended.toByteArray()
					val header = ByteArrayOutputStream()
					header.write(0) // size, patched below
					header.write(0) // checksum, patched below
					header.write("-lh0-".toByteArray())
					header.u32(data.size + ext.size)
					header.u32(data.size)
					header.u32(0)
					header.write(0x20)
					header.write(member.level)
					header.write(name.size)
					header.write(name)
					header.u16(0) // CRC16 of the data, unchecked here
					if (member.level == 1) {
						header.write('U'.code)
						header.u16(if (ext.isEmpty()) 0 else ext.size)
					}
					val bytes = header.toByteArray()
					bytes[0] = (bytes.size - 2).toByte()
					bytes[1] = (bytes.drop(2).sumOf { it.toInt() and 0xff } and 0xff).toByte()
					out.write(bytes)
					out.write(ext)
				}
				2 -> {
					val extended = ByteArrayOutputStream()
					fun ext(type: Int, payload: ByteArray) {
						extended.u16(payload.size + 3)
						extended.write(type)
						extended.write(payload)
					}
					ext(0x01, name)
					member.directory?.let { ext(0x02, (it + "\u00ff").toByteArray(Charsets.ISO_8859_1)) }
					extended.u16(0)
					val ext = extended.toByteArray()
					val header = ByteArrayOutputStream()
					header.u16(0) // total size, patched below
					header.write("-lh0-".toByteArray())
					header.u32(data.size)
					header.u32(data.size)
					header.u32(0)
					header.write(0x20)
					header.write(2)
					header.u16(0) // CRC16
					header.write('U'.code)
					header.write(ext)
					val bytes = header.toByteArray()
					bytes[0] = (bytes.size and 0xff).toByte()
					bytes[1] = (bytes.size shr 8).toByte()
					out.write(bytes)
				}
			}
			out.write(data)
		}
		out.write(0)
		return out.toByteArray()
	}

	private fun ByteArrayOutputStream.u16(value: Int) {
		write(value and 0xff)
		write((value shr 8) and 0xff)
	}

	private fun ByteArrayOutputStream.u32(value: Int) {
		u16(value and 0xffff)
		u16((value shr 16) and 0xffff)
	}
}
