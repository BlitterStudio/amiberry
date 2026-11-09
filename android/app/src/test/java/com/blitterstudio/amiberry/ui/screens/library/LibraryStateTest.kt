package com.blitterstudio.amiberry.ui.screens.library

import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.RecentLaunches
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.MediaCarryOverRules
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.ConfigInfo
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class LibraryStateTest {

	private fun config(
		name: String,
		category: String = "",
		description: String = "",
		path: String = "/configs/$name.uae"
	): ConfigInfo = ConfigInfo(
		path = path,
		name = name,
		lastModified = 1,
		description = description,
		category = category
	)

	@Test
	fun `machines filter by category and search name description and path`() {
		val configs = listOf(
			config("Workbench", category = "Work", description = "Classic desktop"),
			config("Lotus", category = "Games"),
			config("AGA Demo")
		)

		assertEquals(configs, filteredConfigs(configs, null, ""))
		assertEquals(listOf(configs[0]), filteredConfigs(configs, "Work", ""))
		assertEquals(listOf(configs[0]), filteredConfigs(configs, null, "desktop"))
		assertEquals(listOf(configs[2]), filteredConfigs(configs, null, "AGA"))
		// A category with no machine in it lists nothing.
		assertEquals(emptyList<ConfigInfo>(), filteredConfigs(configs, "Demos", ""))
	}

	@Test
	fun `the detail pane shows the chosen machine while listed, else the first`() {
		val configs = listOf(config("Workbench"), config("Lotus"))

		assertEquals(configs[1], selectedConfiguration(configs, "Lotus.uae"))
		assertEquals(configs[0], selectedConfiguration(configs, null))
		assertEquals(configs[0], selectedConfiguration(configs, "Deleted.uae"))
		assertNull(selectedConfiguration(emptyList(), "Lotus.uae"))
	}

	@Test
	fun `filter names round trip with a safe default`() {
		val categories = listOf("Work", "Games")
		val filters = LibraryFilter.all(categories)
		assertEquals(filters, filters.map { LibraryFilter.fromName(it.persistName) })
		assertEquals(LibraryFilter.AllMachines, LibraryFilter.fromName("missing"))
	}

	@Test
	fun `machine categories are distinct and sorted for a person`() {
		assertEquals(
			listOf("a10", "A9", "Games", "Work"),
			machineCategories(listOf("Work", "A9", "Games", "a10", "Work", ""))
		)
	}

	@Test
	fun `shelf files match every word of the query`() {
		val files = listOf(
			file("Lost Pixel.adf"),
			file("Lotus II.adf")
		)

		assertEquals(files, filterShelfFiles(files, ""))
		assertEquals(listOf(files[0]), filterShelfFiles(files, "lost pixel"))
		assertEquals(emptyList<AmigaFile>(), filterShelfFiles(files, "turrican"))
	}

	@Test
	fun `recent items match title and detail`() {
		val items = listOf(
			RecentLaunches.Item(
				JSONObject("""{"type":"whdload","path":"/games/LostPixel.lha"}"""),
				RecentLaunches.Details("Lost Pixel", RecentLaunches.Type.WhdLoad, "LostPixel.lha"),
				RecentLaunches.Status.Available
			)
		)

		assertEquals(items, filterRecentItems(items, ""))
		assertEquals(items, filterRecentItems(items, "lost"))
		assertEquals(emptyList<RecentLaunches.Item>(), filterRecentItems(items, "turrican"))
	}

	@Test
	fun `a disk boots the model with its own Kickstart before one on AROS`() {
		val floppyModels = setOf("a500", "a1200", "a600")
		val kickstarts = setOf("a1200")

		fun choose(current: String, available: List<String>) = floppyLaunchModel(
			current = current,
			available = available,
			hasFloppy = { it in floppyModels },
			hasKickstart = { it in kickstarts }
		)

		assertEquals("a500", choose("a500", listOf("a500", "a1200", "cd32")))
		assertEquals("a1200", choose("cd32", listOf("a500", "a1200", "cd32")))
		assertEquals("a500", choose("cd32", listOf("a500", "a600", "cd32")))
		assertNull(choose("cd32", listOf("cd32")))
	}

	@Test
	fun `a recent quick start snapshot uses recent model media and current controls`() {
		val currentSettings = EmulatorSettings.fromModel(AmigaModel.A4000).copy(
			romFile = "/roms/current-a4000.rom",
			romExtFile = "/roms/current-ext.rom",
			floppy0 = "/current/df0.adf",
			floppy1 = "/current/df1.adf",
			cdImage = "/current/cd.iso"
		)

		val snapshot = LibraryLaunch.recentQuickStartSettingsSnapshot(
			model = AmigaModel.A500,
			roms = listOf(rom("kick13.rom", 0xc4f0f55fL)),
			currentSettings = currentSettings,
			floppy0 = "",
			floppy1 = "/recent/df1.adf",
			cdImage = ""
		)

		assertEquals(AmigaModel.A500, snapshot.baseModel)
		assertEquals("/roms/kick13.rom", snapshot.romFile)
		assertEquals("", snapshot.floppy0)
		assertEquals("/recent/df1.adf", snapshot.floppy1)
	}

	@Test
	fun `a model change keeps the inserted disks and drops what the new model cannot take`() {
		val before = EmulatorSettings.fromModel(AmigaModel.A500).copy(
			floppy0 = "/floppies/LostPixel.adf",
			cdImage = "/cds/game.iso"
		)
		val carried = MediaCarryOverRules.keepMedia(before, EmulatorSettings.fromModel(AmigaModel.A1200))

		assertEquals("/floppies/LostPixel.adf", carried.settings.floppy0)
		// The A1200 has no CD drive: the CD goes.
		assertEquals(listOf(com.blitterstudio.amiberry.data.model.MediaDrive.CD), carried.dropped)
	}

	@Test
	fun `settings that boot AROS move to the model's Kickstart once installed`() {
		val roms = listOf(rom("kick13.rom", 0xc4f0f55fL))
		val onAros = EmulatorSettings.fromModel(AmigaModel.A500)

		val updated = ModelRomAvailability.withInstalledKickstart(onAros, roms)
		assertEquals("/roms/kick13.rom", updated?.romFile)

		// A settings that names its own Kickstart keeps it.
		val named = onAros.copy(romFile = "/roms/other.rom")
		assertNull(ModelRomAvailability.withInstalledKickstart(named, roms))
	}

	@Test
	fun `the ROM import report names the models the import made ready`() {
		val roms = listOf(rom("kick13.rom", 0xc4f0f55fL))

		val ready = RomImportReport.of(before = 0, roms = roms)
		assertEquals(R.string.library_rom_report_ready, ready.stringRes)

		val nothingNew = RomImportReport.of(before = roms.size, roms = roms)
		assertEquals(R.string.library_rom_report_no_new, nothingNew.stringRes)
	}

	private fun rom(name: String, crc32: Long): AmigaFile = AmigaFile(
		path = "/roms/$name",
		name = name,
		extension = "rom",
		size = 262144,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc32
	)

	private fun file(name: String): AmigaFile = AmigaFile(
		path = "/floppies/$name",
		name = name,
		extension = "adf",
		size = 901120,
		lastModified = 0,
		category = FileCategory.FLOPPIES
	)
}
