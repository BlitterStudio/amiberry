package com.blitterstudio.amiberry.data.model

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ModelRomAvailabilityTest {

	@Test
	fun `available models only includes models with matching ROM profiles`() {
		val roms = listOf(rom("kick13.rom", 0xc4f0f55fL))

		val models = ModelRomAvailability.availableModels(roms)

		assertTrue(models.contains(AmigaModel.A500))
		assertTrue(models.contains(AmigaModel.A500_PLUS))
		assertTrue(models.contains(AmigaModel.A2000))
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

	private fun rom(name: String, crc32: Long, path: String = "/roms/$name") = AmigaFile(
		path = path,
		name = name,
		extension = "rom",
		size = 512 * 1024,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc32
	)
}
