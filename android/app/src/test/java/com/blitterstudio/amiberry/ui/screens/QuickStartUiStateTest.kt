package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.data.model.SettingText
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
	fun `whdload summary shows only what the launcher passes, and leaves the hardware to WHDBooter`() {
		val summary = QuickStartUiState.buildWhdLoadSummary(
			launchSettings = EmulatorSettings.fromModel(AmigaModel.A500).copy(
				joyport0 = "mouse",
				joyport1 = "joy0",
				onScreenJoystick = false,
				onScreenKeyboardButton = true,
				shader = "presets/crt.glslp"
			),
			selectedWhdloadName = "Lotus.lha",
			perGameConfigName = null
		)

		assertEquals(
			QuickStartWhdLoadSummary(
				media = "WHDLoad: Lotus",
				perGameConfig = null,
				controls = "On-screen keyboard",
				port0 = SettingText.Resource(R.string.settings_input_device_mouse),
				port1 = SettingText.Resource(R.string.settings_input_device_joystick_0),
				shader = SettingText.Plain("crt.glslp")
			),
			summary
		)
	}

	@Test
	fun `whdload summary names the game's own configuration`() {
		val summary = QuickStartUiState.buildWhdLoadSummary(
			launchSettings = EmulatorSettings(),
			selectedWhdloadName = "Lotus.lzx",
			perGameConfigName = "Lotus.uae"
		)

		assertEquals("WHDLoad: Lotus", summary.media)
		assertEquals("Lotus.uae", summary.perGameConfig)
	}

	@Test
	fun `the WHDLoad AROS notice follows the A1200 Kickstart, whatever model the settings hold`() {
		val aros = listOf(rom("aros-rom.bin", 0x11111111L), rom("aros-ext.bin", 0x22222222L))
		val withA500Kick = aros + rom("kick13.rom", 0xc4f0f55fL)
		val withA1200Kick = aros + rom("kick31.rom", 0x1483a091L)

		assertTrue(QuickStartUiState.showsWhdLoadArosNotice(aros))
		assertTrue(QuickStartUiState.showsWhdLoadArosNotice(withA500Kick))
		assertFalse(QuickStartUiState.showsWhdLoadArosNotice(withA1200Kick))
		// Without AROS the launch cannot start at all; the setup card covers that.
		assertFalse(QuickStartUiState.showsWhdLoadArosNotice(emptyList()))
		// The machine notice is for Disk and CD; WHDLoad has its own.
		val onAros = EmulatorSettings.fromModel(AmigaModel.A500).copy(romFile = ModelRomAvailability.arosRomFile)
		val arosRoms = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, aros)
		assertFalse(QuickStartUiState.showsArosNotice(onAros, arosRoms, QuickStartLaunchMode.WHDLOAD))
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
			settings = EmulatorSettings.fromModel(model).copy(romFile = selected.romFile),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = available,
			roms = roms
		)
		assertEquals(RomIdentity.BuiltInAros, summary.rom)
	}

	@Test
	fun `the summary names an installed Kickstart from the emulator's ROM table`() {
		val roms = listOf(rom("kick13.rom", 0xc4f0f55fL))
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A500).copy(romFile = "/roms/kick13.rom"),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true,
			roms = roms
		)

		assertEquals(RomIdentity.Kickstart("KS ROM v1.3 (A500,A1000,A2000)"), summary.rom)
	}

	@Test
	fun `importing the model's Kickstart moves AROS settings to it, and the notice and summary follow`() {
		val aros = listOf(rom("aros-rom.bin", 0x11111111L), rom("aros-ext.bin", 0x22222222L))
		val onAros = EmulatorSettings.fromModel(AmigaModel.A500).copy(romFile = ModelRomAvailability.arosRomFile)
		val arosRoms = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, aros)
		assertTrue(QuickStartUiState.showsArosNotice(onAros, arosRoms, QuickStartLaunchMode.DISK))

		val withKick = aros + rom("kick13.rom", 0xc4f0f55fL)
		val updated = QuickStartUiState.withInstalledKickstart(onAros, withKick)!!

		assertEquals("/roms/kick13.rom", updated.romFile)
		assertEquals("", updated.romExtFile)
		val kickRoms = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, withKick)
		assertFalse(QuickStartUiState.showsArosNotice(updated, kickRoms, QuickStartLaunchMode.DISK))
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = updated,
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true,
			roms = withKick
		)
		assertEquals(RomIdentity.Kickstart("KS ROM v1.3 (A500,A1000,A2000)"), summary.rom)
	}

	@Test
	fun `a ROM import leaves a chosen Kickstart, and AROS without the model's Kickstart, alone`() {
		val roms = listOf(
			rom("aros-rom.bin", 0x11111111L),
			rom("aros-ext.bin", 0x22222222L),
			rom("kick13.rom", 0xc4f0f55fL)
		)
		val chosen = EmulatorSettings.fromModel(AmigaModel.A500).copy(romFile = "/roms/other.rom")
		assertEquals(null, QuickStartUiState.withInstalledKickstart(chosen, roms))

		val a1200OnAros = EmulatorSettings.fromModel(AmigaModel.A1200).copy(romFile = ModelRomAvailability.arosRomFile)
		assertEquals(null, QuickStartUiState.withInstalledKickstart(a1200OnAros, roms))
	}

	@Test
	fun `the AROS notice shows for settings without a ROM when the model boots AROS, not in Recent`() {
		val aros = listOf(rom("aros-rom.bin", 0x11111111L), rom("aros-ext.bin", 0x22222222L))
		val selected = ModelRomAvailability.selectRomsForModel(AmigaModel.A500, aros)
		val noRom = EmulatorSettings.fromModel(AmigaModel.A500).copy(romFile = "")

		assertTrue(QuickStartUiState.showsArosNotice(noRom, selected, QuickStartLaunchMode.CD))
		assertFalse(QuickStartUiState.showsArosNotice(noRom, selected, QuickStartLaunchMode.RECENT))
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

	@Test
	fun `memory summary lists every memory the machine has`() {
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A3000, 0, 0),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true
		)
		assertEquals("2 MB Chip, 8 MB Motherboard Fast RAM", summary.memory)

		// A board below 1 MB is named as its board, not as its KB option.
		val small = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A500)
				.withOption("fastmem_size_k", "512")
				.withOption("fastmem2_size_k", "256"),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true
		)
		assertEquals("512 KB Chip, 512 KB Slow, 512 KB Fast, 256 KB Z2 Fast RAM board 2", small.memory)
	}
}
