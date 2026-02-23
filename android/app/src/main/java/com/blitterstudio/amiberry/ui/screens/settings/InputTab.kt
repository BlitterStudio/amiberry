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
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MenuAnchorType
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun InputTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	val portOptions = listOf(
		"mouse" to "Mouse",
		"joy0" to "Joystick 0",
		"joy1" to "Joystick 1",
		"kbd1" to "Keyboard Layout 1",
		"kbd2" to "Keyboard Layout 2",
		"kbd3" to "Keyboard Layout 3"
	)

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(16.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Port assignments
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("Port Assignments", style = MaterialTheme.typography.titleMedium)
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
						label = { Text("Port 0 (Mouse Port)") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = port0Expanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
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
						label = { Text("Port 1 (Joystick Port)") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = port1Expanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
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
									viewModel.updateSettings { s -> s.copy(joyport1 = value) }
									port1Expanded = false
								}
							)
						}
					}
				}
			}
		}

		// On-screen controls
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("On-Screen Controls", style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				SwitchRow(
					label = "On-Screen Joystick",
					checked = settings.onScreenJoystick,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(onScreenJoystick = it) }
					}
				)

				SwitchRow(
					label = "On-Screen Keyboard Button",
					checked = settings.onScreenKeyboard,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(onScreenKeyboard = it) }
					}
				)
			}
		}
	}
}
