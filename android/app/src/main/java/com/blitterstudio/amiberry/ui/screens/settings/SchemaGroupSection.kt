package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.res.stringResource
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.CollapsibleSettingsSection
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaNumberRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlin.math.roundToInt

/**
 * The section of a settings layer group: the tab's own rows for the group
 * ([leading], when the tab has some) and then every option of the group
 * that [placed] leaves out (SettingsGroups.options), each as its type's
 * row. Unavailable rows are dimmed; the footer gives each reason once
 * ([footer], the tab's own, first). A collapsed section (SettingsGroups.byTab) starts closed.
 * Nothing shows for a group without rows.
 */
@Composable
fun SchemaGroupSection(
	group: String,
	viewModel: SettingsViewModel,
	placed: Set<String> = SettingsGroups.placed,
	title: String? = null,
	footer: String? = null,
	collapsed: Boolean = SettingsGroups.startsCollapsed(group),
	leading: (@Composable ColumnScope.() -> Unit)? = null
) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val state = remember(settings, schema) { settings.schemaState(schema) }
	val rows = shownOptions(schema, settings, group, placed)
	if (rows.isEmpty() && leading == null) return
	val reasons = rows
		.filter { !SettingsControlAvailability.isAvailable(state, it.id, schema) }
		.mapNotNull { it.constraint }
	val sectionFooter = (listOfNotNull(footer) + reasons).distinct().joinToString("\n").ifBlank { null }
	val sectionTitle = title ?: stringResource(SettingsGroups.titleRes(group))
	val content: @Composable ColumnScope.() -> Unit = {
		leading?.invoke(this)
		SchemaGroupRows(rows, settings, state, schema, showReasons = false) { id, value ->
			viewModel.updateSettings { s -> s.withOption(id, value) }
		}
	}
	if (collapsed) {
		var expanded by rememberSaveable(group) { mutableStateOf(false) }
		CollapsibleSettingsSection(
			title = sectionTitle,
			expanded = expanded,
			onExpandedChange = { expanded = it },
			footer = sectionFooter,
			content = content
		)
	} else {
		SettingsSection(title = sectionTitle, footer = sectionFooter, content = content)
	}
}

/**
 * The rows of a group's options [placed] leaves out, inside a card of the
 * tab's own (the floppy drives), each saying why under it while unavailable.
 */
@Composable
fun SchemaGroupInlineRows(group: String, viewModel: SettingsViewModel, placed: Set<String> = SettingsGroups.placed) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val state = remember(settings, schema) { settings.schemaState(schema) }
	SchemaGroupRows(shownOptions(schema, settings, group, placed), settings, state, schema, showReasons = true) { id, value ->
		viewModel.updateSettings { s -> s.withOption(id, value) }
	}
}

/** [group]'s options for its rows, without the memory boards that follow boards not in use. */
private fun shownOptions(
	schema: SettingsSchema,
	settings: EmulatorSettings,
	group: String,
	placed: Set<String>
): List<SettingsSchema.Option> = SettingsGroups.options(schema, group, placed).filter { option ->
	SettingsGroups.shows(option.id) { id -> settings.optionValue(id) ?: schema.option(id)?.defaultOnAndroid }
}

/**
 * [options] as rows (SettingsGroups.rowKind) showing their value in
 * [settings], or the emulator's default where nothing sets it; dimmed
 * where the layer's tables say unavailable in [state], with the reason
 * under the row when [showReasons] (else the section's footer gives it).
 * [onChange] gets the option and its new .uae value.
 */
@Composable
internal fun SchemaGroupRows(
	options: List<SettingsSchema.Option>,
	settings: EmulatorSettings,
	state: Map<String, String>,
	schema: SettingsSchema,
	showReasons: Boolean,
	onChange: (id: String, value: String) -> Unit
) {
	for (option in options) {
		val id = option.id
		val value = settings.optionValue(id) ?: option.defaultOnAndroid
		val enabled = SettingsControlAvailability.isAvailable(state, id, schema)
		when (SettingsGroups.rowKind(option)) {
			SettingsGroups.RowKind.Switch -> SchemaSwitchRow(
				id = id,
				checked = value.lowercase() in TRUE_VALUES,
				enabled = enabled,
				showReason = showReasons,
				onCheckedChange = { onChange(id, it.toString()) }
			)
			SettingsGroups.RowKind.Choice -> SchemaChoiceRow(
				id = id,
				value = value,
				values = SettingsControlAvailability.choiceValues(state, id, schema),
				enabled = enabled,
				showReason = showReasons,
				onSelect = { onChange(id, it) }
			)
			SettingsGroups.RowKind.Level -> SchemaLevelRow(
				id = id,
				stored = storedNumber(option, value),
				enabled = enabled,
				showReason = showReasons,
				onStoredChange = { onChange(id, numberValue(option, it)) }
			)
			SettingsGroups.RowKind.Number -> SchemaNumberRow(
				id = id,
				stored = storedNumber(option, value),
				enabled = enabled,
				showReason = showReasons,
				onStoredChange = { onChange(id, numberValue(option, it)) }
			)
			null -> Unit
		}
	}
}

/** The spellings cfgfile reads as on. */
private val TRUE_VALUES = setOf("true", "yes", "1")

/** A stored number as the rows edit it (whole numbers: every real option the rows show steps by whole stored units). */
private fun storedNumber(option: SettingsSchema.Option, value: String): Int =
	(value.toDoubleOrNull() ?: option.defaultOnAndroid.toDoubleOrNull() ?: 0.0).roundToInt()

/** A number as the configuration stores it: a real option as cfgfile writes reals ("100.0"). */
private fun numberValue(option: SettingsSchema.Option, stored: Int): String =
	if (option.type == SettingsSchema.TYPE_REAL) stored.toDouble().toString() else stored.toString()
