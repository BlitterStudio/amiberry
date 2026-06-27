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
import androidx.compose.foundation.lazy.items
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
					IconButton(onClick = { viewModel.refresh() }) {
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
						Button(onClick = { navController.switchToTab(Screen.Settings.route) }) {
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
				LazyColumn(
					contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
					verticalArrangement = Arrangement.spacedBy(8.dp)
				) {
					items(filteredConfigs, key = { it.path }) { config ->
					ConfigItem(
						config = config,
						launchInProgress = launchInProgress,
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
									snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.deleteMessage(deleted)))
								}
							},
							onDuplicate = {
								scope.launch {
									val result = viewModel.duplicateConfig(config.path)
									snackbarHostState.showSnackbar(actionMessage(ConfigurationActions.duplicateMessage(result)))
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
					IconButton(onClick = onClearSearch) {
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
			modifier = modifier
		) {
			Text(stringResource(filter.labelRes()))
		}
	} else {
		OutlinedButton(
			onClick = { onFilterSelected(filter) },
			modifier = modifier
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
	onLoad: () -> Unit,
	onDelete: () -> Unit,
	onDuplicate: () -> Unit,
	onShare: () -> Unit,
	onAdvancedEdit: () -> Unit,
	onLaunch: () -> Unit
) {
	var showMenu by remember { mutableStateOf(false) }
	var showDeleteDialog by remember { mutableStateOf(false) }

	Card(
		modifier = Modifier
			.fillMaxWidth()
			.dpadFocusIndicator()
			.combinedClickable(
				onClick = onLoad,
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
				IconButton(onClick = onLaunch, enabled = !launchInProgress) {
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
					IconButton(onClick = onLoad) {
						Icon(
							Icons.Default.Edit,
							contentDescription = stringResource(R.string.action_edit_named, config.name),
							modifier = Modifier.size(24.dp)
						)
					}

				// More options button (D-pad accessible alternative to long-press)
				Box {
						IconButton(onClick = { showMenu = true }) {
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
								text = { Text(stringResource(R.string.action_duplicate)) },
								onClick = {
									onDuplicate()
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

	// Delete confirmation dialog
		if (showDeleteDialog) {
			AlertDialog(
				onDismissRequest = { showDeleteDialog = false },
				title = { Text(stringResource(R.string.dialog_delete_config_title)) },
				text = { Text(stringResource(R.string.dialog_delete_config_message, config.name)) },
				confirmButton = {
					TextButton(onClick = {
						onDelete()
						showDeleteDialog = false
					}) {
						Text(stringResource(R.string.action_delete), color = MaterialTheme.colorScheme.error)
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

private fun formatDate(timestamp: Long): String {
	if (timestamp == 0L) return ""
	return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT, Locale.getDefault())
		.format(Date(timestamp))
}
