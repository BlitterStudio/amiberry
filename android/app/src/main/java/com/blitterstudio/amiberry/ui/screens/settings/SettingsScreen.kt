package com.blitterstudio.amiberry.ui.screens.settings

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.BringIntoViewSpec
import androidx.compose.foundation.gestures.LocalBringIntoViewSpec
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.clickable
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.Undo
import androidx.compose.material.icons.filled.AutoFixHigh
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Save
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.input.InputMode
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalInputModeManager
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.ConfigurationSaveActions
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.ImportFeedback
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotice
import com.blitterstudio.amiberry.data.model.SettingsChange
import com.blitterstudio.amiberry.data.model.SettingsIntentPreset
import com.blitterstudio.amiberry.ui.LocalStartGuard
import com.blitterstudio.amiberry.ui.components.rememberImportPicker
import com.blitterstudio.amiberry.ui.components.SidePaneEntry
import com.blitterstudio.amiberry.ui.components.SidePaneList
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadFocusable
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.isTvDevice
import com.blitterstudio.amiberry.ui.launchGuarded
import com.blitterstudio.amiberry.ui.launchImportGuarded
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.ui.windowWidthClass
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.math.min

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(viewModel: SettingsViewModel = viewModel(LocalContext.current.findActivity() as androidx.activity.ComponentActivity)) {
	val context = LocalContext.current
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	val launchGuard = rememberLaunchInFlightGuard()
	val launchInProgress = launchGuard.isLaunching
	val importGuard = rememberImportInFlightGuard()
	val importInProgress = importGuard.isImporting
	var selectedPage by rememberSaveable { mutableIntStateOf(0) }
	// On a phone, whether a page is open over the list of pages; Back returns to the list.
	var pageOpen by rememberSaveable { mutableStateOf(false) }
	// The name the Save dialog opens with; null while it is closed.
	var saveAsName by remember { mutableStateOf<String?>(null) }
	var overwriteRequest by remember { mutableStateOf<OverwriteRequest?>(null) }
	var showTopBarMenu by remember { mutableStateOf(false) }
	var showReviewChangesDialog by remember { mutableStateOf(false) }
	var showRevertDialog by remember { mutableStateOf(false) }
	var showQuickSetups by remember { mutableStateOf(false) }
	val availableRoms by viewModel.availableRoms.collectAsState()
	val canStart = viewModel.settings.romFile.isNotBlank() || availableRoms.isNotEmpty()
	val startGuard = LocalStartGuard.current
	// Up from Start returns to the control last focused in the page; the 2D search
	// would jump over the scrolled page to the controls above it.
	val pageContentFocus = remember { FocusRequester() }
	// The page list or the side list, where D-pad focus enters the screen.
	val categoryFocus = remember { FocusRequester() }
	val keyboardMode = LocalInputModeManager.current.inputMode == InputMode.Keyboard
	val wide = windowWidthClass().showsSideList
	val pages = SettingsPages.all
	val page = SettingsPages.pageFor(selectedPage)
	// The sidebar's marks (the Apple app's): an existing machine's edits,
	// page by page. All Settings lists them all, so it is not marked.
	val editedPages = if (viewModel.currentConfigName != null) {
		SettingsPages.editedPages(viewModel.changeSummary)
	} else {
		emptySet()
	}
	fun importResultMessage(result: FileManager.ImportResult): String {
		val message = ImportFeedback.fromImportResult(result)
		return context.getString(message.stringRes, message.argument)
	}

	val romPicker = rememberImportPicker(
		mimeTypes = FilePickerFilters.mimeTypesFor(FileCategory.ROMS),
		extensions = FileCategory.ROMS.extensions
	) { uris ->
		uris.first().let {
			scope.launchImportGuarded(importGuard) {
				val result = withContext(Dispatchers.IO) {
					FileManager.importFileWithResult(context, it, FileCategory.ROMS).also { importResult ->
						if (importResult is FileManager.ImportResult.Imported) {
							FileRepository.getInstance(context).rescanCategory(FileCategory.ROMS)
						}
					}
				}
				if (result is FileManager.ImportResult.Imported) {
					viewModel.updateSettings { s -> s.copy(romFile = result.path) }
				}
				snackbarHostState.showSnackbar(importResultMessage(result))
			}
		}
	}

	val validSelectedPage = SettingsPages.validSelectedIndex(selectedPage)
	val romRequiredMessage = stringResource(R.string.msg_rom_required_before_start)
	val configWriteFailedMessage = stringResource(R.string.msg_failed_save_config)
	val launchFailedMessage = stringResource(R.string.msg_failed_launch_config)
	fun saveMessage(result: ConfigurationSaveActions.SaveResult): String {
		val message = ConfigurationSaveActions.messageFor(result)
		return message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)
	}

	/** Open the Save dialog: an open configuration's name, else one made from the model's. */
	fun openSaveAs() {
		scope.launch { saveAsName = viewModel.currentConfigName ?: viewModel.suggestedConfigName() }
	}

	/** Save: to the open configuration's file, or by name for a new one. */
	fun save() {
		if (viewModel.currentConfigName == null) {
			openSaveAs()
		} else {
			scope.launch {
				val result = viewModel.saveTracked()
				snackbarHostState.showSnackbar(saveMessage(result))
			}
		}
	}

	// Another screen switched here with a button (the Library's Edit or New), whose
	// focus is gone with it: D-pad focus starts on the page list or the side list
	// instead of falling back to the navigation rail. Entering starts at the list —
	// the category focus is attached to the phone's page list or the side list, and
	// a page left open would leave it attached nowhere (a page's content focus is
	// only requested once composed; every other requestFocus target in this screen
	// — the overflow button, the dialogs' buttons, the hard-drive rows' — is
	// composed before its effect can run).
	LaunchedEffect(viewModel.focusOnEntry) {
		if (!viewModel.focusOnEntry) return@LaunchedEffect
		viewModel.focusOnEntry = false
		if (keyboardMode) {
			pageOpen = false
			withFrameNanos { }
			categoryFocus.requestFocus()
		}
	}

	// Saving disables the focused Save button, which drops D-pad focus to the navigation
	// rail; move it to the overflow button beside it instead.
	val saveEnabled = viewModel.currentConfigName == null || viewModel.isDirty
	var saveHadFocus by remember { mutableStateOf(false) }
	val moreOptionsFocus = remember { FocusRequester() }
	LaunchedEffect(saveEnabled) {
		if (!saveEnabled && saveHadFocus) {
			saveHadFocus = false
			moreOptionsFocus.requestFocus()
		}
	}

	// A page open on a phone: Back returns to the list of pages (the shell's
	// unsaved-changes question takes Back from the list).
	BackHandler(enabled = !wide && pageOpen) { pageOpen = false }

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = {
					Column {
						// The machine being edited, the page under it (the Apple
						// editor's window title).
						Text(viewModel.currentConfigName ?: stringResource(R.string.settings_new_configuration))
						if (wide || pageOpen) {
							Text(
								text = stringResource(page.titleRes),
								style = MaterialTheme.typography.labelSmall,
								color = MaterialTheme.colorScheme.onSurfaceVariant
							)
						}
						// New configurations too: their changes from the model's preset are kept only by Save.
						if (viewModel.hasUnsavedChanges) {
							Text(
								text = stringResource(R.string.settings_unsaved_changes),
								style = MaterialTheme.typography.labelSmall,
								color = MaterialTheme.colorScheme.onSurfaceVariant
							)
						}
					}
				},
				actions = {
					TextButton(
						onClick = ::save,
						enabled = saveEnabled,
						modifier = Modifier
							// Only while enabled: losing focus by being disabled must not clear it.
							.onFocusChanged { if (saveEnabled) saveHadFocus = it.isFocused }
							.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.action_save))
					}
					IconButton(
						onClick = { showTopBarMenu = true },
						modifier = Modifier.focusRequester(moreOptionsFocus).dpadFocusIndicator()
					) {
						Icon(Icons.Default.MoreVert, contentDescription = stringResource(R.string.more_options))
					}
					DropdownMenu(
						expanded = showTopBarMenu,
						onDismissRequest = { showTopBarMenu = false }
					) {
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_review_changes)) },
							modifier = Modifier.dpadFocusIndicator(),
							leadingIcon = { Icon(Icons.Default.Info, contentDescription = null) },
							enabled = viewModel.hasUnsavedChanges,
							onClick = {
								showTopBarMenu = false
								showReviewChangesDialog = true
							}
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_save_as)) },
							modifier = Modifier.dpadFocusIndicator(),
							leadingIcon = { Icon(Icons.Default.Save, contentDescription = null) },
							onClick = {
								showTopBarMenu = false
								openSaveAs()
							}
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_revert)) },
							modifier = Modifier.dpadFocusIndicator(),
							leadingIcon = { Icon(Icons.AutoMirrored.Filled.Undo, contentDescription = null) },
							enabled = viewModel.hasUnsavedChanges,
							onClick = {
								showTopBarMenu = false
								showRevertDialog = true
							}
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_quick_setups)) },
							modifier = Modifier.dpadFocusIndicator(),
							leadingIcon = { Icon(Icons.Default.AutoFixHigh, contentDescription = null) },
							onClick = {
								showTopBarMenu = false
								showQuickSetups = true
							}
						)
					}
				}
			)
		},
		floatingActionButton = {
			ExtendedFloatingActionButton(
				onClick = {
					if (launchInProgress) {
						return@ExtendedFloatingActionButton
					}
					if (!canStart) {
						scope.launch {
							snackbarHostState.showSnackbar(romRequiredMessage)
						}
						return@ExtendedFloatingActionButton
					}
					startGuard.confirmStart {
						val launchSettings = viewModel.settings
						val launchUnknownLines = viewModel.currentUnknownLines
						scope.launchGuarded(launchGuard) {
							val configFile = try {
								withContext(Dispatchers.IO) {
									viewModel.writeSettingsConfig(
										launchSettings,
										".current_settings.uae",
										launchUnknownLines
									)
								}
							} catch (_: Exception) {
								snackbarHostState.showSnackbar(configWriteFailedMessage)
								return@launchGuarded
							}
							runCatching {
								EmulatorLauncher.launchSettingsConfig(context, configFile.absolutePath)
							}.onFailure {
								snackbarHostState.showSnackbar(launchFailedMessage)
							}
						}
					}
				},
				modifier = Modifier
					// With the phone's page list showing, Up lands on the list, not
					// on a page that is not composed.
					.focusProperties { up = if (wide || pageOpen) pageContentFocus else categoryFocus }
					.dpadFocusIndicator(),
				icon = {
					if (launchInProgress) {
						CircularProgressIndicator(
							modifier = Modifier.size(18.dp),
							strokeWidth = 2.dp
						)
					} else {
						Icon(Icons.Default.PlayArrow, contentDescription = null)
					}
				},
				text = {
					Text(
						stringResource(
							if (launchInProgress) R.string.action_launching else R.string.action_start
						)
					)
				}
			)
		}
	) { innerPadding ->
		val onImportRom = {
			if (!importInProgress) {
				romPicker.pickFiles()
			}
		}
		val onImportResult: (FileManager.ImportResult) -> Unit = { result ->
			scope.launch {
				snackbarHostState.showSnackbar(importResultMessage(result))
			}
		}
		if (wide) {
			// Wide windows and the TV list the pages down the left, the selected page beside them.
			Row(
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
			) {
				SidePaneList(
					entries = pages.map { entry ->
						SidePaneEntry(
							label = stringResource(entry.titleRes),
							badge = if (entry in editedPages && entry != SettingsPage.AllSettings) EDITED_DOT else null,
							testTag = entry.testTag
						)
					},
					selectedIndex = validSelectedPage,
					onSelect = { selectedPage = it },
					modifier = Modifier.focusRequester(categoryFocus).contentEntryFocus(),
					contentFocus = pageContentFocus,
					selectOnFocus = true
				)
				VerticalDivider()
				Column(modifier = Modifier.weight(1f).fillMaxHeight()) {
					SettingsEditorPage(
						viewModel = viewModel,
						page = page,
						canStart = canStart,
						importInProgress = importInProgress,
						onImportRom = onImportRom,
						onImportResult = onImportResult,
						onEditMachine = ::openSaveAs,
						pageContentFocus = pageContentFocus
					)
				}
			}
		} else if (!pageOpen) {
			// A phone lists the pages; tapping one opens it, and Back returns here.
			PageList(
				pages = pages,
				editedPages = editedPages,
				onOpen = { index ->
					selectedPage = index
					pageOpen = true
				},
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
					.focusRequester(categoryFocus)
					.contentEntryFocus()
			)
		} else {
			Column(
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
			) {
				SettingsEditorPage(
					viewModel = viewModel,
					page = page,
					canStart = canStart,
					importInProgress = importInProgress,
					onImportRom = onImportRom,
					onImportResult = onImportResult,
					onEditMachine = ::openSaveAs,
					pageContentFocus = pageContentFocus
				)
			}
		}

		saveAsName?.let { initialName ->
			SaveConfigDialog(
				initialName = initialName,
				initialDescription = viewModel.currentConfigDescription,
				onDismiss = { saveAsName = null },
				onSave = { name, description ->
					scope.launch {
						when (val result = viewModel.saveAs(name, description, allowOverwrite = false)) {
							is ConfigurationSaveActions.SaveResult.Saved -> {
								saveAsName = null
								snackbarHostState.showSnackbar(saveMessage(result))
							}
							ConfigurationSaveActions.SaveResult.AlreadyExists ->
								overwriteRequest = OverwriteRequest(name, description)
							else -> snackbarHostState.showSnackbar(saveMessage(result))
						}
					}
				}
			)
		}

		if (showReviewChangesDialog) {
			SettingsChangesDialog(
				title = if (viewModel.currentConfigName == null) {
					stringResource(R.string.dialog_review_changes_new_title, viewModel.settings.baseModel.displayName)
				} else {
					stringResource(R.string.dialog_review_changes_title)
				},
				changes = viewModel.changeSummary,
				onRevert = viewModel::revertChange,
				onDiscardAll = {
					showReviewChangesDialog = false
					viewModel.discardChanges()
				},
				onSave = {
					showReviewChangesDialog = false
					save()
				},
				onDismiss = { showReviewChangesDialog = false }
			)
		}

		if (showRevertDialog) {
			AlertDialog(
				onDismissRequest = { showRevertDialog = false },
				title = { Text(stringResource(R.string.dialog_revert_title)) },
				text = {
					val configName = viewModel.currentConfigName
					Text(
						if (configName != null) {
							stringResource(R.string.dialog_revert_message_saved, configName)
						} else {
							stringResource(R.string.dialog_revert_message_preset, viewModel.settings.baseModel.displayName)
						}
					)
				},
				confirmButton = {
					TextButton(
						onClick = {
							showRevertDialog = false
							viewModel.discardChanges()
						},
						modifier = Modifier.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.action_revert))
					}
				},
				dismissButton = {
					TextButton(
						onClick = { showRevertDialog = false },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.action_cancel))
					}
				}
			)
		}

		if (showQuickSetups) {
			val appliedMessage = stringResource(R.string.msg_quick_setup_applied)
			// Touch controls mean nothing on a TV (its emulator reports a touchscreen it has not).
			val offered = if (isTvDevice()) {
				SettingsIntentPreset.entries.filter { it != SettingsIntentPreset.TouchControls }
			} else {
				SettingsIntentPreset.entries
			}
			val setupNames = offered.associateWith { stringResource(it.titleRes()) }
			QuickSetupsDialog(
				setups = offered,
				selected = viewModel.selectedIntentPreset,
				onApply = { preset ->
					showQuickSetups = false
					viewModel.applyIntentPreset(preset)
					scope.launch { snackbarHostState.showSnackbar(appliedMessage.format(setupNames.getValue(preset))) }
				},
				onDismiss = { showQuickSetups = false }
			)
		}

		overwriteRequest?.let { request ->
			AlertDialog(
				onDismissRequest = { overwriteRequest = null },
				title = { Text(stringResource(R.string.dialog_overwrite_config_title)) },
				text = { Text(stringResource(R.string.dialog_overwrite_config_message, request.name)) },
				confirmButton = {
					TextButton(onClick = {
						scope.launch {
							val result = viewModel.saveAs(request.name, request.description, allowOverwrite = true)
							overwriteRequest = null
							if (result is ConfigurationSaveActions.SaveResult.Saved) {
								saveAsName = null
							}
							snackbarHostState.showSnackbar(saveMessage(result))
						}
					}, modifier = Modifier.dpadFocusIndicator()) {
						Text(stringResource(R.string.action_overwrite))
					}
				},
				dismissButton = {
					TextButton(
						onClick = { overwriteRequest = null },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.action_cancel))
					}
				}
			)
		}
	}
}


/** The dot that marks a page with edits in the side list (the Apple app's sidebar mark). */
private const val EDITED_DOT = "•"

/**
 * The phone's list of the editor's pages, one row per page, a dot marking the
 * pages with edits. Tapping (or OK on) a row opens it; Back from the page
 * returns here.
 */
@Composable
private fun PageList(
	pages: List<SettingsPage>,
	editedPages: Set<SettingsPage>,
	onOpen: (Int) -> Unit,
	modifier: Modifier = Modifier
) {
	val firstRowFocus = remember { FocusRequester() }
	val editedLabel = stringResource(R.string.settings_page_edited_dot)
	Column(
		modifier = modifier
			.verticalScroll(rememberScrollState())
			// Entering the list lands on the first page.
			.focusRestorer(firstRowFocus)
			.padding(horizontal = 16.dp, vertical = 8.dp),
		verticalArrangement = Arrangement.spacedBy(4.dp)
	) {
		pages.forEachIndexed { index, page ->
			// All Settings lists every changed option, so it is not marked.
			val edited = page in editedPages && page != SettingsPage.AllSettings
			Row(
				verticalAlignment = Alignment.CenterVertically,
				modifier = Modifier
					.fillMaxWidth()
					// Entering the list lands on the first page.
					.then(if (index == 0) Modifier.focusRequester(firstRowFocus) else Modifier)
					.dpadFocusIndicator()
					.clickable(role = Role.Button) { onOpen(index) }
					.padding(vertical = 12.dp)
					.testTag(page.testTag)
			) {
				Text(
					stringResource(page.titleRes),
					style = MaterialTheme.typography.titleMedium,
					modifier = Modifier.weight(1f)
				)
				if (edited) {
					Box(
						modifier = Modifier
							.padding(start = 8.dp)
							.size(8.dp)
							.background(MaterialTheme.colorScheme.primary, MaterialTheme.shapes.small)
							.semantics { contentDescription = editedLabel }
					)
				}
			}
		}
	}
}

/**
 * Everything under the page chooser: the adjustment notices, the missing-ROM card and
 * the page. Beside the side list on a wide window, below the top bar on a phone with
 * a page open.
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun ColumnScope.SettingsEditorPage(
	viewModel: SettingsViewModel,
	page: SettingsPage,
	canStart: Boolean,
	importInProgress: Boolean,
	onImportRom: () -> Unit,
	onImportResult: (FileManager.ImportResult) -> Unit,
	onEditMachine: () -> Unit,
	pageContentFocus: FocusRequester
) {
	SettingsAdjustmentBanner(
		notices = viewModel.adjustmentNotices,
		onDismiss = viewModel::clearAdjustmentNotices
	)

	if (!canStart) {
		Card(
			modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 8.dp),
			colors = CardDefaults.cardColors(
				containerColor = MaterialTheme.colorScheme.errorContainer
			)
		) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(
						Icons.Default.Warning,
						contentDescription = null,
						modifier = Modifier.size(24.dp),
						tint = MaterialTheme.colorScheme.onErrorContainer
					)
					Spacer(modifier = Modifier.width(8.dp))
					Text(
						stringResource(R.string.setup_required_title),
						style = MaterialTheme.typography.titleMedium,
						color = MaterialTheme.colorScheme.onErrorContainer
					)
				}
				Spacer(modifier = Modifier.height(8.dp))
				Text(
					text = stringResource(R.string.setup_required_message),
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onErrorContainer
				)
				Spacer(modifier = Modifier.height(12.dp))
				Button(
					onClick = onImportRom,
					enabled = !importInProgress,
					colors = ButtonDefaults.buttonColors(
						containerColor = MaterialTheme.colorScheme.onErrorContainer,
						contentColor = MaterialTheme.colorScheme.errorContainer
					),
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(
						stringResource(
							if (importInProgress) R.string.action_importing else R.string.action_import_rom
						)
					)
				}
			}
		}
	}

	// The Start button floats over the bottom of the page: a control D-pad or keyboard
	// focus scrolls to stops above it instead of behind it. A TV keeps Compose's own
	// leanback scrolling, which already holds the focused control near the top third.
	val clearOfStart = with(LocalDensity.current) { START_BUTTON_CLEARANCE.toPx() }
	val platformSpec = LocalBringIntoViewSpec.current
	val tv = isTvDevice()
	val bringIntoViewSpec = remember(clearOfStart, tv, platformSpec) {
		if (tv) platformSpec else ClearOfBottomBringIntoViewSpec(clearOfStart, platformSpec)
	}
	if (page == SettingsPage.AllSettings) {
		// The page's list carries the focus itself (a LazyColumn is its own focus group).
		CompositionLocalProvider(LocalBringIntoViewSpec provides bringIntoViewSpec) {
			AllSettingsPage(viewModel = viewModel, pageFocus = pageContentFocus)
		}
	} else {
		Column(
			modifier = Modifier
				.weight(1f)
				.fillMaxWidth()
				.focusRequester(pageContentFocus)
				.focusRestorer()
		) {
			CompositionLocalProvider(LocalBringIntoViewSpec provides bringIntoViewSpec) {
				when (page) {
					SettingsPage.Overview -> OverviewPage(
						viewModel = viewModel,
						onEditMachine = onEditMachine,
						importInProgress = importInProgress,
						onImportResult = onImportResult
					)
					SettingsPage.Hardware -> HardwarePage(viewModel)
					SettingsPage.DisplaySound -> DisplaySoundPage(viewModel)
					SettingsPage.Input -> InputPage(viewModel)
					SettingsPage.Storage -> StoragePage(viewModel = viewModel, onImportResult = onImportResult)
					SettingsPage.PortsMisc -> PortsMiscPage(viewModel)
					SettingsPage.AllSettings -> Unit
				}
			}
		}
	}
}

/** The Start button's height and margins, which a focused control must stay above. */
private val START_BUTTON_CLEARANCE = 88.dp

/**
 * Scrolls a focused control into the part of the page [bottomPx] above its bottom edge
 * (where the Start button floats), as little as possible: a control already there stays
 * put, one above it comes down to the top, one below it comes up, never past its own top.
 * What cannot fit above the button gets the [platform] rule. This includes everything in
 * the scrollers nested in the page, such as a text field's own, which are barely taller
 * than the button: scrolling one never cleared the button, so the request repeated every
 * frame and ran the page to its end (opening the Amiga model menu did).
 */
@OptIn(ExperimentalFoundationApi::class)
internal class ClearOfBottomBringIntoViewSpec(
	private val bottomPx: Float,
	private val platform: BringIntoViewSpec
) : BringIntoViewSpec {
	override fun calculateScrollDistance(offset: Float, size: Float, containerSize: Float): Float {
		val clear = containerSize - bottomPx
		if (size > clear) return platform.calculateScrollDistance(offset, size, containerSize)
		val trailingEdge = offset + size
		return when {
			offset >= 0f && trailingEdge <= clear -> 0f
			offset < 0f -> offset
			else -> min(offset, trailingEdge - clear)
		}
	}
}

/**
 * The quick setups: named bundles of display and control settings applied in one go
 * (not machines: the Amiga model picker on the Overview page chooses those).
 */
@Composable
private fun QuickSetupsDialog(
	setups: List<SettingsIntentPreset>,
	selected: SettingsIntentPreset?,
	onApply: (SettingsIntentPreset) -> Unit,
	onDismiss: () -> Unit
) {
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.settings_quick_setups_title)) },
		text = {
			Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
				setups.forEach { setup ->
					Row(
						verticalAlignment = Alignment.CenterVertically,
						modifier = Modifier
							.fillMaxWidth()
							// Before selectable: onFocusChanged reports the focus target after it in the chain.
							.dpadFocusIndicator()
							.selectable(selected = setup == selected, role = Role.Button) { onApply(setup) }
							.padding(8.dp)
					) {
						Column(modifier = Modifier.weight(1f)) {
							Text(stringResource(setup.titleRes()), style = MaterialTheme.typography.titleSmall)
							Text(
								stringResource(setup.descriptionRes()),
								style = MaterialTheme.typography.bodySmall,
								color = MaterialTheme.colorScheme.onSurfaceVariant
							)
						}
						if (setup == selected) {
							Icon(Icons.Default.Check, contentDescription = null, tint = MaterialTheme.colorScheme.primary)
						}
					}
				}
			}
		},
		confirmButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_close))
			}
		}
	)
}

@Composable
private fun SettingsAdjustmentBanner(
	notices: List<SettingsAdjustmentNotice>,
	onDismiss: () -> Unit
) {
	if (notices.isEmpty()) return

	Card(
		modifier = Modifier
			.fillMaxWidth()
			.padding(horizontal = 16.dp, vertical = 8.dp),
		colors = CardDefaults.cardColors(
			containerColor = MaterialTheme.colorScheme.secondaryContainer
		)
	) {
		Column(
			modifier = Modifier.padding(16.dp),
			verticalArrangement = Arrangement.spacedBy(8.dp)
		) {
			Row(verticalAlignment = Alignment.CenterVertically) {
				Icon(
					Icons.Default.Info,
					contentDescription = null,
					modifier = Modifier.size(22.dp),
					tint = MaterialTheme.colorScheme.onSecondaryContainer
				)
				Spacer(modifier = Modifier.width(8.dp))
				Text(
					stringResource(R.string.settings_adjustments_title),
					style = MaterialTheme.typography.titleMedium,
					color = MaterialTheme.colorScheme.onSecondaryContainer
				)
			}
			notices.take(4).forEach { notice ->
				Text(
					notice.message(),
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onSecondaryContainer
				)
			}
			if (notices.size > 4) {
				Text(
					stringResource(R.string.settings_adjustments_more, notices.size - 4),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSecondaryContainer
				)
			}
			TextButton(
				onClick = onDismiss,
				modifier = Modifier.dpadFocusIndicator()
			) {
				Text(stringResource(R.string.action_dismiss))
			}
		}
	}
}

private fun SettingsIntentPreset.titleRes(): Int = when (this) {
	SettingsIntentPreset.PixelPerfect -> R.string.settings_preset_pixel_perfect
	SettingsIntentPreset.TouchControls -> R.string.settings_preset_touch_controls
}

private fun SettingsIntentPreset.descriptionRes(): Int = when (this) {
	SettingsIntentPreset.PixelPerfect -> R.string.settings_preset_pixel_perfect_desc
	SettingsIntentPreset.TouchControls -> R.string.settings_preset_touch_controls_desc
}

/**
 * What a notice says: for the emulator's own rules the option's schema
 * label and constraint, for the launcher's rules its own message.
 */
@Composable
private fun SettingsAdjustmentNotice.message(): String {
	val option = optionId?.let { settingsSchema()?.option(it) }
	if (option != null) {
		val constraint = option.constraint
		return if (constraint != null) {
			stringResource(R.string.settings_adjustment_option_reason, option.label, constraint)
		} else {
			stringResource(R.string.settings_adjustment_option, option.label)
		}
	}
	return stringResource(
		when (this) {
			SettingsAdjustmentNotice.AgaRequires68020 -> R.string.settings_adjustment_aga_cpu
			SettingsAdjustmentNotice.IntegerScalingSynced -> R.string.settings_adjustment_integer_scaling
			SettingsAdjustmentNotice.OnScreenControlsRequireTouch -> R.string.settings_adjustment_touch_required
			SettingsAdjustmentNotice.OnScreenJoystickMovedToPort1 -> R.string.settings_adjustment_touch_port
			SettingsAdjustmentNotice.InvalidInputDeviceReset -> R.string.settings_adjustment_input_reset
			// The emulator's rules, should the schema not list their option.
			SettingsAdjustmentNotice.AddressingChanged,
			SettingsAdjustmentNotice.JitTurnedOff,
			SettingsAdjustmentNotice.Z3MemoryRemoved,
			SettingsAdjustmentNotice.SoundOutputRaised -> R.string.settings_adjustments_title
		}
	)
}

/**
 * The review list: each changed setting with its old and new value, a Revert for the
 * layer's options, and Discard all / Save for the lot.
 */
@Composable
private fun SettingsChangesDialog(
	title: String,
	changes: List<SettingsChange>,
	onRevert: (SettingsChange) -> Unit,
	onDiscardAll: () -> Unit,
	onSave: () -> Unit,
	onDismiss: () -> Unit
) {
	// A revert removes its row and the focused Revert button with it: D-pad focus
	// moves to Close rather than dropping out of the dialog.
	val closeFocus = remember { FocusRequester() }
	var shownCount by remember { mutableIntStateOf(changes.size) }
	var revertHadFocus by remember { mutableStateOf(false) }
	LaunchedEffect(changes.size) {
		if (changes.size < shownCount && revertHadFocus) {
			revertHadFocus = false
			closeFocus.requestFocus()
		}
		shownCount = changes.size
	}
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(title) },
		text = {
			Column(
				modifier = Modifier.verticalScroll(rememberScrollState()),
				verticalArrangement = Arrangement.spacedBy(8.dp)
			) {
				if (changes.isEmpty()) {
					Text(stringResource(R.string.dialog_review_changes_empty))
				} else {
					changes.forEach { change ->
						val label = settingText(change.label)
						val values: @Composable () -> Unit = {
							Text(label, style = MaterialTheme.typography.labelLarge)
							Text(
								stringResource(
									R.string.dialog_review_changes_row,
									settingText(change.before),
									settingText(change.after)
								),
								style = MaterialTheme.typography.bodySmall,
								color = MaterialTheme.colorScheme.onSurfaceVariant
							)
						}
						if (change.optionId == null) {
							// Focusable rows: D-pad Up/Down scroll a list longer than the dialog.
							Column(modifier = Modifier.fillMaxWidth().dpadFocusable().padding(4.dp)) { values() }
						} else {
							val revertLabel = stringResource(R.string.action_revert_named, label)
							Row(
								modifier = Modifier.fillMaxWidth().padding(start = 4.dp),
								verticalAlignment = Alignment.CenterVertically
							) {
								Column(modifier = Modifier.weight(1f)) { values() }
								TextButton(
									onClick = { onRevert(change) },
									modifier = Modifier
										.onFocusChanged { if (it.isFocused) revertHadFocus = true }
										.dpadFocusIndicator()
										.semantics { contentDescription = revertLabel }
								) {
									Text(stringResource(R.string.action_revert))
								}
							}
						}
					}
				}
			}
		},
		confirmButton = {
			TextButton(onClick = onSave, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_save))
			}
		},
		dismissButton = {
			Row {
				TextButton(onClick = onDiscardAll, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_discard_all))
				}
				TextButton(
					onClick = onDismiss,
					modifier = Modifier.focusRequester(closeFocus).dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_close))
				}
			}
		}
	)
}

private data class OverwriteRequest(val name: String, val description: String)

/**
 * Asks for a configuration's name (prefilled with [initialName] and selected, so typing
 * replaces it) and description. On a TV the Save button starts with D-pad focus, so OK
 * saves under the offered name without the on-screen keyboard.
 */
@Composable
fun SaveConfigDialog(
	initialName: String,
	initialDescription: String,
	onDismiss: () -> Unit,
	onSave: (String, String) -> Unit
) {
	var name by remember {
		mutableStateOf(TextFieldValue(initialName, selection = TextRange(0, initialName.length)))
	}
	var description by remember { mutableStateOf(initialDescription) }
	val saveFocus = remember { FocusRequester() }
	val tv = isTvDevice()
	LaunchedEffect(Unit) {
		if (tv && name.text.isNotBlank()) {
			withFrameNanos { }
			saveFocus.requestFocus()
		}
	}
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.dialog_save_config_title)) },
		text = {
			// D-pad Up/Down move between the fields and the buttons; OK on a field opens the
			// IME. Created here, inside the dialog's window, so the gates drive its keyboard.
			val nameTyping = rememberDpadTypingGate()
			val descriptionTyping = rememberDpadTypingGate()
			Column {
				Text(stringResource(R.string.dialog_save_config_message))
				Spacer(modifier = Modifier.height(8.dp))
				OutlinedTextField(
					value = name,
					onValueChange = { name = it },
					label = { Text(stringResource(R.string.label_configuration_name)) },
					singleLine = true,
					readOnly = nameTyping.readOnly,
					keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
					keyboardActions = KeyboardActions(onDone = { nameTyping.stopTyping() }),
					modifier = Modifier.dpadVerticalMovesFocus().then(nameTyping.modifier)
				)
				Spacer(modifier = Modifier.height(8.dp))
				OutlinedTextField(
					value = description,
					onValueChange = { description = it },
					label = { Text(stringResource(R.string.label_configuration_description)) },
					singleLine = true,
					readOnly = descriptionTyping.readOnly,
					keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
					keyboardActions = KeyboardActions(onDone = { descriptionTyping.stopTyping() }),
					modifier = Modifier.dpadVerticalMovesFocus().then(descriptionTyping.modifier)
				)
			}
		},
		confirmButton = {
			TextButton(
				onClick = { onSave(name.text.trim(), description.trim()) },
				enabled = name.text.isNotBlank(),
				modifier = Modifier.focusRequester(saveFocus).dpadFocusIndicator()
			) {
				Text(stringResource(R.string.action_save))
			}
		},
		dismissButton = {
			TextButton(
				onClick = onDismiss,
				modifier = Modifier.dpadFocusIndicator()
			) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}
