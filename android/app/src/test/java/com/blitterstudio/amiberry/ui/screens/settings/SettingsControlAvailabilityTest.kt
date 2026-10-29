package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsControlAvailabilityTest {

	@Test
	fun `a 68030 keeps 24-bit addressing editable without the JIT`() {
		assertTrue(SettingsControlAvailability.isAvailable(EmulatorSettings(cpuModel = 68030), "cpu_24bit_addressing"))
		assertFalse(
			SettingsControlAvailability.isAvailable(
				EmulatorSettings(cpuModel = 68030, address24Bit = false, jitCacheSize = 2048),
				"cpu_24bit_addressing"
			)
		)
	}

	@Test
	fun `the DMA-only cycle-exact mode needs a 68020`() {
		assertEquals(
			listOf("false", "true"),
			SettingsControlAvailability.choiceValues(EmulatorSettings(cpuModel = 68000), "cycle_exact")
		)
		assertEquals(
			listOf("false", "memory", "true"),
			SettingsControlAvailability.choiceValues(EmulatorSettings(cpuModel = 68020), "cycle_exact")
		)
	}

	@Test
	fun `sound and drive sound tables answer for their own options`() {
		val noSound = EmulatorSettings(soundOutput = "none", floppy2Sound = 1)
		assertFalse(SettingsControlAvailability.isAvailable(noSound, "sound_frequency"))
		assertFalse(SettingsControlAvailability.isAvailable(noSound, "floppy2soundvolume_disk"))
		val driveSound = EmulatorSettings(soundOutput = "normal", floppy2Sound = 1)
		assertTrue(SettingsControlAvailability.isAvailable(driveSound, "floppy2soundvolume_empty"))
		assertFalse(SettingsControlAvailability.isAvailable(driveSound, "floppy1soundvolume_empty"))
	}

	@Test
	fun `a state the tables do not list leaves the control available`() {
		val handEdited = EmulatorSettings(cpuModel = 68050)
		assertTrue(SettingsControlAvailability.isAvailable(handEdited, "cpu_compatible"))
		assertNull(SettingsControlAvailability.choiceValues(handEdited, "fpu_model"))
		assertTrue(SettingsControlAvailability.isAvailable(EmulatorSettings(), "cpu_compatible", schema = null))
	}

	private fun available(settings: EmulatorSettings, id: String) = SettingsControlAvailability.isAvailable(settings, id)

	@Test
	fun `the MMU rules out the JIT and the JIT options need the JIT`() {
		val jit = EmulatorSettings(cpuModel = 68030, address24Bit = false, jitCacheSize = 8192)
		assertTrue(available(jit, "comp_nf"))
		assertFalse(available(jit, "mmu_model"))
		assertFalse(available(jit.copy(jitCacheSize = 0), "comp_trustbyte"))

		val mmu = jit.copy(jitCacheSize = 0).withOption("mmu_model", "68030")
		assertFalse(available(mmu, "cachesize"))
		assertEquals(listOf("0", "68030", "68ec030"), SettingsControlAvailability.choiceValues(mmu, "mmu_model"))
		assertFalse(available(EmulatorSettings(cpuModel = 68020), "mmu_model"))
	}

	@Test
	fun `FPU options need an FPU`() {
		assertFalse(available(EmulatorSettings(cpuModel = 68020, fpuModel = 0), "fpu_softfloat"))
		assertTrue(available(EmulatorSettings(cpuModel = 68020, fpuModel = 68882), "fpu_strict"))
	}

	@Test
	fun `speed options follow the speed and the compatibility modes`() {
		val real = EmulatorSettings(cpuSpeed = "real", cpuCompatible = true)
		assertFalse(available(real, "amiberry.cpu_idle"))
		assertTrue(available(real.copy(cpuSpeed = "max"), "amiberry.cpu_idle"))
		assertTrue(available(real, "cpu_multiplier"))
		assertFalse(available(real.copy(cpuSpeed = "max"), "cpu_multiplier"))
		assertFalse(available(real.copy(cycleExact = "true"), "cpu_throttle"))
	}

	@Test
	fun `a chipset extra sets the advanced chipset`() {
		val generic = EmulatorSettings(cpuModel = 68020, chipRam = 1)
		assertFalse(available(generic, "ciaatod"))
		assertEquals(listOf("RP5C01A"), SettingsControlAvailability.choiceValues(generic, "rtc"))

		val custom = generic.withOption("chipset_compatible", "-")
		assertTrue(available(custom, "ciaatod"))
		assertTrue(available(custom, "1mchipjumper"))
		assertFalse(available(custom.copy(chipRam = 4), "1mchipjumper"))
		assertEquals(4, SettingsControlAvailability.choiceValues(custom, "rtc")?.size)
	}

	@Test
	fun `display tables answer for centering, crop and the native mode`() {
		val settings = EmulatorSettings()
		assertTrue(available(settings, "gfx_center_horizontal"))
		assertFalse(available(settings.withOption("gfx_overscanmode", "ultra"), "gfx_center_vertical"))
		assertFalse(available(settings.copy(autoCrop = true), "gfx_center_horizontal"))

		assertFalse(available(settings, "amiberry.gfx_manual_crop_width"))
		val manual = settings.withOption("amiberry.gfx_manual_crop", "true")
		assertTrue(available(manual, "amiberry.gfx_manual_crop_width"))
		assertFalse(available(manual.copy(autoCrop = true), "amiberry.gfx_manual_crop"))

		assertTrue(available(settings, "gfx_linemode"))
		assertFalse(available(settings.copy(gfxAutoresolution = 1), "gfx_resolution"))
	}

	@Test
	fun `sound, genlock, memory and input options follow their switches`() {
		val stereo = EmulatorSettings(soundOutput = "exact", soundChannels = "stereo")
		assertTrue(available(stereo, "sound_stereo_separation"))
		assertFalse(available(stereo.copy(soundChannels = "mono"), "sound_stereo_mixing_delay"))
		assertFalse(available(stereo.copy(soundOutput = "none"), "sound_interpol"))

		assertFalse(available(stereo, "genlock_mix"))
		assertTrue(available(stereo.withOption("genlock", "true"), "genlock_mix"))

		assertFalse(available(EmulatorSettings(address24Bit = true), "z3mem2_size"))
		assertTrue(available(EmulatorSettings(cpuModel = 68020, address24Bit = false), "z3mapping"))

		assertFalse(available(stereo, "tablet_library"))
		assertTrue(available(stereo.withOption("absolute_mouse", "tablet"), "tablet_library"))

		val noKeyboard = EmulatorSettings(onScreenKeyboardButton = false, onScreenKeyboardToggle = "")
		assertFalse(available(noKeyboard, "amiberry.vkbd_transparency"))
		assertTrue(available(EmulatorSettings(), "amiberry.vkbd_language"))
	}
}
