package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings

/**
 * When the emulator offers a setting, mirroring the shared settings layer's
 * availability checks (the *_available functions in src/osdep/settings/).
 * A control these call unavailable shows the schema's constraint text for
 * its option as the reason (SettingsSchema.constraint), so the launcher keeps
 * no reason texts of its own.
 */
object SettingsControlAvailability {

	/**
	 * cpu_24bit_addressing: up to a 68030, and not with the JIT
	 * (address_space_24_available in settings_cpu.cpp). The launcher sets no
	 * accelerator board, whose 32-bit boards also rule it out.
	 */
	fun isAddress24BitEditable(settings: EmulatorSettings): Boolean =
		settings.cpuModel <= 68030 && jitCacheForCpuChecks(settings) == 0

	/** cpu_compatible: always on in cycle-exact modes, off for a 68040/060 with the JIT. */
	fun isCpuCompatibleEditable(settings: EmulatorSettings): Boolean =
		!settings.isCycleExact && !(jitCacheForCpuChecks(settings) > 0 && settings.cpuModel >= 68040)

	/** cachesize: the JIT needs a 68020 or later with 32-bit addressing (jit_possible). */
	fun isJitAllowed(settings: EmulatorSettings): Boolean =
		settings.cpuModel >= 68020 && !settings.address24Bit

	/** compfpu: only with the JIT on and an FPU (compfpu_available). */
	fun isJitFpuAvailable(settings: EmulatorSettings): Boolean =
		isJitAllowed(settings) && settings.jitCacheSize > 0 && settings.fpuModel > 0

	/**
	 * The FPUs the CPU can have (fpu_model_choice_available): none; a 68881
	 * or 68882 below a 68040, which a 68000/010 keeps only outside More
	 * compatible and cycle-exact modes; the internal FPU of a 68040/060.
	 */
	fun fpuOptionValues(settings: EmulatorSettings): List<Int> {
		val cpu = settings.cpuModel
		val external = cpu < 68040 &&
			(cpu >= 68020 || (!settings.cpuCompatible && !settings.isCycleExact))
		return when {
			cpu >= 68040 -> listOf(0, cpu)
			external -> listOf(0, 68881, 68882)
			else -> listOf(0)
		}
	}

	/** z3mem_size: Zorro III needs 32-bit addressing (z3_possible in settings_memory.cpp). */
	fun isZ3MemoryAvailable(settings: EmulatorSettings): Boolean =
		!settings.address24Bit

	/**
	 * The cycle-exact modes offered: DMA/memory only from a 68020, where it
	 * differs from full (cycle_exact_choice_available in settings_chipset.cpp).
	 * A stored value stays listed, so it is shown as it is.
	 */
	fun cycleExactChoiceValues(settings: EmulatorSettings): List<String> =
		EmulatorSettings.cycleExactOptions.map { it.first }.filter { value ->
			value != "memory" || settings.cpuModel >= 68020 || value == settings.cycleExact
		}

	/**
	 * The sound modes offered: not Disabled in cycle-exact modes, which need
	 * Paula emulation (sound_output_choice_available in settings_sound.cpp).
	 */
	fun soundOutputChoiceValues(settings: EmulatorSettings): List<String> =
		EmulatorSettings.soundOutputOptions.map { it.first }.filter { value ->
			value != "none" || !settings.isCycleExact || value == settings.soundOutput
		}

	/**
	 * Sound emulation is on: channels, frequency, buffer, the volumes other
	 * than the master volume and the drive sounds need it (sound_option_available).
	 */
	fun isSoundEmulationOn(settings: EmulatorSettings): Boolean =
		settings.soundOutput != "none"

	/** A drive's click volumes need sound emulation and the drive's sound on (floppy_volume_available). */
	fun isFloppySoundVolumeAvailable(settings: EmulatorSettings, drive: Int): Boolean =
		isSoundEmulationOn(settings) && floppySound(settings, drive) != 0

	/** The JIT cache the core's CPU checks see: none below a 68020 (jit_cache_for_cpu_checks). */
	private fun jitCacheForCpuChecks(settings: EmulatorSettings): Int =
		if (settings.cpuModel < 68020) 0 else settings.jitCacheSize

	private fun floppySound(settings: EmulatorSettings, drive: Int): Int = when (drive) {
		0 -> settings.floppy0Sound
		1 -> settings.floppy1Sound
		2 -> settings.floppy2Sound
		3 -> settings.floppy3Sound
		else -> 0
	}
}
