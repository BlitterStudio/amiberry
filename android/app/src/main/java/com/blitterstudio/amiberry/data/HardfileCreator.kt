package com.blitterstudio.amiberry.data

import java.io.File
import java.io.RandomAccessFile
import java.security.SecureRandom
import kotlin.math.ceil

/** Writes the raw and dynamic-VHD formats the native hardfile creator writes. */
object HardfileCreator {
	fun create(file: File, sizeBytes: Long, dynamicVhd: Boolean): File {
		require(sizeBytes > 0) { "Hardfile size must be positive" }
		require(!file.exists()) { "Hardfile already exists" }
		file.parentFile?.mkdirs()
		if (dynamicVhd) createDynamicVhd(file, sizeBytes) else RandomAccessFile(file, "rw").use { it.setLength(sizeBytes) }
		return file
	}

	private fun createDynamicVhd(file: File, size: Long) {
		val blockSize = if (size >= 10L * 1024 * 1024 * 1024) 2 * 1024 * 1024 else 512 * 1024
		val entries = ceil(size.toDouble() / blockSize).toInt()
		val batSize = ((entries * 4 + 511) / 512) * 512
		val footer = footer(size)
		val header = dynamicHeader(entries, blockSize)
		RandomAccessFile(file, "rw").use { output ->
			output.write(footer)
			output.write(header)
			val bat = ByteArray(batSize)
			for (i in 0 until entries * 4) bat[i] = 0xff.toByte()
			output.write(bat)
			output.write(footer)
		}
	}

	private fun footer(size: Long): ByteArray = ByteArray(512).also { bytes ->
		putAscii(bytes, 0, "conectix")
		putInt(bytes, 8, 2)
		putInt(bytes, 12, 0x0001_0000)
		putLong(bytes, 16, 512)
		putInt(bytes, 24, ((System.currentTimeMillis() / 1000) - VHD_EPOCH).toInt())
		putAscii(bytes, 28, "vpc ")
		putInt(bytes, 32, 0x0005_0000)
		putAscii(bytes, 36, "Wi2k")
		putLong(bytes, 40, size)
		putLong(bytes, 48, size)
		val (cylinders, heads) = geometry(size)
		putShort(bytes, 56, cylinders)
		bytes[58] = heads.toByte()
		bytes[59] = 63
		putInt(bytes, 60, 3)
		val guid = ByteArray(16)
		SecureRandom().nextBytes(guid)
		System.arraycopy(guid, 0, bytes, 68, guid.size)
		bytes[84] = 0
		putInt(bytes, 64, checksum(bytes, 64))
	}

	private fun dynamicHeader(entries: Int, blockSize: Int): ByteArray = ByteArray(1024).also { bytes ->
		putAscii(bytes, 0, "cxsparse")
		for (i in 8 until 16) bytes[i] = 0xff.toByte()
		putLong(bytes, 16, 1536)
		putInt(bytes, 24, 0x0001_0000)
		putInt(bytes, 28, entries)
		putInt(bytes, 32, blockSize)
		putInt(bytes, 36, checksum(bytes, 36))
	}

	private fun geometry(size: Long): Pair<Int, Int> {
		val totalKb = size / 1024
		val heads = when {
			totalKb <= 504L * 1024 -> 16
			totalKb <= 1008L * 1024 -> 32
			totalKb <= 2016L * 1024 -> 64
			totalKb <= 4032L * 1024 -> 128
			else -> 255
		}
		return ((size / 512 / (63 * heads)).toInt()) to heads
	}

	private fun checksum(bytes: ByteArray, checksumOffset: Int): Int {
		var sum = 0L
		for (index in bytes.indices) if (index !in checksumOffset until checksumOffset + 4) sum += bytes[index].toInt() and 0xff
		return sum.inv().toInt()
	}

	private fun putAscii(bytes: ByteArray, offset: Int, value: String) = value.encodeToByteArray().copyInto(bytes, offset)
	private fun putShort(bytes: ByteArray, offset: Int, value: Int) { bytes[offset] = (value ushr 8).toByte(); bytes[offset + 1] = value.toByte() }
	private fun putInt(bytes: ByteArray, offset: Int, value: Int) { for (i in 0..3) bytes[offset + i] = (value ushr (24 - i * 8)).toByte() }
	private fun putLong(bytes: ByteArray, offset: Int, value: Long) { for (i in 0..7) bytes[offset + i] = (value ushr (56 - i * 8)).toByte() }

	private const val VHD_EPOCH = 946684800L
}
