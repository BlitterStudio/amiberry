package com.blitterstudio.amiberry.ui.components

import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.FolderOff
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.ui.hasFullStorageAccess

/**
 * Displays an informational banner when the app does not have full storage access.
 * Includes a button to open system settings so the user can grant the permission.
 * Renders nothing if permission is already granted or on Android 10 and below.
 */
@Composable
fun StoragePermissionBanner(modifier: Modifier = Modifier) {
	if (hasFullStorageAccess()) return

	val context = LocalContext.current

	OutlinedCard(modifier = modifier.fillMaxWidth()) {
		Row(
			modifier = Modifier.padding(16.dp),
			verticalAlignment = Alignment.Top
		) {
			Icon(
				Icons.Default.FolderOff,
				contentDescription = null,
				modifier = Modifier.size(24.dp),
				tint = MaterialTheme.colorScheme.onSurfaceVariant
			)
			Spacer(modifier = Modifier.width(12.dp))
			Column(modifier = Modifier.weight(1f)) {
				Text(
					text = stringResource(R.string.storage_permission_banner_title),
					style = MaterialTheme.typography.titleSmall
				)
				Text(
					text = stringResource(R.string.storage_permission_banner_message),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
				TextButton(
					onClick = {
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
							try {
								val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
								intent.data = Uri.parse("package:${context.packageName}")
								context.startActivity(intent)
							} catch (_: Exception) {
								val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
								context.startActivity(intent)
							}
						}
					}
				) {
					Text(stringResource(R.string.storage_permission_banner_action))
				}
			}
		}
	}
}
