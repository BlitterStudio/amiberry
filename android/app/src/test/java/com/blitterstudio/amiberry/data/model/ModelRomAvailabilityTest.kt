package com.blitterstudio.amiberry.data.model

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ModelRomAvailabilityTest {

	@Test
	fun `available models only includes models with matching ROM profiles`() {
		val roms = listOf(rom("kick13.A500", 0xc4f0f55fL))

		val models = ModelRomAvailability.availableModels(roms)

		assertTrue(models.contains(AmigaModel.A500))
		// The emulator's A500+ preset asks for its own 2.04 Kickstart.
		assertFalse(models.contains(AmigaModel.A500_PLUS))
		assertFalse(models.contains(AmigaModel.A600))
		assertFalse(models.contains(AmigaModel.A1200))
		assertFalse(models.contains(AmigaModel.CD32))
	}

	@Test
	fun `unknown ROM CRC does not make any model available`() {
		val roms = listOf(rom("unknown.rom", 0x12345678L))

		assertTrue(ModelRomAvailability.availableModels(roms).isEmpty())
	}

	@Test
	fun `CD32 split Kickstart requires extended ROM`() {
		val splitKickOnly = listOf(rom("cd32-kick.rom", 0x1e62d4a5L))
		val splitKickAndExt = splitKickOnly + rom("cd32-ext.rom", 0x87746be2L)

		assertFalse(ModelRomAvailability.isModelAvailable(AmigaModel.CD32, splitKickOnly))
		assertTrue(ModelRomAvailability.isModelAvailable(AmigaModel.CD32, splitKickAndExt))
	}

	@Test
	fun `CD32 combined ROM does not require extended ROM`() {
		val roms = listOf(rom("cd32-combined.rom", 0xf5d4f3c8L))

		assertTrue(ModelRomAvailability.isModelAvailable(AmigaModel.CD32, roms))
	}

	@Test
	fun `CDTV requires both Kickstart and extended ROM`() {
		val kickOnly = listOf(rom("kick13.rom", 0xc4f0f55fL))
		val kickAndExt = kickOnly + rom("cdtv-ext.rom", 0x42baa124L)

		assertFalse(ModelRomAvailability.isModelAvailable(AmigaModel.CDTV, kickOnly))
		assertTrue(ModelRomAvailability.isModelAvailable(AmigaModel.CDTV, kickAndExt))
	}

	@Test
	fun `selected ROMs are deterministic by filename`() {
		val roms = listOf(
			rom("z-kick13.rom", 0xc4f0f55fL, "/roms/z-kick13.rom"),
			rom("a-kick13.rom", 0xc4f0f55fL, "/roms/a-kick13.rom")
		)

		val selected = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, roms)

		assertEquals("/roms/a-kick13.rom", selected.kick?.path)
	}

	@Test
	fun `a model is built from a later configuration when only its ROM is installed`() {
		val kick12 = listOf(rom("kick12.rom", 0xa6ce1636L))
		val a500 = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, kick12)
		assertEquals("/roms/kick12.rom", a500.kick?.path)
		assertEquals("1.2 ROM", AmigaModel.A500.configs[a500.config].label.substringBefore(','))

		val kick31 = listOf(rom("kick31-a3000.rom", 0xefb239ccL))
		val a3000 = ModelRomAvailability.selectRomsForModel(AmigaModel.A3000, kick31)
		assertEquals("/roms/kick31-a3000.rom", a3000.kick?.path)
		assertTrue(AmigaModel.A3000.configs[a3000.config].label.startsWith("3.1 ROM"))
	}

	@Test
	fun `the first configuration wins when its ROM is installed`() {
		val roms = listOf(rom("kick12.rom", 0xa6ce1636L), rom("kick13.rom", 0xc4f0f55fL))
		val selected = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, roms)
		assertEquals(0, selected.config)
		assertEquals("/roms/kick13.rom", selected.kick?.path)
	}

	@Test
	fun `an empty file is not a ROM`() {
		assertFalse(ModelRomAvailability.isRecognizedRom(rom("empty.rom", 0L)))
	}

	private fun rom(name: String, crc32: Long, path: String = "/roms/$name") = AmigaFile(
		path = path,
		name = name,
		extension = name.substringAfterLast('.', missingDelimiterValue = "").lowercase(),
		size = 512 * 1024,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc32
	)
}
