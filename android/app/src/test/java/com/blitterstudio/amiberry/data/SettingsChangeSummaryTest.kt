package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.HardDrive
import com.blitterstudio.amiberry.data.model.SettingText
import com.blitterstudio.amiberry.data.model.SettingsChange
import com.blitterstudio.amiberry.data.model.SettingsChangeSummary
import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SettingsChangeSummaryTest {

	private val schema = requireNotNull(SettingsSchema.current) { "the bundled settings schema did not load" }

	/** The launcher strings by resource name, as res/values/strings.xml defines them. */
	private val strings: Map<String, String> by lazy {
		val xml = File("src/main/res/values/strings.xml").readText()
		Regex("""<string name="([a-z0-9_]+)"[^>]*>([^<]*)</string>""").findAll(xml)
			.associate { it.groupValues[1] to it.groupValues[2].replace("\\'", "'").replace("\\\"", "\"") }
	}

	private val resourceNames: Map<Int, String> by lazy {
		R.string::class.java.fields.associate { it.getInt(null) to it.name }
	}

	/** [text] as the dialog shows it. */
	private fun read(text: SettingText): String = when (text) {
		is SettingText.Plain -> text.text
		is SettingText.Resource -> {
			val format = strings.getValue(resourceNames.getValue(text.id))
			String.format(format, *text.args.map { if (it is SettingText) read(it) else it }.toTypedArray())
		}
	}

	private data class Row(val label: String, val before: String, val after: String)

	private fun rows(changes: List<SettingsChange>) = changes.map { Row(read(it.label), read(it.before), read(it.after)) }

	@Test
	fun `changes are named by the settings layer's labels with value labels`() {
		val before = EmulatorSettings.fromModel(AmigaModel.A500).copy(
			floppy0 = "/old/Workbench.adf",
			onScreenJoystick = true
		)
		val after = before.copy(
			cpuModel = 68030,
			address24Bit = false,
			floppy0 = "/new/Lotus.adf",
			onScreenJoystick = false
		)

		val rows = rows(SettingsChangeSummary.diff(before, after))

		assertEquals(
			listOf(
				Row(schema.label("cpu_model")!!, schema.choiceLabel("cpu_model", "68000")!!, schema.choiceLabel("cpu_model", "68030")!!),
				Row(schema.label("cpu_24bit_addressing")!!, "On", "Off"),
				Row(schema.label("amiberry.onscreen_joystick")!!, "On", "Off"),
				Row(schema.label("floppy0")!!, "Workbench.adf", "Lotus.adf")
			),
			rows
		)
		assertEquals("68020 (A1200)", schema.choiceLabel("cpu_model", "68020"))
	}

	@Test
	fun `identical settings have no changes`() {
		val settings = EmulatorSettings.fromModel(AmigaModel.A1200)

		assertTrue(SettingsChangeSummary.diff(settings, settings).isEmpty())
	}

	@Test
	fun `devices, modes and shaders read as names, not stored tokens`() {
		val before = EmulatorSettings(
			joyport0 = "mouse",
			joyport1 = "joy0",
			cpuSpeed = "real",
			cycleExact = "false",
			shader = "none",
			onScreenKeyboardToggle = null
		)
		val after = before.copy(
			joyport0 = "kbd1",
			joyport1 = "joy1",
			cpuSpeed = "max",
			cycleExact = "memory",
			shader = "presets/custom.glslp",
			onScreenKeyboardToggle = "leftshoulder"
		)

		val rows = rows(SettingsChangeSummary.diff(before, after)).associateBy { it.label }

		assertEquals("As fast as possible", rows.getValue(schema.label("cpu_speed")!!).after)
		assertEquals(
			Row(schema.label("cycle_exact")!!, "Off", "Cycle Exact (DMA/Memory)"),
			rows.getValue(schema.label("cycle_exact")!!)
		)
		assertEquals(strings.getValue("settings_input_device_mouse"), rows.getValue(schema.label("joyport0")!!).before)
		assertEquals(strings.getValue("settings_input_device_joystick_1"), rows.getValue(schema.label("joyport1")!!).after)
		val shader = rows.values.single { it.after == "custom.glslp" }
		assertEquals(strings.getValue("settings_shader_none"), shader.before)
		val toggle = rows.getValue(schema.label("amiberry.vkbd_toggle")!!)
		assertEquals(strings.getValue("settings_osk_toggle_default"), toggle.before)
		assertEquals(strings.getValue("settings_osk_toggle_leftshoulder"), toggle.after)
	}

	@Test
	fun `every setting is reported, by a readable label and readable values`() {
		val before = EmulatorSettings()
		val after = before.copy(
			baseModel = AmigaModel.A1200,
			cpuModel = 68020, cpuCompatible = false, address24Bit = false, cpuSpeed = "max",
			fpuModel = 68881, jitCacheSize = 8192, jitFpu = true,
			chipset = "aga", immediateBlits = true, collisionLevel = "sprites", cycleExact = "true", ntsc = true,
			chipRam = 4, slowRam = 0, fastRam = 8, z3Ram = 64,
			romFile = "/roms/kick31.rom", romExtFile = "/roms/ext.rom",
			floppy0 = "/disks/a.adf", floppy0Type = 1, floppy1 = "/disks/b.adf", floppy1Type = 2,
			floppy2 = "/disks/c.adf", floppy2Type = 7, floppy3 = "/disks/d.adf", floppy3Type = 3,
			cdImage = "/cds/game.iso",
			hardDrives = listOf(HardDrive(path = "/hd/work.hdf", readOnly = true, bootPriority = 1)),
			soundOutput = "normal", soundFreq = 32000, soundBuffer = 4096, soundChannels = "mono",
			soundVolume = 10, soundVolumePaula = 10, soundVolumeCd = 10, soundVolumeAhi = 10, soundVolumeMidi = 10,
			floppy0Sound = 1, floppy1Sound = 1, floppy2Sound = 1, floppy3Sound = 1,
			floppy0SoundVolumeDisk = 50, floppy1SoundVolumeDisk = 50, floppy2SoundVolumeDisk = 50, floppy3SoundVolumeDisk = 50,
			floppy0SoundVolumeEmpty = 50, floppy1SoundVolumeEmpty = 50, floppy2SoundVolumeEmpty = 50, floppy3SoundVolumeEmpty = 50,
			gfxWidth = 800, gfxHeight = 600, correctAspect = false, autoCrop = true,
			scalingMethod = 2, gfxAutoresolution = 1, shader = "tv",
			joyport0 = "joy0", joyport1 = "mouse", onScreenJoystick = false, onScreenKeyboardButton = false,
			onScreenKeyboardNumpad = true, onScreenKeyboardToggle = "", joyport0MouseMap = true, joyport1MouseMap = true,
			analogMouseSpeed = 50
		)

		val rows = rows(SettingsChangeSummary.diff(before, after))
		val labels = rows.map { it.label }

		// One row per field.
		assertEquals(labels.toString(), labels.size, labels.toSet().size)
		val storedTokens = setOf("true", "false", "max", "real", "memory", "joy0", "mouse", "kbd1", "aga", "sprites", "tv", "")
		for (row in rows) {
			assertFalse("raw option id as label: ${row.label}", row.label.contains('_') || row.label.startsWith("amiberry."))
			for (value in listOf(row.before, row.after)) {
				assertFalse("${row.label}: raw value '$value'", value in storedTokens || value.startsWith("/"))
			}
		}
		assertTrue(labels.toString(), rows.size >= 60)
		val masterVolume = rows.single { it.label == schema.label("sound_volume") }
		assertEquals("100%", masterVolume.before)
		assertEquals("90%", masterVolume.after)
		assertEquals("8192 KB", rows.single { it.label == schema.label("cachesize") }.after)
		assertEquals("work.hdf", rows.single { it.label == read(SettingText.Resource(R.string.settings_storage_hard_drive_label, listOf(0))) }.after)
	}
}
