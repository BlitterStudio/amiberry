package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.OutlinedCard
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
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AdvancedLaunchActions
import com.blitterstudio.amiberry.data.ConfigurationSaveActions
import com.blitterstudio.amiberry.data.ConfigRepository
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.ImportFeedback
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.SettingsChange
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotice
import com.blitterstudio.amiberry.data.model.SettingsIntentPreset
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.ui.Alignment
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.launchImportGuarded
import com.blitterstudio.amiberry.ui.launchGuarded
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.launch

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
	var showSaveAsDialog by remember { mutableStateOf(false) }
	var overwriteRequest by remember { mutableStateOf<OverwriteRequest?>(null) }
	var showTopBarMenu by remember { mutableStateOf(false) }
	var showReviewChangesDialog by remember { mutableStateOf(false) }
	val availableRoms by viewModel.availableRoms.collectAsState()
	val canStart = viewModel.settings.romFile.isNotBlank() || availableRoms.isNotEmpty()
	fun importResultMessage(result: FileManager.ImportResult): String {
		val message = ImportFeedback.fromImportResult(result)
		return context.getString(message.stringRes, message.argument)
	}

	val romPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
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

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = {
					Column {
						Text(viewModel.currentConfigName ?: stringResource(R.string.settings_new_configuration))
						if (viewModel.isDirty) {
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
						onClick = {
							if (viewModel.currentConfigName == null) {
								showSaveAsDialog = true
							} else {
								scope.launch {
									val result = viewModel.saveTracked()
									snackbarHostState.showSnackbar(saveMessage(result))
								}
							}
						},
						enabled = viewModel.currentConfigName == null || viewModel.isDirty
					) {
						Text(stringResource(R.string.action_save))
					}
					IconButton(onClick = { showTopBarMenu = true }) {
						Icon(Icons.Default.MoreVert, contentDescription = stringResource(R.string.more_options))
					}
					DropdownMenu(
						expanded = showTopBarMenu,
						onDismissRequest = { showTopBarMenu = false }
					) {
						DropdownMenuItem(
							text = { Text(stringResource(R.string.action_review_changes)) },
							leadingIcon = { Icon(Icons.Default.Info, contentDescription = null) },
							enabled = viewModel.isDirty,
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
								showSaveAsDialog = true
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
							EmulatorLauncher.launchSettingsConfig(
								context,
								launchSettings.baseModel,
								configFile.absolutePath
							)
						}.onFailure {
							snackbarHostState.showSnackbar(launchFailedMessage)
						}
					}
				},
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
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
		) {
			PrimaryScrollableTabRow(
				selectedTabIndex = validSelectedTab,
				modifier = Modifier
					.fillMaxWidth()
					.focusGroup()
			) {
				tabs.forEachIndexed { index, tab ->
					Tab(
						selected = validSelectedTab == index,
						onClick = { selectedTab = index },
						modifier = Modifier.testTag(tab.testTag),
						text = { Text(stringResource(tab.titleRes)) }
					)
				}
			}

			SettingsPresetSelector(
				onPresetSelected = { preset -> viewModel.applyIntentPreset(preset) }
			)

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
							onClick = {
								if (!importInProgress) {
									romPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.ROMS))
								}
							},
							enabled = !importInProgress,
							colors = ButtonDefaults.buttonColors(
								containerColor = MaterialTheme.colorScheme.onErrorContainer,
								contentColor = MaterialTheme.colorScheme.errorContainer
							)
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

			// Tab content
			Column(
				modifier = Modifier
					.weight(1f)
					.fillMaxWidth()
			) {
				when (SettingsTabs.tabFor(selectedTab)) {
					SettingsTab.Cpu -> CpuTab(viewModel)
					SettingsTab.Chipset -> ChipsetTab(viewModel)
					SettingsTab.Memory -> MemoryTab(viewModel)
					SettingsTab.Display -> DisplayTab(viewModel)
					SettingsTab.Sound -> SoundTab(viewModel)
					SettingsTab.Input -> InputTab(viewModel)
					SettingsTab.Storage -> StorageTab(
						viewModel = viewModel,
						onImportResult = { result ->
							scope.launch {
								snackbarHostState.showSnackbar(importResultMessage(result))
							}
						}
					)
				}
			}
		}

		if (showSaveAsDialog) {
				SaveConfigDialog(
					initialName = viewModel.currentConfigName.orEmpty(),
					initialDescription = viewModel.currentConfigDescription,
					onDismiss = { showSaveAsDialog = false },
					onSave = { name, description ->
						scope.launch {
							when (val result = viewModel.saveAs(name, description, allowOverwrite = false)) {
								is ConfigurationSaveActions.SaveResult.Saved -> {
									showSaveAsDialog = false
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
					changes = viewModel.changeSummary,
					onDismiss = { showReviewChangesDialog = false }
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
									showSaveAsDialog = false
								}
								snackbarHostState.showSnackbar(saveMessage(result))
							}
						}) {
							Text(stringResource(R.string.action_overwrite))
						}
					},
					dismissButton = {
						TextButton(onClick = { overwriteRequest = null }) {
							Text(stringResource(R.string.action_cancel))
						}
					}
				)
			}
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun SettingsPresetSelector(
	onPresetSelected: (SettingsIntentPreset) -> Unit
) {
	var expanded by remember { mutableStateOf(false) }

	OutlinedCard(
		modifier = Modifier
			.fillMaxWidth()
			.padding(horizontal = 16.dp, vertical = 8.dp)
	) {
		Column(modifier = Modifier.padding(12.dp)) {
			ExposedDropdownMenuBox(
				expanded = expanded,
				onExpandedChange = { expanded = it }
			) {
				OutlinedTextField(
					value = stringResource(R.string.settings_preset_select),
					onValueChange = {},
					readOnly = true,
					label = { Text(stringResource(R.string.settings_presets_title)) },
					trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
					modifier = Modifier
						.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
						.fillMaxWidth()
				)
				ExposedDropdownMenu(
					expanded = expanded,
					onDismissRequest = { expanded = false }
				) {
					SettingsIntentPreset.entries.forEach { preset ->
						DropdownMenuItem(
							text = {
								Column {
									Text(stringResource(preset.titleRes()))
									Text(
										stringResource(preset.descriptionRes()),
										style = MaterialTheme.typography.bodySmall,
										color = MaterialTheme.colorScheme.onSurfaceVariant
									)
								}
							},
							onClick = {
								onPresetSelected(preset)
								expanded = false
							}
						)
					}
				}
			}
		}
	}
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
					stringResource(notice.messageRes()),
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
			TextButton(onClick = onDismiss) {
				Text(stringResource(R.string.action_dismiss))
			}
		}
	}
}

private fun SettingsIntentPreset.titleRes(): Int = when (this) {
	SettingsIntentPreset.Compatibility -> R.string.settings_preset_compatibility
	SettingsIntentPreset.Balanced -> R.string.settings_preset_balanced
	SettingsIntentPreset.Performance -> R.string.settings_preset_performance
	SettingsIntentPreset.PixelPerfect -> R.string.settings_preset_pixel_perfect
	SettingsIntentPreset.TouchControls -> R.string.settings_preset_touch_controls
}

private fun SettingsIntentPreset.descriptionRes(): Int = when (this) {
	SettingsIntentPreset.Compatibility -> R.string.settings_preset_compatibility_desc
	SettingsIntentPreset.Balanced -> R.string.settings_preset_balanced_desc
	SettingsIntentPreset.Performance -> R.string.settings_preset_performance_desc
	SettingsIntentPreset.PixelPerfect -> R.string.settings_preset_pixel_perfect_desc
	SettingsIntentPreset.TouchControls -> R.string.settings_preset_touch_controls_desc
}

private fun SettingsAdjustmentNotice.messageRes(): Int = when (this) {
	SettingsAdjustmentNotice.AgaRequires68020 -> R.string.settings_adjustment_aga_cpu
	SettingsAdjustmentNotice.CpuRequires32BitAddressing -> R.string.settings_adjustment_cpu_32bit
	SettingsAdjustmentNotice.CycleExactDisabledJit -> R.string.settings_adjustment_cycle_exact_jit
	SettingsAdjustmentNotice.CycleExactDisabledImmediateBlits -> R.string.settings_adjustment_cycle_exact_blits
	SettingsAdjustmentNotice.Z3Requires32BitAddressing -> R.string.settings_adjustment_z3_32bit
	SettingsAdjustmentNotice.IntegerScalingSynced -> R.string.settings_adjustment_integer_scaling
	SettingsAdjustmentNotice.JitRequires68020 -> R.string.settings_adjustment_jit_cpu
	SettingsAdjustmentNotice.JitRequires32BitAddressing -> R.string.settings_adjustment_jit_32bit
	SettingsAdjustmentNotice.OnScreenControlsRequireTouch -> R.string.settings_adjustment_touch_required
	SettingsAdjustmentNotice.OnScreenJoystickMovedToPort1 -> R.string.settings_adjustment_touch_port
	SettingsAdjustmentNotice.InvalidInputDeviceReset -> R.string.settings_adjustment_input_reset
}

@Composable
private fun SettingsChangesDialog(
	changes: List<SettingsChange>,
	onDismiss: () -> Unit
) {
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.dialog_review_changes_title)) },
		text = {
			Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
				if (changes.isEmpty()) {
					Text(stringResource(R.string.dialog_review_changes_empty))
				} else {
					changes.take(12).forEach { change ->
						Column {
							Text(change.label, style = MaterialTheme.typography.labelLarge)
							Text(
								stringResource(R.string.dialog_review_changes_row, change.before, change.after),
								style = MaterialTheme.typography.bodySmall,
								color = MaterialTheme.colorScheme.onSurfaceVariant
							)
						}
					}
					if (changes.size > 12) {
						Text(
							stringResource(R.string.dialog_review_changes_more, changes.size - 12),
							style = MaterialTheme.typography.bodySmall,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)
					}
				}
			}
		},
		confirmButton = {
			TextButton(onClick = onDismiss) {
				Text(stringResource(R.string.action_done))
			}
		}
	)
}

private data class OverwriteRequest(val name: String, val description: String)

@Composable
fun SaveConfigDialog(
	initialName: String,
	initialDescription: String,
	onDismiss: () -> Unit,
	onSave: (String, String) -> Unit
) {
	var name by remember { mutableStateOf(initialName) }
	var description by remember { mutableStateOf(initialDescription) }

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
