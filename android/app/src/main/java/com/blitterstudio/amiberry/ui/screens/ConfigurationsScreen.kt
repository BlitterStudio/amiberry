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
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.Search
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

	LaunchedEffect(Unit) {
		viewModel.refresh()
	}

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.configurations_title)) },
				actions = {
					IconButton(
						onClick = { viewModel.refresh() },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(
							Icons.Default.Refresh,
							contentDescription = stringResource(R.string.configurations_refresh)
						)
					}
				}
			)
		}
	) { innerPadding ->
		Column(modifier = Modifier.fillMaxSize().padding(innerPadding)) {

		if (configs.isEmpty()) {
			Box(
				modifier = Modifier
					.fillMaxSize()
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
							onClick = { navController.switchToTab(Screen.Settings.route) },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Text(stringResource(R.string.configurations_empty_cta))
						}
				}
			}
		} else {
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
				// Renaming re-keys the card and deleting removes it, either of which drops
				// D-pad focus to the navigation rail: the card named here (by file name) takes
				// focus once it is composed.
				var focusConfigFile by remember { mutableStateOf<String?>(null) }
				LazyColumn(
					contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
					verticalArrangement = Arrangement.spacedBy(8.dp)
				) {
					itemsIndexed(filteredConfigs, key = { _, config -> config.path }) { index, config ->
					ConfigItem(
						config = config,
						launchInProgress = launchInProgress,
						isOpenInSettings = settingsViewModel.isConfigOpen(config.path),
						focusRequested = File(config.path).name == focusConfigFile,
						onFocusRequestHandled = { focusConfigFile = null },
						suggestedCopyName = { viewModel.suggestedCopyName(config) },
						nameProblem = { name, renaming -> viewModel.nameProblem(name, config.takeIf { renaming }) },
						onLoad = {
							scope.launch {
								when (val result = ConfigurationActions.load(runCatching { viewModel.loadConfig(config.path) })) {
									is ConfigurationActions.LoadResult.Loaded -> {
										settingsViewModel.loadConfig(result.value, config.name, config.path)
										navController.switchToTab(Screen.Settings.route)
									}
									is ConfigurationActions.LoadResult.Failed -> {
										snackbarHostState.showSnackbar(actionMessage(result.message))
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
										// Keeping the same name leaves the file as it was: nothing to report.
										if (result.file.name == File(config.path).name) return@launch
									}
									snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.renameMessage(result)))
								}
							},
							onShare = {
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
							},
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
								val controlSettings = settingsViewModel.settings
								when (val target = ConfigurationActions.launchTarget(configPath)) {
									is ConfigurationActions.LaunchTarget.Ready -> {
										scope.launchGuarded(launchGuard) {
											runCatching {
												EmulatorLauncher.launchWithConfig(
													context,
													target.path,
													skipGui = true,
													controlSettings = controlSettings
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
					}
				}
			}
		}
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

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun ConfigItem(
	config: ConfigInfo,
	launchInProgress: Boolean,
	isOpenInSettings: Boolean,
	focusRequested: Boolean,
	onFocusRequestHandled: () -> Unit,
	suggestedCopyName: () -> String,
	nameProblem: (name: String, renaming: Boolean) -> ConfigFiles.NameProblem?,
	onLoad: () -> Unit,
	onDelete: () -> Unit,
	onDuplicate: (String) -> Unit,
	onRename: (String) -> Unit,
	onShare: () -> Unit,
	onAdvancedEdit: () -> Unit,
	onLaunch: () -> Unit
) {
	var showMenu by remember { mutableStateOf(false) }
	var showDeleteDialog by remember { mutableStateOf(false) }
	var showRenameDialog by remember { mutableStateOf(false) }
	var duplicateName by remember { mutableStateOf<String?>(null) }
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
		modifier = Modifier
			.fillMaxWidth()
			.dpadFocusIndicator()
			.focusRequester(cardFocus)
			.focusProperties { right = playFocus }
			.combinedClickable(
				onClickLabel = stringResource(R.string.action_edit_config),
				onClick = onLoad,
				onLongClickLabel = stringResource(R.string.action_show_actions),
				onLongClick = { showMenu = true }
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
			}

			// Quick action buttons
			Row(
				modifier = Modifier.wrapContentWidth(),
				verticalAlignment = Alignment.CenterVertically
			) {
				IconButton(
					onClick = onLaunch,
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
					onClick = onLoad,
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
								onAdvancedEdit()
								showMenu = false
							},
							enabled = !launchInProgress,
							leadingIcon = { Icon(Icons.Default.Settings, contentDescription = null) }
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_rename)) },
							onClick = {
								showRenameDialog = true
								showMenu = false
							},
							leadingIcon = { Icon(Icons.Default.DriveFileRenameOutline, contentDescription = null) }
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_duplicate)) },
							onClick = {
								duplicateName = suggestedCopyName()
								showMenu = false
							},
							leadingIcon = { Icon(Icons.Default.ContentCopy, contentDescription = null) }
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_share)) },
							onClick = {
								onShare()
								showMenu = false
							},
							leadingIcon = { Icon(Icons.Default.Share, contentDescription = null) }
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_delete)) },
							onClick = {
								showDeleteDialog = true
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

	if (showRenameDialog) {
		ConfigNameDialog(
			title = stringResource(R.string.dialog_rename_config_title),
			confirmLabel = stringResource(R.string.action_rename_confirm),
			initialName = config.name,
			problemFor = { nameProblem(it, true) },
			onConfirm = { name ->
				showRenameDialog = false
				onRename(name)
			},
			onDismiss = { showRenameDialog = false }
		)
	}

	duplicateName?.let { initialName ->
		ConfigNameDialog(
			title = stringResource(R.string.dialog_duplicate_config_title),
			confirmLabel = stringResource(R.string.action_duplicate),
			initialName = initialName,
			problemFor = { nameProblem(it, false) },
			onConfirm = { name ->
				duplicateName = null
				onDuplicate(name)
			},
			onDismiss = { duplicateName = null }
		)
	}

	if (showDeleteDialog) {
		AlertDialog(
			onDismissRequest = { showDeleteDialog = false },
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
					onDelete()
					showDeleteDialog = false
				}, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_delete), color = MaterialTheme.colorScheme.error)
				}
			},
			dismissButton = {
				TextButton(
					onClick = { showDeleteDialog = false },
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

	val typingGate = rememberDpadTypingGate()
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(title) },
		text = {
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
