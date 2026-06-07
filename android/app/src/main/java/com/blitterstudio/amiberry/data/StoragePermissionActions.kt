package com.blitterstudio.amiberry.data

import android.os.Build
import android.provider.Settings
import com.blitterstudio.amiberry.R

object StoragePermissionActions {

	data class PermissionRequest(
		val action: String,
		val dataUri: String? = null
	)

	fun shouldShowBanner(hasFullStorageAccess: Boolean): Boolean =
		!hasFullStorageAccess

	fun permissionRequests(packageName: String, sdkInt: Int): List<PermissionRequest> {
		if (sdkInt < Build.VERSION_CODES.R) return emptyList()

		return listOf(
			PermissionRequest(
				action = Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
				dataUri = "package:$packageName"
			),
			PermissionRequest(action = Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
		)
	}

	fun openSettingsFailedMessage(): ConfigurationActions.Message =
		ConfigurationActions.Message(R.string.msg_storage_permission_settings_failed)
}
