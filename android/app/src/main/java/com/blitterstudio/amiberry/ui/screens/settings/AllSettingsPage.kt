package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
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
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/**
 * The All Settings page: every listed option the launcher's rows can edit,
 * grouped by layer group, searchable over the label, help and id (the
 * Apple app's SearchResultsView). The "Only changed" toggle leaves the
 * options that differ from what Review changes compares with (the open
 * configuration as last saved, or the model's preset).
 */
@Composable
fun AllSettingsPage(
	viewModel: SettingsViewModel,
	pageFocus: FocusRequester
) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val state = remember(settings, schema) { settings.schemaState(schema) }
	val changed = remember(viewModel.changeSummary) {
		viewModel.changeSummary.mapNotNull { it.optionId }.toSet()
	}
	var query by rememberSaveable { mutableStateOf("") }
	var onlyChanged by rememberSaveable { mutableStateOf(false) }
	val terms = query.trim().lowercase().split(' ').filter { it.isNotEmpty() }

	fun matches(option: SettingsSchema.Option): Boolean {
		if (onlyChanged && option.id !in changed) return false
		if (terms.isEmpty()) return true
		val text = "${option.label} ${option.help.orEmpty()} ${option.id}".lowercase()
		return terms.all { text.contains(it) }
	}

	// The rows of every group that has a match, in the layer's order. Unlike
	// a page's group sections these leave nothing placed out, and the memory
	// boards a page shows only while in use are listed too.
	val sections = SettingsGroups.groupsOf(SettingsPage.AllSettings, schema)
		.mapNotNull { group ->
			val rows = SettingsGroups.options(schema, group, placed = emptySet()).filter(::matches)
			if (rows.isEmpty()) null else group to rows
		}

	Column(modifier = Modifier.fillMaxSize()) {
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
				modifier = Modifier.fillMaxWidth().dpadVerticalMovesFocus()
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
					.focusRequester(pageFocus)
					.focusRestorer(),
				contentPadding = PaddingValues(
					start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp
				),
				verticalArrangement = Arrangement.spacedBy(16.dp)
			) {
				items(sections, key = { it.first }) { (group, rows) ->
					val title = stringResource(SettingsGroups.titleRes(group))
					SettingsSection(
						title = title,
						footer = SettingsGroups.constraintNotes(rows) {
							SettingsControlAvailability.isAvailable(state, it, schema)
						}.joinToString("\n").ifBlank { null }
					) {
						SchemaGroupRows(
							options = rows,
							settings = settings,
							state = state,
							schema = schema,
							showReasons = false,
							sectionTitle = title
						) { id, value ->
							viewModel.updateSettings { s -> s.withOption(id, value) }
						}
					}
				}
			}
		}
	}
}
