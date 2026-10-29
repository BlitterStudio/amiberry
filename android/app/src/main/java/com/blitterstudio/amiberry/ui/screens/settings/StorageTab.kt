package com.blitterstudio.amiberry.ui.screens.settings

import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Eject
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.Storage
import androidx.compose.ui.text.input.KeyboardType
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.HardDrive
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
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
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SupportingText
import com.blitterstudio.amiberry.ui.components.settings.schemaLabel
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.ImportInFlightGuard
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.launchImportGuarded
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun StorageTab(
	viewModel: SettingsViewModel,
	onImportResult: (FileManager.ImportResult) -> Unit = {}
) {
	val context = LocalContext.current
	val settings = viewModel.settings
	val roms by viewModel.availableRoms.collectAsState()
	val floppies by viewModel.availableFloppies.collectAsState()
	val cds by viewModel.availableCds.collectAsState()
	val hardDrives by viewModel.availableHardDrives.collectAsState()
	val importGuard = rememberImportInFlightGuard()
	val importInProgress = importGuard.isImporting

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// ROM selection
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(Icons.Default.Memory, contentDescription = null, modifier = Modifier.size(20.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(stringResource(R.string.settings_storage_kickstart_rom_title), style = MaterialTheme.typography.titleMedium)
				}
				Spacer(modifier = Modifier.height(8.dp))

				if (!shouldShowStorageFileDropdown(roms, settings.romFile)) {
					Text(
						stringResource(R.string.settings_storage_no_roms_found),
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.error
					)
				} else {
					FileDropdown(
						label = schemaLabel("kickstart_rom_file"),
						files = kickstartRomFiles(roms),
						selectedPath = settings.romFile,
						onSelect = { path -> viewModel.updateSettings { s -> s.withKickstartRom(path) } },
						onClear = { viewModel.updateSettings { s -> s.copy(romFile = "") } },
						emptyLabel = stringResource(R.string.settings_storage_rom_automatic),
						extraPaths = if (ModelRomAvailability.hasBuiltInAros(roms)) {
							listOf(ModelRomAvailability.AROS_ROM_FILE)
						} else {
							emptyList()
						},
						help = schemaOption("kickstart_rom_file")?.help?.takeIf { it.isNotBlank() },
						fileLabel = { romLabel(it) },
						pathLabel = { path -> romPathLabel(path, roms) }
					)
				}
			}
		}

		// Floppy drives
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(Icons.Default.SaveAlt, contentDescription = null, modifier = Modifier.size(20.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(stringResource(R.string.settings_storage_floppy_drives_title), style = MaterialTheme.typography.titleMedium)
				}
				Spacer(modifier = Modifier.height(8.dp))

				// DF0
				FloppyDriveRow(
					index = 0,
					label = stringResource(R.string.settings_storage_drive_df0),
					files = floppies,
					selectedPath = settings.floppy0,
					driveType = settings.floppy0Type,
					onSelect = { viewModel.updateSettings { s -> s.copy(floppy0 = it) } },
					onClear = { viewModel.updateSettings { s -> s.copy(floppy0 = "") } },
					onTypeChange = { viewModel.updateSettings { s -> s.copy(floppy0Type = it) } },
					context = context,
					importGuard = importGuard,
					importInProgress = importInProgress,
					onImportResult = onImportResult
				)

				Spacer(modifier = Modifier.height(8.dp))

				// DF1
				FloppyDriveRow(
					index = 1,
					label = stringResource(R.string.settings_storage_drive_df1),
					files = floppies,
					selectedPath = settings.floppy1,
					driveType = settings.floppy1Type,
					onSelect = { viewModel.updateSettings { s -> s.copy(floppy1 = it) } },
					onClear = { viewModel.updateSettings { s -> s.copy(floppy1 = "") } },
					onTypeChange = { viewModel.updateSettings { s -> s.copy(floppy1Type = it) } },
					context = context,
					importGuard = importGuard,
					importInProgress = importInProgress,
					onImportResult = onImportResult
				)

				Spacer(modifier = Modifier.height(8.dp))

				// DF2
				FloppyDriveRow(
					index = 2,
					label = stringResource(R.string.settings_storage_drive_df2),
					files = floppies,
					selectedPath = settings.floppy2,
					driveType = settings.floppy2Type,
					onSelect = { viewModel.updateSettings { s -> s.copy(floppy2 = it) } },
					onClear = { viewModel.updateSettings { s -> s.copy(floppy2 = "") } },
					onTypeChange = { viewModel.updateSettings { s -> s.copy(floppy2Type = it) } },
					context = context,
					importGuard = importGuard,
					importInProgress = importInProgress,
					onImportResult = onImportResult
				)

				Spacer(modifier = Modifier.height(8.dp))

				// DF3
				FloppyDriveRow(
					index = 3,
					label = stringResource(R.string.settings_storage_drive_df3),
					files = floppies,
					selectedPath = settings.floppy3,
					driveType = settings.floppy3Type,
					onSelect = { viewModel.updateSettings { s -> s.copy(floppy3 = it) } },
					onClear = { viewModel.updateSettings { s -> s.copy(floppy3 = "") } },
					onTypeChange = { viewModel.updateSettings { s -> s.copy(floppy3Type = it) } },
					context = context,
					importGuard = importGuard,
					importInProgress = importInProgress,
					onImportResult = onImportResult
				)
			}
		}

		// CD image
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(Icons.Default.Album, contentDescription = null, modifier = Modifier.size(20.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(stringResource(R.string.settings_storage_cd_image_title), style = MaterialTheme.typography.titleMedium)
				}
				Spacer(modifier = Modifier.height(8.dp))

				if (!shouldShowStorageFileDropdown(cds, settings.cdImage)) {
					Text(
						stringResource(R.string.settings_storage_no_cd_images_found),
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				} else {
					FileDropdown(
						label = schemaLabel("cdimage0"),
						files = cds,
						selectedPath = settings.cdImage,
						onSelect = { viewModel.updateSettings { s -> s.copy(cdImage = it) } },
						onClear = { viewModel.updateSettings { s -> s.copy(cdImage = "") } }
					)
				}
			}
		}

		// Hard drives
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(Icons.Default.Storage, contentDescription = null, modifier = Modifier.size(20.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(stringResource(R.string.settings_storage_hard_drives_title), style = MaterialTheme.typography.titleMedium)
				}
				Spacer(modifier = Modifier.height(8.dp))

				settings.hardDrives.forEachIndexed { index, drive ->
					HardDriveRow(
						index = index,
						drive = drive,
						files = hardDrives,
						onPathChange = { newPath ->
							viewModel.updateSettings { s ->
								s.copy(hardDrives = s.hardDrives.updatedAt(index) { it.copy(path = newPath) })
							}
						},
						onReadOnlyChange = { ro ->
							viewModel.updateSettings { s ->
								s.copy(hardDrives = s.hardDrives.updatedAt(index) { it.copy(readOnly = ro) })
							}
						},
						onBootPriorityChange = { bp ->
							viewModel.updateSettings { s ->
								s.copy(hardDrives = s.hardDrives.updatedAt(index) { it.copy(bootPriority = bp) })
							}
						},
						onRemove = {
							viewModel.updateSettings { s ->
								s.copy(hardDrives = s.hardDrives.removedAt(index))
							}
						},
						context = context,
						importGuard = importGuard,
						importInProgress = importInProgress,
						onImportResult = onImportResult
					)
					Spacer(modifier = Modifier.height(8.dp))
				}

				TextButton(
					onClick = {
						viewModel.updateSettings { s -> s.copy(hardDrives = s.hardDrives + HardDrive(path = "")) }
					},
					modifier = Modifier.dpadFocusIndicator()
				) {
					Icon(Icons.Default.Add, contentDescription = null, modifier = Modifier.size(18.dp))
					Spacer(modifier = Modifier.width(4.dp))
					Text(stringResource(R.string.settings_storage_add_hard_drive))
				}
			}
		}
	}
}

internal fun shouldShowStorageFileDropdown(files: List<AmigaFile>, selectedPath: String): Boolean =
	files.isNotEmpty() || selectedPath.isNotBlank()

/**
 * The installed ROMs the Kickstart list offers as files. The two AROS files
 * are left out: neither boots on its own as a Kickstart, so AROS is offered
 * once as [ModelRomAvailability.AROS_ROM_FILE] instead.
 */
internal fun kickstartRomFiles(roms: List<AmigaFile>): List<AmigaFile> =
	roms.filter { ModelRomAvailability.romIdentity(it) !is RomIdentity.BuiltInAros }

/**
 * [path] as the Kickstart. AROS brings its own extension ROM, so picking it
 * clears kickstart_ext_rom_file; another Kickstart keeps the extended ROM.
 */
internal fun EmulatorSettings.withKickstartRom(path: String): EmulatorSettings =
	if (ModelRomAvailability.isBuiltInArosRom(path)) copy(romFile = path, romExtFile = "") else copy(romFile = path)

/** A scanned ROM by what it is: its Kickstart name, or its file name. */
@Composable
private fun romLabel(rom: AmigaFile): String = when (val identity = ModelRomAvailability.romIdentity(rom)) {
	is RomIdentity.Kickstart -> identity.name
	RomIdentity.BuiltInAros, RomIdentity.Unknown -> rom.name
}

/**
 * A kickstart_rom_file value by what it boots: the built-in AROS, a scanned
 * ROM by [romLabel], or else its file name (an AROS file named directly,
 * which does not boot AROS, reads as that file).
 */
@Composable
private fun romPathLabel(path: String, roms: List<AmigaFile>): String = when {
	ModelRomAvailability.isBuiltInArosRom(path) -> stringResource(R.string.rom_aros_builtin)
	else -> roms.firstOrNull { it.path == path }?.let { romLabel(it) } ?: LauncherChoices.fileName(path)
}

/**
 * A file picked from [files], or none ([emptyLabel]). [extraPaths] are
 * values that are no scanned file (the built-in AROS), listed after the
 * empty entry and read by [pathLabel]; [help] shows under the row.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun FileDropdown(
	label: String,
	files: List<AmigaFile>,
	selectedPath: String,
	onSelect: (String) -> Unit,
	onClear: () -> Unit,
	emptyLabel: String = stringResource(R.string.placeholder_none),
	extraPaths: List<String> = emptyList(),
	help: String? = null,
	fileLabel: @Composable (AmigaFile) -> String = { it.name },
	pathLabel: @Composable (String) -> String = { LauncherChoices.fileName(it) }
) {
	var expanded by remember { mutableStateOf(false) }
	val selectedName = if (selectedPath.isEmpty()) emptyLabel else pathLabel(selectedPath)

	Column(modifier = Modifier.fillMaxWidth()) {
		Row(
			modifier = Modifier.fillMaxWidth(),
			verticalAlignment = Alignment.CenterVertically
		) {
			ExposedDropdownMenuBox(
				expanded = expanded,
				onExpandedChange = { expanded = it },
				modifier = Modifier.weight(1f)
			) {
				OutlinedTextField(
					value = selectedName,
					onValueChange = {},
					readOnly = true,
					label = { Text(label) },
					trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
					modifier = Modifier
						.dpadVerticalMovesFocus()
						.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
						.fillMaxWidth()
				)
				ExposedDropdownMenu(
					expanded = expanded,
					onDismissRequest = { expanded = false }
				) {
					DropdownMenuItem(
						text = { Text(emptyLabel) },
						onClick = {
							onClear()
							expanded = false
						}
					)
					extraPaths.forEach { path ->
						DropdownMenuItem(
							text = { Text(pathLabel(path)) },
							onClick = {
								onSelect(path)
								expanded = false
							}
						)
					}
					files.forEach { file ->
						DropdownMenuItem(
							text = { Text(fileLabel(file)) },
							onClick = {
								onSelect(file.path)
								expanded = false
							}
						)
					}
				}
			}
			if (selectedPath.isNotEmpty()) {
				IconButton(
					onClick = onClear,
					modifier = Modifier.dpadFocusIndicator()
				) {
					Icon(Icons.Default.Eject, contentDescription = stringResource(R.string.action_eject_named, selectedName))
				}
			}
		}
		if (help != null) SupportingText(help)
	}
}

@Composable
private fun FloppyDriveRow(
	index: Int,
	label: String,
	files: List<AmigaFile>,
	selectedPath: String,
	driveType: Int,
	onSelect: (String) -> Unit,
	onClear: () -> Unit,
	onTypeChange: (Int) -> Unit,
	context: android.content.Context,
	importGuard: ImportInFlightGuard,
	importInProgress: Boolean,
	onImportResult: (FileManager.ImportResult) -> Unit
) {
	val scope = rememberCoroutineScope()

	Column(modifier = Modifier.fillMaxWidth()) {
		Text(label, style = MaterialTheme.typography.labelMedium)
		Spacer(modifier = Modifier.height(4.dp))

		// Every drive type the emulator has, by the layer's labels.
		SchemaChoiceRow(
			id = "floppy${index}type",
			value = driveType.toString(),
			onSelect = { value -> value.toIntOrNull()?.let(onTypeChange) }
		)

		if (driveType >= 0) {
			Spacer(modifier = Modifier.height(8.dp))

			FileDropdown(
				label = stringResource(R.string.settings_storage_disk_label),
				files = files,
				selectedPath = selectedPath,
				onSelect = onSelect,
				onClear = onClear
			)

			val importLauncher = rememberLauncherForActivityResult(
				ActivityResultContracts.OpenDocument()
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
						if (result is FileManager.ImportResult.Imported) onSelect(result.path)
						onImportResult(result)
					}
				}
			}
			TextButton(
				onClick = { importLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.FLOPPIES)) },
				enabled = !importInProgress,
				modifier = Modifier.dpadFocusIndicator()
			) {
				if (importInProgress) {
					CircularProgressIndicator(
						modifier = Modifier.size(16.dp),
						strokeWidth = 2.dp
					)
					Spacer(modifier = Modifier.width(4.dp))
				}
				Text(
					stringResource(
						if (importInProgress) R.string.action_importing else R.string.settings_storage_import_disk_image
					)
				)
			}
		}
	}
}

private fun List<HardDrive>.updatedAt(index: Int, transform: (HardDrive) -> HardDrive): List<HardDrive> =
	mapIndexed { i, hd -> if (i == index) transform(hd) else hd }

private fun List<HardDrive>.removedAt(index: Int): List<HardDrive> =
	filterIndexed { i, _ -> i != index }

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun HardDriveRow(
	index: Int,
	drive: HardDrive,
	files: List<AmigaFile>,
	onPathChange: (String) -> Unit,
	onReadOnlyChange: (Boolean) -> Unit,
	onBootPriorityChange: (Int) -> Unit,
	onRemove: () -> Unit,
	context: android.content.Context,
	importGuard: ImportInFlightGuard,
	importInProgress: Boolean,
	onImportResult: (FileManager.ImportResult) -> Unit
) {
	val scope = rememberCoroutineScope()

	Column(modifier = Modifier.fillMaxWidth()) {
		Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
			Text(
				stringResource(R.string.settings_storage_hard_drive_label, index),
				style = MaterialTheme.typography.labelMedium,
				modifier = Modifier.weight(1f)
			)
			IconButton(
				onClick = onRemove,
				modifier = Modifier.dpadFocusIndicator()
			) {
				Icon(Icons.Default.Delete, contentDescription = stringResource(R.string.settings_storage_remove_hard_drive))
			}
		}

		FileDropdown(
			label = stringResource(R.string.settings_storage_hardfile_label),
			files = files,
			selectedPath = drive.path,
			onSelect = onPathChange,
			onClear = { onPathChange("") }
		)

		val importLauncher = rememberLauncherForActivityResult(
			ActivityResultContracts.OpenDocument()
		) { uri ->
			uri?.let {
				scope.launchImportGuarded(importGuard) {
					val result = withContext(Dispatchers.IO) {
						FileManager.importFileWithResult(context, it, FileCategory.HARD_DRIVES).also { importResult ->
							if (importResult is FileManager.ImportResult.Imported) {
								FileRepository.getInstance(context).rescanCategory(FileCategory.HARD_DRIVES)
							}
						}
					}
					if (result is FileManager.ImportResult.Imported) onPathChange(result.path)
					onImportResult(result)
				}
			}
		}
		TextButton(
			onClick = { importLauncher.launch(FilePickerFilters.mimeTypesFor(FileCategory.HARD_DRIVES)) },
			enabled = !importInProgress,
			modifier = Modifier.dpadFocusIndicator()
		) {
			if (importInProgress) {
				CircularProgressIndicator(modifier = Modifier.size(16.dp), strokeWidth = 2.dp)
				Spacer(modifier = Modifier.width(4.dp))
			}
			Text(
				stringResource(
					if (importInProgress) R.string.action_importing else R.string.settings_storage_import_hardfile
				)
			)
		}

		SettingSwitchRow(
			label = stringResource(R.string.settings_storage_hard_drive_read_only),
			checked = drive.readOnly,
			onCheckedChange = onReadOnlyChange
		)

		var bootText by remember(drive.bootPriority) { mutableStateOf(drive.bootPriority.toString()) }
		OutlinedTextField(
			value = bootText,
			onValueChange = { input ->
				bootText = input
				input.toIntOrNull()?.let { onBootPriorityChange(it) }
			},
			label = { Text(stringResource(R.string.settings_storage_boot_priority)) },
			singleLine = true,
			keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
			modifier = Modifier.fillMaxWidth()
		)
	}
}
