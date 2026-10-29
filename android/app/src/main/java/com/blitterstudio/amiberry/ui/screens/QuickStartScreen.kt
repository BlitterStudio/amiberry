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
import androidx.compose.runtime.produceState
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.vector.ImageVector
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
import com.blitterstudio.amiberry.data.WhdLoadPerGameConfig
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomReadinessDiagnostics
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.ui.dpadArrowsMoveFocus
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.launchImportGuarded
import com.blitterstudio.amiberry.ui.launchGuarded
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.rememberLaunchInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.QuickStartViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.ui.components.MediaSelector
import com.blitterstudio.amiberry.ui.components.settings.CompatibilityLevelRow
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
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
	val appPreferences = remember { AppPreferences.getInstance(context) }
	val selectedLaunchMode = viewModel.selectedLaunchMode
	val model = settingsViewModel.settings.baseModel
	val floppies by viewModel.availableFloppies.collectAsState()
	val cds by viewModel.availableCds.collectAsState()
	val roms by viewModel.availableRoms.collectAsState()
	val whdloadGames by viewModel.availableWhdloadGames.collectAsState()
	val selectedWhdloadGame = viewModel.selectedWhdload
	val isScanning by FileRepository.getInstance(context).isScanning.collectAsState()
	val hasRoms = roms.isNotEmpty()
	val availableModels = remember(roms) { ModelRomAvailability.availableModels(roms) }
	val romDiagnostics = remember(roms) { RomReadinessDiagnostics.from(roms) }
	// Models whose Kickstart is not installed: they boot the built-in AROS ROM.
	val arosModels = remember(roms, availableModels) {
		availableModels.filterTo(mutableSetOf()) { !ModelRomAvailability.hasKickstart(it, roms) }
	}
	val selectedModelRoms = remember(roms, model) { ModelRomAvailability.selectRomsForModel(model, roms) }
	val selectedModelAvailable = model in availableModels
	val modeAvailableModels = remember(availableModels, selectedLaunchMode) {
		when (selectedLaunchMode) {
			QuickStartLaunchMode.CD -> availableModels.filter { it.hasCd }
			QuickStartLaunchMode.DISK -> availableModels.filter { it.hasFloppy }
			QuickStartLaunchMode.WHDLOAD,
			QuickStartLaunchMode.RECENT -> availableModels
		}
	}
	val selectedModeModelAvailable = model in modeAvailableModels
	val canStart = QuickStartUiState.canStart(
		launchMode = selectedLaunchMode,
		hasRoms = hasRoms,
		selectedModelAvailable = selectedModeModelAvailable,
		hasSelectedWhdloadGame = selectedWhdloadGame != null
	)
	val allRecent by appPreferences.recentLaunches
	val recentFileExists: (String) -> Boolean = remember {
		{ path -> File(path).exists() }
	}
	val recentItems = remember(allRecent, availableModels) {
		RecentLaunches.items(allRecent, availableModels, recentFileExists)
	}
	val hasSeenWelcome by appPreferences.hasSeenWelcome
	val quickStartModeFocusRequester = remember { FocusRequester() }
	var requestQuickStartModeFocus by remember { mutableStateOf(false) }
	LaunchedEffect(hasSeenWelcome, requestQuickStartModeFocus) {
		if (hasSeenWelcome && requestQuickStartModeFocus) {
			quickStartModeFocusRequester.requestFocus()
			requestQuickStartModeFocus = false
		}
	}

	val selectedFloppyPath = settingsViewModel.settings.floppy0
	val selectedFloppy1Path = settingsViewModel.settings.floppy1
	val selectedCdPath = settingsViewModel.settings.cdImage
	val selectedFloppy = floppies.firstOrNull { it.path == selectedFloppyPath }
	val selectedCd = cds.firstOrNull { it.path == selectedCdPath }
	val mediaFileExists: (String) -> Boolean = remember {
		{ path -> File(path).exists() }
	}
	val noCompatibleModelMessage = stringResource(R.string.quick_start_no_compatible_models_message)
	val configWriteFailedMessage = stringResource(R.string.quick_start_config_write_failed)
	val scanningMediaDescription = stringResource(R.string.quick_start_scanning_media)
	var floppyImportTarget by remember { mutableStateOf(0) }
	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)

	LaunchedEffect(availableModels, modeAvailableModels, model, selectedLaunchMode) {
		val selectableModels = if (selectedLaunchMode == QuickStartLaunchMode.DISK ||
			selectedLaunchMode == QuickStartLaunchMode.CD
		) {
			modeAvailableModels
		} else {
			availableModels
		}
		// A model with its own Kickstart comes before one that would boot AROS.
		val firstAvailableModel = selectableModels.firstOrNull { it !in arosModels } ?: selectableModels.firstOrNull()
		if (firstAvailableModel != null && model !in selectableModels) {
			settingsViewModel.applyModel(firstAvailableModel)
		}
	}

	// The WHDLoad summary reads the game's own configuration, if it has one, off the main thread.
	val whdloadSummary by produceState<QuickStartWhdLoadSummary?>(
		initialValue = null,
		selectedLaunchMode,
		selectedWhdloadGame,
		settingsViewModel.settings
	) {
		if (selectedLaunchMode != QuickStartLaunchMode.WHDLOAD) return@produceState
		val game = selectedWhdloadGame
		val currentSettings = settingsViewModel.settings
		value = withContext(Dispatchers.IO) {
			val perGameConfig = game?.let { WhdLoadPerGameConfig.existingFile(context, it.path) }
			QuickStartUiState.buildWhdLoadSummary(
				launchSettings = WhdLoadPerGameConfig.launch(perGameConfig, currentSettings).settings,
				selectedWhdloadName = game?.name,
				perGameConfigName = perGameConfig?.name
			)
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
				if (result is FileManager.ImportResult.Imported) {
					// Settings that boot AROS switch to the model's Kickstart once it is
					// installed; otherwise Start would keep booting AROS.
					val installedRoms = FileRepository.getInstance(context).roms.value
					QuickStartUiState.withInstalledKickstart(settingsViewModel.settings, installedRoms)?.let { updated ->
						settingsViewModel.updateSettings { updated }
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
					IconButton(
						onClick = { navController?.navigate(Screen.About.route) },
						modifier = Modifier.dpadFocusIndicator()
					) {
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
						when (selectedLaunchMode) {
							QuickStartLaunchMode.WHDLOAD -> {
								val whdloadPath = selectedWhdloadGame?.path ?: return@ExtendedFloatingActionButton
								val controlSettings = settingsViewModel.settings
								scope.launchGuarded(launchGuard) {
									val prepared = try {
										withContext(Dispatchers.IO) {
											AndroidLaunchConfig.prepareWhdLoad(context, whdloadPath, controlSettings)
										}
									} catch (_: Exception) {
										snackbarHostState.showSnackbar(configWriteFailedMessage)
										return@launchGuarded
									}
									EmulatorLauncher.launchWhdload(
										context = context,
										lhaPath = whdloadPath,
										configPath = prepared.configFile.absolutePath,
										shaderOverride = prepared.shaderOverride,
										controlOverrides = prepared.controlOverrides
									)
								}
							}
							QuickStartLaunchMode.DISK,
							QuickStartLaunchMode.CD -> {
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
										configPath = configFile.absolutePath,
										floppyPath = launchSettings.floppy0.takeIf { it.isNotBlank() },
										floppy1Path = launchSettings.floppy1.takeIf { it.isNotBlank() },
										cdPath = launchSettings.cdImage.takeIf { it.isNotBlank() }
									)
								}
							}
							QuickStartLaunchMode.RECENT -> Unit
						}
					},
					modifier = Modifier.dpadFocusIndicator(),
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
				.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
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
							TextButton(
								onClick = {
									requestQuickStartModeFocus = true
									appPreferences.setHasSeenWelcome(true)
								},
								modifier = Modifier.dpadFocusIndicator()
							) {
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
						RomReadinessSummary(romDiagnostics)
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

			if (selectedLaunchMode == QuickStartLaunchMode.WHDLOAD) {
				if (QuickStartUiState.showsWhdLoadArosNotice(roms)) {
					ArosFallbackNotice(
						message = stringResource(R.string.quick_start_whdload_aros_notice),
						importInProgress = importInProgress,
						onImportRom = {
							if (!importInProgress) {
								romPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.ROMS))
							}
						}
					)
				}
			} else if (QuickStartUiState.showsArosNotice(settingsViewModel.settings, selectedModelRoms, selectedLaunchMode)) {
				ArosFallbackNotice(
					message = stringResource(R.string.quick_start_aros_notice),
					importInProgress = importInProgress,
					onImportRom = {
						if (!importInProgress) {
							romPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.ROMS))
						}
					}
				)
			}

			QuickStartModeSelector(
				selectedMode = selectedLaunchMode,
				onModeSelected = viewModel::selectLaunchMode,
				focusRequester = quickStartModeFocusRequester
			)

			when (selectedLaunchMode) {
				QuickStartLaunchMode.WHDLOAD -> {
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
				}
				QuickStartLaunchMode.DISK -> {
					ModelSelectorCard(
						settings = settingsViewModel.settings,
						availableModels = modeAvailableModels,
						selectedModelAvailable = selectedModeModelAvailable,
						arosModels = arosModels,
						onModelSelected = settingsViewModel::applyModel,
						onLevelSelected = settingsViewModel::applyCompatibilityLevel
					)
					if (selectedModeModelAvailable && model.hasFloppy) {
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
				}
				QuickStartLaunchMode.CD -> {
					ModelSelectorCard(
						settings = settingsViewModel.settings,
						availableModels = modeAvailableModels,
						selectedModelAvailable = selectedModeModelAvailable,
						arosModels = arosModels,
						onModelSelected = settingsViewModel::applyModel,
						onLevelSelected = settingsViewModel::applyCompatibilityLevel
					)
					if (selectedModeModelAvailable && model.hasCd) {
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
				}
				QuickStartLaunchMode.RECENT -> {
					QuickStartRecentSection(
						items = recentItems,
						launchInProgress = launchInProgress,
						onLaunch = { entry ->
							when (entry.optString("type")) {
								"config" -> {
									val configPath = entry.optString("path")
									// Not the editor state: that may belong to another configuration.
									val rememberedControls = appPreferences.rememberedAndroidControls()
									scope.launchGuarded(launchGuard) {
										EmulatorLauncher.launchWithConfig(
											context,
											configPath,
											rememberedControls = rememberedControls
										)
									}
								}
								"whdload" -> {
									val whdloadPath = entry.optString("path")
									val controlSettings = settingsViewModel.settings
									scope.launchGuarded(launchGuard) {
										val prepared = try {
											withContext(Dispatchers.IO) {
												AndroidLaunchConfig.prepareWhdLoad(context, whdloadPath, controlSettings)
											}
										} catch (_: Exception) {
											snackbarHostState.showSnackbar(configWriteFailedMessage)
											return@launchGuarded
										}
										EmulatorLauncher.launchWhdload(
											context = context,
											lhaPath = whdloadPath,
											configPath = prepared.configFile.absolutePath,
											shaderOverride = prepared.shaderOverride,
											controlOverrides = prepared.controlOverrides
										)
									}
								}
								"rp9" -> {
									val rp9Path = entry.optString("path")
									val controlSettings = settingsViewModel.settings
									scope.launchGuarded(launchGuard) {
										EmulatorLauncher.launchRp9(context, rp9Path, controlSettings)
									}
								}
								"quickstart" -> {
									val model = AmigaModel.find(entry.optString("model")) ?: AmigaModel.A500
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
											EmulatorLauncher.launchQuickStart(
												context,
												model,
												configPath = configFile.absolutePath,
												floppyPath = recentFloppy0.takeIf { it.isNotBlank() },
												floppy1Path = recentFloppy1.takeIf { it.isNotBlank() },
												cdPath = recentCd.takeIf { it.isNotBlank() }
											)
										}
									} else {
										scope.launch {
											snackbarHostState.showSnackbar(noCompatibleModelMessage)
										}
									}
								}
							}
						},
						onRemove = { entry -> appPreferences.removeRecentLaunch(entry) },
						onClear = { appPreferences.clearRecentLaunches() },
						// Nothing in the emptied section can hold focus: return it to the Recent mode button.
						onEmptied = { quickStartModeFocusRequester.requestFocus() }
					)
				}
			}

			if (selectedLaunchMode == QuickStartLaunchMode.WHDLOAD) {
				whdloadSummary?.let { WhdLoadSummaryCard(it) }
			} else {
				EffectiveSummaryCard(
					summary = QuickStartUiState.buildEffectiveSummary(
						settings = settingsViewModel.settings,
						launchMode = selectedLaunchMode,
						selectedModelAvailable = if (selectedLaunchMode == QuickStartLaunchMode.DISK ||
							selectedLaunchMode == QuickStartLaunchMode.CD
						) {
							selectedModeModelAvailable
						} else {
							selectedModelAvailable
						},
						roms = roms
					)
				)
			}

		}
	}
}

@Composable
private fun RomReadinessSummary(diagnostics: RomReadinessDiagnostics) {
	Column(
		modifier = Modifier.padding(top = 12.dp),
		verticalArrangement = Arrangement.spacedBy(4.dp)
	) {
		Text(
			stringResource(R.string.rom_readiness_scanned, diagnostics.scannedRomCount),
			style = MaterialTheme.typography.bodySmall,
			color = MaterialTheme.colorScheme.onErrorContainer
		)
		Text(
			stringResource(R.string.rom_readiness_recognized, diagnostics.recognizedRomCount),
			style = MaterialTheme.typography.bodySmall,
			color = MaterialTheme.colorScheme.onErrorContainer
		)
		if (diagnostics.availableModels.isEmpty()) {
			Text(
				stringResource(R.string.rom_readiness_no_available_models),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onErrorContainer
			)
		} else {
			Text(
				stringResource(
					R.string.rom_readiness_available_models,
					diagnostics.availableModels.joinToString { it.displayName }
				),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onErrorContainer
			)
		}
		if (diagnostics.unknownRomCount > 0) {
			Text(
				stringResource(R.string.rom_readiness_unknown_roms, diagnostics.unknownRomCount),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onErrorContainer
			)
		}
		if (diagnostics.missingCommonModels.isNotEmpty()) {
			Text(
				stringResource(
					R.string.rom_readiness_missing_models,
					diagnostics.missingCommonModels.joinToString { it.displayName }
				),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onErrorContainer
			)
		}
	}
}

@Composable
private fun QuickStartModeSelector(
	selectedMode: QuickStartLaunchMode,
	onModeSelected: (QuickStartLaunchMode) -> Unit,
	focusRequester: FocusRequester
) {
	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(
			modifier = Modifier.padding(16.dp),
			verticalArrangement = Arrangement.spacedBy(8.dp)
		) {
			Text(
				stringResource(R.string.quick_start_launch_mode_title),
				style = MaterialTheme.typography.titleMedium
			)
			Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
				QuickStartModeButton(
					mode = QuickStartLaunchMode.WHDLOAD,
					selectedMode = selectedMode,
					modifier = Modifier.weight(1f),
					focusRequester = focusRequester,
					onModeSelected = onModeSelected
				)
				QuickStartModeButton(
					mode = QuickStartLaunchMode.DISK,
					selectedMode = selectedMode,
					modifier = Modifier.weight(1f),
					focusRequester = focusRequester,
					onModeSelected = onModeSelected
				)
			}
			Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
				QuickStartModeButton(
					mode = QuickStartLaunchMode.CD,
					selectedMode = selectedMode,
					modifier = Modifier.weight(1f),
					focusRequester = focusRequester,
					onModeSelected = onModeSelected
				)
				QuickStartModeButton(
					mode = QuickStartLaunchMode.RECENT,
					selectedMode = selectedMode,
					modifier = Modifier.weight(1f),
					focusRequester = focusRequester,
					onModeSelected = onModeSelected
				)
			}
		}
	}
}

@Composable
private fun QuickStartModeButton(
	mode: QuickStartLaunchMode,
	selectedMode: QuickStartLaunchMode,
	modifier: Modifier,
	focusRequester: FocusRequester,
	onModeSelected: (QuickStartLaunchMode) -> Unit
) {
	val selected = mode == selectedMode
	// One button node for both states: swapping Button/OutlinedButton would drop
	// D-pad focus to the navigation rail when the mode changes.
	Button(
		onClick = { onModeSelected(mode) },
		// The selected mode's button is where focus returns (Welcome dismissed, Recent emptied).
		modifier = modifier
			.then(if (selected) Modifier.focusRequester(focusRequester) else Modifier)
			.dpadFocusIndicator(),
		colors = if (selected) ButtonDefaults.buttonColors() else ButtonDefaults.outlinedButtonColors(),
		elevation = if (selected) ButtonDefaults.buttonElevation() else null,
		border = if (selected) null else ButtonDefaults.outlinedButtonBorder(enabled = true)
	) {
		QuickStartModeButtonContent(mode)
	}
}

@Composable
private fun QuickStartModeButtonContent(mode: QuickStartLaunchMode) {
	Icon(mode.icon(), contentDescription = null, modifier = Modifier.size(18.dp))
	Spacer(modifier = Modifier.width(6.dp))
	Text(stringResource(mode.labelRes()))
}

private fun QuickStartLaunchMode.labelRes(): Int = when (this) {
	QuickStartLaunchMode.WHDLOAD -> R.string.quick_start_mode_whdload
	QuickStartLaunchMode.DISK -> R.string.quick_start_mode_disk
	QuickStartLaunchMode.CD -> R.string.quick_start_mode_cd
	QuickStartLaunchMode.RECENT -> R.string.quick_start_mode_recent
}

private fun QuickStartLaunchMode.icon(): ImageVector = when (this) {
	QuickStartLaunchMode.WHDLOAD -> Icons.Default.SportsEsports
	QuickStartLaunchMode.DISK -> Icons.Default.SaveAlt
	QuickStartLaunchMode.CD -> Icons.Default.Album
	QuickStartLaunchMode.RECENT -> Icons.Default.History
}

/** The model to start, and the compatibility level of its preset. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ModelSelectorCard(
	settings: EmulatorSettings,
	availableModels: List<AmigaModel>,
	selectedModelAvailable: Boolean,
	/** Available models whose Kickstart is not installed, which boot AROS. */
	arosModels: Set<AmigaModel>,
	onModelSelected: (AmigaModel) -> Unit,
	onLevelSelected: (Int) -> Unit
) {
	val model = settings.baseModel
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
						model.displayName
					} else {
						stringResource(R.string.quick_start_no_compatible_models)
					},
					onValueChange = {},
					readOnly = true,
					enabled = availableModels.isNotEmpty(),
					trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = modelExpanded) },
					modifier = Modifier
						.dpadArrowsMoveFocus()
						.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
						.fillMaxWidth(),
					supportingText = {
						Text(
							if (selectedModelAvailable) {
								model.description
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
					availableModels.forEach { availableModel ->
						DropdownMenuItem(
							text = {
								Column {
									Text(availableModel.displayName)
									Text(
										availableModel.description,
										style = MaterialTheme.typography.bodySmall,
										color = MaterialTheme.colorScheme.onSurfaceVariant
									)
									if (availableModel in arosModels) {
										Text(
											stringResource(R.string.quick_start_model_boots_aros),
											style = MaterialTheme.typography.bodySmall,
											color = MaterialTheme.colorScheme.onSurfaceVariant
										)
									}
								}
							},
							onClick = {
								onModelSelected(availableModel)
								modelExpanded = false
							}
						)
					}
				}
			}
			if (selectedModelAvailable) {
				CompatibilityLevelRow(
					settings = settings,
					onSelect = onLevelSelected,
					modifier = Modifier.padding(top = 8.dp)
				)
			}
		}
	}
}

@Composable
private fun SummaryCard(rows: @Composable () -> Unit) {
	Card(
		modifier = Modifier.fillMaxWidth(),
		colors = CardDefaults.cardColors(
			containerColor = MaterialTheme.colorScheme.primaryContainer
		)
	) {
		Column(
			modifier = Modifier.padding(16.dp),
			verticalArrangement = Arrangement.spacedBy(8.dp)
		) {
			Text(stringResource(R.string.quick_start_ready_title), style = MaterialTheme.typography.titleLarge)
			Text(
				text = stringResource(R.string.quick_start_ready_subtitle),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
			rows()
		}
	}
}

@Composable
private fun EffectiveSummaryCard(summary: QuickStartEffectiveSummary) {
	SummaryCard {
		SummaryRow(stringResource(R.string.quick_start_summary_model), summary.model)
		SummaryRow(stringResource(R.string.quick_start_summary_cpu), summary.cpu)
		summary.rom?.let { rom ->
			SummaryRow(
				stringResource(R.string.quick_start_summary_rom),
				when (rom) {
					is RomIdentity.Kickstart -> rom.name
					RomIdentity.BuiltInAros -> stringResource(R.string.rom_aros_builtin)
					RomIdentity.Unknown -> summary.romFileName
				}
			)
		}
		SummaryRow(stringResource(R.string.quick_start_summary_chipset), summary.chipset)
		SummaryRow(stringResource(R.string.quick_start_summary_memory), summary.memory)
		SummaryRow(stringResource(R.string.quick_start_summary_media), summary.media)
		SummaryRow(stringResource(R.string.quick_start_summary_controls), summary.controls)
	}
}

/** A WHDLoad launch: WHDBooter's hardware choice is named, not previewed; the rest is the launcher's. */
@Composable
private fun WhdLoadSummaryCard(summary: QuickStartWhdLoadSummary) {
	val schema = settingsSchema()
	SummaryCard {
		SummaryRow(stringResource(R.string.quick_start_summary_media), summary.media)
		SummaryRow(
			stringResource(R.string.quick_start_summary_hardware),
			summary.perGameConfig?.let { stringResource(R.string.quick_start_whdload_hardware_per_game, it) }
				?: stringResource(R.string.quick_start_whdload_hardware_whdbooter)
		)
		SummaryRow(stringResource(R.string.quick_start_summary_controls), summary.controls)
		SummaryRow(schema?.label("joyport0") ?: "joyport0", settingText(summary.port0))
		SummaryRow(schema?.label("joyport1") ?: "joyport1", settingText(summary.port1))
		SummaryRow(schema?.label("amiberry.shader") ?: "amiberry.shader", settingText(summary.shader))
	}
}

/**
 * Quick Start would boot the free AROS ROM ([message] says when); importing
 * the Kickstart makes it boot its own.
 */
@Composable
private fun ArosFallbackNotice(message: String, importInProgress: Boolean, onImportRom: () -> Unit) {
	Card(
		modifier = Modifier.fillMaxWidth(),
		colors = CardDefaults.cardColors(
			containerColor = MaterialTheme.colorScheme.secondaryContainer
		)
	) {
		Column(modifier = Modifier.padding(16.dp)) {
			Row(verticalAlignment = Alignment.CenterVertically) {
				Icon(
					Icons.Default.Info,
					contentDescription = null,
					modifier = Modifier.size(24.dp),
					tint = MaterialTheme.colorScheme.onSecondaryContainer
				)
				Spacer(modifier = Modifier.width(8.dp))
				Text(
					message,
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onSecondaryContainer
				)
			}
			Spacer(modifier = Modifier.height(12.dp))
			Button(
				onClick = onImportRom,
				enabled = !importInProgress,
				modifier = Modifier.dpadFocusIndicator()
			) {
				Text(stringResource(if (importInProgress) R.string.action_importing else R.string.action_import_rom))
			}
		}
	}
}

@Composable
private fun SummaryRow(label: String, value: String) {
	Row(
		modifier = Modifier.fillMaxWidth(),
		horizontalArrangement = Arrangement.spacedBy(12.dp)
	) {
		Text(
			label,
			modifier = Modifier.weight(0.35f),
			style = MaterialTheme.typography.bodyMedium,
			color = MaterialTheme.colorScheme.onPrimaryContainer
		)
		Text(
			value,
			modifier = Modifier.weight(0.65f),
			style = MaterialTheme.typography.bodyMedium,
			color = MaterialTheme.colorScheme.onPrimaryContainer
		)
	}
}
