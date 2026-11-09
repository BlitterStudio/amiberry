package com.blitterstudio.amiberry.ui.screens.library

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.layout.heightIn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.ClearAll
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.FolderOpen
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PhoneAndroid
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Storage
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.InputMode
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalInputModeManager
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.disabled
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.DocumentPickerAvailability
import com.blitterstudio.amiberry.data.RecentLaunches
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.LocalContentEntryFocus
import com.blitterstudio.amiberry.ui.components.ImportPicker
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.isTvDevice
import org.json.JSONObject
import android.text.format.DateUtils

/**
 * A Media shelf: its files (or the last launches, on Recent), each playable at once or in
 * a saved machine; a folder shelf imports, Recent forgets. A file no longer there stays
 * listed on Recent, dimmed, so it can be taken off the list.
 */
@Composable
internal fun LibraryMediaShelfPage(
	modifier: Modifier,
	shelf: MediaShelf,
	query: String,
	floppies: List<AmigaFile>,
	cds: List<AmigaFile>,
	whdloadGames: List<AmigaFile>,
	hardDrives: List<AmigaFile>,
	recentItems: List<RecentLaunches.Item>,
	machines: List<ConfigInfo>,
	isScanning: Boolean,
	launchInProgress: Boolean,
	importInProgress: Boolean,
	importPicker: ImportPicker,
	onDelete: (AmigaFile) -> Unit,
	onQuickPlay: (AmigaFile) -> Unit,
	onPlayIn: (AmigaFile, ConfigInfo) -> Unit,
	onPlayRecent: (JSONObject) -> Unit,
	onRemoveRecent: (RecentLaunches.Item) -> Unit,
	onClearRecent: () -> Unit
) {
	val context = LocalContext.current
	val inputModeManager = LocalInputModeManager.current
	// Focus moves only for D-pad/keyboard users; a touch user's taps place no focus.
	fun requestDpadFocus(requester: FocusRequester) {
		if (inputModeManager.inputMode == InputMode.Keyboard) requester.requestFocus()
	}

	val scanningText = stringResource(R.string.file_manager_scanning_files)
	var actionsFor by remember { mutableStateOf<AmigaFile?>(null) }
	var recentActionsFor by remember { mutableStateOf<RecentLaunches.Item?>(null) }
	var playInFor by remember { mutableStateOf<AmigaFile?>(null) }
	var confirmDelete by remember { mutableStateOf<AmigaFile?>(null) }
	var confirmClearRecent by remember { mutableStateOf(false) }
	var showImportMenu by remember { mutableStateOf(false) }

	val allFiles = filesOf(shelf, floppies, cds, whdloadGames, hardDrives)
	val shownFiles = filterShelfFiles(allFiles, query)
	val shownRecent = filterRecentItems(recentItems, query)

	// Deleting a file takes its focused row away, which dropped D-pad focus: once the
	// deleted file has left the shelf, focus returns to the list, or to Import. A failed
	// delete leaves the file listed, so nothing moves.
	val listFocus = remember { FocusRequester() }
	val importFocus = remember { FocusRequester() }
	var refocusAfterDeleting by remember { mutableStateOf<String?>(null) }
	LaunchedEffect(allFiles, refocusAfterDeleting) {
		val deletedPath = refocusAfterDeleting ?: return@LaunchedEffect
		if (allFiles.any { it.path == deletedPath }) return@LaunchedEffect
		refocusAfterDeleting = null
		requestDpadFocus(if (shownFiles.isNotEmpty()) listFocus else importFocus)
	}
	// Removing an entry empties Recent: focus returns to Clear Recent's neighbour, Import
	// not being on that shelf.
	var refocusAfterRemoving by remember { mutableStateOf(false) }
	LaunchedEffect(recentItems, refocusAfterRemoving) {
		if (!refocusAfterRemoving) return@LaunchedEffect
		if (recentItems.isNotEmpty()) return@LaunchedEffect
		refocusAfterRemoving = false
		requestDpadFocus(listFocus)
	}

	LazyColumn(
		modifier = modifier
			.focusRequester(listFocus)
			.contentEntryFocus()
			.focusRestorer(),
		contentPadding = PaddingValues(start = 16.dp, top = 8.dp, end = 16.dp, bottom = 16.dp),
		verticalArrangement = Arrangement.spacedBy(4.dp)
	) {
		item(key = "header") {
			Row(
				modifier = Modifier
					.fillMaxWidth()
					.padding(bottom = 4.dp),
				verticalAlignment = Alignment.CenterVertically
			) {
				Text(
					text = when (shelf) {
						MediaShelf.Recent -> stringResource(R.string.library_media_recent_hint)
						else -> stringResource(
							R.string.library_media_folder_hint,
							stringResource(shelf.titleRes)
						)
					},
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant,
					modifier = Modifier.weight(1f)
				)
				if (shelf == MediaShelf.Recent) {
					if (recentItems.isNotEmpty()) {
						TextButton(
							onClick = { confirmClearRecent = true },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(Icons.Default.ClearAll, contentDescription = null, modifier = Modifier.size(18.dp))
							Spacer(modifier = Modifier.width(4.dp))
							Text(stringResource(R.string.action_clear_recent))
						}
					}
				} else {
					Box {
						val busy = importInProgress || isScanning
						FilledTonalButton(
							onClick = { if (!busy) showImportMenu = true },
							modifier = importFocusModifier(
								hasRows = allFiles.isNotEmpty(),
								listFocus = listFocus,
								importFocus = importFocus
							)
								.dpadFocusIndicator()
								.semantics { if (busy) disabled() }
						) {
							if (importInProgress) {
								CircularProgressIndicator(modifier = Modifier.size(18.dp), strokeWidth = 2.dp)
							} else {
								Icon(Icons.Default.Add, contentDescription = null)
							}
							Spacer(modifier = Modifier.width(8.dp))
							Text(stringResource(if (importInProgress) R.string.action_importing else R.string.action_import))
						}
						ImportMenu(
							expanded = showImportMenu,
							onDismiss = { showImportMenu = false },
							importPicker = importPicker,
							hasDocumentPicker = remember { DocumentPickerAvailability.canOpenDocuments(context) }
						)
					}
				}
			}
		}

		if (isScanning) {
			item(key = "scanning") {
				LinearProgressIndicator(
					modifier = Modifier
						.fillMaxWidth()
						.semantics { contentDescription = scanningText }
				)
			}
		}

		if (shelf == MediaShelf.Recent) {
			if (recentItems.isEmpty() && !isScanning) {
				item(key = "empty") {
					ShelfEmptyState(shelf = shelf)
				}
			} else if (shownRecent.isEmpty() && query.isNotBlank()) {
				item(key = "no-results") { NoSearchResults() }
			} else {
				items(shownRecent, key = { it.entry.toString() }) { item ->
					RecentRow(
						item = item,
						launchInProgress = launchInProgress,
						hasMachines = machines.isNotEmpty(),
						onPlay = { onPlayRecent(item.entry) },
						onActions = { recentActionsFor = item },
						onRemove = {
							refocusAfterRemoving = recentItems.size == 1
							onRemoveRecent(item)
						}
					)
				}
			}
		} else {
			if (allFiles.isEmpty() && !isScanning) {
				item(key = "empty") {
					ShelfEmptyState(shelf = shelf)
				}
			} else if (shownFiles.isEmpty() && query.isNotBlank()) {
				item(key = "no-results") { NoSearchResults() }
			} else {
				items(shownFiles, key = { it.path }) { file ->
					MediaRow(
						file = file,
						launchInProgress = launchInProgress,
						onPlay = { onQuickPlay(file) },
						onActions = { actionsFor = file }
					)
				}
			}
		}
	}

	actionsFor?.let { file ->
		ShelfActionsDialog(
			file = file,
			hasMachines = machines.isNotEmpty(),
			busy = launchInProgress || importInProgress || isScanning,
			onDismiss = { actionsFor = null },
			onAction = { action ->
				actionsFor = null
				when (action) {
					ShelfAction.Play -> onQuickPlay(file)
					ShelfAction.PlayIn -> playInFor = file
					ShelfAction.Delete -> confirmDelete = file
				}
			}
		)
	}

	recentActionsFor?.let { item ->
		RecentActionsDialog(
			item = item,
			launchInProgress = launchInProgress,
			hasMachines = machines.isNotEmpty(),
			onDismiss = { recentActionsFor = null },
			onPlay = {
				recentActionsFor = null
				onPlayRecent(item.entry)
			},
			onPlayIn = {
				recentActionsFor = null
				playInFor = recentMediaFor(item.entry)
			},
			onRemove = {
				recentActionsFor = null
				refocusAfterRemoving = recentItems.size == 1
				onRemoveRecent(item)
			}
		)
	}

	playInFor?.let { file ->
		PlayInDialog(
			file = file,
			machines = machines,
			onDismiss = { playInFor = null },
			onPick = { machine ->
				playInFor = null
				onPlayIn(file, machine)
			}
		)
	}

	confirmDelete?.let { file ->
		AlertDialog(
			onDismissRequest = { confirmDelete = null },
			title = { Text(stringResource(R.string.file_manager_delete_title)) },
			text = { Text(stringResource(R.string.file_manager_delete_message, file.name)) },
			confirmButton = {
				TextButton(
					onClick = {
						confirmDelete = null
						refocusAfterDeleting = file.path
						onDelete(file)
					},
					enabled = !importInProgress,
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_delete), color = MaterialTheme.colorScheme.error)
				}
			},
			dismissButton = {
				// Cancel takes focus, so an OK pressed at once deletes nothing.
				val cancelFocus = remember { FocusRequester() }
				LaunchedEffect(Unit) { cancelFocus.requestFocus() }
				TextButton(
					onClick = { confirmDelete = null },
					modifier = Modifier.focusRequester(cancelFocus).dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_cancel))
				}
			}
		)
	}

	if (confirmClearRecent) {
		AlertDialog(
			onDismissRequest = { confirmClearRecent = false },
			title = { Text(stringResource(R.string.dialog_clear_recent_title)) },
			text = { Text(stringResource(R.string.dialog_clear_recent_message)) },
			confirmButton = {
				TextButton(
					onClick = {
						confirmClearRecent = false
						refocusAfterRemoving = true
						onClearRecent()
					},
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_clear), color = MaterialTheme.colorScheme.error)
				}
			},
			dismissButton = {
				TextButton(
					onClick = { confirmClearRecent = false },
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_cancel))
				}
			}
		)
	}
}


/**
 * The disk or CD a Recent entry played, for Play In; null where Play In does not apply.
 * Quick Start and Play In entries keep their media in df0/df1/cd (a Play In's path names
 * its machine, not a file to play), and WHDLoad archives are left to WHDBooter, which
 * decides their hardware.
 */
internal fun recentMediaFor(entry: JSONObject): AmigaFile? {
	if (RecentLaunches.Type.of(entry) !in setOf(RecentLaunches.Type.QuickStart, RecentLaunches.Type.PlayIn)) {
		return null
	}
	val path = listOf("df0", "df1", "cd").firstNotNullOfOrNull { key ->
		entry.optString(key).takeIf { it.isNotBlank() }
	} ?: return null
	val category = FileCategory.fromExtension(path.substringAfterLast('.'))
		?: return null
	// Only the drives a machine takes a file in.
	if (category != FileCategory.FLOPPIES && category != FileCategory.CD_IMAGES) return null
	return AmigaFile(
		path = path,
		name = path.substringAfterLast('/').substringAfterLast('\\'),
		extension = path.substringAfterLast('.').lowercase(),
		size = 0,
		lastModified = 0,
		category = category
	)
}


/** The Import button's focus wiring: Down enters the rows, Up returns to them. */
private fun importFocusModifier(
	hasRows: Boolean,
	listFocus: FocusRequester,
	importFocus: FocusRequester
): Modifier = Modifier
	.then(
		if (hasRows) {
			Modifier.focusProperties { down = listFocus }
		} else {
			Modifier
		}
	)
	.focusRequester(importFocus)

/** The files a folder shelf lists. */
private fun filesOf(
	shelf: MediaShelf,
	floppies: List<AmigaFile>,
	cds: List<AmigaFile>,
	whdloadGames: List<AmigaFile>,
	hardDrives: List<AmigaFile>
): List<AmigaFile> = when (shelf.category) {
	FileCategory.FLOPPIES -> floppies
	FileCategory.CD_IMAGES -> cds
	FileCategory.WHDLOAD_GAMES -> whdloadGames
	FileCategory.HARD_DRIVES -> hardDrives
	else -> emptyList()
}

/** The shelf's files whose name matches every word of [query]. */
internal fun filterShelfFiles(files: List<AmigaFile>, query: String): List<AmigaFile> {
	val words = query.trim().lowercase().split(Regex("\\s+")).filter { it.isNotEmpty() }
	if (words.isEmpty()) return files
	return files.filter { file -> words.all { it in file.name.lowercase() } }
}

/** The Recent items whose title or detail matches every word of [query]. */
internal fun filterRecentItems(items: List<RecentLaunches.Item>, query: String): List<RecentLaunches.Item> {
	val words = query.trim().lowercase().split(Regex("\\s+")).filter { it.isNotEmpty() }
	if (words.isEmpty()) return items
	return items.filter { item ->
		words.all { it in item.details.title.lowercase() || it in item.details.detail.lowercase() }
	}
}

@Composable
private fun NoSearchResults() {
	Box(
		modifier = Modifier
			.fillMaxWidth()
			.padding(horizontal = 16.dp, vertical = 32.dp),
		contentAlignment = Alignment.Center
	) {
		Column(horizontalAlignment = Alignment.CenterHorizontally) {
			Text(stringResource(R.string.file_manager_no_search_results), style = MaterialTheme.typography.bodyLarge)
			Spacer(modifier = Modifier.height(8.dp))
			Text(
				stringResource(R.string.file_manager_search_help),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
		}
	}
}

/** A shelf with nothing on it: where its files go. */
@Composable
private fun ShelfEmptyState(shelf: MediaShelf) {
	Box(
		modifier = Modifier
			.fillMaxWidth()
			.padding(horizontal = 16.dp, vertical = 32.dp),
		contentAlignment = Alignment.Center
	) {
		Column(horizontalAlignment = Alignment.CenterHorizontally) {
			Icon(
				shelf.icon,
				contentDescription = null,
				modifier = Modifier.size(32.dp),
				tint = MaterialTheme.colorScheme.onSurfaceVariant
			)
			Spacer(modifier = Modifier.height(8.dp))
			Text(
				stringResource(
					when (shelf) {
						MediaShelf.Recent -> R.string.library_media_no_recent_title
						MediaShelf.FloppyDisks -> R.string.library_media_no_floppies_title
						MediaShelf.CompactDiscs -> R.string.library_media_no_cds_title
						MediaShelf.WhdLoad -> R.string.library_media_no_whdload_title
						MediaShelf.HardDrives -> R.string.library_media_no_hard_drives_title
					}
				),
				style = MaterialTheme.typography.titleSmall
			)
			Spacer(modifier = Modifier.height(4.dp))
			Text(
				stringResource(
					when (shelf) {
						MediaShelf.Recent -> R.string.library_media_no_recent_message
						MediaShelf.FloppyDisks -> R.string.library_media_no_floppies_message
						MediaShelf.CompactDiscs -> R.string.library_media_no_cds_message
						MediaShelf.WhdLoad -> R.string.library_media_no_whdload_message
						MediaShelf.HardDrives -> R.string.library_media_no_hard_drives_message
					}
				),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
		}
	}
}

/** What a shelf row offers. */
private enum class ShelfAction { Play, PlayIn, Delete }

/** A file on a shelf: its icon, name and size, a play button, its actions one tap away. */
@Composable
private fun MediaRow(
	file: AmigaFile,
	launchInProgress: Boolean,
	onPlay: () -> Unit,
	onActions: () -> Unit
) {
	val playable = file.category in PLAYABLE_CATEGORIES
	Card(modifier = Modifier.fillMaxWidth()) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.dpadFocusIndicator()
				.clickable(onClickLabel = stringResource(R.string.action_show_actions), onClick = onActions)
				.padding(start = 12.dp, top = 10.dp, bottom = 10.dp, end = 8.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			Icon(
				categoryIcon(file.category),
				contentDescription = null,
				modifier = Modifier.size(24.dp),
				tint = MaterialTheme.colorScheme.primary
			)
			Spacer(modifier = Modifier.width(12.dp))
			Column(modifier = Modifier.weight(1f)) {
				Text(text = file.name, style = MaterialTheme.typography.bodyMedium)
				Text(
					text = file.sizeDisplay,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
			if (playable) {
				IconButton(
					onClick = onPlay,
					enabled = !launchInProgress,
					modifier = Modifier.dpadFocusIndicator()
				) {
					Icon(
						Icons.Default.PlayArrow,
						contentDescription = stringResource(R.string.action_launch_named, file.name)
					)
				}
			} else {
				Icon(
					Icons.AutoMirrored.Filled.KeyboardArrowRight,
					contentDescription = null,
					tint = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
	}
}

/** One of the last launches: what it was, when, and whether it can run again. */
@Composable
private fun RecentRow(
	item: RecentLaunches.Item,
	launchInProgress: Boolean,
	hasMachines: Boolean,
	onPlay: () -> Unit,
	onActions: () -> Unit,
	onRemove: () -> Unit
) {
	val launchable = item.status == RecentLaunches.Status.Available
	val statusText = when (item.status) {
		RecentLaunches.Status.Available -> null
		RecentLaunches.Status.MissingFiles -> stringResource(R.string.recent_status_missing)
		RecentLaunches.Status.ModelUnavailable -> stringResource(R.string.recent_status_model_unavailable)
	}
	val justNow = stringResource(R.string.recent_launched_just_now)
	val launchedAt = item.details.launchedAtMs?.let { time ->
		val now = System.currentTimeMillis()
		if (now - time < DateUtils.MINUTE_IN_MILLIS) {
			justNow
		} else {
			DateUtils.getRelativeTimeSpanString(time, now, DateUtils.MINUTE_IN_MILLIS).toString()
		}
	}
	val contentAlpha = if (launchable) 1f else 0.38f
	Card(modifier = Modifier.fillMaxWidth()) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.dpadFocusIndicator()
				.clickable(
					onClickLabel = stringResource(if (launchable) R.string.action_show_actions else R.string.library_recent_remove),
					onClick = { if (!launchable && !hasMachines) onRemove() else onActions() }
				)
				.padding(start = 12.dp, top = 10.dp, bottom = 10.dp, end = 8.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			Column(modifier = Modifier.weight(1f)) {
				Text(
					text = item.details.title,
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onSurface.copy(alpha = contentAlpha)
				)
				Text(
					text = listOfNotNull(
						stringResource(
							R.string.recent_entry_summary,
							stringResource(item.details.type.labelRes),
							item.details.detail
						),
						launchedAt
					).joinToString(" · "),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = contentAlpha)
				)
				if (statusText != null) {
					Text(
						statusText,
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
			}
			if (launchable) {
				IconButton(
					onClick = onPlay,
					enabled = !launchInProgress,
					modifier = Modifier.dpadFocusIndicator()
				) {
					Icon(
						Icons.Default.PlayArrow,
						contentDescription = stringResource(R.string.action_launch_named, item.details.title)
					)
				}
			} else {
				IconButton(onClick = onRemove, modifier = Modifier.dpadFocusIndicator()) {
					Icon(
						Icons.Default.Delete,
						contentDescription = stringResource(R.string.library_recent_remove_named, item.details.title)
					)
				}
			}
		}
	}
}

/** The file actions a shelf offers: play, play in a saved machine, delete. */
@Composable
private fun ShelfActionsDialog(
	file: AmigaFile,
	hasMachines: Boolean,
	busy: Boolean,
	onDismiss: () -> Unit,
	onAction: (ShelfAction) -> Unit
) {
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(file.name) },
		text = {
			Column {
				val firstFocus = remember { FocusRequester() }
				LaunchedEffect(Unit) { firstFocus.requestFocus() }
				var index = 0
				if (file.category in PLAYABLE_CATEGORIES) {
					ShelfActionRow(
						icon = Icons.Default.PlayArrow,
						label = stringResource(R.string.library_action_play),
						enabled = !busy,
						focus = if (index == 0) firstFocus else null,
						onClick = { onAction(ShelfAction.Play) }
					)
					index++
					if (file.category != FileCategory.WHDLOAD_GAMES && hasMachines) {
						ShelfActionRow(
							icon = Icons.Default.SaveAlt,
							label = stringResource(R.string.library_action_play_in),
							enabled = !busy,
							focus = null,
							onClick = { onAction(ShelfAction.PlayIn) }
						)
						index++
					}
				}
				ShelfActionRow(
					icon = Icons.Default.Delete,
					label = stringResource(R.string.action_delete),
					enabled = !busy,
					focus = if (index == 0) firstFocus else null,
					onClick = { onAction(ShelfAction.Delete) },
					destructive = true
				)
			}
		},
		confirmButton = {},
		dismissButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}

/** A Recent row's actions: play it again, play it in a machine, take it off the list. */
@Composable
private fun RecentActionsDialog(
	item: RecentLaunches.Item,
	launchInProgress: Boolean,
	hasMachines: Boolean,
	onDismiss: () -> Unit,
	onPlay: () -> Unit,
	onPlayIn: () -> Unit,
	onRemove: () -> Unit
) {
	val launchable = item.status == RecentLaunches.Status.Available
	val file = recentMediaFor(item.entry)
	val playableIn = launchable && file != null
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(item.details.title) },
		text = {
			Column {
				val firstFocus = remember { FocusRequester() }
				LaunchedEffect(Unit) { firstFocus.requestFocus() }
				if (launchable) {
					ShelfActionRow(
						icon = Icons.Default.PlayArrow,
						label = stringResource(R.string.library_action_play),
						enabled = !launchInProgress,
						focus = firstFocus,
						onClick = onPlay
					)
					if (playableIn && hasMachines) {
						ShelfActionRow(
							icon = Icons.Default.SaveAlt,
							label = stringResource(R.string.library_action_play_in),
							enabled = !launchInProgress,
							focus = null,
							onClick = onPlayIn
						)
					}
				}
				ShelfActionRow(
					icon = Icons.Default.Delete,
					label = stringResource(R.string.library_recent_remove),
					enabled = true,
					focus = if (!launchable) firstFocus else null,
					onClick = onRemove
				)
			}
		},
		confirmButton = {},
		dismissButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}

/** Asks which saved machine a file plays in. */
@Composable
private fun PlayInDialog(
	file: AmigaFile,
	machines: List<ConfigInfo>,
	onDismiss: () -> Unit,
	onPick: (ConfigInfo) -> Unit
) {
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.library_play_in_title, file.name)) },
		text = {
			val firstFocus = remember { FocusRequester() }
			LaunchedEffect(Unit) { firstFocus.requestFocus() }
			// Bounded and lazy: every machine stays reachable however many there are.
			LazyColumn(
				modifier = Modifier.heightIn(max = 320.dp),
				verticalArrangement = Arrangement.spacedBy(4.dp)
			) {
				itemsIndexed(machines, key = { _, machine -> machine.path }) { index, machine ->
					TextButton(
						onClick = { onPick(machine) },
						modifier = Modifier
							.fillMaxWidth()
							.then(if (index == 0) Modifier.focusRequester(firstFocus) else Modifier)
							.dpadFocusIndicator()
					) {
						Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(20.dp))
						Spacer(modifier = Modifier.width(12.dp))
						Text(machine.name, modifier = Modifier.weight(1f))
					}
				}
			}
		},
		confirmButton = {},
		dismissButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}

@Composable
private fun ShelfActionRow(
	icon: ImageVector,
	label: String,
	enabled: Boolean,
	focus: FocusRequester?,
	onClick: () -> Unit,
	destructive: Boolean = false
) {
	TextButton(
		onClick = onClick,
		enabled = enabled,
		modifier = Modifier
			.fillMaxWidth()
			.then(if (focus != null) Modifier.focusRequester(focus) else Modifier)
			.dpadFocusIndicator()
	) {
		Icon(
			icon,
			contentDescription = null,
			modifier = Modifier.size(20.dp),
			tint = if (destructive) MaterialTheme.colorScheme.error else androidx.compose.material3.LocalContentColor.current
		)
		Spacer(modifier = Modifier.width(12.dp))
		Text(
			label,
			modifier = Modifier.weight(1f),
			color = if (destructive) MaterialTheme.colorScheme.error else androidx.compose.material3.LocalContentColor.current
		)
	}
}

/** The shelf's import menu: files, a folder, or the device's storage. */
@Composable
private fun ImportMenu(
	expanded: Boolean,
	onDismiss: () -> Unit,
	importPicker: ImportPicker,
	hasDocumentPicker: Boolean
) {
	DropdownMenu(expanded = expanded, onDismissRequest = onDismiss) {
		DropdownMenuItem(
			text = { Text(stringResource(R.string.action_import_files)) },
			modifier = Modifier.dpadFocusIndicator(),
			leadingIcon = { Icon(Icons.Default.Add, contentDescription = null) },
			onClick = {
				onDismiss()
				importPicker.pickFiles()
			}
		)
		DropdownMenuItem(
			text = { Text(stringResource(R.string.action_import_folder)) },
			modifier = Modifier.dpadFocusIndicator(),
			leadingIcon = { Icon(Icons.Default.FolderOpen, contentDescription = null) },
			onClick = {
				onDismiss()
				importPicker.pickFolder()
			}
		)
		if (hasDocumentPicker) {
			DropdownMenuItem(
				text = { Text(stringResource(R.string.file_manager_browse_device)) },
				modifier = Modifier.dpadFocusIndicator(),
				leadingIcon = { Icon(Icons.Default.PhoneAndroid, contentDescription = null) },
				onClick = {
					onDismiss()
					importPicker.browseDevice()
				}
			)
		}
	}
}

/** The kinds a shelf plays at once (hard drives only store). */
private val PLAYABLE_CATEGORIES = setOf(FileCategory.FLOPPIES, FileCategory.CD_IMAGES, FileCategory.WHDLOAD_GAMES)

private fun categoryIcon(category: FileCategory): ImageVector = when (category) {
	FileCategory.ROMS -> Icons.Default.Memory
	FileCategory.FLOPPIES -> Icons.Default.SaveAlt
	FileCategory.HARD_DRIVES -> Icons.Default.Storage
	FileCategory.CD_IMAGES -> Icons.Default.Album
	FileCategory.WHDLOAD_GAMES -> Icons.Default.SportsEsports
}
