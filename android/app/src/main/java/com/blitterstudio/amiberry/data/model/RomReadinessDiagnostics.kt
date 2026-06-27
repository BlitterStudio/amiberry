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
	val availableModels: List<AmigaModel>,
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
			val recognizedRomCount = roms.count { ModelRomAvailability.isRecognizedRom(it) }
			val status = when {
				roms.isEmpty() -> RomReadinessStatus.NoRoms
				availableModels.isEmpty() -> RomReadinessStatus.NoCompatibleModels
				else -> RomReadinessStatus.Ready
			}

			return RomReadinessDiagnostics(
				status = status,
				scannedRomCount = roms.size,
				recognizedRomCount = recognizedRomCount,
				unknownRomCount = roms.size - recognizedRomCount,
				availableModels = availableModels,
				missingCommonModels = COMMON_MODELS.filter { it !in availableModels }
			)
		}
	}
}
