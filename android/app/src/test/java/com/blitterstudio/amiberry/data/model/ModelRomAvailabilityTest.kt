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
	fun `without its Kickstart a model boots the built-in AROS ROM`() {
		val roms = listOf(rom("aros-rom.bin", 0x11111111L), rom("aros-ext.bin", 0x22222222L))

		val selected = ModelRomAvailability.selectRomsForModel(AmigaModel.A1200, roms)

		assertTrue(selected.aros)
		assertTrue(selected.isBootable)
		assertEquals(ModelRomAvailability.arosRomFile, selected.romFile)
		assertEquals("", selected.romExtFile)
		assertTrue(ModelRomAvailability.isModelAvailable(AmigaModel.A500, roms))
		assertFalse(ModelRomAvailability.hasKickstart(AmigaModel.A500, roms))
		// The CD32 and CDTV need their own extended ROMs to play discs.
		assertFalse(ModelRomAvailability.isModelAvailable(AmigaModel.CD32, roms))
		assertFalse(ModelRomAvailability.isModelAvailable(AmigaModel.CDTV, roms))
		// A board with its own boot ROM (the DraCo) needs it too.
		assertFalse(ModelRomAvailability.isModelAvailable(AmigaModel.find("macrosystem")!!, roms))
	}

	@Test
	fun `an installed Kickstart wins over AROS`() {
		val roms = listOf(
			rom("aros-rom.bin", 0x11111111L),
			rom("aros-ext.bin", 0x22222222L),
			rom("kick13.rom", 0xc4f0f55fL)
		)

		val selected = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, roms)

		assertFalse(selected.aros)
		assertEquals("/roms/kick13.rom", selected.romFile)
	}

	@Test
	fun `AROS needs both of its files`() {
		val roms = listOf(rom("aros-rom.bin", 0x11111111L))

		assertFalse(ModelRomAvailability.selectRomsForModel(AmigaModel.A500, roms).isBootable)
		assertEquals("", ModelRomAvailability.selectRomsForModel(AmigaModel.A500, roms).romFile)
	}

	@Test
	fun `ROMs are identified by the emulator's ROM table and AROS by its files`() {
		assertEquals(
			RomIdentity.Kickstart("KS ROM v1.3 (A500,A1000,A2000)"),
			ModelRomAvailability.romIdentity(rom("kick13.rom", 0xc4f0f55fL))
		)
		assertEquals(RomIdentity.BuiltInAros, ModelRomAvailability.romIdentity(rom("aros-ext.bin", 0x22222222L)))
		assertEquals(RomIdentity.Unknown, ModelRomAvailability.romIdentity(rom("mystery.rom", 0x12345678L)))
	}

	@Test
	fun `the AROS extension file is told apart from the AROS ROM`() {
		assertTrue(ModelRomAvailability.isBuiltInArosExtension(rom("AROS-EXT.BIN", 0x22222222L)))
		assertFalse(ModelRomAvailability.isBuiltInArosExtension(rom("aros-rom.bin", 0x11111111L)))
	}

	@Test
	fun `the AROS token is recognised in a kickstart_rom_file value`() {
		assertTrue(ModelRomAvailability.isBuiltInArosRom(":AROS"))
		assertTrue(ModelRomAvailability.isBuiltInArosRom("/games/a.rp9:AROS"))
		assertFalse(ModelRomAvailability.isBuiltInArosRom("/roms/kick13.rom"))
		assertFalse(ModelRomAvailability.isBuiltInArosRom(""))
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
