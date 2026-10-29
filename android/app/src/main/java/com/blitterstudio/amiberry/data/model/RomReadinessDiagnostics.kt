package com.blitterstudio.amiberry.data.model

enum class RomReadinessStatus {
	NoRoms,
	NoCompatibleModels,
	Ready
}

data class RomReadinessDiagnostics(
	val status: RomReadinessStatus,
	val scannedRomCount: Int,
	val recognizedRomCount: Int,
	val unknownRomCount: Int,
	/** The models that can start, with their Kickstart or on AROS. */
	val availableModels: List<AmigaModel>,
	/** Common models whose Kickstart is not installed (they may still boot AROS). */
	val missingCommonModels: List<AmigaModel>
) {
	companion object {
		private val COMMON_MODELS = listOf(
			AmigaModel.A500,
			AmigaModel.A600,
			AmigaModel.A1200,
			AmigaModel.CD32
		)

		fun from(roms: List<AmigaFile>): RomReadinessDiagnostics {
			val availableModels = ModelRomAvailability.availableModels(roms)
			val identities = roms.map { ModelRomAvailability.romIdentity(it) }
			val recognizedRomCount = identities.count { it is RomIdentity.Kickstart }
			val unknownRomCount = identities.count { it is RomIdentity.Unknown }
			val status = when {
				roms.isEmpty() -> RomReadinessStatus.NoRoms
				availableModels.isEmpty() -> RomReadinessStatus.NoCompatibleModels
				else -> RomReadinessStatus.Ready
			}

			return RomReadinessDiagnostics(
				status = status,
				scannedRomCount = roms.size,
				recognizedRomCount = recognizedRomCount,
				unknownRomCount = unknownRomCount,
				availableModels = availableModels,
				missingCommonModels = COMMON_MODELS.filter { !ModelRomAvailability.hasKickstart(it, roms) }
			)
		}
	}
}
