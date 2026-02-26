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
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SoundTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	val freqOptions = listOf(
		22050 to "22050 Hz",
		44100 to "44100 Hz",
		48000 to "48000 Hz"
	)

	val channelOptions = listOf(
		"mono" to "Mono",
		"stereo" to "Stereo",
		"mixed" to "Mixed (75% stereo)"
	)

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(16.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("Sound Settings", style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				// Sound output mode
				var outputExpanded by remember { mutableStateOf(false) }
				val outputLabel = EmulatorSettings.soundOutputOptions
					.firstOrNull { it.first == settings.soundOutput }?.second ?: settings.soundOutput

				ExposedDropdownMenuBox(
					expanded = outputExpanded,
					onExpandedChange = { outputExpanded = it }
				) {
					OutlinedTextField(
						value = outputLabel,
						onValueChange = {},
						readOnly = true,
						label = { Text("Sound Output") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = outputExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = outputExpanded,
						onDismissRequest = { outputExpanded = false }
					) {
						EmulatorSettings.soundOutputOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								onClick = {
									viewModel.updateSettings { s -> s.copy(soundOutput = value) }
									outputExpanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				// Frequency
				var freqExpanded by remember { mutableStateOf(false) }
				val freqLabel = freqOptions.firstOrNull { it.first == settings.soundFreq }?.second
					?: "${settings.soundFreq} Hz"

				ExposedDropdownMenuBox(
					expanded = freqExpanded,
					onExpandedChange = { freqExpanded = it }
				) {
					OutlinedTextField(
						value = freqLabel,
						onValueChange = {},
						readOnly = true,
						label = { Text("Sample Rate") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = freqExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = freqExpanded,
						onDismissRequest = { freqExpanded = false }
					) {
						freqOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								onClick = {
									viewModel.updateSettings { s -> s.copy(soundFreq = value) }
									freqExpanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				// Channels
				var channelExpanded by remember { mutableStateOf(false) }
				val channelLabel = channelOptions.firstOrNull { it.first == settings.soundChannels }?.second
					?: settings.soundChannels

				ExposedDropdownMenuBox(
					expanded = channelExpanded,
					onExpandedChange = { channelExpanded = it }
				) {
					OutlinedTextField(
						value = channelLabel,
						onValueChange = {},
						readOnly = true,
						label = { Text("Stereo Mode") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = channelExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = channelExpanded,
						onDismissRequest = { channelExpanded = false }
					) {
						channelOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								onClick = {
									viewModel.updateSettings { s -> s.copy(soundChannels = value) }
									channelExpanded = false
								}
							)
						}
					}
				}
			}
		}
	}
}
