package com.blitterstudio.amiberry.ui.screens.library

import android.net.Uri
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.FilterList
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavController
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.data.ConfigurationSaveActions
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.FileListing
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.ImportBatchFeedback
import com.blitterstudio.amiberry.data.ImportFeedback
import com.blitterstudio.amiberry.data.RecentLaunches
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.ui.MainActivity
import com.blitterstudio.amiberry.ui.components.ImportPicker
import com.blitterstudio.amiberry.ui.components.PickedFolder
import com.blitterstudio.amiberry.ui.components.SidePaneEntry
import com.blitterstudio.amiberry.ui.components.SidePaneList
import com.blitterstudio.amiberry.ui.components.rememberImportPicker
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.isTvDevice
import com.blitterstudio.amiberry.ui.launchGuarded
import com.blitterstudio.amiberry.ui.launchImportGuarded
import com.blitterstudio.amiberry.ui.navigation.Screen
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.screens.settings.SaveConfigDialog
import com.blitterstudio.amiberry.ui.viewmodel.ConfigurationsViewModel
import com.blitterstudio.amiberry.ui.viewmodel.FileManagerViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.ui.windowWidthClass
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.json.JSONObject

/**
 * The Library the app opens to: the saved machines, the media shelves and the Kickstart
 * ROMs, one of them listed at a time. A phone picks what it lists from the filter menu in
 * the top bar; a wide window (and a TV) lists the filters beside the content.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LibraryScreen(
	navController: NavController,
	configurationsViewModel: ConfigurationsViewModel = viewModel(),
	fileViewModel: FileManagerViewModel = viewModel(),
	settingsViewModel: SettingsViewModel = viewModel(LocalContext.current.findActivity() as ComponentActivity)
) {
	val context = LocalContext.current
	val activity = context.findActivity() as? MainActivity
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	val launchGuard = rememberLaunchInFlightGuard()
	val importGuard = rememberImportInFlightGuard()
	val launchInProgress = launchGuard.isLaunching
	// One launch admission for the whole Library: a Play a File copy runs in the
	// activity's own guard, so its state keeps every other Play action waiting too.
	val launchBusy = launchInProgress || (activity?.importLaunchInProgress == true)
	val importInProgress = importGuard.isImporting
	val isTv = isTvDevice()
	val preferences = remember { AppPreferences.getInstance(context) }
	val repository = remember { FileRepository.getInstance(context) }

	// The machines and their categories; the media folders; the ROMs.
	val configs by configurationsViewModel.configs.collectAsState()
	val categories = remember(configs) { machineCategories(configs.map { it.category }) }
	val filters = remember(categories) { LibraryFilter.all(categories) }
	val roms by fileViewModel.roms.collectAsState()
	val floppies by fileViewModel.floppies.collectAsState()
	val cds by fileViewModel.cdImages.collectAsState()
	val whdloadGames by fileViewModel.whdloadGames.collectAsState()
	val hardDrives by fileViewModel.hardDrives.collectAsState()
	val isScanning by fileViewModel.isScanning.collectAsState()
	// The complete ROM inventory decides what can run (the display list below hides the
	// AROS extension ROM, which availability also needs); the filtered list is display only.
	val allRoms by repository.roms.collectAsState()
	val availableModels = remember(allRoms) { ModelRomAvailability.availableModels(allRoms) }
	val kickstartInstalled = availableModels.any { ModelRomAvailability.hasKickstart(it, allRoms) }
	// A shelf import runs through the view model: its busy state keeps the Import action
	// honest while it works (and stops a second batch from being silently dropped).
	val filesImporting by fileViewModel.isImporting.collectAsState()

	// What was played last, most recent first. An entry's files are checked against the
	// shelf inventories: deleting a played disk or re-importing a missing one shows on
	// its row as soon as the scan that follows lands.
	val recentEntries by preferences.recentLaunches
	val recentItems = remember(
		recentEntries, availableModels, floppies, cds, whdloadGames, hardDrives
	) {
		RecentLaunches.items(recentEntries, availableModels)
	}

	// The chosen filter, kept across recreation by name; a category that went (its last
	// machine deleted) falls back to them all.
	var filterName by rememberSaveable { mutableStateOf(LibraryFilter.AllMachines.persistName) }
	val filter = remember(filterName, categories) {
		val chosen = LibraryFilter.fromName(filterName)
		if (chosen is LibraryFilter.Category && chosen.name !in categories) LibraryFilter.AllMachines else chosen
	}
	var searchQuery by rememberSaveable { mutableStateOf("") }
	var searchOpen by rememberSaveable { mutableStateOf(false) }
	LaunchedEffect(filterName) {
		if (searchQuery.isNotEmpty()) searchQuery = ""
	}
	BackHandler(enabled = searchOpen) { searchOpen = false }

	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)

	// Files copied in while the app was away show up when the Library is shown again.
	val lifecycleOwner = LocalLifecycleOwner.current
	DisposableEffect(lifecycleOwner, configurationsViewModel, fileViewModel) {
		val observer = LifecycleEventObserver { _, event ->
			if (event == Lifecycle.Event.ON_RESUME) {
				configurationsViewModel.refresh()
				fileViewModel.rescan()
			}
		}
		lifecycleOwner.lifecycle.addObserver(observer)
		onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
	}

	// The ROM advice over the Library, and what the last import found.
	val romAdviceDismissed by preferences.romAdviceDismissed
	var romReport by remember { mutableStateOf<RomImportReport?>(null) }

	/** Opens the machine editor, D-pad focus with it. */
	fun openEditor() {
		settingsViewModel.focusOnEntry = true
		navController.navigate(Screen.Settings.route)
	}

	// An action that replaces what the editor holds (Edit, New Machine) waits here while
	// the editor has unsaved changes, a new configuration's included.
	var pendingReplace by remember { mutableStateOf<(() -> Unit)?>(null) }
	fun replaceSettings(action: () -> Unit) {
		if (settingsViewModel.hasUnsavedChanges) pendingReplace = action else action()
	}

	// Save chosen for a new configuration: the name dialog, and the action waiting for it.
	var saveNewName by remember { mutableStateOf<String?>(null) }
	var saveNewThen by remember { mutableStateOf<(() -> Unit)?>(null) }
	fun saveMessage(result: ConfigurationSaveActions.SaveResult): String {
		val message = ConfigurationSaveActions.messageFor(result)
		return message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)
	}

	/** A new machine from [model]: the editor opens with its preset applied. */
	fun newMachine(model: AmigaModel) {
		replaceSettings {
			settingsViewModel.newConfiguration()
			settingsViewModel.applyModel(model)
			openEditor()
		}
	}

	/** Loads the machine into the editor and opens it. */
	fun editMachine(config: ConfigInfo) {
		if (settingsViewModel.isConfigOpen(config.path)) {
			// Already open: its edits stay.
			openEditor()
			return
		}
		replaceSettings {
			scope.launch {
				when (val result = ConfigurationActions.load(runCatching { configurationsViewModel.loadConfig(config.path) })) {
					is ConfigurationActions.LoadResult.Loaded -> {
						settingsViewModel.loadConfig(result.value, config.name, config.path)
						openEditor()
					}
					is ConfigurationActions.LoadResult.Failed ->
						snackbarHostState.showSnackbar(actionMessage(result.message))
				}
			}
		}
	}

	fun showResult(result: LibraryLaunch.PlayResult) {
		val refused = result as? LibraryLaunch.PlayResult.Refused ?: return
		val message = refused.argument?.let { context.getString(refused.messageRes, it) }
			?: context.getString(refused.messageRes)
		scope.launch { snackbarHostState.showSnackbar(message) }
	}

	/** Quick play: the file's kind in its machine. */
	fun quickPlay(file: AmigaFile) {
		if (launchBusy) return
		scope.launchGuarded(launchGuard) {
			showResult(LibraryLaunch.play(context, file, repository.roms.value))
		}
	}

	/** A recent entry again, as it was launched. */
	fun playRecent(entry: JSONObject) {
		if (launchBusy) return
		scope.launchGuarded(launchGuard) {
			showResult(
				LibraryLaunch.playRecent(
					context = context,
					entry = entry,
					roms = repository.roms.value,
					loadMachine = { path -> configurationsViewModel.loadConfig(path) },
					writeConfig = { settings, unknownLines ->
						settingsViewModel.writeSettingsConfig(settings, LibraryLaunch.PLAY_IN_CONFIG, unknownLines)
					}
				)
			)
		}
	}

	/** A file in a saved machine: the machine loads, the file goes in DF0: or its CD drive. */
	fun playIn(file: AmigaFile, machine: ConfigInfo) {
		if (launchBusy) return
		scope.launchGuarded(launchGuard) {
			showResult(
				LibraryLaunch.playIn(
					context = context,
					file = file,
					machine = machine,
					loadMachine = { path -> configurationsViewModel.loadConfig(path) },
					writeConfig = { settings, unknownLines ->
						settingsViewModel.writeSettingsConfig(settings, LibraryLaunch.PLAY_IN_CONFIG, unknownLines)
					}
				)
			)
		}
	}

	fun romImportFeedback(results: List<FileManager.ImportResult>) {
		val message = if (results.size == 1) {
			val feedback = ImportFeedback.fromImportResult(results.first())
			context.getString(feedback.stringRes, feedback.argument)
		} else {
			val feedback = ImportBatchFeedback.messageFor(results)
			context.getString(feedback.stringRes, *feedback.args.map { it as Any }.toTypedArray())
		}
		scope.launch { snackbarHostState.showSnackbar(message) }
	}

	/**
	 * A pick or a folder can hold other kinds than ROMs (a whole folder imported from the
	 * in-app browser often does): every category the import filled is rescanned, so its
	 * files reach their shelves at once, not only the ROMs.
	 */
	suspend fun rescanImported(results: List<FileManager.ImportResult>) {
		val imported = FileListing.importSummary(results, FileCategory.ROMS).categories + FileCategory.ROMS
		imported.distinct().forEach { repository.rescanCategory(it) }
	}

	// A new machine that boots AROS moves to the model's Kickstart; a named
	// configuration keeps the ROM it was saved with.
	fun importedKickstartInstalled() {
		if (settingsViewModel.currentConfigName == null) {
			ModelRomAvailability.withInstalledKickstart(settingsViewModel.settings, repository.roms.value)?.let {
				settingsViewModel.updateSettings { it }
			}
		}
	}

	/** Imports ROM files and says what the scan found, as the banner reports it. */
	fun importRoms(uris: List<Uri>) {
		if (uris.isEmpty() || importInProgress) return
		scope.launchImportGuarded(importGuard) {
			val results = withContext(Dispatchers.IO) {
				val before = repository.roms.value.size
				val imported = uris.map { FileManager.importFileWithResult(context, it, FileCategory.ROMS) }
				rescanImported(imported)
				romReport = RomImportReport.of(before, repository.roms.value)
				imported
			}
			importedKickstartInstalled()
			romImportFeedback(results)
		}
	}

	fun importRomFolder(folder: PickedFolder) {
		if (importInProgress) return
		scope.launchImportGuarded(importGuard) {
			val results = withContext(Dispatchers.IO) {
				val before = repository.roms.value.size
				val imported = when (folder) {
					is PickedFolder.Tree -> FileManager.importFolderWithResults(context, folder.uri)
					is PickedFolder.Local -> FileManager.importDirectoryWithResults(context, folder.dir)
				}
				rescanImported(imported)
				romReport = RomImportReport.of(before, repository.roms.value)
				imported
			}
			importedKickstartInstalled()
			romImportFeedback(results)
		}
	}


	// Play a File: the picked file is imported and started, as a file manager's Open does.
	val playFilePicker = rememberImportPicker(
		mimeTypes = arrayOf("*/*"),
		extensions = FileCategory.allExtensions + setOf("uae", "rp9")
	) { uris ->
		uris.firstOrNull()?.let { uri -> activity?.importAndLaunch(uri) }
	}

	val romPicker = rememberImportPicker(
		mimeTypes = FilePickerFilters.mimeTypesFor(FileCategory.ROMS),
		extensions = FileCategory.ROMS.extensions,
		multiple = true,
		onFolder = { folder -> importRomFolder(folder) }
	) { uris ->
		importRoms(uris)
	}

	// A shelf import goes to the category of each file's extension; the picker's filters
	// offer every supported kind, as the Files tab's did.
	val shelfPicker = rememberImportPicker(
		mimeTypes = arrayOf("*/*"),
		extensions = FileCategory.allExtensions,
		multiple = true,
		onFolder = { folder ->
			val category = (filter as? LibraryFilter.Media)?.shelf?.category ?: return@rememberImportPicker
			if (!importInProgress) fileViewModel.importFolder(folder, category)
		}
	) { uris ->
		val category = (filter as? LibraryFilter.Media)?.shelf?.category ?: return@rememberImportPicker
		if (uris.isNotEmpty() && !importInProgress) fileViewModel.importFiles(uris, category)
	}

	val importResult by fileViewModel.importResult.collectAsState()
	LaunchedEffect(importResult) {
		importResult?.let { message ->
			snackbarHostState.showSnackbar(message)
			fileViewModel.clearImportResult()
		}
	}

	// Every file of a shelf import went to one category: show that shelf.
	val revealCategory by fileViewModel.revealCategory.collectAsState()
	LaunchedEffect(revealCategory) {
		val category = revealCategory ?: return@LaunchedEffect
		MediaShelf.forCategory(category)?.let { shelf ->
			filterName = LibraryFilter.Media(shelf).persistName
		}
		searchQuery = ""
		fileViewModel.clearRevealCategory()
	}

	val wide = windowWidthClass().showsSideList
	val showRomAdvice = filter != LibraryFilter.KickstartRoms && !romAdviceDismissed && !kickstartInstalled
	var showFilterMenu by remember { mutableStateOf(false) }
	var showOverflowMenu by remember { mutableStateOf(false) }
	var showNewMachineMenu by remember { mutableStateOf(false) }
	// Each filter's row count, for the badges of the side list and the filter menu.
	val counts = remember(configs, categories, recentItems, floppies, cds, whdloadGames, hardDrives) {
		filterCounts(configs, categories, recentItems.size, floppies.size, cds.size, whdloadGames.size, hardDrives.size)
	}

	fun selectFilter(index: Int) {
		filters.getOrNull(index)?.let { filterName = it.persistName }
	}

	// The machines, a shelf or the ROMs: the whole content on a phone, the pane beside the
	// filter list on a wide window.
	val content: @Composable (Modifier) -> Unit = { modifier ->
		LibraryContent(
			modifier = modifier,
			filter = filter,
			searchQuery = searchQuery,
			configs = configs,
			settingsViewModel = settingsViewModel,
			configurationsViewModel = configurationsViewModel,
			scope = scope,
			snackbarHostState = snackbarHostState,
			launchGuard = launchGuard,
			preferences = preferences,
			roms = roms,
			allRoms = allRoms,
			floppies = floppies,
			cds = cds,
			whdloadGames = whdloadGames,
			hardDrives = hardDrives,
			recentItems = recentItems,
			isScanning = isScanning,
			launchInProgress = launchBusy,
			importInProgress = importInProgress,
			filesImporting = filesImporting,
			shelfPicker = shelfPicker,
			romPicker = romPicker,
			fileViewModel = fileViewModel,
			onEditMachine = ::editMachine,
			onNewMachine = ::newMachine,
			onPlayFile = { if (!launchBusy) playFilePicker.pickFiles() },
			onQuickPlay = ::quickPlay,
			onPlayIn = ::playIn,
			onPlayRecent = ::playRecent,
			onRemoveRecent = { item -> preferences.removeRecentLaunch(item.entry) },
			onClearRecent = { preferences.clearRecentLaunches() }
		)
	}

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				navigationIcon = if (!wide) {
					{
						Box {
							IconButton(
								onClick = { showFilterMenu = true },
								modifier = Modifier.contentEntryFocus().dpadFocusIndicator()
							) {
								Icon(
									Icons.Default.FilterList,
									contentDescription = stringResource(R.string.library_show_filter)
								)
							}
							FilterMenu(
								expanded = showFilterMenu,
								onDismiss = { showFilterMenu = false },
								filters = filters,
								selected = filter,
								counts = counts,
								onSelect = { index ->
									showFilterMenu = false
									selectFilter(index)
								}
							)
						}
					}
				} else {
					{}
				},
				title = { Text(filterTitle(filter)) },
				actions = {
					IconButton(
						onClick = { if (!launchBusy) playFilePicker.pickFiles() },
						enabled = !launchBusy,
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(
							Icons.Default.PlayArrow,
							contentDescription = stringResource(R.string.library_play_file)
						)
					}
					Box {
						IconButton(
							onClick = { showNewMachineMenu = true },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(
								Icons.Default.Add,
								contentDescription = stringResource(R.string.library_new_machine)
							)
						}
						NewMachineMenu(
							expanded = showNewMachineMenu,
							onDismiss = { showNewMachineMenu = false },
							roms = repository.roms.value,
							onNewMachine = { model ->
								showNewMachineMenu = false
								newMachine(model)
							}
						)
					}
					IconButton(
						onClick = { searchOpen = !searchOpen },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(
							Icons.Default.Search,
							contentDescription = stringResource(R.string.library_search)
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
								text = { Text(stringResource(R.string.nav_about)) },
								modifier = Modifier.dpadFocusIndicator(),
								leadingIcon = { Icon(Icons.Default.Info, contentDescription = null) },
								onClick = {
									showOverflowMenu = false
									navController.navigate(Screen.About.route)
								}
							)
						}
					}
				}
			)
		}
	) { innerPadding ->
		// Where D-pad Right from the filter list enters the content, restored to the
		// control last focused there.
		val contentFocus = remember { FocusRequester() }
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
		) {
			if (searchOpen) {
				SearchRow(
					filter = filter,
					query = searchQuery,
					onQueryChange = { searchQuery = it },
					onClose = { searchOpen = false }
				)
			}
			if (showRomAdvice || romReport != null) {
				RomNotice(
					showAdvice = showRomAdvice,
					report = romReport,
					importInProgress = importInProgress,
					onImport = { if (!importInProgress) romPicker.pickFiles() },
					onDismissAdvice = { preferences.dismissRomAdvice() },
					onDismissReport = { romReport = null }
				)
			}
			if (wide) {
				Row(modifier = Modifier.fillMaxSize()) {
					SidePaneList(
						entries = filters.mapIndexed { index, entry ->
							SidePaneEntry(
								label = filterTitle(entry),
								icon = entry.icon,
								badge = counts[index]
							)
						},
					selectedIndex = filters.indexOfFirst { it == filter }.coerceAtLeast(0),
						onSelect = { selectFilter(it) },
						modifier = Modifier.contentEntryFocus(),
						contentFocus = contentFocus,
						selectOnFocus = isTv,
						// Wide enough that every label and its count keep one line, with the
						// sections the filter menu also shows.
						paneWidth = 280.dp,
						headers = mapOf(
							0 to stringResource(R.string.library_filter_machines),
							filters.indexOfFirst {
								it is LibraryFilter.Media && it.shelf == MediaShelf.Recent
							} to stringResource(R.string.library_filter_media),
							filters.indexOfFirst { it == LibraryFilter.KickstartRoms } to
								stringResource(R.string.library_filter_system)
						)
					)
					VerticalDivider()
					content(
						Modifier
							.weight(1f)
							.fillMaxSize()
							.focusRequester(contentFocus)
							.focusRestorer()
					)
				}
			} else {
				content(Modifier.fillMaxSize())
			}
		}
	}

	pendingReplace?.let { proceed ->
		val openName = settingsViewModel.currentConfigName
		UnsavedSettingsDialog(
			message = if (openName != null) {
				stringResource(R.string.dialog_unsaved_changes_switch_message, openName)
			} else {
				stringResource(R.string.dialog_unsaved_new_config_message)
			},
			onSave = {
				pendingReplace = null
				scope.launch {
					if (openName != null) {
						val result = settingsViewModel.saveTracked()
						if (result is ConfigurationSaveActions.SaveResult.Saved) {
							proceed()
						} else {
							snackbarHostState.showSnackbar(saveMessage(result))
						}
					} else {
						saveNewThen = proceed
						saveNewName = settingsViewModel.suggestedConfigName()
					}
				}
			},
			onDontSave = {
				pendingReplace = null
				proceed()
			},
			onCancel = { pendingReplace = null }
		)
	}

	saveNewName?.let { initialName ->
		SaveConfigDialog(
			initialName = initialName,
			initialDescription = "",
			onDismiss = {
				saveNewName = null
				saveNewThen = null
			},
			onSave = { name, description ->
				scope.launch {
					val result = settingsViewModel.saveAs(name, description, allowOverwrite = false)
					if (result is ConfigurationSaveActions.SaveResult.Saved) {
						saveNewName = null
						configurationsViewModel.refresh()
						saveNewThen?.invoke()
						saveNewThen = null
					}
					snackbarHostState.showSnackbar(saveMessage(result))
				}
			}
		)
	}
}

@Composable
private fun LibraryContent(
	modifier: Modifier,
	filter: LibraryFilter,
	searchQuery: String,
	configs: List<ConfigInfo>,
	settingsViewModel: SettingsViewModel,
	configurationsViewModel: ConfigurationsViewModel,
	roms: List<AmigaFile>,
	allRoms: List<AmigaFile>,
	filesImporting: Boolean,
	floppies: List<AmigaFile>,
	cds: List<AmigaFile>,
	whdloadGames: List<AmigaFile>,
	hardDrives: List<AmigaFile>,
	recentItems: List<RecentLaunches.Item>,
	isScanning: Boolean,
	launchInProgress: Boolean,
	importInProgress: Boolean,
	shelfPicker: ImportPicker,
	romPicker: ImportPicker,
	fileViewModel: FileManagerViewModel,
	scope: kotlinx.coroutines.CoroutineScope,
	snackbarHostState: androidx.compose.material3.SnackbarHostState,
	launchGuard: com.blitterstudio.amiberry.ui.LaunchInFlightGuard,
	preferences: AppPreferences,
	onEditMachine: (ConfigInfo) -> Unit,
	onNewMachine: (AmigaModel) -> Unit,
	onPlayFile: () -> Unit,
	onQuickPlay: (AmigaFile) -> Unit,
	onPlayIn: (AmigaFile, ConfigInfo) -> Unit,
	onPlayRecent: (JSONObject) -> Unit,
	onRemoveRecent: (RecentLaunches.Item) -> Unit,
	onClearRecent: () -> Unit
) {
	when (filter) {
		is LibraryFilter.AllMachines, is LibraryFilter.Category -> LibraryMachinesPane(
			modifier = modifier,
			configs = configs,
			category = (filter as? LibraryFilter.Category)?.name,
			query = searchQuery,
			settingsViewModel = settingsViewModel,
			viewModel = configurationsViewModel,
			scope = scope,
			snackbarHostState = snackbarHostState,
			launchGuard = launchGuard,
			launchInProgress = launchInProgress,
			onEdit = onEditMachine,
			onNewMachine = onNewMachine,
			onPlayFile = onPlayFile
		)
		is LibraryFilter.Media -> LibraryMediaShelfPage(
			modifier = modifier,
			shelf = filter.shelf,
			query = searchQuery,
			floppies = floppies,
			cds = cds,
			whdloadGames = whdloadGames,
			hardDrives = hardDrives,
			recentItems = recentItems,
			machines = configs,
			isScanning = isScanning,
			launchInProgress = launchInProgress,
			importInProgress = importInProgress || filesImporting,
			importPicker = shelfPicker,
			onDelete = fileViewModel::deleteFile,
			onQuickPlay = onQuickPlay,
			onPlayIn = onPlayIn,
			onPlayRecent = onPlayRecent,
			onRemoveRecent = { item -> preferences.removeRecentLaunch(item.entry) },
			onClearRecent = { preferences.clearRecentLaunches() }
		)
		LibraryFilter.KickstartRoms -> LibraryKickstartRomsPage(
			modifier = modifier,
			roms = roms,
			modelRoms = allRoms,
			query = searchQuery,
			importInProgress = importInProgress,
			importPicker = romPicker,
			onDelete = fileViewModel::deleteFile
		)
	}
}

@Composable
private fun filterTitle(filter: LibraryFilter): String = when (filter) {
	is LibraryFilter.Category -> filter.name
	else -> stringResource(filter.titleRes)
}


/** Each filter's row count, in [LibraryFilter.all]'s order; null for none (the ROMs). */
private fun filterCounts(
	configs: List<ConfigInfo>,
	categories: List<String>,
	recentCount: Int,
	floppies: Int,
	cds: Int,
	whdloadGames: Int,
	hardDrives: Int
): List<String?> = buildList {
	add(configs.size.toString())
	categories.forEach { category ->
		add(configs.count { it.category == category }.toString())
	}
	add(recentCount.toString())
	add(floppies.toString())
	add(cds.toString())
	add(whdloadGames.toString())
	add(hardDrives.toString())
	add(null)
}

/** The phone's filter menu: the Library's sections as one list, the chosen entry ticked. */
@Composable
private fun FilterMenu(
	expanded: Boolean,
	onDismiss: () -> Unit,
	filters: List<LibraryFilter>,
	selected: LibraryFilter,
	counts: List<String?>,
	onSelect: (Int) -> Unit
) {
	DropdownMenu(expanded = expanded, onDismissRequest = onDismiss) {
		filters.forEachIndexed { index, filter ->
			// A section's header, above its first entry.
			val header = when {
				filter == LibraryFilter.AllMachines -> R.string.library_filter_machines
				filter is LibraryFilter.Category && filters.take(index).none { it is LibraryFilter.Category } ->
					R.string.library_filter_machines
				filter is LibraryFilter.Media && filter.shelf == MediaShelf.Recent -> R.string.library_filter_media
				filter == LibraryFilter.KickstartRoms -> R.string.library_filter_system
				else -> null
			}
			if (header != null) {
				DropdownMenuItem(
					text = {
						Text(
							stringResource(header),
							style = MaterialTheme.typography.labelMedium,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)
					},
					enabled = false,
					onClick = {}
				)
			}
			DropdownMenuItem(
				text = { Text(filterTitle(filter)) },
				trailingIcon = {
					Row(verticalAlignment = Alignment.CenterVertically) {
						counts[index]?.let { count ->
							Text(
								count,
								style = MaterialTheme.typography.labelMedium,
								color = MaterialTheme.colorScheme.onSurfaceVariant
							)
							Spacer(modifier = Modifier.width(8.dp))
						}
						if (filter == selected) {
							Icon(Icons.Default.Check, contentDescription = null, modifier = Modifier.size(18.dp))
						}
					}
				},
				modifier = Modifier.dpadFocusIndicator(),
				onClick = { onSelect(index) }
			)
		}
	}
}

/** New Machine: one entry per model, those without their Kickstart marked. */
@Composable
private fun NewMachineMenu(
	expanded: Boolean,
	onDismiss: () -> Unit,
	roms: List<AmigaFile>,
	onNewMachine: (AmigaModel) -> Unit
) {
	DropdownMenu(expanded = expanded, onDismissRequest = onDismiss) {
		AmigaModel.entries.forEach { model ->
			DropdownMenuItem(
				text = {
					Text(
						when {
							ModelRomAvailability.hasKickstart(model, roms) -> model.displayName
							ModelRomAvailability.canBootAros(model) ->
								stringResource(R.string.library_new_machine_boots_aros, model.displayName)
							else -> stringResource(R.string.library_new_machine_needs_rom, model.displayName)
						}
					)
				},
				modifier = Modifier.dpadFocusIndicator(),
				onClick = { onNewMachine(model) }
			)
		}
	}
}

/** The search field for the listed filter, with its close button. */
@Composable
private fun SearchRow(
	filter: LibraryFilter,
	query: String,
	onQueryChange: (String) -> Unit,
	onClose: () -> Unit
) {
	val typingGate = rememberDpadTypingGate()
	Surface(color = MaterialTheme.colorScheme.surface) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(start = 16.dp, end = 8.dp, top = 4.dp, bottom = 4.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			OutlinedTextField(
				value = query,
				onValueChange = onQueryChange,
				placeholder = { Text(stringResource(filter.searchPromptRes)) },
				singleLine = true,
				readOnly = typingGate.readOnly,
				modifier = Modifier
					.weight(1f)
					.dpadVerticalMovesFocus()
					.then(typingGate.modifier),
				trailingIcon = {
					if (query.isNotEmpty()) {
						IconButton(
							onClick = { onQueryChange("") },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_clear_search))
						}
					}
				}
			)
			IconButton(onClick = onClose, modifier = Modifier.dpadFocusIndicator()) {
				Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_cancel))
			}
		}
	}
}

/**
 * Until a Kickstart is installed: what machines boot meanwhile, and the way to import
 * ROMs; then what the last import found, until it is closed.
 */
@Composable
private fun RomNotice(
	showAdvice: Boolean,
	report: RomImportReport?,
	importInProgress: Boolean,
	onImport: () -> Unit,
	onDismissAdvice: () -> Unit,
	onDismissReport: () -> Unit
) {
	Surface(color = MaterialTheme.colorScheme.surfaceVariant) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(start = 16.dp, end = 8.dp, top = 8.dp, bottom = 8.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			Icon(
				if (report != null) Icons.Default.Check else Icons.Default.Memory,
				tint = if (report != null) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.error,
				contentDescription = null,
				modifier = Modifier.size(24.dp)
			)
			Spacer(modifier = Modifier.width(12.dp))
			Column(modifier = Modifier.weight(1f)) {
				if (showAdvice) {
					Text(stringResource(R.string.library_rom_notice_title), style = MaterialTheme.typography.titleSmall)
					Text(
						stringResource(R.string.library_rom_notice_message),
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
				report?.let {
					Text(
						stringResource(it.stringRes, *it.args.toTypedArray()),
						style = MaterialTheme.typography.bodySmall
					)
				}
			}
			if (showAdvice) {
				Button(onClick = onImport, enabled = !importInProgress, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_import_roms))
				}
			}
			IconButton(
				onClick = {
					if (showAdvice) onDismissAdvice()
					onDismissReport()
				},
				modifier = Modifier.dpadFocusIndicator()
			) {
				Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_close))
			}
		}
	}
}

/** What a ROM import found, in the Library's words. */
internal data class RomImportReport(val stringRes: Int, val args: List<String>) {
	companion object {
		/** The report of the change from [before] files to the scanned [roms]. */
		fun of(before: Int, roms: List<AmigaFile>): RomImportReport {
			val added = roms.size - before
			val ready = ModelRomAvailability.availableModels(roms)
				.filter { ModelRomAvailability.hasKickstart(it, roms) }
				.map { it.displayName }
			return when {
				added <= 0 -> RomImportReport(R.string.library_rom_report_no_new, emptyList())
				ready.isEmpty() ->
					RomImportReport(R.string.library_rom_report_no_kickstart, listOf(added.toString()))
				else -> RomImportReport(R.string.library_rom_report_ready, listOf(ready.joinToString(", ")))
			}
		}
	}
}
