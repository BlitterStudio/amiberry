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
import androidx.compose.material.icons.filled.Eject
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Upload
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
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.viewmodel.QuickStartViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun QuickStartScreen(
	viewModel: QuickStartViewModel = viewModel(LocalContext.current as androidx.activity.ComponentActivity),
	settingsViewModel: SettingsViewModel = viewModel(LocalContext.current as androidx.activity.ComponentActivity)
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
			val path = FileManager.importFile(context, it, FileCategory.FLOPPIES)
			if (path != null) {
				settingsViewModel.updateSettings { s -> s.copy(floppy0 = path) }
			}
		}
	}

	val cdPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			val path = FileManager.importFile(context, it, FileCategory.CD_IMAGES)
			if (path != null) {
				settingsViewModel.updateSettings { s -> s.copy(cdImage = path) }
			}
		}
	}

	val whdloadPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
				val path = FileManager.importFile(context, it, FileCategory.WHDLOAD_GAMES)
				if (path != null) {
					viewModel.rescanWhdload()
					scope.launch {
						snackbarHostState.showSnackbar(gameImportedMessage)
					}
				}
			}
		}

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(title = { Text(stringResource(R.string.quick_start_title)) })
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
				OutlinedCard(modifier = Modifier.fillMaxWidth()) {
					Column(modifier = Modifier.padding(16.dp)) {
						Row(
							modifier = Modifier.fillMaxWidth(),
							verticalAlignment = Alignment.CenterVertically,
							horizontalArrangement = Arrangement.SpaceBetween
						) {
							Row(verticalAlignment = Alignment.CenterVertically) {
									Icon(Icons.Default.SaveAlt, contentDescription = null, modifier = Modifier.size(20.dp))
									Spacer(modifier = Modifier.width(8.dp))
									Text(stringResource(R.string.quick_start_floppy_df0), style = MaterialTheme.typography.titleMedium)
								}
								TextButton(onClick = { floppyPickerLauncher.launch(arrayOf("*/*")) }) {
									Icon(Icons.Default.Upload, contentDescription = null, modifier = Modifier.size(16.dp))
									Spacer(modifier = Modifier.width(4.dp))
									Text(stringResource(R.string.action_import))
								}
							}
						Spacer(modifier = Modifier.height(8.dp))

							if (floppies.isEmpty() && selectedFloppyPath.isBlank()) {
								Text(
									text = stringResource(R.string.quick_start_no_floppy_images),
									style = MaterialTheme.typography.bodySmall,
									color = MaterialTheme.colorScheme.onSurfaceVariant
								)
						} else {
							var floppyExpanded by remember { mutableStateOf(false) }

							Row(
								modifier = Modifier.fillMaxWidth(),
								verticalAlignment = Alignment.CenterVertically
							) {
								ExposedDropdownMenuBox(
									expanded = floppyExpanded,
									onExpandedChange = { floppyExpanded = it },
									modifier = Modifier.weight(1f)
								) {
									OutlinedTextField(
											value = selectedFloppy?.name
												?: selectedFloppyPath.takeIf { it.isNotBlank() }?.substringAfterLast('/')
												?: stringResource(R.string.placeholder_none),
										onValueChange = {},
										readOnly = true,
											trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = floppyExpanded) },
											modifier = Modifier
												.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
												.fillMaxWidth()
										)
									ExposedDropdownMenu(
										expanded = floppyExpanded,
										onDismissRequest = { floppyExpanded = false }
									) {
										floppies.forEach { file ->
											DropdownMenuItem(
												text = {
													Column {
														Text(file.name)
														Text(
															file.sizeDisplay,
															style = MaterialTheme.typography.bodySmall,
															color = MaterialTheme.colorScheme.onSurfaceVariant
														)
													}
												},
												onClick = {
													settingsViewModel.updateSettings { s -> s.copy(floppy0 = file.path) }
													floppyExpanded = false
												}
											)
										}
									}
								}

								if (selectedFloppyPath.isNotBlank()) {
										IconButton(onClick = {
											settingsViewModel.updateSettings { s -> s.copy(floppy0 = "") }
										}) {
											Icon(Icons.Default.Eject, contentDescription = stringResource(R.string.action_eject))
										}
									}
							}
						}
					}
				}
			}

			if (model.hasCd) {
				OutlinedCard(modifier = Modifier.fillMaxWidth()) {
					Column(modifier = Modifier.padding(16.dp)) {
						Row(
							modifier = Modifier.fillMaxWidth(),
							verticalAlignment = Alignment.CenterVertically,
							horizontalArrangement = Arrangement.SpaceBetween
						) {
							Row(verticalAlignment = Alignment.CenterVertically) {
									Icon(Icons.Default.Album, contentDescription = null, modifier = Modifier.size(20.dp))
									Spacer(modifier = Modifier.width(8.dp))
									Text(stringResource(R.string.quick_start_cd_image), style = MaterialTheme.typography.titleMedium)
								}
								TextButton(onClick = { cdPickerLauncher.launch(arrayOf("*/*")) }) {
									Icon(Icons.Default.Upload, contentDescription = null, modifier = Modifier.size(16.dp))
									Spacer(modifier = Modifier.width(4.dp))
									Text(stringResource(R.string.action_import))
								}
							}
						Spacer(modifier = Modifier.height(8.dp))

							if (cds.isEmpty() && selectedCdPath.isBlank()) {
								Text(
									text = stringResource(R.string.quick_start_no_cd_images),
									style = MaterialTheme.typography.bodySmall,
									color = MaterialTheme.colorScheme.onSurfaceVariant
								)
						} else {
							var cdExpanded by remember { mutableStateOf(false) }

							Row(
								modifier = Modifier.fillMaxWidth(),
								verticalAlignment = Alignment.CenterVertically
							) {
								ExposedDropdownMenuBox(
									expanded = cdExpanded,
									onExpandedChange = { cdExpanded = it },
									modifier = Modifier.weight(1f)
								) {
									OutlinedTextField(
											value = selectedCd?.name
												?: selectedCdPath.takeIf { it.isNotBlank() }?.substringAfterLast('/')
												?: stringResource(R.string.placeholder_none),
										onValueChange = {},
										readOnly = true,
											trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = cdExpanded) },
											modifier = Modifier
												.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
												.fillMaxWidth()
										)
									ExposedDropdownMenu(
										expanded = cdExpanded,
										onDismissRequest = { cdExpanded = false }
									) {
										cds.forEach { file ->
											DropdownMenuItem(
												text = {
													Column {
														Text(file.name)
														Text(
															file.sizeDisplay,
															style = MaterialTheme.typography.bodySmall,
															color = MaterialTheme.colorScheme.onSurfaceVariant
														)
													}
												},
												onClick = {
													settingsViewModel.updateSettings { s -> s.copy(cdImage = file.path) }
													cdExpanded = false
												}
											)
										}
									}
								}

								if (selectedCdPath.isNotBlank()) {
										IconButton(onClick = {
											settingsViewModel.updateSettings { s -> s.copy(cdImage = "") }
										}) {
											Icon(Icons.Default.Eject, contentDescription = stringResource(R.string.action_eject))
										}
									}
							}
						}
					}
				}
			}

			HorizontalDivider()

			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					Row(
						modifier = Modifier.fillMaxWidth(),
						verticalAlignment = Alignment.CenterVertically,
						horizontalArrangement = Arrangement.SpaceBetween
					) {
						Row(verticalAlignment = Alignment.CenterVertically) {
								Icon(
									Icons.Default.SportsEsports,
								contentDescription = null,
								modifier = Modifier.size(20.dp),
								tint = MaterialTheme.colorScheme.primary
							)
								Spacer(modifier = Modifier.width(8.dp))
								Text(stringResource(R.string.quick_start_whdload_title), style = MaterialTheme.typography.titleMedium)
							}
							TextButton(onClick = { whdloadPickerLauncher.launch(arrayOf("*/*")) }) {
								Icon(Icons.Default.Upload, contentDescription = null, modifier = Modifier.size(16.dp))
								Spacer(modifier = Modifier.width(4.dp))
								Text(stringResource(R.string.action_import))
							}
						}

						Text(
							text = stringResource(R.string.quick_start_whdload_help),
							style = MaterialTheme.typography.bodySmall,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)

					Spacer(modifier = Modifier.height(8.dp))

						if (whdloadGames.isEmpty()) {
							Text(
								text = stringResource(R.string.quick_start_no_whdload_games),
								style = MaterialTheme.typography.bodySmall,
								color = MaterialTheme.colorScheme.onSurfaceVariant
							)
					} else {
						var whdExpanded by remember { mutableStateOf(false) }

						Row(
							modifier = Modifier.fillMaxWidth(),
							verticalAlignment = Alignment.CenterVertically
						) {
							ExposedDropdownMenuBox(
								expanded = whdExpanded,
								onExpandedChange = { whdExpanded = it },
								modifier = Modifier.weight(1f)
							) {
									OutlinedTextField(
										value = selectedWhdloadGame?.name?.removeSuffix(".lha")?.removeSuffix(".lzx")?.removeSuffix(".lzh")
											?: stringResource(R.string.quick_start_select_game_placeholder),
									onValueChange = {},
									readOnly = true,
										trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = whdExpanded) },
										modifier = Modifier
											.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
											.fillMaxWidth()
									)
								ExposedDropdownMenu(
									expanded = whdExpanded,
									onDismissRequest = { whdExpanded = false }
								) {
									whdloadGames.forEach { game ->
										DropdownMenuItem(
											text = {
												Column {
													Text(game.name.removeSuffix(".lha").removeSuffix(".lzx").removeSuffix(".lzh"))
													Text(
														game.sizeDisplay,
														style = MaterialTheme.typography.bodySmall,
														color = MaterialTheme.colorScheme.onSurfaceVariant
													)
												}
											},
											onClick = {
												viewModel.selectWhdload(game)
												whdExpanded = false
											}
										)
									}
								}
							}

							if (selectedWhdloadGame != null) {
								IconButton(onClick = { viewModel.selectWhdload(null) }) {
									Icon(Icons.Default.Eject, contentDescription = stringResource(R.string.action_eject))
								}
							}
						}
					}
				}
			}

			Spacer(modifier = Modifier.height(80.dp))
		}
	}
}
