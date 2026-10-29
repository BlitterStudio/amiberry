package com.blitterstudio.amiberry.ui.components

import androidx.compose.foundation.layout.Arrangement
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
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.data.DocumentPickerAvailability
import com.blitterstudio.amiberry.data.StoragePermissionActions
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.hasFullStorageAccess

/**
 * The optional All Files Access card: imports work without it (through the document
 * picker), and the access only lets the in-app browser read this device's folders and
 * USB drives. Without a document picker (Google TV) that browser is the only way in, and
 * the card says so. "Not now" hides it for good; renders nothing once access is granted.
 * [onGrant] replaces the default (open the grant screen), e.g. to open the browser after.
 */
@Composable
fun StoragePermissionBanner(
	modifier: Modifier = Modifier,
	onGrant: (() -> Unit)? = null,
	onMessage: (ConfigurationActions.Message) -> Unit = {}
) {
	val context = LocalContext.current
	val preferences = remember { AppPreferences.getInstance(context) }
	val dismissed by preferences.storageAccessCardDismissed
	if (!StoragePermissionActions.shouldShowBanner(hasFullStorageAccess(), dismissed)) return
	val hasDocumentPicker = remember { DocumentPickerAvailability.canOpenDocuments(context) }

	OutlinedCard(modifier = modifier.fillMaxWidth()) {
		Row(
			modifier = Modifier.padding(start = 16.dp, top = 12.dp, end = 8.dp, bottom = 4.dp),
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
					text = stringResource(R.string.storage_access_card_title),
					style = MaterialTheme.typography.titleSmall
				)
				Text(
					text = stringResource(
						if (hasDocumentPicker) R.string.storage_access_card_message else R.string.storage_access_card_message_no_picker
					),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
				Row(
					modifier = Modifier.fillMaxWidth(),
					horizontalArrangement = Arrangement.End
				) {
					TextButton(
						onClick = { preferences.dismissStorageAccessCard() },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.storage_access_card_not_now))
					}
					TextButton(
						onClick = {
							if (onGrant != null) {
								onGrant()
							} else if (!openAllFilesAccessSettings(context)) {
								onMessage(StoragePermissionActions.openSettingsFailedMessage())
							}
						},
						modifier = Modifier.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.storage_permission_banner_action))
					}
				}
			}
		}
	}
}
