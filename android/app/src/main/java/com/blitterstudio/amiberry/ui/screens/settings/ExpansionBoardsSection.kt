package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
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
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.BoardUnit
import com.blitterstudio.amiberry.data.model.SettingsBoards
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.SettingChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.ChoiceItem
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import com.blitterstudio.amiberry.data.model.BoardEditor

/**
 * The Hardware page's Expansion boards section, after the Apple app's
 * BoardsSections: the units the configuration lists, each with its fields
 * (SettingsBoards' codec writes their lines), and adding more. The
 * built-in controllers list while their chipset options are on and cannot
 * be removed through here (a machine has them).
 */
@Composable
fun ExpansionBoardsSection(viewModel: SettingsViewModel) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val units = BoardEditor.units(schema, settings)
	var adding by remember { mutableStateOf(false) }

	SettingsSection(
		title = stringResource(R.string.settings_hardware_boards_title),
		footer = stringResource(R.string.settings_hardware_boards_footer)
	) {
		if (units.isEmpty()) {
			Text(
				stringResource(R.string.settings_hardware_boards_empty),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
		}
		for (unit in units) {
			BoardUnitCard(unit, viewModel)
		}
		TextButton(
			onClick = { adding = true },
			modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
		) {
			Icon(Icons.Default.Add, contentDescription = null, modifier = Modifier.width(18.dp))
			Spacer(Modifier.width(4.dp))
			Text(stringResource(R.string.settings_hardware_boards_add))
		}
	}
	if (adding) {
		AddBoardDialog(
			onDismiss = { adding = false },
			onAdd = { typeId ->
				adding = false
				viewModel.updateSettings { BoardEditor.add(it, schema, typeId) }
			}
		)
	}
}

/** One unit: its fields, and removing it. */
@Composable
private fun BoardUnitCard(unit: BoardUnit, viewModel: SettingsViewModel) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val type = schema.boardType(unit.typeId) ?: return
	val removable = BoardEditor.removable(schema, settings, unit)

	Column(modifier = Modifier.fillMaxWidth()) {
		Row(verticalAlignment = Alignment.CenterVertically) {
			Column(modifier = Modifier.weight(1f)) {
				Text(BoardEditor.title(schema, unit), style = MaterialTheme.typography.titleSmall)
				type.manufacturer?.let {
					Text(it, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
				}
			}
			if (removable) {
				val name = BoardEditor.title(schema, unit)
				IconButton(
					onClick = { viewModel.updateSettings { BoardEditor.remove(it, schema, unit) } },
					modifier = Modifier.dpadFocusIndicator()
				) {
					Icon(
						Icons.Default.Delete,
						contentDescription = stringResource(R.string.settings_hardware_boards_remove) + " " + name
					)
				}
			}
		}
		Spacer(Modifier.height(4.dp))
		for (field in type.fields) {
			when (field.id) {
				"enabled" -> {}
				"rom_file" -> BoardRomRow(unit, viewModel)
				else -> BoardFieldRow(unit, field, viewModel)
			}
		}
	}
}

/** The board's ROM image: the scanned ROMs, or none (the emulator loads the one it scanned). */
@Composable
private fun BoardRomRow(unit: BoardUnit, viewModel: SettingsViewModel) {
	val schema = settingsSchema() ?: return
	val type = schema.boardType(unit.typeId) ?: return
	val roms by viewModel.availableRoms.collectAsState()
	val value = SettingsBoards.romFileOf(unit.romFile)
	FileDropdown(
		label = type.field("rom_file")?.label
			?: stringResource(R.string.settings_hardware_boards_rom_automatic),
		files = roms,
		selectedPath = value,
		emptyLabel = stringResource(R.string.settings_hardware_boards_rom_automatic),
		onSelect = { path ->
			viewModel.updateSettings { BoardEditor.setField(it, schema, unit, "rom_file", path) }
		},
		onClear = {
			viewModel.updateSettings { BoardEditor.setField(it, schema, unit, "rom_file", "") }
		}
	)
}

/** One field of the unit: a switch, a dropdown or a text field, as its type calls for. */
@Composable
private fun BoardFieldRow(unit: BoardUnit, field: SettingsSchema.BoardField, viewModel: SettingsViewModel) {
	val schema = settingsSchema() ?: return
	val own = viewModel.settings.boards.firstOrNull { it.typeId == unit.typeId && it.unit == unit.unit }
	val value = when (field.id) {
		"rom_file" -> SettingsBoards.romFileOf(own?.romFile ?: unit.romFile)
		else -> (own?.fields ?: unit.fields)[field.id] ?: field.default
	}
	when (field.type) {
		SettingsSchema.TYPE_BOOL -> SettingSwitchRow(
			label = field.label,
			checked = value == "true",
			help = field.help.takeIf { it.isNotBlank() },
			onCheckedChange = { on ->
				viewModel.updateSettings {
					BoardEditor.setField(it, schema, unit, field.id, if (on) "true" else "false")
				}
			}
		)
		SettingsSchema.TYPE_CHOICE -> {
			val choices = field.choices.map { ChoiceItem(it.value, it.label) }
			// A value outside the type's list (a hand-edited file) stays shown.
			val offered = if (choices.any { it.value == value }) choices
			else choices + ChoiceItem(value, value)
			SettingChoiceRow(
				label = field.label,
				valueText = field.valueLabel(value),
				choices = offered,
				help = field.help.takeIf { it.isNotBlank() },
				onSelect = { selected ->
					viewModel.updateSettings { BoardEditor.setField(it, schema, unit, field.id, selected) }
				}
			)
		}
		else -> BoardTextFieldRow(field.label, value, field.help.takeIf { it.isNotBlank() }) { input ->
			viewModel.updateSettings { BoardEditor.setField(it, schema, unit, field.id, input) }
		}
	}
}

/** The Add board dialog: the board types the configuration can take, searched. */
@Composable
private fun AddBoardDialog(onDismiss: () -> Unit, onAdd: (String) -> Unit) {
	val schema = settingsSchema() ?: return
	val types = BoardEditor.addable(schema)
	var query by remember { mutableStateOf("") }

	AlertDialog(
		onDismissRequest = onDismiss,
		title = { Text(stringResource(R.string.settings_hardware_boards_add_title)) },
		text = {
			Column {
				OutlinedTextField(
					value = query,
					onValueChange = { query = it },
					label = { Text(stringResource(R.string.settings_hardware_boards_search_hint)) },
					singleLine = true,
					modifier = Modifier.fillMaxWidth()
				)
				Spacer(Modifier.height(8.dp))
				val needle = query.trim().lowercase()
				val matches = types.filter {
					needle.isEmpty() || it.label.lowercase().contains(needle) ||
						it.manufacturer?.lowercase()?.contains(needle) == true
				}
				LazyColumn(modifier = Modifier.heightIn(max = 384.dp)) {
					items(matches, key = { it.id }) { type ->
						Row(
							verticalAlignment = Alignment.CenterVertically,
							modifier = Modifier
								.fillMaxWidth()
								.dpadFocusIndicator()
								.clickable { onAdd(type.id) }
								.padding(vertical = 8.dp)
						) {
							Column {
								Text(type.label, style = MaterialTheme.typography.bodyLarge)
								Text(
									type.manufacturer ?: type.categories.joinToString(),
									style = MaterialTheme.typography.bodySmall,
									color = MaterialTheme.colorScheme.onSurfaceVariant
								)
							}
						}
					}
				}
			}
		},
		confirmButton = {},
		dismissButton = {
			TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
				Text(stringResource(R.string.action_cancel))
			}
		}
	)
}

/** A free-form text setting (a board's or a jumper's); the codec escapes what the line needs. */
@Composable
internal fun BoardTextFieldRow(label: String, value: String, help: String? = null, onChange: (String) -> Unit) {
	var text by remember(value) { mutableStateOf(value) }
	Column(modifier = Modifier.fillMaxWidth()) {
		OutlinedTextField(
			value = text,
			onValueChange = { input ->
				text = input
				onChange(input)
			},
			label = { Text(label) },
			singleLine = true,
			modifier = Modifier.fillMaxWidth()
		)
		if (help != null) {
			Text(help, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
		}
	}
}
