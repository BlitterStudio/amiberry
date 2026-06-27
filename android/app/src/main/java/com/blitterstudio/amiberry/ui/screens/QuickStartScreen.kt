package com.blitterstudio.amiberry.ui.screens

import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
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
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AndroidLaunchConfig
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.ImportFeedback
import com.blitterstudio.amiberry.data.RecentLaunches
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.launchImportGuarded
import com.blitterstudio.amiberry.ui.launchGuarded
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.QuickStartViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.ui.components.MediaSelector
import com.blitterstudio.amiberry.ui.components.StoragePermissionBanner
import com.blitterstudio.amiberry.ui.navigation.Screen
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

internal fun buildRecentQuickStartSettingsSnapshot(
	model: AmigaModel,
	roms: List<AmigaFile>,
	currentSettings: EmulatorSettings,
	floppy0: String,
	floppy1: String,
	cdImage: String
): EmulatorSettings = AndroidLaunchConfig.buildQuickStartSettingsSnapshot(
	model = model,
	roms = roms,
	currentSettings = currentSettings,
	floppy0 = floppy0,
	floppy1 = floppy1,
	cdImage = cdImage
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun QuickStartScreen(
	navController: androidx.navigation.NavController? = null,
	viewModel: QuickStartViewModel = viewModel(LocalContext.current.findActivity() as androidx.activity.ComponentActivity),
	settingsViewModel: SettingsViewModel = viewModel(LocalContext.current.findActivity() as androidx.activity.ComponentActivity)
) {
	val context = LocalContext.current
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	val launchGuard = rememberLaunchInFlightGuard()
	val launchInProgress = launchGuard.isLaunching
	val importGuard = rememberImportInFlightGuard()
	val importInProgress = importGuard.isImporting
	val model = settingsViewModel.settings.baseModel
	val floppies by viewModel.availableFloppies.collectAsState()
	val cds by viewModel.availableCds.collectAsState()
	val roms by viewModel.availableRoms.collectAsState()
	val whdloadGames by viewModel.availableWhdloadGames.collectAsState()
	val selectedWhdloadGame = viewModel.selectedWhdload
	val isScanning by FileRepository.getInstance(context).isScanning.collectAsState()
	val hasRoms = roms.isNotEmpty()
	val availableModels = remember(roms) { ModelRomAvailability.availableModels(roms) }
	val selectedModelAvailable = model in availableModels
	val canStart = if (selectedWhdloadGame != null) hasRoms else selectedModelAvailable

	val selectedFloppyPath = settingsViewModel.settings.floppy0
	val selectedFloppy1Path = settingsViewModel.settings.floppy1
	val selectedCdPath = settingsViewModel.settings.cdImage
	val selectedFloppy = floppies.firstOrNull { it.path == selectedFloppyPath }
	val selectedCd = cds.firstOrNull { it.path == selectedCdPath }
	val mediaFileExists: (String) -> Boolean = remember {
		{ path -> File(path).exists() }
	}
	val mediaSummaryWhdload = stringResource(R.string.quick_start_media_whdload)
	val mediaSummaryBoth = stringResource(R.string.quick_start_media_floppy_cd)
	val mediaSummaryFloppy = stringResource(R.string.quick_start_media_floppy)
	val mediaSummaryCd = stringResource(R.string.quick_start_media_cd)
	val mediaSummaryNone = stringResource(R.string.quick_start_media_none)
	val noCompatibleModelMessage = stringResource(R.string.quick_start_no_compatible_models_message)
	val configWriteFailedMessage = stringResource(R.string.quick_start_config_write_failed)
	val scanningMediaDescription = stringResource(R.string.quick_start_scanning_media)
	var floppyImportTarget by remember { mutableStateOf(0) }
	var whdloadAutoConfigPath by remember { mutableStateOf<String?>(null) }
	val mediaSummary = when {
		selectedWhdloadGame != null -> mediaSummaryWhdload
		selectedFloppyPath.isNotBlank() && selectedCdPath.isNotBlank() -> mediaSummaryBoth
		selectedFloppyPath.isNotBlank() -> mediaSummaryFloppy
		selectedCdPath.isNotBlank() -> mediaSummaryCd
		else -> mediaSummaryNone
	}
	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)

	LaunchedEffect(availableModels, model) {
		val firstAvailableModel = availableModels.firstOrNull()
		if (firstAvailableModel != null && model !in availableModels) {
			settingsViewModel.applyModel(firstAvailableModel)
		}
	}

	LaunchedEffect(selectedWhdloadGame?.path) {
		val game = selectedWhdloadGame
		if (game == null) {
			whdloadAutoConfigPath = null
			return@LaunchedEffect
		}
		if (whdloadAutoConfigPath != game.path) {
			whdloadAutoConfigPath = game.path
			settingsViewModel.applyWhdLoadAutoConfig(game)
		}
	}

	LaunchedEffect(isScanning, floppies, cds, selectedFloppyPath, selectedFloppy1Path, selectedCdPath) {
		val currentSettings = settingsViewModel.settings
		val prunedSettings = QuickStartMediaState.pruneMissingMedia(
			settings = currentSettings,
			floppies = floppies,
			cds = cds,
			isScanning = isScanning,
			fileExists = mediaFileExists
		)
		if (prunedSettings != currentSettings) {
			settingsViewModel.updateSettings { settings ->
				QuickStartMediaState.pruneMissingMedia(
					settings = settings,
					floppies = floppies,
					cds = cds,
					isScanning = isScanning,
					fileExists = mediaFileExists
				)
			}
		}
	}

	fun importResultMessage(result: FileManager.ImportResult): String {
		val message = ImportFeedback.fromImportResult(result)
		return context.getString(message.stringRes, message.argument)
	}

	val floppyPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			scope.launchImportGuarded(importGuard) {
				val result = withContext(Dispatchers.IO) {
					FileManager.importFileWithResult(context, it, FileCategory.FLOPPIES).also { importResult ->
						if (importResult is FileManager.ImportResult.Imported) {
							FileRepository.getInstance(context).rescanCategory(FileCategory.FLOPPIES)
						}
					}
				}
				if (result is FileManager.ImportResult.Imported) {
					settingsViewModel.updateSettings { s ->
						if (floppyImportTarget == 1) {
							s.copy(floppy1 = result.path, floppy1Type = 0)
						} else {
							s.copy(floppy0 = result.path)
						}
					}
				}
				snackbarHostState.showSnackbar(importResultMessage(result))
			}
		}
	}

	val cdPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			scope.launchImportGuarded(importGuard) {
				val result = withContext(Dispatchers.IO) {
					FileManager.importFileWithResult(context, it, FileCategory.CD_IMAGES).also { importResult ->
						if (importResult is FileManager.ImportResult.Imported) {
							FileRepository.getInstance(context).rescanCategory(FileCategory.CD_IMAGES)
						}
					}
				}
				if (result is FileManager.ImportResult.Imported) {
					settingsViewModel.updateSettings { s -> s.copy(cdImage = result.path) }
				}
				snackbarHostState.showSnackbar(importResultMessage(result))
			}
		}
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
				snackbarHostState.showSnackbar(importResultMessage(result))
			}
		}
	}

	val whdloadPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			scope.launchImportGuarded(importGuard) {
				val result = withContext(Dispatchers.IO) {
					FileManager.importFileWithResult(context, it, FileCategory.WHDLOAD_GAMES).also { importResult ->
						if (importResult is FileManager.ImportResult.Imported) {
							FileRepository.getInstance(context).rescanCategory(FileCategory.WHDLOAD_GAMES)
						}
					}
				}
				if (result is FileManager.ImportResult.Imported) {
					val game = FileRepository.getInstance(context).whdloadGames.value.firstOrNull { game ->
						game.path == result.path
					}
					if (game != null) viewModel.selectWhdload(game)
				}
				snackbarHostState.showSnackbar(importResultMessage(result))
			}
		}
	}

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.quick_start_title)) },
				actions = {
					IconButton(onClick = {
						navController?.navigate(Screen.About.route)
					}) {
						Icon(
							Icons.Default.Info,
							contentDescription = stringResource(R.string.nav_about)
						)
					}
				}
			)
		},
		floatingActionButton = {
			if (canStart) {
				ExtendedFloatingActionButton(
					onClick = {
						if (launchInProgress) {
							return@ExtendedFloatingActionButton
						}
						if (selectedWhdloadGame != null) {
							val whdloadPath = selectedWhdloadGame.path
							val controlSettings = settingsViewModel.settings
							scope.launchGuarded(launchGuard) {
								val configFile = try {
									withContext(Dispatchers.IO) {
										AndroidLaunchConfig.writeControlConfig(context, controlSettings)
									}
								} catch (_: Exception) {
									snackbarHostState.showSnackbar(configWriteFailedMessage)
									return@launchGuarded
								}
								EmulatorLauncher.launchWhdload(context, whdloadPath, configFile.absolutePath)
							}
						} else {
							val launchSettings = settingsViewModel.settings
							val launchUnknownLines = settingsViewModel.currentUnknownLines
							scope.launchGuarded(launchGuard) {
								val configFile = try {
									withContext(Dispatchers.IO) {
										settingsViewModel.writeSettingsConfig(
											launchSettings,
											AndroidLaunchConfig.QUICKSTART_CONFIG,
											launchUnknownLines
										)
									}
								} catch (_: Exception) {
									snackbarHostState.showSnackbar(configWriteFailedMessage)
									return@launchGuarded
								}
								EmulatorLauncher.launchQuickStart(
									context = context,
									model = launchSettings.baseModel,
									floppyPath = launchSettings.floppy0.takeIf { it.isNotBlank() },
									floppy1Path = launchSettings.floppy1.takeIf { it.isNotBlank() },
									cdPath = launchSettings.cdImage.takeIf { it.isNotBlank() },
									configPath = configFile.absolutePath
								)
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
		}
	) { innerPadding ->
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
				.verticalScroll(rememberScrollState())
				.padding(16.dp),
			verticalArrangement = Arrangement.spacedBy(16.dp)
		) {
			if (isScanning) {
				LinearProgressIndicator(
					modifier = Modifier
						.fillMaxWidth()
						.semantics { contentDescription = scanningMediaDescription }
				)
			}

			StoragePermissionBanner(
				onMessage = { message ->
					scope.launch { snackbarHostState.showSnackbar(actionMessage(message)) }
				}
			)

			val appPreferences = remember { AppPreferences.getInstance(context) }
			val hasSeenWelcome by appPreferences.hasSeenWelcome

			if (!hasSeenWelcome) {
				Card(
					modifier = Modifier.fillMaxWidth(),
					colors = CardDefaults.cardColors(
						containerColor = MaterialTheme.colorScheme.tertiaryContainer
					)
				) {
					Column(modifier = Modifier.padding(16.dp)) {
						Row(verticalAlignment = Alignment.CenterVertically) {
							Icon(
								Icons.Default.Info,
								contentDescription = null,
								modifier = Modifier.size(24.dp),
								tint = MaterialTheme.colorScheme.onTertiaryContainer
							)
							Spacer(modifier = Modifier.width(8.dp))
							Text(
								stringResource(R.string.welcome_title),
								style = MaterialTheme.typography.titleMedium,
								color = MaterialTheme.colorScheme.onTertiaryContainer
							)
						}
						Spacer(modifier = Modifier.height(8.dp))
						Text(
							text = stringResource(R.string.welcome_message),
							style = MaterialTheme.typography.bodyMedium,
							color = MaterialTheme.colorScheme.onTertiaryContainer
						)
						Spacer(modifier = Modifier.height(8.dp))
						Row(
							modifier = Modifier.fillMaxWidth(),
							horizontalArrangement = Arrangement.End
						) {
							TextButton(onClick = { appPreferences.setHasSeenWelcome(true) }) {
								Text(stringResource(R.string.welcome_dismiss))
							}
						}
					}
				}
			}

			if (!hasRoms || availableModels.isEmpty()) {
				Card(
					modifier = Modifier.fillMaxWidth(),
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
								if (hasRoms) {
									stringResource(R.string.quick_start_no_compatible_models)
								} else {
									stringResource(R.string.setup_required_title)
								},
								style = MaterialTheme.typography.titleMedium,
								color = MaterialTheme.colorScheme.onErrorContainer
							)
						}
						Spacer(modifier = Modifier.height(8.dp))
						Text(
							text = if (hasRoms) {
								noCompatibleModelMessage
							} else {
								stringResource(R.string.setup_required_message)
							},
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

			MediaSelector(
				title = stringResource(R.string.quick_start_whdload_title),
				icon = Icons.Default.SportsEsports,
				items = whdloadGames,
				selectedItem = selectedWhdloadGame,
				selectedPath = selectedWhdloadGame?.path ?: "",
				emptyText = stringResource(R.string.quick_start_no_whdload_games),
				placeholder = stringResource(R.string.quick_start_select_game_placeholder),
				helpText = stringResource(R.string.quick_start_whdload_help),
				onItemSelected = { game -> viewModel.selectWhdload(game) },
				onEject = { viewModel.selectWhdload(null) },
				onImport = {
					if (!importInProgress) {
						whdloadPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.WHDLOAD_GAMES))
					}
				},
				importInProgress = importInProgress,
				displayName = { it.name.removeSuffix(".lha").removeSuffix(".lzx").removeSuffix(".lzh") }
			)

			HorizontalDivider()

			// Recent launches — reads observable state, recomposes when addRecentLaunch() is called
			val allRecent by appPreferences.recentLaunches
			val recentFileExists: (String) -> Boolean = remember {
				{ path -> java.io.File(path).exists() }
			}
			val recentLaunches = remember(allRecent, availableModels) {
				RecentLaunches.available(allRecent, availableModels, recentFileExists)
			}
			val storedRecentLaunches = remember(allRecent) {
				RecentLaunches.pruneMissingFiles(allRecent, recentFileExists)
			}
			LaunchedEffect(allRecent, storedRecentLaunches) {
				if (!RecentLaunches.sameEntries(allRecent, storedRecentLaunches)) {
					appPreferences.replaceRecentLaunches(storedRecentLaunches)
				}
			}
			if (recentLaunches.isNotEmpty()) {
				OutlinedCard(modifier = Modifier.fillMaxWidth()) {
					Column(modifier = Modifier.padding(16.dp)) {
						Row(verticalAlignment = Alignment.CenterVertically) {
							Icon(Icons.Default.History, contentDescription = null, modifier = Modifier.size(20.dp))
							Spacer(modifier = Modifier.width(8.dp))
							Text(stringResource(R.string.quick_start_recent_title), style = MaterialTheme.typography.titleMedium)
						}
						Spacer(modifier = Modifier.height(8.dp))
						recentLaunches.take(5).forEach { entry ->
							val label = RecentLaunches.label(entry)
							TextButton(
								onClick = {
									when (entry.optString("type")) {
										"config" -> {
											val configPath = entry.optString("path")
											val controlSettings = settingsViewModel.settings
											scope.launchGuarded(launchGuard) {
												EmulatorLauncher.launchWithConfig(
													context,
													configPath,
													controlSettings = controlSettings
												)
											}
										}
										"whdload" -> {
											val whdloadPath = entry.optString("path")
											val controlSettings = settingsViewModel.settings
											scope.launchGuarded(launchGuard) {
												val configFile = try {
													withContext(Dispatchers.IO) {
														AndroidLaunchConfig.writeControlConfig(context, controlSettings)
													}
												} catch (_: Exception) {
													snackbarHostState.showSnackbar(configWriteFailedMessage)
													return@launchGuarded
												}
												EmulatorLauncher.launchWhdload(context, whdloadPath, configFile.absolutePath)
											}
										}
										"quickstart" -> {
											val model = AmigaModel.entries.firstOrNull { it.cmdArg == entry.optString("model") } ?: AmigaModel.A500
											if (model in availableModels) {
												val recentFloppy0 = entry.optString("df0")
												val recentFloppy1 = entry.optString("df1")
												val recentCd = entry.optString("cd")
												val currentSettings = settingsViewModel.settings
												scope.launchGuarded(launchGuard) {
													val configFile = try {
														withContext(Dispatchers.IO) {
															val settingsSnapshot = buildRecentQuickStartSettingsSnapshot(
																model = model,
																roms = roms,
																currentSettings = currentSettings,
																floppy0 = recentFloppy0,
																floppy1 = recentFloppy1,
																cdImage = recentCd
															)
															settingsViewModel.writeSettingsConfig(settingsSnapshot, AndroidLaunchConfig.QUICKSTART_CONFIG)
														}
													} catch (_: Exception) {
														snackbarHostState.showSnackbar(configWriteFailedMessage)
														return@launchGuarded
													}
													EmulatorLauncher.launchQuickStart(context, model,
														floppyPath = recentFloppy0.takeIf { it.isNotBlank() },
														floppy1Path = recentFloppy1.takeIf { it.isNotBlank() },
														cdPath = recentCd.takeIf { it.isNotBlank() },
														configPath = configFile.absolutePath)
												}
											} else {
												scope.launch {
													snackbarHostState.showSnackbar(noCompatibleModelMessage)
												}
											}
										}
									}
								},
								enabled = !launchInProgress,
								modifier = Modifier.fillMaxWidth()
							) {
								Text(label, modifier = Modifier.fillMaxWidth())
							}
						}
					}
				}
			}

			Card(
				modifier = Modifier.fillMaxWidth(),
				colors = CardDefaults.cardColors(
					containerColor = MaterialTheme.colorScheme.primaryContainer
				)
			) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(stringResource(R.string.quick_start_ready_title), style = MaterialTheme.typography.titleLarge)
					Spacer(modifier = Modifier.height(4.dp))
					Text(
						text = stringResource(R.string.quick_start_ready_subtitle),
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
					Spacer(modifier = Modifier.height(12.dp))
					Text(
						text = stringResource(
							R.string.quick_start_model_summary,
							if (selectedModelAvailable) model.displayName else stringResource(R.string.quick_start_no_compatible_models)
						),
						style = MaterialTheme.typography.bodyMedium
					)
					Text(mediaSummary, style = MaterialTheme.typography.bodyMedium)
				}
			}

			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(stringResource(R.string.quick_start_amiga_model), style = MaterialTheme.typography.titleMedium)
					Spacer(modifier = Modifier.height(8.dp))

					var modelExpanded by remember { mutableStateOf(false) }
					ExposedDropdownMenuBox(
						expanded = modelExpanded,
						onExpandedChange = { modelExpanded = it && availableModels.isNotEmpty() }
					) {
						OutlinedTextField(
							value = if (selectedModelAvailable) {
								settingsViewModel.settings.baseModel.displayName
							} else {
								stringResource(R.string.quick_start_no_compatible_models)
							},
							onValueChange = {},
							readOnly = true,
							enabled = availableModels.isNotEmpty(),
							trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = modelExpanded) },
							modifier = Modifier
								.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
								.fillMaxWidth(),
							supportingText = {
								Text(
									if (selectedModelAvailable) {
										settingsViewModel.settings.baseModel.description
									} else {
										stringResource(R.string.quick_start_no_compatible_models_help)
									}
								)
							}
						)
						ExposedDropdownMenu(
							expanded = modelExpanded,
							onDismissRequest = { modelExpanded = false }
						) {
							availableModels.forEach { m ->
								DropdownMenuItem(
									text = {
										Column {
											Text(m.displayName)
											Text(
												m.description,
												style = MaterialTheme.typography.bodySmall,
												color = MaterialTheme.colorScheme.onSurfaceVariant
											)
										}
									},
									onClick = {
										settingsViewModel.applyModel(m)
										modelExpanded = false
									}
								)
							}
						}
					}
				}
			}

			if (selectedModelAvailable && model.hasFloppy) {
				MediaSelector(
					title = stringResource(R.string.quick_start_floppy_df0),
					icon = Icons.Default.SaveAlt,
					items = floppies,
					selectedItem = selectedFloppy,
					selectedPath = selectedFloppyPath,
					emptyText = stringResource(R.string.quick_start_no_floppy_images),
					onItemSelected = { file ->
						settingsViewModel.updateSettings { s -> s.copy(floppy0 = file.path) }
					},
					onEject = {
						settingsViewModel.updateSettings { s -> s.copy(floppy0 = "") }
					},
					onImport = {
						if (!importInProgress) {
							floppyImportTarget = 0
							floppyPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.FLOPPIES))
						}
					},
					importInProgress = importInProgress
				)

				val selectedFloppy1 = floppies.firstOrNull { it.path == selectedFloppy1Path }
				MediaSelector(
					title = stringResource(R.string.quick_start_floppy_df1),
					icon = Icons.Default.SaveAlt,
					items = floppies,
					selectedItem = selectedFloppy1,
					selectedPath = selectedFloppy1Path,
					emptyText = stringResource(R.string.quick_start_no_floppy_images),
					onItemSelected = { file ->
						settingsViewModel.updateSettings { s -> s.copy(floppy1 = file.path, floppy1Type = 0) }
					},
					onEject = {
						settingsViewModel.updateSettings { s -> s.copy(floppy1 = "", floppy1Type = -1) }
					},
					onImport = {
						if (!importInProgress) {
							floppyImportTarget = 1
							floppyPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.FLOPPIES))
						}
					},
					importInProgress = importInProgress
				)
			}

			if (selectedModelAvailable && model.hasCd) {
				MediaSelector(
					title = stringResource(R.string.quick_start_cd_image),
					icon = Icons.Default.Album,
					items = cds,
					selectedItem = selectedCd,
					selectedPath = selectedCdPath,
					emptyText = stringResource(R.string.quick_start_no_cd_images),
					onItemSelected = { file ->
						settingsViewModel.updateSettings { s -> s.copy(cdImage = file.path) }
					},
					onEject = {
						settingsViewModel.updateSettings { s -> s.copy(cdImage = "") }
					},
					onImport = {
						if (!importInProgress) {
							cdPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.CD_IMAGES))
						}
					},
					importInProgress = importInProgress
				)
			}

			Spacer(modifier = Modifier.height(80.dp))
		}
	}
}

