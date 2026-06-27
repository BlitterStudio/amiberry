package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.EmulatorSettings

enum class QuickStartLaunchMode {
	WHDLOAD,
	DISK,
	CD,
	RECENT;

	companion object {
		fun fromName(name: String): QuickStartLaunchMode =
			entries.firstOrNull { it.name == name } ?: WHDLOAD
	}
}

data class QuickStartEffectiveSummary(
	val model: String,
	val cpu: String,
	val chipset: String,
	val memory: String,
	val media: String,
	val controls: String
)

object QuickStartUiState {
	fun canStart(
		launchMode: QuickStartLaunchMode,
		hasRoms: Boolean,
		selectedModelAvailable: Boolean,
		hasSelectedWhdloadGame: Boolean
	): Boolean {
		return when (launchMode) {
			QuickStartLaunchMode.WHDLOAD -> hasRoms && hasSelectedWhdloadGame
			QuickStartLaunchMode.DISK,
			QuickStartLaunchMode.CD -> hasRoms && selectedModelAvailable
			QuickStartLaunchMode.RECENT -> false
		}
	}

	fun buildEffectiveSummary(
		settings: EmulatorSettings,
		launchMode: QuickStartLaunchMode,
		selectedModelAvailable: Boolean,
		selectedWhdloadName: String? = null
	): QuickStartEffectiveSummary {
		return QuickStartEffectiveSummary(
			model = if (selectedModelAvailable) settings.baseModel.displayName else "No compatible model",
			cpu = buildCpuSummary(settings),
			chipset = buildChipsetSummary(settings),
			memory = buildMemorySummary(settings),
			media = buildMediaSummary(settings, launchMode, selectedWhdloadName),
			controls = buildControlsSummary(settings)
		)
	}

	private fun buildCpuSummary(settings: EmulatorSettings): String {
		val addressMode = if (settings.address24Bit) "24-bit addressing" else "32-bit addressing"
		val jit = if (settings.jitCacheSize > 0) ", JIT" else ""
		return "${settings.cpuModel}, $addressMode$jit"
	}

	private fun buildChipsetSummary(settings: EmulatorSettings): String =
		EmulatorSettings.chipsetOptions.firstOrNull { it.first == settings.chipset }?.second
			?: settings.chipset.uppercase()

	private fun buildMemorySummary(settings: EmulatorSettings): String {
		val parts = mutableListOf("${chipRamLabel(settings.chipRam)} Chip")
		if (settings.slowRam > 0) {
			parts += "${slowRamLabel(settings.slowRam)} Slow"
		}
		if (settings.fastRam > 0) {
			parts += "${settings.fastRam} MB Fast"
		}
		if (settings.z3Ram > 0) {
			parts += "${settings.z3Ram} MB Z3"
		}
		return parts.joinToString(", ")
	}

	private fun buildMediaSummary(
		settings: EmulatorSettings,
		launchMode: QuickStartLaunchMode,
		selectedWhdloadName: String?
	): String {
		return when (launchMode) {
			QuickStartLaunchMode.WHDLOAD -> {
				val gameName = selectedWhdloadName?.let(::stripKnownArchiveSuffix).orEmpty()
				if (gameName.isBlank()) "No WHDLoad game selected" else "WHDLoad: $gameName"
			}
			QuickStartLaunchMode.DISK -> {
				val disks = listOfNotNull(
					mediaName(settings.floppy0)?.let { "DF0: $it" },
					mediaName(settings.floppy1)?.let { "DF1: $it" }
				)
				disks.joinToString(", ").ifBlank { "No floppy disks selected" }
			}
			QuickStartLaunchMode.CD -> {
				mediaName(settings.cdImage)?.let { "CD: $it" } ?: "No CD image selected"
			}
			QuickStartLaunchMode.RECENT -> "Choose an item from Recent"
		}
	}

	private fun buildControlsSummary(settings: EmulatorSettings): String {
		return when {
			settings.onScreenJoystick && settings.onScreenKeyboard -> "On-screen joystick and keyboard"
			settings.onScreenJoystick -> "On-screen joystick"
			settings.onScreenKeyboard -> "On-screen keyboard"
			else -> "Physical controls only"
		}
	}

	private fun chipRamLabel(value: Int): String =
		EmulatorSettings.chipRamOptions.firstOrNull { it.first == value }?.second
			?: "${value / 2} MB"

	private fun slowRamLabel(value: Int): String =
		EmulatorSettings.slowRamOptions.firstOrNull { it.first == value }?.second
			?: "${value * 256} KB"

	private fun mediaName(path: String): String? =
		path.substringAfterLast('/').substringAfterLast('\\').takeIf { it.isNotBlank() }

	private fun stripKnownArchiveSuffix(name: String): String =
		name.removeSuffix(".lha")
			.removeSuffix(".lzx")
			.removeSuffix(".lzh")
}
