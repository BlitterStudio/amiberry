package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
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
			onScreenKeyboard = true
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
