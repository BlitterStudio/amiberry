package com.blitterstudio.amiberry.data.model

object ModelRomAvailability {

	/** The ROM files for a model, and the preset configuration they belong to. */
	data class SelectedRoms(
		val kick: AmigaFile?,
		val ext: AmigaFile?,
		val config: Int = 0
	)

	fun availableModels(roms: List<AmigaFile>): List<AmigaModel> {
		return AmigaModel.entries.filter { model -> isModelAvailable(model, roms) }
	}

	fun isModelAvailable(model: AmigaModel, roms: List<AmigaFile>): Boolean {
		return selectRomsForModel(model, roms).kick != null
	}

	fun isRecognizedRom(rom: AmigaFile): Boolean =
		detectRomId(rom) != null

	/**
	 * The ROMs a model is built with: the first ROM set (from the emulator's
	 * preset builder, in the bundled settings schema) whose every request has
	 * an installed ROM, recognised by checksum. The first configuration's
	 * sets come first; a later configuration (an A500 with Kickstart 1.2, an
	 * A3000 with 3.1) is chosen only when none of the earlier ones can be built.
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
		return SelectedRoms(null, null)
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
