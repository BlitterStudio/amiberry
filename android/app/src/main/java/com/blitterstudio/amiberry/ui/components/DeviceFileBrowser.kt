package com.blitterstudio.amiberry.ui.components

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.selection.toggleable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.InsertDriveFile
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Download
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.PhoneAndroid
import androidx.compose.material.icons.filled.Usb
import androidx.compose.material3.Button
import androidx.compose.material3.Checkbox
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.DeviceStorage
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

/** What the browser lists: the storage roots, or one folder (null: it cannot be read). */
private sealed interface BrowserListing {
	data object Loading : BrowserListing
	data class Roots(val roots: List<DeviceStorage.Root>) : BrowserListing
	data class Folder(val entries: List<DeviceStorage.Entry>?) : BrowserListing
}

/**
 * A full-screen browser of the device's storage and USB drives. Folders open on OK/tap;
 * files with one of [extensions] are picked ([multiple]: ticked, then Import) and
 * [allowFolder] adds "Import this folder". Back goes up a folder, and closes at the top.
 */
@Composable
internal fun DeviceFileBrowserDialog(
	extensions: Set<String>,
	multiple: Boolean,
	allowFolder: Boolean,
	onDismiss: () -> Unit,
	onPickFiles: (List<File>) -> Unit,
	onPickFolder: (File) -> Unit
) {
	val context = LocalContext.current
	var roots by remember { mutableStateOf<List<DeviceStorage.Root>>(emptyList()) }
	var currentDir by remember { mutableStateOf<File?>(null) }
	// The root the user opened bounds "up": above it, the browser lists the roots again.
	var currentRoot by remember { mutableStateOf<File?>(null) }
	// The folder just left, so going up puts focus back on it.
	var cameFrom by remember { mutableStateOf<File?>(null) }
	var selected by remember { mutableStateOf<Set<File>>(emptySet()) }

	val listing by produceState<BrowserListing>(BrowserListing.Loading, currentDir) {
		val dir = currentDir
		value = withContext(Dispatchers.IO) {
			if (dir == null) {
				BrowserListing.Roots(DeviceStorage.roots(context).also { roots = it })
			} else {
				BrowserListing.Folder(DeviceStorage.entries(dir, extensions))
			}
		}
	}

	fun goUp() {
		val dir = currentDir ?: return onDismiss()
		cameFrom = dir
		currentDir = if (dir == currentRoot) null else dir.parentFile
	}
	fun open(dir: File) {
		if (currentDir == null) currentRoot = dir
		cameFrom = null
		currentDir = dir
	}

	// Back (the dialog's dismiss request) goes up a folder; Close leaves at once.
	Dialog(
		onDismissRequest = ::goUp,
		properties = DialogProperties(usePlatformDefaultWidth = false, decorFitsSystemWindows = false)
	) {
		Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.surface) {
			Column(modifier = Modifier.fillMaxSize().safeDrawingPadding()) {
				BrowserHeader(
					title = currentDir?.let { dir -> roots.firstOrNull { it.dir == dir }?.label ?: dir.name }
						?: stringResource(R.string.device_browser_title),
					path = currentDir?.absolutePath,
					onUp = ::goUp,
					onClose = onDismiss
				)
				HorizontalDivider()
				Column(modifier = Modifier.weight(1f).fillMaxWidth()) {
					when (val current = listing) {
						BrowserListing.Loading -> Unit
						is BrowserListing.Roots -> BrowserRows(
							rows = current.roots.map { root ->
								BrowserRow(root.dir, root.label, rootIcon(root.kind), isFolder = true)
							},
							focusFile = cameFrom,
							multiple = multiple,
							selected = selected,
							emptyText = stringResource(R.string.device_browser_no_storage),
							onOpen = ::open,
							onToggle = {},
							onPick = {}
						)
						is BrowserListing.Folder -> BrowserRows(
							rows = current.entries?.map { entry ->
								BrowserRow(
									entry.file,
									entry.file.name,
									if (entry.isDirectory) Icons.Default.Folder else Icons.AutoMirrored.Filled.InsertDriveFile,
									entry.isDirectory
								)
							}.orEmpty(),
							focusFile = cameFrom,
							multiple = multiple,
							selected = selected,
							emptyText = stringResource(
								if (current.entries == null) R.string.device_browser_unreadable else R.string.device_browser_empty
							),
							onOpen = ::open,
							onToggle = { file -> selected = if (file in selected) selected - file else selected + file },
							onPick = { file -> onPickFiles(listOf(file)) }
						)
					}
				}
				val dir = currentDir
				if (dir != null && (allowFolder || multiple)) {
					HorizontalDivider()
					Row(
						modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 8.dp),
						horizontalArrangement = Arrangement.spacedBy(8.dp, Alignment.End),
						verticalAlignment = Alignment.CenterVertically
					) {
						if (allowFolder) {
							OutlinedButton(onClick = { onPickFolder(dir) }, modifier = Modifier.dpadFocusIndicator()) {
								Text(stringResource(R.string.device_browser_import_folder))
							}
						}
						if (multiple) {
							Button(
								onClick = { onPickFiles(selected.sortedBy { it.name.lowercase() }) },
								enabled = selected.isNotEmpty(),
								modifier = Modifier.dpadFocusIndicator()
							) {
								Text(stringResource(R.string.device_browser_import_selected, selected.size))
							}
						}
					}
				}
			}
		}
	}
}

private data class BrowserRow(val file: File, val label: String, val icon: ImageVector, val isFolder: Boolean)

private fun rootIcon(kind: DeviceStorage.RootKind): ImageVector = when (kind) {
	DeviceStorage.RootKind.Internal -> Icons.Default.PhoneAndroid
	DeviceStorage.RootKind.Downloads -> Icons.Default.Download
	DeviceStorage.RootKind.Removable -> Icons.Default.Usb
}

@Composable
private fun BrowserHeader(title: String, path: String?, onUp: () -> Unit, onClose: () -> Unit) {
	Row(
		modifier = Modifier.fillMaxWidth().padding(horizontal = 4.dp, vertical = 4.dp),
		verticalAlignment = Alignment.CenterVertically
	) {
		IconButton(onClick = onUp, modifier = Modifier.dpadFocusIndicator()) {
			Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = stringResource(R.string.device_browser_up))
		}
		Column(modifier = Modifier.weight(1f).padding(horizontal = 8.dp)) {
			Text(title, style = MaterialTheme.typography.titleLarge, maxLines = 1, overflow = TextOverflow.Ellipsis)
			if (path != null) {
				Text(
					path,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant,
					maxLines = 1,
					overflow = TextOverflow.StartEllipsis
				)
			}
		}
		IconButton(onClick = onClose, modifier = Modifier.dpadFocusIndicator()) {
			Icon(Icons.Default.Close, contentDescription = stringResource(R.string.device_browser_close))
		}
	}
}

@Composable
private fun BrowserRows(
	rows: List<BrowserRow>,
	focusFile: File?,
	multiple: Boolean,
	selected: Set<File>,
	emptyText: String,
	onOpen: (File) -> Unit,
	onToggle: (File) -> Unit,
	onPick: (File) -> Unit
) {
	if (rows.isEmpty()) {
		Text(
			emptyText,
			modifier = Modifier.padding(24.dp),
			style = MaterialTheme.typography.bodyLarge,
			color = MaterialTheme.colorScheme.onSurfaceVariant
		)
		return
	}
	// Focus the first row of each listing (or the folder just left), so the D-pad starts
	// in the list rather than on the header buttons.
	val focusIndex = rows.indexOfFirst { it.file == focusFile }.coerceAtLeast(0)
	val listState = rememberLazyListState()
	val rowFocus = remember { FocusRequester() }
	LaunchedEffect(rows) {
		listState.scrollToItem(focusIndex)
		withFrameNanos { }
		rowFocus.requestFocus()
	}
	LazyColumn(state = listState, modifier = Modifier.fillMaxSize()) {
		itemsIndexed(rows, key = { _, row -> row.file.path }) { index, row ->
			val focusModifier = if (index == focusIndex) Modifier.focusRequester(rowFocus) else Modifier
			val rowModifier = when {
				row.isFolder -> Modifier.clickable { onOpen(row.file) }
				multiple -> Modifier.toggleable(
					value = row.file in selected,
					role = Role.Checkbox,
					onValueChange = { onToggle(row.file) }
				)
				else -> Modifier.clickable { onPick(row.file) }
			}
			Row(
				modifier = Modifier
					.fillMaxWidth()
					.then(focusModifier)
					.dpadFocusIndicator()
					.then(rowModifier)
					.padding(horizontal = 16.dp, vertical = 12.dp),
				verticalAlignment = Alignment.CenterVertically
			) {
				if (!row.isFolder && multiple) {
					Checkbox(checked = row.file in selected, onCheckedChange = null)
					Spacer(modifier = Modifier.width(8.dp))
				}
				Icon(row.icon, contentDescription = null, tint = MaterialTheme.colorScheme.primary, modifier = Modifier.size(24.dp))
				Spacer(modifier = Modifier.width(16.dp))
				Text(
					row.label,
					modifier = Modifier.weight(1f),
					style = MaterialTheme.typography.bodyLarge,
					maxLines = 2,
					overflow = TextOverflow.Ellipsis
				)
				if (row.isFolder) {
					Icon(
						Icons.AutoMirrored.Filled.KeyboardArrowRight,
						contentDescription = null,
						tint = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
			}
		}
	}
}
