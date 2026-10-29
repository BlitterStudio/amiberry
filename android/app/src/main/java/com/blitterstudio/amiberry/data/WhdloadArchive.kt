package com.blitterstudio.amiberry.data

import java.io.File
import java.io.IOException
import java.io.InputStream

/**
 * Whether an LhA archive is a WHDLoad game: WHDBooter starts the archive's `.slave`
 * (any member whose name ends in ".slave", any case), so an archive without one boots
 * to an AmigaDOS prompt. Only the member headers are read; the packed data is skipped.
 */
object WhdloadArchive {

	/** True or false for a readable .lha/.lzh archive; null when it cannot tell (LZX, damaged). */
	fun hasSlave(file: File): Boolean? {
		val extension = file.extension.lowercase()
		if (extension != "lha" && extension != "lzh") return null
		val names = try {
			file.inputStream().buffered().use { LhaHeaders.memberNames(it) }
		} catch (_: IOException) {
			null
		} ?: return null
		return names.any { it.endsWith(SLAVE_SUFFIX, ignoreCase = true) }
	}

	private const val SLAVE_SUFFIX = ".slave"
}

/** Reads the member names of an LhA archive (header levels 0, 1 and 2). */
internal object LhaHeaders {

	/** The member names in archive order, or null when the stream is not a whole LhA archive. */
	fun memberNames(input: InputStream): List<String>? {
		val names = mutableListOf<String>()
		while (true) {
			val first = input.read()
			// End of the archive: a zero header size, or the end of the stream.
			if (first <= 0) return names.takeIf { it.isNotEmpty() }
			// Every level has the method at 2..6 and the level at 20.
			val head = input.readExactly(LEVEL_OFFSET) ?: return null
			val start = byteArrayOf(first.toByte()) + head
			if (!hasMethod(start)) return null
			val name = when (val level = start[LEVEL_OFFSET].toInt() and 0xff) {
				0, 1 -> readLevel01(input, start, level)
				2 -> readLevel2(input, start)
				else -> null
			} ?: return null
			names += name
		}
	}

	/**
	 * Level 0/1: byte 0 is the header size less 2; the name follows its length byte at 21.
	 * A level-1 member's packed size counts its extended headers too.
	 */
	private fun readLevel01(input: InputStream, start: ByteArray, level: Int): String? {
		val headerSize = (start[0].toInt() and 0xff) + 2
		if (headerSize < LEVEL_OFFSET + 2) return null
		val header = start + (input.readExactly(headerSize - start.size) ?: return null)
		val nameLength = header[LEVEL_OFFSET + 1].toInt() and 0xff
		if (LEVEL_OFFSET + 2 + nameLength > header.size) return null
		val name = String(header, LEVEL_OFFSET + 2, nameLength, Charsets.ISO_8859_1)
		var packedSize = header.u32(PACKED_SIZE_OFFSET)
		var directory: String? = null
		if (level == 1) {
			// The size of the first extended header closes the base header.
			var next = header.u16(header.size - 2)
			while (next != 0) {
				if (next < 3) return null
				val ext = input.readExactly(next) ?: return null
				packedSize -= next
				if ((ext[0].toInt() and 0xff) == EXT_DIRECTORY) {
					directory = String(ext, 1, next - 3, Charsets.ISO_8859_1)
				}
				next = ext.u16(next - 2)
			}
			if (packedSize < 0) return null
		}
		if (!input.skipExactly(packedSize)) return null
		return withDirectory(directory, name)
	}

	/** Level 2: a 16-bit total header size; names live in extended headers (1 file, 2 directory). */
	private fun readLevel2(input: InputStream, start: ByteArray): String? {
		val totalSize = start.u16(0)
		if (totalSize < LEVEL2_FIXED_SIZE) return null
		val header = start + (input.readExactly(totalSize - start.size) ?: return null)
		var offset = LEVEL2_FIXED_SIZE
		var next = header.u16(offset - 2)
		var name: String? = null
		var directory: String? = null
		while (next != 0) {
			if (next < 3 || offset + next > header.size) return null
			val data = String(header, offset + 1, next - 3, Charsets.ISO_8859_1)
			when (header[offset].toInt() and 0xff) {
				EXT_FILENAME -> name = data
				EXT_DIRECTORY -> directory = data
			}
			offset += next
			next = header.u16(offset - 2)
		}
		if (!input.skipExactly(header.u32(PACKED_SIZE_OFFSET))) return null
		return withDirectory(directory, name ?: return null)
	}

	// Directory names separate their parts with 0xff.
	private fun withDirectory(directory: String?, name: String): String =
		if (directory.isNullOrEmpty()) name else directory.replace('\u00ff', '/').trimEnd('/') + "/" + name

	private fun hasMethod(header: ByteArray): Boolean =
		header[2] == '-'.code.toByte() && header[3] == 'l'.code.toByte() && header[6] == '-'.code.toByte()

	private fun ByteArray.u16(offset: Int): Int =
		(this[offset].toInt() and 0xff) or ((this[offset + 1].toInt() and 0xff) shl 8)

	private fun ByteArray.u32(offset: Int): Long =
		u16(offset).toLong() or (u16(offset + 2).toLong() shl 16)

	private fun InputStream.readExactly(count: Int): ByteArray? {
		if (count < 0) return null
		val buffer = ByteArray(count)
		var read = 0
		while (read < count) {
			val n = read(buffer, read, count - read)
			if (n < 0) return null
			read += n
		}
		return buffer
	}

	private fun InputStream.skipExactly(count: Long): Boolean {
		var remaining = count
		while (remaining > 0) {
			val skipped = skip(remaining)
			if (skipped > 0) {
				remaining -= skipped
			} else {
				if (read() < 0) return false
				remaining--
			}
		}
		return true
	}

	private const val LEVEL_OFFSET = 20
	private const val PACKED_SIZE_OFFSET = 7
	private const val LEVEL2_FIXED_SIZE = 26
	private const val EXT_FILENAME = 0x01
	private const val EXT_DIRECTORY = 0x02
}
