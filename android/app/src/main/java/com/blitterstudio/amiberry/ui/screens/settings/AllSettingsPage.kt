package com.blitterstudio.amiberry.ui.screens.settings

import androidx.annotation.StringRes
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.ReasonText
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.hasTouchScreen
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/** One group's rows on the All Settings page. */
private data class AllSettingsSection(
	val group: String,
	@param:StringRes val titleRes: Int,
	val rows: List<SettingsSchema.Option>,
	/** Why the unavailable rows are, once each (SettingsGroups.constraintNotes). */
	val notes: List<String>
)

/**
 * The All Settings page: every listed option the launcher's rows can edit,
 * grouped by layer group, searchable over the label, help and id (the
 * Apple app's SearchResultsView). Options the launcher manages itself
 * (SettingsGroups.managed) have no working control and do not show; the
 * options whose working control is one of the pages' own rows (the joyport
 * devices, the shader) get that row here too. The "Only changed" toggle
 * leaves the options that differ from what Review changes compares with
 * (the open configuration as last saved, or the model's preset).
 */
@Composable
fun AllSettingsPage(
	viewModel: SettingsViewModel,
	pageFocus: FocusRequester
) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val state = remember(settings, schema) { settings.schemaState(schema) }
	var query by rememberSaveable { mutableStateOf("") }
	var onlyChanged by rememberSaveable { mutableStateOf(false) }
	// The IDs "Only changed" matched when it was turned on, kept until it is
	// turned off or the query changes: editing a row back to its saved value
	// must not take the row (and the D-pad focus on it) away mid-edit.
	var frozenChanged by remember { mutableStateOf<Set<String>?>(null) }
	LaunchedEffect(onlyChanged, query) {
		frozenChanged = if (onlyChanged) {
			viewModel.changeSummary.mapNotNull { it.optionId }.toSet()
		} else {
			null
		}
	}
	val terms = query.trim().lowercase().split(' ').filter { it.isNotEmpty() }
	// The shader row offers the catalog's entries, so it needs them scanned.
	LaunchedEffect(viewModel) {
		viewModel.refreshShaderCatalog()
	}
	// The rows of every group that has a match, in the layer's order. Unlike a
	// page's group sections these leave out only the managed options, and the
	// memory boards a page shows only while in use are listed too.
	val sections = remember(settings, schema, terms, onlyChanged, frozenChanged) {
		val changed = frozenChanged
		SettingsGroups.groupsOf(SettingsPage.AllSettings, schema).mapNotNull { group ->
			val rows = SettingsGroups.options(schema, group, placed = SettingsGroups.managed)
				.filter { option ->
					(!onlyChanged || option.id in changed.orEmpty()) &&
						terms.all {
							"${option.label} ${option.help.orEmpty()} ${option.id}".lowercase().contains(it)
						}
				}
			if (rows.isEmpty()) {
				null
			} else {
				AllSettingsSection(
					group = group,
					titleRes = SettingsGroups.titleRes(group),
					rows = rows,
					notes = SettingsGroups.constraintNotes(rows) {
						SettingsControlAvailability.isAvailable(state, it, schema)
					}
				)
			}
		}
	}

	Column(
		modifier = Modifier
			.fillMaxSize()
			// Right from the side list and Up from Start enter here, at the
			// control last focused, even when no section matched.
			.focusRequester(pageFocus)
			.focusRestorer()
	) {
		val typingGate = rememberDpadTypingGate()
		Column(
			modifier = Modifier
				.fillMaxWidth()
				.padding(start = 16.dp, top = 16.dp, end = 16.dp),
			verticalArrangement = Arrangement.spacedBy(8.dp)
		) {
			OutlinedTextField(
				value = query,
				onValueChange = { query = it },
				label = { Text(stringResource(R.string.settings_search_all_label)) },
				leadingIcon = { Icon(Icons.Default.Search, contentDescription = null) },
				trailingIcon = {
					if (query.isNotBlank()) {
						IconButton(
							onClick = { query = "" },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_clear_search))
						}
					}
				},
				singleLine = true,
				readOnly = typingGate.readOnly,
				keyboardOptions = KeyboardOptions(imeAction = ImeAction.Search),
				keyboardActions = KeyboardActions(onSearch = { typingGate.stopTyping() }, onDone = { typingGate.stopTyping() }),
				modifier = Modifier
					.fillMaxWidth()
					.dpadVerticalMovesFocus()
					.then(typingGate.modifier)
			)
			SettingSwitchRow(
				label = stringResource(R.string.settings_only_changed),
				checked = onlyChanged,
				onCheckedChange = { onlyChanged = it }
			)
		}

		if (sections.isEmpty()) {
			Text(
				stringResource(R.string.settings_search_no_matches),
				style = MaterialTheme.typography.bodyLarge,
				color = MaterialTheme.colorScheme.onSurfaceVariant,
				modifier = Modifier.padding(16.dp)
			)
		} else {
			LazyColumn(
				modifier = Modifier
					.weight(1f)
					.fillMaxWidth()
					.focusRestorer(),
				contentPadding = PaddingValues(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
				verticalArrangement = Arrangement.spacedBy(8.dp)
			) {
				sections.forEach { section ->
					item(key = "header-${section.group}", contentType = "header") {
						Text(
							stringResource(section.titleRes),
							style = MaterialTheme.typography.titleMedium,
							modifier = Modifier.padding(top = 8.dp)
						)
					}
					items(section.rows, key = { it.id }, contentType = { "row" }) { option ->
						AllSettingsOptionRow(
							option = option,
							viewModel = viewModel,
							settings = settings,
							state = state,
							schema = schema,
							sectionTitle = stringResource(section.titleRes)
						)
					}
					if (section.notes.isNotEmpty()) {
						item(key = "footer-${section.group}", contentType = "footer") {
							ReasonText(section.notes.joinToString("\n"))
						}
					}
				}
			}
		}
	}
}

/**
 * One option's row on the All Settings page: the same row the page that owns
 * the option shows where the launcher has its own for it (the joyport
 * devices, the shader), else the generated row of its type.
 */
@Composable
private fun AllSettingsOptionRow(
	option: SettingsSchema.Option,
	viewModel: SettingsViewModel,
	settings: EmulatorSettings,
	state: Map<String, String>,
	schema: SettingsSchema,
	sectionTitle: String
) {
	when (option.id) {
		// The schema has no device list (the layer enumerates the host's
		// devices at run time), so the launcher offers its own.
		"joyport0" -> SchemaChoiceRow(
			id = "joyport0",
			value = settings.joyport0,
			choices = inputDeviceOptionIds(hasTouchScreen(), portIndex = 0).map { token ->
				SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
			},
			onSelect = { value -> viewModel.updateSettings { s -> s.copy(joyport0 = value) } }
		)
		"joyport1" -> SchemaChoiceRow(
			id = "joyport1",
			value = settings.joyport1,
			choices = inputDeviceOptionIds(hasTouchScreen(), portIndex = 1).map { token ->
				SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
			},
			onSelect = { value -> viewModel.updateSettings { s -> InputSettingsActions.selectPort1Device(s, value) } }
		)
		// The shader list is the catalog's, not the schema's.
		"amiberry.shader" -> ShaderDropdownRow(viewModel)
		else -> SchemaGroupRows(
			options = listOf(option),
			settings = settings,
			state = state,
			schema = schema,
			showReasons = false,
			sectionTitle = sectionTitle
		) { id, value ->
			viewModel.updateSettings { s -> s.withOption(id, value) }
		}
	}
}
