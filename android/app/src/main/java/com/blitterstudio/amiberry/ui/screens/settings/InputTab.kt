package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.ChoiceItem
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SettingChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.hasTouchScreen
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

internal fun inputDeviceOptionIds(hasTouchScreen: Boolean, portIndex: Int): List<String> = buildList {
	add("none")
	add("mouse")
	add("joy0")
	add("joy1")
	if (hasTouchScreen && portIndex == 1) {
		add("onscreen_joy")
	}
	add("kbd1")
	add("kbd2")
	add("kbd3")
	add("kbd9")
}

/**
 * The analog mouse map feeds the Amiga mouse on port 0, so it needs a physical
 * controller on port 1 and the system mouse kept on port 0. Port 0 can hold
 * either a controller or the mouse, never both — so the map is port-1 only.
 */
internal fun portSupportsMouseMap(portIndex: Int, deviceId: String, port0Device: String): Boolean =
	portIndex == 1 && (deviceId == "joy0" || deviceId == "joy1") && port0Device == "mouse"

@Composable
fun InputTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val hasTouchScreen = hasTouchScreen()

	// The schema has no device list (the layer enumerates the host's devices
	// at run time), so the launcher offers its own, by readable labels.
	val port0Choices = inputDeviceOptionIds(hasTouchScreen, portIndex = 0).map { token ->
		SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
	}
	val port1Choices = inputDeviceOptionIds(hasTouchScreen, portIndex = 1).map { token ->
		SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
	}
	val toggleOption = schemaOption("amiberry.vkbd_toggle")
	val toggleChoices = LauncherChoices.oskToggleButtons.map { (value, label) -> ChoiceItem(value, stringResource(label)) }

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		SettingsSection(title = stringResource(R.string.settings_input_port_assignments_title)) {
			SchemaChoiceRow(
				id = "joyport0",
				value = settings.joyport0,
				choices = port0Choices,
				testTag = "input-port0-dropdown",
				optionTestTag = { "input-port0-option-$it" },
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(joyport0 = value) } }
			)
			SchemaChoiceRow(
				id = "joyport1",
				value = settings.joyport1,
				choices = port1Choices,
				testTag = "input-port1-dropdown",
				optionTestTag = { "input-port1-option-$it" },
				onSelect = { value -> viewModel.updateSettings { s -> InputSettingsActions.selectPort1Device(s, value) } }
			)

			if (portSupportsMouseMap(1, settings.joyport1, settings.joyport0)) {
				SchemaChoiceRow(
					id = "joyport1mousemap",
					value = if (settings.joyport1MouseMap) "1" else "0",
					onSelect = { value ->
						viewModel.updateSettings { s -> InputSettingsActions.setPortMouseMap(s, 1, value == "1") }
					}
				)
				SchemaLevelRow(
					id = "input.joymouse_speed_analog",
					stored = settings.analogMouseSpeed,
					enabled = settings.joyport1MouseMap,
					testTag = "input-analog-mouse-speed-slider",
					onStoredChange = { speed -> viewModel.updateSettings { s -> s.copy(analogMouseSpeed = speed) } }
				)
			}
		}

		// The keyboard can be driven by a game controller, so keep its settings
		// available on non-touch devices as well. The KB touch button and the
		// gamepad toggle are independent ways to open it.
		SettingsSection(title = stringResource(R.string.settings_input_on_screen_controls_title)) {
			if (hasTouchScreen) {
				SchemaSwitchRow(
					id = "amiberry.onscreen_joystick",
					checked = settings.onScreenJoystick,
					onCheckedChange = { isEnabled ->
						viewModel.updateSettings { s -> InputSettingsActions.setOnScreenJoystick(s, isEnabled) }
					}
				)
				SchemaSwitchRow(
					id = "amiberry.vkbd_touch_button",
					checked = settings.onScreenKeyboardButton,
					enabled = settings.onScreenJoystick,
					reason = stringResource(R.string.settings_input_needs_on_screen_joystick),
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(onScreenKeyboardButton = it) }
					}
				)
			}

			SchemaSwitchRow(
				id = "amiberry.vkbd_numpad",
				checked = settings.onScreenKeyboardNumpad,
				enabled = settings.onScreenKeyboardAvailable,
				onCheckedChange = {
					viewModel.updateSettings { s -> s.copy(onScreenKeyboardNumpad = it) }
				}
			)

			if (!hasTouchScreen) {
				Text(
					stringResource(R.string.settings_input_on_screen_keyboard_toggle_hint),
					style = MaterialTheme.typography.bodySmall
				)
			}

			// A hotkey option: the schema has no button list, so the launcher
			// offers the gamepad buttons the toggle takes.
			SettingChoiceRow(
				label = toggleOption?.label ?: stringResource(R.string.settings_input_on_screen_keyboard_toggle),
				valueText = settingText(LauncherChoices.oskToggle(settings.onScreenKeyboardToggle)),
				choices = toggleChoices,
				help = toggleOption?.help,
				testTag = "input-osk-toggle-dropdown",
				optionTestTag = { "input-osk-toggle-option-$it" },
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(onScreenKeyboardToggle = value) } }
			)
		}
	}
}
