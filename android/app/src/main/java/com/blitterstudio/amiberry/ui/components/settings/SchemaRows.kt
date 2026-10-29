package com.blitterstudio.amiberry.ui.components.settings

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.VolumeMute
import androidx.compose.material.icons.automirrored.filled.VolumeUp
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.KeyEventType
import androidx.compose.ui.input.key.key
import androidx.compose.ui.input.key.onPreviewKeyEvent
import androidx.compose.ui.input.key.type
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadSliderKeys
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate
import kotlin.math.abs
import kotlin.math.ceil
import kotlin.math.floor
import kotlin.math.roundToInt

/**
 * The next multiple of [step] above ([up]) or below [current], kept within [range]:
 * a D-pad press on a level slider lands on 5% stops even from an in-between value.
 */
internal fun nextLevelStop(current: Float, step: Float, up: Boolean, range: ClosedFloatingPointRange<Float>): Float {
	val stops = current / step
	val next = (if (up) floor(stops + 1e-4f) + 1 else ceil(stops - 1e-4f) - 1) * step
	return next.coerceIn(range.start, range.endInclusive)
}

/*
 * Rows that take an option id and show what the shared settings layer says
 * about it (settings_schema.json): its label, help, choice labels and unit.
 * While a row is unavailable it is dimmed and shows the reason: the
 * caller's, or else the layer's constraint. Pass showReason = false when
 * the section footer says it once for several rows.
 */

/** The bundled settings schema (loaded by MainActivity; read here if it was not yet). */
@Composable
fun settingsSchema(): SettingsSchema? = SettingsSchema.current ?: SettingsSchema.bundled(LocalContext.current)

/** The schema option [id]; null when the schema is missing or does not list it. */
@Composable
fun schemaOption(id: String): SettingsSchema.Option? = settingsSchema()?.option(id)

/** The layer's label of [id], or the id itself when the schema cannot say. */
@Composable
fun schemaLabel(id: String): String = schemaOption(id)?.label ?: id

/** The text for a stored value of [id]: on/off for switches, choice labels, numbers in their unit. */
@Composable
fun schemaValueText(id: String, value: String): String {
	val on = stringResource(R.string.settings_value_on)
	val off = stringResource(R.string.settings_value_off)
	return schemaOption(id)?.valueText(value, on, off) ?: value
}

private fun rowReason(option: SettingsSchema.Option?, enabled: Boolean, reason: String?, showReason: Boolean): String? =
	if (enabled || !showReason) null else reason ?: option?.constraint

/** A bool option (or a 0/1 option that reads as a switch) as a switch row. */
@Composable
fun SchemaSwitchRow(
	id: String,
	checked: Boolean,
	onCheckedChange: (Boolean) -> Unit,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	reason: String? = null,
	showReason: Boolean = true,
	label: String? = null,
	help: String? = null
) {
	val option = schemaOption(id)
	SettingSwitchRow(
		label = label ?: option?.label ?: id,
		checked = checked,
		onCheckedChange = onCheckedChange,
		modifier = modifier,
		enabled = enabled,
		help = help ?: option?.help,
		reason = rowReason(option, enabled, reason, showReason)
	)
}

/**
 * A choice option as a dropdown of the layer's choice labels. [values]
 * narrows the schema's choices (in the schema's order) to those offered
 * now; [choices] replaces them for options whose list the schema cannot
 * carry (devices). A stored value outside the offered list stays shown by
 * its label. An option that reads as a switch ([SettingsSchema.Option.showsAsSwitch])
 * shows as one, storing "1" and "0".
 */
@Composable
fun SchemaChoiceRow(
	id: String,
	value: String,
	onSelect: (String) -> Unit,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	reason: String? = null,
	showReason: Boolean = true,
	values: Collection<String>? = null,
	choices: List<SettingsSchema.Choice>? = null,
	label: String? = null,
	help: String? = null,
	testTag: String? = null,
	optionTestTag: ((String) -> String)? = null
) {
	val option = schemaOption(id)
	if (choices == null && option != null && option.type == SettingsSchema.TYPE_CHOICE && option.showsAsSwitch(value)) {
		SchemaSwitchRow(
			id = id,
			checked = value == "1",
			onCheckedChange = { onSelect(if (it) "1" else "0") },
			modifier = modifier,
			enabled = enabled,
			reason = reason,
			showReason = showReason,
			label = label,
			help = help
		)
		return
	}
	val all = choices ?: option?.choices.orEmpty()
	val offered = if (values == null) all else all.filter { it.value in values || it.value == value }
	val valueText = offered.firstOrNull { it.value == value }?.label
		?: option?.choiceLabel(value)
		?: value
	SettingChoiceRow(
		label = label ?: option?.label ?: id,
		valueText = valueText,
		choices = offered.map { ChoiceItem(it.value, it.label) },
		onSelect = onSelect,
		modifier = modifier,
		enabled = enabled,
		help = help ?: option?.help,
		reason = rowReason(option, enabled, reason, showReason),
		testTag = testTag,
		optionTestTag = optionTestTag
	)
}

/**
 * The model preset's compatibility level as a dropdown of the layer's level
 * labels (levels 0 to the model's maximum). It reads as the level the
 * settings are at, or as custom once an option a level sets was edited.
 * Nothing shows for a model with one level.
 */
@Composable
fun CompatibilityLevelRow(
	settings: EmulatorSettings,
	onSelect: (Int) -> Unit,
	modifier: Modifier = Modifier
) {
	val model = settings.baseModel
	if (model.compatibilityMax == 0) return
	val labels = settingsSchema()?.compatibilityLevels.orEmpty()
	fun label(level: Int) = labels.getOrNull(level) ?: level.toString()
	val level = settings.compatibilityLevel
	SettingChoiceRow(
		label = stringResource(R.string.settings_compatibility_level),
		valueText = level?.let(::label) ?: stringResource(R.string.settings_compatibility_level_custom),
		choices = (0..model.compatibilityMax).map { ChoiceItem(it, label(it)) },
		onSelect = onSelect,
		modifier = modifier,
		help = stringResource(R.string.settings_compatibility_level_help)
	)
}

/**
 * A number set by feel as a slider, the value with its unit as text and as
 * TalkBack's state ("80%"). An integer option slides over its shown range
 * (a reversed volume reads 100% at full, between speaker icons); a choice
 * option slides over its choices in order, read by their labels. [stops]
 * limits an integer option to those stored values; [valueText] overrides
 * how a stored value reads. [onStoredChange] runs once a drag, key press
 * or TalkBack adjustment ends, not for every position on the way.
 */
@Composable
fun SchemaLevelRow(
	id: String,
	stored: Int,
	onStoredChange: (Int) -> Unit,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	reason: String? = null,
	showReason: Boolean = true,
	label: String? = null,
	help: String? = null,
	stops: List<Int>? = null,
	valueText: ((Int) -> String)? = null,
	testTag: String? = null
) {
	val option = schemaOption(id)
	val title = label ?: option?.label ?: id
	val choiceStops = option?.takeIf { it.type == SettingsSchema.TYPE_CHOICE }
		?.choices?.mapNotNull { it.value.toIntOrNull() }
	val steps = stops ?: choiceStops

	val sliderValue: Float
	val sliderRange: ClosedFloatingPointRange<Float>
	val sliderSteps: Int
	val toStored: (Float) -> Int
	// Shown units per D-pad Left/Right press; null keeps the slider's own key step.
	val keyStep: Float?
	if (steps != null && steps.isNotEmpty()) {
		// Off-list values (hand-edited configurations) sit at the nearest stop.
		sliderValue = steps.indices.minBy { abs(steps[it] - stored) }.toFloat()
		sliderRange = 0f..steps.lastIndex.toFloat()
		sliderSteps = (steps.size - 2).coerceAtLeast(0)
		toStored = { steps[it.roundToInt().coerceIn(0, steps.lastIndex)] }
		keyStep = null
	} else {
		val range = option?.shownRange ?: (0.0..100.0)
		sliderValue = (option?.shown(stored.toDouble()) ?: stored.toDouble()).toFloat()
		sliderRange = range.start.toFloat()..range.endInclusive.toFloat()
		// Fine ranges (0-100%) slide freely and snap to the step when stored.
		sliderSteps = 0
		toStored = { shown -> (option?.stored(shown.toDouble()) ?: shown.toDouble()).roundToInt() }
		// The slider's own key step is 1% of the range: 100 presses across a 0-100% level.
		// Wide ranges step 5 shown units per press; small ranges keep the finer step.
		keyStep = if (range.endInclusive - range.start >= 20.0) 5f else null
	}
	// The thumb follows the drag here; the value is stored once the drag ends,
	// so one drag writes the configuration once instead of at every position.
	var dragPosition by remember { mutableStateOf<Float?>(null) }
	val rtl = LocalLayoutDirection.current == LayoutDirection.Rtl
	val shownStored = dragPosition?.let(toStored) ?: stored
	val text = valueText?.invoke(shownStored)
		?: option?.takeIf { it.type != SettingsSchema.TYPE_BOOL }?.valueText(shownStored.toString(), "", "")
		?: shownStored.toString()
	val reversedVolume = option?.reversed == true

	Column(modifier = modifier.fillMaxWidth()) {
		Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth()) {
			Text(
				text = title,
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurface.copy(alpha = if (enabled) 1f else DISABLED_ALPHA),
				modifier = Modifier.weight(1f)
			)
			Text(
				text = text,
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = if (enabled) 1f else DISABLED_ALPHA)
			)
		}
		Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth()) {
			if (reversedVolume) {
				Icon(
					Icons.AutoMirrored.Filled.VolumeMute,
					contentDescription = null,
					modifier = Modifier.size(20.dp),
					tint = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = if (enabled) 1f else DISABLED_ALPHA)
				)
				Spacer(modifier = Modifier.width(8.dp))
			}
			Slider(
				value = (dragPosition ?: sliderValue).coerceIn(sliderRange.start, sliderRange.endInclusive),
				onValueChange = { position -> dragPosition = position },
				// Called after a drag and after each key or TalkBack step too.
				onValueChangeFinished = {
					val next = dragPosition?.let(toStored)
					dragPosition = null
					if (next != null && next != stored) onStoredChange(next)
				},
				valueRange = sliderRange,
				steps = sliderSteps,
				enabled = enabled,
				modifier = Modifier
					.weight(1f)
					.then(if (testTag != null) Modifier.testTag(testTag) else Modifier)
					.dpadFocusIndicator()
					.dpadSliderKeys()
					.then(
						if (keyStep == null || !enabled) Modifier else Modifier.onPreviewKeyEvent { event ->
							val right = when (event.key) {
								Key.DirectionRight -> true
								Key.DirectionLeft -> false
								else -> return@onPreviewKeyEvent false
							}
							if (event.type == KeyEventType.KeyDown) {
								// The pressed side, mirrored in RTL like the slider itself.
								val up = right != rtl
								val next = nextLevelStop(dragPosition ?: sliderValue, keyStep, up, sliderRange)
								val stepped = toStored(next)
								if (stepped != stored) onStoredChange(stepped)
							}
							true
						}
					)
					.semantics {
						contentDescription = title
						stateDescription = text
					}
			)
			if (reversedVolume) {
				Spacer(modifier = Modifier.width(8.dp))
				Icon(
					Icons.AutoMirrored.Filled.VolumeUp,
					contentDescription = null,
					modifier = Modifier.size(20.dp),
					tint = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = if (enabled) 1f else DISABLED_ALPHA)
				)
			}
		}
		RowFooter(help ?: option?.help, enabled, rowReason(option, enabled, reason, showReason))
	}
}

/**
 * A number people look up as a field with its unit after it ("8192 KB").
 * The typed number is stored as soon as it is one of the option's values
 * (within its range, on its step) that [accepts] allows, so Start or Save
 * tapped while the field still has focus see it. Invalid text shows why
 * (the range, or the layer's constraint for a number in range that [accepts]
 * refuses) and goes back to the value it had when the edit began when the
 * field is left or Done is pressed. [suggestions] are stored values offered in the field's menu,
 * which is also how a D-pad user picks one without typing.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SchemaNumberRow(
	id: String,
	stored: Int,
	onStoredChange: (Int) -> Unit,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	reason: String? = null,
	showReason: Boolean = true,
	label: String? = null,
	help: String? = null,
	suggestions: List<Int> = emptyList(),
	accepts: (Int) -> Boolean = { true }
) {
	val option = schemaOption(id)
	val title = label ?: option?.label ?: id
	fun shownText(value: Int): String =
		option?.let { it.shownText(it.shown(value.toDouble())) } ?: value.toString()
	// The stored number for typed text within the option's range and on its step.
	fun inRange(text: String): Int? {
		val typed = text.trim().toDoubleOrNull() ?: return null
		if (option == null) return typed.roundToInt()
		val number = option.stored(typed)
		val min = option.min ?: Double.NEGATIVE_INFINITY
		val max = option.max ?: Double.POSITIVE_INFINITY
		if (number < min || number > max) return null
		if (abs(option.shown(number) - typed) > 1e-6) return null
		return number.roundToInt()
	}
	fun storedFor(text: String): Int? = inRange(text)?.takeIf(accepts)

	var text by remember { mutableStateOf(shownText(stored)) }
	// Follow a change from elsewhere (a preset, a model switch) unless the
	// text already says it, so typing "0128" is not rewritten under the cursor.
	var syncedStored by remember { mutableStateOf(stored) }
	if (stored != syncedStored) {
		syncedStored = stored
		if (storedFor(text) != stored) text = shownText(stored)
	}
	var expanded by remember { mutableStateOf(false) }
	val typingGate = rememberDpadTypingGate()
	val typedInRange = inRange(text)
	val invalid = typedInRange == null || !accepts(typedInRange)
	// Valid input is stored as it is typed, so deleting "8192" digit by digit stores 819
	// on the way. Text still invalid when the edit ends goes back to the value the field
	// had when the edit began (stored again if a value on the way replaced it).
	var editStart by remember { mutableIntStateOf(stored) }
	fun endEdit() {
		val typed = storedFor(text)
		if (typed == null) {
			text = shownText(editStart)
			if (stored != editStart) onStoredChange(editStart)
		} else {
			editStart = typed
		}
	}
	val range = option?.shownRange
	val rangeText = if (option != null && range != null) {
		stringResource(
			R.string.settings_value_range,
			option.withUnit(option.shownText(range.start)),
			option.withUnit(option.shownText(range.endInclusive))
		)
	} else null
	// In range but refused: give the layer's reason rather than a range the number is within.
	val invalidText = if (typedInRange != null) option?.constraint ?: rangeText else rangeText
	val unit = option?.unit

	Column(modifier = modifier.fillMaxWidth()) {
		val menuShown = expanded && enabled && suggestions.isNotEmpty()
		ExposedDropdownMenuBox(
			expanded = menuShown,
			onExpandedChange = { if (enabled) expanded = it }
		) {
			OutlinedTextField(
				value = text,
				onValueChange = { typed ->
					text = typed
					storedFor(typed)?.let { next -> if (next != stored) onStoredChange(next) }
				},
				enabled = enabled,
				readOnly = typingGate.readOnly,
				singleLine = true,
				isError = invalid,
				label = { Text(title) },
				suffix = if (unit != null) {
					{ Text(unit) }
				} else null,
				supportingText = if (invalid && invalidText != null) {
					{ Text(invalidText) }
				} else null,
				trailingIcon = if (suggestions.isNotEmpty()) {
					{
						ExposedDropdownMenuDefaults.TrailingIcon(
							expanded = expanded,
							modifier = Modifier.menuAnchor(ExposedDropdownMenuAnchorType.SecondaryEditable, enabled)
						)
					}
				} else null,
				keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal, imeAction = ImeAction.Done),
				keyboardActions = KeyboardActions(onDone = {
					endEdit()
					typingGate.stopTyping()
				}),
				modifier = Modifier
					// While the suggestions are open, Up/Down belong to the menu: the anchor
					// makes it focusable on them, so the D-pad can reach the suggestions.
					.then(if (menuShown) Modifier else Modifier.dpadVerticalMovesFocus())
					.then(typingGate.modifier)
					.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryEditable, enabled)
					.onFocusChanged { if (it.isFocused) editStart = stored else endEdit() }
					.fillMaxWidth()
			)
			if (suggestions.isNotEmpty()) {
				ExposedDropdownMenu(
					expanded = expanded && enabled,
					onDismissRequest = { expanded = false }
				) {
					suggestions.forEach { value ->
						DropdownMenuItem(
							text = { Text(option?.numberText(value.toString()) ?: value.toString()) },
							onClick = {
								text = shownText(value)
								// A picked value is a finished edit; later typing starts from it.
								editStart = value
								if (value != stored) onStoredChange(value)
								expanded = false
							}
						)
					}
				}
			}
		}
		RowFooter(help ?: option?.help, enabled, rowReason(option, enabled, reason, showReason))
	}
}
