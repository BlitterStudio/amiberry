package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class WhdLoadAutoConfigTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `AGA WHDLoad database match updates Kotlin settings to A1200 hardware`() {
		val lha = tempDir.newFile("Atome_v1.1_Skarla_AGA.lha")
		lha.writeText("fixture")
		val detected = requireNotNull(
			WhdLoadAutoConfig.settingsFromDatabaseJson(
				json = """
					{
					  "games": [
					    {
					      "filename": "Atome_v1.1_Skarla_AGA",
					      "hardware": {
					        "chipset": "aga",
					        "cpu": "68020",
					        "fast_ram": 8,
					        "primary_control": "joystick",
					        "port0": "joy",
					        "port1": "joy"
					      }
					    }
					  ]
					}
				""".trimIndent(),
				lhaFile = lha,
				availableRoms = listOf(rom("kick31-a1200.rom", 0x6c9b07d2L)),
				currentSettings = EmulatorSettings(joyport0 = "mouse", joyport1 = "joy1", onScreenJoystick = false)
			)
		)

		assertEquals(AmigaModel.A1200, detected.baseModel)
		assertEquals(68020, detected.cpuModel)
		assertEquals("aga", detected.chipset)
		assertEquals(8, detected.fastRam)
		assertEquals("/roms/kick31-a1200.rom", detected.romFile)
		assertEquals("mouse", detected.joyport0)
		assertEquals("joy1", detected.joyport1)
		assertFalse(detected.onScreenJoystick)
	}

	@Test
	fun `non AGA WHDLoad database match uses A600 hardware when an A600 ROM is available`() {
		val lha = tempDir.newFile("CannonFodder_v1.3_0860.lha")
		lha.writeText("fixture")
		val detected = requireNotNull(
			WhdLoadAutoConfig.settingsFromDatabaseJson(
				json = """
					{
					  "games": [
					    {
					      "filename": "CannonFodder_v1.3_0860",
					      "hardware": {
					        "primary_control": "mouse",
					        "port0": "mouse",
					        "port1": "joy",
					        "screen_autoheight": true,
					        "cpu_exact": true
					      }
					    }
					  ]
					}
				""".trimIndent(),
				lhaFile = lha,
				availableRoms = listOf(rom("kick40-a600.rom", 0x43b0df7bL)),
				currentSettings = EmulatorSettings(onScreenKeyboard = false)
			)
		)

		assertEquals(AmigaModel.A600, detected.baseModel)
		assertEquals(68000, detected.cpuModel)
		assertEquals("ecs", detected.chipset)
		assertEquals(8, detected.fastRam)
		assertTrue(detected.autoCrop)
		assertTrue(detected.cycleExact)
		assertFalse(detected.onScreenKeyboard)
	}

	@Test
	fun `WHDLoad database match falls back to A1200 hardware when only A1200 ROM is available`() {
		val lha = tempDir.newFile("UnknownHardware_v1.0.lha")
		lha.writeText("fixture")
		val detected = requireNotNull(
			WhdLoadAutoConfig.settingsFromDatabaseJson(
				json = """
					{
					  "games": [
					    {
					      "filename": "UnknownHardware_v1.0",
					      "hardware": {
					        "screen_autoheight": false,
					        "screen_height": 400,
					        "screen_width": 720
					      }
					    }
					  ]
					}
				""".trimIndent(),
				lhaFile = lha,
				availableRoms = listOf(rom("kick31-a1200.rom", 0x6c9b07d2L)),
				currentSettings = EmulatorSettings()
			)
		)

		assertEquals(AmigaModel.A1200, detected.baseModel)
		assertEquals(68020, detected.cpuModel)
		assertEquals("aga", detected.chipset)
	}

	private fun rom(name: String, crc32: Long, path: String = "/roms/$name") = AmigaFile(
		path = path,
		name = name,
		extension = "rom",
		size = 512 * 1024,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc32
	)
}
