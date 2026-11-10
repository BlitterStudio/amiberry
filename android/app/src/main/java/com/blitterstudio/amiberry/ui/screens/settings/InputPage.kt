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
import androidx.compose.runtime.getValue
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.ui.components.settings.ChoiceItem
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

/**
 * The Input page: the joyports, the layer's input and on-screen rows, and
 * the launcher's own control settings (the in-game menu's).
 */
@Composable
fun InputPage(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val toggleOption = schemaOption("amiberry.vkbd_toggle")
	val toggleChoices = LauncherChoices.oskToggleButtons.map { (value, label) -> ChoiceItem(value, stringResource(label)) }

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Every option's row comes from the registry (SettingsOptionRow), the
		// same rows All Settings shows; the registry hides the ones this device
		// cannot use.
		SettingsSection(title = stringResource(R.string.settings_input_port_assignments_title)) {
			SettingsOptionRow("joyport0", viewModel)
			SettingsOptionRow("joyport1", viewModel)
			SettingsOptionRow("joyport1mousemap", viewModel)
			SettingsOptionRow("input.joymouse_speed_analog", viewModel)
		}

		// Not "Input" on the Input page: the section holds autofire, port modes and mouse options.
		SchemaGroupSection("input", viewModel, title = stringResource(R.string.settings_input_controller_options_title))

		// The keyboard can be driven by a game controller, so its settings stay
		// on non-touch devices too. The registry rows hide the touch-only ones;
		// the gamepad toggle is a hotkey option the schema has no list for.
		SchemaGroupSection("onscreen", viewModel, title = stringResource(R.string.settings_input_on_screen_controls_title)) {
			SettingsOptionRow("amiberry.onscreen_joystick", viewModel)
			SettingsOptionRow("amiberry.vkbd_touch_button", viewModel)
			SettingsOptionRow("amiberry.vkbd_numpad", viewModel)

			if (!hasTouchScreen()) {
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
