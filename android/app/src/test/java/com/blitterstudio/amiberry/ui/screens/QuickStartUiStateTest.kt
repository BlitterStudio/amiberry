package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class QuickStartUiStateTest {

	@Test
	fun `disk summary describes the currently applied emulator settings and selected media`() {
		val settings = EmulatorSettings.fromModel(AmigaModel.A1200).copy(
			cpuModel = 68030,
			address24Bit = false,
			floppy0 = "/media/Workbench.adf",
			floppy1 = "/media/Extras.adf",
			onScreenJoystick = false,
			onScreenKeyboardButton = true
		)

		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = settings,
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true
		)

		assertEquals("Amiga 1200", summary.model)
		assertEquals("68030, 32-bit addressing", summary.cpu)
		assertEquals("AGA", summary.chipset)
		assertEquals("2 MB Chip", summary.memory)
		assertEquals("DF0: Workbench.adf, DF1: Extras.adf", summary.media)
		assertEquals("On-screen keyboard", summary.controls)
	}

	@Test
	fun `whdload summary shows the selected title and inherited auto-configured model`() {
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A1200),
			launchMode = QuickStartLaunchMode.WHDLOAD,
			selectedModelAvailable = true,
			selectedWhdloadName = "Lotus.lha"
		)

		assertEquals("Amiga 1200", summary.model)
		assertEquals("WHDLoad: Lotus", summary.media)
	}

	@Test
	fun `a model without its Kickstart starts on AROS and the summary says so`() {
		val roms = listOf(rom("aros-rom.bin", 0x11111111L), rom("aros-ext.bin", 0x22222222L))
		val model = AmigaModel.A1200
		val selected = ModelRomAvailability.selectRomsForModel(model, roms)
		val available = model in ModelRomAvailability.availableModels(roms)

		assertTrue(selected.aros)
		assertTrue(
			QuickStartUiState.canStart(
				launchMode = QuickStartLaunchMode.DISK,
				hasRoms = roms.isNotEmpty(),
				selectedModelAvailable = available,
				hasSelectedWhdloadGame = false
			)
		)
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(model),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = available,
			romFile = selected.romFile,
			roms = roms
		)
		assertEquals(RomIdentity.BuiltInAros, summary.rom)
	}

	@Test
	fun `the summary names an installed Kickstart from the emulator's ROM table`() {
		val roms = listOf(rom("kick13.rom", 0xc4f0f55fL))
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A500),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true,
			romFile = "/roms/kick13.rom",
			roms = roms
		)

		assertEquals(RomIdentity.Kickstart("KS ROM v1.3 (A500,A1000,A2000)"), summary.rom)
	}

	private fun rom(name: String, crc32: Long) = AmigaFile(
		path = "/roms/$name",
		name = name,
		extension = name.substringAfterLast('.'),
		size = 524288,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc32
	)

	@Test
	fun `start availability follows the active launch mode`() {
		assertTrue(
			QuickStartUiState.canStart(
				launchMode = QuickStartLaunchMode.WHDLOAD,
				hasRoms = true,
				selectedModelAvailable = false,
				hasSelectedWhdloadGame = true
			)
		)
		assertFalse(
			QuickStartUiState.canStart(
				launchMode = QuickStartLaunchMode.WHDLOAD,
				hasRoms = true,
				selectedModelAvailable = true,
				hasSelectedWhdloadGame = false
			)
		)
		assertTrue(
			QuickStartUiState.canStart(
				launchMode = QuickStartLaunchMode.DISK,
				hasRoms = true,
				selectedModelAvailable = true,
				hasSelectedWhdloadGame = false
			)
		)
		assertFalse(
			QuickStartUiState.canStart(
				launchMode = QuickStartLaunchMode.RECENT,
				hasRoms = true,
				selectedModelAvailable = true,
				hasSelectedWhdloadGame = true
			)
		)
	}
}
