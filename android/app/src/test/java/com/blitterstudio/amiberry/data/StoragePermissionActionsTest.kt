package com.blitterstudio.amiberry.data

import android.os.Build
import android.provider.Settings
import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class StoragePermissionActionsTest {

	@Test
	fun `banner is hidden only when full storage access is already granted`() {
		assertEquals(false, StoragePermissionActions.shouldShowBanner(hasFullStorageAccess = true))
		assertEquals(true, StoragePermissionActions.shouldShowBanner(hasFullStorageAccess = false))
	}

	@Test
	fun `android 11 and newer tries app-specific settings before generic all-files settings`() {
		val requests = StoragePermissionActions.permissionRequests(
			packageName = "com.blitterstudio.amiberry",
			sdkInt = Build.VERSION_CODES.R
		)

		assertEquals(2, requests.size)
		assertEquals(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, requests[0].action)
		assertEquals("package:com.blitterstudio.amiberry", requests[0].dataUri)
		assertEquals(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION, requests[1].action)
		assertEquals(null, requests[1].dataUri)
	}

	@Test
	fun `android 10 and older has no all-files settings request`() {
		assertTrue(
			StoragePermissionActions.permissionRequests(
				packageName = "com.blitterstudio.amiberry",
				sdkInt = Build.VERSION_CODES.Q
			).isEmpty()
		)
	}

	@Test
	fun `failed settings launch has a user-visible message`() {
		assertEquals(
			ConfigurationActions.Message(R.string.msg_storage_permission_settings_failed),
			StoragePermissionActions.openSettingsFailedMessage()
		)
	}
}
