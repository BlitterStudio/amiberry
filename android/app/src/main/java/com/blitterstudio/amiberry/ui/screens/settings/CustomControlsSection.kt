package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.CustomControls
import com.blitterstudio.amiberry.ui.components.settings.ChoiceItem
import com.blitterstudio.amiberry.ui.components.settings.SettingChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/**
 * The Input page's Custom controls section, after the Apple app's
 * CustomControlsSections: what each button and axis of the joystick on a
 * port triggers, alone or with the device's hotkey held. The mappings are
 * the configuration's own lines (CustomControls' codec); the events are
 * the schema's input_events. A port without a joystick (the launcher's
 * device list knows it) has nothing to map.
 */
@Composable
fun CustomControlsSection(viewModel: SettingsViewModel) {
	val schema = settingsSchema() ?: return
	if (schema.customControls.isEmpty()) return
	val settings = viewModel.settings
	var port by remember { mutableStateOf(1) }
	var hotkey by remember { mutableStateOf(false) }

	val portName = settings.joyportOf(port)
	SettingsSection(
		title = stringResource(R.string.settings_input_custom_controls_title),
		footer = stringResource(R.string.settings_input_custom_controls_footer)
	) {
		SettingChoiceRow(
			label = stringResource(R.string.settings_input_custom_controls_port),
			valueText = portTitle(port),
			choices = (0..3).map { ChoiceItem(it.toString(), portTitle(it)) },
			onSelect = { selected -> port = selected.toIntOrNull() ?: port }
		)
		SettingChoiceRow(
			label = stringResource(R.string.settings_input_custom_controls_mapping),
			valueText = stringResource(
				if (hotkey) R.string.settings_input_custom_controls_hotkey
				else R.string.settings_input_custom_controls_alone
			),
			choices = listOf(
				ChoiceItem("false", stringResource(R.string.settings_input_custom_controls_alone)),
				ChoiceItem("true", stringResource(R.string.settings_input_custom_controls_hotkey))
			),
			onSelect = { selected -> hotkey = selected == "true" }
		)
		if (portName !in JOYSTICK_DEVICES) {
			Text(
				stringResource(R.string.settings_input_custom_controls_none_connected),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
		} else {
			Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
				for (control in schema.customControls) {
					val key = CustomControls.Key(port, hotkey, control.axis, control.id)
					val event = settings.customControls.event(key)
					CustomControlRow(key, control, event) { value ->
						viewModel.updateSettings { s ->
							s.copy(customControls = s.customControls.with(key, value))
						}
					}
				}
			}
		}
	}
}

/** One control: Default, None, or an input event. */
@Composable
private fun CustomControlRow(
	key: CustomControls.Key,
	control: com.blitterstudio.amiberry.data.model.SettingsSchema.CustomControl,
	event: String,
	onSelect: (String) -> Unit
) {
	val schema = settingsSchema() ?: return
	val defaultLabel = stringResource(R.string.settings_input_custom_controls_value_default)
	val noneLabel = stringResource(R.string.settings_input_custom_controls_value_none)
	val choices = listOf(ChoiceItem("", defaultLabel), ChoiceItem("none", noneLabel)) +
		schema.inputEvents.map { ChoiceItem(it.value, it.value) }
	// An event the schema does not list (an older file) stays offered.
	val offered = if (event.isNotEmpty() && event != "none" && choices.none { it.value == event })
		choices + ChoiceItem(event, event) else choices
	SettingChoiceRow(
		label = control.label,
		valueText = when (event) {
			"" -> defaultLabel
			"none" -> noneLabel
			else -> schema.inputEvents.firstOrNull { it.value == event }?.label ?: event
		},
		choices = offered,
		onSelect = onSelect,
		modifier = Modifier.fillMaxWidth()
	)
}

/** The port names, as the Apple editor labels them. */
@Composable
private fun portTitle(port: Int): String = stringResource(
	when (port) {
		0 -> R.string.settings_input_custom_controls_port_0
		1 -> R.string.settings_input_custom_controls_port_1
		2 -> R.string.settings_input_custom_controls_port_2
		else -> R.string.settings_input_custom_controls_port_3
	}
)

/**
 * The devices whose ports a joystick can hold (the launcher's device
 * tokens): the on-screen joystick registers as one, so its overlay takes
 * the mappings too.
 */
private val JOYSTICK_DEVICES = setOf("joy0", "joy1", "onscreen_joy")

/** The device on [port], with the on-screen joystick as port 1's fallback (its overlay is the joystick). */
private fun com.blitterstudio.amiberry.data.model.EmulatorSettings.joyportOf(port: Int): String = when (port) {
	0 -> joyport0
	1 -> joyport1
	else -> "none"
}
