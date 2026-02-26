package com.blitterstudio.amiberry.ui.screens

import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
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
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.RocketLaunch
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Upload
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MenuAnchorType
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.viewmodel.QuickStartViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun QuickStartScreen(viewModel: QuickStartViewModel = viewModel()) {
	val context = LocalContext.current
	val model = viewModel.selectedModel
	val floppies by viewModel.availableFloppies.collectAsState()
	val cds by viewModel.availableCds.collectAsState()
	val roms by viewModel.availableRoms.collectAsState()
	val whdloadGames by viewModel.availableWhdloadGames.collectAsState()
	val hasRoms = roms.isNotEmpty()

	// SAF picker for importing floppy
	val floppyPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			val path = FileManager.importFile(context, it, FileCategory.FLOPPIES)
			if (path != null) {
				viewModel.selectFloppy(
					AmigaFile(path, path.substringAfterLast('/'), "adf", 0, 0, FileCategory.FLOPPIES)
				)
			}
		}
	}

	// SAF picker for importing CD
	val cdPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			val path = FileManager.importFile(context, it, FileCategory.CD_IMAGES)
			if (path != null) {
				viewModel.selectCd(
					AmigaFile(path, path.substringAfterLast('/'), "iso", 0, 0, FileCategory.CD_IMAGES)
				)
			}
		}
	}

	// SAF picker for importing WHDLoad games
	val whdloadPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			val path = FileManager.importFile(context, it, FileCategory.WHDLOAD_GAMES)
			if (path != null) {
				viewModel.rescanWhdload()
				Toast.makeText(context, "Game imported", Toast.LENGTH_SHORT).show()
			}
		}
	}

	Column(
		modifier = Modifier
			.fillMaxSize()
			.verticalScroll(rememberScrollState())
			.padding(16.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		Text(
			text = "Quick Start",
			style = MaterialTheme.typography.headlineMedium
		)

		// ROM status
		Card(
			modifier = Modifier.fillMaxWidth(),
			colors = CardDefaults.cardColors(
				containerColor = if (hasRoms)
					MaterialTheme.colorScheme.primaryContainer
				else
					MaterialTheme.colorScheme.errorContainer
			)
		) {
			Row(
				modifier = Modifier.padding(12.dp),
				verticalAlignment = Alignment.CenterVertically
			) {
				Icon(
					if (hasRoms) Icons.Default.CheckCircle else Icons.Default.Warning,
					contentDescription = null,
					modifier = Modifier.size(24.dp)
				)
				Spacer(modifier = Modifier.width(8.dp))
				Text(
					text = if (hasRoms)
						"${roms.size} ROM(s) available"
					else
						"No Kickstart ROMs found - import via Files tab",
					style = MaterialTheme.typography.bodyMedium
				)
			}
		}

		// WHDLoad Games section
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
						Text("WHDLoad Game", style = MaterialTheme.typography.titleMedium)
					}
					TextButton(onClick = { whdloadPickerLauncher.launch(arrayOf("*/*")) }) {
						Icon(Icons.Default.Upload, contentDescription = null, modifier = Modifier.size(16.dp))
						Spacer(modifier = Modifier.width(4.dp))
						Text("Import")
					}
				}

				Text(
					text = "Select an .lha game to auto-configure and launch",
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)

				Spacer(modifier = Modifier.height(8.dp))

				if (whdloadGames.isEmpty()) {
					Text(
						text = "No WHDLoad games found. Import .lha files to get started.",
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				} else {
					// WHDLoad game selector dropdown
					var whdExpanded by remember { mutableStateOf(false) }
					val selectedWhdload = viewModel.selectedWhdload

					ExposedDropdownMenuBox(
						expanded = whdExpanded,
						onExpandedChange = { whdExpanded = it }
					) {
						OutlinedTextField(
							value = selectedWhdload?.name?.removeSuffix(".lha")
								?.removeSuffix(".lzx")?.removeSuffix(".lzh") ?: "(select a game)",
							onValueChange = {},
							readOnly = true,
							trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = whdExpanded) },
							modifier = Modifier
								.menuAnchor(MenuAnchorType.PrimaryNotEditable)
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
											Text(game.name.removeSuffix(".lha")
												.removeSuffix(".lzx").removeSuffix(".lzh"))
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

					if (selectedWhdload != null) {
						Spacer(modifier = Modifier.height(8.dp))
						Button(
							onClick = {
								EmulatorLauncher.launchWhdload(context, selectedWhdload.path)
							},
							modifier = Modifier.fillMaxWidth()
						) {
							Icon(Icons.Default.RocketLaunch, contentDescription = null, modifier = Modifier.size(20.dp))
							Spacer(modifier = Modifier.width(8.dp))
							Text("Launch WHDLoad Game")
						}
					}
				}
			}
		}

		HorizontalDivider()

		// Model selector
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("Amiga Model", style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				var modelExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = modelExpanded,
					onExpandedChange = { modelExpanded = it }
				) {
					OutlinedTextField(
						value = model.displayName,
						onValueChange = {},
						readOnly = true,
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = modelExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth(),
						supportingText = { Text(model.description) }
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
									viewModel.selectModel(m)
									modelExpanded = false
								}
							)
						}
					}
				}
			}
		}

		// Floppy selector
		if (model.hasFloppy) {
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					Row(verticalAlignment = Alignment.CenterVertically) {
						Icon(Icons.Default.SaveAlt, contentDescription = null, modifier = Modifier.size(20.dp))
						Spacer(modifier = Modifier.width(8.dp))
						Text("Floppy Disk (DF0)", style = MaterialTheme.typography.titleMedium)
					}
					Spacer(modifier = Modifier.height(8.dp))

					if (floppies.isEmpty()) {
						Text(
							text = "No floppy images found",
							style = MaterialTheme.typography.bodySmall,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)
					} else {
						var floppyExpanded by remember { mutableStateOf(false) }
						ExposedDropdownMenuBox(
							expanded = floppyExpanded,
							onExpandedChange = { floppyExpanded = it }
						) {
							OutlinedTextField(
								value = viewModel.selectedFloppy?.name ?: "(none)",
								onValueChange = {},
								readOnly = true,
								trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = floppyExpanded) },
								modifier = Modifier
									.menuAnchor(MenuAnchorType.PrimaryNotEditable)
									.fillMaxWidth()
							)
							ExposedDropdownMenu(
								expanded = floppyExpanded,
								onDismissRequest = { floppyExpanded = false }
							) {
								DropdownMenuItem(
									text = { Text("(none)") },
									onClick = {
										viewModel.selectFloppy(null)
										floppyExpanded = false
									}
								)
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
											viewModel.selectFloppy(file)
											floppyExpanded = false
										}
									)
								}
							}
						}
					}

					Spacer(modifier = Modifier.height(8.dp))
					TextButton(onClick = { floppyPickerLauncher.launch(arrayOf("*/*")) }) {
						Text("Import floppy image...")
					}
				}
			}
		}

		// CD selector
		if (model.hasCd) {
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					Row(verticalAlignment = Alignment.CenterVertically) {
						Icon(Icons.Default.Album, contentDescription = null, modifier = Modifier.size(20.dp))
						Spacer(modifier = Modifier.width(8.dp))
						Text("CD Image", style = MaterialTheme.typography.titleMedium)
					}
					Spacer(modifier = Modifier.height(8.dp))

					if (cds.isEmpty()) {
						Text(
							text = "No CD images found",
							style = MaterialTheme.typography.bodySmall,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)
					} else {
						var cdExpanded by remember { mutableStateOf(false) }
						ExposedDropdownMenuBox(
							expanded = cdExpanded,
							onExpandedChange = { cdExpanded = it }
						) {
							OutlinedTextField(
								value = viewModel.selectedCd?.name ?: "(none)",
								onValueChange = {},
								readOnly = true,
								trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = cdExpanded) },
								modifier = Modifier
									.menuAnchor(MenuAnchorType.PrimaryNotEditable)
									.fillMaxWidth()
							)
							ExposedDropdownMenu(
								expanded = cdExpanded,
								onDismissRequest = { cdExpanded = false }
							) {
								DropdownMenuItem(
									text = { Text("(none)") },
									onClick = {
										viewModel.selectCd(null)
										cdExpanded = false
									}
								)
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
											viewModel.selectCd(file)
											cdExpanded = false
										}
									)
								}
							}
						}
					}

					Spacer(modifier = Modifier.height(8.dp))
					TextButton(onClick = { cdPickerLauncher.launch(arrayOf("*/*")) }) {
						Text("Import CD image...")
					}
				}
			}
		}

		Spacer(modifier = Modifier.height(8.dp))

		// Action buttons
		Button(
			onClick = {
				EmulatorLauncher.launchQuickStart(
					context = context,
					model = model,
					floppyPath = viewModel.selectedFloppy?.path,
					cdPath = viewModel.selectedCd?.path
				)
			},
			modifier = Modifier.fillMaxWidth()
		) {
			Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(20.dp))
			Spacer(modifier = Modifier.width(8.dp))
			Text("Start Emulation")
		}

		OutlinedButton(
			onClick = { EmulatorLauncher.launchAdvancedGui(context) },
			modifier = Modifier.fillMaxWidth()
		) {
			Icon(Icons.Default.Settings, contentDescription = null, modifier = Modifier.size(20.dp))
			Spacer(modifier = Modifier.width(8.dp))
			Text("Advanced Settings (ImGui)")
		}
	}
}
