package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class ConfigParserTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private fun writeConfig(content: String): File {
		val file = tempDir.newFile("test.uae")
		file.writeText(content)
		return file
	}

	// --- Basic parsing ---

	@Test
	fun `parse empty file returns defaults`() {
		val file = writeConfig("")
		val result = ConfigParser.parse(file)

		assertEquals(68000, result.settings.cpuModel)
		assertEquals("ocs", result.settings.chipset)
		assertEquals(1, result.settings.chipRam)
		assertTrue(result.unknownLines.isEmpty())
		assertEquals("", result.description)
	}

	@Test
	fun `parse nonexistent file returns defaults`() {
		val file = File(tempDir.root, "nonexistent.uae")
		val result = ConfigParser.parse(file)

		assertEquals(68000, result.settings.cpuModel)
		assertTrue(result.unknownLines.isEmpty())
	}

	@Test
	fun `parse skips comment lines`() {
		val file = writeConfig("""
			; This is a comment
			; Another comment
			cpu_model=68020
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(68020, result.settings.cpuModel)
		assertTrue(result.unknownLines.isEmpty())
	}

	@Test
	fun `parse skips blank lines`() {
		val file = writeConfig("""
			cpu_model=68020
			
			chipset=aga
			
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(68020, result.settings.cpuModel)
		assertEquals("aga", result.settings.chipset)
	}

	// --- CPU settings ---

	@Test
	fun `parse CPU settings`() {
		val file = writeConfig("""
			cpu_model=68040
			cpu_compatible=false
			cpu_24bit_addressing=false
			cpu_speed=max
			fpu_model=68040
			cachesize=16384
			compfpu=true
		""".trimIndent())
		val result = ConfigParser.parse(file)
		val s = result.settings

		assertEquals(68040, s.cpuModel)
		assertFalse(s.cpuCompatible)
		assertFalse(s.address24Bit)
		assertEquals("max", s.cpuSpeed)
		assertEquals(68040, s.fpuModel)
		assertEquals(16384, s.jitCacheSize)
		assertTrue(s.jitFpu)
	}

	// --- Chipset settings ---

	@Test
	fun `parse chipset settings`() {
		val file = writeConfig("""
			chipset=aga
			immediate_blits=true
			collision_level=full
			cycle_exact=true
			ntsc=true
		""".trimIndent())
		val result = ConfigParser.parse(file)
		val s = result.settings

		assertEquals("aga", s.chipset)
		assertTrue(s.immediateBlits)
		assertEquals("full", s.collisionLevel)
		assertTrue(s.cycleExact)
		assertTrue(s.ntsc)
	}

	// --- Memory settings ---

	@Test
	fun `parse memory settings`() {
		val file = writeConfig("""
			chipmem_size=4
			bogomem_size=0
			fastmem_size=8
			z3mem_size=64
		""".trimIndent())
		val result = ConfigParser.parse(file)
		val s = result.settings

		assertEquals(4, s.chipRam)
		assertEquals(0, s.slowRam)
		assertEquals(8, s.fastRam)
		assertEquals(64, s.z3Ram)
	}

	// --- ROM and media ---

	@Test
	fun `parse ROM and floppy paths`() {
		val file = writeConfig("""
			kickstart_rom_file=/path/to/kick.rom
			kickstart_ext_rom_file=/path/to/ext.rom
			floppy0=/path/to/game.adf
			floppy0type=0
			floppy1=
			floppy1type=-1
		""".trimIndent())
		val result = ConfigParser.parse(file)
		val s = result.settings

		assertEquals("/path/to/kick.rom", s.romFile)
		assertEquals("/path/to/ext.rom", s.romExtFile)
		assertEquals("/path/to/game.adf", s.floppy0)
		assertEquals(0, s.floppy0Type)
		assertEquals("", s.floppy1)
		assertEquals(-1, s.floppy1Type)
	}

	@Test
	fun `parse CD image`() {
		val file = writeConfig("cdimage0=/path/to/game.iso")
		val result = ConfigParser.parse(file)

		assertEquals("/path/to/game.iso", result.settings.cdImage)
	}

	// --- Sound settings ---

	@Test
	fun `parse sound settings`() {
		val file = writeConfig("""
			sound_output=normal
			sound_frequency=48000
			sound_channels=mono
		""".trimIndent())
		val result = ConfigParser.parse(file)
		val s = result.settings

		assertEquals("normal", s.soundOutput)
		assertEquals(48000, s.soundFreq)
		assertEquals("mono", s.soundChannels)
	}

	// --- Display settings ---

	@Test
	fun `parse display settings`() {
		val file = writeConfig("""
			gfx_width=800
			gfx_height=600
			gfx_correct_aspect=false
			gfx_auto_crop=true
		""".trimIndent())
		val result = ConfigParser.parse(file)
		val s = result.settings

		assertEquals(800, s.gfxWidth)
		assertEquals(600, s.gfxHeight)
		assertFalse(s.correctAspect)
		assertTrue(s.autoCrop)
	}

	// --- Input settings ---

	@Test
	fun `parse input with android joyport1 key`() {
		val file = writeConfig("""
			joyport0=mouse
			joyport1=joy0
			amiberry.android_joyport1=onscreen_joy
			amiberry.onscreen_joystick=true
			amiberry.onscreen_keyboard=false
		""".trimIndent())
		val result = ConfigParser.parse(file)
		val s = result.settings

		assertEquals("mouse", s.joyport0)
		// android_joyport1 takes priority over joyport1
		assertEquals("onscreen_joy", s.joyport1)
		assertTrue(s.onScreenJoystick)
		assertFalse(s.onScreenKeyboard)
	}

	@Test
	fun `parse input infers onscreen_joy when no android key`() {
		val file = writeConfig("""
			joyport0=mouse
			joyport1=joy0
			amiberry.onscreen_joystick=true
		""".trimIndent())
		val result = ConfigParser.parse(file)

		// onscreen_joystick=true + joyport1=joy0 → inferred as onscreen_joy
		assertEquals("onscreen_joy", result.settings.joyport1)
	}

	@Test
	fun `parse input uses joyport1 when onscreen joystick disabled`() {
		val file = writeConfig("""
			joyport1=joy1
			amiberry.onscreen_joystick=false
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals("joy1", result.settings.joyport1)
	}

	// --- Description ---

	@Test
	fun `parse extracts config description`() {
		val file = writeConfig("config_description=My A500 setup")
		val result = ConfigParser.parse(file)

		assertEquals("My A500 setup", result.description)
	}

	// --- Unknown lines preservation ---

	@Test
	fun `unknown keys are preserved in unknownLines`() {
		val file = writeConfig("""
			cpu_model=68000
			some_future_key=value
			another_unknown=42
			chipset=ocs
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(68000, result.settings.cpuModel)
		assertEquals("ocs", result.settings.chipset)
		assertEquals(2, result.unknownLines.size)
		assertTrue(result.unknownLines.any { it.contains("some_future_key") })
		assertTrue(result.unknownLines.any { it.contains("another_unknown") })
	}

	@Test
	fun `lines without equals sign are preserved as unknown`() {
		val file = writeConfig("""
			cpu_model=68000
			this line has no equals
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(1, result.unknownLines.size)
		assertEquals("this line has no equals", result.unknownLines[0])
	}

	// --- toBool extension ---

	@Test
	fun `boolean parsing accepts true, 1, yes`() {
		val file = writeConfig("""
			cpu_compatible=true
			immediate_blits=1
			ntsc=yes
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertTrue(result.settings.cpuCompatible)
		assertTrue(result.settings.immediateBlits)
		assertTrue(result.settings.ntsc)
	}

	@Test
	fun `boolean parsing rejects other values as false`() {
		val file = writeConfig("""
			cpu_compatible=false
			immediate_blits=0
			ntsc=no
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertFalse(result.settings.cpuCompatible)
		assertFalse(result.settings.immediateBlits)
		assertFalse(result.settings.ntsc)
	}

	// --- Model guessing ---

	@Test
	fun `guessModel returns A500 for OCS 68000`() {
		val file = writeConfig("""
			cpu_model=68000
			chipset=ocs
			chipmem_size=1
			bogomem_size=2
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.A500, result.settings.baseModel)
	}

	@Test
	fun `guessModel returns A1200 for AGA 68020`() {
		val file = writeConfig("""
			cpu_model=68020
			chipset=aga
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.A1200, result.settings.baseModel)
	}

	@Test
	fun `guessModel returns A4000 for AGA 68040`() {
		val file = writeConfig("""
			cpu_model=68040
			chipset=aga
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.A4000, result.settings.baseModel)
	}

	@Test
	fun `guessModel returns CD32 for AGA with CD image`() {
		val file = writeConfig("""
			cpu_model=68020
			chipset=aga
			cdimage0=/path/to/game.iso
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.CD32, result.settings.baseModel)
	}

	@Test
	fun `guessModel returns CDTV for OCS with CD image`() {
		val file = writeConfig("""
			cpu_model=68000
			chipset=ocs
			cdimage0=/path/to/disc.iso
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.CDTV, result.settings.baseModel)
	}

	@Test
	fun `guessModel returns A3000 for ECS 68030`() {
		val file = writeConfig("""
			cpu_model=68030
			chipset=ecs
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.A3000, result.settings.baseModel)
	}

	@Test
	fun `guessModel returns A500_PLUS for ECS without hardware path`() {
		val file = writeConfig("""
			cpu_model=68000
			chipset=ecs
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.A500_PLUS, result.settings.baseModel)
	}

	@Test
	fun `guessModel uses config_hardware_path for A600`() {
		val file = writeConfig("""
			cpu_model=68000
			chipset=ecs
			config_hardware_path=A600
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(AmigaModel.A600, result.settings.baseModel)
	}

	// --- Whitespace handling ---

	@Test
	fun `parse handles spaces around equals sign`() {
		val file = writeConfig("""
			cpu_model = 68020
			chipset = aga
		""".trimIndent())
		val result = ConfigParser.parse(file)

		assertEquals(68020, result.settings.cpuModel)
		assertEquals("aga", result.settings.chipset)
	}
}
