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
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.ui.hasTouchScreen
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun InputTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	val portOptions = buildList {
		add("mouse" to stringResource(R.string.settings_input_device_mouse))
		add("joy0" to stringResource(R.string.settings_input_device_joystick_0))
		add("joy1" to stringResource(R.string.settings_input_device_joystick_1))
		if (hasTouchScreen()) {
			add("onscreen_joy" to stringResource(R.string.settings_input_device_on_screen_joystick))
		}
		add("kbd1" to stringResource(R.string.settings_input_device_keyboard_layout_1))
		add("kbd2" to stringResource(R.string.settings_input_device_keyboard_layout_2))
		add("kbd3" to stringResource(R.string.settings_input_device_keyboard_layout_3))
	}

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
				val port0Label = portOptions.firstOrNull { it.first == settings.joyport0 }?.second
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
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = port0Expanded,
						onDismissRequest = { port0Expanded = false }
					) {
						portOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
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
				val port1Label = portOptions.firstOrNull { it.first == settings.joyport1 }?.second
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
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = port1Expanded,
						onDismissRequest = { port1Expanded = false }
					) {
						portOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								onClick = {
									viewModel.updateSettings { s ->
										if (value == "onscreen_joy") {
											s.copy(joyport1 = value, onScreenJoystick = true)
										} else {
											s.copy(joyport1 = value)
										}
									}
									port1Expanded = false
								}
							)
						}
					}
				}
			}
		}

		// On-screen controls (only shown on devices with a touchscreen)
		if (hasTouchScreen()) {
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(stringResource(R.string.settings_input_on_screen_controls_title), style = MaterialTheme.typography.titleMedium)
					Spacer(modifier = Modifier.height(8.dp))

					SwitchRow(
						label = stringResource(R.string.settings_input_on_screen_joystick),
						checked = settings.onScreenJoystick,
						onCheckedChange = { isEnabled ->
							viewModel.updateSettings { s ->
								if (isEnabled) {
									s.copy(onScreenJoystick = true, joyport1 = "onscreen_joy")
								} else {
									if (s.joyport1 == "onscreen_joy") {
										// Revert to first joystick available (joy0 or joy1, usually joy1 for port 1)
										s.copy(onScreenJoystick = false, joyport1 = "joy1")
									} else {
										s.copy(onScreenJoystick = false)
									}
								}
							}
						}
					)

					SwitchRow(
						label = stringResource(R.string.settings_input_on_screen_keyboard_button),
						checked = settings.onScreenKeyboard,
						onCheckedChange = {
							viewModel.updateSettings { s -> s.copy(onScreenKeyboard = it) }
						}
					)
				}
			}
		}
	}
}
