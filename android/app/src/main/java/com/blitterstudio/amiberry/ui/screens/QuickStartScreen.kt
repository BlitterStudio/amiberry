package com.blitterstudio.amiberry.ui.screens

import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.fillMaxHeight
import com.blitterstudio.amiberry.ui.windowWidthClass
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
import androidx.compose.material3.SnackbarDuration
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.SnackbarResult
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.semantics.selected
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadFocusable
import com.blitterstudio.amiberry.ui.isTvDevice
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
import androidx.compose.ui.focus.focusRestorer
import com.blitterstudio.amiberry.ui.dpadTrailingPane
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
import com.blitterstudio.amiberry.ui.components.SettingsSummaryCard
import com.blitterstudio.amiberry.ui.components.SummaryCard
import com.blitterstudio.amiberry.ui.components.SummaryRow
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
	val tv = isTvDevice()
	val quickStartModeFocusRequester = remember { FocusRequester() }
	var requestQuickStartModeFocus by remember { mutableStateOf(false) }
	LaunchedEffect(hasSeenWelcome, requestQuickStartModeFocus) {
		if (hasSeenWelcome && requestQuickStartModeFocus) {
			quickStartModeFocusRequester.requestFocus()
			requestQuickStartModeFocus = false
		}
	}
	// The WHDLoad games launched lately, most recent first: the game list shows them first.
	val recentWhdloadPaths = remember(allRecent) {
		allRecent.filter { RecentLaunches.Type.of(it) == RecentLaunches.Type.WhdLoad }.map { it.optString("path") }
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
	val undoLabel = stringResource(R.string.action_undo)
	fun ejectedMessage(change: SettingsViewModel.ModelChange, model: AmigaModel): String {
		val floppies = change.droppedMedia.filter { it != MediaDrive.CD }
		return listOfNotNull(
			floppies.takeIf { it.isNotEmpty() }?.let { drives ->
				context.getString(R.string.quick_start_media_ejected_no_floppy, drives.joinToString { it.label }, model.displayName)
			},
			context.getString(R.string.quick_start_media_ejected_no_cd, model.displayName)
				.takeIf { MediaDrive.CD in change.droppedMedia }
		).joinToString(" ")
	}
	/** The user picked [model]: its preset keeps the inserted media; a disk it has no drive for goes, with Undo. */
	fun selectModel(model: AmigaModel) {
		val change = settingsViewModel.applyModel(model)
		if (change.droppedMedia.isEmpty()) return
		scope.launch {
			val result = snackbarHostState.showSnackbar(
				message = ejectedMessage(change, model),
				actionLabel = undoLabel,
				duration = SnackbarDuration.Long
			)
			if (result == SnackbarResult.ActionPerformed) settingsViewModel.undoModelChange(change)
		}
	}
	fun launchWhdload(whdloadPath: String) {
		if (launchInProgress) return
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
	var floppyImportTarget by remember { mutableStateOf(0) }

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
			// A mode change picks the model: media the new one has no drive for goes quietly
			// (it belongs to the other mode).
			settingsViewModel.applyModel(firstAvailableModel)
		}
	}

	// A Kickstart imported in the Files tab replaces AROS in the Quick Start settings, as
	// an import here does; a named configuration keeps the ROM it was saved with.
	LaunchedEffect(roms) {
		if (settingsViewModel.currentConfigName != null) return@LaunchedEffect
		QuickStartUiState.withInstalledKickstart(settingsViewModel.settings, roms)?.let { updated ->
			settingsViewModel.updateSettings { updated }
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

	val twoPanes = windowWidthClass().showsTwoPanes
	// Up from Start returns to the control last focused in the choices.
	val launchColumnFocus = remember { FocusRequester() }
	val startLabel = stringResource(if (launchInProgress) R.string.action_launching else R.string.action_start)
	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.quick_start_title)) },
				// A phone held sideways has little height: a lower bar leaves more for the choices.
				expandedHeight = if (LocalConfiguration.current.screenHeightDp < COMPACT_HEIGHT_DP) {
					COMPACT_TOP_BAR_HEIGHT
				} else {
					TopAppBarDefaults.TopAppBarExpandedHeight
				},
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
								launchWhdload(whdloadPath)
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
					modifier = Modifier
						.focusProperties { up = launchColumnFocus }
						.dpadFocusIndicator()
						// The text slot alone left the button nameless to accessibility services.
						.semantics { contentDescription = startLabel },
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
					text = { Text(startLabel) }
				)
			}
		}
	) { innerPadding ->
		val scanningIndicator: @Composable () -> Unit = {
			if (isScanning) {
				LinearProgressIndicator(
					modifier = Modifier
						.fillMaxWidth()
						.semantics { contentDescription = scanningMediaDescription }
				)
			}
		}
		val importRom: () -> Unit = {
			if (!importInProgress) romPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.ROMS))
		}
		val effectiveSummary = QuickStartUiState.buildEffectiveSummary(
			settings = settingsViewModel.settings,
			launchMode = selectedLaunchMode,
			selectedModelAvailable = if (selectedLaunchMode == QuickStartLaunchMode.DISK ||
				selectedLaunchMode == QuickStartLaunchMode.CD
			) {
				selectedModeModelAvailable
			} else {
				selectedModelAvailable
			},
			roms = roms,
			fileExists = mediaFileExists
		)
		// The first-run card, ROM notices: above the launch mode on a phone, above the
		// summary in the right column on a wide window, so the launch mode stays at the top.
		val noticesSection: @Composable ColumnScope.() -> Unit = {
			if (!hasSeenWelcome) {
				WelcomeCard(
					launchMode = selectedLaunchMode,
					kickstartInstalled = availableModels.any { it !in arosModels },
					importInProgress = importInProgress,
					onImportRom = importRom,
					onDismiss = {
						requestQuickStartModeFocus = true
						appPreferences.setHasSeenWelcome(true)
					}
				)
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
							onClick = importRom,
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

			val diskOrCd = selectedLaunchMode == QuickStartLaunchMode.DISK || selectedLaunchMode == QuickStartLaunchMode.CD
			when {
				// The Kickstart the settings name was deleted: say so whatever else shows.
				diskOrCd && effectiveSummary.romMissing -> ArosFallbackNotice(
					message = stringResource(
						if (effectiveSummary.romFallsBackToAros) {
							R.string.quick_start_rom_missing_notice_aros
						} else {
							R.string.quick_start_rom_missing_notice
						},
						effectiveSummary.romFileName
					),
					importInProgress = importInProgress,
					onImportRom = importRom
				)
				// The first-run card already offers the Kickstart import.
				!hasSeenWelcome -> Unit
				// Which games boot AROS matters once there is one to start.
				selectedLaunchMode == QuickStartLaunchMode.WHDLOAD -> {
					if (selectedWhdloadGame != null && QuickStartUiState.showsWhdLoadArosNotice(roms)) {
						ArosFallbackNotice(
							message = stringResource(R.string.quick_start_whdload_aros_notice),
							importInProgress = importInProgress,
							onImportRom = importRom
						)
					}
				}
				QuickStartUiState.showsArosNotice(settingsViewModel.settings, selectedModelRoms, selectedLaunchMode) ->
					ArosFallbackNotice(
						message = stringResource(R.string.quick_start_aros_notice),
						importInProgress = importInProgress,
						onImportRom = importRom
					)
			}
		}
		// The launch mode and its media: the left column on a wide window.
		val launchSection: @Composable ColumnScope.() -> Unit = {
			val recentRowsAreEntry = selectedLaunchMode == QuickStartLaunchMode.RECENT && recentItems.isNotEmpty()
			QuickStartModeSelector(
				selectedMode = selectedLaunchMode,
				onModeSelected = viewModel::selectLaunchMode,
				focusRequester = quickStartModeFocusRequester,
				isContentEntry = !recentRowsAreEntry
			)

			when (selectedLaunchMode) {
				QuickStartLaunchMode.WHDLOAD -> {
					WhdLoadGameCard(
						games = whdloadGames,
						selectedGame = selectedWhdloadGame,
						recentPaths = recentWhdloadPaths,
						importInProgress = importInProgress,
						onImport = {
							if (!importInProgress) {
								whdloadPickerLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.WHDLOAD_GAMES))
							}
						},
						onSelect = { game -> viewModel.selectWhdload(game) },
						onClear = { viewModel.selectWhdload(null) },
						// On a TV one OK on a game starts it, as a Recent row does.
						onLaunch = if (tv && hasRoms) { game -> launchWhdload(game.path) } else null
					)
				}
				QuickStartLaunchMode.DISK -> {
					ModelSelectorCard(
						settings = settingsViewModel.settings,
						availableModels = modeAvailableModels,
						selectedModelAvailable = selectedModeModelAvailable,
						arosModels = arosModels,
						configName = settingsViewModel.currentConfigName,
						onModelSelected = ::selectModel,
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
					if (modeAvailableModels.isEmpty()) {
						// AROS cannot stand in for a CD32 or CDTV: name the ROMs they need.
						CdRomsNeededCard(importInProgress = importInProgress, onImportRom = importRom)
					} else {
						ModelSelectorCard(
							settings = settingsViewModel.settings,
							availableModels = modeAvailableModels,
							selectedModelAvailable = selectedModeModelAvailable,
							arosModels = arosModels,
							configName = settingsViewModel.currentConfigName,
							onModelSelected = ::selectModel,
							onLevelSelected = settingsViewModel::applyCompatibilityLevel
						)
					}
					val cdModelReady = selectedModeModelAvailable && model.hasCd
					// Without a CD model the picker still shows, disabled, so the way ahead is visible.
					MediaSelector(
						title = stringResource(R.string.quick_start_cd_image),
						icon = Icons.Default.Album,
						items = cds,
						selectedItem = selectedCd?.takeIf { cdModelReady },
						selectedPath = if (cdModelReady) selectedCdPath else "",
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
						importInProgress = importInProgress,
						enabled = cdModelReady
					)
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
								"whdload" -> launchWhdload(entry.optString("path"))
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
						onEmptied = { quickStartModeFocusRequester.requestFocus() },
						firstRowIsEntry = recentRowsAreEntry
					)
				}
			}
		}
		// On a TV the summary takes focus (read-only), so the D-pad scrolls it all into view.
		val summaryModifier = if (tv) Modifier.dpadFocusable() else Modifier
		val summarySection: @Composable () -> Unit = {
			when (selectedLaunchMode) {
				QuickStartLaunchMode.WHDLOAD -> whdloadSummary?.let { WhdLoadSummaryCard(it, summaryModifier) }
				// A Recent row starts with its own settings, not these.
				QuickStartLaunchMode.RECENT -> Unit
				QuickStartLaunchMode.DISK,
				QuickStartLaunchMode.CD -> SettingsSummaryCard(
					title = stringResource(R.string.quick_start_ready_title),
					subtitle = stringResource(R.string.quick_start_ready_subtitle),
					summary = effectiveSummary,
					modifier = summaryModifier
				)
			}
		}
		// Nothing scrolls under Start: the scrolled area ends above it.
		val startClearance = if (canStart) START_CLEARANCE else 0.dp
		if (twoPanes) {
			// Wide windows keep what Start launches in view beside the choices.
			Row(
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
					.padding(horizontal = 16.dp),
				horizontalArrangement = Arrangement.spacedBy(16.dp)
			) {
				Column(
					modifier = Modifier
						.weight(1f)
						.fillMaxHeight()
						.focusRequester(launchColumnFocus)
						.focusRestorer()
						.verticalScroll(rememberScrollState())
						.padding(vertical = 16.dp),
					verticalArrangement = Arrangement.spacedBy(16.dp)
				) {
					scanningIndicator()
					launchSection()
				}
				// Start sits over the bottom of this column.
				Column(
					modifier = Modifier
						.weight(1f)
						.fillMaxHeight()
						.padding(bottom = startClearance)
						.dpadTrailingPane(launchColumnFocus)
						.verticalScroll(rememberScrollState())
						.padding(vertical = 16.dp),
					verticalArrangement = Arrangement.spacedBy(16.dp)
				) {
					noticesSection()
					summarySection()
				}
			}
		} else {
			Column(
				modifier = Modifier
					.fillMaxSize()
					.padding(innerPadding)
					.padding(bottom = startClearance)
					.focusRequester(launchColumnFocus)
					.focusRestorer()
					.verticalScroll(rememberScrollState())
					.padding(16.dp),
				verticalArrangement = Arrangement.spacedBy(16.dp)
			) {
				scanningIndicator()
				noticesSection()
				launchSection()
				summarySection()
			}
		}
	}
}

/** A phone held sideways: Quick Start's top bar is lower there. */
private const val COMPACT_HEIGHT_DP = 480
private val COMPACT_TOP_BAR_HEIGHT = 48.dp

/** The height Start (an extended FAB and its margin) covers at the bottom of the screen. */
private val START_CLEARANCE = 88.dp

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
	focusRequester: FocusRequester,
	/** The selected mode is where D-pad focus enters the screen. */
	isContentEntry: Boolean
) {
	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(
			modifier = Modifier
				.padding(16.dp)
				.selectableGroup(),
			verticalArrangement = Arrangement.spacedBy(8.dp)
		) {
			Text(
				stringResource(R.string.quick_start_launch_mode_title),
				style = MaterialTheme.typography.titleMedium
			)
			listOf(
				listOf(QuickStartLaunchMode.WHDLOAD, QuickStartLaunchMode.DISK),
				listOf(QuickStartLaunchMode.CD, QuickStartLaunchMode.RECENT)
			).forEach { modes ->
				Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
					modes.forEach { mode ->
						QuickStartModeButton(
							mode = mode,
							selectedMode = selectedMode,
							modifier = Modifier.weight(1f),
							focusRequester = focusRequester,
							isContentEntry = isContentEntry,
							onModeSelected = onModeSelected
						)
					}
				}
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
	isContentEntry: Boolean,
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
			.then(if (selected && isContentEntry) Modifier.contentEntryFocus() else Modifier)
			.dpadFocusIndicator()
			// The colours alone showed which mode is chosen; accessibility services read this.
			.semantics { this.selected = mode == selectedMode },
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

/**
 * The model to start, and the compatibility level of its preset. Each model reads as one
 * spec line; once the machine was edited, what differs from the preset replaces it, and
 * the open configuration is named with Reset to preset.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ModelSelectorCard(
	settings: EmulatorSettings,
	availableModels: List<AmigaModel>,
	selectedModelAvailable: Boolean,
	/** Available models whose Kickstart is not installed, which boot AROS. */
	arosModels: Set<AmigaModel>,
	/** The saved configuration the settings came from; null for Quick Start's own. */
	configName: String?,
	onModelSelected: (AmigaModel) -> Unit,
	onLevelSelected: (Int) -> Unit
) {
	val model = settings.baseModel
	val edits = remember(settings) { QuickStartUiState.presetEdits(settings) }
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
							when {
								!selectedModelAvailable -> stringResource(R.string.quick_start_no_compatible_models_help)
								edits.isNotEmpty() -> presetEditsText(edits)
								else -> QuickStartUiState.modelSpec(model)
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
								Column(
									modifier = Modifier.padding(vertical = 8.dp),
									verticalArrangement = Arrangement.spacedBy(2.dp)
								) {
									Text(availableModel.displayName, style = MaterialTheme.typography.bodyLarge)
									Text(
										QuickStartUiState.modelSpec(availableModel),
										style = MaterialTheme.typography.bodySmall,
										color = MaterialTheme.colorScheme.onSurfaceVariant
									)
									if (availableModel in arosModels) {
										val needed = QuickStartUiState.romsNeeded(availableModel)
										Text(
											if (needed.isEmpty()) {
												stringResource(R.string.quick_start_model_boots_aros)
											} else {
												stringResource(R.string.quick_start_model_needs_rom, needed.joinToString(" + "))
											},
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
			if (selectedModelAvailable && (configName != null || edits.isNotEmpty())) {
				Row(
					modifier = Modifier.fillMaxWidth().padding(top = 4.dp),
					verticalAlignment = Alignment.CenterVertically
				) {
					Text(
						configName?.let { stringResource(R.string.quick_start_model_using_config, it) }
							?: stringResource(R.string.quick_start_model_edited_title),
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant,
						modifier = Modifier.weight(1f)
					)
					if (edits.isNotEmpty()) {
						TextButton(
							onClick = { onModelSelected(model) },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Text(stringResource(R.string.action_reset_to_preset))
						}
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

/** "Edited: CPU model 68030, Z3 Fast RAM 64 MB, +2 more": what differs from the preset. */
@Composable
private fun presetEditsText(edits: List<PresetEdit>): String {
	val shown = edits.take(MAX_EDITS_SHOWN).map { "${it.label} ${settingText(it.value)}" }
	val list = shown.joinToString(", ")
	return stringResource(
		R.string.quick_start_model_edited,
		if (edits.size > shown.size) stringResource(R.string.quick_start_model_edited_more, list, edits.size - shown.size) else list
	)
}

private const val MAX_EDITS_SHOWN = 3

/**
 * The first-run card: two short steps, the second for the launch mode chosen, then Got it.
 * The Kickstart step offers the import until one is installed.
 */
@Composable
private fun WelcomeCard(
	launchMode: QuickStartLaunchMode,
	kickstartInstalled: Boolean,
	importInProgress: Boolean,
	onImportRom: () -> Unit,
	onDismiss: () -> Unit
) {
	Card(
		modifier = Modifier.fillMaxWidth(),
		colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.tertiaryContainer)
	) {
		Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
			val textColor = MaterialTheme.colorScheme.onTertiaryContainer
			Text(stringResource(R.string.welcome_title), style = MaterialTheme.typography.titleMedium, color = textColor)
			Text(
				stringResource(
					R.string.welcome_step,
					1,
					stringResource(if (kickstartInstalled) R.string.welcome_step_kickstart_done else R.string.welcome_step_kickstart)
				),
				style = MaterialTheme.typography.bodyMedium,
				color = textColor
			)
			Text(
				stringResource(
					R.string.welcome_step,
					2,
					stringResource(
						when (launchMode) {
							QuickStartLaunchMode.WHDLOAD -> R.string.welcome_step_whdload
							QuickStartLaunchMode.DISK -> R.string.welcome_step_disk
							QuickStartLaunchMode.CD -> R.string.welcome_step_cd
							QuickStartLaunchMode.RECENT -> R.string.welcome_step_recent
						}
					)
				),
				style = MaterialTheme.typography.bodyMedium,
				color = textColor
			)
			Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
				if (!kickstartInstalled) {
					Button(onClick = onImportRom, enabled = !importInProgress, modifier = Modifier.dpadFocusIndicator()) {
						Text(stringResource(if (importInProgress) R.string.action_importing else R.string.action_import_rom))
					}
				}
				Spacer(modifier = Modifier.weight(1f))
				TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.welcome_dismiss))
				}
			}
		}
	}
}

/**
 * CD mode without a CD model: the CD32 and CDTV need their own ROMs, which AROS does
 * not replace; each is named as the emulator's ROM list names it.
 */
@Composable
private fun CdRomsNeededCard(importInProgress: Boolean, onImportRom: () -> Unit) {
	val cdModels = remember { AmigaModel.entries.filter { it.hasCd } }
	Card(
		modifier = Modifier.fillMaxWidth(),
		colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.secondaryContainer)
	) {
		Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
			val textColor = MaterialTheme.colorScheme.onSecondaryContainer
			Text(stringResource(R.string.quick_start_cd_roms_title), style = MaterialTheme.typography.titleMedium, color = textColor)
			Text(stringResource(R.string.quick_start_cd_roms_message), style = MaterialTheme.typography.bodyMedium, color = textColor)
			cdModels.forEach { cdModel ->
				val needed = QuickStartUiState.romsNeeded(cdModel)
				if (needed.isNotEmpty()) {
					Text(
						stringResource(R.string.quick_start_cd_roms_model, cdModel.displayName, needed.joinToString(" + ")),
						style = MaterialTheme.typography.bodyMedium,
						color = textColor
					)
				}
			}
			Row(modifier = Modifier.fillMaxWidth()) {
				Button(onClick = onImportRom, enabled = !importInProgress, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(if (importInProgress) R.string.action_importing else R.string.action_import_rom))
				}
				Spacer(modifier = Modifier.weight(1f))
			}
		}
	}
}

/** A WHDLoad launch: WHDBooter's hardware choice is named, not previewed; the rest is the launcher's. */
@Composable
private fun WhdLoadSummaryCard(summary: QuickStartWhdLoadSummary, modifier: Modifier = Modifier) {
	val schema = settingsSchema()
	SummaryCard(
		title = stringResource(R.string.quick_start_ready_title),
		subtitle = stringResource(R.string.quick_start_ready_subtitle),
		modifier = modifier
	) {
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
