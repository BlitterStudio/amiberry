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
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.navigation.Screen
import com.blitterstudio.amiberry.ui.viewmodel.ConfigurationsViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.launch
import java.text.DateFormat
import java.util.Date
import java.util.Locale

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
	val configs by viewModel.configs.collectAsState()
	val loadFailedMessage = stringResource(R.string.msg_failed_load_config)
	val deletedMessage = stringResource(R.string.msg_config_deleted)
	val deleteFailedMessage = stringResource(R.string.msg_failed_delete_config)
	val duplicateFailedMessage = stringResource(R.string.msg_failed_duplicate_config)

	LaunchedEffect(Unit) {
		viewModel.refresh()
	}

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(title = { Text(stringResource(R.string.configurations_title)) })
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
				}
			}
		} else {
			LazyColumn(
				contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
				verticalArrangement = Arrangement.spacedBy(8.dp)
			) {
				items(configs, key = { it.path }) { config ->
					ConfigItem(
						config = config,
						onLoad = {
							scope.launch {
									runCatching { viewModel.loadConfig(config.path) }
										.onSuccess { parsedConfig ->
											settingsViewModel.loadConfig(parsedConfig)
										navController.navigate(Screen.Settings.route) {
											popUpTo(Screen.Configurations.route) { inclusive = false }
										}
										}
										.onFailure {
											scope.launch {
												snackbarHostState.showSnackbar(loadFailedMessage)
											}
										}
								}
							},
							onDelete = {
								scope.launch {
									val deleted = viewModel.deleteConfig(config.path)
									snackbarHostState.showSnackbar(
										if (deleted) deletedMessage else deleteFailedMessage
									)
								}
							},
							onDuplicate = {
								scope.launch {
									val result = viewModel.duplicateConfig(config.path)
									@Suppress("LocalContextGetResourceValueCall")
									val message = result.fold(
										onSuccess = { context.getString(R.string.msg_config_duplicated_as, it.name) },
										onFailure = { duplicateFailedMessage }
									)
									snackbarHostState.showSnackbar(message)
								}
							}
						)
					}
			}
		}
	}
}
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun ConfigItem(
	config: ConfigInfo,
	onLoad: () -> Unit,
	onDelete: () -> Unit,
	onDuplicate: () -> Unit
) {
	val context = LocalContext.current
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
				IconButton(onClick = {
					EmulatorLauncher.launchWithConfig(context, config.path, skipGui = true)
				}) {
						Icon(
							Icons.Default.PlayArrow,
							contentDescription = stringResource(R.string.action_launch),
							modifier = Modifier.size(24.dp)
						)
					}
					IconButton(onClick = onLoad) {
						Icon(
							Icons.Default.Edit,
							contentDescription = stringResource(R.string.action_edit_config),
							modifier = Modifier.size(24.dp)
						)
					}

				// More options button (D-pad accessible alternative to long-press)
				Box {
						IconButton(onClick = { showMenu = true }) {
							Icon(
								Icons.Default.MoreVert,
								contentDescription = stringResource(R.string.more_options),
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
									EmulatorLauncher.launchAdvancedGui(context, config.path)
									showMenu = false
								},
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
