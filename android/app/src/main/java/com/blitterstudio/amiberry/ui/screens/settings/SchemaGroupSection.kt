package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
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
 * The section of a settings layer group: the page's own rows for the group
 * ([leading], when the page has some) and then the options the section
 * takes (SettingsGroups.options), each as its type's row. Options are named
 * in the section's words (SettingsGroups.labelIn); unavailable rows are
 * dimmed, and why is said once in the footer (SettingsGroups.constraintNotes),
 * [footer]'s own text first. A collapsed section (SettingsGroups.byPage)
 * starts closed, and stays open once opened wherever the user goes (the
 * editor's view model remembers). Nothing shows for a section without rows.
 */
@Composable
fun SchemaGroupSection(
	placement: SettingsGroups.Placement,
	viewModel: SettingsViewModel,
	placed: Set<String> = SettingsGroups.placed,
	footer: String? = null,
	leading: (@Composable ColumnScope.() -> Unit)? = null
) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val state = remember(settings, schema) { settings.schemaState(schema) }
	val rows = shownOptions(settings, schema, SettingsGroups.options(schema, placement, placed))
	if (rows.isEmpty() && leading == null) return
	val sectionTitle = stringResource(placement.titleRes)
	val notes = SettingsGroups.constraintNotes(rows) { SettingsControlAvailability.isAvailable(state, it, schema) }
	val sectionFooter = (listOfNotNull(footer) + notes).joinToString("\n").ifBlank { null }
	val content: @Composable ColumnScope.() -> Unit = {
		leading?.invoke(this)
		SchemaGroupRows(rows, settings, state, schema, showReasons = false, sectionTitle = sectionTitle) { id, value ->
			viewModel.updateSettings { s -> s.withOption(id, value) }
		}
	}
	if (placement.collapsed) {
		CollapsibleSettingsSection(
			title = sectionTitle,
			expanded = placement.key in viewModel.expandedSections,
			onExpandedChange = { expanded -> viewModel.setSectionExpanded(placement.key, expanded) },
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
	SchemaGroupRows(
		shownOptions(settings, schema, SettingsGroups.options(schema, group, placed)),
		settings, state, schema, showReasons = true
	) { id, value ->
		viewModel.updateSettings { s -> s.withOption(id, value) }
	}
}

/** [options] without the memory boards that follow boards not in use. */
private fun shownOptions(
	settings: EmulatorSettings,
	schema: SettingsSchema,
	options: List<SettingsSchema.Option>
): List<SettingsSchema.Option> = options.filter { option ->
	SettingsGroups.shows(option.id) { id -> settings.optionValue(id) ?: schema.option(id)?.defaultOnAndroid }
}

/**
 * [options] as rows (SettingsGroups.rowKind) showing their value in
 * [settings], or the emulator's default where nothing sets it; dimmed
 * where the layer's tables say unavailable in [state], with the reason
 * under the row when [showReasons] (else the section's footer gives it).
 * Named in [sectionTitle]'s words (SettingsGroups.labelIn), TalkBack keeps
 * the whole label. [onChange] gets the option and its new .uae value.
 */
@Composable
internal fun SchemaGroupRows(
	options: List<SettingsSchema.Option>,
	settings: EmulatorSettings,
	state: Map<String, String>,
	schema: SettingsSchema,
	showReasons: Boolean,
	sectionTitle: String? = null,
	onChange: (id: String, value: String) -> Unit
) {
	for (option in options) {
		val id = option.id
		val value = settings.optionValue(id) ?: option.defaultOnAndroid
		val enabled = SettingsControlAvailability.isAvailable(state, id, schema)
		val wholeLabel = option.label
		val shownLabel = sectionTitle?.let { SettingsGroups.labelIn(wholeLabel, it) }
		val semanticsLabel = if (shownLabel != null && shownLabel != wholeLabel) wholeLabel else null
		when (SettingsGroups.rowKind(option)) {
			SettingsGroups.RowKind.Switch -> SchemaSwitchRow(
				id = id,
				checked = value.lowercase() in TRUE_VALUES,
				enabled = enabled,
				showReason = showReasons,
				label = shownLabel,
				semanticsLabel = semanticsLabel,
				onCheckedChange = { onChange(id, it.toString()) }
			)
			SettingsGroups.RowKind.Choice -> SchemaChoiceRow(
				id = id,
				value = value,
				values = SettingsControlAvailability.choiceValues(state, id, schema),
				enabled = enabled,
				showReason = showReasons,
				label = shownLabel,
				semanticsLabel = semanticsLabel,
				onSelect = { onChange(id, it) }
			)
			SettingsGroups.RowKind.Level -> SchemaLevelRow(
				id = id,
				stored = storedNumber(option, value),
				enabled = enabled,
				showReason = showReasons,
				label = shownLabel,
				semanticsLabel = semanticsLabel,
				onStoredChange = { onChange(id, numberValue(option, it)) }
			)
			SettingsGroups.RowKind.Number -> SchemaNumberRow(
				id = id,
				stored = storedNumber(option, value),
				enabled = enabled,
				showReason = showReasons,
				label = shownLabel,
				semanticsLabel = semanticsLabel,
				suggestions = SettingsGroups.numberSuggestions(option),
				accepts = SettingsGroups.numberAccepts(option),
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
