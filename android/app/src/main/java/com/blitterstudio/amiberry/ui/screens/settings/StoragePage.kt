package com.blitterstudio.amiberry.ui.screens.settings

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
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Eject
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.Storage
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.MountDefaults
import com.blitterstudio.amiberry.data.model.MountEntry
import com.blitterstudio.amiberry.data.model.MountKind
import com.blitterstudio.amiberry.data.model.MountNames
import com.blitterstudio.amiberry.data.model.SettingsSchema
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Switch
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
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.ContentFolders
import com.blitterstudio.amiberry.data.HardfileCreator
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FilePickerFilters
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.ui.components.rememberImportPicker
import com.blitterstudio.amiberry.ui.components.PickedFolder
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SupportingText
import com.blitterstudio.amiberry.ui.components.settings.schemaLabel
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.ImportInFlightGuard
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadArrowsMoveFocus
import com.blitterstudio.amiberry.ui.dpadFieldFocusIndicator
import com.blitterstudio.amiberry.ui.components.settings.dpadMenuItemInitialFocus
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate
import com.blitterstudio.amiberry.ui.launchImportGuarded
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun StoragePage(
	viewModel: SettingsViewModel,
	onImportResult: (FileManager.ImportResult) -> Unit = {}
) {
	val context = LocalContext.current
	val mountSchema = settingsSchema()
	val settings = viewModel.settings
	val floppies by viewModel.availableFloppies.collectAsState()
	val cds by viewModel.availableCds.collectAsState()
	val hardDrives by viewModel.availableHardDrives.collectAsState()
	val importGuard = rememberImportInFlightGuard()
	val importInProgress = importGuard.isImporting
	val mountScope = rememberCoroutineScope()
	var creatingHardfile by remember { mutableStateOf(false) }
	var hardfileError by remember { mutableStateOf<Int?>(null) }
	var pendingFolderMount by remember { mutableStateOf<MountKind?>(null) }
	val mountFolderPicker = rememberImportPicker(
		mimeTypes = arrayOf("*/*"),
		extensions = emptySet(),
		// The folder is mounted where it is, not copied into the library.
		folderLabelRes = R.string.settings_storage_mount_folder,
		onFolder = { picked ->
			val kind = pendingFolderMount
			pendingFolderMount = null
			val path = (picked as? PickedFolder.Local)?.dir?.path ?: return@rememberImportPicker
			if (kind != null) viewModel.updateSettings { current -> MountDefaults.added(current, kind, path, mountSchema) }
		},
		onFiles = {}
	)
	val archivePicker = rememberImportPicker(
		mimeTypes = arrayOf("application/zip", "application/x-7z-compressed", "application/x-rar-compressed", "application/octet-stream"),
		extensions = setOf("zip", "7z", "rar", "lha", "lzh", "lzx")
	) { uris ->
		uris.firstOrNull()?.let { uri ->
			mountScope.launchImportGuarded(importGuard) {
				val result = withContext(Dispatchers.IO) {
					FileManager.importMountArchiveWithResult(context, uri).also {
						if (it is FileManager.ImportResult.Imported) FileRepository.getInstance(context).rescanCategory(FileCategory.HARD_DRIVES)
					}
				}
				if (result is FileManager.ImportResult.Imported) {
					viewModel.updateSettings { current -> MountDefaults.added(current, MountKind.Directory, result.path, mountSchema) }
				}
				onImportResult(result)
			}
		}
	}
	val tapeImagePicker = rememberImportPicker(
		mimeTypes = arrayOf("*/*"),
		extensions = emptySet()
	) { uris ->
		uris.firstOrNull()?.let { uri ->
			mountScope.launchImportGuarded(importGuard) {
				val result = withContext(Dispatchers.IO) {
					FileManager.importTapeImageWithResult(context, uri).also {
						if (it is FileManager.ImportResult.Imported) FileRepository.getInstance(context).rescanCategory(FileCategory.HARD_DRIVES)
					}
				}
				if (result is FileManager.ImportResult.Imported) {
					viewModel.updateSettings { current -> MountDefaults.added(current, MountKind.Tape, result.path, mountSchema) }
				}
				onImportResult(result)
			}
		}
	}
	if (creatingHardfile) {
		HardfileCreateDialog(
			error = hardfileError,
			onDismiss = {
				creatingHardfile = false
				hardfileError = null
			},
			onCreate = { name, sizeMb, dynamic ->
				mountScope.launchImportGuarded(importGuard) {
					val created = runCatching {
						withContext(Dispatchers.IO) {
							val targetName = if (dynamic && name.endsWith(".hdf", ignoreCase = true)) name.dropLast(4) + ".vhd" else name
							HardfileCreator.create(
								ContentFolders.categoryDir(context, FileCategory.HARD_DRIVES).resolve(targetName),
								sizeMb.toLong() * 1024 * 1024,
								dynamic
							).path
						}
					}.getOrElse {
						hardfileError = R.string.settings_storage_create_failed
						return@launchImportGuarded
					}
					creatingHardfile = false
					hardfileError = null
					FileRepository.getInstance(context).rescanCategory(FileCategory.HARD_DRIVES)
					viewModel.updateSettings { current -> MountDefaults.added(current, MountKind.Hardfile, created, mountSchema) }
				}
			}
		)
	}

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Floppy drives
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(Icons.Default.SaveAlt, contentDescription = null, modifier = Modifier.size(20.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(stringResource(R.string.settings_storage_floppy_drives_title), style = MaterialTheme.typography.titleMedium)
				}
				Spacer(modifier = Modifier.height(8.dp))

				for (index in 0..3) {
					FloppyDriveRow(
						index = index,
						label = stringResource(driveLabelRes(index)),
						files = floppies,
						selectedPath = settings.drivePath(index),
						driveType = settings.driveType(index),
						onSelect = { path -> viewModel.updateSettings { s -> s.withDrivePath(index, path) } },
						onClear = { viewModel.updateSettings { s -> s.withDrivePath(index, "") } },
						onTypeChange = { type -> viewModel.updateSettings { s -> s.withDriveType(index, type) } },
						context = context,
						importGuard = importGuard,
						importInProgress = importInProgress,
						onImportResult = onImportResult
					)
					Spacer(modifier = Modifier.height(8.dp))
				}
				SchemaGroupInlineRows("floppy", viewModel)
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
				Spacer(modifier = Modifier.height(8.dp))
				SchemaGroupInlineRows("cd", viewModel)
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

				// Adding a drive moves D-pad focus to its HDF field; removing one takes its
				// focused Delete button away, so focus goes to Add hard drive instead of the
				// navigation rail. Requested once the list has changed size.
				val newDriveFocus = remember { FocusRequester() }
				val addDriveFocus = remember { FocusRequester() }
				var pendingFocus by remember { mutableStateOf<Pair<FocusRequester, Int>?>(null) }
				LaunchedEffect(settings.hardDrives.size, pendingFocus) {
					val (target, sizeBefore) = pendingFocus ?: return@LaunchedEffect
					if (settings.hardDrives.size == sizeBefore) return@LaunchedEffect
					pendingFocus = null
					target.requestFocus()
				}

				settings.hardDrives.forEachIndexed { index, drive ->
					MountRow(
						settings = settings,
						schema = mountSchema,
						index = index,
						drive = drive,
						files = hardDrives,
						onPathChange = { newPath ->
							viewModel.updateSettings { s ->
								s.copy(hardDrives = s.hardDrives.updatedAt(index) { MountDefaults.withPath(it, newPath) })
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
						onMountChange = { updated ->
							viewModel.updateSettings { s -> s.copy(hardDrives = s.hardDrives.updatedAt(index) { updated }) }
						},
						onRemove = {
							pendingFocus = addDriveFocus to settings.hardDrives.size
							viewModel.updateSettings { s ->
								s.copy(hardDrives = s.hardDrives.removedAt(index))
							}
						},
						fieldFocus = if (index == settings.hardDrives.lastIndex) newDriveFocus else null,
						context = context,
						importGuard = importGuard,
						importInProgress = importInProgress,
						onImportResult = onImportResult
					)
					Spacer(modifier = Modifier.height(8.dp))
				}

				Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
					TextButton(
						onClick = {
							if (!MountDefaults.canAdd(MountKind.Hardfile, settings, mountSchema)) return@TextButton
							pendingFocus = newDriveFocus to settings.hardDrives.size
							viewModel.updateSettings { s -> MountDefaults.added(s, MountKind.Hardfile, "", mountSchema) }
						},
						enabled = MountDefaults.canAdd(MountKind.Hardfile, settings, mountSchema),
						modifier = Modifier.fillMaxWidth().focusRequester(addDriveFocus).dpadFocusIndicator()
					) {
						Icon(Icons.Default.Add, contentDescription = null, modifier = Modifier.size(18.dp))
						Spacer(modifier = Modifier.width(4.dp))
						Text(stringResource(R.string.settings_storage_add_hard_drive))
						Spacer(modifier = Modifier.weight(1f))
					}
					TextButton(
						onClick = {
							pendingFolderMount = MountKind.Directory
							mountFolderPicker.browseFolders()
						},
						enabled = MountDefaults.canAdd(MountKind.Directory, settings, mountSchema),
						modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
					) {
						Text(stringResource(R.string.settings_storage_add_directory))
						Spacer(modifier = Modifier.weight(1f))
					}
					TextButton(
						onClick = archivePicker::pickFiles,
						enabled = MountDefaults.canAdd(MountKind.Directory, settings, mountSchema) && !importInProgress,
						modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
					) {
						Text(stringResource(R.string.settings_storage_add_archive))
						Spacer(modifier = Modifier.weight(1f))
					}
					TextButton(
						onClick = {
							if (!MountDefaults.canAdd(MountKind.Cd, settings, mountSchema)) return@TextButton
							pendingFocus = newDriveFocus to settings.hardDrives.size
							viewModel.updateSettings { current -> MountDefaults.added(current, MountKind.Cd, "", mountSchema) }
						},
						enabled = MountDefaults.canAdd(MountKind.Cd, settings, mountSchema),
						modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
					) {
						Text(stringResource(R.string.settings_storage_add_cd_drive))
						Spacer(modifier = Modifier.weight(1f))
					}
					TextButton(
						onClick = {
							pendingFolderMount = MountKind.Tape
							mountFolderPicker.browseFolders()
						},
						enabled = MountDefaults.canAdd(MountKind.Tape, settings, mountSchema),
						modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
					) {
						Text(stringResource(R.string.settings_storage_add_tape_drive))
						Spacer(modifier = Modifier.weight(1f))
					}
					TextButton(
						onClick = tapeImagePicker::pickFiles,
						enabled = MountDefaults.canAdd(MountKind.Tape, settings, mountSchema) && !importInProgress,
						modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
					) {
						Text(stringResource(R.string.settings_storage_add_tape_image))
						Spacer(modifier = Modifier.weight(1f))
					}
					TextButton(
						onClick = { creatingHardfile = true },
						enabled = MountDefaults.canAdd(MountKind.Hardfile, settings, mountSchema),
						modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
					) {
						Text(stringResource(R.string.settings_storage_create_hardfile))
						Spacer(modifier = Modifier.weight(1f))
					}
				}
			}
		}

		DiskSwapperSection(viewModel, onImportResult)

		SettingsGroups.placementOf(SettingsPage.Storage, "filesystem")?.let { SchemaGroupSection(it, viewModel) }
	}
}

/**
 * The disk swapper: the twenty entries a configuration keeps (diskimage0
 * to diskimage19), as the Apple editor's growing list — the filled ones
 * and one empty one to fill next, never fewer while it shows (clearing an
 * entry to retype it must not take its row away mid-edit). The entries
 * reach the in-game menu's swapper during emulation.
 */
@Composable
internal fun DiskSwapperSection(
	viewModel: SettingsViewModel,
	onImportResult: (FileManager.ImportResult) -> Unit
) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val floppies by viewModel.availableFloppies.collectAsState()
	val ids = (0 until DISK_SWAPPER_SLOTS).map { "diskimage$it" }
	val filled = ids.indexOfLast { !(settings.optionValue(it) ?: "").isEmpty() }
	val shown = ids.take(minOf(filled + 2, ids.count()))

	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Text(
				stringResource(R.string.settings_storage_diskswapper_title),
				style = MaterialTheme.typography.titleMedium
			)
			Spacer(modifier = Modifier.height(8.dp))
			shown.forEach { id ->
				val index = id.removePrefix("diskimage").toIntOrNull() ?: return@forEach
				FileDropdown(
					label = stringResource(R.string.overlay_swapper_slot, index + 1),
					files = floppies,
					selectedPath = settings.optionValue(id) ?: "",
					onSelect = { path -> viewModel.updateSettings { it.withOption(id, path) } },
					onClear = { viewModel.updateSettings { it.withOption(id, "") } }
				)
				Spacer(modifier = Modifier.height(8.dp))
			}
			Text(
				stringResource(R.string.settings_storage_diskswapper_footer),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
		}
	}
}

/** The disk swapper entries a configuration keeps (AMIBERRY_ACTIONS_SWAPPER_ENTRIES). */
internal const val DISK_SWAPPER_SLOTS = 20

/**
 * The Kickstart picker: the scanned ROMs by what they are, with the
 * built-in AROS and an empty (the model's own) entry. The Overview leads
 * with it, and the Hardware page's ROMs section re-shows it (the Apple
 * Hardware page's first ROM row).
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
internal fun KickstartRomRow(viewModel: SettingsViewModel) {
	val roms by viewModel.availableRoms.collectAsState()
	val settings = viewModel.settings
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
				listOf(ModelRomAvailability.arosRomFile)
			} else {
				emptyList()
			},
			help = schemaOption("kickstart_rom_file")?.help?.takeIf { it.isNotBlank() },
			fileLabel = { romLabel(it) },
			pathLabel = { path -> romPathLabel(path, roms) }
		)
	}
}

internal fun shouldShowStorageFileDropdown(files: List<AmigaFile>, selectedPath: String): Boolean =
	files.isNotEmpty() || selectedPath.isNotBlank()

/**
 * The installed ROMs the Kickstart list offers as files. The two AROS files
 * are left out: neither boots on its own as a Kickstart, so AROS is offered
 * once as [ModelRomAvailability.arosRomFile] instead.
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
internal fun romLabel(rom: AmigaFile): String = when (val identity = ModelRomAvailability.romIdentity(rom)) {
	is RomIdentity.Kickstart -> identity.name
	RomIdentity.BuiltInAros, RomIdentity.Unknown -> rom.name
}

/**
 * A kickstart_rom_file value by what it boots: the built-in AROS, a scanned
 * ROM by [romLabel], or else its file name (an AROS file named directly,
 * which does not boot AROS, reads as that file).
 */
@Composable
internal fun romPathLabel(path: String, roms: List<AmigaFile>): String = when {
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
internal fun FileDropdown(
	label: String,
	files: List<AmigaFile>,
	selectedPath: String,
	onSelect: (String) -> Unit,
	onClear: () -> Unit,
	emptyLabel: String = stringResource(R.string.placeholder_none),
	extraPaths: List<String> = emptyList(),
	help: String? = null,
	fileLabel: @Composable (AmigaFile) -> String = { it.name },
	pathLabel: @Composable (String) -> String = { LauncherChoices.fileName(it) },
	fieldModifier: Modifier = Modifier
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
						.then(fieldModifier)
						.dpadFieldFocusIndicator()
						.dpadArrowsMoveFocus()
						.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
						.fillMaxWidth()
				)
				ExposedDropdownMenu(
					expanded = expanded,
					onDismissRequest = { expanded = false }
				) {
					DropdownMenuItem(
						text = { Text(emptyLabel) },
						modifier = Modifier.dpadMenuItemInitialFocus(selectedPath.isEmpty()),
						onClick = {
							onClear()
							expanded = false
						}
					)
					extraPaths.forEach { path ->
						DropdownMenuItem(
							text = { Text(pathLabel(path)) },
							modifier = Modifier.dpadMenuItemInitialFocus(path == selectedPath),
							onClick = {
								onSelect(path)
								expanded = false
							}
						)
					}
					files.forEach { file ->
						DropdownMenuItem(
							text = { Text(fileLabel(file)) },
							modifier = Modifier.dpadMenuItemInitialFocus(file.path == selectedPath),
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
internal fun FloppyDriveRow(
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

			val importPicker = rememberImportPicker(
				mimeTypes = FilePickerFilters.mimeTypesFor(FileCategory.FLOPPIES),
				extensions = FileCategory.FLOPPIES.extensions
			) { uris ->
				uris.first().let {
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
				onClick = importPicker::pickFiles,
				enabled = !importInProgress,
				// Full width so D-pad Up/Down reach it: the focus search weighs the distance
				// between centres, and a short left-aligned button between two full-width
				// fields lost to the next field. The trailing spacer keeps the text left-aligned.
				modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
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
				Spacer(modifier = Modifier.weight(1f))
			}
		}
	}
}

private fun List<MountEntry>.updatedAt(index: Int, transform: (MountEntry) -> MountEntry): List<MountEntry> =
	mapIndexed { i, mount -> if (i == index) transform(mount) else mount }

private fun List<MountEntry>.removedAt(index: Int): List<MountEntry> =
	filterIndexed { i, _ -> i != index }

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun MountRow(
	settings: EmulatorSettings,
	schema: SettingsSchema?,
	index: Int,
	drive: MountEntry,
	files: List<AmigaFile>,
	onPathChange: (String) -> Unit,
	onReadOnlyChange: (Boolean) -> Unit,
	onBootPriorityChange: (Int) -> Unit,
	onMountChange: (MountEntry) -> Unit,
	onRemove: () -> Unit,
	fieldFocus: FocusRequester?,
	context: android.content.Context,
	importGuard: ImportInFlightGuard,
	importInProgress: Boolean,
	onImportResult: (FileManager.ImportResult) -> Unit
) {
	val scope = rememberCoroutineScope()
	val deleteFocus = remember { FocusRequester() }

	Column(modifier = Modifier.fillMaxWidth()) {
		Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
			Text(
				"${stringResource(R.string.settings_storage_hard_drive_label, index)} — ${mountKindLabel(drive.kind)}",
				style = MaterialTheme.typography.labelMedium,
				modifier = Modifier.weight(1f)
			)
			IconButton(
				onClick = onRemove,
				modifier = Modifier.focusRequester(deleteFocus).dpadFocusIndicator()
			) {
				Icon(Icons.Default.Delete, contentDescription = stringResource(R.string.settings_storage_remove_hard_drive))
			}
		}

		when (drive.kind) {
			MountKind.Hardfile -> {
				FileDropdown(
					label = stringResource(R.string.settings_storage_hardfile_label),
					files = files,
					selectedPath = drive.path,
					onSelect = onPathChange,
					onClear = { onPathChange("") },
					fieldModifier = Modifier
						.focusProperties { up = deleteFocus }
						.then(if (fieldFocus != null) Modifier.focusRequester(fieldFocus) else Modifier)
				)
				val importPicker = rememberImportPicker(
					mimeTypes = FilePickerFilters.mimeTypesFor(FileCategory.HARD_DRIVES),
					extensions = FileCategory.HARD_DRIVES.extensions
				) { uris ->
					uris.firstOrNull()?.let {
						scope.launchImportGuarded(importGuard) {
							val result = withContext(Dispatchers.IO) {
								FileManager.importFileWithResult(context, it, FileCategory.HARD_DRIVES).also { importResult ->
									if (importResult is FileManager.ImportResult.Imported) FileRepository.getInstance(context).rescanCategory(FileCategory.HARD_DRIVES)
								}
							}
							if (result is FileManager.ImportResult.Imported) onPathChange(result.path)
							onImportResult(result)
						}
					}
				}
				TextButton(
					onClick = importPicker::pickFiles,
					enabled = !importInProgress,
					modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
				) {
					if (importInProgress) {
						CircularProgressIndicator(modifier = Modifier.size(16.dp), strokeWidth = 2.dp)
						Spacer(modifier = Modifier.width(4.dp))
					}
					Text(stringResource(if (importInProgress) R.string.action_importing else R.string.settings_storage_import_hardfile))
					Spacer(modifier = Modifier.weight(1f))
				}
			}
			MountKind.Directory, MountKind.Tape -> MountTextField(
				stringResource(R.string.settings_storage_path),
				drive.path,
				modifier = if (fieldFocus != null) Modifier.focusRequester(fieldFocus) else Modifier,
				onValueChange = onPathChange
			)
			MountKind.Cd -> Unit
		}

		if (drive.kind == MountKind.Directory || drive.kind == MountKind.Hardfile) {
			// The core refuses a device name in use (add_filesys_config), so
			// the second drive would vanish when the machine starts.
			val usedNames = settings.hardDrives.filterIndexed { i, other ->
				i != index && (other.kind == MountKind.Directory || other.kind == MountKind.Hardfile)
			}.map { it.device.lowercase() }.filter { it.isNotEmpty() }.toSet()
			val inUse = stringResource(R.string.settings_storage_device_in_use)
			MountTextField(stringResource(R.string.settings_storage_device_name), drive.device,
				allowed = { MountNames.allowed(it, device = true) },
				refusal = { name -> if (name.lowercase() in usedNames) inUse else null }) {
				onMountChange(drive.copy(device = it))
			}
		}
		if (drive.kind == MountKind.Directory) {
			MountTextField(stringResource(R.string.settings_storage_volume_label), drive.volume,
				allowed = { MountNames.allowed(it, device = false) }) {
				onMountChange(drive.copy(volume = it))
			}
		}
		if (drive.kind != MountKind.Directory) {
			MountControllerControls(
				schema = schema,
				settings = settings,
				kind = drive.kind,
				controller = drive.controller,
				// A CD drive has no path field: its controller takes the
				// focus a newly added drive gets.
				focus = if (drive.kind == MountKind.Cd) fieldFocus else null,
				onControllerChange = { onMountChange(drive.copy(controller = it)) }
			)
		}
		if (drive.kind == MountKind.Hardfile) {
			Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
				MountTextField(stringResource(R.string.settings_storage_sectors), drive.sectors.toString(), KeyboardType.Number, Modifier.weight(1f), allowed = Char::isDigit) {
					it.toIntOrNull()?.let { value -> onMountChange(drive.copy(sectors = value)) }
				}
				MountTextField(stringResource(R.string.settings_storage_surfaces), drive.surfaces.toString(), KeyboardType.Number, Modifier.weight(1f), allowed = Char::isDigit) {
					it.toIntOrNull()?.let { value -> onMountChange(drive.copy(surfaces = value)) }
				}
			}
			Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
				MountTextField(stringResource(R.string.settings_storage_reserved), drive.reserved.toString(), KeyboardType.Number, Modifier.weight(1f), allowed = Char::isDigit) {
					it.toIntOrNull()?.let { value -> onMountChange(drive.copy(reserved = value)) }
				}
				MountTextField(stringResource(R.string.settings_storage_block_size), drive.blockSize.toString(), KeyboardType.Number, Modifier.weight(1f), allowed = Char::isDigit) {
					it.toIntOrNull()?.let { value -> onMountChange(drive.copy(blockSize = value)) }
				}
			}
			Text(stringResource(R.string.settings_storage_rdb_geometry), style = MaterialTheme.typography.bodySmall)
		}

		if (drive.kind == MountKind.Directory || drive.kind == MountKind.Hardfile) {
			SettingSwitchRow(
				label = stringResource(R.string.settings_storage_hard_drive_read_only),
				checked = drive.readOnly,
				onCheckedChange = onReadOnlyChange
			)
			val bootable = drive.bootPriority > MountEntry.BOOTPRI_NOAUTOBOOT
			SettingSwitchRow(
				label = stringResource(R.string.settings_storage_bootable),
				checked = bootable,
				onCheckedChange = { enabled ->
					onBootPriorityChange(if (enabled) 0 else MountEntry.BOOTPRI_NOAUTOBOOT)
				}
			)
			MountTextField(
				label = stringResource(R.string.settings_storage_boot_priority),
				value = drive.bootPriority.toString(),
				keyboardType = KeyboardType.Number,
				enabled = bootable,
				onValueChange = { input ->
					input.toIntOrNull()?.takeIf { it in MountEntry.BOOTPRI_NOAUTOMOUNT..127 }?.let(onBootPriorityChange)
				}
			)
		}
	}
}

@Composable
private fun mountKindLabel(kind: MountKind): String = stringResource(when (kind) {
	MountKind.Directory -> R.string.settings_storage_mount_directory
	MountKind.Hardfile -> R.string.settings_storage_mount_hardfile
	MountKind.Cd -> R.string.settings_storage_mount_cd
	MountKind.Tape -> R.string.settings_storage_mount_tape
})

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun MountControllerControls(
	schema: SettingsSchema?,
	settings: EmulatorSettings,
	kind: MountKind,
	controller: String,
	focus: FocusRequester? = null,
	onControllerChange: (String) -> Unit
) {
	val controllers = schema?.mountControllersFor(kind, settings, controller).orEmpty()
	val selected = controllers.firstOrNull { controllerMatches(controller, it.id) }
	val controllerChoices = if (selected == null) {
		listOf(controller to controller) + controllers.map { it.id to it.label }
	} else {
		controllers.map { it.id to it.label }
	}
	MountChoiceDropdown(
		label = stringResource(R.string.settings_storage_controller),
		focus = focus,
		selected = selected?.label ?: controller,
		choices = controllerChoices,
		onSelect = { id ->
			val choice = controllers.firstOrNull { it.id == id } ?: return@MountChoiceDropdown
			val units = choice.units.filter { it.supports(kind) }
			val currentUnit = controllerUnit(controller)
			val unit = units.firstOrNull { it.value == currentUnit } ?: units.firstOrNull() ?: return@MountChoiceDropdown
			onControllerChange(controllerValue(choice.id, unit.value))
		}
	)
	selected?.let { choice ->
		val units = choice.units.filter { it.supports(kind) }
		if (units.isNotEmpty()) {
			val currentUnit = controllerUnit(controller)
			MountChoiceDropdown(
				label = stringResource(R.string.settings_storage_unit),
				selected = units.firstOrNull { it.value == currentUnit }?.label ?: currentUnit.toString(),
				choices = units.map { it.value.toString() to it.label },
				onSelect = { value -> value.toIntOrNull()?.let { onControllerChange(controllerValue(choice.id, it)) } }
			)
		}
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun MountChoiceDropdown(
	label: String,
	selected: String,
	choices: List<Pair<String, String>>,
	focus: FocusRequester? = null,
	onSelect: (String) -> Unit
) {
	var expanded by remember { mutableStateOf(false) }
	ExposedDropdownMenuBox(expanded = expanded, onExpandedChange = { expanded = it }) {
		OutlinedTextField(
			value = selected,
			onValueChange = {},
			readOnly = true,
			label = { Text(label) },
			trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded) },
			modifier = Modifier
				.then(if (focus != null) Modifier.focusRequester(focus) else Modifier)
				.dpadFieldFocusIndicator()
				.dpadArrowsMoveFocus()
				.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
				.fillMaxWidth()
		)
		ExposedDropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
			choices.forEach { (value, text) ->
				DropdownMenuItem(
					text = { Text(text) },
					modifier = Modifier.dpadMenuItemInitialFocus(text == selected),
					onClick = {
						onSelect(value)
						expanded = false
					}
				)
			}
		}
	}
}

private fun controllerMatches(value: String, template: String): Boolean =
	Regex("^" + template.split("%d").joinToString("\\d+") { Regex.escape(it) } + "$").matches(value)

private fun controllerUnit(value: String): Int =
	Regex("""^(?:uae|ide|scsi|custom)(\d+)""").find(value)?.groupValues?.get(1)?.toIntOrNull() ?: 0

private fun controllerValue(template: String, unit: Int): String = template.replace("%d", unit.toString())

@Composable
private fun HardfileCreateDialog(
	error: Int?,
	onDismiss: () -> Unit,
	onCreate: (String, Int, Boolean) -> Unit
) {
	var name by remember { mutableStateOf("hardfile.hdf") }
	var sizeText by remember { mutableStateOf("100") }
	var dynamic by remember { mutableStateOf(false) }
	val size = sizeText.toIntOrNull()?.takeIf { it >= 1 }
	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.settings_storage_create_hardfile)) },
		text = {
			Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
				OutlinedTextField(
					value = name,
					onValueChange = { name = it },
					label = { Text(stringResource(R.string.settings_storage_hardfile_label)) },
					singleLine = true,
					modifier = Modifier.fillMaxWidth().dpadVerticalMovesFocus()
				)
				OutlinedTextField(
					value = sizeText,
					onValueChange = { sizeText = it },
					label = { Text(stringResource(R.string.settings_storage_hardfile_size_mb)) },
					singleLine = true,
					keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
					modifier = Modifier.fillMaxWidth().dpadVerticalMovesFocus()
				)
				if (sizeText.isNotBlank() && size == null) {
					Text(stringResource(R.string.settings_storage_size_minimum), color = MaterialTheme.colorScheme.error)
				}
				error?.let { Text(stringResource(it), color = MaterialTheme.colorScheme.error) }
				Row(verticalAlignment = Alignment.CenterVertically) {
					Text(stringResource(R.string.settings_storage_dynamic_vhd), modifier = Modifier.weight(1f))
					Switch(checked = dynamic, onCheckedChange = { dynamic = it })
				}
			}
		},
		confirmButton = {
			TextButton(
				onClick = { onCreate(name, size!!, dynamic) },
				enabled = name.isNotBlank() && size != null,
				modifier = Modifier.dpadFocusIndicator()
			) { Text(stringResource(R.string.action_create)) }
		},
		dismissButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}

@Composable
private fun MountTextField(
	label: String,
	value: String,
	keyboardType: KeyboardType = KeyboardType.Text,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	/** Characters the field takes; others are dropped as they are typed. */
	allowed: (Char) -> Boolean = { true },
	/** Why a value cannot be kept (shown under the field, which then keeps the last good value), or null. */
	refusal: (String) -> String? = { null },
	onValueChange: (String) -> Unit
) {
	var text by remember(value) { mutableStateOf(value) }
	val problem = refusal(text)
	val typing = rememberDpadTypingGate()
	OutlinedTextField(
		value = text,
		onValueChange = { typed ->
			val kept = typed.filter(allowed)
			text = kept
			if (refusal(kept) == null) onValueChange(kept)
		},
		label = { Text(label) },
		singleLine = true,
		readOnly = typing.readOnly,
		enabled = enabled,
		isError = problem != null,
		supportingText = problem?.let { { Text(it) } },
		keyboardOptions = KeyboardOptions(keyboardType = keyboardType, imeAction = ImeAction.Done),
		keyboardActions = KeyboardActions(onDone = { typing.stopTyping() }),
		modifier = modifier.fillMaxWidth().dpadVerticalMovesFocus().then(typing.modifier)
	)
}
