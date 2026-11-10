package com.blitterstudio.amiberry.overlay

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Save
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.components.DeviceFileBrowserDialog
import com.blitterstudio.amiberry.ui.components.hasDeviceStorageAccess

/**
 * The native dialogs the emulator asks for over whatever runs (the menu, a
 * running emulation): gui_message's message boxes and gui_ask_disk's
 * recovery requester (see src/osdep/android_overlay.cpp).
 */
@Composable
fun NativeDialogs(viewModel: OverlayViewModel) {
	viewModel.nativeMessages.firstOrNull()?.let { text ->
		AlertDialog(
			onDismissRequest = viewModel::dismissMessage,
			title = { Text(stringResource(R.string.overlay_message_title)) },
			text = { Text(text) },
			confirmButton = {
				TextButton(onClick = viewModel::dismissMessage) {
					Text(stringResource(R.string.overlay_message_ok))
				}
			}
		)
	}
	viewModel.recovery?.let { request -> FloppyRecoveryDialog(request, viewModel) }
}

/**
 * A drive's image was not found (a restored state, a moved file): locate a
 * replacement, keep going without it, or eject the disk. Locate lists the
 * scanned floppy images and the device's storage.
 */
@Composable
private fun FloppyRecoveryDialog(request: FloppyRecoveryRequest, viewModel: OverlayViewModel) {
	if (viewModel.recoveryPicking) {
		RecoveryFilePicker(request, viewModel)
		return
	}
	AlertDialog(
		onDismissRequest = { viewModel.answerRecovery(OverlayNative.RECOVERY_SKIP) },
		title = { Text(stringResource(R.string.overlay_recovery_title, request.drive)) },
		text = {
			Text(
				stringResource(
					R.string.overlay_recovery_text,
					request.drive,
					request.missingPath.substringAfterLast('/')
				)
			)
		},
		confirmButton = {
			Button(onClick = viewModel::openRecoveryPicker) {
				Text(stringResource(R.string.overlay_recovery_locate))
			}
		},
		dismissButton = {
			Column(horizontalAlignment = Alignment.End) {
				TextButton(onClick = { viewModel.answerRecovery(OverlayNative.RECOVERY_SKIP) }) {
					Text(stringResource(R.string.overlay_recovery_skip))
				}
				TextButton(onClick = { viewModel.answerRecovery(OverlayNative.RECOVERY_EJECT) }) {
					Text(stringResource(R.string.overlay_recovery_eject))
				}
			}
		}
	)
}

/** The replacement images Locate offers: the scanned library, then the device's storage. */
@Composable
private fun RecoveryFilePicker(request: FloppyRecoveryRequest, viewModel: OverlayViewModel) {
	val files by viewModel.floppies.collectAsState()
	val scanning by viewModel.scanning.collectAsState()
	val context = LocalContext.current
	var browsing by remember { mutableStateOf(false) }

	if (browsing) {
		DeviceFileBrowserDialog(
			extensions = FileCategory.FLOPPIES.extensions,
			multiple = false,
			allowFolder = false,
			onDismiss = { browsing = false },
			onPickFiles = { picked ->
				browsing = false
				picked.firstOrNull()?.let { viewModel.answerRecovery(OverlayNative.RECOVERY_LOCATE, it.path) }
			},
			onPickFolder = {}
		)
		return
	}

	Dialog(onDismissRequest = viewModel::closeRecoveryPicker) {
		Surface(shape = RoundedCornerShape(24.dp), tonalElevation = 6.dp) {
			Column(modifier = Modifier.padding(vertical = 16.dp)) {
				Text(
					stringResource(R.string.overlay_recovery_pick_title, request.drive),
					style = MaterialTheme.typography.titleMedium,
					modifier = Modifier.padding(horizontal = 24.dp)
				)
				if (files.isEmpty()) {
					Text(
						text = stringResource(
							if (scanning) R.string.overlay_picker_scanning
							else R.string.overlay_picker_empty_floppy
						),
						style = MaterialTheme.typography.bodyMedium,
						modifier = Modifier.padding(24.dp)
					)
				} else {
					LazyColumn(
						modifier = Modifier
							.fillMaxWidth()
							.heightIn(max = 420.dp)
							.padding(vertical = 8.dp)
					) {
						itemsIndexed(files, key = { _, file -> file.path }) { index, file ->
							Row(
								verticalAlignment = Alignment.CenterVertically,
								modifier = Modifier
									.fillMaxWidth()
									.heightIn(min = 52.dp)
									.clickable {
										viewModel.answerRecovery(OverlayNative.RECOVERY_LOCATE, file.path)
									}
									.padding(horizontal = 24.dp, vertical = 8.dp)
							) {
								Icon(Icons.Filled.Save, contentDescription = null)
								Spacer(modifier = Modifier.width(16.dp))
								Text(
									file.name,
									style = MaterialTheme.typography.bodyLarge,
									maxLines = 2,
									overflow = TextOverflow.Ellipsis,
									modifier = Modifier.weight(1f)
								)
							}
						}
					}
				}
				Row(
					horizontalArrangement = Arrangement.End,
					modifier = Modifier
						.fillMaxWidth()
						.padding(horizontal = 16.dp)
				) {
					OutlinedButton(onClick = { browsing = true }, enabled = hasDeviceStorageAccess(context)) {
						Text(stringResource(R.string.overlay_recovery_browse))
					}
					Spacer(modifier = Modifier.width(8.dp))
					OutlinedButton(onClick = viewModel::closeRecoveryPicker) {
						Text(stringResource(R.string.overlay_cancel))
					}
				}
			}
		}
	}
}
