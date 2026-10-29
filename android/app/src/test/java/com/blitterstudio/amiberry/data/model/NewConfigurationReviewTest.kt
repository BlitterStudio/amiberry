package com.blitterstudio.amiberry.data.model

import com.blitterstudio.amiberry.data.ConfigFiles
import com.blitterstudio.amiberry.data.ConfigMachine
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * Review changes for a new configuration compares it with its model's preset; a row
 * of a layer option reverts to the reviewed value; Save offers the model's name.
 */
class NewConfigurationReviewTest {

	init {
		requireNotNull(SettingsSchema.current) { "the bundled settings schema did not load" }
	}

	private val a1200 = EmulatorSettings.fromModel(AmigaModel.A1200)

	@Test
	fun `a fresh model preset with the user's ROM, disk and controls has nothing to review`() {
		val settings = a1200.copy(
			romFile = "/roms/kick31.rom",
			floppy0 = "/disks/Lotus.adf",
			joyport1 = "joy0",
			onScreenJoystick = false,
			shader = "tv",
			hardDrives = listOf(HardDrive(path = "/hd/Work.hdf"))
		)

		assertEquals(settings, settings.presetMachine())
		assertTrue(SettingsChangeSummary.diff(settings.presetMachine(), settings).isEmpty())
	}

	@Test
	fun `machine edits are reviewed against the preset, by option`() {
		val settings = a1200.copy(ntsc = true).withOption("amiberry.vkbd_transparency", "40")

		val changes = SettingsChangeSummary.diff(settings.presetMachine(), settings)

		assertEquals(setOf("ntsc", "amiberry.vkbd_transparency"), changes.mapNotNull { it.optionId }.toSet())
	}

	@Test
	fun `the compatibility level counts as a change from the preset's default level`() {
		val levels = AmigaModel.A1200.compatibilityMax
		if (levels == 0) return
		val other = if (AmigaModel.A1200.defaultLevel == 0) 1 else 0
		val settings = a1200.withCompatibilityLevel(other)

		val changes = SettingsChangeSummary.diff(settings.presetMachine(), settings)

		assertTrue(changes.isNotEmpty())
	}

	@Test
	fun `reverting a row sets the option back to the reviewed value`() {
		val edited = a1200.copy(ntsc = true).withOption("amiberry.vkbd_transparency", "40")
		val baseline = edited.presetMachine()

		var reverted = edited
		for (change in SettingsChangeSummary.diff(baseline, edited)) {
			val id = requireNotNull(change.optionId)
			val value = baseline.optionValue(id) ?: SettingsSchema.current?.option(id)?.defaultOnAndroid
			reverted = reverted.withOption(id, requireNotNull(value))
		}

		assertTrue(SettingsChangeSummary.diff(baseline, reverted).isEmpty())
		assertNull(reverted.options["amiberry.vkbd_transparency"])
	}

	@Test
	fun `launcher rows carry no option to revert`() {
		val settings = a1200.copy(gfxWidth = 800, gfxHeight = 600)

		val changes = SettingsChangeSummary.diff(settings.presetMachine(), settings)

		assertEquals(1, changes.size)
		assertNull(changes.single().optionId)
	}

	@Test
	fun `save offers the model's name, numbered past the saved ones`() {
		assertEquals("Amiga 1200", ConfigFiles.suggestedName("Amiga 1200", listOf("Amiga 500")))
		assertEquals("Amiga 1200 3", ConfigFiles.suggestedName("Amiga 1200", listOf("amiga 1200", "Amiga 1200 2")))
		val name = ConfigFiles.suggestedName("American Laser Games / Picmatic", emptyList())
		assertEquals("American Laser Games - Picmatic", name)
		assertNull(ConfigFiles.nameProblem(name, emptyList()))
	}

	@Test
	fun `a configuration card names its model, CPU, RAM and first medium`() {
		val settings = EmulatorSettings.fromModel(AmigaModel.A500).copy(
			chipRam = 1,
			slowRam = 2,
			fastRam = 0,
			z3Ram = 0,
			cpuModel = 68000,
			floppy1 = "/disks/Second.adf",
			cdImage = "/cd/Game.iso"
		)

		val machine = ConfigMachine.of(settings)

		assertEquals(AmigaModel.A500.displayName, machine.model)
		assertEquals(68000, machine.cpu)
		assertEquals(1024, machine.memoryKb)
		assertEquals("DF1: Second.adf", machine.firstMedium)
		assertNull(ConfigMachine.of(settings.copy(floppy1 = "", cdImage = "")).firstMedium)
	}
}
