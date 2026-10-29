package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.data.model.SettingText
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class QuickStartHomeStateTest {

	@Test
	fun `a model change keeps the inserted disks and enables DF1`() {
		val before = EmulatorSettings.fromModel(AmigaModel.A500).copy(
			floppy0 = "/floppies/LostPixel.adf",
			floppy1 = "/floppies/Data.adf",
			floppy1Type = 0
		)
		val next = EmulatorSettings.fromModel(AmigaModel.A1200)

		val carried = QuickStartMediaState.keepMedia(before, next)

		assertEquals("/floppies/LostPixel.adf", carried.settings.floppy0)
		assertEquals("/floppies/Data.adf", carried.settings.floppy1)
		assertTrue(carried.settings.floppy1Type != -1)
		assertEquals(AmigaModel.A1200, carried.settings.baseModel)
		assertTrue(carried.dropped.isEmpty())
	}

	@Test
	fun `picking the same model again keeps the disk`() {
		val before = EmulatorSettings.fromModel(AmigaModel.A500).copy(floppy0 = "/floppies/LostPixel.adf")

		val carried = QuickStartMediaState.keepMedia(before, EmulatorSettings.fromModel(AmigaModel.A500))

		assertEquals(before.floppy0, carried.settings.floppy0)
	}

	@Test
	fun `a model without floppy drives drops the disks and names them, a CD model keeps the CD`() {
		val before = EmulatorSettings.fromModel(AmigaModel.CDTV).copy(
			floppy0 = "/floppies/Disk.adf",
			cdImage = "/cds/Game.iso"
		)

		val toCd32 = QuickStartMediaState.keepMedia(before, EmulatorSettings.fromModel(AmigaModel.CD32))
		assertEquals("", toCd32.settings.floppy0)
		assertEquals("/cds/Game.iso", toCd32.settings.cdImage)
		assertEquals(listOf(MediaDrive.DF0), toCd32.dropped)

		val toA500 = QuickStartMediaState.keepMedia(before, EmulatorSettings.fromModel(AmigaModel.A500))
		assertEquals("/floppies/Disk.adf", toA500.settings.floppy0)
		assertEquals("", toA500.settings.cdImage)
		assertEquals(listOf(MediaDrive.CD), toA500.dropped)
	}

	@Test
	fun `Quick Start opens in Recent once something was launched, then WHDLoad with games, else Disk`() {
		assertEquals(QuickStartLaunchMode.RECENT, QuickStartUiState.defaultLaunchMode(hasRecent = true, hasWhdloadGames = true))
		assertEquals(QuickStartLaunchMode.WHDLOAD, QuickStartUiState.defaultLaunchMode(hasRecent = false, hasWhdloadGames = true))
		assertEquals(QuickStartLaunchMode.DISK, QuickStartUiState.defaultLaunchMode(hasRecent = false, hasWhdloadGames = false))
	}

	@Test
	fun `the summary names the Kickstart a launch picks when the settings name none`() {
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A500),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true,
			roms = listOf(rom("kick13.rom", KICK13_CRC))
		)

		assertEquals(RomIdentity.Kickstart("KS ROM v1.3 (A500,A1000,A2000)"), summary.rom)
		assertFalse(summary.romMissing)
	}

	@Test
	fun `a deleted Kickstart shows as missing, falling back to AROS where the model can`() {
		val arosOnly = listOf(rom("aros-rom.bin", 0x11111111L), rom("aros-ext.bin", 0x22222222L))
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A1200).copy(romFile = "/roms/amiga-os-310-a1200.rom"),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true,
			roms = arosOnly,
			fileExists = { false }
		)

		assertTrue(summary.romMissing)
		assertTrue(summary.romFallsBackToAros)
		assertNull(summary.rom)
		assertEquals("amiga-os-310-a1200.rom", summary.romFileName)
	}

	@Test
	fun `a Kickstart outside the ROM folder that still exists is not missing`() {
		val summary = QuickStartUiState.buildEffectiveSummary(
			settings = EmulatorSettings.fromModel(AmigaModel.A500).copy(romFile = "/sdcard/kick.rom"),
			launchMode = QuickStartLaunchMode.DISK,
			selectedModelAvailable = true,
			roms = emptyList(),
			fileExists = { it == "/sdcard/kick.rom" }
		)

		assertFalse(summary.romMissing)
		assertEquals(RomIdentity.Unknown, summary.rom)
	}

	@Test
	fun `a model names the ROMs it is built with`() {
		assertEquals(listOf("KS ROM v1.3 (A500,A1000,A2000)"), QuickStartUiState.romsNeeded(AmigaModel.A500))
		assertEquals(listOf("A1000 bootstrap ROM"), QuickStartUiState.romsNeeded(AmigaModel.A1000))
		assertEquals(listOf("CD32 ROM (KS + extended)"), QuickStartUiState.romsNeeded(AmigaModel.CD32))
		assertEquals(2, QuickStartUiState.romsNeeded(AmigaModel.CDTV).size)
	}

	@Test
	fun `every model has one spec line of CPU, chipset and memory`() {
		assertEquals("68000 · OCS + OCS Denise · 512 KB Chip, 512 KB Slow", QuickStartUiState.modelSpec(AmigaModel.A500))
		AmigaModel.entries.forEach { model ->
			assertEquals(model.cmdArg, 3, QuickStartUiState.modelSpec(model).split(" · ").size)
		}
	}

	@Test
	fun `machine edits are listed against the preset the settings came from, controls and media are not`() {
		val preset = EmulatorSettings.fromModel(AmigaModel.A1200)
		assertTrue(QuickStartUiState.presetEdits(preset).isEmpty())
		assertTrue(
			QuickStartUiState.presetEdits(preset.copy(floppy0 = "/floppies/Disk.adf", joyport1 = "joy0", shader = "crt")).isEmpty()
		)

		val edits = QuickStartUiState.presetEdits(preset.withOption("cpu_model", "68030"))
		assertEquals(listOf("CPU model"), edits.map { it.label }.filter { it == "CPU model" })
		// Named as the layer's choice label ("68030 (A3000/A4000)").
		val cpu = edits.first { it.label == "CPU model" }.value as SettingText.Plain
		assertTrue(cpu.text, cpu.text.startsWith("68030"))
	}

	@Test
	fun `the WHDLoad list puts recently played games first, then the rest by title, and searches titles`() {
		val lostPixel = game("TheLostPixelv1.3_WHD4.lha")
		val castlevania = game("CastlevaniaAGA_WHDLoad.lha")
		val alienBreed = game("AlienBreed_v2.1.lha")
		val games = listOf(lostPixel, castlevania, alienBreed)

		assertEquals(
			listOf(lostPixel, alienBreed, castlevania),
			QuickStartUiState.whdloadGameList(games, recentPaths = listOf(lostPixel.path), query = "")
		)
		assertEquals(
			listOf(lostPixel),
			QuickStartUiState.whdloadGameList(games, recentPaths = emptyList(), query = "lost pixel")
		)
		assertEquals(
			listOf(castlevania),
			QuickStartUiState.whdloadGameList(games, recentPaths = emptyList(), query = "aga")
		)
	}

	private fun game(name: String) = AmigaFile(
		path = "/whdload/$name",
		name = name,
		extension = "lha",
		size = 1,
		lastModified = 1,
		category = FileCategory.WHDLOAD_GAMES
	)

	private fun rom(name: String, crc32: Long) = AmigaFile(
		path = "/roms/$name",
		name = name,
		extension = "rom",
		size = 1,
		lastModified = 1,
		category = FileCategory.ROMS,
		crc32 = crc32
	)

	private companion object {
		const val KICK13_CRC = 0xc4f0f55fL
	}
}
