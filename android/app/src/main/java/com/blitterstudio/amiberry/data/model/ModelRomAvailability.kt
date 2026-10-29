package com.blitterstudio.amiberry.data.model

/** What an installed ROM file is. */
sealed interface RomIdentity {
	/** A ROM the emulator's ROM table knows, by its name there ("KS ROM v1.3 (A500,A1000,A2000)"). */
	data class Kickstart(val name: String) : RomIdentity

	/** One of the free AROS ROM files the emulator boots when a Kickstart is missing. */
	data object BuiltInAros : RomIdentity

	data object Unknown : RomIdentity
}

object ModelRomAvailability {

	/**
	 * The kickstart_rom_file value that boots the built-in AROS ROM
	 * (is_builtin_aros_rom in src/memory.cpp). No file at all would first
	 * look for a kick.rom, so the launcher names AROS explicitly.
	 */
	const val AROS_ROM_FILE = ":AROS"

	/** The AROS ROM files the emulator loads from its ROMs directory (find_kickstart_replacement_directory). */
	private const val AROS_EXT_FILE_NAME = "aros-ext.bin"
	private val AROS_FILES = setOf("aros-rom.bin", AROS_EXT_FILE_NAME)

	/**
	 * The ROM files for a model, and the preset configuration they belong to.
	 * [aros] is set when no Kickstart for the model is installed and it boots
	 * the built-in AROS ROM instead.
	 */
	data class SelectedRoms(
		val kick: AmigaFile?,
		val ext: AmigaFile?,
		val config: Int = 0,
		val aros: Boolean = false
	) {
		/** The model can start: with its Kickstart, or on AROS. */
		val isBootable: Boolean
			get() = kick != null || aros

		/** The kickstart_rom_file to write; empty when the model cannot start. */
		val romFile: String
			get() = kick?.path ?: if (aros) AROS_ROM_FILE else ""

		val romExtFile: String
			get() = ext?.path.orEmpty()
	}

	fun availableModels(roms: List<AmigaFile>): List<AmigaModel> {
		return AmigaModel.entries.filter { model -> isModelAvailable(model, roms) }
	}

	/** The model can start, with its Kickstart or on AROS. */
	fun isModelAvailable(model: AmigaModel, roms: List<AmigaFile>): Boolean {
		return selectRomsForModel(model, roms).isBootable
	}

	/** One of the model's Kickstart ROM sets is installed. */
	fun hasKickstart(model: AmigaModel, roms: List<AmigaFile>): Boolean =
		selectRomsForModel(model, roms).kick != null

	fun romIdentity(rom: AmigaFile): RomIdentity {
		if (rom.name.lowercase() in AROS_FILES) return RomIdentity.BuiltInAros
		val id = detectRomId(rom) ?: return RomIdentity.Unknown
		val name = SettingsSchema.current?.roms?.get(id)?.name ?: return RomIdentity.Unknown
		return RomIdentity.Kickstart(name)
	}

	/** Whether a kickstart_rom_file value names the built-in AROS ROM (an RP9's "<file>:AROS" too). */
	fun isBuiltInArosRom(romFile: String): Boolean =
		romFile == AROS_ROM_FILE || (romFile.length > AROS_ROM_FILE.length && romFile.endsWith(AROS_ROM_FILE))

	/** The installed file is the AROS extension ROM (aros-ext.bin), the second of the two AROS files. */
	fun isBuiltInArosExtension(rom: AmigaFile): Boolean =
		rom.name.equals(AROS_EXT_FILE_NAME, ignoreCase = true)

	/** Both AROS ROM files are installed, so a model without its Kickstart can boot AROS. */
	fun hasBuiltInAros(roms: List<AmigaFile>): Boolean =
		AROS_FILES.all { file -> roms.any { it.name.equals(file, ignoreCase = true) } }

	/**
	 * Whether AROS can stand in for the model's ROMs: only for a computer
	 * that needs nothing but a Kickstart. The CD32 and CDTV play their discs
	 * with their own (extended) ROMs and boards such as the DraCo need their
	 * own boot ROM, which AROS does not replace.
	 */
	fun canBootAros(model: AmigaModel): Boolean =
		!model.hasCd && model.configs.first().romSets.all { set -> set.all { it.role == "kick" } }

	/**
	 * The ROMs a model is built with: the first ROM set (from the emulator's
	 * preset builder, in the bundled settings schema) whose every request has
	 * an installed ROM, recognised by checksum. The first configuration's
	 * sets come first; a later configuration (an A500 with Kickstart 1.2, an
	 * A3000 with 3.1) is chosen only when none of the earlier ones can be built.
	 * With none installed, a model AROS can stand in for boots the built-in
	 * AROS ROM in its first configuration.
	 */
	fun selectRomsForModel(model: AmigaModel, roms: List<AmigaFile>): SelectedRoms {
		if (roms.isEmpty()) return SelectedRoms(null, null)
		val romsById = roms
			.mapNotNull { rom -> detectRomId(rom)?.let { id -> id to rom } }
			.groupBy({ it.first }, { it.second })

		model.configs.forEachIndexed { config, presetConfig ->
			for (set in presetConfig.romSets) {
				val picks = set.map { request ->
					val id = request.ids.firstOrNull { !romsById[it].isNullOrEmpty() } ?: return@map null
					request.role to pickDeterministic(romsById[id].orEmpty())
				}
				if (picks.any { it == null }) continue
				val kick = picks.firstOrNull { it!!.first == "kick" }?.second ?: continue
				val ext = picks.firstOrNull { it!!.first == "ext" }?.second
				return SelectedRoms(kick, ext, config)
			}
		}
		return SelectedRoms(null, null, aros = canBootAros(model) && hasBuiltInAros(roms))
	}

	private fun detectRomId(rom: AmigaFile): Int? {
		val crc = rom.crc32 ?: return null
		val masked = crc and 0xffffffffL
		// The emulator identifies a few ROMs from several files; the schema
		// lists those with no checksum, which an empty file would match.
		if (masked == 0L) return null
		return SettingsSchema.current?.roms?.values?.firstOrNull { it.crc32 == masked }?.id
	}

	private fun pickDeterministic(candidates: List<AmigaFile>): AmigaFile? {
		return candidates.sortedBy { it.name.lowercase() }.firstOrNull()
	}
}
