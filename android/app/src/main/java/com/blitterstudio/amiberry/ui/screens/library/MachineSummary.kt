package com.blitterstudio.amiberry.ui.screens.library

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.data.model.installedMemory

data class MachineSummary(
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

object MachineSummaries {

	/**
	 * A saved configuration's summary (the Library's detail pane), with every medium the
	 * configuration inserts; [MachineSummary.media] is blank when it inserts none.
	 */
	fun build(settings: EmulatorSettings, roms: List<AmigaFile>): MachineSummary {
		val media = listOfNotNull(
			mediaName(settings.floppy0)?.let { "DF0: $it" },
			mediaName(settings.floppy1)?.let { "DF1: $it" },
			mediaName(settings.floppy2)?.let { "DF2: $it" },
			mediaName(settings.floppy3)?.let { "DF3: $it" },
			mediaName(settings.cdImage)?.let { "CD: $it" }
		) + settings.hardDrives.mapNotNull { drive ->
			// A folder mounted as a drive may end in a separator.
			mediaName(drive.path.trimEnd('/', '\\'))?.let { "HD: $it" }
		}
		return MachineSummary(
			model = settings.baseModel.displayName,
			cpu = buildCpuSummary(settings),
			chipset = buildChipsetSummary(settings),
			memory = buildMemorySummary(settings),
			media = media.joinToString(", "),
			controls = buildControlsSummary(settings),
			rom = romIdentity(settings.romFile, roms),
			romFileName = mediaName(settings.romFile).orEmpty()
		)
	}

	/** What a kickstart_rom_file value boots, from the scanned ROMs; null for none. */
	fun romIdentity(romFile: String, roms: List<AmigaFile>): RomIdentity? = when {
		romFile.isBlank() -> null
		ModelRomAvailability.isBuiltInArosRom(romFile) -> RomIdentity.BuiltInAros
		else -> roms.firstOrNull { it.path == romFile }?.let(ModelRomAvailability::romIdentity)
			?: RomIdentity.Unknown
	}

	/**
	 * The ROMs [model] is built with, by the emulator's names ("KS ROM v1.3
	 * (A500,A1000,A2000)"): the first ROM set of its first configuration, the
	 * emulator's first choice of each.
	 */
	fun romsNeeded(model: AmigaModel, schema: SettingsSchema? = SettingsSchema.current): List<String> {
		val set = model.configs.first().romSets.firstOrNull() ?: return emptyList()
		return set.mapNotNull { request -> request.ids.firstOrNull()?.let { schema?.roms?.get(it)?.name } }
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
	 * Every memory the machine has (installedMemory, as the Memory tab counts it): chip,
	 * slow, Zorro II and Zorro III fast RAM by their short names, every other board by the
	 * layer's label. A board below 1 MB (its *_size_k option) is named as its board.
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
}
