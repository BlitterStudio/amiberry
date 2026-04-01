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
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
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
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.findActivity
import com.blitterstudio.amiberry.ui.viewmodel.QuickStartViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.ui.components.MediaSelector
import com.blitterstudio.amiberry.ui.components.StoragePermissionBanner
import com.blitterstudio.amiberry.ui.navigation.Screen
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

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
	val model = settingsViewModel.settings.baseModel
	val floppies by viewModel.availableFloppies.collectAsState()
	val cds by viewModel.availableCds.collectAsState()
	val roms by viewModel.availableRoms.collectAsState()
	val whdloadGames by viewModel.availableWhdloadGames.collectAsState()
	val selectedWhdloadGame = viewModel.selectedWhdload
	val isScanning by FileRepository.getInstance(context).isScanning.collectAsState()
	val hasRoms = roms.isNotEmpty()
	val canStart = settingsViewModel.settings.romFile.isNotBlank() || hasRoms

	val selectedFloppyPath = settingsViewModel.settings.floppy0
	val selectedCdPath = settingsViewModel.settings.cdImage
	val selectedFloppy = floppies.firstOrNull { it.path == selectedFloppyPath }
	val selectedCd = cds.firstOrNull { it.path == selectedCdPath }
	val mediaSummaryWhdload = stringResource(R.string.quick_start_media_whdload)
	val mediaSummaryBoth = stringResource(R.string.quick_start_media_floppy_cd)
	val mediaSummaryFloppy = stringResource(R.string.quick_start_media_floppy)
	val mediaSummaryCd = stringResource(R.string.quick_start_media_cd)
	val mediaSummaryNone = stringResource(R.string.quick_start_media_none)
	val gameImportedMessage = stringResource(R.string.msg_game_imported)
	val romRequiredMessage = stringResource(R.string.msg_rom_required_before_start)
	val mediaSummary = when {
		selectedWhdloadGame != null -> mediaSummaryWhdload
		selectedFloppyPath.isNotBlank() && selectedCdPath.isNotBlank() -> mediaSummaryBoth
		selectedFloppyPath.isNotBlank() -> mediaSummaryFloppy
		selectedCdPath.isNotBlank() -> mediaSummaryCd
		else -> mediaSummaryNone
	}

	val floppyPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			scope.launch {
				val path = withContext(Dispatchers.IO) {
					FileManager.importFile(context, it, FileCategory.FLOPPIES)
				}
				if (path != null) {
					settingsViewModel.updateSettings { s -> s.copy(floppy0 = path) }
				}
			}
		}
	}

	val cdPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			scope.launch {
				val path = withContext(Dispatchers.IO) {
					FileManager.importFile(context, it, FileCategory.CD_IMAGES)
				}
				if (path != null) {
					settingsViewModel.updateSettings { s -> s.copy(cdImage = path) }
				}
			}
		}
	}

	val whdloadPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			scope.launch {
				val path = withContext(Dispatchers.IO) {
					FileManager.importFile(context, it, FileCategory.WHDLOAD_GAMES)
				}
				if (path != null) {
					viewModel.rescanWhdload()
					snackbarHostState.showSnackbar(gameImportedMessage)
				}
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
			ExtendedFloatingActionButton(
				onClick = {
					if (!canStart) {
						scope.launch {
							snackbarHostState.showSnackbar(romRequiredMessage)
						}
						return@ExtendedFloatingActionButton
					}
					if (selectedWhdloadGame != null) {
						EmulatorLauncher.launchWhdload(context, selectedWhdloadGame.path)
					} else {
						EmulatorLauncher.launchQuickStart(
							context = context,
							model = settingsViewModel.settings.baseModel,
							floppyPath = settingsViewModel.settings.floppy0.takeIf { it.isNotBlank() },
							floppy1Path = settingsViewModel.settings.floppy1.takeIf { it.isNotBlank() },
							cdPath = settingsViewModel.settings.cdImage.takeIf { it.isNotBlank() }
						)
					}
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
				.verticalScroll(rememberScrollState())
				.padding(16.dp),
			verticalArrangement = Arrangement.spacedBy(16.dp)
		) {
			if (isScanning) {
				LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
			}

			StoragePermissionBanner()

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

			// Recent launches (filtered to existing files, recomputed on every resume)
			val recentLaunches = AppPreferences.getInstance(context).getRecentLaunches().filter { entry ->
				val parts = entry.split(":", limit = 3)
				when (parts.getOrNull(0)) {
					"config", "whdload" -> {
						val path = parts.getOrNull(1)
						path != null && java.io.File(path).exists()
					}
					"quickstart" -> true // Model-based launches are always valid
					else -> false
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
							val parts = entry.split(":", limit = 3)
							val label = when (parts.getOrNull(0)) {
								"quickstart" -> {
									val modelName = parts.getOrNull(1) ?: "?"
									val mediaParts = parts.getOrNull(2)?.split(";") ?: emptyList()
									val df0 = mediaParts.getOrNull(0)?.substringAfterLast('/')?.takeIf { it.isNotBlank() }
									val df1 = mediaParts.getOrNull(1)?.substringAfterLast('/')?.takeIf { it.isNotBlank() }
									val media = listOfNotNull(df0, df1).joinToString(", ").takeIf { it.isNotBlank() }
									if (media != null) "$modelName — $media" else modelName
								}
								"config" -> parts.getOrNull(1)?.substringAfterLast('/')?.removeSuffix(".uae") ?: "Config"
								"whdload" -> parts.getOrNull(1)?.substringAfterLast('/')?.removeSuffix(".lha")?.removeSuffix(".lzx") ?: "WHDLoad"
								else -> entry.take(40)
							}
							TextButton(
								onClick = {
									when (parts.getOrNull(0)) {
										"config" -> parts.getOrNull(1)?.let {
											EmulatorLauncher.launchWithConfig(context, it)
										}
										"whdload" -> parts.getOrNull(1)?.let {
											EmulatorLauncher.launchWhdload(context, it)
										}
										"quickstart" -> {
											val model = AmigaModel.entries.firstOrNull { it.cmdArg == parts.getOrNull(1) } ?: AmigaModel.A500
											val mediaParts = parts.getOrNull(2)?.split(";") ?: emptyList()
											val df0 = mediaParts.getOrNull(0)?.takeIf { it.isNotBlank() }
											val df1 = mediaParts.getOrNull(1)?.takeIf { it.isNotBlank() }
											val cd = mediaParts.getOrNull(2)?.takeIf { it.isNotBlank() }
											EmulatorLauncher.launchQuickStart(context, model,
												floppyPath = if (model.hasFloppy) df0 else null,
												floppy1Path = if (model.hasFloppy) df1 else null,
												cdPath = if (model.hasCd) cd else null)
										}
									}
								},
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
							text = stringResource(R.string.quick_start_model_summary, model.displayName),
							style = MaterialTheme.typography.bodyMedium
						)
						Text(mediaSummary, style = MaterialTheme.typography.bodyMedium)
						if (!hasRoms) {
						Spacer(modifier = Modifier.height(10.dp))
						Row(verticalAlignment = Alignment.CenterVertically) {
							Icon(
								Icons.Default.Warning,
								contentDescription = null,
								modifier = Modifier.size(20.dp),
								tint = MaterialTheme.colorScheme.error
							)
								Spacer(modifier = Modifier.width(8.dp))
								Text(
									text = stringResource(R.string.quick_start_no_rom_warning),
									style = MaterialTheme.typography.bodySmall,
									color = MaterialTheme.colorScheme.error
								)
						}
					}
				}
			}

				OutlinedCard(modifier = Modifier.fillMaxWidth()) {
					Column(modifier = Modifier.padding(16.dp)) {
						Text(stringResource(R.string.quick_start_amiga_model), style = MaterialTheme.typography.titleMedium)
						Spacer(modifier = Modifier.height(8.dp))

					var modelExpanded by remember { mutableStateOf(false) }
					ExposedDropdownMenuBox(
						expanded = modelExpanded,
						onExpandedChange = { modelExpanded = it }
					) {
						OutlinedTextField(
							value = settingsViewModel.settings.baseModel.displayName,
							onValueChange = {},
							readOnly = true,
								trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = modelExpanded) },
								modifier = Modifier
									.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
									.fillMaxWidth(),
								supportingText = { Text(settingsViewModel.settings.baseModel.description) }
							)
						ExposedDropdownMenu(
							expanded = modelExpanded,
							onDismissRequest = { modelExpanded = false }
						) {
							AmigaModel.entries.forEach { m ->
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

			if (model.hasFloppy) {
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
					onImport = { floppyPickerLauncher.launch(arrayOf("*/*")) }
				)

				val selectedFloppy1Path = settingsViewModel.settings.floppy1
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
					onImport = { floppyPickerLauncher.launch(arrayOf("*/*")) }
				)
			}

			if (model.hasCd) {
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
					onImport = { cdPickerLauncher.launch(arrayOf("*/*")) }
				)
			}

			HorizontalDivider()

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
				onImport = { whdloadPickerLauncher.launch(arrayOf("*/*")) },
				displayName = { it.name.removeSuffix(".lha").removeSuffix(".lzx").removeSuffix(".lzh") }
			)

			Spacer(modifier = Modifier.height(80.dp))
		}
	}
}
