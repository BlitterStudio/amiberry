package com.blitterstudio.amiberry.ui.screens

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.focusGroup
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
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.FolderOpen
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Storage
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SecondaryScrollableTabRow
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Tab
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.Button
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.Search
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.components.StoragePermissionBanner
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.viewmodel.FileManagerViewModel
import java.util.Locale
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FileManagerScreen(viewModel: FileManagerViewModel = viewModel()) {
	val context = LocalContext.current
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	var selectedTab by rememberSaveable { mutableIntStateOf(0) }

	val tabs = listOf(
		TabInfo(FileCategory.ROMS, stringResource(R.string.file_manager_tab_roms), Icons.Default.Memory),
		TabInfo(FileCategory.WHDLOAD_GAMES, stringResource(R.string.file_manager_tab_whdload), Icons.Default.SportsEsports),
		TabInfo(FileCategory.FLOPPIES, stringResource(R.string.file_manager_tab_floppies), Icons.Default.SaveAlt),
		TabInfo(FileCategory.HARD_DRIVES, stringResource(R.string.file_manager_tab_hard_drives), Icons.Default.Storage),
		TabInfo(FileCategory.CD_IMAGES, stringResource(R.string.file_manager_tab_cds), Icons.Default.Album)
	)
	val pathCopiedMessage = stringResource(R.string.msg_path_copied)
	val clipboardLabelPath = stringResource(R.string.clipboard_label_path)
	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)

	var searchQuery by rememberSaveable { mutableStateOf("") }

	val currentCategory = tabs[selectedTab].category
	val acceptedExtensions = remember(currentCategory) {
		FilePickerFilters.extensionLabelsFor(currentCategory).joinToString(", ")
	}
	val allFiles by when (currentCategory) {
		FileCategory.ROMS -> viewModel.roms
		FileCategory.FLOPPIES -> viewModel.floppies
		FileCategory.HARD_DRIVES -> viewModel.hardDrives
		FileCategory.CD_IMAGES -> viewModel.cdImages
		FileCategory.WHDLOAD_GAMES -> viewModel.whdloadGames
	}.collectAsState()

	val showSearch = shouldShowFileManagerSearch(allFiles.size)
	val effectiveSearchQuery = effectiveFileManagerSearchQuery(searchQuery, allFiles.size)

	LaunchedEffect(showSearch, searchQuery) {
		if (!showSearch && searchQuery.isNotEmpty()) {
			searchQuery = ""
		}
	}

	val files = filterFileManagerFiles(allFiles, effectiveSearchQuery)

	val importResult by viewModel.importResult.collectAsState()
	val isScanning by viewModel.isScanning.collectAsState()
	val isImporting by viewModel.isImporting.collectAsState()
	val showProgress = isScanning || isImporting
	val progressDescription = stringResource(
		if (isImporting) R.string.file_manager_importing_files else R.string.file_manager_scanning_files
	)
	val filePickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenMultipleDocuments()
	) { uris ->
		if (uris.isNotEmpty() && !isImporting) {
			viewModel.importFiles(uris, currentCategory)
		}
	}

	LaunchedEffect(importResult) {
		importResult?.let { msg ->
			snackbarHostState.showSnackbar(msg)
			viewModel.clearImportResult()
		}
	}

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.file_manager_title)) },
				actions = {
					IconButton(
						onClick = { viewModel.rescan() },
						enabled = !showProgress
					) {
						Icon(
							Icons.Default.Refresh,
							contentDescription = stringResource(R.string.file_manager_refresh)
						)
					}
				}
			)
		},
		floatingActionButton = {
			ExtendedFloatingActionButton(
				onClick = {
					if (isImporting) {
						return@ExtendedFloatingActionButton
					}
					filePickerLauncher.launch(FilePickerFilters.mimeTypesFor(currentCategory))
				},
				icon = {
					if (isImporting) {
						CircularProgressIndicator(
							modifier = Modifier.size(18.dp),
							strokeWidth = 2.dp
						)
					} else {
						Icon(Icons.Default.Add, contentDescription = null)
					}
				},
				text = {
					Text(
						stringResource(
							if (isImporting) R.string.action_importing else R.string.action_import
						)
					)
				}
			)
		}
	) { innerPadding ->
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
		) {
			StoragePermissionBanner(
				modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
				onMessage = { message ->
					scope.launch { snackbarHostState.showSnackbar(actionMessage(message)) }
				}
			)

			OutlinedCard(
				modifier = Modifier
					.fillMaxWidth()
					.padding(horizontal = 16.dp, vertical = 8.dp)
			) {
				Row(
					modifier = Modifier
						.fillMaxWidth()
						.padding(horizontal = 12.dp, vertical = 8.dp),
					verticalAlignment = Alignment.CenterVertically,
					horizontalArrangement = Arrangement.SpaceBetween
				) {
					Column(modifier = Modifier.weight(1f)) {
						Row(verticalAlignment = Alignment.CenterVertically) {
							Icon(
								Icons.Default.FolderOpen,
								contentDescription = null,
								modifier = Modifier.size(16.dp)
							)
							Spacer(modifier = Modifier.width(6.dp))
							Text(
								stringResource(R.string.file_manager_app_storage),
								style = MaterialTheme.typography.labelMedium
							)
						}
						Text(
							text = viewModel.getStoragePath(),
							style = MaterialTheme.typography.bodySmall,
							fontFamily = FontFamily.Monospace,
							color = MaterialTheme.colorScheme.onSurfaceVariant,
							maxLines = 1
						)
					}

					IconButton(
						onClick = {
							val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
							clipboard.setPrimaryClip(ClipData.newPlainText(clipboardLabelPath, viewModel.getStoragePath()))
							scope.launch { snackbarHostState.showSnackbar(pathCopiedMessage) }
						}
					) {
						Icon(
							Icons.Default.ContentCopy,
							contentDescription = stringResource(R.string.file_manager_copy_path),
							modifier = Modifier.size(18.dp)
						)
					}
				}
			}

			SecondaryScrollableTabRow(
				selectedTabIndex = selectedTab,
				modifier = Modifier
					.fillMaxWidth()
					.focusGroup()
			) {
				tabs.forEachIndexed { index, tab ->
					Tab(
						selected = selectedTab == index,
						onClick = { selectedTab = index; searchQuery = "" },
						text = { Text(tab.title) },
						icon = { Icon(tab.icon, contentDescription = tab.title, modifier = Modifier.size(18.dp)) }
					)
				}
			}

			if (showSearch) {
				OutlinedTextField(
					value = searchQuery,
					onValueChange = { searchQuery = it },
					modifier = Modifier
						.fillMaxWidth()
						.padding(horizontal = 16.dp, vertical = 4.dp),
					placeholder = { Text(stringResource(R.string.search_placeholder)) },
					leadingIcon = { Icon(Icons.Default.Search, contentDescription = null, modifier = Modifier.size(18.dp)) },
					trailingIcon = {
						if (searchQuery.isNotEmpty()) {
							IconButton(onClick = { searchQuery = "" }) {
								Icon(
									Icons.Default.Clear,
									contentDescription = stringResource(R.string.action_clear_search),
									modifier = Modifier.size(18.dp)
								)
							}
						}
					},
					singleLine = true
				)
			}

			if (showProgress) {
				LinearProgressIndicator(
					modifier = Modifier
						.fillMaxWidth()
						.semantics { contentDescription = progressDescription }
				)
			}

			if (files.isEmpty() && !isScanning) {
				val searchHasNoResults = effectiveSearchQuery.isNotBlank() && allFiles.isNotEmpty()
				Box(
					modifier = Modifier
						.fillMaxSize()
						.padding(32.dp),
					contentAlignment = Alignment.Center
				) {
					Column(horizontalAlignment = Alignment.CenterHorizontally) {
						Text(
							text = if (searchHasNoResults) {
								stringResource(R.string.file_manager_no_search_results)
							} else {
								stringResource(
									R.string.file_manager_no_files_found,
									tabs[selectedTab].title.lowercase(Locale.getDefault())
								)
							},
							style = MaterialTheme.typography.bodyLarge
						)
						Spacer(modifier = Modifier.height(8.dp))
						Text(
							text = if (searchHasNoResults) {
								stringResource(R.string.file_manager_search_help)
							} else {
								stringResource(
									R.string.file_manager_empty_help_with_extensions,
									viewModel.getStoragePath(),
									currentCategory.dirName,
									acceptedExtensions
								)
							},
							style = MaterialTheme.typography.bodySmall,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)
						Spacer(modifier = Modifier.height(24.dp))
						Button(
							onClick = {
								if (searchHasNoResults) {
									searchQuery = ""
								} else {
									filePickerLauncher.launch(FilePickerFilters.mimeTypesFor(currentCategory))
								}
							},
							enabled = searchHasNoResults || !isImporting
						) {
							Icon(
								if (searchHasNoResults) Icons.Default.Clear else Icons.Default.Add,
								contentDescription = null,
								modifier = Modifier.size(18.dp)
							)
							Spacer(modifier = Modifier.width(8.dp))
							Text(
								if (searchHasNoResults) {
									stringResource(R.string.action_clear_search)
								} else {
									stringResource(R.string.file_manager_import_category, tabs[selectedTab].title)
								}
							)
						}
					}
				}
			} else {
				LazyColumn(
					contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
					verticalArrangement = Arrangement.spacedBy(6.dp)
				) {
					items(files, key = { it.path }) { file ->
						FileItem(
							file = file,
							onDelete = { viewModel.deleteFile(file) }
						)
					}
				}
			}
		}
	}
}

@Composable
private fun FileItem(file: AmigaFile, onDelete: () -> Unit) {
	var showDeleteDialog by remember { mutableStateOf(false) }

	Card(modifier = Modifier.fillMaxWidth().dpadFocusIndicator()) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(start = 12.dp, top = 12.dp, bottom = 12.dp, end = 4.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			val icon = when (file.category) {
				FileCategory.ROMS -> Icons.Default.Memory
				FileCategory.FLOPPIES -> Icons.Default.SaveAlt
				FileCategory.HARD_DRIVES -> Icons.Default.Storage
				FileCategory.CD_IMAGES -> Icons.Default.Album
				FileCategory.WHDLOAD_GAMES -> Icons.Default.SportsEsports
			}
			Icon(
				icon,
				contentDescription = null,
				modifier = Modifier.size(28.dp),
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
			IconButton(onClick = { showDeleteDialog = true }) {
				Icon(
					Icons.Default.Delete,
					contentDescription = stringResource(R.string.action_delete_named, file.name),
					modifier = Modifier.size(20.dp),
					tint = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
	}

	if (showDeleteDialog) {
		AlertDialog(
			onDismissRequest = { showDeleteDialog = false },
			title = { Text(stringResource(R.string.file_manager_delete_title)) },
			text = { Text(stringResource(R.string.file_manager_delete_message, file.name)) },
			confirmButton = {
				TextButton(onClick = {
					onDelete()
					showDeleteDialog = false
				}) {
					Text(
						stringResource(R.string.action_delete),
						color = MaterialTheme.colorScheme.error
					)
				}
			},
			dismissButton = {
				TextButton(onClick = { showDeleteDialog = false }) {
					Text(stringResource(R.string.action_cancel))
				}
			}
		)
	}
}

private data class TabInfo(
	val category: FileCategory,
	val title: String,
	val icon: ImageVector
)

internal fun shouldShowFileManagerSearch(fileCount: Int): Boolean =
	fileCount > FILE_MANAGER_SEARCH_THRESHOLD

internal fun effectiveFileManagerSearchQuery(query: String, fileCount: Int): String =
	if (shouldShowFileManagerSearch(fileCount)) query else ""

internal fun filterFileManagerFiles(files: List<AmigaFile>, query: String): List<AmigaFile> =
	if (query.isBlank()) files else files.filter { it.name.contains(query, ignoreCase = true) }

private const val FILE_MANAGER_SEARCH_THRESHOLD = 5
