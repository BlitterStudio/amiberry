package com.blitterstudio.amiberry.data

object AboutSupportInfo {

	fun format(
		versionName: String,
		versionCode: Long,
		packageName: String,
		buildType: String,
		androidRelease: String,
		sdkInt: Int,
		primaryAbi: String,
		device: String,
		hasFullStorageAccess: Boolean,
		externalFilesDir: String?
	): String = listOf(
		"Amiberry ${clean(versionName)} ($versionCode)",
		"Package: ${clean(packageName)}",
		"Build: ${clean(buildType)}",
		"Android: ${clean(androidRelease)} (API $sdkInt)",
		"ABI: ${clean(primaryAbi)}",
		"Device: ${clean(device)}",
		"All files access: ${if (hasFullStorageAccess) "granted" else "limited"}",
		"App storage: ${clean(externalFilesDir.orEmpty())}"
	).joinToString(separator = "\n")

	private fun clean(value: String): String =
		value.trim().ifBlank { "unknown" }
}
