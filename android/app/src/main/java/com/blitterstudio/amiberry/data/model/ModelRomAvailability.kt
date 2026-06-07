package com.blitterstudio.amiberry.data.model

object ModelRomAvailability {

	data class SelectedRoms(
		val kick: AmigaFile?,
		val ext: AmigaFile?
	)

	fun availableModels(roms: List<AmigaFile>): List<AmigaModel> {
		return AmigaModel.entries.filter { model -> isModelAvailable(model, roms) }
	}

	fun isModelAvailable(model: AmigaModel, roms: List<AmigaFile>): Boolean {
		return selectRomsForModel(model, roms).kick != null
	}

	fun selectRomsForModel(model: AmigaModel, roms: List<AmigaFile>): SelectedRoms {
		if (roms.isEmpty()) return SelectedRoms(null, null)
		val profile = MODEL_ROM_PROFILE[model] ?: return SelectedRoms(null, null)

		val romsById = roms
			.mapNotNull { rom -> detectRomId(rom)?.let { id -> id to rom } }
			.groupBy({ it.first }, { it.second })

		val selectedKickId = profile.kickIds.firstOrNull { id -> !romsById[id].isNullOrEmpty() }
		val kick = selectedKickId?.let { id -> pickDeterministic(romsById[id].orEmpty()) }

		if (kick == null) {
			return SelectedRoms(null, null)
		}

		val ext = when (model) {
			AmigaModel.CD32 -> {
				// id 64 is combined KS+ext ROM; id 18 requires ext id 19.
				if (selectedKickId == 64) {
					null
				} else {
					val extId = profile.extIds.firstOrNull { id -> !romsById[id].isNullOrEmpty() }
					if (extId == null) return SelectedRoms(null, null)
					pickDeterministic(romsById[extId].orEmpty())
				}
			}
			AmigaModel.CDTV -> {
				// CDTV defaults require both Kickstart and extended ROM.
				val extId = profile.extIds.firstOrNull { id -> !romsById[id].isNullOrEmpty() }
				if (extId == null) return SelectedRoms(null, null)
				pickDeterministic(romsById[extId].orEmpty())
			}
			else -> null
		}

		return SelectedRoms(kick, ext)
	}

	private fun detectRomId(rom: AmigaFile): Int? {
		val crc = rom.crc32 ?: return null
		return ROM_CRC_TO_ID[crc and 0xffffffffL]
	}

	private fun pickDeterministic(candidates: List<AmigaFile>): AmigaFile? {
		return candidates.sortedBy { it.name.lowercase() }.firstOrNull()
	}

	private data class ModelRomProfile(
		val kickIds: List<Int>,
		val extIds: List<Int> = emptyList()
	)

	/**
	 * Exact --model ROM priority behavior from main.cpp wrappers:
	 * A500->bip_a500(130), A500P->bip_a500plus(-1), A2000->bip_a2000(130), etc.
	 */
	private val MODEL_ROM_PROFILE = mapOf(
		AmigaModel.A500 to ModelRomProfile(kickIds = listOf(6, 5, 4)),
		AmigaModel.A500_PLUS to ModelRomProfile(kickIds = listOf(7, 6, 5)),
		AmigaModel.A600 to ModelRomProfile(kickIds = listOf(10, 9, 8)),
		AmigaModel.A1000 to ModelRomProfile(kickIds = listOf(24)),
		AmigaModel.A2000 to ModelRomProfile(kickIds = listOf(6, 5, 4)),
		AmigaModel.A3000 to ModelRomProfile(kickIds = listOf(59)),
		AmigaModel.A1200 to ModelRomProfile(kickIds = listOf(11, 15, 276, 281, 286, 291, 304)),
		AmigaModel.A4000 to ModelRomProfile(kickIds = listOf(16, 31, 13, 12, 46, 278, 283, 288, 293, 306)),
		AmigaModel.CD32 to ModelRomProfile(kickIds = listOf(64, 18), extIds = listOf(19)),
		AmigaModel.CDTV to ModelRomProfile(kickIds = listOf(6, 32), extIds = listOf(20, 21, 22))
	)

	// CRC32 -> ROM ID mapping for the ids used above.
	private val ROM_CRC_TO_ID: Map<Long, Int> = mapOf(
		0x9ed783d0L to 4,
		0xa6ce1636L to 5,
		0xc4f0f55fL to 6,
		0xc3bdb240L to 7,
		0x83028fb5L to 8,
		0x64466c2aL to 9,
		0x43b0df7bL to 10,
		0x6c9b07d2L to 11,
		0x9e6ac152L to 12,
		0x2b4566f1L to 13,
		0x1483a091L to 15,
		0xd6bae334L to 16,
		0x1e62d4a5L to 18,
		0x87746be2L to 19,
		0x42baa124L to 20,
		0x30b54232L to 21,
		0xceae68d2L to 22,
		0x0b1ad2d0L to 24,
		0x43b6dd22L to 31,
		0xe0f37258L to 32,
		0xbc0ec13fL to 59,
		0xf5d4f3c8L to 64,
		0xf17fa97fL to 276,
		0xd47e18fdL to 278,
		0xb87506a7L to 281,
		0x1b84cb33L to 283,
		0xbd1ff75eL to 286,
		0x9bb8fc93L to 288,
		0x2b653371L to 291,
		0xf3ced3b8L to 293,
		0x5c40328aL to 304,
		0x4bea9798L to 306,
	)
}
