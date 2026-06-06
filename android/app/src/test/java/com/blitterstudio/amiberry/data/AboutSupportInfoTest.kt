package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Test

class AboutSupportInfoTest {

	@Test
	fun `format includes app and device fields in stable order`() {
		val info = AboutSupportInfo.format(
			versionName = "7.1.0-beta1",
			versionCode = 70100123,
			packageName = "com.blitterstudio.amiberry.debug",
			buildType = "debug",
			androidRelease = "16",
			sdkInt = 36,
			primaryAbi = "arm64-v8a",
			device = "Google Pixel 8",
			hasFullStorageAccess = false,
			externalFilesDir = "/storage/emulated/0/Android/data/com.blitterstudio.amiberry.debug/files"
		)

		assertEquals(
			"""
			Amiberry 7.1.0-beta1 (70100123)
			Package: com.blitterstudio.amiberry.debug
			Build: debug
			Android: 16 (API 36)
			ABI: arm64-v8a
			Device: Google Pixel 8
			All files access: limited
			App storage: /storage/emulated/0/Android/data/com.blitterstudio.amiberry.debug/files
			""".trimIndent(),
			info
		)
	}

	@Test
	fun `format falls back for blank optional values`() {
		val info = AboutSupportInfo.format(
			versionName = "",
			versionCode = 0,
			packageName = "",
			buildType = "",
			androidRelease = "",
			sdkInt = 0,
			primaryAbi = "",
			device = "",
			hasFullStorageAccess = true,
			externalFilesDir = null
		)

		assertEquals(
			"""
			Amiberry unknown (0)
			Package: unknown
			Build: unknown
			Android: unknown (API 0)
			ABI: unknown
			Device: unknown
			All files access: granted
			App storage: unknown
			""".trimIndent(),
			info
		)
	}
}
