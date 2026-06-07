package com.blitterstudio.amiberry.ui.screens.settings

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings

object SettingsControlAvailability {

	fun isAddress24BitEditable(cpuModel: Int): Boolean =
		cpuModel > 68010

	@StringRes
	fun address24BitReasonRes(cpuModel: Int): Int? =
		if (isAddress24BitEditable(cpuModel)) null else R.string.settings_cpu_24bit_forced_68000

	fun isFastestSpeedEditable(settings: EmulatorSettings): Boolean =
		!settings.cycleExact

	@StringRes
	fun fastestSpeedReasonRes(settings: EmulatorSettings): Int? =
		if (settings.cycleExact) R.string.settings_cpu_fastest_disabled_cycle_exact else null

	fun isCpuCompatibleEditable(settings: EmulatorSettings): Boolean =
		!(settings.cpuModel >= 68040 && settings.jitCacheSize > 0)

	@StringRes
	fun cpuCompatibleReasonRes(settings: EmulatorSettings): Int? =
		if (isCpuCompatibleEditable(settings)) null else R.string.settings_cpu_compatible_disabled_jit

	fun isImmediateBlitsEditable(settings: EmulatorSettings): Boolean =
		!settings.cycleExact

	@StringRes
	fun immediateBlitsReasonRes(settings: EmulatorSettings): Int? =
		if (isImmediateBlitsEditable(settings)) null else R.string.settings_chipset_immediate_blitter_disabled_cycle_exact

	fun isJitAllowed(settings: EmulatorSettings): Boolean =
		settings.cpuModel >= 68020 && !settings.cycleExact && !settings.address24Bit

	@StringRes
	fun jitDisabledReasonRes(settings: EmulatorSettings): Int? = when {
		settings.cpuModel < 68020 -> R.string.settings_cpu_jit_disabled_cpu
		settings.cycleExact -> R.string.settings_cpu_jit_disabled_cycle_exact
		settings.address24Bit -> R.string.settings_cpu_jit_disabled_24bit
		else -> null
	}

	fun fpuOptionValues(cpuModel: Int): List<Int> = when {
		cpuModel < 68020 -> listOf(0)
		cpuModel < 68040 -> listOf(0, 68881, 68882)
		else -> listOf(0, cpuModel)
	}

	fun isZ3MemoryVisible(settings: EmulatorSettings): Boolean =
		!settings.address24Bit

	@StringRes
	fun z3HiddenReasonRes(settings: EmulatorSettings): Int? =
		if (settings.address24Bit) R.string.settings_memory_z3_disabled_24bit else null
}
