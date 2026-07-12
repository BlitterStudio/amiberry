package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
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

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun InputTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val hasTouchScreen = hasTouchScreen()

	val optionLabels = mapOf(
		"none" to stringResource(R.string.settings_option_none),
		"mouse" to stringResource(R.string.settings_input_device_mouse),
		"joy0" to stringResource(R.string.settings_input_device_joystick_0),
		"joy1" to stringResource(R.string.settings_input_device_joystick_1),
		"onscreen_joy" to stringResource(R.string.settings_input_device_on_screen_joystick),
		"kbd1" to stringResource(R.string.settings_input_device_keyboard_layout_1),
		"kbd2" to stringResource(R.string.settings_input_device_keyboard_layout_2),
		"kbd3" to stringResource(R.string.settings_input_device_keyboard_layout_3),
		"kbd9" to stringResource(R.string.settings_input_device_keyboard_layout_4)
	)
	val port0Options = inputDeviceOptionIds(hasTouchScreen, portIndex = 0).map { it to optionLabels.getValue(it) }
	val port1Options = inputDeviceOptionIds(hasTouchScreen, portIndex = 1).map { it to optionLabels.getValue(it) }

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Port assignments
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_input_port_assignments_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				// Port 0
				var port0Expanded by remember { mutableStateOf(false) }
				val port0Label = port0Options.firstOrNull { it.first == settings.joyport0 }?.second
					?: settings.joyport0

				ExposedDropdownMenuBox(
					expanded = port0Expanded,
					onExpandedChange = { port0Expanded = it }
				) {
					OutlinedTextField(
						value = port0Label,
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_input_port0_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = port0Expanded) },
						modifier = Modifier
							.testTag("input-port0-dropdown")
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = port0Expanded,
						onDismissRequest = { port0Expanded = false }
					) {
						port0Options.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								modifier = Modifier.testTag("input-port0-option-$value"),
								onClick = {
									viewModel.updateSettings { s -> s.copy(joyport0 = value) }
									port0Expanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				// Port 1
				var port1Expanded by remember { mutableStateOf(false) }
				val port1Label = port1Options.firstOrNull { it.first == settings.joyport1 }?.second
					?: settings.joyport1

				ExposedDropdownMenuBox(
					expanded = port1Expanded,
					onExpandedChange = { port1Expanded = it }
				) {
					OutlinedTextField(
						value = port1Label,
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_input_port1_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = port1Expanded) },
						modifier = Modifier
							.testTag("input-port1-dropdown")
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = port1Expanded,
						onDismissRequest = { port1Expanded = false }
					) {
						port1Options.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								modifier = Modifier.testTag("input-port1-option-$value"),
								onClick = {
									viewModel.updateSettings { s -> InputSettingsActions.selectPort1Device(s, value) }
									port1Expanded = false
								}
							)
						}
					}
				}
			}
		}

		// The keyboard can be driven by a game controller, so keep its settings
		// available on non-touch devices as well.
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_input_on_screen_controls_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				if (hasTouchScreen) {
					SwitchRow(
						label = stringResource(R.string.settings_input_on_screen_joystick),
						checked = settings.onScreenJoystick,
						onCheckedChange = { isEnabled ->
							viewModel.updateSettings { s -> InputSettingsActions.setOnScreenJoystick(s, isEnabled) }
						}
					)
				}

				SwitchRow(
					label = stringResource(R.string.settings_input_on_screen_keyboard_button),
					checked = settings.onScreenKeyboard,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(onScreenKeyboard = it) }
					}
				)

				SwitchRow(
					label = stringResource(R.string.settings_input_on_screen_keyboard_numpad),
					checked = settings.onScreenKeyboardNumpad,
					enabled = settings.onScreenKeyboard,
					supportingText = stringResource(R.string.settings_input_on_screen_keyboard_numpad_summary),
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(onScreenKeyboardNumpad = it) }
					}
				)
			}
		}
	}
}
