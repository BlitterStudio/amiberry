package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R

object AssetExtractionFailureActions {

	fun summary(copySummary: AssetExtraction.CopySummary?): String? {
		if (copySummary == null || copySummary.isSuccess) return null

		val visibleFailures = copySummary.failedAssets.take(3)
		val remaining = copySummary.failedAssets.size - visibleFailures.size
		return buildString {
			append(visibleFailures.joinToString(", "))
			if (remaining > 0) {
				append(", +")
				append(remaining)
				append(" more")
			}
		}
	}

	fun copyPayload(
		copySummary: AssetExtraction.CopySummary?,
		packageName: String,
		versionName: String,
		versionCode: Long,
		sdkInt: Int,
		externalDirPath: String?
	): String = buildString {
		appendLine("Amiberry Android setup failure")
		appendLine("Package: $packageName")
		appendLine("Version: ${versionName.ifBlank { "unknown" }} ($versionCode)")
		appendLine("SDK: $sdkInt")
		appendLine("External files dir: ${externalDirPath ?: "unavailable"}")
		appendLine()
		if (copySummary == null) {
			appendLine("Copy summary: unavailable")
		} else {
			appendLine("Copied files: ${copySummary.copiedFiles}")
			appendLine("Preserved files: ${copySummary.preservedFiles}")
			appendLine(
				"Failed assets: ${
					if (copySummary.failedAssets.isEmpty()) {
						"none"
					} else {
						copySummary.failedAssets.joinToString(", ")
					}
				}"
			)
		}
	}

	fun copyMessage(): ConfigurationActions.Message =
		ConfigurationActions.Message(R.string.msg_asset_extraction_details_copied)
}
