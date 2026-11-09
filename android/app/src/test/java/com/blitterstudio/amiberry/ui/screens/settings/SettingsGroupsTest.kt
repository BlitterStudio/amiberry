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

	/** Every option each content page shows: its own rows, then its groups' sections. */
	private fun shownByPage(): Map<SettingsPage, List<String>> =
		SettingsPages.all.associateWith { page ->
			SettingsGroups.laidOut[page].orEmpty().toList() +
				SettingsGroups.byPage[page].orEmpty().flatMap { SettingsGroups.options(schema, it.group).map { o -> o.id } }
		}

	@Test
	fun `every listed option shows exactly once across the pages`() {
		val listed = schema.options.values.filter { !it.presetOnly }.map { it.id }
		val shown = shownByPage().values.flatten() + SettingsGroups.managed
		assertEquals(shown.size, shown.toSet().size)
		assertEquals(listed.toSet(), shown.toSet())
	}
	@Test
	fun `each option lands on exactly one page, the page that owns it`() {
		for (option in schema.options.values) {
			if (option.presetOnly || option.id in SettingsGroups.managed) continue
			assertEquals(option.id, SettingsGroups.pageFor(option.id), pageOwning(option.id))
		}
	}

	/** The page the option's group section or own row shows it on, computed independently of pageFor. */
	private fun pageOwning(optionId: String): SettingsPage {
		val own = shownByPage().entries.singleOrNull { (_, ids) -> optionId in ids }
		return own?.key ?: error("$optionId shows on no page")
	}

	@Test
	fun `all settings lists every option a row can edit, except the managed ones`() {
		val all = SettingsGroups.groupsOf(SettingsPage.AllSettings, schema)
			.flatMap { SettingsGroups.options(schema, it, placed = SettingsGroups.managed).map { o -> o.id } }
		val listed = schema.options.values
			.filter { !it.presetOnly && SettingsGroups.rowKind(it) != null }
			.map { it.id }
			// The launcher writes these itself (or resets them): no control works.
			.filter { it !in SettingsGroups.managed }
		assertEquals(listed.toSet(), all.toSet())
		// Every group the layer has, in the layer's order.
		assertEquals(schema.options.values.map { it.group }.distinct(), SettingsGroups.groupsOf(SettingsPage.AllSettings, schema))
		// The options whose working control is a page's own row are listed too.
		assertTrue("joyport0" in all)
		assertTrue("joyport1" in all)
		assertTrue("amiberry.shader" in all)
		assertTrue("cachesize" in all)
		// The JIT cache takes 0 (off) or at least 128 KB, on every page that edits it.
		val accepts = SettingsGroups.numberAccepts("cachesize")
		assertTrue(accepts(0))
		assertTrue(accepts(128))
		assertFalse(accepts(64))
		assertTrue(SettingsGroups.numberAccepts("ramsey")(3))
	}

	@Test
	fun `each group has exactly one page and every page has a place`() {
		assertEquals(SettingsPages.all.filter { it != SettingsPage.AllSettings }.toSet(), SettingsGroups.byPage.keys)
		val groups = SettingsGroups.byPage.values.flatten().map { it.group }
		assertEquals(groups.size, groups.toSet().size)
		// Every group the layer has is placed (Apple's pages, without the ones the
		// launcher's layer does not have: no x86, diskswapper, drawbridge or priority).
		assertEquals(schema.options.values.map { it.group }.distinct().toSet(), groups.toSet())
		groups.forEach { SettingsGroups.titleRes(it) }
	}

	@Test
	fun `the pages cover the Apple app's groups`() {
		assertEquals(
			listOf("cpu", "mmu", "fpu", "cpu_speed", "jit", "chipset", "adv_chipset", "memory", "rom", "accelerator", "rtg", "expansions", "ppc", "genlock"),
			SettingsGroups.byPage.getValue(SettingsPage.Hardware).map { it.group }
		)
		assertEquals(listOf("display", "filter", "sound"), SettingsGroups.byPage.getValue(SettingsPage.DisplaySound).map { it.group })
		assertEquals(listOf("input", "onscreen"), SettingsGroups.byPage.getValue(SettingsPage.Input).map { it.group })
		assertEquals(listOf("floppy", "cd", "filesystem"), SettingsGroups.byPage.getValue(SettingsPage.Storage).map { it.group })
		assertEquals(listOf("misc", "io"), SettingsGroups.byPage.getValue(SettingsPage.PortsMisc).map { it.group })
	}

	@Test
	fun `options a preset alone sets do not show`() {
		val presetOnly = schema.options.values.filter { it.presetOnly }.map { it.id }
		assertTrue("ppc_model" in presetOnly)
		val shown = shownByPage().values.flatten()
		assertTrue(presetOnly.none { it in shown })
	}

	@Test
	fun `a group section follows the layer order and leaves out the page's own rows`() {
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
	fun `an option is named in its section's words, not the section's`() {
		// The Apple app's OptionSection.label(_:in:).
		assertEquals("Extra", SettingsGroups.labelIn("Chipset extra", "Chipset"))
		assertEquals("Adjustment", SettingsGroups.labelIn("CPU speed adjustment", "CPU speed"))
		assertEquals("Image", SettingsGroups.labelIn("DF0: image", "DF0:"))
		// A label that is the section's name stays.
		assertEquals("Chipset", SettingsGroups.labelIn("Chipset", "Chipset"))
		// A label that only starts with the same letters stays.
		assertEquals("Floppy drive emulation speed", SettingsGroups.labelIn("Floppy drive emulation speed", "Floppy drives"))
		// A label the section does not start stays whole.
		assertEquals("NTSC", SettingsGroups.labelIn("NTSC", "Chipset"))
		// A prefix that is not followed by a space or colon stays whole.
		assertEquals("Chipsetry", SettingsGroups.labelIn("Chipsetry", "Chipset"))
		// A label that is the section plus a separator alone stays.
		assertEquals("Chipset:", SettingsGroups.labelIn("Chipset:", "Chipset"))
	}

	@Test
	fun `a section's footer says each unavailability reason once, naming a lone option`() {
		// The Apple app's OptionSection.constraintNotes.
		val a = schema.option("cpu_data_cache")!!.copy(constraint = "Needs a CPU with a data cache")
		val b = schema.option("cpu_no_unimplemented")!!.copy(constraint = "Needs a CPU with a data cache")
		val lone = schema.option("mmu_model")!!.copy(constraint = "Needs a 68030 or better")

		// Two options share a reason: it is said once, no names.
		assertEquals(
			listOf("Needs a CPU with a data cache"),
			SettingsGroups.constraintNotes(listOf(a, b)) { false }
		)
		// A reason one option has names it, unless it starts with the option's name.
		assertEquals(
			listOf(
				"Data cache emulation: Needs a CPU with a data cache",
				"MMU: Needs a 68030 or better"
			),
			SettingsGroups.constraintNotes(listOf(a, lone)) { false }
		)
		val named = lone.copy(constraint = "MMU needs a 68030 or better")
		assertEquals(
			listOf("MMU needs a 68030 or better"),
			SettingsGroups.constraintNotes(listOf(named)) { false }
		)
		// Available options say nothing.
		assertTrue(SettingsGroups.constraintNotes(listOf(a, lone)) { true }.isEmpty())
		// Options without a constraint say nothing.
		val quiet = schema.option("ntsc")!!.copy(constraint = null)
		assertTrue(SettingsGroups.constraintNotes(listOf(quiet)) { false }.isEmpty())
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
