package com.blitterstudio.amiberry.ui.screens

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.combinedClickable
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
import androidx.compose.foundation.layout.wrapContentWidth
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.produceState
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.components.SettingsSummaryCard
import com.blitterstudio.amiberry.ui.windowWidthClass
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.Button
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.DisposableEffect
import androidx.compose.material3.Surface
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import com.blitterstudio.amiberry.data.ConfigMachine
import com.blitterstudio.amiberry.data.ConfigurationSaveActions
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.isTvDevice
import com.blitterstudio.amiberry.ui.screens.settings.SaveConfigDialog
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import android.content.Intent
import androidx.core.content.FileProvider
import com.blitterstudio.amiberry.data.AdvancedLaunchActions
import kotlinx.coroutines.CancellationException
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.launchGuarded
import androidx.navigation.NavController
import androidx.navigation.NavGraph.Companion.findStartDestination
import com.blitterstudio.amiberry.ui.navigation.Screen
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.ConfigurationsViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.launch
import java.text.DateFormat
import java.util.Date
import java.util.Locale
import java.io.File
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.filled.DriveFileRenameOutline
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.TextFieldValue
import com.blitterstudio.amiberry.data.ConfigFiles
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate

private fun NavController.switchToTab(route: String) {
	navigate(route) {
		popUpTo(graph.findStartDestination().id) { saveState = true }
		launchSingleTop = true
		restoreState = true
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ConfigurationsScreen(
	viewModel: ConfigurationsViewModel = viewModel(),
	settingsViewModel: SettingsViewModel = viewModel(LocalContext.current.findActivity() as androidx.activity.ComponentActivity),
	navController: androidx.navigation.NavController
) {
	val context = LocalContext.current
	// Google TV has no app to share a file with.
	val isTv = isTvDevice()
	// The card list: where D-pad focus enters the screen, and Left from Launch returns to.
	val listFocus = remember { FocusRequester() }
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	val launchGuard = rememberLaunchInFlightGuard()
	val launchInProgress = launchGuard.isLaunching
	val configs by viewModel.configs.collectAsState()
	var searchQuery by rememberSaveable { mutableStateOf("") }
	var selectedFilterName by rememberSaveable { mutableStateOf(ConfigurationListFilter.All.name) }
	val selectedFilter = ConfigurationListFilter.fromName(selectedFilterName)
	val showConfigurationSearch = shouldShowConfigurationSearch(configs.size)
	val effectiveFilter = if (showConfigurationSearch) selectedFilter else ConfigurationListFilter.All
	val filteredConfigs = remember(configs, searchQuery, effectiveFilter) {
		filterConfigurations(
			configs = configs,
			query = if (showConfigurationSearch) searchQuery else "",
			filter = effectiveFilter
		)
	}
	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)

	// Files copied in while the app was away show up when the tab is shown again.
	val lifecycleOwner = LocalLifecycleOwner.current
	DisposableEffect(lifecycleOwner, viewModel) {
		val observer = LifecycleEventObserver { _, event ->
			if (event == Lifecycle.Event.ON_RESUME) viewModel.refresh()
		}
		lifecycleOwner.lifecycle.addObserver(observer)
		onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
	}

	/** Switch to Settings, D-pad focus with it (the button that had it is gone). */
	fun openSettings() {
		settingsViewModel.focusOnEntry = true
		navController.switchToTab(Screen.Settings.route)
	}

	// An action that replaces what Settings holds (Edit, New) waits here while the
	// editor has unsaved changes, a new configuration's included.
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

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.configurations_title)) },
				actions = {
					TextButton(
						onClick = {
							replaceSettings {
								settingsViewModel.newConfiguration()
								openSettings()
							}
						},
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(Icons.Default.Add, contentDescription = null, modifier = Modifier.size(18.dp))
						Spacer(modifier = Modifier.width(8.dp))
						Text(stringResource(R.string.configurations_new))
					}
				}
			)
		}
	) { innerPadding ->
		// Renaming re-keys the card and deleting removes it, either of which drops
		// D-pad focus to the navigation rail: the card named here (by file name) takes
		// focus once it is composed.
		var focusConfigFile by remember { mutableStateOf<String?>(null) }
		// The configuration the detail pane shows on a wide window, by file name.
		var selectedConfigFile by rememberSaveable { mutableStateOf<String?>(null) }
		val listDetail = windowWidthClass().showsTwoPanes
		val selectedConfig = if (listDetail) selectedConfiguration(filteredConfigs, selectedConfigFile) else null
		val detailFocus = remember { FocusRequester() }

		fun actionsFor(index: Int, config: ConfigInfo) = ConfigItemActions(
			suggestedCopyName = { viewModel.suggestedCopyName(config) },
			nameProblem = { name, renaming -> viewModel.nameProblem(name, config.takeIf { renaming }) },
			onLoad = {
				if (settingsViewModel.isConfigOpen(config.path)) {
					// Already open: its edits stay.
					openSettings()
				} else {
					replaceSettings {
						scope.launch {
							when (val result = ConfigurationActions.load(runCatching { viewModel.loadConfig(config.path) })) {
								is ConfigurationActions.LoadResult.Loaded -> {
									settingsViewModel.loadConfig(result.value, config.name, config.path)
									openSettings()
								}
								is ConfigurationActions.LoadResult.Failed -> {
									snackbarHostState.showSnackbar(actionMessage(result.message))
								}
							}
						}
					}
				}
			},
			onDelete = {
				scope.launch {
					val deleted = viewModel.deleteConfig(config.path)
					if (deleted) {
						val neighbour = filteredConfigs.getOrNull(index + 1) ?: filteredConfigs.getOrNull(index - 1)
						focusConfigFile = neighbour?.let { File(it.path).name }
						selectedConfigFile = focusConfigFile
					}
					val wasOpen = deleted && settingsViewModel.onConfigDeleted(config.path)
					snackbarHostState.showSnackbar(
						actionMessage(ConfigurationActions.deleteMessage(config.name, deleted, wasOpen))
					)
				}
			},
			onDuplicate = { newName ->
				scope.launch {
					val result = viewModel.duplicateConfig(config.path, newName)
					snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.duplicateMessage(result)))
				}
			},
			onRename = { newName ->
				scope.launch {
					val result = viewModel.renameConfig(config.path, newName)
					if (result is ConfigFiles.EditResult.Done) {
						settingsViewModel.onConfigRenamed(config.path, result.file)
						focusConfigFile = result.file.name
						if (selectedConfigFile == File(config.path).name) selectedConfigFile = result.file.name
						// Keeping the same name leaves the file as it was: nothing to report.
						if (result.file.name == File(config.path).name) return@launch
					}
					snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.renameMessage(result)))
				}
			},
			onShare = if (isTv) null else ({
				when (val target = ConfigurationActions.shareTarget(config.path)) {
					is ConfigurationActions.ShareTarget.Ready -> {
						try {
							val uri = FileProvider.getUriForFile(context, "${context.packageName}.fileprovider", target.file)
							val shareIntent = Intent(Intent.ACTION_SEND).apply {
								type = "application/octet-stream"
								putExtra(Intent.EXTRA_STREAM, uri)
								addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
							}
							context.startActivity(Intent.createChooser(shareIntent, config.name))
						} catch (e: Exception) {
							scope.launch {
								snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.shareFailedMessage()))
							}
						}
					}
					is ConfigurationActions.ShareTarget.Failed -> {
						scope.launch {
							snackbarHostState.showSnackbar(actionMessage(target.message))
						}
					}
				}
			}),
			onAdvancedEdit = {
				when (val target = AdvancedLaunchActions.savedConfigTarget(config.path)) {
					is AdvancedLaunchActions.ConfigTarget.Ready -> {
						scope.launchGuarded(launchGuard) {
							runCatching {
								EmulatorLauncher.launchAdvancedGui(context, target.path)
							}.onFailure {
								snackbarHostState.showSnackbar(
									actionMessage(AdvancedLaunchActions.launchFailedMessage())
								)
							}
						}
					}
					is AdvancedLaunchActions.ConfigTarget.Failed -> {
						scope.launch {
							snackbarHostState.showSnackbar(actionMessage(target.message))
						}
					}
				}
			},
			onLaunch = {
				val configPath = config.path
				// Not the editor state: that may belong to another configuration.
				val rememberedControls = AppPreferences.getInstance(context).rememberedAndroidControls()
				when (val target = ConfigurationActions.launchTarget(configPath)) {
					is ConfigurationActions.LaunchTarget.Ready -> {
						scope.launchGuarded(launchGuard) {
							runCatching {
								EmulatorLauncher.launchWithConfig(
									context,
									target.path,
									skipGui = true,
									rememberedControls = rememberedControls
								)
							}.onFailure {
								snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.Message(R.string.msg_failed_launch_config)))
							}
						}
					}
					is ConfigurationActions.LaunchTarget.Failed -> {
						scope.launch {
							snackbarHostState.showSnackbar(actionMessage(target.message))
						}
					}
				}
			}
		)

		// The search and the configuration cards: the whole screen on a phone, the left
		// pane on a wide window, where a card selects what the detail pane shows.
		val listPane: @Composable ColumnScope.() -> Unit = {
			if (showConfigurationSearch) {
				ConfigurationSearchControls(
					query = searchQuery,
					selectedFilter = selectedFilter,
					onQueryChange = { searchQuery = it },
					onClearSearch = { searchQuery = "" },
					onFilterSelected = { selectedFilterName = it.name }
				)
			}

			if (filteredConfigs.isEmpty()) {
				Box(
					modifier = Modifier
						.fillMaxSize()
						.padding(32.dp),
					contentAlignment = Alignment.Center
				) {
					Column(horizontalAlignment = Alignment.CenterHorizontally) {
						Text(stringResource(R.string.configurations_no_search_results), style = MaterialTheme.typography.bodyLarge)
						Spacer(modifier = Modifier.height(8.dp))
						Text(
							stringResource(R.string.configurations_search_help),
							style = MaterialTheme.typography.bodySmall,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)
					}
				}
			} else {
				LazyColumn(
					// Entering the list (from the rail, or Left from the detail pane) lands
					// on the card last focused, else the first.
					modifier = Modifier.focusRequester(listFocus).contentEntryFocus().focusRestorer(),
					contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
					verticalArrangement = Arrangement.spacedBy(8.dp)
				) {
					itemsIndexed(filteredConfigs, key = { _, config -> config.path }) { index, config ->
						val fileName = File(config.path).name
						ConfigItem(
							config = config,
							launchInProgress = launchInProgress,
							isOpenInSettings = settingsViewModel.isConfigOpen(config.path),
							focusRequested = fileName == focusConfigFile,
							onFocusRequestHandled = { focusConfigFile = null },
							actions = actionsFor(index, config),
							selection = if (listDetail) {
								ConfigItemSelection(
									selected = config == selectedConfig,
									detailFocus = detailFocus,
									onSelect = { selectedConfigFile = fileName }
								)
							} else {
								null
							}
						)
					}
				}
			}
		}

		if (configs.isEmpty()) {
			Box(
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
					.padding(32.dp),
				contentAlignment = Alignment.Center
			) {
				Column(horizontalAlignment = Alignment.CenterHorizontally) {
					Text(stringResource(R.string.configurations_empty_title), style = MaterialTheme.typography.bodyLarge)
					Spacer(modifier = Modifier.height(8.dp))
					Text(
						stringResource(R.string.configurations_empty_message),
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
					Spacer(modifier = Modifier.height(24.dp))
					Button(
						onClick = ::openSettings,
						modifier = Modifier.contentEntryFocus().dpadFocusIndicator()
					) {
						Text(stringResource(R.string.configurations_empty_cta))
					}
				}
			}
		} else if (listDetail) {
			Row(modifier = Modifier.fillMaxSize().padding(innerPadding)) {
				Column(modifier = Modifier.weight(0.45f).fillMaxHeight(), content = listPane)
				VerticalDivider()
				if (selectedConfig != null) {
					ConfigDetailPane(
						config = selectedConfig,
						loadSettings = { path -> viewModel.loadConfig(path).settings },
						isOpenInSettings = settingsViewModel.isConfigOpen(selectedConfig.path),
						launchInProgress = launchInProgress,
						launchFocus = detailFocus,
						listFocus = listFocus,
						actions = actionsFor(filteredConfigs.indexOf(selectedConfig), selectedConfig),
						modifier = Modifier.weight(0.55f).fillMaxHeight()
					)
				} else {
					Spacer(modifier = Modifier.weight(0.55f))
				}
			}
		} else {
			Column(modifier = Modifier.fillMaxSize().padding(innerPadding), content = listPane)
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
							viewModel.refresh()
							saveNewThen?.invoke()
							saveNewThen = null
						}
						snackbarHostState.showSnackbar(saveMessage(result))
					}
				}
			)
		}
	}
}

@Composable
private fun ConfigurationSearchControls(
	query: String,
	selectedFilter: ConfigurationListFilter,
	onQueryChange: (String) -> Unit,
	onClearSearch: () -> Unit,
	onFilterSelected: (ConfigurationListFilter) -> Unit
) {
	Column(
		modifier = Modifier
			.fillMaxWidth()
			.padding(horizontal = 16.dp, vertical = 8.dp),
		verticalArrangement = Arrangement.spacedBy(8.dp)
	) {
		OutlinedTextField(
			value = query,
			onValueChange = onQueryChange,
			label = { Text(stringResource(R.string.configurations_search_label)) },
			leadingIcon = { Icon(Icons.Default.Search, contentDescription = null) },
			trailingIcon = {
				if (query.isNotBlank()) {
					IconButton(
						onClick = onClearSearch,
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_clear_search))
					}
				}
			},
			singleLine = true,
			modifier = Modifier.fillMaxWidth()
		)
		Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
			ConfigurationFilterButton(
				filter = ConfigurationListFilter.All,
				selectedFilter = selectedFilter,
				modifier = Modifier.weight(1f),
				onFilterSelected = onFilterSelected
			)
			ConfigurationFilterButton(
				filter = ConfigurationListFilter.WithDescription,
				selectedFilter = selectedFilter,
				modifier = Modifier.weight(1f),
				onFilterSelected = onFilterSelected
			)
			ConfigurationFilterButton(
				filter = ConfigurationListFilter.Recent,
				selectedFilter = selectedFilter,
				modifier = Modifier.weight(1f),
				onFilterSelected = onFilterSelected
			)
		}
	}
}

@Composable
private fun ConfigurationFilterButton(
	filter: ConfigurationListFilter,
	selectedFilter: ConfigurationListFilter,
	modifier: Modifier,
	onFilterSelected: (ConfigurationListFilter) -> Unit
) {
	if (filter == selectedFilter) {
		Button(
			onClick = { onFilterSelected(filter) },
			modifier = modifier.dpadFocusIndicator()
		) {
			Text(stringResource(filter.labelRes()))
		}
	} else {
		OutlinedButton(
			onClick = { onFilterSelected(filter) },
			modifier = modifier.dpadFocusIndicator()
		) {
			Text(stringResource(filter.labelRes()))
		}
	}
}

private fun ConfigurationListFilter.labelRes(): Int = when (this) {
	ConfigurationListFilter.All -> R.string.configurations_filter_all
	ConfigurationListFilter.WithDescription -> R.string.configurations_filter_described
	ConfigurationListFilter.Recent -> R.string.configurations_filter_recent
}

/** What can be done with one configuration, from its card or the detail pane. */
private class ConfigItemActions(
	val suggestedCopyName: () -> String,
	val nameProblem: (name: String, renaming: Boolean) -> ConfigFiles.NameProblem?,
	val onLoad: () -> Unit,
	val onDelete: () -> Unit,
	val onDuplicate: (String) -> Unit,
	val onRename: (String) -> Unit,
	/** Null where no app can take a shared file (Google TV). */
	val onShare: (() -> Unit)?,
	val onAdvancedEdit: () -> Unit,
	val onLaunch: () -> Unit
)

/**
 * A card in the list-detail layout: focusing or tapping it selects it for the detail
 * pane, and D-pad Right goes to the pane's Launch button ([detailFocus]).
 */
private class ConfigItemSelection(val selected: Boolean, val detailFocus: FocusRequester, val onSelect: () -> Unit)

/** The confirmation or name dialog a configuration action opens. */
private sealed interface ConfigDialog {
	data object Rename : ConfigDialog
	data class Duplicate(val initialName: String) : ConfigDialog
	data object Delete : ConfigDialog
}

/**
 * A configuration's card. On a phone it carries Play, Edit and More, and tapping it
 * edits the configuration; with a [selection] (list-detail layout) it only names the
 * configuration and tapping selects it, the actions being in the detail pane.
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun ConfigItem(
	config: ConfigInfo,
	launchInProgress: Boolean,
	isOpenInSettings: Boolean,
	focusRequested: Boolean,
	onFocusRequestHandled: () -> Unit,
	actions: ConfigItemActions,
	selection: ConfigItemSelection?
) {
	var showMenu by remember { mutableStateOf(false) }
	var dialog by remember { mutableStateOf<ConfigDialog?>(null) }
	// The buttons lie inside the focusable card, so the 2D focus search from the card
	// never finds them (a candidate must lie past the card's edge): Right enters them,
	// Left from Play returns to the card.
	val cardFocus = remember { FocusRequester() }
	val playFocus = remember { FocusRequester() }
	LaunchedEffect(focusRequested) {
		if (focusRequested) {
			cardFocus.requestFocus()
			onFocusRequestHandled()
		}
	}

	Card(
		colors = if (selection?.selected == true) {
			CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.secondaryContainer)
		} else {
			CardDefaults.cardColors()
		},
		modifier = Modifier
			.fillMaxWidth()
			.dpadFocusIndicator()
			.focusRequester(cardFocus)
			.then(
				if (selection != null) {
					Modifier
						// Browsing the list with the D-pad shows each configuration in turn.
						.onFocusChanged { if (it.isFocused && !selection.selected) selection.onSelect() }
						.focusProperties { if (!launchInProgress) right = selection.detailFocus }
						.selectable(selected = selection.selected, onClick = selection.onSelect)
				} else {
					Modifier
						.focusProperties { right = playFocus }
						.combinedClickable(
							onClickLabel = stringResource(R.string.action_edit_config),
							onClick = actions.onLoad,
							onLongClickLabel = stringResource(R.string.action_show_actions),
							onLongClick = { showMenu = true }
						)
				}
			)
	) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(12.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			Column(modifier = Modifier.weight(1f)) {
				Text(
					text = config.name,
					style = MaterialTheme.typography.titleMedium
				)
				config.machine?.let { machine ->
					Text(
						text = configMachineText(machine),
						style = MaterialTheme.typography.bodyMedium
					)
				}
				if (config.description.isNotEmpty()) {
					Text(
						text = config.description,
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
				Text(
					text = formatDate(config.lastModified),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
				if (isOpenInSettings) {
					LoadedInSettingsChip(modifier = Modifier.padding(top = 4.dp))
				}
			}

			if (selection == null) {
				// Quick action buttons
				Row(
					modifier = Modifier.wrapContentWidth(),
					verticalAlignment = Alignment.CenterVertically
				) {
					IconButton(
						onClick = actions.onLaunch,
						enabled = !launchInProgress,
						modifier = Modifier
							.focusRequester(playFocus)
							.focusProperties { left = cardFocus }
							.dpadFocusIndicator()
					) {
						if (launchInProgress) {
							CircularProgressIndicator(
								modifier = Modifier.size(20.dp),
								strokeWidth = 2.dp
							)
						} else {
							Icon(
								Icons.Default.PlayArrow,
								contentDescription = stringResource(R.string.action_launch_named, config.name),
								modifier = Modifier.size(24.dp)
							)
						}
					}
					IconButton(
						onClick = actions.onLoad,
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(
							Icons.Default.Edit,
							contentDescription = stringResource(R.string.action_edit_named, config.name),
							modifier = Modifier.size(24.dp)
						)
					}

					// More options button (D-pad accessible alternative to long-press)
					Box {
						IconButton(
							onClick = { showMenu = true },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(
								Icons.Default.MoreVert,
								contentDescription = stringResource(R.string.action_more_for_named, config.name),
								modifier = Modifier.size(24.dp)
							)
						}
						DropdownMenu(
							expanded = showMenu,
							onDismissRequest = { showMenu = false }
						) {
							DropdownMenuItem(
								text = { Text(stringResource(R.string.action_edit_advanced_imgui)) },
								onClick = {
									actions.onAdvancedEdit()
									showMenu = false
								},
								enabled = !launchInProgress,
								leadingIcon = { Icon(Icons.Default.Settings, contentDescription = null) }
							)
							DropdownMenuItem(
								text = { Text(stringResource(R.string.action_rename)) },
								onClick = {
									dialog = ConfigDialog.Rename
									showMenu = false
								},
								leadingIcon = { Icon(Icons.Default.DriveFileRenameOutline, contentDescription = null) }
							)
							DropdownMenuItem(
								text = { Text(stringResource(R.string.action_duplicate)) },
								onClick = {
									dialog = ConfigDialog.Duplicate(actions.suggestedCopyName())
									showMenu = false
								},
								leadingIcon = { Icon(Icons.Default.ContentCopy, contentDescription = null) }
							)
							actions.onShare?.let { share ->
								DropdownMenuItem(
									text = { Text(stringResource(R.string.action_share)) },
									onClick = {
										share()
										showMenu = false
									},
									leadingIcon = { Icon(Icons.Default.Share, contentDescription = null) }
								)
							}
							DropdownMenuItem(
								text = { Text(stringResource(R.string.action_delete)) },
								onClick = {
									dialog = ConfigDialog.Delete
									showMenu = false
								},
								leadingIcon = {
									Icon(
										Icons.Default.Delete,
										contentDescription = null,
										tint = MaterialTheme.colorScheme.error
									)
								}
							)
						}
					}
				}
			}
		}
	}

	ConfigDialogs(
		config = config,
		isOpenInSettings = isOpenInSettings,
		dialog = dialog,
		onDismiss = { dialog = null },
		actions = actions
	)
}

/**
 * The selected configuration on a wide window: its name and description, every action
 * as a labelled button, and the machine it describes.
 */
@Composable
private fun ConfigDetailPane(
	config: ConfigInfo,
	loadSettings: suspend (path: String) -> EmulatorSettings,
	isOpenInSettings: Boolean,
	launchInProgress: Boolean,
	launchFocus: FocusRequester,
	listFocus: FocusRequester,
	actions: ConfigItemActions,
	modifier: Modifier = Modifier
) {
	val context = LocalContext.current
	val roms by FileRepository.getInstance(context).roms.collectAsState()
	var dialog by remember(config.path) { mutableStateOf<ConfigDialog?>(null) }
	// Null while reading; a failure to read shows a message instead of the summary.
	val summary by produceState<Result<QuickStartEffectiveSummary>?>(null, config.path, config.lastModified, roms) {
		// Browsing the list restarts this for every configuration focused: show "reading"
		// rather than the previous one's summary, and let a cancelled read stay cancelled
		// instead of reporting it as unreadable.
		value = null
		value = try {
			Result.success(QuickStartUiState.buildConfigurationSummary(loadSettings(config.path), roms))
		} catch (e: CancellationException) {
			throw e
		} catch (e: Exception) {
			Result.failure(e)
		}
	}

	Column(
		modifier = modifier
			.verticalScroll(rememberScrollState())
			.padding(16.dp),
		verticalArrangement = Arrangement.spacedBy(12.dp)
	) {
		Text(config.name, style = MaterialTheme.typography.headlineSmall)
		if (config.description.isNotEmpty()) {
			Text(config.description, style = MaterialTheme.typography.bodyMedium)
		}
		Text(
			text = formatDate(config.lastModified),
			style = MaterialTheme.typography.bodySmall,
			color = MaterialTheme.colorScheme.onSurfaceVariant
		)
		if (isOpenInSettings) LoadedInSettingsChip()

		FlowRow(
			horizontalArrangement = Arrangement.spacedBy(8.dp),
			verticalArrangement = Arrangement.spacedBy(8.dp)
		) {
			Button(
				onClick = actions.onLaunch,
				enabled = !launchInProgress,
				modifier = Modifier
					.focusRequester(launchFocus)
					// Left returns to the card list (the 2D search went to the rail past it).
					.focusProperties { left = listFocus }
					.dpadFocusIndicator()
			) {
				if (launchInProgress) {
					CircularProgressIndicator(modifier = Modifier.size(18.dp), strokeWidth = 2.dp)
				} else {
					Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(18.dp))
				}
				Spacer(modifier = Modifier.width(8.dp))
				Text(stringResource(R.string.action_launch))
			}
			ConfigDetailAction(Icons.Default.Edit, stringResource(R.string.action_edit_config), actions.onLoad)
			ConfigDetailAction(
				Icons.Default.Settings,
				stringResource(R.string.action_edit_advanced_imgui),
				actions.onAdvancedEdit,
				enabled = !launchInProgress
			)
			ConfigDetailAction(
				Icons.Default.DriveFileRenameOutline,
				stringResource(R.string.action_rename),
				onClick = { dialog = ConfigDialog.Rename }
			)
			ConfigDetailAction(
				Icons.Default.ContentCopy,
				stringResource(R.string.action_duplicate),
				onClick = { dialog = ConfigDialog.Duplicate(actions.suggestedCopyName()) }
			)
			actions.onShare?.let { share ->
				ConfigDetailAction(Icons.Default.Share, stringResource(R.string.action_share), share)
			}
			ConfigDetailAction(
				Icons.Default.Delete,
				stringResource(R.string.action_delete),
				onClick = { dialog = ConfigDialog.Delete },
				tint = MaterialTheme.colorScheme.error
			)
		}

		when (val loaded = summary) {
			null -> Text(
				stringResource(R.string.configurations_detail_loading),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
			else -> loaded.fold(
				onSuccess = { SettingsSummaryCard(summary = it, title = stringResource(R.string.configurations_detail_machine)) },
				onFailure = {
					Text(
						stringResource(R.string.configurations_detail_unreadable),
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.error
					)
				}
			)
		}
	}

	ConfigDialogs(
		config = config,
		isOpenInSettings = isOpenInSettings,
		dialog = dialog,
		onDismiss = { dialog = null },
		actions = actions
	)
}

@Composable
private fun ConfigDetailAction(
	icon: ImageVector,
	label: String,
	onClick: () -> Unit,
	enabled: Boolean = true,
	tint: Color = Color.Unspecified
) {
	OutlinedButton(
		onClick = onClick,
		enabled = enabled,
		modifier = Modifier.dpadFocusIndicator()
	) {
		Icon(icon, contentDescription = null, modifier = Modifier.size(18.dp), tint = if (tint == Color.Unspecified) LocalContentColor.current else tint)
		Spacer(modifier = Modifier.width(8.dp))
		Text(label, color = tint)
	}
}

/** The rename, duplicate and delete dialogs of a configuration's card or detail pane. */
@Composable
private fun ConfigDialogs(
	config: ConfigInfo,
	isOpenInSettings: Boolean,
	dialog: ConfigDialog?,
	onDismiss: () -> Unit,
	actions: ConfigItemActions
) {
	when (dialog) {
		null -> Unit
		ConfigDialog.Rename -> ConfigNameDialog(
			title = stringResource(R.string.dialog_rename_config_title),
			confirmLabel = stringResource(R.string.action_rename_confirm),
			initialName = config.name,
			problemFor = { actions.nameProblem(it, true) },
			onConfirm = { name ->
				onDismiss()
				actions.onRename(name)
			},
			onDismiss = onDismiss
		)
		is ConfigDialog.Duplicate -> ConfigNameDialog(
			title = stringResource(R.string.dialog_duplicate_config_title),
			confirmLabel = stringResource(R.string.action_duplicate),
			initialName = dialog.initialName,
			problemFor = { actions.nameProblem(it, false) },
			onConfirm = { name ->
				onDismiss()
				actions.onDuplicate(name)
			},
			onDismiss = onDismiss
		)
		ConfigDialog.Delete -> AlertDialog(
			onDismissRequest = onDismiss,
			title = { Text(stringResource(R.string.dialog_delete_config_title)) },
			text = {
				Column {
					Text(stringResource(R.string.dialog_delete_config_message, config.name))
					if (isOpenInSettings) {
						Spacer(modifier = Modifier.height(8.dp))
						Text(stringResource(R.string.dialog_delete_config_open_note))
					}
				}
			},
			confirmButton = {
				TextButton(onClick = {
					actions.onDelete()
					onDismiss()
				}, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_delete), color = MaterialTheme.colorScheme.error)
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
}

/**
 * Asks for a configuration name, prefilled with [initialName] (selected, ready to type over).
 * [problemFor] says why a name cannot be used; the reason shows under the field and the
 * confirm button waits until there is none.
 */
@Composable
private fun ConfigNameDialog(
	title: String,
	confirmLabel: String,
	initialName: String,
	problemFor: (String) -> ConfigFiles.NameProblem?,
	onConfirm: (String) -> Unit,
	onDismiss: () -> Unit
) {
	var field by remember(initialName) {
		mutableStateOf(TextFieldValue(initialName, selection = TextRange(0, initialName.length)))
	}
	val problem = problemFor(field.text)
	val problemText = problem?.let { configNameProblemText(it) }
	val focusRequester = remember { FocusRequester() }
	LaunchedEffect(Unit) { focusRequester.requestFocus() }
	fun confirm() {
		if (problem == null) onConfirm(field.text.trim())
	}

	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(title) },
		text = {
			// Inside the dialog's window, so the gate drives the dialog's keyboard.
			val typingGate = rememberDpadTypingGate()
			OutlinedTextField(
				value = field,
				onValueChange = { field = it },
				label = { Text(stringResource(R.string.label_config_name)) },
				singleLine = true,
				// On a TV the IME opens on OK, not on focus: returning to the field with Up
				// re-opened it and the next Down typed into the keyboard instead of moving.
				readOnly = typingGate.readOnly,
				isError = problemText != null,
				supportingText = if (problemText != null) {
					{ Text(problemText) }
				} else {
					null
				},
				keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
				keyboardActions = KeyboardActions(onDone = { confirm() }),
				modifier = Modifier
					.fillMaxWidth()
					// A single-line field has no lines to move between: D-pad up/down reach the buttons.
					.dpadVerticalMovesFocus()
					.then(typingGate.modifier)
					.focusRequester(focusRequester)
			)
		},
		confirmButton = {
			TextButton(
				onClick = { confirm() },
				enabled = problem == null,
				modifier = Modifier.dpadFocusIndicator()
			) {
				Text(confirmLabel)
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

@Composable
private fun configNameProblemText(problem: ConfigFiles.NameProblem): String {
	val message = ConfigurationActions.nameProblemMessage(problem)
	return message.argument?.let { stringResource(message.stringRes, it) } ?: stringResource(message.stringRes)
}

private fun formatDate(timestamp: Long): String {
	if (timestamp == 0L) return ""
	return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT, Locale.getDefault())
		.format(Date(timestamp))
}

/**
 * The machine line of a configuration's card: "Amiga 1200 · 68030 · 66 MB · DF0: Lost Pixel.adf"
 * (the RAM fitted in total; the first medium it inserts, if any).
 */
@Composable
private fun configMachineText(machine: ConfigMachine): String {
	val memory = when {
		machine.memoryKb <= 0 -> null
		machine.memoryKb % 1024 == 0 -> stringResource(R.string.settings_memory_value_mb, machine.memoryKb / 1024)
		machine.memoryKb > 1024 -> stringResource(R.string.settings_memory_value_mb_decimal, machine.memoryKb / 1024f)
		else -> stringResource(R.string.settings_memory_value_kb, machine.memoryKb)
	}
	return listOfNotNull(machine.model, machine.cpu.toString(), memory, machine.firstMedium).joinToString(" · ")
}

/** Marks the configuration Settings holds: a label, not a link. */
@Composable
private fun LoadedInSettingsChip(modifier: Modifier = Modifier) {
	Surface(
		shape = MaterialTheme.shapes.small,
		color = MaterialTheme.colorScheme.secondaryContainer,
		contentColor = MaterialTheme.colorScheme.onSecondaryContainer,
		modifier = modifier
	) {
		Text(
			stringResource(R.string.configurations_detail_open_in_settings),
			style = MaterialTheme.typography.labelMedium,
			modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp)
		)
	}
}

/**
 * Asks before Edit or New replaces what Settings holds: Save (the default for D-pad
 * focus, so an OK pressed at once keeps the edits), Don't Save or Cancel.
 */
@Composable
private fun UnsavedSettingsDialog(
	message: String,
	onSave: () -> Unit,
	onDontSave: () -> Unit,
	onCancel: () -> Unit
) {
	val saveFocus = remember { FocusRequester() }
	LaunchedEffect(Unit) { saveFocus.requestFocus() }
	AlertDialog(
		onDismissRequest = onCancel,
		title = { Text(stringResource(R.string.dialog_unsaved_changes_title)) },
		text = { Text(message) },
		confirmButton = {
			TextButton(onClick = onSave, modifier = Modifier.focusRequester(saveFocus).dpadFocusIndicator()) {
				Text(stringResource(R.string.action_save))
			}
		},
		dismissButton = {
			Row {
				TextButton(onClick = onDontSave, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_dont_save))
				}
				TextButton(onClick = onCancel, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_cancel))
				}
			}
		}
	)
}
