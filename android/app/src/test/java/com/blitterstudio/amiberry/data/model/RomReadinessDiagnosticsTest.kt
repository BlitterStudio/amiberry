package com.blitterstudio.amiberry.data.model

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class RomReadinessDiagnosticsTest {

	@Test
	fun `no ROMs reports setup as missing`() {
		val diagnostics = RomReadinessDiagnostics.from(emptyList())

		assertEquals(RomReadinessStatus.NoRoms, diagnostics.status)
		assertEquals(0, diagnostics.scannedRomCount)
		assertEquals(0, diagnostics.recognizedRomCount)
		assertTrue(diagnostics.availableModels.isEmpty())
	}

	@Test
	fun `unknown ROMs are counted but do not enable quick start models`() {
		val diagnostics = RomReadinessDiagnostics.from(
			listOf(rom("unknown.rom", 0x12345678L))
		)

		assertEquals(RomReadinessStatus.NoCompatibleModels, diagnostics.status)
		assertEquals(1, diagnostics.scannedRomCount)
		assertEquals(0, diagnostics.recognizedRomCount)
		assertEquals(1, diagnostics.unknownRomCount)
	}

	@Test
	fun `recognized ROMs report available and missing common models`() {
		val diagnostics = RomReadinessDiagnostics.from(
			listOf(
				rom("kick13.rom", 0xc4f0f55fL),
				rom("kick31-a1200.rom", 0x6c9b07d2L)
			)
		)

		assertEquals(RomReadinessStatus.Ready, diagnostics.status)
		assertEquals(2, diagnostics.recognizedRomCount)
		assertTrue(AmigaModel.A500 in diagnostics.availableModels)
		assertTrue(AmigaModel.A1200 in diagnostics.availableModels)
		assertTrue(AmigaModel.CD32 in diagnostics.missingCommonModels)
	}

	private fun rom(name: String, crc32: Long, path: String = "/roms/$name") = AmigaFile(
		path = path,
		name = name,
		extension = "rom",
		size = 524288,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc32
	)
}
