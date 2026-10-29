package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class SettingsGroupsTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private val schema = SettingsSchema.current!!

	/** Every option each tab shows: its own rows, then its groups' sections. */
	private fun shownByTab(): Map<SettingsTab, List<String>> = SettingsTabs.all.associateWith { tab ->
		SettingsGroups.laidOut[tab].orEmpty().toList() +
			SettingsGroups.byTab.getValue(tab).flatMap { SettingsGroups.options(schema, it.group).map { o -> o.id } }
	}

	@Test
	fun `every listed option shows exactly once across the tabs`() {
		val listed = schema.options.values.filter { !it.presetOnly }.map { it.id }
		val shown = shownByTab().values.flatten() + SettingsGroups.managed
		assertEquals(shown.size, shown.toSet().size)
		assertEquals(listed.toSet(), shown.toSet())
	}

	@Test
	fun `every tab has a place and each group one tab`() {
		assertEquals(SettingsTabs.all.toSet(), SettingsGroups.byTab.keys)
		val groups = SettingsGroups.byTab.values.flatten().map { it.group }
		assertEquals(groups.size, groups.toSet().size)
		groups.forEach { SettingsGroups.titleRes(it) }
	}

	@Test
	fun `options a preset alone sets do not show`() {
		val presetOnly = schema.options.values.filter { it.presetOnly }.map { it.id }
		assertTrue("ppc_model" in presetOnly)
		val shown = shownByTab().values.flatten()
		assertTrue(presetOnly.none { it in shown })
	}

	@Test
	fun `a group section follows the layer order and leaves out the tab's own rows`() {
		val cpu = SettingsGroups.options(schema, "cpu").map { it.id }
		assertEquals(listOf("cpu_no_unimplemented", "cpu_data_cache"), cpu)
		val order = schema.options.keys.toList()
		val chipset = SettingsGroups.options(schema, "chipset").map { it.id }
		assertEquals(chipset.sortedBy { order.indexOf(it) }, chipset)
		assertFalse("cycle_exact" in chipset)
		assertTrue("chipset_compatible" in chipset)
	}

	@Test
	fun `rows follow the option type`() {
		fun kind(id: String) = SettingsGroups.rowKind(schema.option(id)!!)
		assertEquals(SettingsGroups.RowKind.Switch, kind("cpu_data_cache"))
		assertEquals(SettingsGroups.RowKind.Choice, kind("mmu_model"))
		// Numbers set by feel are sliders, the real throttle included; others are fields.
		assertEquals(SettingsGroups.RowKind.Level, kind("amiberry.cpu_idle"))
		assertEquals(SettingsGroups.RowKind.Level, kind("cpu_throttle"))
		assertEquals(SettingsGroups.RowKind.Level, kind("gfx_gamma"))
		assertEquals(SettingsGroups.RowKind.Number, kind("gfx_monitorblankdelay"))
		assertEquals(SettingsGroups.RowKind.Number, kind("ramsey"))
		assertNull(kind("kickstart_rom_file"))
	}

	@Test
	fun `only advanced groups start collapsed`() {
		assertTrue(SettingsGroups.startsCollapsed("adv_chipset"))
		assertTrue(SettingsGroups.startsCollapsed("display"))
		assertFalse(SettingsGroups.startsCollapsed("cpu"))
		assertFalse(SettingsGroups.startsCollapsed("misc"))
	}

	@Test
	fun `memory boards two to four appear once the board before is fitted`() {
		fun shows(id: String, settings: EmulatorSettings) = SettingsGroups.shows(id) { settings.optionValue(it) }
		val bare = EmulatorSettings(fastRam = 0, z3Ram = 0)
		assertFalse(shows("fastmem2_size", bare))
		assertFalse(shows("z3mem2_size", bare))
		assertTrue(shows("fastmem_size_k", bare))

		assertTrue(shows("fastmem2_size", bare.copy(fastRam = 2)))
		assertTrue(shows("fastmem2_size_k", bare.withOption("fastmem_size_k", "256")))
		assertFalse(shows("fastmem3_size", bare.copy(fastRam = 2)))
		assertTrue(shows("fastmem3_size", bare.copy(fastRam = 2).withOption("fastmem2_size", "4")))
		assertTrue(shows("z3mem2_size", bare.copy(z3Ram = 64)))
		// A board in use shows even when the one before it is not.
		assertTrue(shows("z3mem3_size", bare.withOption("z3mem3_size", "16")))
	}

	@Test
	fun `options without a field round-trip through a configuration file`() {
		val settings = EmulatorSettings()
			.withOption("mmu_model", "68ec030")
			.withOption("cpu_throttle", "200.0")
			.withOption("chipset_compatible", "A1200")
			.withOption("gfx_center_horizontal", "smart")
			.withOption("amiberry.vkbd_transparency", "40")
			.withOption("bsdsocket_emu", "true")
		val file = tempDir.newFile("options.uae")
		file.writeText(ConfigGenerator.generate(settings))

		val parsed = ConfigParser.parse(file)
		assertTrue(parsed.unknownLines.isEmpty())
		for (id in listOf(
			"mmu_model", "cpu_throttle", "chipset_compatible", "gfx_center_horizontal",
			"amiberry.vkbd_transparency", "bsdsocket_emu"
		)) {
			assertEquals(id, settings.optionValue(id), parsed.settings.optionValue(id))
			assertTrue(id, id in parsed.explicitKeys)
		}
	}
}
