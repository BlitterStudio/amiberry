package com.blitterstudio.amiberry.ui.components

import android.Manifest
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.Stable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.core.content.ContextCompat
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.DocumentPickerAvailability
import com.blitterstudio.amiberry.data.StoragePermissionActions
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import java.io.File

/** A folder chosen for import: a document-picker tree, or a folder of the device's storage. */
sealed interface PickedFolder {
	data class Tree(val uri: Uri) : PickedFolder
	data class Local(val dir: File) : PickedFolder
}

/**
 * Opens the system document picker, or the in-app browser of the device's storage where
 * no picker exists (Google TV) or when asked for ([browseDevice]). The browser needs All
 * Files Access (READ_EXTERNAL_STORAGE on Android 10): without it the user is told why and
 * sent to the grant screen, and the browser opens once they come back with it granted.
 * Files picked in the browser arrive as file:// URIs, which the importers read like any other.
 */
@Stable
class ImportPicker internal constructor(
	private val openDocuments: () -> Boolean,
	private val openTree: () -> Boolean,
	private val browse: (BrowseMode) -> Unit,
	private val requestAccess: (BrowseMode) -> Unit
) {
	/** Pick files to import: the document picker, or the browser without one. */
	fun pickFiles() {
		if (!openDocuments()) browse(BrowseMode.Files)
	}

	/** Pick a folder to import: the document picker, or the browser without one. */
	fun pickFolder() {
		if (!openTree()) browse(BrowseMode.Folder)
	}

	/** The in-app browser of the device's storage and USB drives, whatever pickers exist. */
	fun browseDevice() = browse(BrowseMode.Files)

	/**
	 * Straight to the grant screen (the caller has already said why), then the browser
	 * once the user returns with access.
	 */
	fun grantAccessThenBrowse() = requestAccess(BrowseMode.Files)
}

enum class BrowseMode { Files, Folder }

/**
 * [mimeTypes] filter the document picker, [extensions] the browser's file list. With
 * [multiple] the user picks several files; [onFolder] (when given) lets the browser's
 * current folder be imported whole. Hosts the browser and the access dialog, so call it
 * from the screen that owns the Import action.
 */
@Composable
fun rememberImportPicker(
	mimeTypes: Array<String>,
	extensions: Set<String>,
	multiple: Boolean = false,
	onFolder: ((PickedFolder) -> Unit)? = null,
	onFiles: (List<Uri>) -> Unit
): ImportPicker {
	val context = LocalContext.current
	val currentOnFiles by rememberUpdatedState(onFiles)
	val currentOnFolder by rememberUpdatedState(onFolder)
	val currentMimeTypes by rememberUpdatedState(mimeTypes)

	var browserMode by rememberSaveable { mutableStateOf<BrowseMode?>(null) }
	var accessDialogMode by rememberSaveable { mutableStateOf<BrowseMode?>(null) }
	// Set while the user is on the grant screen: the browser opens when they return with access.
	var browseAfterGrant by rememberSaveable { mutableStateOf<BrowseMode?>(null) }

	val multipleLauncher = rememberLauncherForActivityResult(ActivityResultContracts.OpenMultipleDocuments()) { uris ->
		if (uris.isNotEmpty()) currentOnFiles(uris)
	}
	val singleLauncher = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
		if (uri != null) currentOnFiles(listOf(uri))
	}
	val treeLauncher = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri ->
		if (uri != null) currentOnFolder?.invoke(PickedFolder.Tree(uri))
	}
	val readPermissionLauncher = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
		val mode = browseAfterGrant
		browseAfterGrant = null
		if (granted && mode != null) browserMode = mode
	}

	val lifecycleOwner = LocalLifecycleOwner.current
	DisposableEffect(lifecycleOwner) {
		val observer = LifecycleEventObserver { _, event ->
			if (event == Lifecycle.Event.ON_RESUME && Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
				val mode = browseAfterGrant
				if (mode != null) {
					browseAfterGrant = null
					if (hasDeviceStorageAccess(context)) browserMode = mode
				}
			}
		}
		lifecycleOwner.lifecycle.addObserver(observer)
		onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
	}

	fun requestAccess(mode: BrowseMode) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
			if (openAllFilesAccessSettings(context)) {
				browseAfterGrant = mode
			} else {
				Toast.makeText(context, R.string.msg_storage_permission_settings_failed, Toast.LENGTH_SHORT).show()
			}
		} else {
			browseAfterGrant = mode
			readPermissionLauncher.launch(Manifest.permission.READ_EXTERNAL_STORAGE)
		}
	}

	val picker = remember(multiple) {
		ImportPicker(
			openDocuments = {
				DocumentPickerAvailability.canOpenDocuments(context) && launches {
					if (multiple) multipleLauncher.launch(currentMimeTypes) else singleLauncher.launch(currentMimeTypes)
				}
			},
			openTree = {
				DocumentPickerAvailability.canOpenDocumentTree(context) && launches { treeLauncher.launch(null) }
			},
			browse = { mode ->
				when {
					hasDeviceStorageAccess(context) -> browserMode = mode
					// Android 11+: say why before sending the user to the grant screen.
					Build.VERSION.SDK_INT >= Build.VERSION_CODES.R -> accessDialogMode = mode
					else -> requestAccess(mode)
				}
			},
			requestAccess = { mode ->
				if (hasDeviceStorageAccess(context)) browserMode = mode else requestAccess(mode)
			}
		)
	}

	accessDialogMode?.let { mode ->
		StorageAccessDialog(
			onOpenSettings = {
				accessDialogMode = null
				requestAccess(mode)
			},
			onDismiss = { accessDialogMode = null }
		)
	}

	browserMode?.let { mode ->
		DeviceFileBrowserDialog(
			extensions = extensions,
			multiple = multiple,
			allowFolder = mode == BrowseMode.Folder || (onFolder != null && multiple),
			onDismiss = { browserMode = null },
			onPickFiles = { files ->
				browserMode = null
				currentOnFiles(files.map(Uri::fromFile))
			},
			onPickFolder = { dir ->
				browserMode = null
				currentOnFolder?.invoke(PickedFolder.Local(dir))
			}
		)
	}

	return picker
}

/** Whether the in-app browser can read shared storage and USB drives. */
fun hasDeviceStorageAccess(context: Context): Boolean =
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
		Environment.isExternalStorageManager()
	} else {
		ContextCompat.checkSelfPermission(context, Manifest.permission.READ_EXTERNAL_STORAGE) ==
			PackageManager.PERMISSION_GRANTED
	}

/** Opens the system's All Files Access screen for Amiberry; false when none opened. */
fun openAllFilesAccessSettings(context: Context): Boolean =
	StoragePermissionActions.permissionRequests(context.packageName, Build.VERSION.SDK_INT).any { request ->
		val intent = Intent(request.action).apply { request.dataUri?.let { data = Uri.parse(it) } }
		runCatching { context.startActivity(intent) }.isSuccess
	}

private inline fun launches(launch: () -> Unit): Boolean =
	try {
		launch()
		true
	} catch (_: ActivityNotFoundException) {
		false
	}

@Composable
private fun StorageAccessDialog(onOpenSettings: () -> Unit, onDismiss: () -> Unit) {
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.device_browser_access_title)) },
		text = { Text(stringResource(R.string.device_browser_access_message)) },
		confirmButton = {
			// OK on a remote goes straight on to the grant screen.
			val openFocus = remember { FocusRequester() }
			LaunchedEffect(Unit) { openFocus.requestFocus() }
			TextButton(
				onClick = onOpenSettings,
				modifier = Modifier.focusRequester(openFocus).dpadFocusIndicator()
			) {
				Text(stringResource(R.string.device_browser_access_open_settings))
			}
		},
		dismissButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}
