package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.PrimaryScrollableTabRow
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Tab
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.ConfigRepository
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(viewModel: SettingsViewModel = viewModel(LocalContext.current.findActivity() as androidx.activity.ComponentActivity)) {
	val context = LocalContext.current
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	var selectedTab by rememberSaveable { mutableIntStateOf(0) }
	var showSaveDialog by remember { mutableStateOf(false) }
	var showTopBarMenu by remember { mutableStateOf(false) }
	val availableRoms by viewModel.availableRoms.collectAsState()
	val canStart = viewModel.settings.romFile.isNotBlank() || availableRoms.isNotEmpty()

	val tabs = listOf(
		stringResource(R.string.tab_cpu),
		stringResource(R.string.tab_chipset),
		stringResource(R.string.tab_memory),
		stringResource(R.string.tab_display),
		stringResource(R.string.tab_sound),
		stringResource(R.string.tab_input),
		stringResource(R.string.tab_storage)
	)
	val invalidConfigNameMessage = stringResource(R.string.msg_invalid_config_name)
	val failedSaveConfigMessage = stringResource(R.string.msg_failed_save_config)
	val romRequiredMessage = stringResource(R.string.msg_rom_required_before_start)

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.settings_title)) },
				actions = {
					IconButton(onClick = { showTopBarMenu = true }) {
						Icon(Icons.Default.MoreVert, contentDescription = stringResource(R.string.more_options))
					}
					DropdownMenu(
						expanded = showTopBarMenu,
						onDismissRequest = { showTopBarMenu = false }
					) {
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_save_configuration)) },
							leadingIcon = { Icon(Icons.Default.Save, contentDescription = null) },
							onClick = {
								showTopBarMenu = false
								showSaveDialog = true
							}
						)
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_advanced)) },
							leadingIcon = { Icon(Icons.Default.Settings, contentDescription = null) },
							onClick = {
								showTopBarMenu = false
								val configFile = ConfigGenerator.writeConfig(
									context, viewModel.settings, ".temp_native_ui.uae"
								)
								if (viewModel.currentUnknownLines.isNotEmpty()) {
									configFile.appendText("\n; Preserved settings from original config\n")
									viewModel.currentUnknownLines.forEach { configFile.appendText("$it\n") }
								}
								EmulatorLauncher.launchAdvancedGui(context, configFile.absolutePath)
							}
						)
					}
				}
			)
		},
		floatingActionButton = {
			ExtendedFloatingActionButton(
				onClick = {
					if (!canStart) {
						scope.launch {
							snackbarHostState.showSnackbar(romRequiredMessage)
						}
						return@ExtendedFloatingActionButton
					}
					val args = viewModel.generateLaunchArgs()
					EmulatorLauncher.launchWithArgs(context, args)
				},
				icon = { Icon(Icons.Default.PlayArrow, contentDescription = null) },
				text = { Text(stringResource(R.string.action_start)) }
			)
		}
	) { innerPadding ->
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
		) {
			PrimaryScrollableTabRow(
				selectedTabIndex = selectedTab,
				modifier = Modifier
					.fillMaxWidth()
					.focusGroup()
			) {
				tabs.forEachIndexed { index, title ->
					Tab(
						selected = selectedTab == index,
						onClick = { selectedTab = index },
						text = { Text(title) }
					)
				}
			}

			if (!canStart) {
				Text(
					text = romRequiredMessage,
					color = androidx.compose.material3.MaterialTheme.colorScheme.error,
					style = androidx.compose.material3.MaterialTheme.typography.bodySmall,
					modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp)
				)
			}

			// Tab content
			Column(
				modifier = Modifier
					.weight(1f)
					.fillMaxWidth()
			) {
				when (selectedTab) {
					0 -> CpuTab(viewModel)
					1 -> ChipsetTab(viewModel)
					2 -> MemoryTab(viewModel)
					3 -> DisplayTab(viewModel)
					4 -> SoundTab(viewModel)
					5 -> InputTab(viewModel)
					6 -> StorageTab(viewModel)
				}
			}
		}

		if (showSaveDialog) {
			SaveConfigDialog(
				onDismiss = { showSaveDialog = false },
				onSave = { name, description ->
					val repo = ConfigRepository.getInstance(context)
					val safeName = name.trim()
					if (!repo.isValidConfigName(safeName)) {
						scope.launch {
							snackbarHostState.showSnackbar(invalidConfigNameMessage)
						}
						return@SaveConfigDialog
					}
					val savedFile = repo.saveConfig(viewModel.settings, safeName, viewModel.currentUnknownLines, description)
					if (savedFile == null) {
						scope.launch {
							snackbarHostState.showSnackbar(failedSaveConfigMessage)
						}
						return@SaveConfigDialog
					}
					showSaveDialog = false
					@Suppress("LocalContextGetResourceValueCall")
					scope.launch {
						snackbarHostState.showSnackbar(
							context.getString(R.string.msg_saved_configuration, savedFile.name)
						)
					}
				}
			)
		}
	}
}

@Composable
fun SaveConfigDialog(
	onDismiss: () -> Unit,
	onSave: (String, String) -> Unit
) {
	var name by remember { mutableStateOf("") }
	var description by remember { mutableStateOf("") }

	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.dialog_save_config_title)) },
		text = {
			Column {
				Text(stringResource(R.string.dialog_save_config_message))
				Spacer(modifier = Modifier.height(8.dp))
				OutlinedTextField(
					value = name,
					onValueChange = { name = it },
					label = { Text(stringResource(R.string.label_configuration_name)) },
					singleLine = true
				)
				Spacer(modifier = Modifier.height(8.dp))
				OutlinedTextField(
					value = description,
					onValueChange = { description = it },
					label = { Text(stringResource(R.string.label_configuration_description)) },
					singleLine = true
				)
			}
		},
		confirmButton = {
			TextButton(
				onClick = { onSave(name.trim(), description.trim()) },
				enabled = name.isNotBlank()
			) {
				Text(stringResource(R.string.action_save))
			}
		},
		dismissButton = {
			TextButton(onClick = onDismiss) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}
