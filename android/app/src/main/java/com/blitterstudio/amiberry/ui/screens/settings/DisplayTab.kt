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
fun DisplayTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	val resolutionPresets = listOf(
		Triple(640, 256, "640x256 (PAL Low-res)"),
		Triple(640, 512, "640x512 (PAL Hi-res)"),
		Triple(720, 568, "720x568 (Default)"),
		Triple(800, 600, "800x600"),
		Triple(1024, 768, "1024x768")
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
				Text("Display Settings", style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				// Resolution preset
				var resExpanded by remember { mutableStateOf(false) }
				val currentRes = "${settings.gfxWidth}x${settings.gfxHeight}"
				val currentLabel = resolutionPresets.firstOrNull {
					it.first == settings.gfxWidth && it.second == settings.gfxHeight
				}?.third ?: currentRes

				ExposedDropdownMenuBox(
					expanded = resExpanded,
					onExpandedChange = { resExpanded = it }
				) {
					OutlinedTextField(
						value = currentLabel,
						onValueChange = {},
						readOnly = true,
						label = { Text("Resolution") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = resExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = resExpanded,
						onDismissRequest = { resExpanded = false }
					) {
						resolutionPresets.forEach { (w, h, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								onClick = {
									viewModel.updateSettings { s ->
										s.copy(gfxWidth = w, gfxHeight = h)
									}
									resExpanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				SwitchRow(
					label = "Correct Aspect Ratio",
					checked = settings.correctAspect,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(correctAspect = it) }
					}
				)

				SwitchRow(
					label = "Auto-Crop",
					checked = settings.autoCrop,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(autoCrop = it) }
					}
				)
			}
		}
	}
}
