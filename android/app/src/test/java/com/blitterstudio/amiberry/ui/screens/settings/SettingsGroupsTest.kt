package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
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

	/** Every option each content page shows: its own rows, then its sections' options. */
	private fun shownByPage(): Map<SettingsPage, List<String>> =
		SettingsPages.all.associateWith { page ->
			SettingsGroups.laidOut[page].orEmpty().toList() +
				SettingsGroups.sectionsOf(page).flatMap { SettingsGroups.options(schema, it).map { o -> o.id } }
		}

	@Test
	fun `every listed option shows exactly once across the pages`() {
		val listed = schema.options.values.filter { !it.presetOnly && !it.hiddenOnAndroid }.map { it.id }
		val shown = shownByPage().values.flatten() + SettingsGroups.managed
		assertEquals(shown.size, shown.toSet().size)
		assertEquals(listed.toSet(), shown.toSet())
	}
	@Test
	fun `each option lands on exactly one page, the page that owns it`() {
		for (option in schema.options.values) {
			if (option.presetOnly || option.hiddenOnAndroid || option.id in SettingsGroups.managed) continue
			assertEquals(option.id, SettingsGroups.pageFor(option.id), pageOwning(option.id))
		}
	}

	/** The page the option's section or own row shows it on, computed independently of pageFor. */
	private fun pageOwning(optionId: String): SettingsPage {
		val own = shownByPage().entries.singleOrNull { (_, ids) -> optionId in ids }
		return own?.key ?: error("$optionId shows on no page")
	}


	@Test
	fun `all settings lists every option a row can edit, except the managed and coupled ones`() {
		val all = SettingsGroups.groupsOf(SettingsPage.AllSettings, schema)
			.flatMap { SettingsGroups.allSettingsRows(schema, it) { _ -> true }.map { o -> o.id } }
		val listed = schema.options.values
			.filter { !it.presetOnly && !it.hiddenOnAndroid && SettingsGroups.rowKind(it) != null }
			.map { it.id }
			// The launcher writes these itself (or resets them): no control works.
			.filter { it !in SettingsGroups.managed }
			// A coupled pair shows as its first member's row alone.
			.filter { id -> SettingsGroups.coupledPairOf(id)?.let { it.first() == id } ?: true }
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
	fun `a coupled pair shows as one row that matches when either half does`() {
		assertEquals(
			listOf("amiberry.scaling_method", "gfx_autoresolution"),
			SettingsGroups.coupledPairOf("gfx_autoresolution")
		)

		// Both halves' rows collapse into the first's, shown once.
		assertEquals(
			listOf("amiberry.scaling_method"),
			SettingsGroups.allSettingsRows(schema, "display") { _ -> true }
				.map { it.id }.filter { SettingsGroups.coupledPairOf(it) != null }
		)
		// A search that matches only the second half still finds the pair's row.
		assertEquals(
			listOf("amiberry.scaling_method"),
			SettingsGroups.allSettingsRows(schema, "display") { it == "gfx_autoresolution" }.map { it.id }
		)
		// A search that matches neither half shows neither.
		assertTrue(SettingsGroups.allSettingsRows(schema, "display") { false }.isEmpty())
	}

	@Test
	fun `the constraints restore a coupled half edited alone, so pages pair the update`() {
		val integer = EmulatorSettings().copy(scalingMethod = 2, gfxAutoresolution = 1)

		// Turning one half off alone is restored from the other.
		val halfOff = EmulatorSettingsConstraints.apply(
			integer.withOption("amiberry.scaling_method", "0"),
			hasTouchScreen = true
		)
		assertEquals(2, halfOff.scalingMethod)
		assertEquals(1, halfOff.gfxAutoresolution)

		// Both together turn it off (the paired row's update).
		val bothOff = EmulatorSettingsConstraints.apply(
			integer.copy(scalingMethod = -1, gfxAutoresolution = 0),
			hasTouchScreen = true
		)
		assertEquals(-1, bothOff.scalingMethod)
		assertEquals(0, bothOff.gfxAutoresolution)
	}

	@Test
	fun `each group is placed once, and a group no section takes is named by one`() {
		assertEquals(SettingsPages.all.filter { it != SettingsPage.AllSettings }.toSet(), SettingsGroups.byPage.keys)
		val groups = SettingsGroups.byPage.values.flatten().mapNotNull { it.group }
		assertEquals(groups.size, groups.toSet().size)
		val ids = SettingsGroups.byPage.values.flatten().flatMap { it.ids }
		assertEquals(ids.size, ids.toSet().size)
		// Every group the layer has is either a section of its own or fully
		// named by id sections (Apple's pages, without the ones the launcher's
		// layer does not have: no x86, diskswapper, drawbridge or priority).
		val unplaced = schema.options.values
			.filter { !it.presetOnly && !it.hiddenOnAndroid && it.group !in groups && SettingsGroups.rowKind(it) != null }
			.filter { it.id !in SettingsGroups.placed && it.id !in SettingsGroups.managed }
		assertTrue(unplaced.joinToString(",") { it.id }, unplaced.isEmpty())
	}

	@Test
	fun `every listed option lands in exactly one section, the expert rows in collapsed ones`() {
		val landed = HashMap<String, Pair<SettingsPage, SettingsGroups.Placement?>>()
		for (page in SettingsPages.all.filter { it != SettingsPage.AllSettings }) {
			SettingsGroups.laidOut[page].orEmpty().forEach { id ->
				assertNull("$id shows both as the page's own row and in a section", landed[id])
				landed[id] = page to null
			}
			for (placement in SettingsGroups.sectionsOf(page)) {
				for (option in SettingsGroups.options(schema, placement)) {
					assertNull("${option.id} shows in two sections", landed[option.id])
					landed[option.id] = page to placement
				}
			}
		}
		// Every listed option a row can edit lands somewhere (the managed and
		// coupled ones aside), and All Settings lists it again whatever section
		// holds it — a search there reaches the rows a page keeps collapsed.
		val listed = schema.options.values
			.filter { !it.presetOnly && !it.hiddenOnAndroid && it.id !in SettingsGroups.managed && SettingsGroups.rowKind(it) != null }
			.filter { option -> SettingsGroups.coupledPairOf(option.id)?.let { it.first() == option.id } ?: true }
			.map { it.id }
		// The pages' own rows without an editable control (the Kickstart and
		// drive paths, the hotkey options) show nowhere else by design.
		assertEquals(listed.toSet(), landed.keys intersect listed)
		val allSettings = SettingsGroups.groupsOf(SettingsPage.AllSettings, schema)
			.flatMap { SettingsGroups.allSettingsRows(schema, it) { _ -> true }.map { o -> o.id } }
		(landed.keys intersect listed).forEach { id -> assertTrue(id, id in allSettings) }

		fun collapsed(id: String) = landed[id]?.second?.collapsed ?: false
		listOf(
			// CPU · Advanced
			"cpu_throttle", "cpu_multiplier", "amiberry.cpu_idle", "cpu_threaded", "cachesize", "compfpu",
			"comp_constjump", "comp_flushmode", "comp_nf", "comp_catchfault", "comp_trustbyte",
			"cpu_no_unimplemented", "cpu_data_cache", "mmu_model", "fpu_strict", "fpu_softfloat", "fpu_no_unimplemented",
			// Chipset · Advanced and the compatibility internals
			"waiting_blits", "amiberry.multithreaded_denise", "keyboard_type", "keyboard_nkro", "display_optimizations", "hvcsync",
			"rtc", "agnusrev", "denisemodel",
			// On-board interfaces, Memory · Advanced, ROMs · Advanced, Display · Advanced, Genlock
			"pcmcia", "scsi_a3000", "z3mapping", "megachipmem_size", "boot_rom_uae", "kickshifter",
			"gfx_monitorblankdelay", "genlock",
			// Ports & Misc's collapsed Compatibility and Debugging
			"uae_hide_autoconfig", "log_illegal_mem"
		).forEach { id -> assertEquals(id, true, collapsed(id)) }
		listOf(
			"cpu_model", "chipset", "chipset_compatible", "cycle_exact", "chipmem_size", "bsdsocket_emu",
			"show_leds", "dongle", "cpu_reset_pause", "floppy_write_protect", "amiberry.use_retroarch_menu"
		).forEach { id -> assertEquals(id, false, collapsed(id)) }
	}

	@Test
	fun `the pages' designed sections mirror the Apple app's`() {
		assertEquals(
			listOf(
				"cpu", "chipset", "memory", "rom", "expansions",
				"pcmcia,ide,scsi_a3000,scsi_a4000t",
				"cpu_throttle,cpu_multiplier,amiberry.cpu_idle,cpu_threaded,cachesize,compfpu,comp_constjump,comp_flushmode,comp_nf,comp_catchfault,comp_trustbyte,cpu_no_unimplemented,cpu_data_cache,mmu_model,fpu_strict,fpu_softfloat,fpu_no_unimplemented",
				"waiting_blits,amiberry.multithreaded_denise,keyboard_type,keyboard_nkro,display_optimizations,hvcsync",
				"adv_chipset", "z3mapping,a3000mem_size,mbresmem_size,megachipmem_size",
				"boot_rom_uae,uaeboard,kickshifter", "genlock"
			),
			SettingsGroups.byPage.getValue(SettingsPage.Hardware).map { it.key }
		)
		assertEquals(
			listOf("display", "filter", "sound"),
			SettingsGroups.byPage.getValue(SettingsPage.DisplaySound).mapNotNull { it.group }
		)
		assertEquals(listOf("input", "onscreen"), SettingsGroups.byPage.getValue(SettingsPage.Input).mapNotNull { it.group })
		assertEquals(listOf("floppy", "cd", "filesystem"), SettingsGroups.byPage.getValue(SettingsPage.Storage).mapNotNull { it.group })
		// The misc group split by purpose, the io group's dongle its own section.
		assertEquals(
			listOf(
				"show_leds,show_leds_rtg,power_led_dim",
				"io",
				"cpu_reset_pause,warpboot",
				"clipboard_sharing,synchronize_clock",
				"amiberry.right_control_is_right_win",
				"amiberry.use_retroarch_quit,amiberry.use_retroarch_menu,amiberry.use_retroarch_reset",
				"floppy_write_protect,harddrive_write_protect",
				"uae_hide_autoconfig,scsidevice_disable",
				"log_illegal_mem,debug_mem,cpu_halt_auto_reset"
			),
			SettingsGroups.byPage.getValue(SettingsPage.PortsMisc).map { it.key }
		)
	}

	@Test
	fun `only the expert sections start collapsed`() {
		val collapsed = SettingsGroups.byPage.values.flatten().filter { it.collapsed }.map { it.key }
		assertEquals(
			listOf(
				"pcmcia,ide,scsi_a3000,scsi_a4000t",
				"cpu_throttle,cpu_multiplier,amiberry.cpu_idle,cpu_threaded,cachesize,compfpu,comp_constjump,comp_flushmode,comp_nf,comp_catchfault,comp_trustbyte,cpu_no_unimplemented,cpu_data_cache,mmu_model,fpu_strict,fpu_softfloat,fpu_no_unimplemented",
				"waiting_blits,amiberry.multithreaded_denise,keyboard_type,keyboard_nkro,display_optimizations,hvcsync",
				"adv_chipset", "z3mapping,a3000mem_size,mbresmem_size,megachipmem_size",
				"boot_rom_uae,uaeboard,kickshifter", "genlock",
				"display",
				"uae_hide_autoconfig,scsidevice_disable",
				"log_illegal_mem,debug_mem,cpu_halt_auto_reset"
			),
			collapsed
		)
	}

	@Test
	fun `options a preset alone sets do not show`() {
		val presetOnly = schema.options.values.filter { it.presetOnly }.map { it.id }
		assertTrue("ppc_model" in presetOnly)
		val shown = shownByPage().values.flatten()
		assertTrue(presetOnly.none { it in shown })
	}

	@Test
	fun `a section follows the layer order and leaves out the page's own rows`() {
		// The designed sections take the group's remainder: the CPU and
		// Chipset sections' rows are the page's own alone (their groups'
		// experts sit in the Advanced sections), the sound section's follow
		// the layer's order.
		val cpu = SettingsGroups.options(schema, SettingsGroups.placementOf(SettingsPage.Hardware, "cpu")!!)
		assertTrue(cpu.isEmpty())
		val chipset = SettingsGroups.options(schema, SettingsGroups.placementOf(SettingsPage.Hardware, "chipset")!!)
		assertTrue(chipset.isEmpty())
		val order = schema.options.keys.toList()
		val sound = SettingsGroups.options(schema, SettingsGroups.placementOf(SettingsPage.DisplaySound, "sound")!!)
		assertEquals(sound.sortedBy { order.indexOf(it.id) }, sound)
		assertFalse("sound_output" in sound.map { it.id })
		assertTrue("sound_stereo_separation" in sound.map { it.id })
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
