package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.gestures.BringIntoViewSpec
import androidx.compose.foundation.gestures.LocalBringIntoViewSpec
import androidx.compose.foundation.layout.Arrangement
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
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AutoFixHigh
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Warning
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
import androidx.compose.material3.PrimaryScrollableTabRow
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Tab
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
import com.blitterstudio.amiberry.data.AdvancedLaunchActions
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
	var selectedTab by rememberSaveable { mutableIntStateOf(0) }
	// The name the Save dialog opens with; null while it is closed.
	var saveAsName by remember { mutableStateOf<String?>(null) }
	var overwriteRequest by remember { mutableStateOf<OverwriteRequest?>(null) }
	var showTopBarMenu by remember { mutableStateOf(false) }
	var showReviewChangesDialog by remember { mutableStateOf(false) }
	var showQuickSetups by remember { mutableStateOf(false) }
	val availableRoms by viewModel.availableRoms.collectAsState()
	val canStart = viewModel.settings.romFile.isNotBlank() || availableRoms.isNotEmpty()
	val startGuard = LocalStartGuard.current
	// Up from Start returns to the control last focused in the tab; the 2D search
	// would jump over the scrolled tab to the controls above it.
	val tabContentFocus = remember { FocusRequester() }
	// The tab row or the category list, where D-pad focus enters the screen.
	val categoryFocus = remember { FocusRequester() }
	val keyboardMode = LocalInputModeManager.current.inputMode == InputMode.Keyboard
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

	val tabs = SettingsTabs.all
	val validSelectedTab = SettingsTabs.validSelectedIndex(selectedTab)
	val romRequiredMessage = stringResource(R.string.msg_rom_required_before_start)
	val configWriteFailedMessage = stringResource(R.string.msg_failed_save_config)
	val launchFailedMessage = stringResource(R.string.msg_failed_launch_config)
	val advancedLaunchFailedMessage = stringResource(AdvancedLaunchActions.launchFailedMessage().stringRes)
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

	// Another screen switched here with a button (Configurations' Edit or New), whose
	// focus is gone with it: D-pad focus starts on the tab row or the category list
	// instead of falling back to the navigation rail.
	LaunchedEffect(viewModel.focusOnEntry) {
		if (!viewModel.focusOnEntry) return@LaunchedEffect
		viewModel.focusOnEntry = false
		if (keyboardMode) {
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

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = {
					Column {
						Text(viewModel.currentConfigName ?: stringResource(R.string.settings_new_configuration))
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
							leadingIcon = { Icon(Icons.Default.Info, contentDescription = null) },
							enabled = viewModel.hasUnsavedChanges,
							onClick = {
								showTopBarMenu = false
								showReviewChangesDialog = true
							}
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_save_as)) },
							leadingIcon = { Icon(Icons.Default.Save, contentDescription = null) },
							onClick = {
								showTopBarMenu = false
								openSaveAs()
							}
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_quick_setups)) },
							leadingIcon = { Icon(Icons.Default.AutoFixHigh, contentDescription = null) },
							onClick = {
								showTopBarMenu = false
								showQuickSetups = true
							}
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_advanced)) },
							leadingIcon = { Icon(Icons.Default.Settings, contentDescription = null) },
							enabled = !launchInProgress,
							onClick = {
								showTopBarMenu = false
								val advancedSettings = viewModel.settings
								val advancedUnknownLines = viewModel.currentUnknownLines
								scope.launchGuarded(launchGuard) {
									val configFile = try {
										withContext(Dispatchers.IO) {
											viewModel.writeSettingsConfig(
												advancedSettings,
												AdvancedLaunchActions.SETTINGS_ADVANCED_CONFIG,
												advancedUnknownLines
											)
										}
									} catch (_: Exception) {
										snackbarHostState.showSnackbar(advancedLaunchFailedMessage)
										return@launchGuarded
									}
									runCatching {
										EmulatorLauncher.launchAdvancedGui(context, configFile.absolutePath)
									}.onFailure {
										snackbarHostState.showSnackbar(advancedLaunchFailedMessage)
									}
								}
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
					.focusProperties { up = tabContentFocus }
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
		if (windowWidthClass().showsSideList) {
			// Wide windows list the tabs down the left, the selected tab's page beside them.
			Row(
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
			) {
				SidePaneList(
					entries = tabs.map { tab -> SidePaneEntry(stringResource(tab.titleRes), testTag = tab.testTag) },
					selectedIndex = validSelectedTab,
					onSelect = { selectedTab = it },
					modifier = Modifier.focusRequester(categoryFocus).contentEntryFocus(),
					contentFocus = tabContentFocus,
					selectOnFocus = true
				)
				VerticalDivider()
				Column(modifier = Modifier.weight(1f).fillMaxHeight()) {
					SettingsTabPage(
						viewModel = viewModel,
						tab = SettingsTabs.tabFor(selectedTab),
						canStart = canStart,
						importInProgress = importInProgress,
						onImportRom = onImportRom,
						onImportResult = onImportResult,
						tabContentFocus = tabContentFocus
					)
				}
			}
		} else {
			Column(
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
			) {
				val tabFocus = remember(tabs.size) { List(tabs.size) { FocusRequester() } }
				PrimaryScrollableTabRow(
					selectedTabIndex = validSelectedTab,
					modifier = Modifier
						.fillMaxWidth()
						.focusRequester(categoryFocus)
						.contentEntryFocus()
						// Entering the row lands on the selected tab.
						.focusRestorer(tabFocus[validSelectedTab])
				) {
					tabs.forEachIndexed { index, tab ->
						Tab(
							selected = validSelectedTab == index,
							onClick = { selectedTab = index },
							modifier = Modifier.focusRequester(tabFocus[index]).testTag(tab.testTag).dpadFocusIndicator(),
							text = { Text(stringResource(tab.titleRes)) }
						)
					}
				}
				SettingsTabPage(
					viewModel = viewModel,
					tab = SettingsTabs.tabFor(selectedTab),
					canStart = canStart,
					importInProgress = importInProgress,
					onImportRom = onImportRom,
					onImportResult = onImportResult,
					tabContentFocus = tabContentFocus
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

/**
 * Everything under the tab chooser: the adjustment notices, the missing-ROM card and
 * the selected tab's page. Below the tab row on a phone, beside the tab list on a
 * wide window.
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun ColumnScope.SettingsTabPage(
	viewModel: SettingsViewModel,
	tab: SettingsTab,
	canStart: Boolean,
	importInProgress: Boolean,
	onImportRom: () -> Unit,
	onImportResult: (FileManager.ImportResult) -> Unit,
	tabContentFocus: FocusRequester
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
	Column(
		modifier = Modifier
			.weight(1f)
			.fillMaxWidth()
			.focusRequester(tabContentFocus)
			.focusRestorer()
	) {
		CompositionLocalProvider(LocalBringIntoViewSpec provides bringIntoViewSpec) {
			when (tab) {
				SettingsTab.Cpu -> CpuTab(viewModel)
				SettingsTab.Chipset -> ChipsetTab(viewModel)
				SettingsTab.Memory -> MemoryTab(viewModel)
				SettingsTab.Display -> DisplayTab(viewModel)
				SettingsTab.Sound -> SoundTab(viewModel)
				SettingsTab.Input -> InputTab(viewModel)
				SettingsTab.Storage -> StorageTab(viewModel = viewModel, onImportResult = onImportResult)
				SettingsTab.Misc -> MiscTab(viewModel)
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
 * What cannot fit above the button gets the [platform] rule. That includes everything in
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
 * (not machines: the Amiga model picker on the CPU tab chooses those).
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
