package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity

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
	val controls: String,
	/** The ROM the launch boots: a Kickstart by name, the built-in AROS, or null without one. */
	val rom: RomIdentity? = null,
	/** The ROM's file name, for one the launcher cannot identify. */
	val romFileName: String = ""
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

	/** The summary of what Start launches; its ROM row is the settings' own ROM, which the launch boots. */
	fun buildEffectiveSummary(
		settings: EmulatorSettings,
		launchMode: QuickStartLaunchMode,
		selectedModelAvailable: Boolean,
		selectedWhdloadName: String? = null,
		roms: List<AmigaFile> = emptyList()
	): QuickStartEffectiveSummary {
		return QuickStartEffectiveSummary(
			model = if (selectedModelAvailable) settings.baseModel.displayName else "No compatible model",
			cpu = buildCpuSummary(settings),
			chipset = buildChipsetSummary(settings),
			memory = buildMemorySummary(settings),
			media = buildMediaSummary(settings, launchMode, selectedWhdloadName),
			controls = buildControlsSummary(settings),
			rom = if (selectedModelAvailable) romIdentity(settings.romFile, roms) else null,
			romFileName = mediaName(settings.romFile).orEmpty()
		)
	}

	/**
	 * Whether Quick Start tells the user the launch boots AROS: the settings name the
	 * built-in AROS ROM, or name no ROM while the model's Kickstart is not installed.
	 */
	fun showsArosNotice(
		settings: EmulatorSettings,
		selectedRoms: ModelRomAvailability.SelectedRoms,
		launchMode: QuickStartLaunchMode
	): Boolean = launchMode != QuickStartLaunchMode.RECENT && (
		ModelRomAvailability.isBuiltInArosRom(settings.romFile) ||
			(settings.romFile.isBlank() && selectedRoms.aros)
		)

	/**
	 * After a ROM import: settings that boot AROS (or name no ROM) move to the model's
	 * Kickstart once one is installed, as the emulator would otherwise keep booting
	 * AROS. Null when nothing changes.
	 */
	fun withInstalledKickstart(settings: EmulatorSettings, roms: List<AmigaFile>): EmulatorSettings? {
		if (settings.romFile.isNotBlank() && !ModelRomAvailability.isBuiltInArosRom(settings.romFile)) return null
		val selected = ModelRomAvailability.selectRomsForModel(settings.baseModel, roms)
		val kick = selected.kick ?: return null
		return settings.copy(
			baseConfig = selected.config,
			romFile = kick.path,
			romExtFile = selected.ext?.path.orEmpty()
		)
	}

	/** What a kickstart_rom_file value boots, from the scanned ROMs; null for none. */
	fun romIdentity(romFile: String, roms: List<AmigaFile>): RomIdentity? = when {
		romFile.isBlank() -> null
		ModelRomAvailability.isBuiltInArosRom(romFile) -> RomIdentity.BuiltInAros
		else -> roms.firstOrNull { it.path == romFile }?.let(ModelRomAvailability::romIdentity)
			?: RomIdentity.Unknown
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
			settings.onScreenJoystick && settings.onScreenKeyboardAvailable -> "On-screen joystick and keyboard"
			settings.onScreenJoystick -> "On-screen joystick"
			settings.onScreenKeyboardAvailable -> "On-screen keyboard"
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
