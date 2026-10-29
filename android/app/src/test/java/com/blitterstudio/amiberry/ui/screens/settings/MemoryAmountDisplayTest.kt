package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.junit.Assert.assertEquals
import org.junit.Test

class MemoryAmountDisplayTest {

	@Test
	fun `memory display preserves fractional megabytes`() {
		assertEquals(MemoryAmountDisplay.FractionalMegabytes(tenths = 15), memoryAmountDisplayValue(1536))
		assertEquals(MemoryAmountDisplay.FractionalMegabytes(tenths = 18), memoryAmountDisplayValue(1792))
	}

	@Test
	fun `memory display keeps none kilobyte and whole megabyte values`() {
		assertEquals(MemoryAmountDisplay.None, memoryAmountDisplayValue(0))
		assertEquals(MemoryAmountDisplay.Kilobytes(512), memoryAmountDisplayValue(512))
		assertEquals(MemoryAmountDisplay.WholeMegabytes(1), memoryAmountDisplayValue(1024))
		assertEquals(MemoryAmountDisplay.WholeMegabytes(2), memoryAmountDisplayValue(2048))
	}

	private val schema = SettingsSchema.current!!

	@Test
	fun `the summary counts every memory option in use`() {
		val settings = EmulatorSettings(chipRam = 4, slowRam = 2, fastRam = 2, z3Ram = 64)
			.withOption("fastmem2_size", "4")
			.withOption("fastmem3_size_k", "512")
			.withOption("z3mem2_size", "32")
			.withOption("a3000mem_size", "8")
			.withOption("mbresmem_size", "16")
			.withOption("megachipmem_size", "16")
			.withOption("z3mapping", "real")
		val installed = installedMemory(settings, schema)
		assertEquals(
			listOf(
				"chipmem_size", "bogomem_size", "fastmem_size", "fastmem2_size", "fastmem3_size_k", "z3mem_size",
				"z3mem2_size", "a3000mem_size", "mbresmem_size", "megachipmem_size"
			).sorted(),
			installed.map { it.id }.sorted()
		)
		val mb = 2 + 4 + 64 + 32 + 8 + 16 + 16
		assertEquals(2048 + 512 + 512 + mb * 1024, installed.sumOf { it.kb })
	}

	@Test
	fun `a board's KB size is its size where set`() {
		// The launcher writes fastmem_size_k after fastmem_size; the later key sets the board.
		val small = EmulatorSettings(chipRam = 1, slowRam = 0, fastRam = 2).withOption("fastmem_size_k", "256")
		assertEquals(listOf("chipmem_size" to 512, "fastmem_size_k" to 256), installedMemory(small, schema).map { it.id to it.kb })
	}
}
