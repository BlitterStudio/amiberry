package com.blitterstudio.amiberry.ui.screens.library

import android.content.Intent
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
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
import androidx.compose.foundation.layout.wrapContentWidth
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.DriveFileRenameOutline
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.AssistChip
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.SnackbarHostState
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import androidx.core.content.FileProvider
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigFiles
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.ConfigMachine
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.components.SettingsSummaryCard
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.LaunchInFlightGuard
import com.blitterstudio.amiberry.ui.isTvDevice
import com.blitterstudio.amiberry.ui.launchGuarded
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.ConfigurationsViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.ui.windowWidthClass
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch
import java.io.File
import java.text.DateFormat
import java.util.Date
import java.util.Locale

/**
 * The Library's machines: the saved configurations as cards (a wide window shows the
 * chosen one beside the list), each to play, edit, duplicate, rename, delete or
 * share. With none saved, the models start a new machine.
 */
@Composable
internal fun LibraryMachinesPane(
	modifier: Modifier,
	configs: List<ConfigInfo>,
	category: String?,
	query: String,
	settingsViewModel: SettingsViewModel,
	viewModel: ConfigurationsViewModel,
	scope: CoroutineScope,
	snackbarHostState: SnackbarHostState,
	launchGuard: LaunchInFlightGuard,
	launchInProgress: Boolean,
	onEdit: (ConfigInfo) -> Unit,
	onNewMachine: (AmigaModel) -> Unit,
	onPlayFile: () -> Unit
) {
	val context = LocalContext.current
	val isTv = isTvDevice()

	// Renaming re-keys the card and deleting removes it, either of which drops D-pad
	// focus: the card named here (by file name) takes focus once it is composed.
	var focusConfigFile by remember { mutableStateOf<String?>(null) }
	// The machine the detail pane shows on a wide window, by file name.
	var selectedConfigFile by rememberSaveable { mutableStateOf<String?>(null) }

	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)

	fun actionsFor(index: Int, config: ConfigInfo) = ConfigItemActions(
		suggestedCopyName = { viewModel.suggestedCopyName(config) },
		nameProblem = { name, renaming -> viewModel.nameProblem(name, config.takeIf { renaming }) },
		onLoad = { onEdit(config) },
		onDelete = {
			scope.launch {
				val deleted = viewModel.deleteConfig(config.path)
				if (deleted) {
					val neighbour = filteredConfigs(configs, category, query).getOrNull(index + 1)
						?: filteredConfigs(configs, category, query).getOrNull(index - 1)
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
					} catch (_: Exception) {
						scope.launch {
							snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.shareFailedMessage()))
						}
					}
				}
				is ConfigurationActions.ShareTarget.Failed ->
					scope.launch { snackbarHostState.showSnackbar(actionMessage(target.message)) }
			}
		}),
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
				is ConfigurationActions.LaunchTarget.Failed ->
					scope.launch { snackbarHostState.showSnackbar(actionMessage(target.message)) }
			}
		}
	)

	val shown = remember(configs, category, query) { filteredConfigs(configs, category, query) }
	val listDetail = windowWidthClass().showsTwoPanes
	val selectedConfig = if (listDetail) selectedConfiguration(shown, selectedConfigFile) else null
	val detailFocus = remember { FocusRequester() }
	// The card list: where D-pad focus enters the pane, and Left from the detail pane returns to.
	val listFocus = remember { FocusRequester() }

	if (configs.isEmpty()) {
		EmptyLibrary(
			modifier = modifier,
			onNewMachine = onNewMachine,
			onPlayFile = onPlayFile
		)
	} else {
		val listPane: @Composable ColumnScope.() -> Unit = {
			if (shown.isEmpty()) {
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
					// Entering the list (D-pad Right from the filter list, or Left from the
					// detail pane) lands on the card last focused, else the first.
					modifier = Modifier
						.fillMaxSize()
						.focusRequester(listFocus)
						.contentEntryFocus()
						.focusRestorer(),
					contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
					verticalArrangement = Arrangement.spacedBy(8.dp)
				) {
					itemsIndexed(shown, key = { _, config -> config.path }) { index, config ->
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

		if (listDetail) {
			Row(modifier = modifier.fillMaxSize()) {
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
						actions = actionsFor(shown.indexOf(selectedConfig), selectedConfig),
						modifier = Modifier.weight(0.55f).fillMaxHeight()
					)
				} else {
					Spacer(modifier = Modifier.weight(0.55f))
				}
			}
		} else {
			Column(modifier = modifier.fillMaxSize(), content = listPane)
		}
	}
}

/** No saved machines: start from a model, or play a file. */
@Composable
private fun EmptyLibrary(
	modifier: Modifier,
	onNewMachine: (AmigaModel) -> Unit,
	onPlayFile: () -> Unit
) {
	Box(
		modifier = modifier
			.fillMaxSize()
			.padding(32.dp),
		contentAlignment = Alignment.Center
	) {
		Column(horizontalAlignment = Alignment.CenterHorizontally) {
			Text(stringResource(R.string.library_no_saved_machines), style = MaterialTheme.typography.titleMedium)
			Spacer(modifier = Modifier.height(8.dp))
			Text(
				stringResource(R.string.library_no_saved_machines_message),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
			Spacer(modifier = Modifier.height(24.dp))
			// The four main models, as the Apple apps' empty Library offers them.
			FlowRow(
				horizontalArrangement = Arrangement.spacedBy(8.dp),
				verticalArrangement = Arrangement.spacedBy(8.dp)
			) {
				listOf(AmigaModel.A500, AmigaModel.A1200, AmigaModel.CD32, AmigaModel.A4000).forEach { model ->
					AssistChip(
						onClick = { onNewMachine(model) },
						label = { Text(model.displayName) },
						leadingIcon = { Icon(Icons.Default.Add, contentDescription = null, modifier = Modifier.size(18.dp)) },
						modifier = Modifier.contentEntryFocus().dpadFocusIndicator()
					)
				}
			}
			Spacer(modifier = Modifier.height(16.dp))
			OutlinedButton(onClick = onPlayFile, modifier = Modifier.dpadFocusIndicator()) {
				Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(18.dp))
				Spacer(modifier = Modifier.width(8.dp))
				Text(stringResource(R.string.library_play_file))
			}
		}
	}
}

/** The machines of [category] (all for null) whose name, description or path matches [query]. */
internal fun filteredConfigs(configs: List<ConfigInfo>, category: String?, query: String): List<ConfigInfo> {
	val inCategory = if (category == null) configs else configs.filter { it.category == category }
	val normalizedQuery = query.trim().lowercase()
	if (normalizedQuery.isBlank()) return inCategory
	return inCategory.filter { config ->
		config.name.contains(normalizedQuery, ignoreCase = true) ||
			config.description.contains(normalizedQuery, ignoreCase = true) ||
			config.path.contains(normalizedQuery, ignoreCase = true)
	}
}

/**
 * The configuration the list-detail layout shows: the one chosen (by file name) while it
 * is listed, otherwise the first listed, as when the choice was filtered out or deleted.
 */
internal fun selectedConfiguration(configs: List<ConfigInfo>, selectedFileName: String?): ConfigInfo? =
	configs.firstOrNull { File(it.path).name == selectedFileName } ?: configs.firstOrNull()

/** What can be done with one machine, from its card or the detail pane. */
private class ConfigItemActions(
	val suggestedCopyName: () -> String,
	val nameProblem: (name: String, renaming: Boolean) -> ConfigFiles.NameProblem?,
	val onLoad: () -> Unit,
	val onDelete: () -> Unit,
	val onDuplicate: (String) -> Unit,
	val onRename: (String) -> Unit,
	/** Null where no app can take a shared file (Google TV). */
	val onShare: (() -> Unit)?,
	val onLaunch: () -> Unit
)

/**
 * A card in the list-detail layout: focusing or tapping it selects it for the detail
 * pane, and D-pad Right goes to the pane's Launch button ([detailFocus]).
 */
private class ConfigItemSelection(val selected: Boolean, val detailFocus: FocusRequester, val onSelect: () -> Unit)

/** The confirmation or name dialog a machine action opens. */
private sealed interface ConfigDialog {
	data object Rename : ConfigDialog
	data class Duplicate(val initialName: String) : ConfigDialog
	data object Delete : ConfigDialog
}

/**
 * A machine's card. On a phone it carries Play, Edit and More, and tapping it opens the
 * machine in the editor; with a [selection] (list-detail layout) it only names the
 * machine and tapping selects it, the actions being in the detail pane.
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
						// Browsing the list with the D-pad shows each machine in turn.
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
				Text(text = config.name, style = MaterialTheme.typography.titleMedium)
				if (config.category.isNotBlank()) {
					Text(
						text = config.category,
						style = MaterialTheme.typography.labelMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
				config.machine?.let { machine ->
					Text(text = configMachineText(machine), style = MaterialTheme.typography.bodyMedium)
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
							CircularProgressIndicator(modifier = Modifier.size(20.dp), strokeWidth = 2.dp)
						} else {
							Icon(
								Icons.Default.PlayArrow,
								contentDescription = stringResource(R.string.action_launch_named, config.name),
								modifier = Modifier.size(24.dp)
							)
						}
					}
					IconButton(onClick = actions.onLoad, modifier = Modifier.dpadFocusIndicator()) {
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
						DropdownMenu(expanded = showMenu, onDismissRequest = { showMenu = false }) {
							DropdownMenuItem(
								text = { Text(stringResource(R.string.action_rename)) },
								modifier = Modifier.dpadFocusIndicator(),
								onClick = {
									dialog = ConfigDialog.Rename
									showMenu = false
								},
								leadingIcon = { Icon(Icons.Default.DriveFileRenameOutline, contentDescription = null) }
							)
							DropdownMenuItem(
								text = { Text(stringResource(R.string.action_duplicate)) },
								modifier = Modifier.dpadFocusIndicator(),
								onClick = {
									dialog = ConfigDialog.Duplicate(actions.suggestedCopyName())
									showMenu = false
								},
								leadingIcon = { Icon(Icons.Default.ContentCopy, contentDescription = null) }
							)
							actions.onShare?.let { share ->
								DropdownMenuItem(
									text = { Text(stringResource(R.string.action_share)) },
									modifier = Modifier.dpadFocusIndicator(),
									onClick = {
										share()
										showMenu = false
									},
									leadingIcon = { Icon(Icons.Default.Share, contentDescription = null) }
								)
							}
							DropdownMenuItem(
								text = { Text(stringResource(R.string.action_delete)) },
								modifier = Modifier.dpadFocusIndicator(),
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
 * The selected machine on a wide window: its name and description, every action as a
 * labelled button, and the machine it describes.
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
	val roms by com.blitterstudio.amiberry.data.FileRepository.getInstance(context).roms.collectAsState()
	var dialog by remember(config.path) { mutableStateOf<ConfigDialog?>(null) }
	// Null while reading; a failure to read shows a message instead of the summary.
	val summary by produceState<Result<MachineSummary>?>(null, config.path, config.lastModified, roms) {
		// Browsing the list restarts this for every machine focused: show "reading"
		// rather than the previous one's summary, and let a cancelled read stay cancelled
		// instead of reporting it as unreadable.
		value = null
		value = try {
			Result.success(MachineSummaries.build(loadSettings(config.path), roms))
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
		if (config.category.isNotBlank()) {
			Text(config.category, style = MaterialTheme.typography.labelLarge, color = MaterialTheme.colorScheme.onSurfaceVariant)
		}
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
					// Left returns to the card list (the 2D search went past it).
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
	OutlinedButton(onClick = onClick, enabled = enabled, modifier = Modifier.dpadFocusIndicator()) {
		Icon(icon, contentDescription = null, modifier = Modifier.size(18.dp), tint = if (tint == Color.Unspecified) LocalContentColor.current else tint)
		Spacer(modifier = Modifier.width(8.dp))
		Text(label, color = tint)
	}
}

/** The rename, duplicate and delete dialogs of a machine's card or detail pane. */
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
				TextButton(
					onClick = {
						actions.onDelete()
						onDismiss()
					},
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_delete), color = MaterialTheme.colorScheme.error)
				}
			},
			dismissButton = {
				TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_cancel))
				}
			}
		)
	}
}

/**
 * Asks for a machine name, prefilled with [initialName] (selected, ready to type over).
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
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
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
 * The machine line of a card: "Amiga 1200 · 68030 · 66 MB · DF0: Lost Pixel.adf"
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

/** Marks the machine the editor holds: a label, not a link. */
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
 * Asks before Edit or New Machine replaces what the editor holds: Save (the default for
 * D-pad focus, so an OK pressed at once keeps the edits), Don't Save or Cancel.
 */
@Composable
internal fun UnsavedSettingsDialog(
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
