package com.blitterstudio.amiberry.ui.screens

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import androidx.activity.ComponentActivity
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyListScope
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.FolderOpen
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PhoneAndroid
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Storage
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.VerticalDivider
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.InputMode
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalInputModeManager
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.disabled
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.graphics.Color
import androidx.compose.foundation.clickable
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.data.DocumentPickerAvailability
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.StoragePermissionActions
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.ui.components.SidePaneEntry
import com.blitterstudio.amiberry.ui.components.SidePaneList
import com.blitterstudio.amiberry.ui.components.StoragePermissionBanner
import com.blitterstudio.amiberry.ui.components.rememberImportPicker
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.hasFullStorageAccess
import com.blitterstudio.amiberry.ui.isTvDevice
import com.blitterstudio.amiberry.ui.launchGuarded
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.FileManagerViewModel
import com.blitterstudio.amiberry.ui.viewmodel.QuickStartViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.ui.windowWidthClass
import kotlinx.coroutines.launch
import java.util.Locale

@OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
@Composable
fun FileManagerScreen(viewModel: FileManagerViewModel = viewModel()) {
	val context = LocalContext.current
	val activity = context.findActivity() as ComponentActivity
	val settingsViewModel: SettingsViewModel = viewModel(activity)
	val quickStartViewModel: QuickStartViewModel = viewModel(activity)
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	val launchGuard = rememberLaunchInFlightGuard()
	val isTv = isTvDevice()
	val inputModeManager = LocalInputModeManager.current
	var selectedTab by rememberSaveable { mutableIntStateOf(0) }
	var showImportMenu by remember { mutableStateOf(false) }
	var showOverflowMenu by remember { mutableStateOf(false) }
	var showStorageLocation by remember { mutableStateOf(false) }
	var actionsFor by remember { mutableStateOf<AmigaFile?>(null) }
	var confirmDelete by remember { mutableStateOf<AmigaFile?>(null) }

	val tabs = listOf(
		TabInfo(FileCategory.ROMS, stringResource(R.string.file_manager_tab_roms), Icons.Default.Memory),
		TabInfo(FileCategory.WHDLOAD_GAMES, stringResource(R.string.file_manager_tab_whdload), Icons.Default.SportsEsports),
		TabInfo(FileCategory.FLOPPIES, stringResource(R.string.file_manager_tab_floppies), Icons.Default.SaveAlt),
		TabInfo(FileCategory.HARD_DRIVES, stringResource(R.string.file_manager_tab_hard_drives), Icons.Default.Storage),
		TabInfo(FileCategory.CD_IMAGES, stringResource(R.string.file_manager_tab_cds), Icons.Default.Album)
	)
	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)

	var searchQuery by rememberSaveable { mutableStateOf("") }

	val currentCategory = tabs[selectedTab].category
	val acceptedExtensions = remember(currentCategory) {
		FilePickerFilters.extensionLabelsFor(currentCategory).joinToString(", ")
	}
	val categoryFlows = listOf(
		viewModel.roms,
		viewModel.whdloadGames,
		viewModel.floppies,
		viewModel.hardDrives,
		viewModel.cdImages
	)
	val allFiles by categoryFlows[selectedTab].collectAsState()

	val showSearch = shouldShowFileManagerSearch(allFiles.size)
	val effectiveSearchQuery = effectiveFileManagerSearchQuery(searchQuery, allFiles.size)

	LaunchedEffect(showSearch, searchQuery) {
		if (!showSearch && searchQuery.isNotEmpty()) {
			searchQuery = ""
		}
	}

	val files = filterFileManagerFiles(allFiles, effectiveSearchQuery)
	// Rows open their actions, except the built-in AROS, which has none.
	val fileListFocus = remember { FocusRequester() }
	val listHasFocusableRows = remember(files) { files.any { hasFileActions(it) } }
	// Focus moves only for D-pad/keyboard users; a touch user's taps place no focus.
	fun requestDpadFocus(requester: FocusRequester) {
		if (inputModeManager.inputMode == InputMode.Keyboard) requester.requestFocus()
	}
	// Deleting a file takes its focused row away, which dropped D-pad focus to the
	// navigation rail: once the deleted file has left this category, focus returns to the
	// list, or to Import. A failed delete leaves the file listed, so nothing moves. Choosing
	// a tab or editing the search forgets the request (cleared where the user acts: the
	// search also empties itself when a delete leaves too few files to search).
	val importFocus = remember { FocusRequester() }
	var refocusAfterDeleting by remember { mutableStateOf<String?>(null) }
	LaunchedEffect(allFiles, refocusAfterDeleting) {
		val deletedPath = refocusAfterDeleting ?: return@LaunchedEffect
		if (allFiles.any { it.path == deletedPath }) return@LaunchedEffect
		refocusAfterDeleting = null
		requestDpadFocus(if (listHasFocusableRows) fileListFocus else importFocus)
	}

	// The access card leaving (Not now, or access granted) takes its focused button away.
	val preferences = remember { AppPreferences.getInstance(context) }
	val accessCardVisible = StoragePermissionActions.shouldShowBanner(
		hasFullStorageAccess(),
		preferences.storageAccessCardDismissed.value
	)
	var accessCardWasVisible by remember { mutableStateOf(accessCardVisible) }
	LaunchedEffect(accessCardVisible) {
		if (accessCardWasVisible && !accessCardVisible) requestDpadFocus(importFocus)
		accessCardWasVisible = accessCardVisible
	}

	val importResult by viewModel.importResult.collectAsState()
	val isScanning by viewModel.isScanning.collectAsState()
	val isImporting by viewModel.isImporting.collectAsState()
	val showProgress = isScanning || isImporting
	val progressDescription = stringResource(
		if (isImporting) R.string.file_manager_importing_files else R.string.file_manager_scanning_files
	)
	// Imports are sorted by extension, so the picker offers every supported kind of file.
	val importPicker = rememberImportPicker(
		mimeTypes = arrayOf("*/*"),
		extensions = FileCategory.allExtensions,
		multiple = true,
		onFolder = { folder -> if (!showProgress) viewModel.importFolder(folder, currentCategory) }
	) { uris ->
		if (uris.isNotEmpty() && !showProgress) {
			viewModel.importFiles(uris, currentCategory)
		}
	}
	// Without a document picker (Google TV) Import files already opens the browser.
	val hasDocumentPicker = remember { DocumentPickerAvailability.canOpenDocuments(context) }
	val openImportPicker = {
		showImportMenu = true
	}

	LaunchedEffect(importResult) {
		importResult?.let { msg ->
			snackbarHostState.showSnackbar(msg)
			viewModel.clearImportResult()
		}
	}
	// Every file of an import went to another category: show that tab.
	val revealCategory by viewModel.revealCategory.collectAsState()
	LaunchedEffect(revealCategory) {
		val category = revealCategory ?: return@LaunchedEffect
		selectedTab = tabs.indexOfFirst { it.category == category }.coerceAtLeast(0)
		searchQuery = ""
		viewModel.clearRevealCategory()
	}

	val widthClass = windowWidthClass()
	// A phone held sideways: a short top bar, and Import in it rather than a FAB over the list.
	val compactHeight = LocalConfiguration.current.screenHeightDp < COMPACT_HEIGHT_DP
	// The FAB is for phones held upright; wide windows and TVs put Import in the top bar,
	// where it never covers the list's last row.
	val importInTopBar = compactHeight || widthClass.showsSideList

	val importButton: @Composable (Modifier) -> Unit = { modifier ->
		Box {
			// The list sits below the top-bar button and above the FAB; the 2D search
			// skipped over the list (a focus group) to the tabs or the rail.
			val importModifier = modifier
				.then(
					when {
						!listHasFocusableRows -> Modifier
						importInTopBar -> Modifier.focusProperties { down = fileListFocus }
						else -> Modifier.focusProperties { up = fileListFocus }
					}
				)
				.focusRequester(importFocus)
				.dpadFocusIndicator()
				.semantics { if (showProgress) disabled() }
			val onImport: () -> Unit = {
				if (!showProgress) openImportPicker()
			}
			val icon: @Composable () -> Unit = {
				if (isImporting) {
					CircularProgressIndicator(
						modifier = Modifier.size(18.dp),
						strokeWidth = 2.dp
					)
				} else {
					Icon(Icons.Default.Add, contentDescription = null)
				}
			}
			val label = stringResource(if (isImporting) R.string.action_importing else R.string.action_import)
			if (importInTopBar) {
				FilledTonalButton(onClick = onImport, modifier = importModifier) {
					icon()
					Spacer(modifier = Modifier.width(8.dp))
					Text(label)
				}
			} else {
				ExtendedFloatingActionButton(
					onClick = onImport,
					modifier = importModifier,
					icon = icon,
					text = { Text(label) }
				)
			}
			DropdownMenu(
				expanded = showImportMenu,
				onDismissRequest = { showImportMenu = false }
			) {
				DropdownMenuItem(
					text = { Text(stringResource(R.string.action_import_files)) },
					leadingIcon = { Icon(Icons.Default.Add, contentDescription = null) },
					onClick = {
						showImportMenu = false
						importPicker.pickFiles()
					}
				)
				DropdownMenuItem(
					text = { Text(stringResource(R.string.action_import_folder)) },
					leadingIcon = { Icon(Icons.Default.FolderOpen, contentDescription = null) },
					onClick = {
						showImportMenu = false
						importPicker.pickFolder()
					}
				)
				if (hasDocumentPicker) {
					DropdownMenuItem(
						text = { Text(stringResource(R.string.file_manager_browse_device)) },
						leadingIcon = { Icon(Icons.Default.PhoneAndroid, contentDescription = null) },
						onClick = {
							showImportMenu = false
							importPicker.browseDevice()
						}
					)
				}
			}
		}
	}

	// The app shell's Scaffold already pads every screen for the system bars; insetting
	// again left an empty band above the title (and less room in landscape).
	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		contentWindowInsets = WindowInsets(0),
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.file_manager_title)) },
				expandedHeight = if (compactHeight) COMPACT_TOP_BAR_HEIGHT else TopAppBarDefaults.TopAppBarExpandedHeight,
				windowInsets = WindowInsets(0),
				actions = {
					if (importInTopBar) importButton(Modifier.padding(end = 4.dp))
					IconButton(
						onClick = { viewModel.rescan() },
						enabled = !showProgress,
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(
							Icons.Default.Refresh,
							contentDescription = stringResource(R.string.file_manager_refresh)
						)
					}
					Box {
						IconButton(
							onClick = { showOverflowMenu = true },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(Icons.Default.MoreVert, contentDescription = stringResource(R.string.more_options))
						}
						DropdownMenu(expanded = showOverflowMenu, onDismissRequest = { showOverflowMenu = false }) {
							DropdownMenuItem(
								text = { Text(stringResource(R.string.file_manager_storage_location)) },
								leadingIcon = { Icon(Icons.Default.Storage, contentDescription = null) },
								onClick = {
									showOverflowMenu = false
									showStorageLocation = true
								}
							)
						}
					}
				}
			)
		},
		floatingActionButton = {
			if (!importInTopBar) importButton(Modifier)
		}
	) { innerPadding ->
		val selectCategory: (Int) -> Unit = { index ->
			selectedTab = index
			searchQuery = ""
			refocusAfterDeleting = null
		}
		val accessCard: LazyListScope.() -> Unit = {
			if (accessCardVisible) {
				item(key = "access-card") {
					StoragePermissionBanner(
						modifier = Modifier.padding(vertical = 4.dp),
						onGrant = importPicker::grantAccessThenBrowse,
						onMessage = { message ->
							scope.launch { snackbarHostState.showSnackbar(actionMessage(message)) }
						}
					)
				}
			}
		}
		val emptyHelp = if (isTv) {
			stringResource(R.string.file_manager_empty_help_tv, acceptedExtensions)
		} else {
			stringResource(R.string.file_manager_empty_help_touch, acceptedExtensions)
		}
		// One scrolling list per page, so that in a short window the card and category
		// chips scroll away with the files rather than leaving no room for them.
		val categoryList: @Composable (Modifier, LazyListScope.() -> Unit) -> Unit = { modifier, header ->
			FileCategoryList(
				modifier = modifier,
				header = header,
				files = files,
				allFiles = allFiles,
				showSearch = showSearch,
				searchQuery = searchQuery,
				effectiveSearchQuery = effectiveSearchQuery,
				onSearchQueryChange = { searchQuery = it; refocusAfterDeleting = null },
				showProgress = showProgress,
				isScanning = isScanning,
				progressDescription = progressDescription,
				categoryTitle = tabs[selectedTab].title,
				emptyHelp = emptyHelp,
				onImport = openImportPicker,
				fileListFocus = fileListFocus,
				bottomPadding = if (importInTopBar) 16.dp else FAB_CLEARANCE,
				onOpenActions = { file -> actionsFor = file }
			)
		}

		if (widthClass.showsSideList) {
			// Wide windows list the categories down the left with their file counts.
			val counts = categoryFlows.map { it.collectAsState().value.size }
			Row(modifier = Modifier.fillMaxSize().padding(innerPadding)) {
				SidePaneList(
					entries = tabs.mapIndexed { index, tab ->
						SidePaneEntry(tab.title, icon = tab.icon, badge = counts[index].toString())
					},
					selectedIndex = selectedTab,
					onSelect = selectCategory,
					// Right from the rail enters the categories, not the top bar's Import.
					modifier = Modifier.contentEntryFocus()
				)
				VerticalDivider()
				categoryList(Modifier.weight(1f).fillMaxHeight(), accessCard)
			}
		} else {
			categoryList(Modifier.fillMaxSize().padding(innerPadding)) {
				accessCard()
				stickyHeader(key = "categories") {
					Surface(modifier = Modifier.fillMaxWidth(), color = MaterialTheme.colorScheme.surface) {
						CategoryChips(tabs = tabs, selectedIndex = selectedTab, onSelect = selectCategory)
					}
				}
			}
		}
	}

	actionsFor?.let { file ->
		FileActionsDialog(
			file = file,
			busy = showProgress || launchGuard.isLaunching,
			onDismiss = { actionsFor = null },
			onAction = { action ->
				actionsFor = null
				when (action) {
					FileAction.InsertDf0AndStart -> scope.launchGuarded(launchGuard) {
						val roms = FileRepository.getInstance(context).roms.value
						val started = try {
							FileLaunch.insertDf0AndStart(context, file, roms, settingsViewModel, quickStartViewModel)
						} catch (_: Exception) {
							snackbarHostState.showSnackbar(context.getString(R.string.quick_start_config_write_failed))
							return@launchGuarded
						}
						if (!started) {
							snackbarHostState.showSnackbar(context.getString(R.string.quick_start_no_compatible_models_message))
						}
					}
					FileAction.InsertDf1 -> {
						FileLaunch.insertDf1(file, settingsViewModel, quickStartViewModel)
						scope.launch {
							snackbarHostState.showSnackbar(context.getString(R.string.file_manager_inserted_df1, file.name))
						}
					}
					FileAction.LaunchWhdload -> scope.launchGuarded(launchGuard) {
						try {
							FileLaunch.launchWhdload(context, file, settingsViewModel, quickStartViewModel)
						} catch (_: Exception) {
							snackbarHostState.showSnackbar(context.getString(R.string.quick_start_config_write_failed))
						}
					}
					FileAction.Delete -> confirmDelete = file
				}
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
						viewModel.deleteFile(file)
					},
					enabled = !showProgress,
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(
						stringResource(R.string.action_delete),
						color = MaterialTheme.colorScheme.error
					)
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

	if (showStorageLocation) {
		StorageLocationDialog(
			path = viewModel.getStoragePath(),
			canCopy = !isTv,
			onCopied = {
				scope.launch { snackbarHostState.showSnackbar(context.getString(R.string.msg_path_copied)) }
			},
			onDismiss = { showStorageLocation = false }
		)
	}
}

/** The categories as chips that wrap, so none hides off the edge of a narrow screen. */
@Composable
private fun CategoryChips(tabs: List<TabInfo>, selectedIndex: Int, onSelect: (Int) -> Unit) {
	FlowRow(
		modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
		horizontalArrangement = Arrangement.spacedBy(8.dp)
	) {
		tabs.forEachIndexed { index, tab ->
			FilterChip(
				selected = index == selectedIndex,
				onClick = { onSelect(index) },
				label = { Text(tab.title) },
				leadingIcon = { Icon(tab.icon, contentDescription = null, modifier = Modifier.size(18.dp)) },
				// Right from the rail enters at the selected category, not the top bar.
				modifier = Modifier
					.then(if (index == selectedIndex) Modifier.contentEntryFocus() else Modifier)
					.dpadFocusIndicator()
			)
		}
	}
}

@Composable
private fun FileCategoryList(
	modifier: Modifier,
	header: LazyListScope.() -> Unit,
	files: List<AmigaFile>,
	allFiles: List<AmigaFile>,
	showSearch: Boolean,
	searchQuery: String,
	effectiveSearchQuery: String,
	onSearchQueryChange: (String) -> Unit,
	showProgress: Boolean,
	isScanning: Boolean,
	progressDescription: String,
	categoryTitle: String,
	emptyHelp: String,
	onImport: () -> Unit,
	fileListFocus: FocusRequester,
	bottomPadding: Dp,
	onOpenActions: (AmigaFile) -> Unit
) {
	LazyColumn(
		modifier = modifier.focusRequester(fileListFocus).focusRestorer(),
		contentPadding = PaddingValues(start = 16.dp, top = 4.dp, end = 16.dp, bottom = bottomPadding),
		verticalArrangement = Arrangement.spacedBy(4.dp)
	) {
		header()

		if (showProgress) {
			item(key = "progress") {
				LinearProgressIndicator(
					modifier = Modifier
						.fillMaxWidth()
						.semantics { contentDescription = progressDescription }
				)
			}
		}

		if (showSearch) {
			item(key = "search") {
				OutlinedTextField(
					value = searchQuery,
					onValueChange = onSearchQueryChange,
					modifier = Modifier.fillMaxWidth().padding(vertical = 2.dp),
					placeholder = { Text(stringResource(R.string.search_placeholder)) },
					leadingIcon = { Icon(Icons.Default.Search, contentDescription = null, modifier = Modifier.size(18.dp)) },
					trailingIcon = {
						if (searchQuery.isNotEmpty()) {
							IconButton(
								onClick = { onSearchQueryChange("") },
								modifier = Modifier.dpadFocusIndicator()
							) {
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
		}

		if (files.isEmpty() && !isScanning) {
			item(key = "empty") {
				val searchHasNoResults = effectiveSearchQuery.isNotBlank() && allFiles.isNotEmpty()
				Column(
					modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 32.dp),
					horizontalAlignment = Alignment.CenterHorizontally
				) {
					Text(
						text = if (searchHasNoResults) {
							stringResource(R.string.file_manager_no_search_results)
						} else {
							stringResource(
								R.string.file_manager_no_files_found,
								categoryTitle.lowercase(Locale.getDefault())
							)
						},
						style = MaterialTheme.typography.bodyLarge
					)
					Spacer(modifier = Modifier.height(8.dp))
					Text(
						text = if (searchHasNoResults) stringResource(R.string.file_manager_search_help) else emptyHelp,
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
					Spacer(modifier = Modifier.height(24.dp))
					Button(
						onClick = {
							if (searchHasNoResults) onSearchQueryChange("") else onImport()
						},
						enabled = searchHasNoResults || !showProgress,
						modifier = Modifier.dpadFocusIndicator()
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
								stringResource(R.string.file_manager_import_category, categoryTitle)
							}
						)
					}
				}
			}
		} else {
			items(files, key = { it.path }) { file ->
				FileItem(file = file, onOpenActions = { onOpenActions(file) })
			}
		}
	}
}

/** The built-in AROS row has no actions: it cannot be deleted and does not launch. */
private fun hasFileActions(file: AmigaFile): Boolean =
	file.category != FileCategory.ROMS || ModelRomAvailability.romIdentity(file) != RomIdentity.BuiltInAros

@Composable
private fun FileItem(file: AmigaFile, onOpenActions: () -> Unit) {
	// A ROM shows what it is (the emulator's name for a Kickstart, or the built-in
	// AROS), with its file name underneath.
	val romIdentity = remember(file) {
		if (file.category == FileCategory.ROMS) ModelRomAvailability.romIdentity(file) else RomIdentity.Unknown
	}
	val content: @Composable () -> Unit = {
		Row(
			modifier = Modifier
				.fillMaxWidth()
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
				Text(
					text = when (romIdentity) {
						is RomIdentity.Kickstart -> romIdentity.name
						RomIdentity.BuiltInAros -> stringResource(R.string.file_manager_aros_row)
						RomIdentity.Unknown -> file.name
					},
					style = MaterialTheme.typography.bodyMedium
				)
				Text(
					text = when (romIdentity) {
						is RomIdentity.Kickstart ->
							stringResource(R.string.file_manager_rom_file_detail, file.name, file.sizeDisplay)
						RomIdentity.BuiltInAros -> stringResource(R.string.file_manager_aros_row_detail)
						RomIdentity.Unknown -> file.sizeDisplay
					},
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
			if (romIdentity != RomIdentity.BuiltInAros) {
				Icon(
					Icons.AutoMirrored.Filled.KeyboardArrowRight,
					contentDescription = null,
					tint = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
	}
	Card(modifier = Modifier.fillMaxWidth()) {
		if (romIdentity == RomIdentity.BuiltInAros) {
			content()
		} else {
			// TalkBack announces the tap as "Show actions".
			Box(
				modifier = Modifier
					.fillMaxWidth()
					.dpadFocusIndicator()
					.clickable(onClickLabel = stringResource(R.string.action_show_actions), onClick = onOpenActions)
			) {
				content()
			}
		}
	}
}

private fun categoryIcon(category: FileCategory): ImageVector = when (category) {
	FileCategory.ROMS -> Icons.Default.Memory
	FileCategory.FLOPPIES -> Icons.Default.SaveAlt
	FileCategory.HARD_DRIVES -> Icons.Default.Storage
	FileCategory.CD_IMAGES -> Icons.Default.Album
	FileCategory.WHDLOAD_GAMES -> Icons.Default.SportsEsports
}

@Composable
private fun FileActionsDialog(
	file: AmigaFile,
	busy: Boolean,
	onDismiss: () -> Unit,
	onAction: (FileAction) -> Unit
) {
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(file.name) },
		text = {
			Column {
				val firstFocus = remember { FocusRequester() }
				LaunchedEffect(Unit) { firstFocus.requestFocus() }
				fileActionsFor(file.category).forEachIndexed { index, action ->
					val (icon, label) = when (action) {
						FileAction.InsertDf0AndStart -> Icons.Default.PlayArrow to stringResource(R.string.file_manager_action_insert_df0_start)
						FileAction.InsertDf1 -> Icons.Default.SaveAlt to stringResource(R.string.file_manager_action_insert_df1)
						FileAction.LaunchWhdload -> Icons.Default.PlayArrow to stringResource(R.string.action_launch)
						FileAction.Delete -> Icons.Default.Delete to stringResource(R.string.action_delete)
					}
					TextButton(
						onClick = { onAction(action) },
						enabled = !busy,
						modifier = Modifier
							.fillMaxWidth()
							.then(if (index == 0) Modifier.focusRequester(firstFocus) else Modifier)
							.dpadFocusIndicator()
					) {
						Icon(icon, contentDescription = null, modifier = Modifier.size(20.dp))
						Spacer(modifier = Modifier.width(12.dp))
						Text(
							label,
							modifier = Modifier.weight(1f),
							color = if (action == FileAction.Delete) MaterialTheme.colorScheme.error else Color.Unspecified
						)
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
private fun StorageLocationDialog(path: String, canCopy: Boolean, onCopied: () -> Unit, onDismiss: () -> Unit) {
	val context = LocalContext.current
	val clipboardLabel = stringResource(R.string.clipboard_label_path)
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.file_manager_storage_location)) },
		text = {
			Column {
				Text(path, style = MaterialTheme.typography.bodyMedium)
				Spacer(modifier = Modifier.height(8.dp))
				Text(
					stringResource(
						if (canCopy) R.string.file_manager_storage_location_help else R.string.file_manager_storage_location_help_tv
					),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		},
		confirmButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_done))
			}
		},
		dismissButton = {
			// TVs have no use for the clipboard.
			if (canCopy) {
				TextButton(
					onClick = {
						val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
						clipboard.setPrimaryClip(ClipData.newPlainText(clipboardLabel, path))
						onCopied()
						onDismiss()
					},
					modifier = Modifier.dpadFocusIndicator()
				) {
					Icon(Icons.Default.ContentCopy, contentDescription = null, modifier = Modifier.size(18.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(stringResource(R.string.file_manager_copy_path))
				}
			}
		}
	)
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

/** A window this short is a phone held sideways (as WindowLayout's 480 dp). */
private const val COMPACT_HEIGHT_DP = 480
private val COMPACT_TOP_BAR_HEIGHT = 48.dp

/** Room below the last row for the Import FAB (56 dp + its 16 dp margin) and some air. */
private val FAB_CLEARANCE = 96.dp
