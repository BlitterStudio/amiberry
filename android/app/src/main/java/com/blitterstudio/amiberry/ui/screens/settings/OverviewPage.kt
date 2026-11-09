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
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.Icon
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
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.installedMemory
import com.blitterstudio.amiberry.ui.ImportInFlightGuard
import com.blitterstudio.amiberry.ui.components.settings.CompatibilityLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.dpadMenuItemInitialFocus
import com.blitterstudio.amiberry.ui.components.settings.schemaLabel
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.schemaValueText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.dpadArrowsMoveFocus
import com.blitterstudio.amiberry.ui.dpadFieldFocusIndicator
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.rememberImportInFlightGuard
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlin.math.roundToInt

internal sealed interface MemoryAmountDisplay {
	data object None : MemoryAmountDisplay
	data class Kilobytes(val kb: Int) : MemoryAmountDisplay
	data class WholeMegabytes(val mb: Int) : MemoryAmountDisplay
	data class FractionalMegabytes(val tenths: Int) : MemoryAmountDisplay
}

internal fun memoryAmountDisplayValue(kb: Int): MemoryAmountDisplay {
	if (kb <= 0) return MemoryAmountDisplay.None
	if (kb < 1024) return MemoryAmountDisplay.Kilobytes(kb)

	val tenths = ((kb / 1024.0) * 10).roundToInt()
	return if (tenths % 10 == 0) {
		MemoryAmountDisplay.WholeMegabytes(tenths / 10)
	} else {
		MemoryAmountDisplay.FractionalMegabytes(tenths)
	}
}

/**
 * What defines the machine at a glance (the Apple app's Overview page): its
 * model and compatibility, its Kickstart, memory and first drives, and its
 * name and description in the launcher. The name and description are the
 * Save dialog's to edit, so its button opens it. The first drives are the
 * Storage page's rows, shown here too (as the Apple Overview shows
 * floppy0/1 beside the Storage page's drive sections).
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun OverviewPage(
	viewModel: SettingsViewModel,
	onEditMachine: () -> Unit,
	importInProgress: Boolean,
	onImportResult: (FileManager.ImportResult) -> Unit
) {
	val context = LocalContext.current
	val settings = viewModel.settings
	val roms by viewModel.availableRoms.collectAsState()
	val floppies by viewModel.availableFloppies.collectAsState()
	val importGuard = rememberImportInFlightGuard()

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// The machine's name and description, as the Library shows them.
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(
					viewModel.currentConfigName ?: stringResource(R.string.settings_new_configuration),
					style = MaterialTheme.typography.titleMedium
				)
				if (viewModel.currentConfigDescription.isNotBlank()) {
					Text(
						viewModel.currentConfigDescription,
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
				Spacer(modifier = Modifier.height(8.dp))
				TextButton(onClick = onEditMachine, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_edit_machine_details))
				}
			}
		}

		// The Amiga model: the one machine picker (its preset sets CPU, chipset and memory).
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_cpu_model_preset), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				var modelExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = modelExpanded,
					onExpandedChange = { modelExpanded = it }
				) {
					OutlinedTextField(
						value = settings.baseModel.displayName,
						onValueChange = {},
						readOnly = true,
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = modelExpanded) },
						modifier = Modifier
							.dpadFieldFocusIndicator()
							.dpadArrowsMoveFocus()
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth(),
						supportingText = {
							Text(stringResource(R.string.settings_cpu_model_preset_help))
						}
					)
					ExposedDropdownMenu(
						expanded = modelExpanded,
						onDismissRequest = { modelExpanded = false }
					) {
						AmigaModel.entries.forEach { m ->
							DropdownMenuItem(
								text = { Text("${m.displayName} - ${m.description}") },
								modifier = Modifier.dpadMenuItemInitialFocus(m == settings.baseModel),
								onClick = {
									viewModel.applyModel(m)
									modelExpanded = false
								}
							)
						}
					}
				}
				CompatibilityLevelRow(
					settings = settings,
					onSelect = viewModel::applyCompatibilityLevel,
					modifier = Modifier.padding(top = 8.dp)
				)
			}
		}

		// Kickstart, memory and the first drives: what the machine boots with.
		SettingsSection(title = stringResource(R.string.settings_overview_at_a_glance_title)) {
			Row(verticalAlignment = Alignment.CenterVertically) {
				Icon(Icons.Default.Memory, contentDescription = null, modifier = Modifier.size(20.dp))
				Spacer(modifier = Modifier.width(8.dp))
				Text(
					stringResource(R.string.settings_storage_kickstart_rom_title),
					style = MaterialTheme.typography.titleSmall
				)
			}

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

			Spacer(modifier = Modifier.height(8.dp))

			// Summary: each installed memory by the layer's label and size, then the total.
			val schema = settingsSchema()
			if (schema != null) {
				val installed = installedMemory(settings, schema)
				installed.forEach { memory ->
					Text(
						stringResource(
							R.string.settings_label_value,
							schemaLabel(memory.id),
							schemaValueText(memory.id, memory.value)
						),
						style = MaterialTheme.typography.bodyMedium
					)
				}
				Text(
					stringResource(R.string.settings_memory_summary_total, formatMemory(installed.sumOf { it.kb })),
					style = MaterialTheme.typography.titleSmall,
					color = MaterialTheme.colorScheme.primary
				)
			}

			Spacer(modifier = Modifier.height(8.dp))

			for (index in 0..1) {
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
		}
	}
}

internal fun driveLabelRes(index: Int): Int = when (index) {
	0 -> R.string.settings_storage_drive_df0
	1 -> R.string.settings_storage_drive_df1
	2 -> R.string.settings_storage_drive_df2
	else -> R.string.settings_storage_drive_df3
}

internal fun EmulatorSettings.drivePath(index: Int): String = when (index) {
	0 -> floppy0
	1 -> floppy1
	2 -> floppy2
	else -> floppy3
}

internal fun EmulatorSettings.driveType(index: Int): Int = when (index) {
	0 -> floppy0Type
	1 -> floppy1Type
	2 -> floppy2Type
	else -> floppy3Type
}

internal fun EmulatorSettings.withDrivePath(index: Int, path: String): EmulatorSettings = when (index) {
	0 -> copy(floppy0 = path)
	1 -> copy(floppy1 = path)
	2 -> copy(floppy2 = path)
	else -> copy(floppy3 = path)
}

internal fun EmulatorSettings.withDriveType(index: Int, type: Int): EmulatorSettings = when (index) {
	0 -> copy(floppy0Type = type)
	1 -> copy(floppy1Type = type)
	2 -> copy(floppy2Type = type)
	else -> copy(floppy3Type = type)
}

@Composable
private fun formatMemory(kb: Int): String {
	return when (val amount = memoryAmountDisplayValue(kb)) {
		MemoryAmountDisplay.None -> stringResource(R.string.settings_option_none)
		is MemoryAmountDisplay.Kilobytes -> stringResource(R.string.settings_memory_value_kb, amount.kb)
		is MemoryAmountDisplay.WholeMegabytes -> stringResource(R.string.settings_memory_value_mb, amount.mb)
		is MemoryAmountDisplay.FractionalMegabytes -> {
			stringResource(R.string.settings_memory_value_mb_decimal, amount.tenths / 10.0)
		}
	}
}
