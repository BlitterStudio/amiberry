package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.data.model.SettingText
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.data.model.installedMemory

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

/**
 * What Quick Start shows for a WHDLoad launch. WHDBooter chooses the machine
 * (model, CPU, chipset, memory, Kickstart) when the game starts, from its game
 * database, the installed Kickstarts or the game's own configuration, so only
 * the values the launcher passes are listed.
 */
data class QuickStartWhdLoadSummary(
	val media: String,
	/** The game's own configuration file, which WHDBooter loads instead of choosing; null without one. */
	val perGameConfig: String?,
	val controls: String,
	val port0: SettingText,
	val port1: SettingText,
	val shader: SettingText
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

	/**
	 * The summary of what a Disk, CD or Recent launch starts; its ROM row is the
	 * settings' own ROM, which the launch boots. WHDLoad: [buildWhdLoadSummary].
	 */
	fun buildEffectiveSummary(
		settings: EmulatorSettings,
		launchMode: QuickStartLaunchMode,
		selectedModelAvailable: Boolean,
		roms: List<AmigaFile> = emptyList()
	): QuickStartEffectiveSummary {
		return QuickStartEffectiveSummary(
			model = if (selectedModelAvailable) settings.baseModel.displayName else "No compatible model",
			cpu = buildCpuSummary(settings),
			chipset = buildChipsetSummary(settings),
			memory = buildMemorySummary(settings),
			media = buildMediaSummary(settings, launchMode, selectedWhdloadName = null),
			controls = buildControlsSummary(settings),
			rom = if (selectedModelAvailable) romIdentity(settings.romFile, roms) else null,
			romFileName = mediaName(settings.romFile).orEmpty()
		)
	}

	/**
	 * The summary of a WHDLoad launch, from [launchSettings]: the shader and
	 * controls the launch passes (the game's own configuration's where it sets
	 * them), and [perGameConfigName], the configuration's file name, if any.
	 */
	fun buildWhdLoadSummary(
		launchSettings: EmulatorSettings,
		selectedWhdloadName: String?,
		perGameConfigName: String?
	): QuickStartWhdLoadSummary = QuickStartWhdLoadSummary(
		media = buildMediaSummary(launchSettings, QuickStartLaunchMode.WHDLOAD, selectedWhdloadName),
		perGameConfig = perGameConfigName,
		controls = buildControlsSummary(launchSettings),
		port0 = LauncherChoices.portDevice(launchSettings.joyport0),
		port1 = LauncherChoices.portDevice(launchSettings.joyport1),
		shader = LauncherChoices.shader(launchSettings.shader)
	)

	/**
	 * Whether a Disk or CD launch boots AROS: the settings name the built-in
	 * AROS ROM, or name no ROM while the model's Kickstart is not installed.
	 */
	fun showsArosNotice(
		settings: EmulatorSettings,
		selectedRoms: ModelRomAvailability.SelectedRoms,
		launchMode: QuickStartLaunchMode
	): Boolean = (launchMode == QuickStartLaunchMode.DISK || launchMode == QuickStartLaunchMode.CD) && (
		ModelRomAvailability.isBuiltInArosRom(settings.romFile) ||
			(settings.romFile.isBlank() && selectedRoms.aros)
		)

	/**
	 * Whether WHDLoad games can boot AROS: the A1200 Kickstart is not installed
	 * and AROS is. WHDBooter starts the games it runs on the A1200 with the
	 * A1200 preset's Kickstart, and the emulator boots AROS without one.
	 * Which games those are is WHDBooter's choice at start.
	 */
	fun showsWhdLoadArosNotice(roms: List<AmigaFile>): Boolean =
		ModelRomAvailability.selectRomsForModel(AmigaModel.A1200, roms).aros

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

	/**
	 * Every memory the machine has (installedMemory, as the Memory tab
	 * counts it): chip, slow, Zorro II and Zorro III fast RAM by their short
	 * names, every other board by the layer's label. A board below 1 MB
	 * (its *_size_k option) is named as its board.
	 */
	private fun buildMemorySummary(settings: EmulatorSettings): String {
		val schema = SettingsSchema.current
			?: return "${chipRamLabel(settings.chipRam)} Chip"
		return installedMemory(settings, schema).joinToString(", ") { memory ->
			val size = when (memory.id) {
				"chipmem_size" -> chipRamLabel(settings.chipRam)
				"bogomem_size" -> slowRamLabel(settings.slowRam)
				else -> sizeLabel(memory.kb)
			}
			val board = memory.id.removeSuffix("_k")
			val name = SHORT_MEMORY_NAMES[board] ?: schema.label(board) ?: board
			"$size $name"
		}
	}

	private val SHORT_MEMORY_NAMES = mapOf(
		"chipmem_size" to "Chip",
		"bogomem_size" to "Slow",
		"fastmem_size" to "Fast",
		"z3mem_size" to "Z3"
	)

	private fun sizeLabel(kb: Int): String = if (kb % 1024 == 0) "${kb / 1024} MB" else "$kb KB"

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
