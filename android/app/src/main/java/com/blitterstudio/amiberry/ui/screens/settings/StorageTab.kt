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
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.Eject
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.SaveAlt
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
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun StorageTab(viewModel: SettingsViewModel) {
	val context = LocalContext.current
	val settings = viewModel.settings
	val roms by viewModel.availableRoms.collectAsState()
	val floppies by viewModel.availableFloppies.collectAsState()
	val cds by viewModel.availableCds.collectAsState()

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

				if (roms.isEmpty()) {
					Text(
						stringResource(R.string.settings_storage_no_roms_found),
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.error
					)
				} else {
					FileDropdown(
						label = stringResource(R.string.settings_storage_rom_file_label),
						files = roms,
						selectedPath = settings.romFile,
						onSelect = { viewModel.updateSettings { s -> s.copy(romFile = it) } },
						onClear = { viewModel.updateSettings { s -> s.copy(romFile = "") } }
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
					label = stringResource(R.string.settings_storage_drive_df0),
					files = floppies,
					selectedPath = settings.floppy0,
					driveType = settings.floppy0Type,
					onSelect = { viewModel.updateSettings { s -> s.copy(floppy0 = it) } },
					onClear = { viewModel.updateSettings { s -> s.copy(floppy0 = "") } },
					onTypeChange = { viewModel.updateSettings { s -> s.copy(floppy0Type = it) } },
					context = context
				)

				Spacer(modifier = Modifier.height(8.dp))

				// DF1
				FloppyDriveRow(
					label = stringResource(R.string.settings_storage_drive_df1),
					files = floppies,
					selectedPath = settings.floppy1,
					driveType = settings.floppy1Type,
					onSelect = { viewModel.updateSettings { s -> s.copy(floppy1 = it) } },
					onClear = { viewModel.updateSettings { s -> s.copy(floppy1 = "") } },
					onTypeChange = { viewModel.updateSettings { s -> s.copy(floppy1Type = it) } },
					context = context
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

				if (cds.isEmpty()) {
					Text(
						stringResource(R.string.settings_storage_no_cd_images_found),
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				} else {
					FileDropdown(
						label = stringResource(R.string.settings_storage_cd_image_label),
						files = cds,
						selectedPath = settings.cdImage,
						onSelect = { viewModel.updateSettings { s -> s.copy(cdImage = it) } },
						onClear = { viewModel.updateSettings { s -> s.copy(cdImage = "") } }
					)
				}
			}
		}
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun FileDropdown(
	label: String,
	files: List<AmigaFile>,
	selectedPath: String,
	onSelect: (String) -> Unit,
	onClear: () -> Unit
) {
	var expanded by remember { mutableStateOf(false) }
	val noneLabel = stringResource(R.string.placeholder_none)
	val selectedName = if (selectedPath.isEmpty()) noneLabel
	else selectedPath.substringAfterLast('/')

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
						.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
						.fillMaxWidth()
				)
			ExposedDropdownMenu(
				expanded = expanded,
				onDismissRequest = { expanded = false }
			) {
				DropdownMenuItem(
					text = { Text(noneLabel) },
					onClick = {
						onClear()
						expanded = false
					}
				)
				files.forEach { file ->
					DropdownMenuItem(
						text = { Text(file.name) },
						onClick = {
							onSelect(file.path)
							expanded = false
						}
					)
				}
			}
		}
		if (selectedPath.isNotEmpty()) {
			IconButton(onClick = onClear) {
				Icon(Icons.Default.Eject, contentDescription = stringResource(R.string.action_eject))
			}
		}
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun FloppyDriveRow(
	label: String,
	files: List<AmigaFile>,
	selectedPath: String,
	driveType: Int,
	onSelect: (String) -> Unit,
	onClear: () -> Unit,
	onTypeChange: (Int) -> Unit,
	context: android.content.Context
) {
	val driveTypeOptions = listOf(
		0 to stringResource(R.string.settings_storage_drive_type_dd),
		1 to stringResource(R.string.settings_storage_drive_type_hd),
		-1 to stringResource(R.string.settings_common_disabled)
	)

	Column(modifier = Modifier.fillMaxWidth()) {
		Text(label, style = MaterialTheme.typography.labelMedium)
		Spacer(modifier = Modifier.height(4.dp))

		// Drive type
		var typeExpanded by remember { mutableStateOf(false) }
		val typeLabel = driveTypeOptions.firstOrNull { it.first == driveType }?.second
			?: stringResource(R.string.settings_storage_drive_type_unknown)

		ExposedDropdownMenuBox(
			expanded = typeExpanded,
			onExpandedChange = { typeExpanded = it },
			modifier = Modifier.fillMaxWidth()
		) {
			OutlinedTextField(
				value = typeLabel,
				onValueChange = {},
				readOnly = true,
				label = { Text(stringResource(R.string.settings_storage_type_label)) },
				trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = typeExpanded) },
				modifier = Modifier
					.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
					.fillMaxWidth()
			)
			ExposedDropdownMenu(
				expanded = typeExpanded,
				onDismissRequest = { typeExpanded = false }
			) {
				driveTypeOptions.forEach { (value, text) ->
					DropdownMenuItem(
						text = { Text(text) },
						onClick = {
							onTypeChange(value)
							typeExpanded = false
						}
					)
				}
			}
		}

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
					val path = FileManager.importFile(context, it, FileCategory.FLOPPIES)
					if (path != null) onSelect(path)
				}
			}
			TextButton(onClick = { importLauncher.launch(arrayOf("*/*")) }) {
				Text(stringResource(R.string.settings_storage_import_disk_image))
			}
		}
	}
}
