package com.blitterstudio.amiberry.ui.components.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.selection.toggleable
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus

/** The alpha Material uses for disabled content. */
internal const val DISABLED_ALPHA = 0.38f

/**
 * A settings card: a [title], its rows, and an optional [footer] that says
 * once why some of the rows are unavailable (a reason several rows share).
 */
@Composable
fun SettingsSection(
	title: String,
	modifier: Modifier = Modifier,
	footer: String? = null,
	content: @Composable ColumnScope.() -> Unit
) {
	OutlinedCard(modifier = modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Text(title, style = MaterialTheme.typography.titleMedium)
			Spacer(modifier = Modifier.height(8.dp))
			Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
				content()
			}
			if (!footer.isNullOrBlank()) {
				Spacer(modifier = Modifier.height(8.dp))
				ReasonText(footer)
			}
		}
	}
}

/** Help under a row, dimmed with the row when it is unavailable. */
@Composable
internal fun SupportingText(text: String, enabled: Boolean = true) {
	Text(
		text = text,
		style = MaterialTheme.typography.bodySmall,
		color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = if (enabled) 0.72f else DISABLED_ALPHA)
	)
}

/** Why a control is unavailable, shown once under it or under its section. */
@Composable
internal fun ReasonText(text: String) {
	Text(
		text = text,
		style = MaterialTheme.typography.bodySmall,
		color = MaterialTheme.colorScheme.onSurfaceVariant
	)
}

/** The help and, while the row is unavailable, the reason, under a row. */
@Composable
internal fun RowFooter(help: String?, enabled: Boolean, reason: String?) {
	if (!help.isNullOrBlank()) SupportingText(help, enabled)
	if (!enabled && !reason.isNullOrBlank()) ReasonText(reason)
}

/**
 * An on/off row: the whole row toggles (one D-pad stop), the [help] and,
 * while it is unavailable, the [reason] under it.
 */
@Composable
fun SettingSwitchRow(
	label: String,
	checked: Boolean,
	onCheckedChange: (Boolean) -> Unit,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	help: String? = null,
	reason: String? = null
) {
	Column(
		modifier = modifier
			.fillMaxWidth()
			.toggleable(
				value = checked,
				enabled = enabled,
				role = Role.Switch,
				onValueChange = onCheckedChange
			)
			.dpadFocusIndicator()
			.padding(vertical = 4.dp)
	) {
		Row(
			verticalAlignment = Alignment.CenterVertically,
			horizontalArrangement = Arrangement.SpaceBetween,
			modifier = Modifier.fillMaxWidth()
		) {
			Text(
				text = label,
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurface.copy(alpha = if (enabled) 1f else DISABLED_ALPHA),
				modifier = Modifier.weight(1f)
			)
			Spacer(modifier = Modifier.width(12.dp))
			Switch(
				checked = checked,
				onCheckedChange = null,
				enabled = enabled
			)
		}
		RowFooter(help, enabled, reason)
	}
}

/** One entry of a [SettingChoiceRow]: the stored [value] and what people read. */
data class ChoiceItem<T>(val value: T, val label: String, val enabled: Boolean = true)

/**
 * A pick-one row as a read-only dropdown. The current value reads as
 * [valueText]; [help] and, while unavailable, [reason] show under it.
 * D-pad Up/Down move to the neighbouring control rather than being
 * swallowed by the text field ([dpadVerticalMovesFocus] before menuAnchor).
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun <T> SettingChoiceRow(
	label: String,
	valueText: String,
	choices: List<ChoiceItem<T>>,
	onSelect: (T) -> Unit,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	help: String? = null,
	reason: String? = null,
	testTag: String? = null,
	optionTestTag: ((T) -> String)? = null
) {
	var expanded by remember { mutableStateOf(false) }
	Column(modifier = modifier.fillMaxWidth()) {
		ExposedDropdownMenuBox(
			expanded = expanded && enabled,
			onExpandedChange = { if (enabled) expanded = it }
		) {
			OutlinedTextField(
				value = valueText,
				onValueChange = {},
				readOnly = true,
				enabled = enabled,
				label = { Text(label) },
				trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded && enabled) },
				modifier = Modifier
					.then(if (testTag != null) Modifier.testTag(testTag) else Modifier)
					.dpadVerticalMovesFocus()
					.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable, enabled)
					.fillMaxWidth()
			)
			ExposedDropdownMenu(
				expanded = expanded && enabled,
				onDismissRequest = { expanded = false }
			) {
				choices.forEach { choice ->
					DropdownMenuItem(
						text = { Text(choice.label) },
						enabled = choice.enabled,
						modifier = optionTestTag?.let { Modifier.testTag(it(choice.value)) } ?: Modifier,
						onClick = {
							onSelect(choice.value)
							expanded = false
						}
					)
				}
			}
		}
		RowFooter(help, enabled, reason)
	}
}
