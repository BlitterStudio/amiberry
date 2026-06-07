package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.StoragePaths

object AssetExtraction {
	data class CopySummary(
		val copiedFiles: Int = 0,
		val preservedFiles: Int = 0,
		val failedAssets: List<String> = emptyList()
	) {
		val isSuccess: Boolean
			get() = failedAssets.isEmpty()

		operator fun plus(other: CopySummary): CopySummary =
			CopySummary(
				copiedFiles = copiedFiles + other.copiedFiles,
				preservedFiles = preservedFiles + other.preservedFiles,
				failedAssets = failedAssets + other.failedAssets
			)

		companion object {
			val Empty = CopySummary()
			fun copied(): CopySummary = CopySummary(copiedFiles = 1)
			fun preserved(): CopySummary = CopySummary(preservedFiles = 1)
			fun failed(assetPath: String): CopySummary = CopySummary(failedAssets = listOf(assetPath))
		}
	}

	fun shouldSkipTopLevelAsset(filename: String): Boolean =
		filename in skippedTopLevelAssets || filename.startsWith("_")

	fun storagePathForAsset(assetPath: String): String {
		val topDir = assetPath.substringBefore('/')
		val remaining = assetPath.substringAfter('/', "")
		val mappedTopDir = when (topDir) {
			"controllers" -> StoragePaths.CONTROLLERS
			"roms" -> StoragePaths.ROMS
			"whdboot" -> StoragePaths.WHDBOOT
			else -> topDir
		}
		return if (remaining.isEmpty()) mappedTopDir else "$mappedTopDir/$remaining"
	}

	fun shouldPreserveExistingAsset(storagePath: String): Boolean {
		val topDir = storagePath.substringBefore('/')
		return topDir in StoragePaths.userModifiableAssetDirs
	}

	private val skippedTopLevelAssets = setOf("images", "sounds", "webkit", "kioskmode")
}
